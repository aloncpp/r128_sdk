/*
 * Copyright (c) 2010-2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file  queue.c
 *
 * @brief dynamic-size QUEUE object.
 */

#include <zephyr.h>
#include <misc/__assert.h>
#include <toolchain.h>
#include <misc/sflist.h>
#include <init.h>

struct alloc_node {
	sys_sfnode_t node;
	void *data;
};

void *z_queue_node_peek(sys_sfnode_t *node, bool needs_free)
{
	void *ret;

	if (node && sys_sfnode_flags_get(node)) {
		/* If the flag is set, then the enqueue operation for this item
		 * did a behind-the scenes memory allocation of an alloc_node
		 * struct, which is what got put in the queue. Free it and pass
		 * back the data pointer.
		 */
		struct alloc_node *anode;

		anode = CONTAINER_OF(node, struct alloc_node, node);
		ret = anode->data;
		printf("[Debug] %p %p\n", ret, anode->data);
		if (needs_free) {
			k_free(anode);
		}
	} else {
		/* Data was directly placed in the queue, the first 4 bytes
		 * reserved for the linked list. User mode isn't allowed to
		 * do this, although it can get data sent this way.
		 */
		ret = (void *)node;
	}

	return ret;
}

#ifdef CONFIG_OBJECT_TRACING

extern struct k_queue _k_queue_list_start[];
extern struct k_queue _k_queue_list_end[];

struct k_queue *_trace_list_k_queue;

/*
 * Complete initialization of statically defined queues.
 */
static int init_queue_module(struct device *dev)
{
	ARG_UNUSED(dev);

	struct k_queue *queue;

	for (queue = (k_queue *)_k_queue_list_start; queue < (k_queue *)_k_queue_list_end; queue++) {
		SYS_TRACING_OBJ_INIT(k_queue, queue);
	}
	return 0;
}

SYS_INIT(init_queue_module, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);

#endif /* CONFIG_OBJECT_TRACING */

void k_queue_init(struct k_queue *queue)
{
	sys_sflist_init(&queue->data_q);
#if defined(CONFIG_POLL)
	sys_dlist_init(&queue->poll_events);
#else //wait_q or poll_events by CONFIG_POLL
	z_waitq_init(&queue->wait_q);
#endif

	SYS_TRACING_OBJ_INIT(k_queue, queue);
	_k_object_init(queue);
}

#if defined(CONFIG_BT_DEINIT)
void k_queue_deinit(struct k_queue *queue)
{
	sys_sflist_deinit(&queue->data_q);
#if defined(CONFIG_POLL)
	sys_dlist_deinit(&queue->poll_events);
#else //wait_q or poll_events by CONFIG_POLL
	z_waitq_deinit_n(&queue->wait_q);
#endif

	SYS_TRACING_OBJ_DEINIT(k_queue, queue);
	_k_object_deinit(queue);
}
#endif

#if !defined(CONFIG_POLL)
static void prepare_thread_to_run(struct k_queue *queue, void *data)
{
	XR_OS_SemaphoreRelease(&queue->wait_q.waitq);
}
#endif /* CONFIG_POLL */

#ifdef CONFIG_POLL
static inline void handle_poll_events(struct k_queue *queue, u32_t state)
{
	z_handle_obj_poll_events(&queue->poll_events, state);
}
#endif

void k_queue_cancel_wait(struct k_queue *queue)
{
	unsigned int key = irq_lock();
#if !defined(CONFIG_POLL)
	k_queue_prepend(queue, NULL);
#else
	handle_poll_events(queue, K_POLL_STATE_CANCELLED);
#endif /* !CONFIG_POLL */

	z_reschedule(key);
}

static int queue_insert(struct k_queue *queue, void *prev, void *data,
			bool alloc)
{
	unsigned int key = irq_lock();
#if !defined(CONFIG_POLL)
	prepare_thread_to_run(queue, NULL);
#endif /* !CONFIG_POLL */

	/* Only need to actually allocate if no threads are pending */
	if (alloc) {
		struct alloc_node *anode;

		anode = z_thread_malloc(sizeof(*anode));
		if (!anode) {
			irq_unlock(key);
			return -ENOMEM;
		}
		anode->data = data;
		sys_sfnode_init(&anode->node, 0x1);
		data = anode;
	} else {
		sys_sfnode_init(data, 0x0);
	}

	sys_sflist_insert(&queue->data_q, prev, data);

#if defined(CONFIG_POLL)
	handle_poll_events(queue, K_POLL_STATE_DATA_AVAILABLE);
#endif /* CONFIG_POLL */

	z_reschedule(key);
	return 0;
}

void k_queue_insert(struct k_queue *queue, void *prev, void *data)
{
	(void)queue_insert(queue, prev, data, false);
}

void k_queue_append(struct k_queue *queue, void *data)
{
	(void)queue_insert(queue, sys_sflist_peek_tail(&queue->data_q),
			   data, false);
}

void k_queue_prepend(struct k_queue *queue, void *data)
{
	(void)queue_insert(queue, NULL, data, false);
}

int k_queue_alloc_append(struct k_queue *queue, void *data)
{
	return queue_insert(queue, sys_sflist_peek_tail(&queue->data_q), data,
			    true);
}

int k_queue_alloc_prepend(struct k_queue *queue, void *data)
{
	return queue_insert(queue, NULL, data, true);
}

int k_queue_append_list(struct k_queue *queue, void *head, void *tail)
{
	__ASSERT(head && tail, "invalid head or tail");

	unsigned int key = irq_lock();
#if !defined(CONFIG_POLL)
	sys_sflist_append_list(&queue->data_q, head, tail);
	prepare_thread_to_run(queue, NULL);
#else
	sys_sflist_append_list(&queue->data_q, head, tail);
	handle_poll_events(queue, K_POLL_STATE_DATA_AVAILABLE);
#endif /* !CONFIG_POLL */

	z_reschedule(key);

	return 0;
}

void k_queue_merge_slist(struct k_queue *queue, sys_slist_t *list)
{
	__ASSERT(!sys_slist_is_empty(list), "list must not be empty");

	/*
	 * note: this works as long as:
	 * - the slist implementation keeps the next pointer as the first
	 *   field of the node object type
	 * - list->tail->next = NULL.
	 * - sflist implementation only differs from slist by stuffing
	 *   flag bytes in the lower order bits of the data pointer
	 * - source list is really an slist and not an sflist with flags set
	 */
	k_queue_append_list(queue, list->head, list->tail);
	sys_slist_init(list);
}

#if defined(CONFIG_POLL)
static void *k_queue_poll(struct k_queue *queue, s32_t timeout)
{
	struct k_poll_event event;
	int err, elapsed = 0, done = 0;
	unsigned int key;
	void *val;
	u32_t start;

	k_poll_event_init(&event, K_POLL_TYPE_FIFO_DATA_AVAILABLE,
			  K_POLL_MODE_NOTIFY_ONLY, queue);

	if (timeout != K_FOREVER) {
		start = k_uptime_get_32();
	}

	do {
		event.state = K_POLL_STATE_NOT_READY;

		err = k_poll(&event, 1, timeout - elapsed);

		if (err && err != -EAGAIN) {
			return NULL;
		}

		/* sys_sflist_* aren't threadsafe, so must be always protected
		 * by irq_lock.
		 */
		key = irq_lock();
		val = z_queue_node_peek(sys_sflist_get(&queue->data_q), true);
		irq_unlock(key);

		if (!val && timeout != K_FOREVER) {
			elapsed = k_uptime_get_32() - start;
			done = elapsed > timeout;
		}
	} while (!val && !done);

	return val;
}
#endif /* CONFIG_POLL */

void *k_queue_get(struct k_queue *queue, k_timeout_t timeout)
{
	unsigned int key = irq_lock();
	void *data;

	if (likely(!sys_sflist_is_empty(&queue->data_q))) {
		sys_sfnode_t *node;

		node = sys_sflist_get_not_empty(&queue->data_q);
		data = z_queue_node_peek(node, true);
		irq_unlock(key);
		return data;
	}

	if (timeout == K_NO_WAIT) {
		irq_unlock(key);
		return NULL;
	}

#if defined(CONFIG_POLL)
	irq_unlock(key);
	//printf("%s %d \n", __func__, __LINE__);
	return k_queue_poll(queue, timeout);

#else
	int ret = XR_OS_OK;

	/*
	 * Warning! This might cause the thread block forever for I don't really
	 * think perfectly, but CONFIG_POLL should open so I am not care too much.
	 */
	irq_unlock(key);
	while (likely(sys_sflist_is_empty(&queue->data_q)) && ret == XR_OS_OK)
		ret = XR_OS_SemaphoreWait(&queue->wait_q.waitq, timeout);

	key = irq_lock();
	if (likely(!sys_sflist_is_empty(&queue->data_q))) {
		sys_sfnode_t *node;

		node = sys_sflist_get_not_empty(&queue->data_q);
		data = z_queue_node_peek(node, true);
		irq_unlock(key);
		return data;
	}
	irq_unlock(key);
	return NULL;
#endif /* CONFIG_POLL */
}
