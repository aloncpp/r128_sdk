#ifndef SUNXI_HAL_SEM_H
#define SUNXI_HAL_SEM_H

#ifdef __cplusplus
extern "C"
{
#endif
#include <stddef.h>
#include <stdint.h>

#ifdef CONFIG_KERNEL_FREERTOS
#include <FreeRTOS.h>
#include <semphr.h>
#elif defined(CONFIG_RTTKERNEL)
#include <rtthread.h>
#else
#error "can not support the RTOS!!"
#endif

typedef struct hal_sem {
#ifdef CONFIG_KERNEL_FREERTOS
	StaticSemaphore_t entry;
	SemaphoreHandle_t ptr;
#elif defined(CONFIG_RTTKERNEL)
	struct rt_semaphore entry;
	rt_sem_t ptr;
#endif
} *hal_sem_t;

/**
 * hal_sem_init - Init a semaphore
 *
 * @sem: a point that point to struct hal_sem entry
 * @cnt: sem init val
 *
 */
void hal_sem_init(hal_sem_t sem, unsigned int cnt);

/**
 * hal_sem_deinit - Deinit a semaphore
 *
 * @sem: a point that point to struct hal_sem entry
 *
 */
void hal_sem_deinit(hal_sem_t sem);

/**
 * hal_sem_create - Create a semaphore
 *
 * @cnt: sem init val
 *
 * Return pointer to rpmsg buffer on success, or NULL on failure.
 */
hal_sem_t hal_sem_create(unsigned int cnt);

/**
 * hal_sem_delete - delete a semaphore
 *
 * @sem: a point that create by hal_sem_create
 *
 * Return HAL_OK on success
 */
int hal_sem_delete(hal_sem_t sem);

/**
 * hal_sem_getvalue - get current semaphore value
 *
 * @sem: a semaphore pointer
 * @val: [out] current semaphore value
 *
 * Return HAL_OK on success
 */
int hal_sem_getvalue(hal_sem_t sem, int *val);

/**
 * hal_sem_post - post semaphore
 *
 * @sem: a semaphore pointer
 * @val: [out] current semaphore value
 *
 * Return HAL_OK on success, or HAL_ERROR on failure
 */
int hal_sem_post(hal_sem_t sem);

/**
 * hal_sem_timedwait - wait semaphore until acquire semaphore or timeout
 *
 * @sem: a semaphore pointer
 * @ticks: timeout ticks, can use MS_TO_OSTICK to
 *         convert ms to ticks
 *
 * Return HAL_OK on success, or HAL_ERROR on failure
 */
int hal_sem_timedwait(hal_sem_t sem, unsigned long ticks);

/**
 * hal_sem_trywait - try to acquire semaphore
 *
 * @sem: a semaphore pointer
 *
 * Return HAL_OK on success, or HAL_ERROR on failure
 */
int hal_sem_trywait(hal_sem_t sem);

/**
 * hal_sem_wait - try to acquire semaphore forever
 *
 * @sem: a semaphore pointer
 *
 * Return HAL_OK on success, or HAL_ERROR on failure
 */
int hal_sem_wait(hal_sem_t sem);

/**
 * hal_sem_clear - reset semaphore
 *
 * @sem: a semaphore pointer
 *
 * Return HAL_OK on success, or HAL_ERROR on failure
 */
int hal_sem_clear(hal_sem_t sem);

#ifdef __cplusplus
}
#endif
#endif
