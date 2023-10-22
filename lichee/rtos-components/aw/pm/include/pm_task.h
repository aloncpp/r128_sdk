#ifndef _PM_TASK_H_
#define _PM_TASK_H_

#ifdef CONFIG_KERNEL_FREERTOS
#include <FreeRTOS.h>
#include <task.h>
#else
#error "PM do not support the RTOS!!"
#endif
#include <osal/hal_timer.h>

#if (INCLUDE_vTaskSuspend != 1)
#error "ERROR: please define macro INCLUDE_vTaskSuspend by 1"
#endif

typedef enum {
	PM_TASK_TYPE_APP = 0,

	PM_TASK_TYPE_PM,
	PM_TASK_TYPE_SYS,
	PM_TASK_TYPE_DSP,
	PM_TASK_TYPE_RISCV,
	PM_TASK_TYPE_BT,
	PM_TASK_TYPE_WLAN,

	PM_TASK_TYPE_MAX,

	PM_TASK_TYPE_BASE = PM_TASK_TYPE_PM,
} pm_task_type_t;

#define pm_task_type_app(_tpy) \
	((_tpy) == PM_TASK_TYPE_APP)

#define pm_task_type_notapp(_tpy) \
	((_tpy) >= PM_TASK_TYPE_PM || (_tpy) < PM_TASK_TYPE_MAX)

#define pm_task_type_valid(_tpy) \
	((_tpy) >= PM_TASK_TYPE_APP || (_tpy) < PM_TASK_TYPE_MAX)

#define PM_TASK_MAX                32
#define PM_TASK_TOTAL_MAX          64

struct pm_task {
	uint8_t      status;
	pm_task_type_t type;
	TaskHandle_t xHandle;
};

int pm_task_init(void);
int pm_task_exit(void);
int pm_task_freeze(pm_task_type_t type);
int pm_task_restore(pm_task_type_t type);
int pm_task_register(TaskHandle_t xHandle, pm_task_type_t type);
int pm_task_unregister(TaskHandle_t xHandle);

int pm_task_attach_timer(osal_timer_t timer, TaskHandle_t xHandle, uint32_t attach);

#if 0
/* need pthread.h 
 * means to get TaskHandle_t from pthread_t.
 */
int pm_pthread_register(pthread_t thread, pm_task_type_t type);
int pm_pthread_unregister(pthread_t thread);
#endif

#endif


