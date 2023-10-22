#ifndef SUNXI_HAL_THREAD_H
#define SUNXI_HAL_THREAD_H

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(CONFIG_KERNEL_FREERTOS)
#include <FreeRTOS.h>
#include <task.h>
typedef TaskHandle_t hal_thread_t;

#define HAL_THREAD_PRIORITY_APP     ((configMAX_PRIORITIES >> 1))
#define HAL_THREAD_PRIORITY_CLI     ((configMAX_PRIORITIES >> 1) + 2)
#define HAL_THREAD_PRIORITY_SYS     ((configMAX_PRIORITIES >> 1) + 1)
#define HAL_THREAD_PRIORITY_NET     ((configMAX_PRIORITIES >> 1) + 1)

#define HAL_THREAD_PRIORITY_HIGHEST (configMAX_PRIORITIES - 1)
#define HAL_THREAD_PRIORITY_LOWEST  (0)
#define HAL_THREAD_PRIORITY_MIDDLE  (configMAX_PRIORITIES >> 1)

#else
#include <rtthread.h>
typedef rt_thread_t hal_thread_t;
typedef struct rt_thread hal_thread;

#define HAL_THREAD_PRIORITY_APP     (4)
#define HAL_THREAD_PRIORITY_CLI     (3)
#define HAL_THREAD_PRIORITY_SYS     (3)
#define HAL_THREAD_PRIORITY_NET     (3)

#define HAL_THREAD_PRIORITY_HIGHEST (0)
#define HAL_THREAD_PRIORITY_LOWEST  (31)
#define HAL_THREAD_PRIORITY_MIDDLE  (15)

#endif

#define HAL_THREAD_STACK_SIZE    (0x2000)
#define HAL_THREAD_TIMESLICE     (    10)

#define HAL_THREAD_PRIORITY      HAL_THREAD_PRIORITY_APP

void *hal_thread_create(void (*threadfn)(void *data), void *data, const char *namefmt, int stacksize, int priority);
int hal_thread_stop(void *thread);
int hal_thread_start(void *thread);
void *hal_thread_self(void);
int hal_thread_resume(void *thread);
int hal_thread_suspend(void *thread);
int hal_thread_msleep(int ms);
int hal_thread_sleep(int tick);
int hal_thread_scheduler_is_running(void);
int hal_thread_is_in_critical_context(void);
void hal_thread_tick_increase(void);
char *hal_thread_get_name(void *thread);
int hal_thread_scheduler_suspend(void);
int hal_thread_scheduler_resume(void);

#define hal_thread_run(threadfn, data, namefmt, ...)			   \
({									   \
	void *__k						   \
		= hal_thread_create(threadfn, data, namefmt, HAL_THREAD_STACK_SIZE, HAL_THREAD_PRIORITY_SYS); \
	if (__k)				   \
		hal_thread_start(__k);					   \
	__k;								   \
})

#ifdef __cplusplus
}
#endif
#endif
