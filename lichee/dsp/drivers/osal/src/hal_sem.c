#include <hal_osal.h>
#include <sunxi_hal_common.h>
#include <queue.h>
#include <hal_status.h>

void hal_sem_init(hal_sem_t sem, unsigned int cnt)
{
	sem->ptr = xSemaphoreCreateCountingStatic(-1, cnt, &sem->entry);
	hal_assert(sem->ptr);
}

void hal_sem_deinit(hal_sem_t sem)
{
	sem->ptr = NULL;
}

hal_sem_t hal_sem_create(unsigned int cnt)
{
	hal_sem_t sem;

	sem = hal_malloc(sizeof(*sem));
	if (!sem)
		return NULL;

	hal_sem_init(sem, cnt);
	return sem;
}

int hal_sem_delete(hal_sem_t sem)
{
	hal_sem_deinit(sem);

	return HAL_OK;
}

int hal_sem_getvalue(hal_sem_t sem, int *val)
{
	hal_assert(sem != NULL);

	*val = uxSemaphoreGetCount(sem->ptr);

	return HAL_OK;
}

int hal_sem_post(hal_sem_t sem)
{
	BaseType_t ret;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	/* Give the semaphore using the FreeRTOS API. */
	if (hal_interrupt_get_nest()) {
		ret = xSemaphoreGiveFromISR(sem->ptr, &xHigherPriorityTaskWoken);
		if (ret == pdPASS)
			portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
	else
		ret = xSemaphoreGive(sem->ptr);

	if (ret == pdPASS)
		return HAL_OK;
	return HAL_ERROR;
}

int hal_sem_timedwait(hal_sem_t sem, unsigned long ticks)
{
	TickType_t xDelay = (TickType_t)ticks;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	BaseType_t ret;

	if (hal_interrupt_get_nest()) {
		ret = xSemaphoreTakeFromISR(sem->ptr, &xHigherPriorityTaskWoken);
		if (ret == pdTRUE) {
			portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
		}
	}
	else
		ret = xSemaphoreTake(sem->ptr, xDelay);

	if (ret != pdTRUE)
		return HAL_ERROR;

	return HAL_OK;
}

int hal_sem_trywait(hal_sem_t sem)
{
	return hal_sem_timedwait(sem, 0);
}

int hal_sem_wait(hal_sem_t sem)
{
	return hal_sem_timedwait(sem, portMAX_DELAY);
}

int hal_sem_clear(hal_sem_t sem)
{
	xQueueReset((QueueHandle_t)sem->ptr);

	return HAL_OK;
}
