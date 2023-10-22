/*
 * Copyright (c) 2017 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file  poll.c
 *
 * @brief Kernel asynchronous event polling interface.
 *
 * This polling mechanism allows waiting on multiple events concurrently,
 * either events triggered directly, or from kernel objects or other kernel
 * constructs.
 */

#include <zephyr.h>
#include <zephyr/types.h>
#include <misc/slist.h>
#include <misc/dlist.h>
#include <misc/util.h>
#include <misc/__assert.h>

void k_poll_event_init(struct k_poll_event *event, u32_t type,
		       int mode, void *obj)
{
	__ASSERT(mode == K_POLL_MODE_NOTIFY_ONLY,
		 "only NOTIFY_ONLY mode is supported\n");
	__ASSERT(type < (BIT(_POLL_NUM_TYPES)), "invalid type\n");
	__ASSERT(obj, "must provide an object\n");

	event->poller = NULL;
	/* event->tag is left uninitialized: the user will set it if needed */
	event->type = type;
	event->state = K_POLL_STATE_NOT_READY;
	event->mode = mode;
	event->unused = 0;
	event->obj = obj;
}

/* must be called with interrupts locked */
static inline int is_condition_met(struct k_poll_event *event, u32_t *state)
{
	switch (event->type) {
	case K_POLL_TYPE_SEM_AVAILABLE:
		if (k_sem_count_get(event->sem) > 0) {
			*state = K_POLL_STATE_SEM_AVAILABLE;
			return 1;
		}
		break;
	case K_POLL_TYPE_DATA_AVAILABLE:
		if (!k_queue_is_empty(event->queue)) {
			*state = K_POLL_STATE_FIFO_DATA_AVAILABLE;
			return 1;
		}
		break;
	case K_POLL_TYPE_SIGNAL:
		if (event->signal->signaled) {
			*state = K_POLL_STATE_SIGNALED;
			return 1;
		}
		break;
	case K_POLL_TYPE_IGNORE:
		return 0;
	default:
		__ASSERT(0, "invalid event type (0x%x)\n", event->type);
	}

	return 0;
}

static inline void add_event(sys_dlist_t *events, struct k_poll_event *event,
			     struct _poller *poller)
{
	struct k_poll_event *pending;

	pending = (struct k_poll_event *)sys_dlist_peek_tail(events);
	if (!pending /*|| _is_t1_higher_prio_than_t2(pending->poller->thread,
						   poller->thread)*/) {
		sys_dlist_append(events, &event->_node);
		return;
	}

	SYS_DLIST_FOR_EACH_CONTAINER(events, pending, _node) {
		if (0/*_is_t1_higher_prio_than_t2(poller->thread,
					       pending->poller->thread)*/) {
			sys_dlist_insert_before(events, &pending->_node,
						&event->_node);
			return;
		}
	}

	sys_dlist_append(events, &event->_node);
}

/* must be called with interrupts locked */
static inline int register_event(struct k_poll_event *event,
				 struct _poller *poller)
{
	switch (event->type) {
	case K_POLL_TYPE_SEM_AVAILABLE:
		__ASSERT(event->sem, "invalid semaphore\n");
		add_event(&event->sem->poll_events, event, poller);
		break;
	case K_POLL_TYPE_DATA_AVAILABLE:
		__ASSERT(event->queue, "invalid queue\n");
		add_event(&event->queue->poll_events, event, poller);
		break;
	case K_POLL_TYPE_SIGNAL:
		__ASSERT(event->signal, "invalid poll signal\n");
		add_event(&event->signal->poll_events, event, poller);
		break;
	case K_POLL_TYPE_IGNORE:
		/* nothing to do */
		break;
	default:
		__ASSERT(0, "invalid event type\n");
	}

	event->poller = poller;

	return 0;
}

/* must be called with interrupts locked */
static inline void clear_event_registration(struct k_poll_event *event)
{
	bool remove = false;

	event->poller = NULL;

	switch (event->type) {
	case K_POLL_TYPE_SEM_AVAILABLE:
		__ASSERT(event->sem != NULL, "invalid semaphore\n");
		remove = true;
		break;
	case K_POLL_TYPE_DATA_AVAILABLE:
		__ASSERT(event->queue != NULL, "invalid queue\n");
		remove = true;
		break;
	case K_POLL_TYPE_SIGNAL:
		__ASSERT(event->signal != NULL, "invalid poll signal\n");
		remove = true;
		break;
	case K_POLL_TYPE_IGNORE:
		/* nothing to do */
		break;
	default:
		__ASSERT(false, "invalid event type\n");
		break;
	}
	if (remove && sys_dnode_is_linked(&event->_node)) {
		sys_dlist_remove(&event->_node);
	}
}

/* must be called with interrupts locked */
static inline void clear_event_registrations(struct k_poll_event *events,
					      int num_events,
					      unsigned int key)
{
	while (num_events--) {
		clear_event_registration(&events[num_events]);
		irq_unlock(key);
		key = irq_lock();
	}
}

static inline void set_event_ready(struct k_poll_event *event, u32_t state)
{
	event->poller = NULL;
	event->state |= state;
}

static inline int register_events(struct k_poll_event *events,
				  int num_events,
				  struct _poller *poller,
				  bool just_check)
{
	int events_registered = 0;

	for (int ii = 0; ii < num_events; ii++) {
		unsigned int key;
		u32_t state;

		key = irq_lock();
		if (is_condition_met(&events[ii], &state)) {
			set_event_ready(&events[ii], state);
			poller->is_polling = false;
		} else if (!just_check && poller->is_polling) {
			int rc = register_event(&events[ii], poller);
			if (rc == 0) {
				events_registered += 1;
			} else {
				__ASSERT(false, "unexpected return code\n");
			}
		}
		irq_unlock(key);
	}

	return events_registered;
}

static int k_poll_poller_cb(struct k_poll_event *event, u32_t state)
{
	if (state == K_POLL_STATE_CANCELLED)
		event->poller->state = -EINTR;

	if (event->poller->wait != NULL)
		XR_OS_SemaphoreRelease(event->poller->wait);

	return 0;
}

int k_poll(struct k_poll_event *events, int num_events, k_timeout_t timeout)
{
	__ASSERT(!arch_is_in_isr(), " ");
	__ASSERT(events, "NULL events\n");
	__ASSERT(num_events > 0, "zero events\n");

	int events_registered;
	unsigned int key;

	struct _poller poller = { .is_polling = 1,
				  .thread     = NULL,
				  .cb         = k_poll_poller_cb };

	events_registered = register_events(events, num_events, &poller,
					    (timeout == K_NO_WAIT));

	key = irq_lock();

	/*
	 * If events happen after register_events() but before irq_lock(),
	 * a event trigger might be missed causing sem pend forever. But the
	 * original code design seems like actually has this bug. So, we check
	 * the events state here for save.
	 */
	for (int ii = 0; ii < num_events; ii++) {
		if (events[ii].state != K_POLL_STATE_NOT_READY) {
			poller.is_polling = 0;
			break;
		}
	}

	/*
	 * If we're not polling anymore, it means that at least one event
	 * condition is met, either when looping through the events here or
	 * because one of the events registered has had its state changed.
	 */
	if (!poller.is_polling) {
		clear_event_registrations(events, events_registered, key);
		irq_unlock(key);
		return 0;
	}

	poller.is_polling = 0;

	if (timeout == K_NO_WAIT) {
		irq_unlock(key);
		return -EAGAIN;
	}

    XR_OS_Semaphore_t sem;
	XR_OS_SemaphoreCreate(&sem, 0, XR_OS_SEMAPHORE_MAX_COUNT);
	poller.wait = &sem;
	irq_unlock(key);

	int swap_rc = XR_OS_SemaphoreWait(&sem, timeout);
	swap_rc = (swap_rc != XR_OS_OK) ? -EAGAIN : poller.state;

	poller.wait = NULL;
	XR_OS_SemaphoreDelete(&sem);

	/*
	 * Clear all event registrations. If events happen while we're in this
	 * loop, and we already had one that triggered, that's OK: they will
	 * end up in the list of events that are ready; if we timed out, and
	 * events happen while we're in this loop, that is OK as well since
	 * we've already know the return code (-EAGAIN), and even if they are
	 * added to the list of events that occurred, the user has to check the
	 * return code first, which invalidates the whole list of event states.
	 */
	key = irq_lock();
	clear_event_registrations(events, events_registered, key);
	irq_unlock(key);

	return swap_rc;
}

/* must be called with interrupts locked */
static int signal_poll_event(struct k_poll_event *event, u32_t state)
{
	struct _poller *poller = event->poller;
	int retcode = 0;

	if (poller) {
		if (poller->cb != NULL) {
			retcode = poller->cb(event, state);
		}

		poller->is_polling = false;

		if (retcode < 0) {
			return retcode;
		}
	}

	set_event_ready(event, state);
	return 0;
}

void z_handle_obj_poll_events(sys_dlist_t *events, u32_t state)
{
	struct k_poll_event *poll_event;

	poll_event = (struct k_poll_event *)sys_dlist_get(events);
	if (poll_event) {
		(void) signal_poll_event(poll_event, state);
	}
}

void k_poll_signal_init(struct k_poll_signal *signal)
{
	sys_dlist_init(&signal->poll_events);
	signal->signaled = 0;
	/* signal->result is left unitialized */
	_k_object_init(signal);
}

void k_poll_signal_reset(struct k_poll_signal *signal)
{
	signal->signaled = 0;
}

void k_poll_signal_check(struct k_poll_signal *signal,
			       unsigned int *signaled, int *result)
{
	*signaled = signal->signaled;
	*result = signal->result;
}

int k_poll_signal(struct k_poll_signal *signal, int result)
{
	unsigned int key = irq_lock();
	struct k_poll_event *poll_event;

	signal->result = result;
	signal->signaled = 1;

	poll_event = (struct k_poll_event *)sys_dlist_get(&signal->poll_events);
	if (!poll_event) {
		irq_unlock(key);
		return 0;
	}

	int rc = signal_poll_event(poll_event, K_POLL_STATE_SIGNALED);

	z_reschedule(key);
	return rc;
}


