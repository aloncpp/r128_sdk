#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <unistd.h>

#include <hal_cmd.h>
#include <hal_time.h>

#include "sunxi_amp.h"
#include "amp_threadpool.h"

#ifdef CONFIG_COMPONENTS_PM
#include "pm_task.h"
#endif

static threadpool_t *global_pool;

int amp_threadpool_free(threadpool_t *pool)
{
    if (pool == NULL)
    {
        return -1;
    }
    if (pool->task_queue)
    {
        vQueueDelete(pool->task_queue);
    }
    if (pool->threads)
    {
        amp_free(pool->threads);
        hal_mutex_lock(pool->lock);
        hal_mutex_delete(pool->lock);
        hal_mutex_lock(pool->thread_counter);
        hal_mutex_delete(pool->thread_counter);
    }
    amp_free(pool);
    global_pool = NULL;

    return 0;
}

void amp_threadpool_thread(void *thread_param)
{
    threadpool_t *pool = (threadpool_t *)global_pool;
#ifdef CONFIG_AMP_THREADPOOL_DEBUG
    thread_debug_t *now_thread = (thread_debug_t *)(thread_param);
#else
    TaskHandle_t *now_thread = (TaskHandle_t *)thread_param;
#endif
    threadpool_task_t task;
    BaseType_t xStatus;
    const TickType_t xTicksToWait = portMAX_DELAY;
    char name[configMAX_TASK_NAME_LEN];
#ifdef CONFIG_AMP_FUNCALL_THREADPOOL_WAIT_RECVQUEUE
    sunxi_amp_info *sunxi_amp = get_amp_info();
    sunxi_amp_msg receive_val;
#endif

    while (true)
    {
        hal_mutex_lock(pool->lock);
        if (pool->wait_exit_thr_num > 0)
        {
            pool->wait_exit_thr_num--;
            if (pool->live_thr_num > pool->min_thr_num)
            {
                memset(name, 0, sizeof(name));
#if ( INCLUDE_xTaskGetHandle == 1 )
                amp_debug("thread %s is exiting \n", pcTaskGetName(xTaskGetCurrentTaskHandle()));
#endif
                pool->live_thr_num--;
                hal_mutex_unlock(pool->lock);
                memset(now_thread, 0, sizeof(*now_thread));
#ifdef CONFIG_COMPONENTS_PM
		        pm_task_unregister(xTaskGetCurrentTaskHandle());
#endif
                vTaskDelete(NULL);
            }
        }
        hal_mutex_unlock(pool->lock);

#ifdef CONFIG_AMP_FUNCALL_THREADPOOL_WAIT_RECVQUEUE
        xStatus = xQueueReceive(sunxi_amp->recv_queue, &receive_val, xTicksToWait);
#else
        xStatus = xQueueReceive(pool->task_queue, &task, xTicksToWait);
#endif
        if (xStatus != pdPASS)
        {
            continue;
        }

#if ( INCLUDE_xTaskGetHandle == 1 )
        amp_debug("thread 0x%x(%s) start working \n", (unsigned int)xTaskGetCurrentTaskHandle(), pcTaskGetName(xTaskGetCurrentTaskHandle()));
#endif
        hal_mutex_lock(pool->thread_counter);
        pool->busy_thr_num++;
        hal_mutex_unlock(pool->thread_counter);

#ifdef CONFIG_AMP_FUNCALL_THREADPOOL_WAIT_RECVQUEUE
        handle_recv_amp_msg(sunxi_amp, &receive_val);
#else
        (*(task.function))(task.arg);
#endif
#ifdef CONFIG_AMP_THREADPOOL_DEBUG
        now_thread->run_num++;
#endif
#if ( INCLUDE_xTaskGetHandle == 1 )
        amp_debug("thread %s end working \n", pcTaskGetName(xTaskGetCurrentTaskHandle()));
#endif
        hal_mutex_lock(pool->thread_counter);
        pool->busy_thr_num--;
        hal_mutex_unlock(pool->thread_counter);
    }

    vTaskDelete(NULL);
}

void amp_admin_thread(void *threadpool)
{
    int i;
    threadpool_t *pool = (threadpool_t *)threadpool;
    while (true)
    {
        amp_info("admin -----------------\n");
        hal_msleep(DEFAULT_TIME * 1000);
        hal_mutex_lock(pool->lock);
        int live_thr_num = pool->live_thr_num;
        hal_mutex_unlock(pool->lock);

        hal_mutex_lock(pool->thread_counter);
        int busy_thr_num = pool->busy_thr_num;
        hal_mutex_unlock(pool->thread_counter);

        int queue_size = uxQueueMessagesWaiting(pool->task_queue);

        amp_info("admin busy live -%d--%d-\n", busy_thr_num, live_thr_num);
        if (queue_size >= MIN_WAIT_TASK_NUM && live_thr_num <= pool->max_thr_num)
        {
            amp_debug("admin add-----------\n");
            hal_mutex_lock(pool->lock);
            int add = 0;
            TaskHandle_t thread;

            for (i = 0; i < pool->max_thr_num && add < DEFAULT_THREAD_NUM
                 && pool->live_thr_num < pool->max_thr_num; i++)
            {
#ifdef CONFIG_AMP_THREADPOOL_DEBUG
                if (pool->threads[i].thread == NULL)
#else
                if (pool->threads[i] == NULL)
#endif
                {
                        char thread_name[16] = {0};
                        snprintf(thread_name, sizeof(thread_name) - 1, "amp-ser%d", i);
#ifdef CONFIG_AMP_THREADPOOL_DEBUG
                    if (xTaskCreate((TaskFunction_t)amp_threadpool_thread, (const char *)thread_name, AMP_THREAD_POOL_TASK_STACK_SIZE, (void *) & (pool->threads[i]), DEFAULT_PRIORITY, (void *)&(pool->threads[i].thread)) == pdPASS)
#else
                    if (xTaskCreate((TaskFunction_t)amp_threadpool_thread, (const char *)thread_name, AMP_THREAD_POOL_TASK_STACK_SIZE, (void *) & (pool->threads[i]), DEFAULT_PRIORITY, (void *)&(pool->threads[i])) == pdPASS)
#endif
                    {
                        char thread_name[16] = {0};
                        snprintf(thread_name, sizeof(thread_name) - 1, "amp-ser%d", i);
#ifdef CONFIG_AMP_THREADPOOL_DEBUG
                        thread = pool->threads[i].thread;
#else
                        thread = pool->threads[i];
#endif
#ifdef CONFIG_COMPONENTS_PM
                        pm_task_register(thread, PM_TASK_TYPE_SYS);
#endif
                        add++;
                        pool->live_thr_num++;
                        amp_debug("new thread -----------------------\n");
                    }
                }
            }

            hal_mutex_unlock(pool->lock);
        }

        if ((busy_thr_num * 2) < live_thr_num  &&  live_thr_num > pool->min_thr_num)
        {
            amp_info("admin busy --%d--%d----\n", busy_thr_num, live_thr_num);
            hal_mutex_lock(pool->lock);
            pool->wait_exit_thr_num = DEFAULT_THREAD_NUM;
            hal_mutex_unlock(pool->lock);
        }
    }

    vTaskDelete(NULL);
}

threadpool_t *amp_threadpool_create(int min_thr_num, int max_thr_num, int queue_max_size)
{
    int i;
    threadpool_t *pool = NULL;
    TaskHandle_t thread = NULL;

    do
    {
        if ((pool = (threadpool_t *)amp_malloc(sizeof(threadpool_t))) == NULL)
        {
            amp_err("malloc threadpool false; \n");
            break;
        }

        pool->min_thr_num = min_thr_num;
        pool->max_thr_num = max_thr_num;
        pool->busy_thr_num = 0;
        pool->live_thr_num = min_thr_num;
        pool->wait_exit_thr_num = 0;
        pool->queue_max_size = queue_max_size;

#ifdef CONFIG_AMP_THREADPOOL_DEBUG
        pool->threads = (thread_debug_t *)amp_malloc(sizeof(thread_debug_t) * max_thr_num);
#else
        pool->threads = (TaskHandle_t *)amp_malloc(sizeof(TaskHandle_t) * max_thr_num);
#endif
        if (pool->threads == NULL)
        {
            amp_err("malloc threads false;\n");
            break;
        }
#ifdef CONFIG_AMP_THREADPOOL_DEBUG
        memset(pool->threads, 0, sizeof(thread_debug_t)*max_thr_num);
#else
        memset(pool->threads, 0, sizeof(TaskHandle_t)*max_thr_num);
#endif

        pool->task_queue = xQueueCreate(queue_max_size, sizeof(threadpool_task_t));
        if (pool->task_queue == NULL)
        {
            amp_err("amp create queue error!\n");
            break;
        }

        pool->lock = hal_mutex_create();
        pool->thread_counter = hal_mutex_create();
        if (pool->lock == NULL || pool->thread_counter == NULL)
        {
            amp_err("init lock or cond false;\n");
            break;
        }

        global_pool = pool;

        for (i = 0; i < min_thr_num; i++)
        {
            char thread_name[16] = {0};
            snprintf(thread_name, sizeof(thread_name) - 1, "amp-ser%d", i);
#ifdef CONFIG_AMP_THREADPOOL_DEBUG
            if (xTaskCreate((TaskFunction_t)amp_threadpool_thread, (const char *)thread_name, AMP_THREAD_POOL_TASK_STACK_SIZE, (void *) & (pool->threads[i]), DEFAULT_PRIORITY, (void *)&(pool->threads[i].thread)) == pdPASS)
#else
            if (xTaskCreate((TaskFunction_t)amp_threadpool_thread, (const char *)thread_name, AMP_THREAD_POOL_TASK_STACK_SIZE, (void *) & (pool->threads[i]), DEFAULT_PRIORITY, (void *)&(pool->threads[i])) == pdPASS)
#endif
            {
#ifdef CONFIG_AMP_THREADPOOL_DEBUG
                thread = pool->threads[i].thread;
#else
                thread = pool->threads[i];
#endif
#ifdef CONFIG_COMPONENTS_PM
                pm_task_register(thread, PM_TASK_TYPE_SYS);
#endif
                amp_debug("start thread %s... \n", thread_name);
            }
        }
        if (xTaskCreate((TaskFunction_t)amp_admin_thread, "amp-admin", AMP_THREAD_POOL_ADMIN_TASK_STACK_SIZE, (void *)pool, DEFAULT_PRIORITY, (void *)&(pool->admin_tid)) == pdPASS)
        {
#ifdef CONFIG_COMPONENTS_PM
            pm_task_register(pool->admin_tid, PM_TASK_TYPE_SYS);
#endif
            amp_debug("start thread amp-admin... \n");
        }
        return pool;
    }
    while (0);

    amp_threadpool_free(pool);
    return NULL;
}

int amp_threadpool_add_task(threadpool_t *pool, void (*function)(void *arg), void *arg)
{
    threadpool_task_t task;

    if (!pool)
    {
        return -1;
    }

    task.function = function;
    task.arg = arg;

    xQueueSend(pool->task_queue, &task, portMAX_DELAY);
    return 0;
}

int amp_threadpool_destroy(threadpool_t *pool)
{
    if (pool == NULL)
    {
        return -1;
    }

    amp_threadpool_free(pool);
    return 0;
}

threadpool_t *amp_get_threadpool(void)
{
    return global_pool;
}

int amp_threadpool_init(void)
{
    threadpool_t *thp = amp_threadpool_create(AMP_THD_POOL_MIN_NUM,
                                              AMP_THD_POOL_MAX_NUM,
                                              AMP_THD_POOL_QUEUE_MAX_SIZE);
    if (thp)
    {
        amp_debug("threadpool init success ... ... \n");
        return 0;
    }
    else
    {
        amp_debug("threadpool init failed ... ... \n");
        return -1;
    }
}

#ifdef CONFIG_AMP_THREADPOOL_DEBUG
static int cmd_amp_threadpool(int argc, char **argv)
{
    printf("\n");
    threadpool_t *pool = global_pool;
    int i;

    if (pool)
    {
        printf("Show amp thread pool:\n");
        printf("min_thr_num=%d\n", pool->min_thr_num);
        printf("max_thr_num=%d\n", pool->max_thr_num);
        printf("busy_thr_num=%d\n", pool->busy_thr_num);
        printf("live_thr_num=%d\n", pool->live_thr_num);
        printf("wait_exit_thr_num=%d\n", pool->wait_exit_thr_num);
        printf("queue_size=%d\n", pool->queue_size);
        printf("queue_max_size=%d\n", pool->queue_max_size);

        printf("\n");
        for (i = 0; i < pool->max_thr_num; i++)
        {
            thread_debug_t *thread = &(pool->threads[i]);
            if (thread->thread)
            {
                printf("%03d: run_num:%d\n", i, thread->run_num);
            }
        }
    }

    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_amp_threadpool, amp_threadpool, show amp thread pool information);
#endif
