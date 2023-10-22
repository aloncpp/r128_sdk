
#ifndef _PM_SUSPEND_H_
#define _PM_SUSPEND_H_

#ifdef CONFIG_KERNEL_FREERTOS
#include <FreeRTOS.h>
#else
#error "PM do not support the RTOS!!"
#endif

#include <task.h>
#include <queue.h>

#include "pm_base.h"
#include "pm_state.h"

struct pm_suspend_thread_t {
	TaskHandle_t  xHandle;
	QueueHandle_t xQueue;
};

int pm_suspend_init(void);
int pm_suspend_exit(void);

int pm_suspend_request(suspend_mode_t mode);

#endif

