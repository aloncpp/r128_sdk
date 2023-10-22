/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file work_q.c
 *
 * Workqueue support functions
 */

#include <zephyr.h>
//#include <kernel_structs.h>
//#include <wait_q.h>
#include <errno.h>
#include <stdio.h>

/*************************************************************************************************************/
/* system_work_q.c */
/*************************************************************************************************************/

//K_THREAD_STACK_DEFINE(sys_work_q_stack, CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE);


struct k_work_q k_sys_work_q = {0};

int k_sys_work_q_init(struct device *dev)
{
	ZBLUE_ARG_UNUSED(dev);

	k_work_q_start(&k_sys_work_q,
		       NULL/*sys_work_q_stack*/,
		       /*K_THREAD_STACK_SIZEOF(sys_work_q_stack)*/CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE,
		       CONFIG_SYSTEM_WORKQUEUE_PRIORITY);

	return 0;
}

#if defined(CONFIG_BT_DEINIT)
int k_sys_work_q_deinit(struct device *dev)
{
	ZBLUE_ARG_UNUSED(dev);

	k_work_q_stop(&k_sys_work_q);

	return 0;
}
#endif

/*************************************************************************************************************/
/* os/work_q.c */
/*************************************************************************************************************/

static void z_work_q_main(void *work_q_ptr, void *p2, void *p3)
{
	struct k_work_q *work_q = work_q_ptr;
#if defined(CONFIG_BT_DEINIT)
	struct k_work *cancel_work;
#endif

	ZBLUE_ARG_UNUSED(p2);
	ZBLUE_ARG_UNUSED(p3);

	//printf("~~[%s] stack used %d\n", __func__, uxTaskGetStackHighWaterMark(k_sys_work_q.thread.task));
	while (1) {
		struct k_work *work;
		k_work_handler_t handler;

		work = k_queue_get(&work_q->queue, K_FOREVER);
		if (work == NULL) {
			continue;
		}

#if defined(CONFIG_BT_DEINIT)
		if (work->handler == NULL) {
			goto exit;
		}
#endif

		handler = work->handler;

		/* Reset pending state so it can be resubmitted by handler */
		if (atomic_test_and_clear_bit(work->flags,
					      K_WORK_STATE_PENDING)) {
			handler(work);
		}

		/* Make sure we don't hog up the CPU if the FIFO never (or
		 * very rarely) gets empty.
		 */
		k_yield();
	}

#if defined(CONFIG_BT_DEINIT)
exit:
	while ((cancel_work = k_queue_get(&work_q->queue, K_NO_WAIT))) {
		atomic_clear_bit(cancel_work->flags, K_WORK_STATE_PENDING);
	}
#endif
}

void k_work_q_start(struct k_work_q *work_q, k_thread_stack_t *stack,
		    size_t stack_size, int prio)
{
	k_queue_init(&work_q->queue);
	k_thread_create(&work_q->thread, stack, "zbt_work_q", stack_size, z_work_q_main,
			work_q, 0, 0, prio, 0, 0);
}

#if defined(CONFIG_BT_DEINIT)
struct k_work work_q_stop_work;
void k_work_q_stop(struct k_work_q *work_q)
{
	k_work_init(&work_q_stop_work, NULL);

	k_work_submit(&work_q_stop_work);

	k_thread_join(&k_sys_work_q.thread, K_FOREVER);

	k_queue_deinit(&work_q->queue);
	k_work_deinit(&work_q_stop_work);
}
#endif

#ifdef CONFIG_SYS_CLOCK_EXISTS
static void work_timeout(struct _timeout *t)
{
	struct k_delayed_work *w = CONTAINER_OF(t, struct k_delayed_work,
							   timeout);

	/* submit work to workqueue */
	k_work_submit_to_queue(w->work_q, &w->work);
}

void k_delayed_work_init(struct k_delayed_work *work, k_work_handler_t handler)
{
	k_work_init(&work->work, handler);
	z_init_timeout(&work->timeout);
	work->work_q = NULL;
}

#if defined(CONFIG_BT_DEINIT)
void k_delayed_work_deinit(struct k_delayed_work *work)
{
	k_work_deinit(&work->work);
	z_init_timeout(&work->timeout);
	work->work_q = NULL;
}
#endif

void k_work_submit_to_queue(struct k_work_q *work_q,struct k_work *work)
{
	if (!atomic_test_and_set_bit(work->flags, K_WORK_STATE_PENDING)) {
		k_queue_append(&work_q->queue, work);
	}
}

int k_delayed_work_submit_to_queue(struct k_work_q *work_q,
				   struct k_delayed_work *work,
				   s32_t delay)
{
	/* Warning, using OS interface while interrupt disable  */
	unsigned int key = irq_lock();
	int err = 0;

	/* Work cannot be active in multiple queues */
	if (work->work_q && work->work_q != work_q) {
		err = -EADDRINUSE;
		goto done;
	}

	/* Cancel if work has been submitted */
	if (work->work_q == work_q) {
		err = k_delayed_work_cancel(work);
		if (err < 0) {
			goto done;
		}
	}

	/* Attach workqueue so the timeout callback can submit it */
	work->work_q = work_q;

	/* Submit work directly if no delay.  Note that this is a
	 * blocking operation, so release the lock first.
	 */
	if (delay == 0) {
		/* Submit work if no ticks is 0 */
		k_work_submit_to_queue(work_q, &work->work);
		irq_unlock(key);
		return err;
	}

	/* Add timeout */
	z_add_timeout(&work->timeout, work_timeout,
			_TICK_ALIGN + k_ms_to_ticks_ceil32(delay));

done:
	irq_unlock(key);
	return err;
}

int k_delayed_work_cancel(struct k_delayed_work *work)
{
	if (!work->work_q) {
		return -EINVAL;
	}

	unsigned int key = irq_lock();

	if (k_work_pending(&work->work)) {
		/* Remove from the queue if already submitted */
		if (!k_queue_remove(&work->work_q->queue, &work->work)) {
			irq_unlock(key);
			return -EINVAL;
		}
	} else {
		z_abort_timeout(&work->timeout);
	}

	/* Detach from workqueue */
	work->work_q = NULL;

	atomic_clear_bit(work->work.flags, K_WORK_STATE_PENDING);
	irq_unlock(key);

	return 0;
}

// bool k_delayed_work_pending(struct k_delayed_work *work)
// {
// 	return !z_is_inactive_timeout(&work->timeout) ||
// 	       k_work_pending(&work->work);
// }

#endif /* CONFIG_SYS_CLOCK_EXISTS */
