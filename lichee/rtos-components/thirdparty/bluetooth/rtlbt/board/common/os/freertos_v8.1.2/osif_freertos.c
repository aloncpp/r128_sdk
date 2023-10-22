/**
 * Copyright (c) 2015, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <stdint.h>
#include <string.h>
#include <FreeRTOS.h>
#include <task.h>
#include <timers.h>
#include <queue.h>
#include <semphr.h>
#include <osif.h>
#include <stdio.h>
#include <stdarg.h>
static char os_buffer[1024];
/****************************************************************************/
/* Check if in task context (true), or isr context (false)                  */
/****************************************************************************/
int os_if_printf(const char *fmt, ...)
{
    int ret;

    va_list args;

    va_start(args, fmt);

    ret = vsnprintf(os_buffer, 1024, fmt, args);
    printf("%s",os_buffer);

    va_end(args);

    return ret;
}
/****************************************************************************/
/* Check if in task context (true), or isr context (false)                  */
/****************************************************************************/
static void *sig_handle;
bool osif_signal_init(void)
{
    sig_handle = (void *)xSemaphoreCreateCounting(1, 0);
    if (sig_handle != NULL)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void osif_signal_deinit(void)
{
    if (sig_handle != NULL)
    {
        vSemaphoreDelete(sig_handle);
        sig_handle = NULL;
    }
}
extern volatile uint32_t ulPortInterruptNesting[];
extern int cur_cpu_id(void);

static inline bool osif_task_context_check(void)
{
	if(ulPortInterruptNesting[cur_cpu_id()] != 0) // is interrupt context
		return false;
	else
		return true;
}

/****************************************************************************/
/* Delay current task in a given milliseconds                               */
/****************************************************************************/
void osif_delay(uint32_t ms)
{
    vTaskDelay((TickType_t)((ms + portTICK_PERIOD_MS - 1) / portTICK_PERIOD_MS));
}

/****************************************************************************/
/* Get system time in milliseconds                                          */
/****************************************************************************/
uint32_t osif_sys_time_get(void)
{
    if (osif_task_context_check() == true)
    {
        return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
    }
    else
    {
        return (uint32_t)(xTaskGetTickCountFromISR() * portTICK_PERIOD_MS);
    }
}

/****************************************************************************/
/* Start os kernel scheduler                                                */
/****************************************************************************/
bool osif_sched_start(void)
{
    vTaskStartScheduler();

    return true;
}

/****************************************************************************/
/* Stop os kernel scheduler                                                 */
/****************************************************************************/
bool osif_sched_stop(void)
{
    vTaskEndScheduler();

    return true;
}

/****************************************************************************/
/* Suspend os kernel scheduler                                              */
/****************************************************************************/
bool osif_sched_suspend(void)
{
    vTaskSuspendAll();

    return true;
}

/****************************************************************************/
/* Resume os kernel scheduler                                               */
/****************************************************************************/
bool osif_sched_resume(void)
{
    xTaskResumeAll();

    return true;
}

/****************************************************************************/
/* Create os level task routine                                             */
/****************************************************************************/
bool osif_task_create(void **pp_handle, const char *p_name, void (*p_routine)(void *),
                      void *p_param, uint16_t stack_size, uint16_t priority)
{
    BaseType_t ret;

    if (pp_handle == NULL)
    {
        return false;
    }

    ret = xTaskCreate(p_routine, (const char *)p_name, stack_size / sizeof(portSTACK_TYPE),
                      p_param, priority+26 , (TaskHandle_t *)pp_handle);
    if (ret == pdPASS)
    {
        return true;
    }
    else
    {
        return false;
    }
}

/****************************************************************************/
/* Delete os level task routine                                             */
/****************************************************************************/
bool osif_task_delete(void *p_handle)
{
    vTaskDelete((TaskHandle_t)p_handle);

    return true;
}

/****************************************************************************/
/* Suspend os level task routine                                            */
/****************************************************************************/
bool osif_task_suspend(void *p_handle)
{
    vTaskSuspend((TaskHandle_t)p_handle);

    return true;
}

/****************************************************************************/
/* Resume os level task routine                                             */
/****************************************************************************/
bool osif_task_resume(void *p_handle)
{
    vTaskResume((TaskHandle_t)p_handle);

    return true;
}

/****************************************************************************/
/* Yield current os level task routine                                      */
/****************************************************************************/
bool osif_task_yield(void)
{
    //taskYIELD();

    //return true;
    return false;
}

/****************************************************************************/
/* Get current os level task routine handle                                 */
/****************************************************************************/
bool osif_task_handle_get(void **pp_handle)
{
    if (pp_handle == NULL)
    {
        return false;
    }

    *pp_handle = (void *)xTaskGetCurrentTaskHandle();

    return true;
}

/****************************************************************************/
/* Get os level task routine priority                                       */
/****************************************************************************/
bool osif_task_priority_get(void *p_handle, uint16_t *p_priority)
{
    if (p_priority == NULL)
    {
        return false;
    }

    *p_priority = uxTaskPriorityGet((TaskHandle_t)p_handle);

    return true;
}

/****************************************************************************/
/* Set os level task routine priority                                       */
/****************************************************************************/
bool osif_task_priority_set(void *p_handle, uint16_t priority)
{
    vTaskPrioritySet((TaskHandle_t)p_handle, priority);

    return true;
}
#if 0
/****************************************************************************/
/* Send signal to target task                                               */
/****************************************************************************/
bool osif_task_signal_send(void *p_handle, uint32_t signal)
{

    (void)p_handle;
    (void)signal;
    if (!sig_handle)
    {
        return false;
    }

    BaseType_t ret;

    if (osif_task_context_check() == true)
    {
        ret = xSemaphoreGive((QueueHandle_t)sig_handle);
    }
    else
    {
        BaseType_t task_woken = pdFALSE;

        ret = xSemaphoreGiveFromISR((QueueHandle_t)sig_handle, &task_woken);

        portEND_SWITCHING_ISR(task_woken);
    }
    (void)ret;

    return true;
}
#else
bool osif_task_signal_send(void *p_handle, uint32_t signal)
{
    (void)signal;

    BaseType_t ret;

    if (osif_task_context_check() == true)

    {
        ret = xTaskNotifyGive(p_handle);
    }
    else
    {
        BaseType_t task_woken = pdFALSE;

        vTaskNotifyGiveFromISR((QueueHandle_t)sig_handle, &task_woken);

        portEND_SWITCHING_ISR(task_woken);

    }
    (void)ret;
    return true;
}
#endif
#if 0
/****************************************************************************/
/* Receive signal in target task                                            */
/****************************************************************************/
bool osif_task_signal_recv(uint32_t *p_handle, uint32_t wait_ms)
{
    (void)p_handle;
    BaseType_t ret;
    TickType_t wait_ticks;

    if (!sig_handle)
    {
		os_if_printf("sig hanlde is NULL.\n");
        return false;
    }

    if (wait_ms == 0xFFFFFFFFUL)
    {
        wait_ticks = portMAX_DELAY;
    }
    else
    {
        wait_ticks = (TickType_t)((wait_ms + portTICK_PERIOD_MS - 1) / portTICK_PERIOD_MS);
    }

    if (osif_task_context_check() == true)
    {
        ret = xSemaphoreTake((QueueHandle_t)sig_handle, wait_ticks);
    }
    else
    {
        BaseType_t task_woken = pdFALSE;

        ret = xSemaphoreTakeFromISR((QueueHandle_t)sig_handle, &task_woken);

        portEND_SWITCHING_ISR(task_woken);
    }

    if (ret == pdTRUE)
    {
        return true;
    }
    else
    {
        return false;
    }
}
#else
bool osif_task_signal_recv(uint32_t *p_handle, uint32_t wait_ms)
{

    (void)p_handle;

    BaseType_t ret;

    TickType_t wait_ticks;

    if (wait_ms == 0xFFFFFFFFUL)
    {
        wait_ticks = portMAX_DELAY;
    }
    else
    {
        wait_ticks = (TickType_t)((wait_ms + portTICK_PERIOD_MS - 1) / portTICK_PERIOD_MS);

    }

    if (osif_task_context_check() == true)

    {
        ret = ulTaskNotifyTake(pdFALSE,wait_ticks);
    }
    else
    {
        //NOT IN ISR PENDING
        ret = pdFAIL;

    }
    if (ret == pdTRUE)

    {
        return true;
    }
    else
    {
        return false;
    }
}
#endif
/****************************************************************************/
/* Clear signal in target task                                              */
/****************************************************************************/
bool osif_task_signal_clear(void *p_handle)
{
    BaseType_t ret;

extern BaseType_t xTaskNotifyStateClear( TaskHandle_t xTask );
    ret = xTaskNotifyStateClear((TaskHandle_t)p_handle);
    if (ret == pdTRUE)
    {
        return true;
    }
    else
    {
        return false;
    }
}

/****************************************************************************/
/* Lock critical section                                                    */
/****************************************************************************/
uint32_t osif_lock(void)
{
    uint32_t flags = 0U;

    if (osif_task_context_check() == true)
    {
#if 1
        taskENTER_CRITICAL(flags);
#else
	__disable_irq();
	vTaskSuspendAll();
#endif
    }
    else
    {
        flags = taskENTER_CRITICAL_FROM_ISR();
    }

    return flags;
}

/****************************************************************************/
/* Unlock critical section                                                  */
/****************************************************************************/
void osif_unlock(uint32_t flags)
{
    if (osif_task_context_check() == true)
    {
#if 1
        taskEXIT_CRITICAL(flags);
#else
	__enable_irq();
	xTaskResumeAll();
#endif
    }
    else
    {
        taskEXIT_CRITICAL_FROM_ISR(flags);
    }

}
/****************************************************************************/
/* Create counting semaphore                                                */
/****************************************************************************/
bool osif_sem_create(void **pp_handle, uint32_t init_count, uint32_t max_count)
{
    if (pp_handle == NULL)
    {
        return false;
    }

    *pp_handle = (void *)xSemaphoreCreateCounting(max_count, init_count);
    if (*pp_handle != NULL)
    {
        return true;
    }
    else
    {
        return false;
    }
}

/****************************************************************************/
/* Delete counting semaphore                                                */
/****************************************************************************/
bool osif_sem_delete(void *p_handle)
{
    vSemaphoreDelete((QueueHandle_t)p_handle);

    return true;
}

/****************************************************************************/
/* Take counting semaphore                                                  */
/****************************************************************************/
bool osif_sem_take(void *p_handle, uint32_t wait_ms)
{
    BaseType_t ret;

    if (osif_task_context_check() == true)
    {
        TickType_t wait_ticks;

        if (wait_ms == 0xFFFFFFFFUL)
        {
            wait_ticks = portMAX_DELAY;
        }
        else
        {
            wait_ticks = (TickType_t)((wait_ms + portTICK_PERIOD_MS - 1) / portTICK_PERIOD_MS);
        }

        ret = xSemaphoreTake((QueueHandle_t)p_handle, wait_ticks);
    }
    else
    {
        BaseType_t task_woken = pdFALSE;

        ret = xSemaphoreTakeFromISR((QueueHandle_t)p_handle, &task_woken);

        portEND_SWITCHING_ISR(task_woken);
    }

    if (ret == pdTRUE)
    {
        return true;
    }
    else
    {
        return false;
    }
}

/****************************************************************************/
/* Give counting semaphore                                                  */
/****************************************************************************/
bool osif_sem_give(void *p_handle)
{
    BaseType_t ret;

    if (osif_task_context_check() == true)
    {
        ret = xSemaphoreGive((QueueHandle_t)p_handle);
    }
    else
    {
        BaseType_t task_woken = pdFALSE;

        ret = xSemaphoreGiveFromISR((QueueHandle_t)p_handle, &task_woken);

        portEND_SWITCHING_ISR(task_woken);
    }

    if (ret == pdTRUE)
    {
        return true;
    }
    else
    {
        return false;
    }
}

/****************************************************************************/
/* Create recursive mutex                                                   */
/****************************************************************************/
bool osif_mutex_create(void **pp_handle)
{
    if (pp_handle == NULL)
    {
        return false;
    }

    *pp_handle = (void *)xSemaphoreCreateRecursiveMutex();
    if (*pp_handle != NULL)
    {
        return true;
    }
    else
    {
        return false;
    }
}

/****************************************************************************/
/* Delete recursive mutex                                                   */
/****************************************************************************/
bool osif_mutex_delete(void *p_handle)
{
    if (xSemaphoreGetMutexHolder((QueueHandle_t)p_handle) == NULL)
    {
        vSemaphoreDelete((QueueHandle_t)p_handle);
        return true;
    }
    else
    {
        /* Do not delete mutex if held by a task */
        return false;
    }
}

/****************************************************************************/
/* Take recursive mutex                                                     */
/****************************************************************************/
bool osif_mutex_take(void *p_handle, uint32_t wait_ms)
{
    TickType_t wait_ticks;
    BaseType_t ret;

    if (wait_ms == 0xFFFFFFFFUL)
    {
        wait_ticks = portMAX_DELAY;
    }
    else
    {
        wait_ticks = (TickType_t)((wait_ms + portTICK_PERIOD_MS - 1) / portTICK_PERIOD_MS);
    }

    ret = xSemaphoreTakeRecursive((QueueHandle_t)p_handle, wait_ticks);
    if (ret == pdTRUE)
    {
        return true;
    }
    else
    {
        return false;
    }
}

/****************************************************************************/
/* Give recursive mutex                                                     */
/****************************************************************************/
bool osif_mutex_give(void *p_handle)
{
    BaseType_t ret;

    ret = xSemaphoreGiveRecursive((QueueHandle_t)p_handle);
    if (ret == pdTRUE)
    {
        return true;
    }
    else
    {
        return false;
    }
}

/****************************************************************************/
/* Create inter-thread message queue                                        */
/****************************************************************************/
bool osif_msg_queue_create(void **pp_handle, uint32_t msg_num, uint32_t msg_size)
{
    if (pp_handle == NULL)
    {
        return false;
    }

    *pp_handle = (void *)xQueueCreate(msg_num, msg_size);
    if (*pp_handle != NULL)
    {
        return true;
    }
    else
    {
        return false;
    }
}

/****************************************************************************/
/* Delete inter-thread message queue                                        */
/****************************************************************************/
bool osif_msg_queue_delete(void *p_handle)
{
    vQueueDelete((QueueHandle_t)p_handle);

    return true;
}

/****************************************************************************/
/* Peek inter-thread message queue's pending but not received msg number    */
/****************************************************************************/
bool osif_msg_queue_peek(void *p_handle, uint32_t *p_msg_num)
{
    if (osif_task_context_check() == true)
    {
        *p_msg_num = (uint32_t)uxQueueMessagesWaiting((QueueHandle_t)p_handle);
    }
    else
    {
        *p_msg_num = (uint32_t)uxQueueMessagesWaitingFromISR((QueueHandle_t)p_handle);
    }

    return true;
}

/****************************************************************************/
/* Send inter-thread message                                                */
/****************************************************************************/
bool osif_msg_send(void *p_handle, void *p_msg, uint32_t wait_ms)
{
    BaseType_t ret;

    if (osif_task_context_check() == true)
    {
        TickType_t wait_ticks;

        if (wait_ms == 0xFFFFFFFFUL)
        {
            wait_ticks = portMAX_DELAY;
        }
        else
        {
            wait_ticks = (TickType_t)((wait_ms + portTICK_PERIOD_MS - 1) / portTICK_PERIOD_MS);
        }

        ret = xQueueSendToBack((QueueHandle_t)p_handle, p_msg, wait_ticks);
    }
    else
    {
        BaseType_t task_woken = pdFALSE;

        ret = xQueueSendToBackFromISR((QueueHandle_t)p_handle, p_msg, &task_woken);

        portEND_SWITCHING_ISR(task_woken);
    }

    if (ret == pdTRUE)
    {
        return true;
    }
    else
    {
        return false;
    }
}

/****************************************************************************/
/* Receive inter-thread message                                             */
/****************************************************************************/
bool osif_msg_recv(void *p_handle, void *p_msg, uint32_t wait_ms)
{
    BaseType_t ret;

    if (osif_task_context_check() == true)
    {
        TickType_t wait_ticks;

        if (wait_ms == 0xFFFFFFFFUL)
        {
            wait_ticks = portMAX_DELAY;
        }
        else
        {
            wait_ticks = (TickType_t)((wait_ms + portTICK_PERIOD_MS - 1) / portTICK_PERIOD_MS);
        }

        ret = xQueueReceive((QueueHandle_t)p_handle, p_msg, wait_ticks);
    }
    else
    {
        BaseType_t task_woken = pdFALSE;

        ret = xQueueReceiveFromISR((QueueHandle_t)p_handle, p_msg, &task_woken);

        portEND_SWITCHING_ISR(task_woken);
    }

    if (ret == pdTRUE)
    {
        return true;
    }
    else
    {
        return false;
    }
}

/****************************************************************************/
/* Peek inter-thread message                                                */
/****************************************************************************/
bool osif_msg_peek(void *p_handle, void *p_msg, uint32_t wait_ms)
{
    BaseType_t ret;

    if (osif_task_context_check() == true)
    {
        TickType_t wait_ticks;

        if (wait_ms == 0xFFFFFFFFUL)
        {
            wait_ticks = portMAX_DELAY;
        }
        else
        {
            wait_ticks = (TickType_t)((wait_ms + portTICK_PERIOD_MS - 1) / portTICK_PERIOD_MS);
        }

        ret = xQueuePeek((QueueHandle_t)p_handle, p_msg, wait_ticks);
    }
    else
    {
        ret = xQueuePeekFromISR((QueueHandle_t)p_handle, p_msg);
    }

    if (ret == pdTRUE)
    {
        return true;
    }
    else
    {
        return false;
    }
}

/****************************************************************************/
/* Allocate memory                                                          */
/****************************************************************************/
void *osif_mem_alloc(RAM_TYPE ram_type, size_t size)
{
#if 0
	  if(size > xPortGetFreeHeapSize(ram_type))
		{
	     printf("\r\nneed %d, free %d, %d\r\n",size,xPortGetFreeHeapSize(ram_type), (uint8_t)ram_type);
		}
    return pvPortMalloc(ram_type, size);
#else
    return pvPortMalloc(size);
#endif
}

/****************************************************************************/
/* Allocate aligned memory                                                  */
/****************************************************************************/
void *osif_mem_aligned_alloc(RAM_TYPE ram_type, size_t size, uint8_t alignment)
{
    void *p;
    void *p_aligned;

    if (alignment == 0)
    {
        alignment = portBYTE_ALIGNMENT;
    }
#if 0
    p = pvPortMalloc(ram_type, size + sizeof(void *) + alignment);
#else
    p = pvPortMalloc(size + sizeof(void *) + alignment);
#endif
    if (p == NULL)
    {
        return p;
    }

    p_aligned = (void *)(((size_t)p + sizeof(void *) + alignment) & ~(alignment - 1));

    memcpy((uint8_t *)p_aligned - sizeof(void *), &p, sizeof(void *));

    return p_aligned;
}

/****************************************************************************/
/* Free memory                                                              */
/****************************************************************************/
void osif_mem_free(void *p_block)
{
    vPortFree(p_block);
}

/****************************************************************************/
/* Free aligned memory                                                      */
/****************************************************************************/
void osif_mem_aligned_free(void *p_block)
{
    void *p;

    memcpy(&p, (uint8_t *)p_block - sizeof(void *), sizeof(void *));

    vPortFree(p);
}

/****************************************************************************/
/* Peek unused (available) memory size                                    */
/****************************************************************************/
size_t osif_mem_peek(RAM_TYPE ram_type)
{
#if 0
    return xPortGetFreeHeapSize(ram_type);
#else
    return xPortGetFreeHeapSize();
#endif
}

/****************************************************************************/
/* Get software timer ID                                                    */
/****************************************************************************/
bool osif_timer_id_get(void **pp_handle, uint32_t *p_timer_id)
{
    if (pp_handle == NULL || *pp_handle == NULL)
    {
        return false;
    }

    *p_timer_id = (uint32_t)pvTimerGetTimerID((TimerHandle_t) * pp_handle);

    return true;
}


/****************************************************************************/
/* Create software timer                                                    */
/****************************************************************************/
bool osif_timer_create(void **pp_handle, const char *p_timer_name, uint32_t timer_id,
                       uint32_t interval_ms, bool reload, void (*p_timer_callback)(void *))
{
    TickType_t timer_ticks;

    if (pp_handle == NULL || p_timer_callback == NULL)
    {
        return false;
    }

    timer_ticks = (TickType_t)((interval_ms + portTICK_PERIOD_MS - 1) / portTICK_PERIOD_MS);

    if (*pp_handle == NULL)
    {
        *pp_handle = xTimerCreate(p_timer_name, timer_ticks, (BaseType_t)reload,
                                  (void *)timer_id, p_timer_callback);
        if (*pp_handle == NULL)
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    return true;
}

/****************************************************************************/
/* Start software timer                                                     */
/****************************************************************************/
bool osif_timer_start(void **pp_handle)
{
    BaseType_t ret;

    if (pp_handle == NULL || *pp_handle == NULL)
    {
        return false;
    }

    if (osif_task_context_check() == true)
    {
        ret = xTimerStart((TimerHandle_t) * pp_handle, (TickType_t)0);
    }
    else
    {
        BaseType_t task_woken = pdFALSE;

        ret = xTimerStartFromISR((TimerHandle_t) * pp_handle, &task_woken);

        portEND_SWITCHING_ISR(task_woken);
    }

    if (ret == pdTRUE)
    {
        return true;
    }
    else
    {
        return false;
    }
}

/****************************************************************************/
/* Restart software timer                                                   */
/****************************************************************************/
bool osif_timer_restart(void **pp_handle, uint32_t interval_ms)
{
    TickType_t timer_ticks;
    BaseType_t ret;

    if (pp_handle == NULL || *pp_handle == NULL)
    {
        return false;
    }

    timer_ticks = (TickType_t)((interval_ms + portTICK_PERIOD_MS - 1) / portTICK_PERIOD_MS);

    if (osif_task_context_check() == true)
    {
        ret = xTimerChangePeriod((TimerHandle_t) * pp_handle, timer_ticks, (TickType_t)0);
    }
    else
    {
        BaseType_t task_woken = pdFALSE;

        ret = xTimerChangePeriodFromISR((TimerHandle_t) * pp_handle, timer_ticks, &task_woken);

        portEND_SWITCHING_ISR(task_woken);
    }

    if (ret == pdTRUE)
    {
        return true;
    }
    else
    {
        return false;
    }
}

/****************************************************************************/
/* Stop software timer                                                      */
/****************************************************************************/
bool osif_timer_stop(void **pp_handle)
{
    BaseType_t ret;

    if (pp_handle == NULL || *pp_handle == NULL)
    {
        return false;
    }

    if (osif_task_context_check() == true)
    {
        ret = xTimerStop((TimerHandle_t) * pp_handle, (TickType_t)0);
    }
    else
    {
        BaseType_t task_woken = pdFALSE;

        ret = xTimerStopFromISR((TimerHandle_t) * pp_handle, &task_woken);

        portEND_SWITCHING_ISR(task_woken);
    }

    if (ret == pdTRUE)
    {
        return true;
    }
    else
    {
        return false;
    }
}

/****************************************************************************/
/* Delete software timer                                                    */
/****************************************************************************/
bool osif_timer_delete(void **pp_handle)
{
    if (pp_handle == NULL || *pp_handle == NULL)
    {
        return false;
    }

    if (xTimerDelete((TimerHandle_t)*pp_handle, (TickType_t)0) == pdFAIL)
    {
        return false;
    }

    *pp_handle = NULL;

    return true;
}

/****************************************************************************/
/* Dump software timer                                                      */
/****************************************************************************/
bool osif_timer_dump(void)
{
    //dumpAllUsedTimer();
    return true;
}

bool osif_timer_state_get(void **pp_handle, uint32_t *p_timer_state)
{
    return true;
}



