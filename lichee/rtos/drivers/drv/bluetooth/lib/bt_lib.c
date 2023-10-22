/*
 * Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of XRADIO TECHNOLOGY CO., LTD. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <string.h>
#include "kernel/os/os.h"
#include "bt_lib.h"
#include "errno.h"

#ifdef CONFIG_BT_DIST_MODE_CORRESPOND
#include "hci_distribute.h"
#endif

#ifdef CONFIG_BT_DRIVERS_LOG_HCIDUMP
#include "hcidump_xr.h"
#endif

/* define traces for BTLIB */
#define BT_LIB_TRACE_LEVEL_ERROR   (0)
#define BT_LIB_TRACE_LEVEL_WARNING (1)
#define BT_LIB_TRACE_LEVEL_DEBUG   (2)

#if defined(CONFIG_BT_DRIVERS_LIB_LOG_LEVEL)
#define BT_LIB_INITIAL_TRACE_LEVEL CONFIG_BT_DRIVERS_LIB_LOG_LEVEL
#else
#define BT_LIB_INITIAL_TRACE_LEVEL BT_LIB_TRACE_LEVEL_ERROR
#endif

#define BT_LIB_TRACE_ERROR(fmt, args...)       {if (BT_LIB_INITIAL_TRACE_LEVEL >= BT_LIB_TRACE_LEVEL_ERROR)   printf("<BT_HCI_E>" fmt, ##args);}
#define BT_LIB_TRACE_WARNING(fmt, args...)     {if (BT_LIB_INITIAL_TRACE_LEVEL >= BT_LIB_TRACE_LEVEL_WARNING) printf("<BT_HCI_W>" fmt, ##args);}
#define BT_LIB_TRACE_DEBUG(fmt, args...)       {if (BT_LIB_INITIAL_TRACE_LEVEL >= BT_LIB_TRACE_LEVEL_DEBUG)   printf("<BT_HCI_D>" fmt, ##args);}
#define BT_LIB_TRACE_DEBUG_LOC()               {if (BT_LIB_INITIAL_TRACE_LEVEL >= BT_LIB_TRACE_LEVEL_DEBUG)   printf("<BT_HCI_D_LOC>%s %d\n", __func__, __LINE__);}

#define BT_HCI_ID_MAX (2)

/*define lib init flag*/
static uint8_t lib_init_dev_num = 0;
#define LIB_NO_DEV_REGISTER      (0)
#define LIB_ONE_DEV_REGISTER     (1)

typedef struct bt_hc_cb {
	uint8_t id;
	uint8_t features;
	uint8_t in_use;
	bt_hc_callbacks_t cbacks;
} bt_hc_cb_t;

typedef struct bt_lib_pcb {
	uint8_t state;
	uint8_t id_index;
	uint8_t id_cnt;
	bt_hc_cb_t hcbs[BT_HCI_ID_MAX];
} bt_lib_pcb_t;

static bt_lib_pcb_t tLibPCB;
static XR_OS_Mutex_t hci_mutex;
static XR_OS_Mutex_t tx_mutex;

static int hci_recv_cb(const uint8_t *buff, uint16_t len)
{
	bt_hc_cb_t *hcb;
	bt_lib_pcb_t *pcb = &tLibPCB;

	uint8_t *origin_buff = (uint8_t *)buff;

#if defined(CONFIG_BT_DRIVERS_LOG_BTSNOOP)
	void btsnoop_capture(uint8_t type, const uint8_t *buf, bool is_received);
	btsnoop_capture(buff[0], buff + 1, 1);
#endif
#if defined(CONFIG_BT_DRIVERS_LOG_HCIDUMP)
	hcidump_process(buff[0], buff + 1, 1);
#endif

	XR_OS_MutexLock(&hci_mutex, XR_OS_WAIT_FOREVER);
#if defined (CONFIG_BT_DIST_MODE_ALL)
	for (int i = 0; i < BT_HCI_ID_MAX; i++) {
		hcb = &pcb->hcbs[i];
		if (hcb->in_use && hcb->cbacks.data_ind) {
			hcb->cbacks.data_ind(origin_buff, len);
		}
	}
#elif defined(CONFIG_BT_DIST_MODE_CORRESPOND)
	int ret = BT_FEATURES_DUAL;
	if (pcb->hcbs[0].in_use == 1 && pcb->hcbs[1].in_use == 1) {
		ret = hci_distribute(buff);
		BT_LIB_TRACE_DEBUG("ret is %d\n", ret);
#if defined(CONFIG_BT_DIST_MODE_CORRESPOND_DEBUG)
		hci_recive_record(ret);
#endif
	}
	for (int i = 0; i < BT_HCI_ID_MAX; i++) {
		hcb = &pcb->hcbs[i];
		if (hcb->in_use && hcb->cbacks.data_ind && (hcb->features & ret)) {
			hcb->cbacks.data_ind(origin_buff, len);
		}
	}
#endif
	XR_OS_MutexUnlock(&hci_mutex);

	return 0;
}

static int hci_open(uint8_t features, bt_hc_callbacks_t* p_cb)
{
	bt_hc_callbacks_t bt_hc_callbacks;
	bt_hc_callbacks.data_ind = &hci_recv_cb;

	bt_hc_cb_t *hcb;

	if (!XR_OS_MutexIsValid(&tx_mutex))
		XR_OS_MutexCreate(&tx_mutex);

	if (!XR_OS_MutexIsValid(&hci_mutex))
		XR_OS_MutexCreate(&hci_mutex);

	XR_OS_MutexLock(&hci_mutex, XR_OS_WAIT_FOREVER);

	bt_lib_pcb_t *pcb = &tLibPCB;
	int id = 0;

	if (pcb->id_cnt >= BT_HCI_ID_MAX) {
		BT_LIB_TRACE_ERROR("ID exceeds limit\n");
		XR_OS_MutexUnlock(&hci_mutex);
		return -1;
	}

	id = pcb->id_index & (BT_HCI_ID_MAX - 1);
	hcb = &pcb->hcbs[id];
	if (hcb->id == id && hcb->in_use) {
		BT_LIB_TRACE_ERROR("ID exist already\n");
		XR_OS_MutexUnlock(&hci_mutex);
		return -1;
	}
	pcb->id_cnt++;
	pcb->id_index++;

	hcb->id = id;
	hcb->in_use = 1;
	hcb->features = features;
	hcb->cbacks.data_ind = p_cb->data_ind;

	if (pcb->state) {
		XR_OS_MutexUnlock(&hci_mutex);
		return id;
	}

	pcb->state = 1;
	XR_OS_MutexUnlock(&hci_mutex);

	hal_hci_open((void *)&bt_hc_callbacks);

	return id;
}

static int hci_write(int id, uint8_t type, uint8_t *data, uint16_t len)
{
	XR_OS_MutexLock(&tx_mutex, XR_OS_WAIT_FOREVER);

#if defined(CONFIG_BT_DIST_MODE_CORRESPOND)
	bt_lib_pcb_t *pcb = &tLibPCB;
	bt_hc_cb_t *hcb = &pcb->hcbs[id];
	hci_distribute_enter_list(hcb->features, data[0], data[1], type);
#endif

#if defined(CONFIG_BT_DIST_MODE_CORRESPOND_DEBUG)
	hci_send_record(hcb->features);
#endif

	static uint8_t pkt[1024] = {0};
	pkt[0] = type;
	memcpy(pkt + 1, data, len);

#ifdef CONFIG_BT_DRIVERS_LOG_BTSNOOP
	void btsnoop_capture(uint8_t type, const uint8_t *buf, bool is_received);
	btsnoop_capture(type, data, 0);
#endif
#ifdef CONFIG_BT_DRIVERS_LOG_HCIDUMP
	hcidump_process(type, data, 0);
#endif

	hal_hci_write(pkt, len + 1);

	XR_OS_MutexUnlock(&tx_mutex);

	return 0;
}

static int hci_read(int id, uint8_t *data, uint16_t len)
{
	return 0;
}

static int hci_close(int id)
{
	bt_hc_cb_t *hcb;
	bt_lib_pcb_t *pcb = &tLibPCB;
	XR_OS_MutexLock(&hci_mutex, XR_OS_WAIT_FOREVER);

	if (pcb->id_cnt <= 0 || id >= BT_HCI_ID_MAX) {
		XR_OS_MutexUnlock(&hci_mutex);
		BT_LIB_TRACE_WARNING("Invalid ID\n");
		return -1;
	}

	hcb = &pcb->hcbs[id];
	if (!hcb->in_use) {
		BT_LIB_TRACE_WARNING("ID not exist\n");
		XR_OS_MutexUnlock(&hci_mutex);
		return -1;
	}
	memset(hcb, 0, sizeof(bt_hc_cb_t));
	pcb->id_index = id;

	pcb->id_cnt--;
	if (pcb->id_cnt <= 0) {
		pcb->state = 0;
		pcb->id_index = 0;
		hal_hci_close();
		XR_OS_MutexUnlock(&hci_mutex);
		XR_OS_MutexDelete(&hci_mutex);
		XR_OS_MutexDelete(&tx_mutex);
		return 0;
	}

	XR_OS_MutexUnlock(&hci_mutex);

	return 0;
}

static const bt_hc_interface_t btHCLibInterface = {
	.open = hci_open,
	.write = hci_write,
	.read = hci_read,
	.close = hci_close,
};

static const bt_hc_ctrl_interface_t btHCLibCtrlInterface = {
	.status = hal_controller_ready,
	.set_mac = hal_controller_set_mac,
};

static int bt_lib_init()
{
	if (!lib_init_dev_num) {
#if defined(CONFIG_BT_DIST_MODE_CORRESPOND)
		hci_distribute_init();
#endif
		hal_controller_init();
	}

	lib_init_dev_num++;
	return 0;
}

static int bt_lib_deinit(void)
{
	if (lib_init_dev_num == LIB_NO_DEV_REGISTER) {
		return -EPERM;
	} else if(lib_init_dev_num > LIB_ONE_DEV_REGISTER) {
		lib_init_dev_num--;
		return 0;
	} else {
#if defined(CONFIG_BT_DIST_MODE_CORRESPOND)
		hci_distribute_deinit();
#endif

		hal_controller_deinit();
		lib_init_dev_num--;
	}

	return 0;
}

static const bt_lib_interface_t btLibInterface = {
	.init = bt_lib_init,
	.deinit = bt_lib_deinit,
	.hci_ops = &btHCLibInterface,
	.hci_ctrl_ops = &btHCLibCtrlInterface,
};

const bt_lib_interface_t *bt_lib_get_interface(void)
{
	return &btLibInterface;
}

