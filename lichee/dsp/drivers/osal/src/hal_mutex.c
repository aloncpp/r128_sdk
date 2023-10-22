#include <hal_mutex.h>
#include <hal_interrupt.h>
#include <hal_status.h>

hal_mutex_t hal_mutex_create(void)
{
	/* default not support recursive */
	return xSemaphoreCreateMutex();
}

int hal_mutex_delete(hal_mutex_t mutex)
{
	if (mutex == NULL) {
		return HAL_INVALID;
	}
	vSemaphoreDelete(mutex);
	return 0;
}

int hal_mutex_unlock(hal_mutex_t mutex)
{
	BaseType_t ret;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	if (mutex == NULL) {
		return HAL_INVALID;
	}

	if (hal_interrupt_get_nest()) {
		ret = xSemaphoreGiveFromISR(mutex, &xHigherPriorityTaskWoken);
		if (ret == pdPASS) {
			portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
		}
	}
	else
		ret = xSemaphoreGive(mutex);

	if (ret != pdPASS) {
		return HAL_ERROR;
	}

	return HAL_OK;
}

int hal_mutex_timedwait(hal_mutex_t mutex, int ticks)
{
	TickType_t xDelay = (TickType_t)ticks;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	BaseType_t ret;

	if (mutex == NULL) {
		return HAL_INVALID;
	}

	if (hal_interrupt_get_nest()) {
		ret = xSemaphoreTakeFromISR(mutex, &xHigherPriorityTaskWoken);
		if (ret == pdPASS) {
			portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
		}
	}
	else
		ret = xSemaphoreTake(mutex, xDelay);

	if (ret != pdPASS)
		return HAL_ERROR;

	return HAL_OK;
}

int hal_mutex_lock(hal_mutex_t mutex)
{
	TickType_t xDelay = portMAX_DELAY;

	return hal_mutex_timedwait(mutex, xDelay);
}

int hal_mutex_trylock(hal_mutex_t mutex)
{
	TickType_t xDelay = 0;

	return hal_mutex_timedwait(mutex, xDelay);
}

