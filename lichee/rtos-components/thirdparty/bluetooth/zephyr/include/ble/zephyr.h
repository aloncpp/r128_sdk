/*
 * Copyright (c) 2016, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Public kernel APIs.
 */

#ifndef ZEPHYR_INCLUDE_KERNEL_H_
#define ZEPHYR_INCLUDE_KERNEL_H_

#include <stdlib.h>
#include <string.h>

#include <FreeRTOS.h>
#include <task.h>

#include "kernel/os/os.h"

#include <kernel_includes.h>
#include <errno.h>
#include <stdbool.h>
#include <ble/toolchain.h>

#include "kernel/os/os_semaphore.h"
#define OS_SEMAPHORE_MAX_COUNT XR_OS_SEMAPHORE_MAX_COUNT
#define OS_Semaphore_t XR_OS_Semaphore_t
#define OS_SemaphoreCreate XR_OS_SemaphoreCreate
#define OS_SemaphoreDelete XR_OS_SemaphoreDelete
#define OS_SemaphoreWait XR_OS_SemaphoreWait
#define OS_SemaphoreRelease XR_OS_SemaphoreRelease
#include "kernel/os/os_thread.h"
#define OS_Thread_t XR_OS_Thread_t
#define OS_ThreadYield XR_OS_ThreadYield
#include "kernel/os/os_timer.h"
#define OS_Timer_t XR_OS_Timer_t
#include "kernel/os/os_mutex.h"
#define OS_Mutex_t XR_OS_Mutex_t
#include "kernel/os/os_time.h"
#define OS_Time_t XR_OS_Time_t
#define OS_MSecsToTicks XR_OS_MSecsToTicks
#define OS_TicksToMSecs XR_OS_TicksToMSecs

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_TICKLESS_KERNEL
#define _TICK_ALIGN 0
#else
#define _TICK_ALIGN 1
#endif

#ifndef UNUSED
#define UNUSED(x) (void)x
#endif

#ifdef CONFIG_KERNEL_DEBUG
#define K_DEBUG(fmt, ...) printf("[%s]  " fmt, __func__, ##__VA_ARGS__)
#else
#define K_DEBUG(fmt, ...)
#endif

#define K_PRIO_COOP(x) (x)
#define K_PRIO_PREEMPT(x) (x)

#define K_HIGHEST_THREAD_PRIO (7 - 1) /* configMAX_PRIORITIES */
#define K_LOWEST_THREAD_PRIO  0

#define K_IDLE_PRIO K_LOWEST_THREAD_PRIO

#define K_HIGHEST_APPLICATION_THREAD_PRIO (K_HIGHEST_THREAD_PRIO)
#define K_LOWEST_APPLICATION_THREAD_PRIO  (K_LOWEST_THREAD_PRIO)


typedef struct _wait_q{
	OS_Semaphore_t waitq;
} _wait_q_t;

#define Z_WAIT_Q_INIT(wait_q) { {XR_OS_INVALID_HANDLE} }

#ifdef CONFIG_OBJECT_TRACING
#define _OBJECT_TRACING_NEXT_PTR(type) struct type *__next;
#define _OBJECT_TRACING_LINKED_FLAG uint8_t __linked;
#define _OBJECT_TRACING_INIT \
	.__next = NULL,	     \
	.__linked = 0,
#else
#define _OBJECT_TRACING_INIT
#define _OBJECT_TRACING_NEXT_PTR(type)
#define _OBJECT_TRACING_LINKED_FLAG
#endif

#ifdef CONFIG_POLL
#define _POLL_EVENT_OBJ_INIT(obj) \
	.poll_events = SYS_DLIST_STATIC_INIT(&obj.poll_events),
#define _POLL_EVENT sys_dlist_t poll_events
#else
#define _POLL_EVENT_OBJ_INIT(obj)
#define _POLL_EVENT
#endif

struct k_thread;
//struct k_mutex;
struct k_sem;
//struct k_msgq;
//struct k_mbox;
//struct k_pipe;
struct k_queue;
struct k_fifo;
struct k_lifo;
//struct k_stack;
//struct k_mem_slab;
//struct k_mem_pool;
//struct k_timer;
struct k_poll_event;
struct k_poll_signal;
//struct k_mem_domain;
//struct k_mem_partition;

/*************************************************************************************************************/
/* k_thread */
/*************************************************************************************************************/
typedef void (*k_thread_entry_t)(void *p1, void *p2, void *p3);

typedef void k_thread_stack_t;

/**
 * @ingroup thread_apis
 * Thread Structure
 */
struct k_thread {
	OS_Thread_t task;
    k_thread_entry_t entry;
	void *p1;
	void *p2;
	void *p3;
};

typedef struct k_thread *k_tid_t;

/**
 * @addtogroup thread_apis
 * @{
 */

typedef void (*k_thread_user_cb_t)(const struct k_thread *thread,
				   void *user_data);

/**
 * @brief Iterate over all the threads in the system.
 *
 * This routine iterates over all the threads in the system and
 * calls the user_cb function for each thread.
 *
 * @param user_cb Pointer to the user callback function.
 * @param user_data Pointer to user data.
 *
 * @note @option{CONFIG_THREAD_MONITOR} must be set for this function
 * to be effective.
 * @note This API uses @ref k_spin_lock to protect the _kernel.threads
 * list which means creation of new threads and terminations of existing
 * threads are blocked until this API returns.
 *
 * @return N/A
 */
static inline void k_thread_foreach(k_thread_user_cb_t user_cb, void *user_data)
{
}

/** @} */

/**
 * @defgroup thread_apis Thread APIs
 * @ingroup kernel_apis
 * @{
 */

/**
 * @brief Create a thread.
 *
 * This routine initializes a thread, then schedules it for execution.
 *
 * The new thread may be scheduled for immediate execution or a delayed start.
 * If the newly spawned thread does not have a delayed start the kernel
 * scheduler may preempt the current thread to allow the new thread to
 * execute.
 *
 * Thread options are architecture-specific, and can include K_ESSENTIAL,
 * K_FP_REGS, and K_SSE_REGS. Multiple options may be specified by separating
 * them using "|" (the logical OR operator).
 *
 * Stack objects passed to this function must be originally defined with
 * either of these macros in order to be portable:
 *
 * - K_THREAD_STACK_DEFINE() - For stacks that may support either user or
 *   supervisor threads.
 * - K_KERNEL_STACK_DEFINE() - For stacks that may support supervisor
 *   threads only. These stacks use less memory if CONFIG_USERSPACE is
 *   enabled.
 *
 * The stack_size parameter has constraints. It must either be:
 *
 * - The original size value passed to K_THREAD_STACK_DEFINE() or
 *   K_KERNEL_STACK_DEFINE()
 * - The return value of K_THREAD_STACK_SIZEOF(stack) if the stack was
 *   defined with K_THREAD_STACK_DEFINE()
 * - The return value of K_KERNEL_STACK_SIZEOF(stack) if the stack was
 *   defined with K_KERNEL_STACK_DEFINE().
 *
 * Using other values, or sizeof(stack) may produce undefined behavior.
 *
 * @param new_thread Pointer to uninitialized struct k_thread
 * @param stack Pointer to the stack space.
 * @param stack_size Stack size in bytes.
 * @param entry Thread entry function.
 * @param p1 1st entry point parameter.
 * @param p2 2nd entry point parameter.
 * @param p3 3rd entry point parameter.
 * @param prio Thread priority.
 * @param options Thread options.
 * @param delay Scheduling delay, or K_NO_WAIT (for no delay).
 *
 * @return ID of new thread.
 *
 */
k_tid_t k_thread_create(struct k_thread *new_thread,
				  k_thread_stack_t *stack,
				  size_t stack_size,
				  k_thread_entry_t entry,
				  void *p1, void *p2, void *p3,
				  int prio, uint32_t options, k_timeout_t delay);

/**
 * @brief Sleep until a thread exits
 *
 * The caller will be put to sleep until the target thread exits, either due
 * to being aborted, self-exiting, or taking a fatal error. This API returns
 * immediately if the thread isn't running.
 *
 * This API may only be called from ISRs with a K_NO_WAIT timeout.
 *
 * @param thread Thread to wait to exit
 * @param timeout upper bound time to wait for the thread to exit.
 * @retval 0 success, target thread has exited or wasn't running
 * @retval -EBUSY returned without waiting
 * @retval -EAGAIN waiting period timed out
 * @retval -EDEADLK target thread is joining on the caller, or target thread
 *                  is the caller
 */
int k_thread_join(struct k_thread *thread, k_timeout_t timeout);

/**
 * @brief Put the current thread to sleep.
 *
 * This routine puts the current thread to sleep for @a duration,
 * specified as a k_timeout_t object.
 *
 * @note if @a timeout is set to K_FOREVER then the thread is suspended.
 *
 * @param timeout Desired duration of sleep.
 *
 * @return Zero if the requested time has elapsed or the number of milliseconds
 * left to sleep, if thread was woken up by \ref k_wakeup call.
 */
int32_t k_sleep(k_timeout_t timeout);

/**
 * @brief Put the current thread to sleep.
 *
 * This routine puts the current thread to sleep for @a duration milliseconds.
 *
 * @param ms Number of milliseconds to sleep.
 *
 * @return Zero if the requested time has elapsed or the number of milliseconds
 * left to sleep, if thread was woken up by \ref k_wakeup call.
 */
static inline int32_t k_msleep(int32_t ms)
{
	return k_sleep(Z_TIMEOUT_MS(ms));
}

/**
 * @brief Yield the current thread.
 *
 * This routine causes the current thread to yield execution to another
 * thread of the same or higher priority. If there are no other ready threads
 * of the same or higher priority, the routine returns immediately.
 *
 * @return N/A
 */
static inline void k_yield()
{
	OS_ThreadYield();
}

/**
 * @brief Get thread ID of the current thread.
 *
 * @return ID of current thread.
 *
 */
#define k_current_get() OS_ThreadGetCurrentHandle()

/**
 * @brief Start an inactive thread
 *
 * If a thread was created with K_FOREVER in the delay parameter, it will
 * not be added to the scheduling queue until this function is called
 * on it.
 *
 * @param thread thread to start
 */
static inline void k_thread_start(k_tid_t thread)
{
}

/**
 * Test if the current context is in interrupt context
 *
 * XXX: This is inconsistently handled among arches wrt exception context
 * See: #17656
 *
 * @return true if we are in interrupt context
 */
int arch_is_in_isr(void);

/**
 * @brief Determine if code is running at interrupt level.
 *
 * This routine allows the caller to customize its actions, depending on
 * whether it is a thread or an ISR.
 *
 * @note Can be called by ISRs.
 *
 * @return false if invoked by a thread.
 * @return true if invoked by an ISR.
 */
static inline bool k_is_in_isr(void)
{
	return arch_is_in_isr() > 0;
}

/**
 * @brief Set current thread name
 *
 * Set the name of the thread to be used when @option{CONFIG_THREAD_MONITOR}
 * is enabled for tracing and debugging.
 *
 * @param thread_id Thread to set name, or NULL to set the current thread
 * @param value Name string
 * @retval 0 on success
 * @retval -EFAULT Memory access error with supplied string
 * @retval -ENOSYS Thread name configuration option not enabled
 * @retval -EINVAL Thread name too long
 */
#define k_thread_name_set(thread_id, value)


/*************************************************************************************************************/
/* k_time */
/*************************************************************************************************************/

/**
 * @addtogroup clock_apis
 * @{
 */

/**
 * @brief Generate null timeout delay.
 *
 * This macro generates a timeout delay that instructs a kernel API
 * not to wait if the requested operation cannot be performed immediately.
 *
 * @return Timeout delay value.
 */
#define K_NO_WAIT Z_TIMEOUT_NO_WAIT

/**
 * @brief Generate timeout delay from nanoseconds.
 *
 * This macro generates a timeout delay that instructs a kernel API to
 * wait up to @a t nanoseconds to perform the requested operation.
 * Note that timer precision is limited to the tick rate, not the
 * requested value.
 *
 * @param t Duration in nanoseconds.
 *
 * @return Timeout delay value.
 */
#define K_NSEC(t)     Z_TIMEOUT_NS(t)

/**
 * @brief Generate timeout delay from microseconds.
 *
 * This macro generates a timeout delay that instructs a kernel API
 * to wait up to @a t microseconds to perform the requested operation.
 * Note that timer precision is limited to the tick rate, not the
 * requested value.
 *
 * @param t Duration in microseconds.
 *
 * @return Timeout delay value.
 */
#define K_USEC(t)     Z_TIMEOUT_US(t)

/**
 * @brief Generate timeout delay from cycles.
 *
 * This macro generates a timeout delay that instructs a kernel API
 * to wait up to @a t cycles to perform the requested operation.
 *
 * @param t Duration in cycles.
 *
 * @return Timeout delay value.
 */
#define K_CYC(t)     Z_TIMEOUT_CYC(t)

/**
 * @brief Generate timeout delay from system ticks.
 *
 * This macro generates a timeout delay that instructs a kernel API
 * to wait up to @a t ticks to perform the requested operation.
 *
 * @param t Duration in system ticks.
 *
 * @return Timeout delay value.
 */
#define K_TICKS(t)     Z_TIMEOUT_TICKS(t)

/**
 * @brief Generate timeout delay from milliseconds.
 *
 * This macro generates a timeout delay that instructs a kernel API
 * to wait up to @a ms milliseconds to perform the requested operation.
 *
 * @param ms Duration in milliseconds.
 *
 * @return Timeout delay value.
 */
#define K_MSEC(ms)     Z_TIMEOUT_MS(ms)

/**
 * @brief Generate timeout delay from seconds.
 *
 * This macro generates a timeout delay that instructs a kernel API
 * to wait up to @a s seconds to perform the requested operation.
 *
 * @param s Duration in seconds.
 *
 * @return Timeout delay value.
 */
#define K_SECONDS(s)   K_MSEC((s) * MSEC_PER_SEC)

/**
 * @brief Generate timeout delay from minutes.

 * This macro generates a timeout delay that instructs a kernel API
 * to wait up to @a m minutes to perform the requested operation.
 *
 * @param m Duration in minutes.
 *
 * @return Timeout delay value.
 */
#define K_MINUTES(m)   K_SECONDS((m) * 60)

/**
 * @brief Generate timeout delay from hours.
 *
 * This macro generates a timeout delay that instructs a kernel API
 * to wait up to @a h hours to perform the requested operation.
 *
 * @param h Duration in hours.
 *
 * @return Timeout delay value.
 */
#define K_HOURS(h)     K_MINUTES((h) * 60)

/**
 * @brief Generate infinite timeout delay.
 *
 * This macro generates a timeout delay that instructs a kernel API
 * to wait as long as necessary to perform the requested operation.
 *
 * @return Timeout delay value.
 */
#define K_FOREVER Z_FOREVER

/**
 * @}
 */

/*************************************************************************************************************/
/* k_timer */
/*************************************************************************************************************/

//tbc...

/*************************************************************************************************************/
/* sys_clock.h */
/*************************************************************************************************************/

/* timeouts */

struct _timeout;
typedef void (*_timeout_func_t)(struct _timeout *t);

struct _timeout {
	OS_Timer_t handle;
#ifdef CONFIG_TIMEOUT_64BIT
	int64_t expiry;	/* tick in milliseconds */
#else
	uint32_t expiry;	/* tick in milliseconds */
#endif
};

/*************************************************************************************************************/
/* timeout_q.h */
/*************************************************************************************************************/

static inline void z_init_timeout(struct _timeout *t)
{
    memset(t, 0, sizeof(*t));
}

void z_add_timeout(struct _timeout *to, _timeout_func_t fn, k_timeout_t ticks);

int z_abort_timeout(struct _timeout *timeout);

int32_t z_timeout_remaining(const struct _timeout *timeout);

bool z_is_inactive_timeout(struct _timeout *t);

/*************************************************************************************************************/
/* time_units.h */
/*************************************************************************************************************/

/** @brief Convert milliseconds to ticks
 *
 * Converts time values in milliseconds to ticks.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @return The converted time value
 */
static inline int32_t k_ms_to_ticks_ceil32(int32_t ms)
{
	return (int32_t)OS_MSecsToTicks(ms);
}

/** @brief Convert ticks to milliseconds
 *
 * Converts time values in ticks to milliseconds.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static inline uint32_t k_ticks_to_ms_floor32(uint32_t t)
{
	return OS_TicksToMSecs(t);
}

/** @brief Convert hardware cycles to nanoseconds
 *
 * Converts time values in hardware cycles to nanoseconds.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static inline uint64_t k_cyc_to_ns_floor64(uint64_t t)
{
	return (uint64_t)OS_TicksToMSecs(t) * (NSEC_PER_USEC) * (USEC_PER_MSEC);
}

/*************************************************************************************************************/
/* k_uptime */
/*************************************************************************************************************/

/**
 * @addtogroup clock_apis
 * @{
 */

/*
 * Warning:! 32bit to 64bit might cause overflow error!
 * TODO: Time comparing should use OS_TimeAfter in zephyr host!
 */

/**
 * @brief Get system uptime, in system ticks.
 *
 * This routine returns the elapsed time since the system booted, in
 * ticks (c.f. @option{CONFIG_SYS_CLOCK_TICKS_PER_SEC}), which is the
 * fundamental unit of resolution of kernel timekeeping.
 *
 * @return Current uptime in ticks.
 */
static inline  int64_t k_uptime_ticks(void)
{
	return z_tick_get();
}

/**
 * @brief Get system uptime.
 *
 * This routine returns the elapsed time since the system booted,
 * in milliseconds.
 *
 * @note
 *    While this function returns time in milliseconds, it does
 *    not mean it has millisecond resolution. The actual resolution depends on
 *    @option{CONFIG_SYS_CLOCK_TICKS_PER_SEC} config option.
 *
 * @return Current uptime in milliseconds.
 */
int64_t k_uptime_get(void);

/*
 * TODO: Time comparing should use OS_TimeAfter in zephyr host!
 */
/**
 * @brief Get system uptime (32-bit version).
 *
 * This routine returns the lower 32 bits of the system uptime in
 * milliseconds.
 *
 * Because correct conversion requires full precision of the system
 * clock there is no benefit to using this over k_uptime_get() unless
 * you know the application will never run long enough for the system
 * clock to approach 2^32 ticks.  Calls to this function may involve
 * interrupt blocking and 64-bit math.
 *
 * @note
 *    While this function returns time in milliseconds, it does
 *    not mean it has millisecond resolution. The actual resolution depends on
 *    @option{CONFIG_SYS_CLOCK_TICKS_PER_SEC} config option
 *
 * @return The low 32 bits of the current uptime, in milliseconds.
 */
uint32_t k_uptime_get_32(void);

/**
 * @brief Get elapsed time.
 *
 * This routine computes the elapsed time between the current system uptime
 * and an earlier reference time, in milliseconds.
 *
 * @param reftime Pointer to a reference time, which is updated to the current
 *                uptime upon return.
 *
 * @return Elapsed time.
 */
static inline int64_t k_uptime_delta(int64_t *reftime)
{
	int64_t uptime, delta;

	uptime = k_uptime_get();
	delta = uptime - *reftime;
	*reftime = uptime;

	return delta;
}

/**
 * @}
 */

/*************************************************************************************************************/
/* waitq */
/*************************************************************************************************************/

static inline int z_waitq_init(_wait_q_t *w)
{
	return OS_SemaphoreCreate(&w->waitq, 0, OS_SEMAPHORE_MAX_COUNT);
}

static inline int z_waitq_init_n(_wait_q_t *w, uint32_t n)
{
	return OS_SemaphoreCreate(&w->waitq, n, OS_SEMAPHORE_MAX_COUNT);
}

#if defined(CONFIG_BT_DEINIT)
static inline int z_waitq_deinit_n(_wait_q_t *w)
{
	return OS_SemaphoreDelete(&w->waitq);
}
#endif

void irq_unlock(unsigned int key);

/* !Warning! The definitions of functions below here are not same as zephyr. */

static inline int z_pend_curr(void *lock, uint32_t key,
	       _wait_q_t *wait_q, int32_t timeout)
{
	irq_unlock(key);
	return OS_SemaphoreWait(&wait_q->waitq, (OS_Time_t)timeout);
}

static inline int z_pend_curr_irqlock(uint32_t key, _wait_q_t *wait_q, int32_t timeout)
{
	irq_unlock(key);
	return OS_SemaphoreWait(&wait_q->waitq, (OS_Time_t)timeout);
}

static inline struct k_thread *z_unpend_first_thread(_wait_q_t *wait_q)
{
	OS_SemaphoreRelease(&wait_q->waitq);
	return (void *)!0;
}

static inline void z_ready_thread(struct k_thread *thread)
{

}

static inline void z_reschedule(unsigned int key)
{
	irq_unlock(key);
	k_yield();	//reschedule?
}

/*************************************************************************************************************/
/* k_queue */
/*************************************************************************************************************/

#define QUEUE_SIZE     7
#define QUEUE_BLK_SIZE sizeof(void *) + 1

/**
 * @cond INTERNAL_HIDDEN
 */

struct k_queue {
	sys_sflist_t data_q;
	union {
		_wait_q_t wait_q;

		_POLL_EVENT;
	};

	_OBJECT_TRACING_NEXT_PTR(k_queue)
	_OBJECT_TRACING_LINKED_FLAG
};

#define Z_QUEUE_INITIALIZER(obj) \
	{ \
	.data_q = SYS_SFLIST_STATIC_INIT(&obj.data_q), \
	{ \
		.wait_q = Z_WAIT_Q_INIT(&obj.wait_q), \
		_POLL_EVENT_OBJ_INIT(obj) \
	}, \
	_OBJECT_TRACING_INIT \
	}

#if !defined(CONFIG_POLL)
#error "Notice! k_fifo/k_lifo/k_queue should init by k_fifo_init/k_lifo_init/k_queue_init."
#endif

/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @defgroup queue_apis Queue APIs
 * @ingroup kernel_apis
 * @{
 */

/**
 * @brief Initialize a queue.
 *
 * This routine initializes a queue object, prior to its first use.
 *
 * @param queue Address of the queue.
 *
 * @return N/A
 */
void k_queue_init(struct k_queue *queue);

#if defined(CONFIG_BT_DEINIT)
/**
 * @brief Deinitialize a queue.
 *
 * This routine deinitializes a queue object, prior to its first use.
 *
 * @param queue Address of the queue.
 *
 * @return N/A
 */
void k_queue_deinit(struct k_queue *queue);
#endif

/**
 * @brief Cancel waiting on a queue.
 *
 * This routine causes first thread pending on @a queue, if any, to
 * return from k_queue_get() call with NULL value (as if timeout expired).
 * If the queue is being waited on by k_poll(), it will return with
 * -EINTR and K_POLL_STATE_CANCELLED state (and per above, subsequent
 * k_queue_get() will return NULL).
 *
 * @note Can be called by ISRs.
 *
 * @param queue Address of the queue.
 *
 * @return N/A
 */
void k_queue_cancel_wait(struct k_queue *queue);

/**
 * @brief Append an element to the end of a queue.
 *
 * This routine appends a data item to @a queue. A queue data item must be
 * aligned on a word boundary, and the first word of the item is reserved
 * for the kernel's use.
 *
 * @note Can be called by ISRs.
 *
 * @param queue Address of the queue.
 * @param data Address of the data item.
 *
 * @return N/A
 */
extern void k_queue_append(struct k_queue *queue, void *data);

/**
 * @brief Append an element to a queue.
 *
 * This routine appends a data item to @a queue. There is an implicit memory
 * allocation to create an additional temporary bookkeeping data structure from
 * the calling thread's resource pool, which is automatically freed when the
 * item is removed. The data itself is not copied.
 *
 * @note Can be called by ISRs.
 *
 * @param queue Address of the queue.
 * @param data Address of the data item.
 *
 * @retval 0 on success
 * @retval -ENOMEM if there isn't sufficient RAM in the caller's resource pool
 */
int32_t k_queue_alloc_append(struct k_queue *queue, void *data);

/**
 * @brief Prepend an element to a queue.
 *
 * This routine prepends a data item to @a queue. A queue data item must be
 * aligned on a word boundary, and the first word of the item is reserved
 * for the kernel's use.
 *
 * @note Can be called by ISRs.
 *
 * @param queue Address of the queue.
 * @param data Address of the data item.
 *
 * @return N/A
 */
extern void k_queue_prepend(struct k_queue *queue, void *data);

/**
 * @brief Prepend an element to a queue.
 *
 * This routine prepends a data item to @a queue. There is an implicit memory
 * allocation to create an additional temporary bookkeeping data structure from
 * the calling thread's resource pool, which is automatically freed when the
 * item is removed. The data itself is not copied.
 *
 * @note Can be called by ISRs.
 *
 * @param queue Address of the queue.
 * @param data Address of the data item.
 *
 * @retval 0 on success
 * @retval -ENOMEM if there isn't sufficient RAM in the caller's resource pool
 */
int32_t k_queue_alloc_prepend(struct k_queue *queue, void *data);

/**
 * @brief Inserts an element to a queue.
 *
 * This routine inserts a data item to @a queue after previous item. A queue
 * data item must be aligned on a word boundary, and the first word of
 * the item is reserved for the kernel's use.
 *
 * @note Can be called by ISRs.
 *
 * @param queue Address of the queue.
 * @param prev Address of the previous data item.
 * @param data Address of the data item.
 *
 * @return N/A
 */
extern void k_queue_insert(struct k_queue *queue, void *prev, void *data);

/**
 * @brief Atomically append a list of elements to a queue.
 *
 * This routine adds a list of data items to @a queue in one operation.
 * The data items must be in a singly-linked list, with the first word
 * in each data item pointing to the next data item; the list must be
 * NULL-terminated.
 *
 * @note Can be called by ISRs.
 *
 * @param queue Address of the queue.
 * @param head Pointer to first node in singly-linked list.
 * @param tail Pointer to last node in singly-linked list.
 *
 * @retval 0 on success
 * @retval -EINVAL on invalid supplied data
 *
 */
extern int k_queue_append_list(struct k_queue *queue, void *head, void *tail);

/**
 * @brief Get an element from a queue.
 *
 * This routine removes first data item from @a queue. The first word of the
 * data item is reserved for the kernel's use.
 *
 * @note Can be called by ISRs, but @a timeout must be set to K_NO_WAIT.
 *
 * @param queue Address of the queue.
 * @param timeout Non-negative waiting period to obtain a data item
 *                or one of the special values K_NO_WAIT and
 *                K_FOREVER.
 *
 * @return Address of the data item if successful; NULL if returned
 * without waiting, or waiting period timed out.
 */
void *k_queue_get(struct k_queue *queue, k_timeout_t timeout);

/**
 * @brief Remove an element from a queue.
 *
 * This routine removes data item from @a queue. The first word of the
 * data item is reserved for the kernel's use. Removing elements from k_queue
 * rely on sys_slist_find_and_remove which is not a constant time operation.
 *
 * @note Can be called by ISRs
 *
 * @param queue Address of the queue.
 * @param data Address of the data item.
 *
 * @return true if data item was removed
 */
static inline bool k_queue_remove(struct k_queue *queue, void *data)
{
	return sys_sflist_find_and_remove(&queue->data_q, (sys_sfnode_t *)data);
}

/**
 * @brief Append an element to a queue only if it's not present already.
 *
 * This routine appends data item to @a queue. The first word of the data
 * item is reserved for the kernel's use. Appending elements to k_queue
 * relies on sys_slist_is_node_in_list which is not a constant time operation.
 *
 * @note Can be called by ISRs
 *
 * @param queue Address of the queue.
 * @param data Address of the data item.
 *
 * @return true if data item was added, false if not
 */
static inline bool k_queue_unique_append(struct k_queue *queue, void *data)
{
	sys_sfnode_t *test;

	SYS_SFLIST_FOR_EACH_NODE(&queue->data_q, test) {
		if (test == (sys_sfnode_t *) data) {
			return false;
		}
	}

	k_queue_append(queue, data);
	return true;
}

/**
 * @brief Query a queue to see if it has data available.
 *
 * Note that the data might be already gone by the time this function returns
 * if other threads are also trying to read from the queue.
 *
 * @note Can be called by ISRs.
 *
 * @param queue Address of the queue.
 *
 * @return Non-zero if the queue is empty.
 * @return 0 if data is available.
 */

static inline int k_queue_is_empty(struct k_queue *queue)
{
	return (int)sys_sflist_is_empty(&queue->data_q);
}

/**
 * @brief Peek element at the head of queue.
 *
 * Return element from the head of queue without removing it.
 *
 * @param queue Address of the queue.
 *
 * @return Head element, or NULL if queue is empty.
 */
void *k_queue_peek_head(struct k_queue *queue);

/**
 * @brief Peek element at the tail of queue.
 *
 * Return element from the tail of queue without removing it.
 *
 * @param queue Address of the queue.
 *
 * @return Tail element, or NULL if queue is empty.
 */
void *k_queue_peek_tail(struct k_queue *queue);

/**
 * @brief Statically define and initialize a queue.
 *
 * The queue can be accessed outside the module where it is defined using:
 *
 * @code extern struct k_queue <name>; @endcode
 *
 * @param name Name of the queue.
 */
#define K_QUEUE_DEFINE(name) \
	struct k_queue name \
		__in_section(_k_queue, static, name) = \
		Z_QUEUE_INITIALIZER(name)

/** @} */

/*************************************************************************************************************/
/* k_fifo */
/*************************************************************************************************************/

struct k_fifo {
	struct k_queue _queue;
};

/**
 * @cond INTERNAL_HIDDEN
 */
#define Z_FIFO_INITIALIZER(obj) \
	{ \
	._queue = Z_QUEUE_INITIALIZER(obj._queue) \
	}

/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @defgroup fifo_apis FIFO APIs
 * @ingroup kernel_apis
 * @{
 */

/**
 * @brief Initialize a FIFO queue.
 *
 * This routine initializes a FIFO queue, prior to its first use.
 *
 * @param fifo Address of the FIFO queue.
 *
 * @return N/A
 */
#define k_fifo_init(fifo) \
	k_queue_init(&(fifo)->_queue)

/**
 * @brief Cancel waiting on a FIFO queue.
 *
 * This routine causes first thread pending on @a fifo, if any, to
 * return from k_fifo_get() call with NULL value (as if timeout
 * expired).
 *
 * @note Can be called by ISRs.
 *
 * @param fifo Address of the FIFO queue.
 *
 * @return N/A
 */
#define k_fifo_cancel_wait(fifo) \
	k_queue_cancel_wait(&(fifo)->_queue)

/**
 * @brief Add an element to a FIFO queue.
 *
 * This routine adds a data item to @a fifo. A FIFO data item must be
 * aligned on a word boundary, and the first word of the item is reserved
 * for the kernel's use.
 *
 * @note Can be called by ISRs.
 *
 * @param fifo Address of the FIFO.
 * @param data Address of the data item.
 *
 * @return N/A
 */
#define k_fifo_put(fifo, data) \
	k_queue_append(&(fifo)->_queue, data)

/**
 * @brief Add an element to a FIFO queue.
 *
 * This routine adds a data item to @a fifo. There is an implicit memory
 * allocation to create an additional temporary bookkeeping data structure from
 * the calling thread's resource pool, which is automatically freed when the
 * item is removed. The data itself is not copied.
 *
 * @note Can be called by ISRs.
 *
 * @param fifo Address of the FIFO.
 * @param data Address of the data item.
 *
 * @retval 0 on success
 * @retval -ENOMEM if there isn't sufficient RAM in the caller's resource pool
 */
#define k_fifo_alloc_put(fifo, data) \
	k_queue_alloc_append(&(fifo)->_queue, data)

/**
 * @brief Atomically add a list of elements to a FIFO.
 *
 * This routine adds a list of data items to @a fifo in one operation.
 * The data items must be in a singly-linked list, with the first word of
 * each data item pointing to the next data item; the list must be
 * NULL-terminated.
 *
 * @note Can be called by ISRs.
 *
 * @param fifo Address of the FIFO queue.
 * @param head Pointer to first node in singly-linked list.
 * @param tail Pointer to last node in singly-linked list.
 *
 * @return N/A
 */
#define k_fifo_put_list(fifo, head, tail) \
	k_queue_append_list(&(fifo)->_queue, head, tail)

/**
 * @brief Atomically add a list of elements to a FIFO queue.
 *
 * This routine adds a list of data items to @a fifo in one operation.
 * The data items must be in a singly-linked list implemented using a
 * sys_slist_t object. Upon completion, the sys_slist_t object is invalid
 * and must be re-initialized via sys_slist_init().
 *
 * @note Can be called by ISRs.
 *
 * @param fifo Address of the FIFO queue.
 * @param list Pointer to sys_slist_t object.
 *
 * @return N/A
 */
#define k_fifo_put_slist(fifo, list) \
	k_queue_merge_slist(&(fifo)->_queue, list)

/**
 * @brief Get an element from a FIFO queue.
 *
 * This routine removes a data item from @a fifo in a "first in, first out"
 * manner. The first word of the data item is reserved for the kernel's use.
 *
 * @note Can be called by ISRs, but @a timeout must be set to K_NO_WAIT.
 *
 * @param fifo Address of the FIFO queue.
 * @param timeout Waiting period to obtain a data item,
 *                or one of the special values K_NO_WAIT and K_FOREVER.
 *
 * @return Address of the data item if successful; NULL if returned
 * without waiting, or waiting period timed out.
 */
#define k_fifo_get(fifo, timeout) \
	k_queue_get(&(fifo)->_queue, timeout)

/**
 * @brief Query a FIFO queue to see if it has data available.
 *
 * Note that the data might be already gone by the time this function returns
 * if other threads is also trying to read from the FIFO.
 *
 * @note Can be called by ISRs.
 *
 * @param fifo Address of the FIFO queue.
 *
 * @return Non-zero if the FIFO queue is empty.
 * @return 0 if data is available.
 */
#define k_fifo_is_empty(fifo) \
	k_queue_is_empty(&(fifo)->_queue)

/**
 * @brief Peek element at the head of a FIFO queue.
 *
 * Return element from the head of FIFO queue without removing it. A usecase
 * for this is if elements of the FIFO object are themselves containers. Then
 * on each iteration of processing, a head container will be peeked,
 * and some data processed out of it, and only if the container is empty,
 * it will be completely remove from the FIFO queue.
 *
 * @param fifo Address of the FIFO queue.
 *
 * @return Head element, or NULL if the FIFO queue is empty.
 */
#define k_fifo_peek_head(fifo) \
	k_queue_peek_head(&(fifo)->_queue)

/**
 * @brief Peek element at the tail of FIFO queue.
 *
 * Return element from the tail of FIFO queue (without removing it). A usecase
 * for this is if elements of the FIFO queue are themselves containers. Then
 * it may be useful to add more data to the last container in a FIFO queue.
 *
 * @param fifo Address of the FIFO queue.
 *
 * @return Tail element, or NULL if a FIFO queue is empty.
 */
#define k_fifo_peek_tail(fifo) \
	k_queue_peek_tail(&(fifo)->_queue)

/**
 * @brief Statically define and initialize a FIFO queue.
 *
 * The FIFO queue can be accessed outside the module where it is defined using:
 *
 * @code extern struct k_fifo <name>; @endcode
 *
 * @param name Name of the FIFO queue.
 */
#define K_FIFO_DEFINE(name) \
	struct k_fifo name \
		/*__in_section(_k_queue, static, name)*/ = \
		Z_FIFO_INITIALIZER(name)

/** @} */

/*************************************************************************************************************/
/* k_lifo */
/*************************************************************************************************************/

struct k_lifo {
	struct k_queue _queue;
};

/**
 * @cond INTERNAL_HIDDEN
 */

#define Z_LIFO_INITIALIZER(obj) \
	{ \
	._queue = Z_QUEUE_INITIALIZER(obj._queue) \
	}

#if defined(CONFIG_BT_DEINIT)
#define Z_LIFO_DEINITIALIZER(obj) \
	{ \
	._queue = Z_QUEUE_INITIALIZER(obj._queue) \
	}
#endif

/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @defgroup lifo_apis LIFO APIs
 * @ingroup kernel_apis
 * @{
 */

/**
 * @brief Initialize a LIFO queue.
 *
 * This routine initializes a LIFO queue object, prior to its first use.
 *
 * @param lifo Address of the LIFO queue.
 *
 * @return N/A
 */
#define k_lifo_init(lifo) \
	k_queue_init(&(lifo)->_queue)

/**
 * @brief Add an element to a LIFO queue.
 *
 * This routine adds a data item to @a lifo. A LIFO queue data item must be
 * aligned on a word boundary, and the first word of the item is
 * reserved for the kernel's use.
 *
 * @note Can be called by ISRs.
 *
 * @param lifo Address of the LIFO queue.
 * @param data Address of the data item.
 *
 * @return N/A
 */
#define k_lifo_put(lifo, data) \
	k_queue_prepend(&(lifo)->_queue, data)

/**
 * @brief Add an element to a LIFO queue.
 *
 * This routine adds a data item to @a lifo. There is an implicit memory
 * allocation to create an additional temporary bookkeeping data structure from
 * the calling thread's resource pool, which is automatically freed when the
 * item is removed. The data itself is not copied.
 *
 * @note Can be called by ISRs.
 *
 * @param lifo Address of the LIFO.
 * @param data Address of the data item.
 *
 * @retval 0 on success
 * @retval -ENOMEM if there isn't sufficient RAM in the caller's resource pool
 */
#define k_lifo_alloc_put(lifo, data) \
	k_queue_alloc_prepend(&(lifo)->_queue, data)

/**
 * @brief Get an element from a LIFO queue.
 *
 * This routine removes a data item from @a LIFO in a "last in, first out"
 * manner. The first word of the data item is reserved for the kernel's use.
 *
 * @note Can be called by ISRs, but @a timeout must be set to K_NO_WAIT.
 *
 * @param lifo Address of the LIFO queue.
 * @param timeout Waiting period to obtain a data item,
 *                or one of the special values K_NO_WAIT and K_FOREVER.
 *
 * @return Address of the data item if successful; NULL if returned
 * without waiting, or waiting period timed out.
 */
#define k_lifo_get(lifo, timeout) \
	k_queue_get(&(lifo)->_queue, timeout)

/**
 * @brief Statically define and initialize a LIFO queue.
 *
 * The LIFO queue can be accessed outside the module where it is defined using:
 *
 * @code extern struct k_lifo <name>; @endcode
 *
 * @param name Name of the fifo.
 */
#define K_LIFO_DEFINE(name) \
	struct k_lifo name \
		__in_section(_k_queue, static, name) = \
		Z_LIFO_INITIALIZER(name)

/*************************************************************************************************************/
/* k_stack */
/*************************************************************************************************************/

//tbc...

/*************************************************************************************************************/
/* k_work */
/*************************************************************************************************************/


typedef struct k_work k_work_t;


/**
 * @addtogroup thread_apis
 * @{
 */

/**
 * @typedef k_work_handler_t
 * @brief Work item handler function type.
 *
 * A work item's handler function is executed by a workqueue's thread
 * when the work item is processed by the workqueue.
 *
 * @param work Address of the work item.
 *
 * @return N/A
 */
typedef void (*k_work_handler_t)(struct k_work *work);

/**
 * @cond INTERNAL_HIDDEN
 */

struct k_work_q {
	struct k_queue queue;
	struct k_thread thread;
};

enum {
	K_WORK_STATE_PENDING,	/* Work item pending state */
};

struct k_work {
	void *_reserved;		/* Used by k_queue implementation. */
	k_work_handler_t handler;
	atomic_t flags[1];
};

struct k_delayed_work {
	struct k_work work;
	struct _timeout timeout;
	struct k_work_q *work_q;
#if defined(CONFIG_BT_DEINIT)
	sys_snode_t node;
#endif
};

extern struct k_work_q k_sys_work_q;

/**
 * INTERNAL_HIDDEN @endcond
 */

#define Z_WORK_INITIALIZER(work_handler) \
	{ \
	._reserved = NULL, \
	.handler = work_handler, \
	.flags = { 0 } \
	}

#if defined(CONFIG_BT_DEINIT)
#define Z_WORK_DEINITIALIZER(work_handler) \
	{ \
	._reserved = NULL, \
	.handler = NULL, \
	.flags = { 0 } \
	}
#endif
/**
 * @brief Initialize a statically-defined work item.
 *
 * This macro can be used to initialize a statically-defined workqueue work
 * item, prior to its first use. For example,
 *
 * @code static K_WORK_DEFINE(<work>, <work_handler>); @endcode
 *
 * @param work Symbol name for work item object
 * @param work_handler Function to invoke each time work item is processed.
 */
#define K_WORK_DEFINE(work, work_handler) \
	struct k_work work = Z_WORK_INITIALIZER(work_handler)

/**
 * @brief Initialize a work item.
 *
 * This routine initializes a workqueue work item, prior to its first use.
 *
 * @param work Address of work item.
 * @param handler Function to invoke each time work item is processed.
 *
 * @return N/A
 */
static inline void k_work_init(struct k_work *work, k_work_handler_t handler)
{
	*work = (struct k_work)Z_WORK_INITIALIZER(handler);
}

#if defined(CONFIG_BT_DEINIT)
/**
 * @brief Deinitialize a work item.
 *
 * This routine deinitializes a workqueue work item, prior to its first use.
 *
 * @param work Address of work item.
 *
 * @return N/A
 */
static inline void k_work_deinit(struct k_work *work)
{
	*work = (struct k_work)Z_WORK_DEINITIALIZER(NULL);
}
#endif

/**
 * @brief Submit a work item.
 *
 * This routine submits work item @p work to be processed by workqueue @p
 * work_q. If the work item is already pending in @p work_q or any other
 * workqueue as a result of an earlier submission, this routine has no
 * effect on the work item. If the work item has already been processed, or
 * is currently being processed, its work is considered complete and the
 * work item can be resubmitted.
 *
 * @warning
 * A submitted work item must not be modified until it has been processed
 * by the workqueue.
 *
 * @note Can be called by ISRs.
 *
 * @param work_q Address of workqueue.
 * @param work Address of work item.
 *
 * @return N/A
 * @req K-WORK-001
 */
void k_work_submit_to_queue(struct k_work_q *work_q,struct k_work *work);

/**
 * @brief Check if a work item is pending.
 *
 * This routine indicates if work item @a work is pending in a workqueue's
 * queue.
 *
 * @note Checking if the work is pending gives no guarantee that the
 *       work will still be pending when this information is used. It is up to
 *       the caller to make sure that this information is used in a safe manner.
 *
 * @note Can be called by ISRs.
 *
 * @param work Address of work item.
 *
 * @return true if work item is pending, or false if it is not pending.
 */
static inline bool k_work_pending(struct k_work *work)
{
	return atomic_test_bit(work->flags, K_WORK_STATE_PENDING);
}

/**
 * @brief  Check if a delayed work item is pending.
 *
 * This routine indicates if the work item @a work is pending in a workqueue's
 * queue or waiting for the delay timeout.
 *
 * @note Checking if the delayed work is pending gives no guarantee that the
 *       work will still be pending when this information is used. It is up to
 *       the caller to make sure that this information is used in a safe manner.
 *
 * @note Can be called by ISRs.
 *
 * @param work Address of delayed work item.
 *
 * @return true if work item is waiting for the delay to expire or pending on a
 *         work queue, or false if it is not pending.
 */
bool k_delayed_work_pending(struct k_delayed_work *work);

/**
 * @brief Start a workqueue.
 *
 * This routine starts workqueue @a work_q. The workqueue spawns its work
 * processing thread, which runs forever.
 *
 * @param work_q Address of workqueue.
 * @param stack Pointer to work queue thread's stack space, as defined by
 *		K_THREAD_STACK_DEFINE()
 * @param stack_size Size of the work queue thread's stack (in bytes), which
 *		should either be the same constant passed to
 *		K_THREAD_STACK_DEFINE() or the value of K_THREAD_STACK_SIZEOF().
 * @param prio Priority of the work queue's thread.
 *
 * @return N/A
 */
extern void k_work_q_start(struct k_work_q *work_q,
			   k_thread_stack_t *stack,
			   size_t stack_size, int prio);

#if defined(CONFIG_BT_DEINIT)
/**
 * @brief Stop workqueue thread.
 *
 * This routine stop workqueue @a work_q. The workqueue spawns its work
 * processing thread, which runs forever.
 *
 * @param work_q Address of workqueue.
 *
 * @return N/A
 */
void k_work_q_stop(struct k_work_q *work_q);
#endif

/**
 * @brief Initialize a delayed work item.
 *
 * This routine initializes a workqueue delayed work item, prior to
 * its first use.
 *
 * @param work Address of delayed work item.
 * @param handler Function to invoke each time work item is processed.
 *
 * @return N/A
 */
extern void k_delayed_work_init(struct k_delayed_work *work,
				k_work_handler_t handler);

#if defined(CONFIG_BT_DEINIT)
/**
 * @brief Denitialize a delayed work item.
 *
 * This routine deinitializes a workqueue delayed work item, prior to
 * its first use.
 *
 * @param work Address of delayed work item.
 *
 * @return N/A
 */
extern void k_delayed_work_deinit(struct k_delayed_work *work);
#endif

/**
 * @brief Submit a delayed work item.
 *
 * This routine schedules work item @a work to be processed by workqueue
 * @a work_q after a delay of @a delay milliseconds. The routine initiates
 * an asynchronous countdown for the work item and then returns to the caller.
 * Only when the countdown completes is the work item actually submitted to
 * the workqueue and becomes pending.
 *
 * Submitting a previously submitted delayed work item that is still counting
 * down or is pending cancels the existing submission and restarts the
 * countdown using the new delay.  Note that this behavior is inherently
 * subject to race conditions with the pre-existing timeouts and work queue,
 * so care must be taken to synchronize such resubmissions externally.
 *
 * Attempts to submit a work item to a queue after it has been submitted to a
 * different queue will fail with @c -EALREADY until k_delayed_work_cancel()
 * is successfully invoked on the work item to clear its internal state.
 *
 * @warning
 * A delayed work item must not be modified until it has been processed
 * by the workqueue.
 *
 * @note Can be called by ISRs.
 *
 * @param work_q Address of workqueue.
 * @param work Address of delayed work item.
 * @param delay Delay before submitting the work item
 *
 * @retval 0 Work item countdown started.
 * @retval -EINVAL
 *    * if a previously submitted work item had to be cancelled and the
 *      cancellation failed; or
 *    * Work item is being processed or has completed its work.
 * @retval -EADDRINUSE Work item was submitted to a different workqueue.
 */
extern int k_delayed_work_submit_to_queue(struct k_work_q *work_q,
					  struct k_delayed_work *work,
					  k_timeout_t delay);

/**
 * @brief Cancel a delayed work item.
 *
 * This routine cancels the submission of delayed work item @a work.  Whether
 * the work item can be successfully cancelled depends on its state.
 *
 * @note Can be called by ISRs.
 *
 * @note When @c -EALREADY is returned the caller cannot distinguish whether
 * the work item handler is still being invoked by the work queue thread or
 * has completed.
 *
 * @param work Address of delayed work item.
 *
 * @retval 0
 *   * Work item countdown cancelled before the item was submitted to its
 *     queue; or
 *   * Work item was removed from its queue before it was processed.
 * @retval -EINVAL
 *   * Work item has never been submitted; or
 *   * Work item has been successfully cancelled; or
 *   * Timeout handler is in the process of submitting the work item to its
 *     queue; or
 *   * Work queue thread has removed the work item from the queue but has not
 *     called its handler.
 * @retval -EALREADY
 *   * Work queue thread has removed the work item from the queue and cleared
 *     its pending flag; or
 *   * Work queue thread is invoking the item handler; or
 *   * Work item handler has completed.
 */
extern int k_delayed_work_cancel(struct k_delayed_work *work);

/**
 * @brief Submit a work item to the system workqueue.
 *
 * This routine submits work item @a work to be processed by the system
 * workqueue. If the work item is already pending in the system workqueue or
 * any other workqueue as a result of an earlier submission, this routine
 * has no effect on the work item. If the work item has already been
 * processed, or is currently being processed, its work is considered
 * complete and the work item can be resubmitted.
 *
 * @warning
 * Work items submitted to the system workqueue should avoid using handlers
 * that block or yield since this may prevent the system workqueue from
 * processing other work items in a timely manner.
 *
 * @note Can be called by ISRs.
 *
 * @param work Address of work item.
 *
 * @return N/A
 */
static inline void k_work_submit(struct k_work *work)
{
	k_work_submit_to_queue(&k_sys_work_q, work);
}

/**
 * @brief Submit a delayed work item to the system workqueue.
 *
 * This routine schedules work item @a work to be processed by the system
 * workqueue after a delay of @a delay milliseconds. The routine initiates
 * an asynchronous countdown for the work item and then returns to the caller.
 * Only when the countdown completes is the work item actually submitted to
 * the workqueue and becomes pending.
 *
 * Submitting a previously submitted delayed work item that is still
 * counting down cancels the existing submission and restarts the countdown
 * using the new delay. If the work item is currently pending on the
 * workqueue's queue because the countdown has completed it is too late to
 * resubmit the item, and resubmission fails without impacting the work item.
 * If the work item has already been processed, or is currently being processed,
 * its work is considered complete and the work item can be resubmitted.
 *
 * Attempts to submit a work item to a queue after it has been submitted to a
 * different queue will fail with @c -EALREADY until k_delayed_work_cancel()
 * is invoked on the work item to clear its internal state.
 *
 * @warning
 * Work items submitted to the system workqueue should avoid using handlers
 * that block or yield since this may prevent the system workqueue from
 * processing other work items in a timely manner.
 *
 * @note Can be called by ISRs.
 *
 * @param work Address of delayed work item.
 * @param delay Delay before submitting the work item
 *
 * @retval 0 Work item countdown started.
 * @retval -EINVAL Work item is being processed or has completed its work.
 * @retval -EADDRINUSE Work item was submitted to a different workqueue.
 */
#if 1
static inline int k_delayed_work_submit(struct k_delayed_work *work,
					k_timeout_t delay)
{
	return k_delayed_work_submit_to_queue(&k_sys_work_q, work, delay);
}
#else
#define k_delayed_work_submit(work, delay) \
{\
	if (delay < 10) printf("<%s : %d> delay %d\n", __func__, __LINE__, delay);\
	k_delayed_work_submit_to_queue(&k_sys_work_q, work, delay);\
}
#endif

/**
 * @brief Get time remaining before a delayed work gets scheduled.
 *
 * This routine computes the (approximate) time remaining before a
 * delayed work gets executed. If the delayed work is not waiting to be
 * scheduled, it returns zero.
 *
 * @param work     Delayed work item.
 *
 * @return Remaining time (in milliseconds).
 */
static inline int32_t k_delayed_work_remaining_get(const struct k_delayed_work *work)
{
	return k_ticks_to_ms_floor32(z_timeout_remaining(&work->timeout));
}

/*************************************************************************************************************/
/* k_mutex */
/*************************************************************************************************************/

/** @} */
/**
 * @defgroup mutex_apis Mutex APIs
 * @ingroup kernel_apis
 * @{
 */

/**
 * Mutex Structure
 * @ingroup mutex_apis
 */
struct k_mutex {
	/** Mutex */
	OS_Mutex_t mutex;

	_OBJECT_TRACING_NEXT_PTR(k_mutex)
	_OBJECT_TRACING_LINKED_FLAG
};

/**
 * @cond INTERNAL_HIDDEN
 */
#define Z_MUTEX_INITIALIZER(obj) \
	{ \
	.mutex = {0}, \
	_OBJECT_TRACING_INIT \
	}

/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @brief Statically define and initialize a mutex.
 *
 * The mutex can be accessed outside the module where it is defined using:
 *
 * @code extern struct k_mutex <name>; @endcode
 *
 * @param name Name of the mutex.
 */
#define K_MUTEX_DEFINE(name) \
	Z_STRUCT_SECTION_ITERABLE(k_mutex, name) = \
		Z_MUTEX_INITIALIZER(name)

/**
 * @brief Initialize a mutex.
 *
 * This routine initializes a mutex object, prior to its first use.
 *
 * Upon completion, the mutex is available and does not have an owner.
 *
 * @param mutex Address of the mutex.
 *
 * @retval 0 Mutex object created
 *
 */
int k_mutex_init(struct k_mutex *mutex);


/**
 * @brief Lock a mutex.
 *
 * This routine locks @a mutex. If the mutex is locked by another thread,
 * the calling thread waits until the mutex becomes available or until
 * a timeout occurs.
 *
 * A thread is permitted to lock a mutex it has already locked. The operation
 * completes immediately and the lock count is increased by 1.
 *
 * Mutexes may not be locked in ISRs.
 *
 * @param mutex Address of the mutex.
 * @param timeout Waiting period to lock the mutex,
 *                or one of the special values K_NO_WAIT and
 *                K_FOREVER.
 *
 * @retval 0 Mutex locked.
 * @retval -EBUSY Returned without waiting.
 * @retval -EAGAIN Waiting period timed out.
 */
int k_mutex_lock(struct k_mutex *mutex, k_timeout_t timeout);

/**
 * @brief Unlock a mutex.
 *
 * This routine unlocks @a mutex. The mutex must already be locked by the
 * calling thread.
 *
 * The mutex cannot be claimed by another thread until it has been unlocked by
 * the calling thread as many times as it was previously locked by that
 * thread.
 *
 * Mutexes may not be unlocked in ISRs, as mutexes must only be manipulated
 * in thread context due to ownership and priority inheritance semantics.
 *
 * @param mutex Address of the mutex.
 *
 * @retval 0 Mutex unlocked.
 * @retval -EPERM The current thread does not own the mutex
 * @retval -EINVAL The mutex is not locked
 *
 */
int k_mutex_unlock(struct k_mutex *mutex);

/**
 * @brief Delete a mutex realization by XRadio.
 *
 * Delete a mutex which is created by k_mutex_init. This must paired with
 * k_mutex_init
 *
 * @param mutex Address of the mutex.
 *
 * @retval 0 if mutex is delete successfully, others if fail
 *
 */
int k_mutex_delete(struct k_mutex *mutex);
/** @} */

/*************************************************************************************************************/
/* k_sem */
/*************************************************************************************************************/

/**
 * @cond INTERNAL_HIDDEN
 */

struct k_sem {
	OS_Semaphore_t sem;
	int count;
	int limit;

	_POLL_EVENT;

	_OBJECT_TRACING_NEXT_PTR(k_sem)
	_OBJECT_TRACING_LINKED_FLAG
};

/* k_sem must be inited by k_sem_init */
#define Z_SEM_INITIALIZER(obj, initial_count, count_limit) \
	{ \
	.sem = {0}, \
	.count = initial_count, \
	.limit = count_limit, \
	_POLL_EVENT_OBJ_INIT(obj) \
	_OBJECT_TRACING_INIT \
	}

/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @defgroup semaphore_apis Semaphore APIs
 * @ingroup kernel_apis
 * @{
 */

/**
 * @brief Initialize a semaphore.
 *
 * This routine initializes a semaphore object, prior to its first use.
 *
 * @param sem Address of the semaphore.
 * @param initial_count Initial semaphore count.
 * @param limit Maximum permitted semaphore count.
 *
 * @retval 0 Semaphore created successfully
 * @retval -EINVAL Invalid values
 *
 */
int k_sem_init(struct k_sem *sem, unsigned int initial_count,
               unsigned int limit);

/**
 * @brief Take a semaphore.
 *
 * This routine takes @a sem.
 *
 * @note Can be called by ISRs, but @a timeout must be set to K_NO_WAIT.
 *
 * @param sem Address of the semaphore.
 * @param timeout Waiting period to take the semaphore,
 *                or one of the special values K_NO_WAIT and K_FOREVER.
 *
 * @retval 0 Semaphore taken.
 * @retval -EBUSY Returned without waiting.
 * @retval -EAGAIN Waiting period timed out.
 */
int k_sem_take(struct k_sem *sem, k_timeout_t timeout);

/**
 * @brief Give a semaphore.
 *
 * This routine gives @a sem, unless the semaphore is already at its maximum
 * permitted count.
 *
 * @note Can be called by ISRs.
 *
 * @param sem Address of the semaphore.
 *
 * @return N/A
 */
void k_sem_give(struct k_sem *sem);

/**
 * @brief Reset a semaphore's count to zero.
 *
 * This routine sets the count of @a sem to zero.
 *
 * @param sem Address of the semaphore.
 *
 * @return N/A
 */
//void k_sem_reset(struct k_sem *sem);

/**
 * @brief check whether a semaphore is valid.
 *
 * This routine check @a sem can be used.
 *
 * @param sem Address of the semaphore.
 *
 * @return 1 if sem is exist
 *         0 if sem is not exist
 */
int k_sem_valid(struct k_sem *sem);

/**
 * @brief Delete a semaphore realization by XRadio.
 *
 * Delete a sem which is created by k_sem_init. This must paired with
 * k_sem_init
 *
 * @param sem Address of the semaphore.
 *
 * @return N/A
 * @req K-SEM-001
 */
int k_sem_delete(struct k_sem *sem);

/**
 * @brief Get a semaphore's count.
 *
 * This routine returns the current count of @a sem.
 *
 * @param sem Address of the semaphore.
 *
 * @return Current semaphore count.
 */
unsigned int k_sem_count_get(struct k_sem *sem);

/**
 * @brief Statically define and initialize a semaphore.
 *
 * The semaphore can be accessed outside the module where it is defined using:
 *
 * @code extern struct k_sem <name>; @endcode
 *
 * @param name Name of the semaphore.
 * @param initial_count Initial semaphore count.
 * @param count_limit Maximum permitted semaphore count.
 */
#define K_SEM_DEFINE(name, initial_count, count_limit) \
	Z_STRUCT_SECTION_ITERABLE(k_sem, name) = \
		Z_SEM_INITIALIZER(name, initial_count, count_limit); \
	BUILD_ASSERT(((count_limit) != 0) && \
		     ((initial_count) <= (count_limit)));

/** @} */

/*************************************************************************************************************/
/* k_msgq */
/*************************************************************************************************************/

//tbc...

/*************************************************************************************************************/
/* k_mem */
/*************************************************************************************************************/

/**
 * @defgroup mem_pool_apis Memory Pool APIs
 * @ingroup kernel_apis
 * @{
 */

/* Note on sizing: the use of a 20 bit field for block means that,
 * assuming a reasonable minimum block size of 16 bytes, we're limited
 * to 16M of memory managed by a single pool.  Long term it would be
 * good to move to a variable bit size based on configuration.
 */
struct k_mem_block_id {
	uint32_t pool : 8;
	uint32_t level : 4;
	uint32_t block : 20;
};

struct k_mem_block {
	void *data;
	struct k_mem_block_id id;
};

/** @} */

/*************************************************************************************************************/
/* k_mbox_msg */
/*************************************************************************************************************/

//tbc...

/*************************************************************************************************************/
/* k_pipe */
/*************************************************************************************************************/

//tbc...

/*************************************************************************************************************/
/* time */
/*************************************************************************************************************/

#define k_cycle_get_32() OS_GetTicks()

#define SYS_CLOCK_HW_CYCLES_TO_NS(c) (OS_TicksToMSecs(c) * 1000U * 1000U)

/*************************************************************************************************************/
/* k_mem_slab */
/*************************************************************************************************************/

/**
 * @cond INTERNAL_HIDDEN
 */

struct k_mem_slab {
	_wait_q_t wait_q;
	uint32_t num_blocks;
	size_t block_size;
	char *buffer;
	char *free_list;
	uint32_t num_used;
#ifdef CONFIG_MEM_SLAB_TRACE_MAX_UTILIZATION
	uint32_t max_used;
#endif

	_OBJECT_TRACING_NEXT_PTR(k_mem_slab)
	_OBJECT_TRACING_LINKED_FLAG
};

#define Z_MEM_SLAB_INITIALIZER(obj, slab_buffer, slab_block_size, \
			       slab_num_blocks) \
	{ \
	.wait_q = Z_WAIT_Q_INIT(&obj.wait_q), \
	.num_blocks = slab_num_blocks, \
	.block_size = slab_block_size, \
	.buffer = slab_buffer, \
	.free_list = NULL, \
	.num_used = 0, \
	_OBJECT_TRACING_INIT \
	}

#if defined(CONFIG_BT_DEINIT)
#if !defined(CONFIG_BT_VAR_MEM_DYNC_ALLOC)
#define Z_MEM_SLAB_DEINITIALIZER(obj, slab_buffer, slab_block_size, \
			       slab_num_blocks) \
	{ \
	.wait_q = Z_WAIT_Q_INIT(&obj.wait_q), \
	.num_blocks = slab_num_blocks, \
	.block_size = slab_block_size, \
	.buffer = slab_buffer, \
	.free_list = NULL, \
	.num_used = 0, \
	_OBJECT_TRACING_INIT \
	}
#endif
#endif

/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @defgroup mem_slab_apis Memory Slab APIs
 * @ingroup kernel_apis
 * @{
 */

/**
 * @brief Statically define and initialize a memory slab.
 *
 * The memory slab's buffer contains @a slab_num_blocks memory blocks
 * that are @a slab_block_size bytes long. The buffer is aligned to a
 * @a slab_align -byte boundary. To ensure that each memory block is similarly
 * aligned to this boundary, @a slab_block_size must also be a multiple of
 * @a slab_align.
 *
 * The memory slab can be accessed outside the module where it is defined
 * using:
 *
 * @code extern struct k_mem_slab <name>; @endcode
 *
 * @param name Name of the memory slab.
 * @param slab_block_size Size of each memory block (in bytes).
 * @param slab_num_blocks Number memory blocks.
 * @param slab_align Alignment of the memory slab's buffer (power of 2).
 */
#if defined(K_MEM_SLAB_BY_HEAP)

#define K_MEM_SLAB_DEFINE(name, slab_block_size, slab_num_blocks, slab_align) \
	static uint8_t name = slab_block_size
#if defined(CONFIG_BT_DEINIT)
#define K_MEM_SLAB_DEINIT(name, slab_block_size, slab_num_blocks, slab_align)
#endif
#else /* K_MEM_SLAB_BY_HEAP */

#if defined(CONFIG_BT_VAR_MEM_DYNC_ALLOC)
#define K_MEM_SLAB_DEFINE(name, slab_block_size, slab_num_blocks, slab_align) \
	Z_STRUCT_SECTION_ITERABLE(k_mem_slab, name) = \
		Z_MEM_SLAB_INITIALIZER(name, NULL, WB_UP(slab_block_size), slab_num_blocks)
#else
#define K_MEM_SLAB_DEFINE(name, slab_block_size, slab_num_blocks, slab_align) \
	char __noinit __aligned(WB_UP(slab_align)) \
	   _k_mem_slab_buf_##name[(slab_num_blocks) * WB_UP(slab_block_size)]; \
	Z_STRUCT_SECTION_ITERABLE(k_mem_slab, name) = \
		Z_MEM_SLAB_INITIALIZER(name, _k_mem_slab_buf_##name, \
					WB_UP(slab_block_size), slab_num_blocks)
#endif

#if defined(CONFIG_BT_DEINIT)
/**
 * @brief Statically define and deinitialize a memory slab.
 *
 * The memory slab's buffer contains @a slab_num_blocks memory blocks
 * that are @a slab_block_size bytes long. The buffer is aligned to a
 * @a slab_align -byte boundary. To ensure that each memory block is similarly
 * aligned to this boundary, @a slab_block_size must also be a multiple of
 * @a slab_align.
 *
 * The memory slab can be accessed outside the module where it is defined
 * using:
 *
 * @code extern struct k_mem_slab <name>; @endcode
 *
 * @param name Name of the memory slab.
 * @param slab_block_size Size of each memory block (in bytes).
 * @param slab_num_blocks Number memory blocks.
 * @param slab_align Alignment of the memory slab's buffer (power of 2).
 */
#define K_MEM_SLAB_DEINIT(name, slab_block_size, slab_num_blocks, slab_align) \
		Z_MEM_SLAB_INITIALIZER(name, _k_mem_slab_buf_##name, \
					WB_UP(slab_block_size), slab_num_blocks)
#endif

#if defined(CONFIG_BLEHOST_Z_ITERABLE_SECTION)
int init_mem_slab_module(void);
#if defined(CONFIG_BT_DEINIT)
int deinit_mem_slab_module(void);
void mem_slab_var_deinit(struct k_mem_slab *mem_slab, size_t block_size,
			uint32_t num_blocks);
#endif
#endif
#endif /* K_MEM_SLAB_BY_HEAP */

/**
 * @brief Initialize a memory slab.
 *
 * Initializes a memory slab, prior to its first use.
 *
 * The memory slab's buffer contains @a slab_num_blocks memory blocks
 * that are @a slab_block_size bytes long. The buffer must be aligned to an
 * N-byte boundary matching a word boundary, where N is a power of 2
 * (i.e. 4 on 32-bit systems, 8, 16, ...).
 * To ensure that each memory block is similarly aligned to this boundary,
 * @a slab_block_size must also be a multiple of N.
 *
 * @param slab Address of the memory slab.
 * @param buffer Pointer to buffer used for the memory blocks.
 * @param block_size Size of each memory block (in bytes).
 * @param num_blocks Number of memory blocks.
 *
 * @retval 0 on success
 * @retval -EINVAL invalid data supplied
 *
 */
#if defined(K_MEM_SLAB_BY_HEAP)
#define k_mem_slab_init(slab, buffer, block_size, num_blocks)
#if defiend(CONFIG_BT_DEINIT)
#define k_mem_slab_deinit(slab)
#endif
#else
extern int k_mem_slab_init(struct k_mem_slab *slab, void *buffer,
			   size_t block_size, uint32_t num_blocks);
#if defined(CONFIG_BT_DEINIT)
/**
 * @brief Deinitialize a memory slab.
 *
 * Deinitializes a memory slab, prior to its first use.
 *
 * The memory slab's buffer contains @a slab_num_blocks memory blocks
 * that are @a slab_block_size bytes long. The buffer must be aligned to an
 * N-byte boundary matching a word boundary, where N is a power of 2
 * (i.e. 4 on 32-bit systems, 8, 16, ...).
 * To ensure that each memory block is similarly aligned to this boundary,
 * @a slab_block_size must also be a multiple of N.
 *
 * @param slab Address of the memory slab.
 * @param buffer Pointer to buffer used for the memory blocks.
 * @param block_size Size of each memory block (in bytes).
 * @param num_blocks Number of memory blocks.
 *
 * @retval 0 on success
 * @retval -EINVAL invalid data supplied
 *
 */
extern int k_mem_slab_deinit(struct k_mem_slab *slab);
#endif
#endif /* K_MEM_SLAB_BY_HEAP */

#if defined(CONFIG_BT_VAR_MEM_DYNC_ALLOC)
extern int mem_slab_mem_alloc(struct k_mem_slab *mem_slab);
extern void mem_slab_mem_free(struct k_mem_slab *mem_slab);
#endif

/**
 * @brief Allocate memory from a memory slab.
 *
 * This routine allocates a memory block from a memory slab.
 *
 * @note Can be called by ISRs, but @a timeout must be set to K_NO_WAIT.
 *
 * @param slab Address of the memory slab.
 * @param mem Pointer to block address area.
 * @param timeout Non-negative waiting period to wait for operation to complete.
 *        Use K_NO_WAIT to return without waiting,
 *        or K_FOREVER to wait as long as necessary.
 *
 * @retval 0 Memory allocated. The block address area pointed at by @a mem
 *         is set to the starting address of the memory block.
 * @retval -ENOMEM Returned without waiting.
 * @retval -EAGAIN Waiting period timed out.
 * @retval -EINVAL Invalid data supplied
 */
#if defined(K_MEM_SLAB_BY_HEAP)
#define k_mem_slab_alloc(slab, ppmem, timeout) ((*(ppmem)) = malloc(*(slab))) == NULL
#else
extern int k_mem_slab_alloc(struct k_mem_slab *slab, void **mem,
			    k_timeout_t timeout);
#endif /* K_MEM_SLAB_BY_HEAP */

/**
 * @brief Free memory allocated from a memory slab.
 *
 * This routine releases a previously allocated memory block back to its
 * associated memory slab.
 *
 * @param slab Address of the memory slab.
 * @param mem Pointer to block address area (as set by k_mem_slab_alloc()).
 *
 * @return N/A
 */
#if defined(K_MEM_SLAB_BY_HEAP)
#define k_mem_slab_free(slab, ppmem) free(*(ppmem))
#else
extern void k_mem_slab_free(struct k_mem_slab *slab, void **mem);
#endif /* K_MEM_SLAB_BY_HEAP */

#if !defined(K_MEM_SLAB_BY_HEAP)
/**
 * @brief Get the number of used blocks in a memory slab.
 *
 * This routine gets the number of memory blocks that are currently
 * allocated in @a slab.
 *
 * @param slab Address of the memory slab.
 *
 * @return Number of allocated memory blocks.
 */
static inline uint32_t k_mem_slab_num_used_get(struct k_mem_slab *slab)
{
	return slab->num_used;
}
#endif /* !K_MEM_SLAB_BY_HEAP */

/**
 * @brief Get the number of maximum used blocks so far in a memory slab.
 *
 * This routine gets the maximum number of memory blocks that were
 * allocated in @a slab.
 *
 * @param slab Address of the memory slab.
 *
 * @return Maximum number of allocated memory blocks.
 */
static inline uint32_t k_mem_slab_max_used_get(struct k_mem_slab *slab)
{
#ifdef CONFIG_MEM_SLAB_TRACE_MAX_UTILIZATION
	return slab->max_used;
#else
	ARG_UNUSED(slab);
	return 0;
#endif
}

/**
 * @brief Get the number of unused blocks in a memory slab.
 *
 * This routine gets the number of memory blocks that are currently
 * unallocated in @a slab.
 *
 * @param slab Address of the memory slab.
 *
 * @return Number of unallocated memory blocks.
 */
#if defined(K_MEM_SLAB_BY_HEAP)
#define k_mem_slab_num_free_get(slab) (1)
#else
static inline uint32_t k_mem_slab_num_free_get(struct k_mem_slab *slab)
{
	return slab->num_blocks - slab->num_used;
}
#endif /* K_MEM_SLAB_BY_HEAP */

/** @} */

/*************************************************************************************************************/
/* k_mem_pool */
/*************************************************************************************************************/

//tbc...

/*************************************************************************************************************/
/* heap */
/*************************************************************************************************************/

/**
 * @brief Allocate memory from the heap.
 *
 * This routine provides traditional malloc() semantics. Memory is
 * allocated from the heap memory pool.
 *
 * @param size Amount of memory requested (in bytes).
 *
 * @return Address of the allocated memory if successful; otherwise NULL.
 */
static inline void *k_malloc(size_t size)
{
	return malloc(size);
}

/**
 * @brief Free memory allocated from heap.
 *
 * This routine provides traditional free() semantics. The memory being
 * returned must have been allocated from the heap memory pool or
 * k_mem_pool_malloc().
 *
 * If @a ptr is NULL, no operation is performed.
 *
 * @param ptr Pointer to previously allocated memory.
 *
 * @return N/A
 */
static inline void k_free(void *ptr)
{
	return free(ptr);
}

/**
 * @brief Allocate memory from heap, array style
 *
 * This routine provides traditional calloc() semantics. Memory is
 * allocated from the heap memory pool and zeroed.
 *
 * @param nmemb Number of elements in the requested array
 * @param size Size of each array element (in bytes).
 *
 * @return Address of the allocated memory if successful; otherwise NULL.
 */
static inline void *k_calloc(size_t nmemb, size_t size)
{
	return calloc(nmemb, size);
}

/*************************************************************************************************************/
/* kernel_internal.h */
/*************************************************************************************************************/

/**
 * @brief Allocate some memory from the current thread's resource pool
 *
 * Threads may be assigned a resource pool, which will be used to allocate
 * memory on behalf of certain kernel and driver APIs. Memory reserved
 * in this way should be freed with k_free().
 *
 * If called from an ISR, the k_malloc() system heap will be used if it exists.
 *
 * @param size Memory allocation size
 * @return A pointer to the allocated memory, or NULL if there is insufficient
 * RAM in the pool or there is no pool to draw memory from
 */
static inline void *z_thread_malloc(size_t size)
{
	return k_malloc(size);
}

/*************************************************************************************************************/
/* k_poll */
/*************************************************************************************************************/

/* private, used by k_poll and k_work_poll */
typedef int (*_poller_cb_t)(struct k_poll_event *event, uint32_t state);
struct z_poller {
	volatile int is_polling;
	struct k_thread *thread;
	_poller_cb_t cb;

    OS_Semaphore_t *wait;
	int state;
};

/* polling API - PRIVATE */

#ifdef CONFIG_POLL
#define _INIT_OBJ_POLL_EVENT(obj) do { (obj)->poll_event = NULL; } while (false)
#else
#define _INIT_OBJ_POLL_EVENT(obj) do { } while (false)
#endif

/* private - types bit positions */
enum _poll_types_bits {
	/* can be used to ignore an event */
	_POLL_TYPE_IGNORE,

	/* to be signaled by k_poll_signal_raise() */
	_POLL_TYPE_SIGNAL,

	/* semaphore availability */
	_POLL_TYPE_SEM_AVAILABLE,

	/* queue/FIFO/LIFO data availability */
	_POLL_TYPE_DATA_AVAILABLE,

	_POLL_NUM_TYPES
};

#define Z_POLL_TYPE_BIT(type) (1U << ((type) - 1U))

/* private - states bit positions */
enum _poll_states_bits {
	/* default state when creating event */
	_POLL_STATE_NOT_READY,

	/* signaled by k_poll_signal_raise() */
	_POLL_STATE_SIGNALED,

	/* semaphore is available */
	_POLL_STATE_SEM_AVAILABLE,

	/* data is available to read on queue/FIFO/LIFO */
	_POLL_STATE_DATA_AVAILABLE,

	/* queue/FIFO/LIFO wait was cancelled */
	_POLL_STATE_CANCELLED,

#if defined(CONFIG_BT_DEINIT)
	/* state when exit event raise */
	_POLL_STATE_DEINIT,
#endif

	_POLL_NUM_STATES
};

#define Z_POLL_STATE_BIT(state) (1U << ((state) - 1U))

#define _POLL_EVENT_NUM_UNUSED_BITS \
	(32 - (0 \
	       + 8 /* tag */ \
	       + _POLL_NUM_TYPES \
	       + _POLL_NUM_STATES \
	       + 1 /* modes */ \
	      ))

/* end of polling API - PRIVATE */


/**
 * @defgroup poll_apis Async polling APIs
 * @ingroup kernel_apis
 * @{
 */

/* Public polling API */

/* public - values for k_poll_event.type bitfield */
#define K_POLL_TYPE_IGNORE 0
#define K_POLL_TYPE_SIGNAL Z_POLL_TYPE_BIT(_POLL_TYPE_SIGNAL)
#define K_POLL_TYPE_SEM_AVAILABLE Z_POLL_TYPE_BIT(_POLL_TYPE_SEM_AVAILABLE)
#define K_POLL_TYPE_DATA_AVAILABLE Z_POLL_TYPE_BIT(_POLL_TYPE_DATA_AVAILABLE)
#define K_POLL_TYPE_FIFO_DATA_AVAILABLE K_POLL_TYPE_DATA_AVAILABLE

/* public - polling modes */
enum k_poll_modes {
	/* polling thread does not take ownership of objects when available */
	K_POLL_MODE_NOTIFY_ONLY = 0,

	K_POLL_NUM_MODES
};

/* public - values for k_poll_event.state bitfield */
#define K_POLL_STATE_NOT_READY 0
#define K_POLL_STATE_SIGNALED Z_POLL_STATE_BIT(_POLL_STATE_SIGNALED)
#define K_POLL_STATE_SEM_AVAILABLE Z_POLL_STATE_BIT(_POLL_STATE_SEM_AVAILABLE)
#define K_POLL_STATE_DATA_AVAILABLE Z_POLL_STATE_BIT(_POLL_STATE_DATA_AVAILABLE)
#define K_POLL_STATE_FIFO_DATA_AVAILABLE K_POLL_STATE_DATA_AVAILABLE
#define K_POLL_STATE_CANCELLED Z_POLL_STATE_BIT(_POLL_STATE_CANCELLED)
#if defined(CONFIG_BT_DEINIT)
#define K_POLL_STATE_DEINIT Z_POLL_STATE_BIT(_POLL_STATE_DEINIT)
#endif

/* public - poll signal object */
struct k_poll_signal {
	/** PRIVATE - DO NOT TOUCH */
	sys_dlist_t poll_events;

	/**
	 * 1 if the event has been signaled, 0 otherwise. Stays set to 1 until
	 * user resets it to 0.
	 */
	unsigned int signaled;

	/** custom result value passed to k_poll_signal_raise() if needed */
	int result;
};

#define K_POLL_SIGNAL_INITIALIZER(obj) \
	{ \
	.poll_events = SYS_DLIST_STATIC_INIT(&obj.poll_events), \
	.signaled = 0, \
	.result = 0, \
	}

#if defined(CONFIG_BT_DEINIT)
#define K_POOL_SIGNAL_DEINITIALIZER(obj) \
	{ \
	.poll_events = SYS_DLIST_STATIC_INIT(&obj.poll_events), \
	.signaled = 0, \
	.result = 0, \
	}
#endif

/**
 * @brief Poll Event
 *
 */
struct k_poll_event {
	/** PRIVATE - DO NOT TOUCH */
	sys_dnode_t _node;

	/** PRIVATE - DO NOT TOUCH */
	struct z_poller *poller;

	/** optional user-specified tag, opaque, untouched by the API */
	uint32_t tag:8;

	/** bitfield of event types (bitwise-ORed K_POLL_TYPE_xxx values) */
	uint32_t type:_POLL_NUM_TYPES;

	/** bitfield of event states (bitwise-ORed K_POLL_STATE_xxx values) */
	uint32_t state:_POLL_NUM_STATES;

	/** mode of operation, from enum k_poll_modes */
	uint32_t mode:1;

	/** unused bits in 32-bit word */
	uint32_t unused:_POLL_EVENT_NUM_UNUSED_BITS;

	/** per-type data */
	union {
		void *obj;
		struct k_poll_signal *signal;
		struct k_sem *sem;
		struct k_fifo *fifo;
		struct k_queue *queue;
	};
};

#define K_POLL_EVENT_INITIALIZER(_event_type, _event_mode, _event_obj) \
	{ \
	.poller = NULL, \
	.type = _event_type, \
	.state = K_POLL_STATE_NOT_READY, \
	.mode = _event_mode, \
	.unused = 0, \
	.obj = _event_obj, \
	}

#define K_POLL_EVENT_STATIC_INITIALIZER(_event_type, _event_mode, _event_obj, \
					event_tag) \
	{ \
	.tag = event_tag, \
	.type = _event_type, \
	.state = K_POLL_STATE_NOT_READY, \
	.mode = _event_mode, \
	.unused = 0, \
	.obj = _event_obj, \
	}

/**
 * @brief Initialize one struct k_poll_event instance
 *
 * After this routine is called on a poll event, the event it ready to be
 * placed in an event array to be passed to k_poll().
 *
 * @param event The event to initialize.
 * @param type A bitfield of the types of event, from the K_POLL_TYPE_xxx
 *             values. Only values that apply to the same object being polled
 *             can be used together. Choosing K_POLL_TYPE_IGNORE disables the
 *             event.
 * @param mode Future. Use K_POLL_MODE_NOTIFY_ONLY.
 * @param obj Kernel object or poll signal.
 *
 * @return N/A
 */

extern void k_poll_event_init(struct k_poll_event *event, uint32_t type,
			      int mode, void *obj);

/**
 * @brief Wait for one or many of multiple poll events to occur
 *
 * This routine allows a thread to wait concurrently for one or many of
 * multiple poll events to have occurred. Such events can be a kernel object
 * being available, like a semaphore, or a poll signal event.
 *
 * When an event notifies that a kernel object is available, the kernel object
 * is not "given" to the thread calling k_poll(): it merely signals the fact
 * that the object was available when the k_poll() call was in effect. Also,
 * all threads trying to acquire an object the regular way, i.e. by pending on
 * the object, have precedence over the thread polling on the object. This
 * means that the polling thread will never get the poll event on an object
 * until the object becomes available and its pend queue is empty. For this
 * reason, the k_poll() call is more effective when the objects being polled
 * only have one thread, the polling thread, trying to acquire them.
 *
 * When k_poll() returns 0, the caller should loop on all the events that were
 * passed to k_poll() and check the state field for the values that were
 * expected and take the associated actions.
 *
 * Before being reused for another call to k_poll(), the user has to reset the
 * state field to K_POLL_STATE_NOT_READY.
 *
 * When called from user mode, a temporary memory allocation is required from
 * the caller's resource pool.
 *
 * @param events An array of events to be polled for.
 * @param num_events The number of events in the array.
 * @param timeout Waiting period for an event to be ready,
 *                or one of the special values K_NO_WAIT and K_FOREVER.
 *
 * @retval 0 One or more events are ready.
 * @retval -EAGAIN Waiting period timed out.
 * @retval -EINTR Polling has been interrupted, e.g. with
 *         k_queue_cancel_wait(). All output events are still set and valid,
 *         cancelled event(s) will be set to K_POLL_STATE_CANCELLED. In other
 *         words, -EINTR status means that at least one of output events is
 *         K_POLL_STATE_CANCELLED.
 * @retval -ENOMEM Thread resource pool insufficient memory (user mode only)
 * @retval -EINVAL Bad parameters (user mode only)
 */

int k_poll(struct k_poll_event *events, int num_events,
		     k_timeout_t timeout);

/**
 * @brief Initialize a poll signal object.
 *
 * Ready a poll signal object to be signaled via k_poll_signal_raise().
 *
 * @param signal A poll signal.
 *
 * @return N/A
 */

void k_poll_signal_init(struct k_poll_signal *signal);

/*
 * @brief Reset a poll signal object's state to unsignaled.
 *
 * @param signal A poll signal object
 */
void k_poll_signal_reset(struct k_poll_signal *signal);

/**
 * @brief Fetch the signaled state and result value of a poll signal
 *
 * @param signal A poll signal object
 * @param signaled An integer buffer which will be written nonzero if the
 *		   object was signaled
 * @param result An integer destination buffer which will be written with the
 *		   result value if the object was signaled, or an undefined
 *		   value if it was not.
 */
void k_poll_signal_check(struct k_poll_signal *signal,
				   unsigned int *signaled, int *result);

int k_poll_signal(struct k_poll_signal *signal, int result);

/**
 * @brief Signal a poll signal object.
 *
 * This routine makes ready a poll signal, which is basically a poll event of
 * type K_POLL_TYPE_SIGNAL. If a thread was polling on that event, it will be
 * made ready to run. A @a result value can be specified.
 *
 * The poll signal contains a 'signaled' field that, when set by
 * k_poll_signal_raise(), stays set until the user sets it back to 0 with
 * k_poll_signal_reset(). It thus has to be reset by the user before being
 * passed again to k_poll() or k_poll() will consider it being signaled, and
 * will return immediately.
 *
 * @note The result is stored and the 'signaled' field is set even if
 * this function returns an error indicating that an expiring poll was
 * not notified.  The next k_poll() will detect the missed raise.
 *
 * @param signal A poll signal.
 * @param result The value to store in the result field of the signal.
 *
 * @retval 0 The signal was delivered successfully.
 * @retval -EAGAIN The polling thread's timeout is in the process of expiring.
 */

#define k_poll_signal_raise(psignal, result) k_poll_signal(psignal, result)

/**
 * @internal
 */
extern void z_handle_obj_poll_events(sys_dlist_t *events, uint32_t state);

/** @} */

/*************************************************************************************************************/
/* k_cpu */
/*************************************************************************************************************/

//tbc...

/*************************************************************************************************************/
/* others */
/*************************************************************************************************************/

/**
 * @brief Fatally terminate a thread
 *
 * This should be called when a thread has encountered an unrecoverable
 * runtime condition and needs to terminate. What this ultimately
 * means is determined by the _fatal_error_handler() implementation, which
 * will be called will reason code K_ERR_KERNEL_OOPS.
 *
 * If this is called from ISR context, the default system fatal error handler
 * will treat it as an unrecoverable system error, just like k_panic().
 * @req K-MISC-003
 */
#define k_oops()

/**
 * @brief Fatally terminate the system
 *
 * This should be called when the Zephyr kernel has encountered an
 * unrecoverable runtime condition and needs to terminate. What this ultimately
 * means is determined by the _fatal_error_handler() implementation, which
 * will be called will reason code K_ERR_KERNEL_PANIC.
 */
#define k_panic()

/**
 * @brief Statically define and initialize a stack
 *
 * The stack can be accessed outside the module where it is defined using:
 *
 * @code extern struct k_stack <name>; @endcode
 *
 * @param name Name of the stack.
 * @param stack_num_entries Maximum number of values that can be stacked.
 * @req K-STACK-002
 */
#define K_THREAD_STACK_DEFINE(sym, size) int *sym = (int *)size

/**
 * @brief Calculate size of stacks to be allocated in a stack array
 *
 * This macro calculates the size to be allocated for the stacks
 * inside a stack array. It accepts the indicated "size" as a parameter
 * and if required, pads some extra bytes (e.g. for MPU scenarios). Refer
 * K_THREAD_STACK_ARRAY_DEFINE definition to see how this is used.
 *
 * @param size Size of the stack memory region
 * @req K-TSTACK-001
 */
#define K_THREAD_STACK_SIZEOF(sym) (int)(uintptr_t)(sym)

/**
 * @brief Get a pointer to the physical stack buffer
 *
 * This macro is deprecated. If a stack buffer needs to be examined, the
 * bounds should be obtained from the associated thread's stack_info struct.
 *
 * @param sym Declared stack symbol name
 * @return The buffer itself, a char *
 * @req K-TSTACK-001
 */
char *K_THREAD_STACK_BUFFER(k_thread_stack_t *sym);

/**
 * @def K_KERNEL_STACK_MEMBER
 * @brief Declare an embedded stack memory region
 *
 * Used for kernel stacks embedded within other data structures.
 *
 * Stacks declared with this macro may not host user mode threads.
 * @param sym Thread stack symbol name
 * @param size Size of the stack memory region
 */
#define K_KERNEL_STACK_DEFINE K_THREAD_STACK_DEFINE
#define K_KERNEL_STACK_SIZEOF K_THREAD_STACK_SIZEOF

/**
 * @internal
 */
static inline void z_object_init(void *obj)
{
}

static inline void _k_object_init(void *obj)
{
}

#if defined(CONFIG_BT_DEINIT)
static inline void _k_object_deinit(void * obj)
{
}
#endif

/*************************************************************************************************************/
/* irq.h */
/*************************************************************************************************************/

/**
 * @brief Lock interrupts.
 * @def irq_lock()
 *
 * This routine disables all interrupts on the CPU. It returns an unsigned
 * integer "lock-out key", which is an architecture-dependent indicator of
 * whether interrupts were locked prior to the call. The lock-out key must be
 * passed to irq_unlock() to re-enable interrupts.
 *
 * This routine can be called recursively, as long as the caller keeps track
 * of each lock-out key that is generated. Interrupts are re-enabled by
 * passing each of the keys to irq_unlock() in the reverse order they were
 * acquired. (That is, each call to irq_lock() must be balanced by
 * a corresponding call to irq_unlock().)
 *
 * This routine can only be invoked from supervisor mode. Some architectures
 * (for example, ARM) will fail silently if invoked from user mode instead
 * of generating an exception.
 *
 * @note
 * This routine can be called by ISRs or by threads. If it is called by a
 * thread, the interrupt lock is thread-specific; this means that interrupts
 * remain disabled only while the thread is running. If the thread performs an
 * operation that allows another thread to run (for example, giving a semaphore
 * or sleeping for N milliseconds), the interrupt lock no longer applies and
 * interrupts may be re-enabled while other processing occurs. When the thread
 * once again becomes the current thread, the kernel re-establishes its
 * interrupt lock; this ensures the thread won't be interrupted until it has
 * explicitly released the interrupt lock it established.
 *
 * @warning
 * The lock-out key should never be used to manually re-enable interrupts
 * or to inspect or manipulate the contents of the CPU's interrupt bits.
 *
 * @return An architecture-dependent lock-out key representing the
 *         "interrupt disable state" prior to the call.
 */
unsigned int irq_lock(void);

/**
 * @brief Unlock interrupts.
 * @def irq_unlock()
 *
 * This routine reverses the effect of a previous call to irq_lock() using
 * the associated lock-out key. The caller must call the routine once for
 * each time it called irq_lock(), supplying the keys in the reverse order
 * they were acquired, before interrupts are enabled.
 *
 * This routine can only be invoked from supervisor mode. Some architectures
 * (for example, ARM) will fail silently if invoked from user mode instead
 * of generating an exception.
 *
 * @note Can be called by ISRs.
 *
 * @param key Lock-out key generated by irq_lock().
 *
 * @return N/A
 */
void irq_unlock(unsigned int key);

/*************************************************************************************************************/
/* ffs.h */
/*************************************************************************************************************/

/**
 *
 * @brief find most significant bit set in a 32-bit word
 *
 * This routine finds the first bit set starting from the most significant bit
 * in the argument passed in and returns the index of that bit.  Bits are
 * numbered starting at 1 from the least significant bit.  A return value of
 * zero indicates that the value passed is zero.
 *
 * @return most significant bit set, 0 if @a op is 0
 */

unsigned int find_msb_set(uint32_t data);

/**
 *
 * @brief find least significant bit set in a 32-bit word
 *
 * This routine finds the first bit set starting from the least significant bit
 * in the argument passed in and returns the index of that bit.  Bits are
 * numbered starting at 1 from the least significant bit.  A return value of
 * zero indicates that the value passed is zero.
 *
 * @return least significant bit set, 0 if @a op is 0
 */

unsigned int find_lsb_set(uint32_t data);

/*************************************************************************************************************/
/* check.h */
/*************************************************************************************************************/

#include <ble/sys/__assert.h>

#if defined(CONFIG_ASSERT_ON_ERRORS)
#define CHECKIF(expr) \
	__ASSERT_NO_MSG(!(expr));   \
	if (0)
#elif defined(CONFIG_NO_RUNTIME_CHECKS)
#define CHECKIF(...) \
	if (0)
#else
#define CHECKIF(expr) \
	if (expr)
#endif


/*************************************************************************************************************/
/* object_tracing_common.h */
/*************************************************************************************************************/

#ifdef CONFIG_OBJECT_TRACING
#define SYS_TRACING_OBJ_INIT(name, obj)
#else
#define SYS_TRACING_OBJ_INIT(name, obj)
#endif

#if defined(CONFIG_BT_DEINIT)
#ifdef CONFIG_OBJECT_TRACING
#define SYS_TRACING_OBJ_DEINIT(name, obj)
#else
#define SYS_TRACING_OBJ_DEINIT(name, obj)
#endif
#endif

/*************************************************************************************************************/
/* device.h */
/*************************************************************************************************************/

struct device {
};

/*************************************************************************************************************/
/* spinlock.h */
/*************************************************************************************************************/

/**
 * @brief Kernel Spin Lock
 *
 * This struct defines a spin lock record on which CPUs can wait with
 * k_spin_lock().  Any number of spinlocks may be defined in
 * application code.
 */
struct k_spinlock {
};

/**
 * @brief Spinlock key type
 *
 * This type defines a "key" value used by a spinlock implementation
 * to store the system interrupt state at the time of a call to
 * k_spin_lock().  It is expected to be passed to a matching
 * k_spin_unlock().
 *
 * This type is opaque and should not be inspected by application
 * code.
 */
typedef unsigned int k_spinlock_key_t;

/**
 * @brief Lock a spinlock
 *
 * This routine locks the specified spinlock, returning a key handle
 * representing interrupt state needed at unlock time.  Upon
 * returning, the calling thread is guaranteed not to be suspended or
 * interrupted on its current CPU until it calls k_spin_unlock().  The
 * implementation guarantees mutual exclusion: exactly one thread on
 * one CPU will return from k_spin_lock() at a time.  Other CPUs
 * trying to acquire a lock already held by another CPU will enter an
 * implementation-defined busy loop ("spinning") until the lock is
 * released.
 *
 * Separate spin locks may be nested. It is legal to lock an
 * (unlocked) spin lock while holding a different lock.  Spin locks
 * are not recursive, however: an attempt to acquire a spin lock that
 * the CPU already holds will deadlock.
 *
 * In circumstances where only one CPU exists, the behavior of
 * k_spin_lock() remains as specified above, though obviously no
 * spinning will take place.  Implementations may be free to optimize
 * in uniprocessor contexts such that the locking reduces to an
 * interrupt mask operation.
 *
 * @param l A pointer to the spinlock to lock
 * @return A key value that must be passed to k_spin_unlock() when the
 *         lock is released.
 */

static ALWAYS_INLINE k_spinlock_key_t k_spin_lock(struct k_spinlock *l)
{
	return irq_lock();
}

/**
 * @brief Unlock a spin lock
 *
 * This releases a lock acquired by k_spin_lock().  After this
 * function is called, any CPU will be able to acquire the lock.  If
 * other CPUs are currently spinning inside k_spin_lock() waiting for
 * this lock, exactly one of them will return synchronously with the
 * lock held.
 *
 * Spin locks must be properly nested.  A call to k_spin_unlock() must
 * be made on the lock object most recently locked using
 * k_spin_lock(), using the key value that it returned.  Attempts to
 * unlock mis-nested locks, or to unlock locks that are not held, or
 * to passing a key parameter other than the one returned from
 * k_spin_lock(), are illegal.  When CONFIG_SPIN_VALIDATE is set, some
 * of these errors can be detected by the framework.
 *
 * @param l A pointer to the spinlock to release
 * @param key The value returned from k_spin_lock() when this lock was
 *        acquired
 */
static ALWAYS_INLINE void k_spin_unlock(struct k_spinlock *l,
					k_spinlock_key_t key)
{
	irq_unlock(key);
}

/*************************************************************************************************************/
/* XRadio */
/*************************************************************************************************************/

#define BLE_HOST_VERSION "XRadio BLE HOST V2.5.0"

#define XRADIO 1

//#define XR829 1

#define GET_VARIABLE_NAME(Variable) (#Variable)

#define HOSTMINI_LOG(fmt, arg...)
//printf

/*
 * Warning! ZEPHYR_IRQ_LOCK_BY_MUTEX might cause multi-thread problem, the irq
 * lock might not criticalize all the variables because irq disable is more
 * save. It also might cause ble performence reduction.
 */
#define ZEPHYR_IRQ_LOCK_BY_MUTEX 1
/*
 * !CAN'T USE! ZEPHYR_IRQ_LOCK_BY_ARCH_IRQ might cause ble controller irq delay.
 */
#define ZEPHYR_IRQ_LOCK_BY_ARCH_IRQ 0

#if IS_ENABLED(ZEPHYR_IRQ_LOCK_BY_MUTEX)
int irq_lock_init(void);
#if defined(CONFIG_BT_DEINIT)
int irq_lock_deinit(void);
#endif
#endif

/* Xradio:workaround for cygwin Kconfig problem */
#if (CONFIG_BT_CREATE_CONN_TIMEOUT && CONFIG_BT_RPA_TIMEOUT)
#if ((CONFIG_BT_CREATE_CONN_TIMEOUT > CONFIG_BT_RPA_TIMEOUT) && (CONFIG_BT_RPA_TIMEOUT < 655))
#undef CONFIG_BT_CREATE_CONN_TIMEOUT
#define CONFIG_BT_CREATE_CONN_TIMEOUT CONFIG_BT_RPA_TIMEOUT
#endif
#endif

#if (CONFIG_BT_MESH_RX_SEG_MAX && CONFIG_BT_MESH_TX_SEG_MAX && CONFIG_BT_MESH_SEG_BUFS)
#if ((CONFIG_BT_MESH_RX_SEG_MAX > CONFIG_BT_MESH_TX_SEG_MAX) && (CONFIG_BT_MESH_SEG_BUFS < CONFIG_BT_MESH_RX_SEG_MAX))
#undef CONFIG_BT_MESH_SEG_BUFS
#define CONFIG_BT_MESH_SEG_BUFS 64
#endif
#endif
/* Xradio:workaround for cygwin Kconfig problem */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_KERNEL_H_ */
