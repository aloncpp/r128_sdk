#ifndef SUNXI_HAL_WAITQUEUE_H
#define SUNXI_HAL_WAITQUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <hal_time.h>
#include <hal_thread.h>
#include <hal_atomic.h>
#include <hal_event.h>
#include <hal_sem.h>
#include <hal_status.h>

#define HAL_WQ_FLAG_EXCLUSIVE		0x01
#define HAL_WQ_FLAG_WOKEN			0x02
#define HAL_WQ_FLAG_BOOKMARK		0x04

#define HAL_WQ_NOTIFY_MAGIC			(0xf3f4a1a2)

/*
 *	NOTE:
 *		Useage:
 *			init:
 *				hal_waitqueue_head_t wq;
 *				hal_waitqueue_head_init(&wq);
 *			deinit:
 *				hal_waitqueue_head_deinit(&wq)
 *
 *			task1:
 *				...
 *				if (hal_wait_event_timeout(wq, condition, wait_ms) <= 0) {
 *					...
 *					return -ETIMEOU
 *				}
 *				...
 *			task2:
 *				condition = xxxxx
 *				hal_wake_up(&wq)
 *				...
 */
struct __hal_wait_queue;
typedef struct __hal_wait_queue hal_waitqueue_t;
typedef int (*wait_queue_func_t)(hal_waitqueue_t *wait, void *key);

struct hal_waitqueue_head {
	hal_spinlock_t lock;
	struct list_head head;
};
typedef struct hal_waitqueue_head hal_waitqueue_head_t;

struct __hal_wait_queue {
	unsigned int flags;
	wait_queue_func_t func;
	struct list_head task_list;
	struct hal_sem sem;
};

long prepare_to_wait_event(hal_waitqueue_head_t *q, hal_waitqueue_t *wait);
void init_wait_entry(hal_waitqueue_t *wait, unsigned int flags);
void finish_wait(hal_waitqueue_head_t *q, hal_waitqueue_t *wait);
void __hal_wake_up(hal_waitqueue_head_t *q, int nr_exclusive, void *key);

static inline long __wait_yeild_cpu(hal_waitqueue_t *wait, int ms)
{
	hal_tick_t tick = ms < 0 ? HAL_WAIT_FOREVER : MS_TO_OSTICK(ms);
	hal_tick_t old, diff;

	old = hal_tick_get();
	hal_sem_timedwait(&wait->sem, tick);
	diff = hal_tick_get();

	if (diff >= old) {
		diff -= old;
	} else {
		/* overflow */
		diff -= old;
		diff -= 1;
		diff = ~diff;
	}
	diff = diff > tick ? tick : diff;

	/* update remain tick */
	if (ms >= 0)
		tick -= diff;

	return OSTICK_TO_MS(tick);
}

static inline void __add_wait_queue(struct hal_waitqueue_head *wq_head, hal_waitqueue_t *wq_entry)
{
	list_add(&wq_entry->task_list, &wq_head->head);
}

static inline void __add_hal_waitqueue_tail(struct hal_waitqueue_head *wq_head, hal_waitqueue_t *wq_entry)
{
	list_add_tail(&wq_entry->task_list, &wq_head->head);
}

#define hal_waitqueue_head_init(q) \
    do {                                     \
        hal_spin_lock_init(&(q)->lock);  \
        INIT_LIST_HEAD(&(q)->head);      \
    } while (0)

#define hal_waitqueue_head_deinit(q) \
    do {                                     \
        hal_spin_lock_deinit(&(q)->lock);  \
    } while (0)

#define ___wait_event(wq, condition, exclusive, ret, cmd)    \
({                                     \
	hal_waitqueue_t __wait;               \
	long __ret = ret;                  \
                                       \
	init_wait_entry(&__wait, exclusive ? HAL_WQ_FLAG_EXCLUSIVE : 0); \
	for (;;) {                         \
		long __int = prepare_to_wait_event(&wq, &__wait); \
                                       \
		if ((condition) || __int)      \
			break;                     \
		cmd;                           \
	}                                  \
	finish_wait(&wq, &__wait);         \
	__ret;                             \
})

#define hal_wait_event(wq_head, condition)        \
  do {                                        \
      if (condition)                          \
          break;                              \
    (void)___wait_event(wq_head, condition, 0, 0, \
             __ret = __wait_yeild_cpu(&__wait, -1)); \
  } while (0)

/*
 *	return 1, if condition == true || remain_main == 0
 *	else return 0
 */
#define ___wait_cond_timeout(condition)                 \
({                                  \
    int __cond = (condition);                  \
    if (__cond && !__ret)                       \
        __ret = 1;                      \
    __cond || !__ret;                       \
})

/*
 * publish function: hal_wait_event_timeout(wq, condition, ms);
 *
 * return:
 * 0 if the @condition evaluated to %false after the @timeout elapsed,
 * 1 if the @condition evaluated to %true after the @timeout elapsed,
 * or the remaining ms (at least 1) if the @condition evaluated,
 * to %true before the @timeout elapsed.
 */
#define hal_wait_event_timeout(wq_head, condition, timeout) \
({                                        \
	int __ret = timeout;                    \
	if (!___wait_cond_timeout(condition))  \
		__ret = ___wait_event(wq_head, \
			___wait_cond_timeout(condition), 0, timeout, \
				__ret = __wait_yeild_cpu(&__wait, __ret)); \
	__ret; \
})

#define hal_wait_event_timeout_exclusive(wq_head, condition, timeout) \
({                                        \
	int __ret = timeout;                    \
	if (!___wait_cond_timeout(condition))  \
		__ret = ___wait_event(wq_head, \
			___wait_cond_timeout(condition), 1, timeout, \
				__ret = __wait_yeild_cpu(&__wait, __ret)) \
	__ret; \
})

/*
 * hal_wake_up function
 * hal_wake_up(hal_waitqueue_head_t *q);
 * hal_wake_up_nr(hal_waitqueue_head_t *q, int nr_exclusive);
 * hal_wake_up_all(hal_waitqueue_head_t *q);
 */
#define hal_wake_up(x)              __hal_wake_up(x, 1, NULL)
#define hal_wake_up_nr(x, nr)       __hal_wake_up(x, nr, NULL)
#define hal_wake_up_all(x)          __hal_wake_up(x, 0, NULL)

#ifdef __cplusplus
}
#endif

#endif

