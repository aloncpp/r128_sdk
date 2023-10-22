#include <stdlib.h>
#include <hal_workqueue.h>
#include <hal_thread.h>
#include <hal_atomic.h>
#include <hal_interrupt.h>
#include <sunxi_hal_common.h>

static void _delayed_work_timeout_handler(void *parameter);

static inline unsigned long workqueue_list_lock(hal_workqueue *queue)
{
#ifdef CONFIG_SMP
    return hal_spin_lock_irqsave(&queue->llock);
#else
    return hal_interrupt_disable_irqsave();
#endif
}

static inline void workqueue_list_unlock(hal_workqueue *queue, unsigned long flag)
{
#ifdef CONFIG_SMP
    hal_spin_unlock_irqrestore(&queue->llock, flag);
#else
    hal_interrupt_enable_irqrestore(flag);
#endif
}

void hal_work_init(hal_work *work, void (*work_func)(hal_work *work, void *work_data), void *work_data)
{
    INIT_LIST_HEAD(&(work->list));
    work->work_func = work_func;
    work->work_data = work_data;
    work->workqueue = NULL;
    work->flags = 0;
    work->type = 0;
}

static int _workqueue_work_completion(hal_workqueue *queue)
{
    return hal_sem_post(queue->sem);
}

static void _workqueue_thread_entry(void *parameter)
{
    unsigned long flag;

    hal_work *work;
    hal_workqueue *queue;

    queue = (hal_workqueue *) parameter;
    hal_assert(queue != NULL);

    while (1)
    {
        if (list_empty(&(queue->work_list)))
        {
            vTaskSuspend(NULL);
        }

        flag = workqueue_list_lock(queue);
        if (list_empty(&(queue->work_list))){
            workqueue_list_unlock(queue, flag);
            continue;
        }
        work = list_entry(queue->work_list.next, hal_work, list);
        list_del_init(&(work->list));
        hal_sem_clear(queue->sem);
        queue->work_current = work;
        work->flags &= ~HAL_WORK_STATE_PENDING;
        work->workqueue = NULL;
        workqueue_list_unlock(queue, flag);

        work->work_func(work, work->work_data);

        flag = workqueue_list_lock(queue);
        queue->work_current = NULL;
        workqueue_list_unlock(queue, flag);

        _workqueue_work_completion(queue);
    }
    vTaskDelete(NULL);
}

static int _workqueue_submit_work(hal_workqueue *queue, hal_work *work)
{
    unsigned long flag;
    BaseType_t ret = pdFALSE;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    flag = workqueue_list_lock(queue);
    if (work->flags & HAL_WORK_STATE_PENDING)
    {
        workqueue_list_unlock(queue, flag);
        return HAL_BUSY;
    }

    if (queue->work_current == work)
    {
        workqueue_list_unlock(queue, flag);
        return HAL_BUSY;
    }

    //list_del(&(work->list));

    list_add_tail(&(work->list), &queue->work_list);
    work->flags |= HAL_WORK_STATE_PENDING;

    if (queue->work_current == NULL)
    {
        workqueue_list_unlock(queue, flag);
        if (hal_interrupt_get_nest()) {
            ret = xTaskResumeFromISR(queue->work_thread);
            if (ret == pdTRUE) {
                portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
            }
        } else {
            vTaskResume(queue->work_thread);
        }
    }
    else
    {
        workqueue_list_unlock(queue, flag);
    }

    return HAL_OK;
}

static int _workqueue_cancel_work(hal_workqueue *queue, hal_work *work)
{
    unsigned long flag;

    flag = workqueue_list_lock(queue);
    if (queue->work_current == work)
    {
        workqueue_list_unlock(queue, flag);
        return HAL_BUSY;
    }
    list_del_init(&(work->list));
    work->flags &= ~HAL_WORK_STATE_PENDING;
    workqueue_list_unlock(queue, flag);

    return HAL_OK;
}

static int _workqueue_cancel_delayed_work(hal_work *work)
{
    unsigned long flag;
    int ret = HAL_OK;

    if (!work->workqueue)
    {
        ret = HAL_INVALID;
        goto __exit;
    }

    if (work->flags & HAL_WORK_STATE_PENDING)
    {
        ret = _workqueue_cancel_work(work->workqueue, work);
        if (ret)
        {
            goto __exit;
        }
    }
    else
    {
        if (work->flags & HAL_WORK_STATE_SUBMITTING)
        {
            flag = workqueue_list_lock(work->workqueue);
            osal_timer_stop(work->timer);
            osal_timer_delete(work->timer);
            work->timer = NULL;
            work->flags &= ~HAL_WORK_STATE_SUBMITTING;
            workqueue_list_unlock(work->workqueue, flag);
        }
    }

    flag = workqueue_list_lock(work->workqueue);
    work->workqueue = NULL;
    work->flags &= ~(HAL_WORK_STATE_PENDING);
    workqueue_list_unlock(work->workqueue, flag);

__exit:
    return ret;
}

static int _workqueue_submit_delayed_work(hal_workqueue *queue,
        hal_work *work, hal_tick_t ticks)
{
    unsigned long flag;
    int ret = HAL_OK;

    if (work->workqueue && work->workqueue != queue)
    {
        ret = HAL_INVALID;
        goto __exit;
    }

    if (work->workqueue == queue)
    {
        ret = _workqueue_cancel_delayed_work(work);
        if (ret < 0)
        {
            goto __exit;
        }
    }

    flag = workqueue_list_lock(queue);
    work->workqueue = queue;
    workqueue_list_unlock(queue, flag);

    if (!ticks)
    {
        ret = _workqueue_submit_work(work->workqueue, work);
    }
    else
    {
        flag = workqueue_list_lock(queue);
        work->flags |= HAL_WORK_STATE_SUBMITTING;
        work->timer = osal_timer_create("work", _delayed_work_timeout_handler, work, ticks, OSAL_TIMER_FLAG_ONE_SHOT | OSAL_TIMER_FLAG_SOFT_TIMER);
        workqueue_list_unlock(queue, flag);
        osal_timer_start(work->timer);
    }

__exit:
    return ret;
}

static void _delayed_work_timeout_handler(void *parameter)
{
    hal_work *delayed_work;
    unsigned long flag;

    delayed_work = (hal_work *)parameter;
    flag = workqueue_list_lock(delayed_work->workqueue);
    osal_timer_stop(delayed_work->timer);
    osal_timer_delete(delayed_work->timer);
    delayed_work->timer = NULL;
    delayed_work->flags &= ~HAL_WORK_STATE_SUBMITTING;
    delayed_work->type &= ~HAL_WORK_TYPE_DELAYED;
    workqueue_list_unlock(delayed_work->workqueue, flag);
    _workqueue_submit_work(delayed_work->workqueue, delayed_work);
}

hal_workqueue *hal_workqueue_create(const char *name, uint16_t stack_size, uint8_t priority)
{
    hal_workqueue *queue = NULL;

    queue = (hal_workqueue *)malloc(sizeof(hal_workqueue));
    if (queue != NULL)
    {
        INIT_LIST_HEAD(&(queue->work_list));
        queue->work_current = NULL;
        queue->sem = hal_sem_create(0);

        queue->work_thread = hal_thread_create(_workqueue_thread_entry, queue, name, stack_size, priority);
        if (queue->work_thread == NULL)
        {
            if (queue->sem) {
                hal_sem_delete(queue->sem);
            }
            free(queue);
            return NULL;
        }

        hal_thread_start(queue->work_thread);
    }

    return queue;
}

int hal_workqueue_destroy(hal_workqueue *queue)
{
    hal_assert(queue != NULL);

    vTaskDelete(queue->work_thread);
    if (queue->sem) {
        hal_sem_delete(queue->sem);
    }
    free(queue);

    return HAL_OK;
}

int hal_workqueue_dowork(hal_workqueue *queue, hal_work *work)
{
    hal_assert(queue != NULL);
    hal_assert(work != NULL);

    return _workqueue_submit_work(queue, work);
}

int hal_workqueue_submit_work(hal_workqueue *queue, hal_work *work, hal_tick_t time)
{
    hal_assert(queue != NULL);
    hal_assert(work != NULL);

    if (time > 0)
    {
        work->type |= HAL_WORK_TYPE_DELAYED;
    }

    if (work->type & HAL_WORK_TYPE_DELAYED)
    {
        return _workqueue_submit_delayed_work(queue, work, time);
    }
    else
    {
        return _workqueue_submit_work(queue, work);
    }
}

int hal_workqueue_critical_work(hal_workqueue *queue, hal_work *work)
{
    unsigned long flag;
    hal_assert(queue != NULL);
    hal_assert(work != NULL);

    BaseType_t ret = pdFALSE;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    flag = workqueue_list_lock(queue);
    if (queue->work_current == work)
    {
        workqueue_list_unlock(queue, flag);
        return HAL_BUSY;
    }

    list_del_init(&(work->list));

    list_add_tail(&(work->list), &queue->work_list);
    if (queue->work_current == NULL)
    {
        workqueue_list_unlock(queue, flag);
        if (hal_interrupt_get_nest()) {
            ret = xTaskResumeFromISR(queue->work_thread);
            if (ret == pdTRUE) {
                portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
            }
        } else {
            vTaskResume(queue->work_thread);
        }
    }
    else
    {
        workqueue_list_unlock(queue, flag);
    }

    return HAL_OK;
}

int hal_workqueue_cancel_work(hal_workqueue *queue, hal_work *work)
{
    hal_assert(queue != NULL);
    hal_assert(work != NULL);

    if (work->type & HAL_WORK_TYPE_DELAYED)
    {
        return _workqueue_cancel_delayed_work(work);
    }
    else
    {
        return _workqueue_cancel_work(queue, work);
    }
}

int hal_workqueue_cancel_work_sync(hal_workqueue *queue, hal_work *work)
{
    unsigned long flag;

    hal_assert(queue != NULL);
    hal_assert(work != NULL);

    flag = workqueue_list_lock(queue);
    if (queue->work_current == work)
    {
        workqueue_list_unlock(queue, flag);
        hal_sem_timedwait(queue->sem, HAL_WAIT_FOREVER);
    }
    else
    {
        list_del_init(&(work->list));
        workqueue_list_unlock(queue, flag);
    }
    flag = workqueue_list_lock(queue);
    work->flags &= ~HAL_WORK_STATE_PENDING;
    workqueue_list_unlock(queue, flag);

    return HAL_OK;
}

int hal_workqueue_cancel_all_work(hal_workqueue *queue)
{
    struct list_head *node, *next;
    unsigned long flag;
    hal_assert(queue != NULL);

    flag = workqueue_list_lock(queue);
    for (node = queue->work_list.next; node != &(queue->work_list); node = next)
    {
        next = node->next;
        list_del_init(node);
    }
    workqueue_list_unlock(queue, flag);

    return HAL_OK;
}

void hal_delayed_work_init(hal_delayed_work *work, void (*work_func)(hal_work *work,
                          void *work_data), void *work_data)
{
    hal_work_init(&work->work, work_func, work_data);
}
