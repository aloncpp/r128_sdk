#include <stddef.h>
#include <hal_thread.h>
#include <hal_interrupt.h>
#include <hal_osal.h>

void *hal_thread_create(void (*threadfn)(void *data), void *data, const char *namefmt, int stacksize, int priority)
{
    TaskHandle_t pxCreatedTask = NULL;
    BaseType_t ret = 0;;
    ret = xTaskCreate(threadfn, namefmt, stacksize, data, priority, &pxCreatedTask);
    if (ret != pdPASS)
    {
        hal_log_err("create task failed!\n");
    }
    return pxCreatedTask;
}

int hal_thread_start(void *thread)
{
    return HAL_OK;
}

int hal_thread_stop(void *thread)
{
    vTaskDelete(thread);
    return HAL_OK;
}

int hal_thread_resume(void *task)
{
    vTaskResume(task);
    return HAL_OK;
}

int hal_thread_suspend(void *task)
{
    vTaskSuspend(task);
    return HAL_OK;
}

void *hal_thread_self(void)
{
    return (void *)xTaskGetCurrentTaskHandle();
}

int hal_thread_scheduler_is_running(void)
{
	return (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING);
}

int hal_thread_is_in_critical_context(void)
{
	if (hal_interrupt_get_nest())
		return 1;
	if (!hal_thread_scheduler_is_running())
		return 1;
	if (hal_interrupt_is_disable())
		return 1;
	return 0;
}

char *hal_thread_get_name(void *thread)
{
    return pcTaskGetName(thread);
}

int hal_thread_scheduler_suspend(void)
{
    vTaskSuspendAll();
    return HAL_OK;
}

int hal_thread_scheduler_resume(void)
{
    return xTaskResumeAll();
}
