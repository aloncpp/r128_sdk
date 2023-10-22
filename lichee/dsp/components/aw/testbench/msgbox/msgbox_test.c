#include <FreeRTOS.h>
#include <task.h>
#include <stdint.h>
#include <queue.h>
#include <sunxi_hal_common.h>
#include <hal_msgbox.h>

#define TASK_TEST_STACK_LEN 1024
#define MESSAGE_QUEUE_LEN   1000
static TaskHandle_t test_task;
static StackType_t  test_stk[TASK_TEST_STACK_LEN];
static volatile int send_finish = 1;
static StaticTask_t tcb_test;
struct messagebox *msgbox_init_sx(enum msgbox_direction dir);

static QueueHandle_t q_handle;
static u32 msg_queue[MESSAGE_QUEUE_LEN];
static StaticQueue_t msg_ctrl_queue;

/* static struct messagebox *msg; */
static struct msg_channel *send_channel;
static struct msg_channel *rev_channel;

static int send_callback(unsigned long v, void *p)
{
	send_finish = 1;

	return 0;
}

static int arisc_send_to_cpu(struct msg_channel *ch, unsigned char *d, int len)
{
	int result;

	while(!send_finish);
	send_finish = 0;

	result = msgbox_channel_send_data(ch, d, len);
	while(!send_finish);

	return result;
}

static int rev_callback(unsigned long v, void *p)
{
	BaseType_t ret;
	BaseType_t xHigherProTaskWoken = pdFALSE;
	ret = xQueueSendFromISR(q_handle, &v, &xHigherProTaskWoken);
	if (ret == pdPASS) {
		portYIELD_FROM_ISR(xHigherProTaskWoken);
	}

	return 0;
}

static void task_test(void *parg)
{
	printf("msgbox test start\n");
	q_handle = xQueueCreateStatic(MESSAGE_QUEUE_LEN, sizeof(uint32_t),
				      (uint8_t *)msg_queue, &msg_ctrl_queue);

	rev_channel = msgbox_alloc_channel(
		msgbox_cpu, 0, MSGBOX_CHANNEL_RECEIVE, rev_callback, NULL);
	send_channel = msgbox_alloc_channel(msgbox_cpu, 1, MSGBOX_CHANNEL_SEND,
					    send_callback, NULL);

	for (;;) {
		u32 rev;

		if (pdPASS == xQueueReceive(q_handle, &rev, portMAX_DELAY)) {
			printf("rev:%d\n", rev);
			arisc_send_to_cpu(send_channel, (unsigned char *)&rev,
					  sizeof(rev));
		}
	}
}

void init_test_msgbox(void)
{
	messagebox_init();
	test_task = xTaskCreateStatic(task_test, "for_test",
				      TASK_TEST_STACK_LEN, NULL,
				      configMAX_PRIORITIES - 4, test_stk,
				      &tcb_test);

}
