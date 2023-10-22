#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <hal_sem.h>
#include <hal_time.h>
#include <hal_interrupt.h>
#include <hal_event.h>
#include <hal_osal.h>
#include <hal_status.h>


int hal_event_init(hal_event_t ev)
{
	hal_event_t pevent;

	pevent = xEventGroupCreateStatic((StaticEventGroup_t *)ev);
	if (!pevent)
		return HAL_NOMEM;
	return HAL_OK;
}

int hal_event_datach(hal_event_t ev)
{
	(void)ev;
	return HAL_OK;
}

hal_event_t hal_event_create(void)
{
	return xEventGroupCreate();
}

int hal_event_delete(hal_event_t ev)
{
	if (!ev)
		return HAL_INVALID;

	vEventGroupDelete(ev);
	return HAL_OK;
}

hal_event_bits_t hal_event_wait(hal_event_t ev, hal_event_bits_t evs, uint8_t option, unsigned long timeout)
{
	int clear, and;
	hal_event_bits_t ret;

	if (!ev)
		return HAL_INVALID;

	if (option & HAL_EVENT_OPTION_AND) {
		and = 1;
	} else if (option & HAL_EVENT_OPTION_OR) {
		and = 0;
	} else {
		return HAL_INVALID;
	}

	if (option & HAL_EVENT_OPTION_CLEAR)
		clear = 1;
	else
		clear = 0;

	return xEventGroupWaitBits(ev, evs, clear, and, timeout);
}

int hal_event_set_bits(hal_event_t ev, hal_event_bits_t evs)
{
	if (!ev)
		return HAL_INVALID;

	if (hal_interrupt_get_nest()) {
		BaseType_t xHigherPriorityTaskWoken, xResult;

		xResult = xEventGroupSetBitsFromISR(ev, evs, &xHigherPriorityTaskWoken);
		if (xResult != pdFAIL)
			return HAL_OK;
		else
			return HAL_ERROR;
	} else {
		hal_event_bits_t bits;
		bits = xEventGroupSetBits(ev, evs);
		if (bits > 0)
			return HAL_OK;
		else
			return HAL_ERROR;
	}
}

hal_event_bits_t hal_event_get(hal_event_t ev)
{
	if (hal_interrupt_get_nest())
		return xEventGroupGetBitsFromISR(ev);
	else
		return xEventGroupGetBits(ev);
}
