#ifndef _AMP_THREADPOOL_H
#define _AMP_THREADPOOL_H

#include <hal_mutex.h>

#define DEFAULT_TIME            1
#define MIN_WAIT_TASK_NUM       2
#define DEFAULT_THREAD_NUM      2
#define DEFAULT_PRIORITY        HAL_THREAD_PRIORITY_HIGHEST

#define AMP_THREAD_POOL_ADMIN_TASK_STACK_SIZE (1024 * 1)
#define AMP_THREAD_POOL_TASK_STACK_SIZE (1024 * 2)

#define AMP_THD_POOL_MIN_NUM        5
#define AMP_THD_POOL_MAX_NUM        20
#define AMP_THD_POOL_QUEUE_MAX_SIZE 50

#define CONFIG_AMP_THREADPOOL_DEBUG

typedef struct
{
    void (*function)(void *);
    void *arg;
} threadpool_task_t;

typedef struct
{
    void *thread;
    unsigned int run_num;
} thread_debug_t;

typedef struct _threadpool_t
{
    hal_mutex_t lock;
    hal_mutex_t thread_counter;

#ifdef CONFIG_AMP_THREADPOOL_DEBUG
    thread_debug_t *threads;
#else
    TaskHandle_t *threads;
#endif
    TaskHandle_t admin_tid;
    QueueHandle_t task_queue;

    int min_thr_num;
    int max_thr_num;
    int live_thr_num;
    int busy_thr_num;
    int wait_exit_thr_num;

    int queue_size;
    int queue_max_size;
} threadpool_t;

threadpool_t *amp_get_threadpool(void);
int amp_threadpool_init(void);
int amp_threadpool_add_task(threadpool_t *pool, void (*function)(void *arg), void *arg);

#endif
