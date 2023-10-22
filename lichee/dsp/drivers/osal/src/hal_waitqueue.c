#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <aw_list.h>
#include <hal_interrupt.h>
#include <hal_thread.h>
#include <hal_waitqueue.h>

static int autoremove_wake_function(hal_waitqueue_t *wait, void *key)
{
	/* return 0 if wake up successed */
	if (hal_sem_post(&wait->sem) == HAL_OK) {
		list_del_init(&wait->task_list);
		return 0;
	}
	return 1;
}

void init_wait_entry(hal_waitqueue_t *wait, unsigned int flags)
{
	wait->flags = flags;
	wait->func = autoremove_wake_function;
	INIT_LIST_HEAD(&wait->task_list);
	hal_sem_init(&wait->sem, 0);
}

long prepare_to_wait_event(hal_waitqueue_head_t *q, hal_waitqueue_t *wait)
{
	unsigned long flags;
	long ret = 0;

	flags = hal_spin_lock_irqsave(&q->lock);
	if (list_empty(&wait->task_list)) {
		if (wait->flags & HAL_WQ_FLAG_EXCLUSIVE)
			__add_hal_waitqueue_tail(q, wait);
		else
			__add_wait_queue(q, wait);
	}
	else {
		ret = -EALREADY;
	}
	hal_spin_unlock_irqrestore(&q->lock, flags);

	return ret;
}

void finish_wait(hal_waitqueue_head_t *q, hal_waitqueue_t *wait)
{
	unsigned long flags;

	if (!list_empty_careful(&wait->task_list)) {
		flags = hal_spin_lock_irqsave(&q->lock);
		list_del_init(&wait->task_list);
		hal_sem_deinit(&wait->sem);
		hal_spin_unlock_irqrestore(&q->lock, flags);
	}
}

static void __wake_up_common(hal_waitqueue_head_t *q, int nr_exclusive,
				void *key)
{
	hal_waitqueue_t *curr, *next;

	list_for_each_entry_safe(curr, next, &q->head, task_list) {
	unsigned flags = curr->flags;

	if (!curr->func(curr, key) &&
		(flags & HAL_WQ_FLAG_EXCLUSIVE) && !--nr_exclusive)
		break;
	}
}

void __hal_wake_up(hal_waitqueue_head_t *q, int nr_exclusive, void *key)
{
	unsigned long flags;

	flags = hal_spin_lock_irqsave(&q->lock);
	__wake_up_common(q, nr_exclusive, key);
	hal_spin_unlock_irqrestore(&q->lock, flags);
}
