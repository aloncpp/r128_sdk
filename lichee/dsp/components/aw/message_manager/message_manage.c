#include <FreeRTOS.h>
#include <task.h>
#include <stdint.h>
#include <queue.h>
#include "message_manage_i.h"
#include "message_manage.h"
#include "compiler_attributes.h"
#include "hal_msgbox.h"
#include "standby.h"
#include <sunxi_hal_common.h>
#include "delay.h"

#define MESSAGE_MANAGE_TASK_NAME 	"message_manage"
#define MESSAGE_MANAGE_BUF_LEN        	256
#define TASK_MESSAGE_MANAGE_STACK_LEN 	1024
#define TICK_TO_MS(x)        		((x) * (1000 / configTICK_RATE_HZ))
#define TIMEOUT_MS                    	2000
#define MESSAGE_QUEUE_LEN             	5


static TaskHandle_t msg_manage;
static StackType_t  msg_manage_stk[TASK_MESSAGE_MANAGE_STACK_LEN];
static StaticTask_t tcb_msg_manage;
static volatile int send_finish = 1;

static StaticQueue_t msg_ctrl_queue;
static struct msg_rec_ctrl_q *msg_queue[MESSAGE_QUEUE_LEN];
static QueueHandle_t q_handle;

static struct msg_channel *send_channel;
static struct msg_channel *rev_channel;

struct msg_rec_ctrl {
	unsigned char idx;

	/* this used to force to message struct */
	unsigned char buf[MESSAGE_MANAGE_BUF_LEN] __attribute__((aligned(8)));
};

struct msg_rec_ctrl_q {
	int used;
	struct msg_rec_ctrl msg_ctrl;
};

void task_msg_manage_notify(uint32_t notice, int irq)
{
	BaseType_t ret;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	if (irq) {
		ret = xTaskNotifyFromISR(msg_manage, 0xff, eSetValueWithOverwrite, &xHigherPriorityTaskWoken);
		if (ret == pdPASS) {
			portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
		}
	}
	else
		xTaskNotify(msg_manage, 0xff, eSetValueWithOverwrite);
}

static struct msg_rec_ctrl_q msg_ctrl_q[MESSAGE_QUEUE_LEN];

#if 0
static void dump_message(struct message *pmessage)
{
	int cnt = (pmessage->count + 2) * 4;
	int i = 0;
	char *p = (char *)pmessage;

	printf("dump message\n");
	for (i = 0; i < cnt; i++) {
		printf("0x%02x ", p[i]);
	}
	printf("\n");
}
#endif


static int send_callback(unsigned long __maybe_unused v, void __maybe_unused *p)
{
	send_finish = 1;

	return 0;
}

/*
 * sample send function
 */
static int arisc_send_to_cpu(struct msg_channel *ch, unsigned char *d, int len)
{
	int result;

	while(!send_finish);
	send_finish = 0;

	result = msgbox_channel_send_data(ch, d, len);
	while(!send_finish);

	return result;
}

/*
 * this function only used in interrupt.
 */
static struct msg_rec_ctrl_q *get_available_buf(void)
{
	int i = 0;

	for (i = 0; i < MESSAGE_QUEUE_LEN; i++) {
		if (!msg_ctrl_q[i].used)
			return &msg_ctrl_q[i];
	}

	return NULL;
}

static int rev_callback(unsigned long v, void __maybe_unused *p)
{
	static TickType_t tick, last_tick;
	static struct msg_rec_ctrl_q *mctrl = &msg_ctrl_q[0];
	struct msg_rec_ctrl *pmsgctl;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	/* printf("M%x\n", v); */

	/* return 0; */
	/* when there's no buffer to use, we find next buf available */
	if (mctrl == NULL) {
		mctrl = get_available_buf();
		if (mctrl == NULL)
			return 0;
	}

	pmsgctl = &mctrl->msg_ctrl;
	tick = xTaskGetTickCountFromISR();
	if (pmsgctl->idx && TICK_TO_MS(tick - last_tick) > TIMEOUT_MS) {
		pmsgctl->idx = 0;
	}

	pmsgctl->buf[pmsgctl->idx++] = (v >> 0) & 0xff;
	pmsgctl->buf[pmsgctl->idx++] = (v >> 8) & 0xff;
	pmsgctl->buf[pmsgctl->idx++] = (v >> 16) & 0xff;
	pmsgctl->buf[pmsgctl->idx++] = (v >> 24) & 0xff;

	/* 5 is the count, so can count the count */
	if (pmsgctl->idx >= 5 && pmsgctl->idx == (pmsgctl->buf[4] * 4 + 8)) {
		pmsgctl->idx = 0;
		mctrl->used = 1;
		/* send queue */
		if (xQueueSendFromISR(q_handle, &mctrl, &xHigherPriorityTaskWoken) != pdTRUE) {
			/*
			 * when there's no queue buf to send.
			 * we lost this message
			 * and we use this mctrl again.
			 */
			mctrl->used = 0;
		} else {
			portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
			mctrl = get_available_buf();
		}
	}

	last_tick = tick;
	return 0;
}

static int msg_feedback(struct message *pmessage,
			 struct msg_channel *send_channel)
{
	u32 i;
	u32 value;

	ASSERT(pmessage != NULL);

	if (pmessage->attr & MESSAGE_ATTR_HARDSYN) {
		/* use ac327 hard syn receiver channel */
		INF("send feedback message\n");

		/* first send message header and misc */
		value = pmessage->state | (pmessage->attr << 8) |
			(pmessage->type << 16) | (pmessage->result << 24);
		arisc_send_to_cpu(send_channel, (unsigned char *)&value, sizeof(value));

		value = pmessage->count;
		arisc_send_to_cpu(send_channel, (unsigned char *)&value, sizeof(value));

		/* then send message paras */
		for (i = 0; i < pmessage->count; i++) {
			arisc_send_to_cpu(send_channel,
					  (unsigned char *)&pmessage->paras[i],
					  sizeof(pmessage->paras[i]));
		}
		INF("feedback hard syn message : %x\n", pmessage->type);
		return OK;
	}

	/* invalid syn message */
	return -ESRCH;
}

static int32_t coming_msg_handler(struct message *pmessage)
{
	int32_t result = 0;
	struct platform_standby *ps = platform_standby_get();

	standby_platform_message(pmessage);
	switch (pmessage->type) {

	case CPU_OP_REQ:
		LOG("cpu op req\n");
		result = cpu_op(pmessage);
		break;

	case SYS_OP_REQ:
		LOG("sys op req\n");
		result = sys_op(pmessage);
		break;

	case CLEAR_WAKEUP_SRC_REQ:
		LOG("clear wakeup src req\n");
		if (ps->clear_wakeup_source)
			result = ps->clear_wakeup_source(pmessage);
		break;

	case SET_WAKEUP_SRC_REQ:
		LOG("set wakeup src req\n");
		if (ps->set_wakeup_source)
			result = ps->set_wakeup_source(pmessage);
		break;

	case SET_DEBUG_LEVEL_REQ:
		INF("set debug level request\n");
		/* result = set_debug_level(pmessage->paras[0]); */
		break;

	case SET_UART_BAUDRATE:
		INF("set uart baudrate request\n");
		/* result = uart_set_baudrate(pmessage->paras[0]); */
		break;

	case SET_DRAM_CRC_PARAS:
		INF("set dram crc paras request\n");
		result = standby_set_dram_crc_paras(pmessage->paras[0], pmessage->paras[1], pmessage->paras[2]);
		break;

	case MESSAGE_LOOPBACK:
		INF("loopback message request\n");
		/* result = OK; */
		break;

	default:
		ERR("imt [%x]\n", pmessage->type);
		/* hexdump("msg", (char *)pmessage, sizeof(struct message)); */
		/* result = -ESRCH; */
		break;
	}

	/* return OK; */
	return result;
}

#include "interrupt.h"
#include "irqs.h"


static void task_message_manage(void __maybe_unused *parg)
{
	struct msg_rec_ctrl_q *pmsgctrl_q;
	struct message *pmsg;
	static struct message msg;

	rev_channel = msgbox_alloc_channel(
		msgbox_cpu, 2, MSGBOX_CHANNEL_RECEIVE, rev_callback, NULL);
	send_channel = msgbox_alloc_channel(msgbox_cpu, 3, MSGBOX_CHANNEL_SEND,
					    send_callback, NULL);
	q_handle = xQueueCreateStatic(MESSAGE_QUEUE_LEN,
				      sizeof(struct msg_rec_ctrl_q *),
				      (uint8_t *)msg_queue, &msg_ctrl_queue);

	/*
	 * irq_disable(SUNXI_DSP_IRQ_MSGBOX0_DSP);
	 * while (1) {
	 *         unsigned long v;
	 *         if (msgbox_cpu->channel_fifo_len(msgbox_cpu, rev_channel->channel)) {
	 *                 msgbox_cpu->channel_read(msgbox_cpu, rev_channel->channel, &v);
	 *                 printf("%x\n", v);
	 *         }
	 * }
	 */

	for (;;) {
		if (pdPASS ==
		    xQueueReceive(q_handle, &pmsgctrl_q, portMAX_DELAY)) {
			int32_t result;

			pmsg = (void *)pmsgctrl_q->msg_ctrl.buf;
			msg.state = pmsg->state;
			msg.attr = pmsg->attr;
			msg.type = pmsg->type;
			msg.result = pmsg->result;
			msg.count = pmsg->count;
			msg.paras = &pmsg->reserved2[0];

			result = coming_msg_handler(&msg);

			msg.result = result;
			/* synchronous message, should feedback process result
			 */
			if (msg.attr &
			    (MESSAGE_ATTR_SOFTSYN | MESSAGE_ATTR_HARDSYN))
				msg_feedback(&msg, send_channel);




			pmsgctrl_q->used = 0;
		}
	}
}


int message_manage_init(void)
{
	printf("create message manage\n");
	/* create task of message */
	msg_manage = xTaskCreateStatic(task_message_manage,
				       MESSAGE_MANAGE_TASK_NAME,
				       TASK_MESSAGE_MANAGE_STACK_LEN, NULL,
				       configMAX_PRIORITIES - 4, msg_manage_stk,
				       &tcb_msg_manage);

	return 0;
}

