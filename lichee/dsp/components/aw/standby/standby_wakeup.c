#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <standby.h>
#include <standby_wakeup.h>


#define MAX_QUEUE_ITEM_LENGTH 12

static uint8_t task_queue[MAX_QUEUE_ITEM_LENGTH];
static StaticQueue_t q_wq;
static QueueHandle_t qhandler;

/**
 * standby_emit_wakeup - used to emit wakeup system
 *
 * normal this msg while wakeup system, but some of the msg
 * while wakeup the cpus and then wakeup up cpu in condition.
 * so it actual as a msg filter.
 */
void standby_emit_wakeup(uint8_t event)
{
	xQueueSend(qhandler, &event, portMAX_DELAY);
}

void standby_emit_wakeup_isr(uint8_t event, BaseType_t *pxHigherPriorityTaskWoken)
{
	xQueueSendFromISR(qhandler, &event, pxHigherPriorityTaskWoken);
}

/*
 * task_standby_wakeup - the wakeup task
 *
 * used to manage task of wakeup
 * the wakeup filter task
 */
void task_standby_wakeup(void *parg)
{
	uint8_t event;
	uint8_t hs_sleep = 0;
	struct platform_standby *ps = parg;

	qhandler = xQueueCreateStatic(MAX_QUEUE_ITEM_LENGTH, sizeof(uint8_t),
				      task_queue, &q_wq);
	for (;;) {
		if (xQueueReceive(qhandler, &event, portMAX_DELAY) != pdPASS)
			continue;

		switch (event) {
		case STANDBY_START_WAKEUP:
			if (!hs_sleep) {
				invoke_function(ps->start_wait_wakeup, ps);
				if (ps->ms_to_wakeup)
					standby_set_delay_wakeup(
						ps->ms_to_wakeup);
				hs_sleep = 1;
			}
			break;
		case STANDBY_STOP_WAKEUP:
			if (hs_sleep) {
				invoke_function(ps->stop_wait_wakeup, ps);
				__standby_wakeup_system(ps);
				hs_sleep = 0;
			}
			break;
		case STANDBY_WAKEUP_MAD:
			/* TODO: wake up dsp system */
			break;
		case STANDBY_WAKEUP_TEXT:
		case STANDBY_WAKEUP_POWER:
		case STANDBY_WAKEUP_IR:
		case STANDBY_WAKEUP_OTHERS:
		default:
			standby_wakeup(ps);
			break;
		}
	}
}

