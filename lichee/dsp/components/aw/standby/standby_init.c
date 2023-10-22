#include <FreeRTOS.h>
#include <task.h>

#include "standby.h"
#include "standby_wakeup.h"
#include "semphr.h"
#include "timers.h"
#include "drivers/hal_prcm.h"

static struct platform_standby standby_cb;

/* for task standby wakeup */
static StackType_t task_standby_wakeup_stack[TASK_STANDBY_WAKEUP_STACK_LEN];
static StaticTask_t task_standby_wakeup_task;

static TaskHandle_t standby_task_wakeup;

static StaticSemaphore_t wakeup_sem;
static SemaphoreHandle_t h_wakeup_sem;

static StaticTimer_t wakeup_timer;
static TimerHandle_t h_wakeup_timer;
/*
 * this file should define in arch platform.
 * put this function in arch dir
 */
void __attribute__((weak)) platform_standby_config(struct platform_standby *ps)
{
	return;
}

/**
 * wait wakeup event
 */
void standby_wait_wakeup(struct platform_standby *ps)
{
	standby_emit_wakeup(STANDBY_START_WAKEUP);
	xSemaphoreTake(h_wakeup_sem, portMAX_DELAY);
}

/*
 * send to wakeup task to perform some finish ops
 */
void standby_wakeup(struct platform_standby *ps)
{
	xTimerStop(h_wakeup_timer, pdMS_TO_TICKS(500));
	standby_emit_wakeup(STANDBY_STOP_WAKEUP);
}

void standby_wakeup_isr(struct platform_standby *ps, BaseType_t *pxHigherPriorityTaskWoken)
{
	BaseType_t WokenTimer = pdFALSE;
	BaseType_t WokenSemap = pdFALSE;

	xTimerStopFromISR(h_wakeup_timer, &WokenTimer);

	standby_emit_wakeup_isr(STANDBY_STOP_WAKEUP, &WokenSemap);

	if (WokenTimer != pdFALSE || WokenSemap != pdFALSE) {
		*pxHigherPriorityTaskWoken = pdTRUE;
		portYIELD_FROM_ISR(*pxHigherPriorityTaskWoken);
	}
}

void standby_set_delay_wakeup(int ms)
{
	xTimerChangePeriod(h_wakeup_timer, pdMS_TO_TICKS(ms),
			   pdMS_TO_TICKS(500));
}
/*
 * wakeup system
 */
void __standby_wakeup_system(struct platform_standby *ps)
{
	xSemaphoreGive(h_wakeup_sem);
}

void wakeup_timer_cb(TimerHandle_t xTimer)
{
	standby_wakeup(NULL);
}

/**
 * task_standby_init - this task init all of standby
 *
 * the user should call this standby init.
 * the task create should use with not static
 * it will delete itself end of init.
 */
void task_standby_init(void *parg)
{
	platform_standby_config(&standby_cb);

	/* create task of standby wakeup */
	standby_task_wakeup = xTaskCreateStatic(
		task_standby_wakeup, STANDBY_WAKEUP_TASK_NAME,
		TASK_STANDBY_WAKEUP_STACK_LEN, &standby_cb,
		configMAX_PRIORITIES - 2, task_standby_wakeup_stack,
		&task_standby_wakeup_task);

	/* wakeup timer */
	h_wakeup_timer = xTimerCreateStatic("wakeup_timer", pdMS_TO_TICKS(2000),
					    pdFALSE, NULL, wakeup_timer_cb,
					    &wakeup_timer);
	/* create wakeup semaphore */
	h_wakeup_sem = xSemaphoreCreateBinaryStatic(&wakeup_sem);

	/* vTaskDelete(NULL); */
}

void standby_platform_message(struct message *msg)
{
	standby_cb.msg = msg;
}

struct platform_standby *platform_standby_get(void)
{
	return &standby_cb;
}
