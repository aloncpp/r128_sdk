/*
********************************************************************************
* @file    porting.c
* @brief   This file contain the definition of zephyr kernel functions.
********************************************************************************
*/
#define _PORTING_C_

#include <zephyr.h>
#include <portmacro.h>
#include <net/buf.h>
#include <string.h>
//#include "driver/chip/hal_cmsis.h"
#include "sys/interrupt.h"
#include "kernel/os/os.h"
#include "kernel/os/os_util.h"

#ifdef CONFIG_COMPONENTS_BT_PM
#include "bt_pm.h"
#endif

#define OS_Status XR_OS_Status
#define OS_Priority XR_OS_Priority
#define OS_ThreadHandle_t XR_OS_ThreadHandle_t
#define OS_ThreadDelete XR_OS_ThreadDelete
#define OS_ThreadCreate XR_OS_ThreadCreate
#define OS_TimerIsValid XR_OS_TimerIsValid
#define OS_GetTicks XR_OS_GetTicks
#define OS_SemaphoreIsValid XR_OS_SemaphoreIsValid
#define OS_TimerStop XR_OS_TimerStop
#define OS_TimerCreate XR_OS_TimerCreate
#define OS_TimerDelete XR_OS_TimerDelete
#define OS_TimerChangePeriod XR_OS_TimerChangePeriod
#define OS_TimerStart XR_OS_TimerStart
#define OS_OK XR_OS_OK
#define OS_TIMER_ONCE XR_OS_TIMER_ONCE
#define OS_FAIL XR_OS_FAIL
#define OS_E_NOMEM XR_OS_E_NOMEM
#define OS_E_PARAM XR_OS_E_PARAM
#define OS_E_TIMEOUT XR_OS_E_TIMEOUT
#define OS_E_ISR XR_OS_E_ISR
#define OS_RecursiveMutexDelete XR_OS_RecursiveMutexDelete
#define OS_RecursiveMutexUnlock XR_OS_RecursiveMutexUnlock
#define OS_RecursiveMutexLock XR_OS_RecursiveMutexLock
#define OS_RecursiveMutexCreate XR_OS_RecursiveMutexCreate
#define OS_TimerIsActive XR_OS_TimerIsActive
#define OS_MSleep XR_OS_MSleep
#define OS_ThreadIsValid XR_OS_ThreadIsValid
#define OS_ThreadGetCurrentHandle XR_OS_ThreadGetCurrentHandle
#define OS_WAIT_FOREVER XR_OS_WAIT_FOREVER
#define OS_MutexIsValid XR_OS_MutexIsValid

#ifdef BT_DBG_ENABLED //modify for compiler error
#undef BT_DBG_ENABLED
#endif
#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_RESOURCES_USE)
#include "common/log.h"

#if defined(CONFIG_BT_DEBUG_RESOURCES_USE)
int ble_mutex_cnt = 0;
int ble_sem_cnt = 0;
int ble_thread_cnt = 0;
#endif

int os_status_to_errno(OS_Status status)
{
	int ret;
	switch (status) {
		case OS_OK:
			ret = 0;
			break;
		case OS_FAIL:
			ret = -EPERM;
			break;
		case OS_E_NOMEM:
			ret = -ENOMEM;
			break;
		case OS_E_PARAM:
			ret = -EINVAL;
			break;
		case OS_E_TIMEOUT:
			ret = -ETIME;
			break;
		case OS_E_ISR:
			ret = -EACCES;
			break;
		default:
			ret = -EPERM;
			break;
	}
	return ret;
}

/*************************************************************************************************************/
/* k_thread */
/*************************************************************************************************************/
static void k_wrap_thread(void * arg)
{
	struct k_thread *this_thread = (struct k_thread *)arg;

	this_thread->entry(this_thread->p1, this_thread->p2, this_thread->p3);

#if defined(CONFIG_BT_DEINIT)
	this_thread->entry = NULL;
	this_thread->p1 = NULL;
	this_thread->p2 = NULL;
	this_thread->p3 = NULL;
#endif

#if defined(CONFIG_BT_DEBUG_RESOURCES_USE)
	ble_thread_cnt--;
#endif

    OS_ThreadDelete(&this_thread->task);
}

#define CONFIG_ZEPHYR_THREAD_EXTRA_STACK (0)

#define K_THREAD_EXTRA_STACK (CONFIG_ZEPHYR_THREAD_EXTRA_STACK)

k_tid_t k_thread_create(struct k_thread *new_thread,
				  k_thread_stack_t *stack,
				  size_t stack_size,
				  k_thread_entry_t entry,
				  void *p1, void *p2, void *p3,
				  int prio, uint32_t options, k_timeout_t delay)
{
    int ret;
    new_thread->entry = entry;
    new_thread->p1 = p1;
    new_thread->p2 = p2;
    new_thread->p3 = p3;

    if (prio > 6 || prio < 0) {
        BT_ERR("create task with error prio \n");
        return NULL;
    }

	/*
	 * (4 * stack_size) is compatative to the old k_thread_create by tw, this
	 * should be optimized
	 */
    ret = OS_ThreadCreate(&new_thread->task, "Zephyr", k_wrap_thread, new_thread,
    					  (OS_Priority)prio, (4 * stack_size) + K_THREAD_EXTRA_STACK);
    if (ret) {
        BT_ERR("create ble task fail\n");
		return NULL;
    }

#if defined(CONFIG_BT_DEBUG_RESOURCES_USE)
    ble_thread_cnt++;
#endif

    return new_thread;
}

int k_thread_join(struct k_thread *thread, k_timeout_t delay)
{
	OS_ThreadHandle_t self = OS_ThreadGetCurrentHandle();
	uint32_t start_time = 0;
	int elapsed = 0, done = 0;

	/* can not wait for current thread exit, it will be dead lock */
	if (thread->task.handle == self) {
		return -EDEADLK;
	}

	if (delay != K_FOREVER) {
		start_time = OS_TicksToMSecs(OS_GetTicks());
	}

	/* if not terminated yet, suspend ourselves */
	while (OS_ThreadIsValid(&thread->task) && !done) {
		if (delay != K_FOREVER) {
			elapsed = OS_TicksToMSecs(OS_GetTicks()) - start_time;
			done = elapsed > delay;
		}

		OS_MSleep(5);
	}

	return 0;
}

/*************************************************************************************************************/
/* timeout.c */
/*************************************************************************************************************/
int z_abort_timeout(struct _timeout *to)
{
	int ret = -1;
	BT_DBG("to %p, tm %p", to, &to->handle);

	if (to && OS_TimerIsValid(&to->handle)) {
		if (OS_TimerStop(&to->handle) != OS_OK)
			BT_ERR("timer stop err\n");
		BT_DBG("to %p, tm %p", to, &to->handle);
#ifndef ZEPHYR_OS_WORK_NOT_DELETE
		ret = (int)OS_TimerDelete(&to->handle);
		BT_DBG("to %p, tm %p", to, &to->handle);
#endif
	}

	return ret;
}

void z_add_timeout(struct _timeout *to, _timeout_func_t fn, k_timeout_t ticks)
{
	to->expiry = ticks + OS_GetTicks();

	BT_DBG("to %p, tm %p", to, &to->handle);

	if (ticks < 10)
		BT_WARN("tick %d\n", ticks);

	if (!OS_TimerIsValid(&to->handle)) {
		OS_TimerCreate(&to->handle, OS_TIMER_ONCE, (void (*)(void *))fn,
	                   to, OS_TicksToMSecs(ticks));
	} else {
		OS_TimerChangePeriod(&to->handle, OS_TicksToMSecs(ticks));
		BT_WARN("to %p, tm %p %d", to, &to->handle, OS_TicksToMSecs(ticks));
	}

	if (!OS_TimerIsValid(&to->handle)) {
		BT_WARN("to %p, tm %p", to, &to->handle);
	} else if (OS_TimerStart(&to->handle) != OS_OK) {
		BT_WARN("to %p, tm %p", to, &to->handle);
	}
}

int32_t z_timeout_remaining(const struct _timeout *to)
{
	return OS_TicksToMSecs(to->expiry - OS_GetTicks());
}

bool z_is_inactive_timeout(struct _timeout *t)
{
	return !OS_TimerIsActive(&t->handle);
}

/*************************************************************************************************************/
/* k_uptime */
/*************************************************************************************************************/

uint32_t k_uptime_get_32(void)
{
	return (uint32_t)OS_TicksToMSecs(OS_GetTicks());
}

int64_t k_uptime_get(void)
{
	/* Warning! 32bit to 64bit might cause overflow error! */
	return (int64_t)OS_TicksToMSecs(OS_GetTicks());
}


/*************************************************************************************************************/
/* k_sem */
/*************************************************************************************************************/

int k_sem_init(struct k_sem *sem, unsigned int initial_count,
               unsigned int limit)
{
	/*
	 * Limit cannot be zero and count cannot be greater than limit
	 */
	if (sem == NULL || limit == 0U || initial_count > limit) {
		return -EINVAL;
	}

	unsigned int key = irq_lock();
    if (OS_SemaphoreCreate(&sem->sem, initial_count, limit) != OS_OK)
		BT_WARN("");
	sem->count = initial_count;
	sem->limit = limit;
#if defined(CONFIG_POLL)
	sys_dlist_init(&sem->poll_events);
#endif
	irq_unlock(key);

#if defined(CONFIG_BT_DEBUG_RESOURCES_USE)
	ble_sem_cnt++;
#endif

	return 0;
}

int k_sem_take(struct k_sem *sem, k_timeout_t timeout)
{
	OS_Status res;
	unsigned int key = irq_lock();
	sem->count--;
	irq_unlock(key);
	res = OS_SemaphoreWait(&sem->sem, timeout);
	return os_status_to_errno(res);
}

static inline void handle_poll_events(struct k_sem *sem)
{
#ifdef CONFIG_POLL
		/* there's no need to handle poll events in ble host right now. */
//why can't use?
		z_handle_obj_poll_events(&sem->poll_events, K_POLL_STATE_SEM_AVAILABLE);
#else
		ARG_UNUSED(sem);
#endif
}

void k_sem_give(struct k_sem *sem)
{
    if (NULL == sem) {
		return;
    }

	unsigned int key = irq_lock();
	sem->count += (sem->count != sem->limit) ? 1U : 0U;
	/*
	 * !Warning, our definition is different to zephyr because it's
	 * not easy to realize without os interface like pthread_cond_wait
	 * if thread is blocking by sem. Can we optimize it?
	 */
	handle_poll_events(sem);
	irq_unlock(key);
	/* what if count == limit but after unlock and before sem release count < limit */
    OS_SemaphoreRelease(&sem->sem);
}

int k_sem_valid(struct k_sem *sem)
{
	return OS_SemaphoreIsValid(&sem->sem);
}

int k_sem_delete(struct k_sem *sem)
{
	if (sem == NULL) {
		return -EINVAL;
	}

	if (!OS_SemaphoreIsValid(&sem->sem)) {
		return -EINVAL;
	}

	unsigned int key = irq_lock();
	if (OS_SemaphoreDelete(&sem->sem) != OS_OK) {
		BT_WARN("");
	}
	sem->count = 0;
	sem->limit = 0;
#if defined(CONFIG_POLL)
	sys_dlist_init(&sem->poll_events);
#endif
	irq_unlock(key);

#if defined(CONFIG_BT_DEBUG_RESOURCES_USE)
	ble_sem_cnt--;
#endif

	return 0;
}

unsigned int k_sem_count_get(struct k_sem *sem)
{
	int32_t ret = sem->count;
    return (ret > 0) ? ret : 0;
}

/*************************************************************************************************************/
/* k_mutex */
/*************************************************************************************************************/
int k_mutex_init(struct k_mutex *mutex)
{
	if (mutex == NULL) {
		return -EINVAL;
	}

	if (OS_RecursiveMutexCreate(&mutex->mutex) != OS_OK) {
		BT_WARN("");
	}
	z_object_init(mutex);

#if defined(CONFIG_BT_DEBUG_RESOURCES_USE)
	ble_mutex_cnt++;
#endif

	return 0;
}

int k_mutex_lock(struct k_mutex *mutex, k_timeout_t timeout)
{
	OS_Status ret;

	ret = OS_RecursiveMutexLock(&mutex->mutex, timeout);
	return os_status_to_errno(ret);
}

int k_mutex_unlock(struct k_mutex *mutex)
{
	OS_Status ret;

	ret = OS_RecursiveMutexUnlock(&mutex->mutex);
	return os_status_to_errno(ret);
}

int k_mutex_delete(struct k_mutex *mutex)
{
	if (mutex == NULL) {
		return -EINVAL;
	}

	if (OS_RecursiveMutexDelete(&mutex->mutex) != OS_OK) {
		BT_WARN("");
	}
	z_object_init(mutex);

#if defined(CONFIG_BT_DEBUG_RESOURCES_USE)
	ble_mutex_cnt--;
#endif

	return 0;
}

/*************************************************************************************************************/
/* k_sleep */
/*************************************************************************************************************/
int32_t k_sleep(k_timeout_t timeout)
{
    OS_MSleep(timeout);
	return k_ms_to_ticks_ceil32(timeout);
}

/*************************************************************************************************************/
/* others */
/*************************************************************************************************************/
int arch_is_in_isr(void)
{
	return XR_OS_IsISRContext();
}

char *K_THREAD_STACK_BUFFER(k_thread_stack_t *sym)
{
	return (char *)sym;
}

#if IS_ENABLED(ZEPHYR_IRQ_LOCK_BY_MUTEX)
static OS_Mutex_t g_irq_lock;

int irq_lock_init(void)
{
	if (!OS_MutexIsValid(&g_irq_lock))
		return OS_RecursiveMutexCreate(&g_irq_lock);
	else
		return -1;
}
#endif

#if IS_ENABLED(ZEPHYR_IRQ_LOCK_BY_ARCH_IRQ)
static unsigned long irq_lock_cnt;
#endif

//static uint32_t task_enter_critical_sr;

unsigned int irq_lock(void)
{
#if IS_ENABLED(ZEPHYR_IRQ_LOCK_BY_MUTEX)
	OS_RecursiveMutexLock(&g_irq_lock, OS_WAIT_FOREVER);
#elif IS_ENABLED(ZEPHYR_IRQ_LOCK_BY_ARCH_IRQ)
	if (irq_lock_cnt == 0)
		arch_irq_disable();
	irq_lock_cnt++;
#else
    taskENTER_CRITICAL();
#endif
    return 0;
}

void irq_unlock(unsigned int key)
{
#if IS_ENABLED(ZEPHYR_IRQ_LOCK_BY_MUTEX)
	OS_RecursiveMutexUnlock(&g_irq_lock);
#elif IS_ENABLED(ZEPHYR_IRQ_LOCK_BY_ARCH_IRQ)
	if (irq_lock_cnt != 0)
		irq_lock_cnt--;
	if (irq_lock_cnt ==0)
		arch_irq_enable();
#else
    taskEXIT_CRITICAL();
#endif
}

#if defined(CONFIG_BT_DEINIT)
int irq_lock_deinit(void)
{
#if IS_ENABLED(ZEPHYR_IRQ_LOCK_BY_MUTEX)
	if (OS_MutexIsValid(&g_irq_lock)) {
		return OS_RecursiveMutexDelete(&g_irq_lock);
	} else {
		BT_ERR("Irq lock is not valid");
		return -1;
	}
#elif IS_ENABLED(ZEPHYR_IRQ_LOCK_BY_ARCH_IRQ)
	if (irq_lock_cnt != 0) {
		BT_ERR("Irq lock count is not zero (count:%d)", irq_lock_cnt);
		return -1;
	} else {
		return 0;
	}
#endif

	return 0;
}
#endif

unsigned int find_msb_set(uint32_t data)
{
    uint32_t count = 0;
    uint32_t mask  = 0x80000000;

    if (!data) {
        return 0;
    }
    while ((data & mask) == 0) {
        count += 1u;
        mask = mask >> 1u;
    }
    return (32 - count);
}

unsigned int find_lsb_set(uint32_t data)
{
    uint32_t count = 0;
    uint32_t mask  = 0x00000001;

    if (!data) {
        return 0;
    }
    while ((data & mask) == 0) {
        count += 1u;
        mask = mask << 1u;
    }
    return (count + 1);
}

void posix_exit(int exit_code)
{
}

#if defined(CONFIG_BT_DEBUG_RESOURCES_USE)
void ble_resource_info(void)
{
    BT_DBG("<<< ble host resource info >>>");
    BT_DBG("ble mutex cnt:   %d", ble_mutex_cnt);
    BT_DBG("ble sem cnt:   %d", ble_sem_cnt);
    BT_DBG("ble thread cnt:   %d", ble_thread_cnt);
}
#endif

