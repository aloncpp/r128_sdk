#include <stdlib.h>
#include <string.h>

#include "hal_hci.h"
#include "hal_controller.h"
#include "xrbtc.h"
#include "hci_list.h"
#include "kernel/os/os_thread.h"
#include "kernel/os/os.h"
#ifdef CONFIG_COMPONENTS_BT_PM
#include "bt_pm.h"
#endif

#define VHCI_SUCCESS 0
#define VHCI_FAILED -1

#define RX_SEM_MAX 1
#define TX_SEM_MAX 1
#define RX_SEM_INIT_VAL 0
#define TX_SEM_INIT_VAL 0

#define HCI_H4_NONE 0x00
#define HCI_H4_CMD  0x01
#define HCI_H4_ACL  0x02
#define HCI_H4_SCO  0x03
#define HCI_H4_EVT  0x04

#define HCI_RX_THREAD_PROI       (XR_OS_PRIORITY_HIGH)
#define HCI_RX_THREAD_STACK_SIZE (4096 + 512)

#define FIRST_PKT  1
static int vhci_h2c_cb(uint8_t status, const uint8_t *buff,
                           uint32_t offset, uint32_t len);
static int vhci_c2h(uint8_t hci_type, const uint8_t *buff,
               uint32_t offset, uint32_t len);

static XR_OS_Thread_t rx_thread;

static hal_hci_callbacks_t vhci_drv;
static hci_list_t *ptr_pkt = NULL;

static XR_OS_Semaphore_t tx_sem;
static XR_OS_Semaphore_t rx_sem;
static XR_OS_Mutex_t rx_mutex;

static int vhci_h2c_cb(unsigned char status, const unsigned char *buff,
                           unsigned int offset, unsigned int len)
{
    XR_OS_SemaphoreRelease(&tx_sem);

    return 0;
}

static int vhci_c2h(unsigned char hci_type, const unsigned char *buff,
               unsigned int offset, unsigned int len)
{
    hci_pkt_t *hci_pkt = NULL;

    if ((hci_type != HCI_H4_ACL) && (hci_type != HCI_H4_EVT) && (hci_type != HCI_H4_SCO)) {
        printf("hci c2h hci type errr: %d\n", hci_type);
        goto fail;
    }

    hci_pkt = (hci_pkt_t *)malloc(sizeof(hci_pkt_t));
    if (!hci_pkt) {
        printf("vhci_c2h hci_pkt malloc fail\n");
        goto fail;
    }
    hci_pkt->buff = (uint8_t *)malloc(len + 1);
    if (!hci_pkt->buff) {
        printf("vhci_c2h hci_pkt->buff malloc fail, %d\n", len + 1);
        free(hci_pkt);
        goto fail;
    }
    hci_pkt->buff[0] = hci_type;
    memcpy(hci_pkt->buff + 1, buff + offset, len);

    hci_pkt->len = len + 1;

    xrbtc_hci_c2h_cb(0, buff, offset, len);

    XR_OS_MutexLock(&rx_mutex, XR_OS_WAIT_FOREVER);
    hci_list_append(ptr_pkt, (void *)hci_pkt);
    XR_OS_MutexUnlock(&rx_mutex);

    XR_OS_SemaphoreRelease(&rx_sem);

    return 0;

fail:
    xrbtc_hci_c2h_cb(0, buff, offset, len);
    return -1;
}

static void rx_thread_handle(void *arg)
{
    uint32_t length = 0;

    while(1) {
        XR_OS_SemaphoreWait(&rx_sem, XR_OS_WAIT_FOREVER);
#ifdef CONFIG_COMPONENTS_BT_PM
        bt_pm_lock();
#endif
        XR_OS_MutexLock(&rx_mutex, XR_OS_WAIT_FOREVER);
        length = hci_list_length(ptr_pkt);
        XR_OS_MutexUnlock(&rx_mutex);

        while (length > 0) {
            vhci_drv.data_ind(ptr_pkt->head->data.pkt->buff, ptr_pkt->head->data.pkt->len);
            XR_OS_MutexLock(&rx_mutex, XR_OS_WAIT_FOREVER);
            hci_list_remove_node(ptr_pkt, FIRST_PKT);
            length = hci_list_length(ptr_pkt);
            XR_OS_MutexUnlock(&rx_mutex);
        }
#ifdef CONFIG_COMPONENTS_BT_PM
        bt_pm_unlock();
#endif

    }
}

static int drv_init()
{
    int ret = 0;
    if (!XR_OS_SemaphoreIsValid(&tx_sem))
        ret = XR_OS_SemaphoreCreate(&tx_sem, TX_SEM_INIT_VAL, TX_SEM_MAX);
    if (ret) {
        printf("tx sem create failed\n");
        return VHCI_FAILED;
    }

    if (!XR_OS_SemaphoreIsValid(&rx_sem))
        ret = XR_OS_SemaphoreCreate(&rx_sem, RX_SEM_INIT_VAL, RX_SEM_MAX);
    if (ret) {
        printf("rx sem create failed\n");
        goto fail_rx_sem;
    }

    ret = XR_OS_MutexCreate(&rx_mutex);
    if (ret) {
        printf("rx mutex create failed\n");
        goto fail_mutex;
    }

    memset(&rx_thread, 0, sizeof(rx_thread));
    if (ret = XR_OS_ThreadCreate(&rx_thread, "rx_thread", rx_thread_handle, NULL, HCI_RX_THREAD_PROI, HCI_RX_THREAD_STACK_SIZE) != XR_OS_OK) {
        printf("uart_rx_thread create failed\n");
        goto fail_thread;
    }

    ptr_pkt = hci_list_new(HCI_PKT_TYPE);
    if(NULL == ptr_pkt) {
        printf("list create failed\n");
        goto fail_list;
    }

    return VHCI_SUCCESS;

fail_list:
    XR_OS_ThreadDelete(&rx_thread);

fail_thread:
    XR_OS_MutexDelete(&rx_mutex);

fail_mutex:
    XR_OS_SemaphoreDelete(&rx_sem);

fail_rx_sem:
    XR_OS_SemaphoreDelete(&tx_sem);

    return VHCI_FAILED;
}

static int drv_deinit()
{
    int ret = XR_OS_SemaphoreDelete(&tx_sem);
    if (ret) {
        printf("tx sem delete failed\n");
    }

    ret = XR_OS_SemaphoreDelete(&rx_sem);
    if (ret) {
        printf("rx sem delete failed\n");
    }

    ret = XR_OS_ThreadDelete(&rx_thread);
    if (ret) {
        printf("thread delete failed\n");
    }

    ret = XR_OS_MutexDelete(&rx_mutex);
    if (ret) {
        printf("rx mutex delete failed\n");
    }

    hci_list_free(ptr_pkt);

    return ret;
}

int hal_hci_open(void *hcb)
{
    int ret;
    hal_hci_callbacks_t *hcb_ptr = (hal_hci_callbacks_t *)hcb;
    if (hcb_ptr->data_ind == NULL)
        return VHCI_FAILED;

    vhci_drv.data_ind = hcb_ptr->data_ind;
    xrbtc_hci_init(vhci_c2h, vhci_h2c_cb);

    ret = drv_init();

    return ret;
}

int hal_hci_close(void)
{
    vhci_drv.data_ind = NULL;
    return drv_deinit();
}


int hal_hci_write(uint8_t *data, uint16_t len)
{
    int ret = xrbtc_hci_h2c(data[0], data + 1, 0, len - 1);
    if (XR_OS_SemaphoreWait(&tx_sem, 5000) != XR_OS_OK) {
        printf("h2c cb timeout\n");
    }

    return ret;
}
