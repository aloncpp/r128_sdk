#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "sunxi_amp.h"
#include <platform-msgbox.h>
#include <hal_msgbox.h>
#include <hal_interrupt.h>

#define CHANNAL_RV2ARM 0
#define CHANNAL_ARM2RV 0

#define CHANNAL_ARM2DSP 0
#define CHANNAL_DSP2ARM 1

#define CHANNAL_RV2DSP 1
#define CHANNAL_DSP2RV 1

static int msgbox_send_to_queue(sunxi_amp_info *amp, sunxi_amp_msg *msg)
{
    BaseType_t ret;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (hal_interrupt_get_nest()) {
        ret = xQueueSendFromISR(amp->send_queue, msg, &xHigherPriorityTaskWoken);
        if (ret == pdPASS) {
            (void)xHigherPriorityTaskWoken;
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    }
    else
        ret = xQueueSend(amp->send_queue, msg, portMAX_DELAY);

    if (ret != pdPASS) {
        return -1;
    }
    return 0;
}

#ifdef CONFIG_AMP_BOOT_CORE_DETECT
#define GPRCM_SYS_PRIV_REG6	(0x40050218)
static int8_t g_boot_core_flag = 0;
void amp_boot_core_save()
{
	volatile uint32_t value;

	value = readl(GPRCM_SYS_PRIV_REG6);
	if (value & ((1 << SELF_DIRECTION))) {
		printf("boot flag already set:0x%x\n", value);
		goto set_local_flag;
	}
	value |= (1 << SELF_DIRECTION);
	writel(value, GPRCM_SYS_PRIV_REG6);
set_local_flag:
	g_boot_core_flag |= (1 << SELF_DIRECTION);
}

static int amp_boot_core_detect(int32_t dir)
{
	volatile uint32_t value;

	if (g_boot_core_flag & (1 << dir))
		return 0;

	value = readl(GPRCM_SYS_PRIV_REG6);
	if (value & (1 << dir)) {
		g_boot_core_flag |= ((1 << dir));
		return 0;
	}

	return -1;
}
#endif

static int msgbox_send_to_dev(sunxi_amp_info *amp, sunxi_amp_msg *msg)
{
    struct msg_endpoint *medp = NULL;
    int ret = -1;

#ifdef CONFIG_AMP_BOOT_CORE_DETECT
    if (amp_boot_core_detect(msg->dst)) {
        amp_err("dst core[%d] flag miss.\n", msg->dst);
        return -1;
    }
#endif

    switch (msg->src)
    {
        case RPC_MSG_DIR_CM33:
            if (msg->dst == RPC_MSG_DIR_RV)
            {
                medp = &amp->sedp_rv;
            }
            else if (msg->dst == RPC_MSG_DIR_DSP)
            {
                medp = &amp->sedp_dsp;
            }
            break;
        case RPC_MSG_DIR_RV:
            if (msg->dst == RPC_MSG_DIR_CM33)
            {
                medp = &amp->sedp_arm;
            }
            else if (msg->dst == RPC_MSG_DIR_DSP)
            {
                medp = &amp->sedp_dsp;
            }
            break;
        case RPC_MSG_DIR_DSP:
            if (msg->dst == RPC_MSG_DIR_CM33)
            {
                medp = &amp->sedp_arm;
            }
            else if (msg->dst == RPC_MSG_DIR_RV)
            {
                medp = &amp->sedp_rv;
            }
            break;
        default:
            amp_err("cann't support [%d]->[%d]\n", msg->src, msg->dst);
            break;
    }
    if (medp)
    {
        hal_msgbox_channel_send(medp, (uint8_t *)msg, sizeof(sunxi_amp_msg));
        ret = 0;
    }
    return ret;
}

static int msgbox_receive_from_dev(sunxi_amp_info *amp, sunxi_amp_msg *msg)
{
    BaseType_t taskWoken;
    BaseType_t ret;

    if (hal_interrupt_get_nest())
    {
        taskWoken = pdFALSE;
        ret = xQueueSendFromISR(amp->recv_queue, msg, &taskWoken);
        if (ret != pdPASS)
        {
            amp_err("receive amp msg fail!\n");
            return -1;
        }
        portYIELD_FROM_ISR(taskWoken);
    }
    else
    {
        ret = xQueueSend(amp->recv_queue, msg, portMAX_DELAY);
        if (ret != pdPASS)
        {
            amp_err("receive amp msg fail!\n");
            return -1;
        }
    }
    return 0;
}

#if defined(CONFIG_ARCH_ARM_CORTEX_M33)
static void amp_msgbox_receive_rv_to_arm(uint32_t data, void *private)
{
    static sunxi_amp_msg msg = {0};
    static int recv_count = 0;
    char *pMsg = (char *)&msg;

    memcpy(pMsg + recv_count * sizeof(data), &data, sizeof(data));
    recv_count ++;

    if ((recv_count * sizeof(data)) == sizeof(sunxi_amp_msg))
    {
        recv_count = 0;
        hal_amp_msg_recv(&msg);
    }
}

static void amp_msgbox_receive_dsp_to_arm(uint32_t data, void *private)
{
    static sunxi_amp_msg msg = {0};
    static int recv_count = 0;
    char *pMsg = (char *)&msg;

    memcpy(pMsg + recv_count * sizeof(data), &data, sizeof(data));
    recv_count ++;

    if ((recv_count * sizeof(data)) == sizeof(sunxi_amp_msg))
    {
        recv_count = 0;
        hal_amp_msg_recv(&msg);
    }
}
#endif

#if defined(CONFIG_ARCH_RISCV_C906)
static void amp_msgbox_receive_arm_to_rv(uint32_t data, void *private)
{
    static sunxi_amp_msg msg = {0};
    static int recv_count = 0;
    char *pMsg = (char *)&msg;

    memcpy(pMsg + recv_count * sizeof(data), &data, sizeof(data));
    recv_count ++;

    if ((recv_count * sizeof(data)) == sizeof(sunxi_amp_msg))
    {
        recv_count = 0;
        hal_amp_msg_recv(&msg);
    }
}

static void amp_msgbox_receive_dsp_to_rv(uint32_t data, void *private)
{
    static sunxi_amp_msg msg = {0};
    static int recv_count = 0;
    char *pMsg = (char *)&msg;

    memcpy(pMsg + recv_count * sizeof(data), &data, sizeof(data));
    recv_count ++;

    if ((recv_count * sizeof(data)) == sizeof(sunxi_amp_msg))
    {
        recv_count = 0;
        hal_amp_msg_recv(&msg);
    }
}

#endif

#if defined(CONFIG_ARCH_DSP)
static void amp_msgbox_receive_arm_to_dsp(uint32_t data, void *private)
{
    static sunxi_amp_msg msg = {0};
    static int recv_count = 0;
    char *pMsg = (char *)&msg;

    memcpy(pMsg + recv_count * sizeof(data), &data, sizeof(data));
    recv_count ++;

    if ((recv_count * sizeof(data)) == sizeof(sunxi_amp_msg))
    {
        recv_count = 0;
        hal_amp_msg_recv(&msg);
    }
}

static void amp_msgbox_receive_rv_to_dsp(uint32_t data, void *private)
{
    static sunxi_amp_msg msg = {0};
    static int recv_count = 0;
    char *pMsg = (char *)&msg;

    memcpy(pMsg + recv_count * sizeof(data), &data, sizeof(data));
    recv_count ++;

    if ((recv_count * sizeof(data)) == sizeof(sunxi_amp_msg))
    {
        recv_count = 0;
        hal_amp_msg_recv(&msg);
    }
}

#endif

static int msgbox_dev_init(void)
{
    sunxi_amp_info *amp = NULL;
    amp = get_amp_info();

    struct msg_endpoint *arm = &amp->sedp_arm;
    struct msg_endpoint *rv = &amp->sedp_rv;
    struct msg_endpoint *dsp = &amp->sedp_dsp;

    (void)arm;
    (void)rv;
    (void)dsp;

#if defined(CONFIG_ARCH_ARM_CORTEX_M33)
    rv->rec = amp_msgbox_receive_rv_to_arm;
    hal_msgbox_alloc_channel(rv, MSGBOX_RISCV, CHANNAL_RV2ARM, CHANNAL_ARM2RV);

    dsp->rec = amp_msgbox_receive_dsp_to_arm;
    hal_msgbox_alloc_channel(dsp, MSGBOX_DSP, CHANNAL_DSP2ARM, CHANNAL_ARM2DSP);

#elif defined(CONFIG_ARCH_RISCV_C906)
    arm->rec = amp_msgbox_receive_arm_to_rv;
    hal_msgbox_alloc_channel(arm, MSGBOX_ARM, CHANNAL_ARM2RV, CHANNAL_RV2ARM);

    dsp->rec = amp_msgbox_receive_dsp_to_rv;
    hal_msgbox_alloc_channel(dsp, MSGBOX_DSP, CHANNAL_DSP2RV, CHANNAL_RV2DSP);

#elif defined(CONFIG_ARCH_DSP)
    arm->rec = amp_msgbox_receive_arm_to_dsp;
    hal_msgbox_alloc_channel(arm, MSGBOX_ARM, CHANNAL_ARM2DSP, CHANNAL_DSP2ARM);

    rv->rec = amp_msgbox_receive_rv_to_dsp;
    hal_msgbox_alloc_channel(rv, MSGBOX_RISCV, CHANNAL_RV2DSP, CHANNAL_DSP2RV);

#endif
    return 0;
}

sunxi_amp_msg_ops msgbox_ops =
{
    .send_to_queue = msgbox_send_to_queue,
    .send_to_dev = msgbox_send_to_dev,
    .receive_from_dev = msgbox_receive_from_dev,
    .init = msgbox_dev_init,
};
