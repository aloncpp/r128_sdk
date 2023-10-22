#include <stdio.h>
#include <string.h>
#include <hal_queue.h>
#include <hal_interrupt.h>
#include <hal_status.h>

hal_queue_t hal_queue_create(const char *name, unsigned int item_size,
			unsigned int queue_size)
{
	return xQueueCreate(queue_size, item_size);
}

int hal_queue_delete(hal_queue_t queue)
{
	int ret;
	if (hal_interrupt_get_nest())
		ret = uxQueueMessagesWaitingFromISR(queue);
	else
		ret = uxQueueMessagesWaiting(queue);

	if (ret > 0) {
		return HAL_ERROR;
	}

	vQueueDelete(queue);

	return HAL_OK;
}

int hal_queue_send_wait(hal_queue_t queue, void *buffer, int timeout)
{
	BaseType_t ret;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	if (hal_interrupt_get_nest()) {
		ret = xQueueSendFromISR(queue, buffer, &xHigherPriorityTaskWoken);
		if (ret == pdPASS)
			portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
	else
		ret = xQueueSend(queue, buffer, timeout);

	if (ret != pdPASS)
		return HAL_ERROR;
	return HAL_OK;
}

int hal_queue_send(hal_queue_t queue, void *buffer)
{
	return hal_queue_send_wait(queue, buffer, 0);
}

int hal_queue_recv(hal_queue_t queue, void *buffer, int timeout)
{
	BaseType_t ret;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	if (hal_interrupt_get_nest()) {
		ret = xQueueReceiveFromISR(queue, buffer, &xHigherPriorityTaskWoken);
		if (ret == pdPASS)
			portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
	else
		ret = xQueueReceive(queue, buffer, timeout);

	if (ret != pdPASS)
		return HAL_ERROR;

	return HAL_OK;
}

int hal_is_queue_empty(hal_queue_t queue)
{
	int arg;

	if (hal_interrupt_get_nest())
		arg = uxQueueMessagesWaitingFromISR(queue);
	else
		arg = uxQueueMessagesWaiting(queue);

	return (arg == 0) ? 1 : 0;
}

hal_mailbox_t hal_mailbox_create(const char *name, unsigned int size)
{
	return (hal_mailbox_t)hal_queue_create(name, sizeof(unsigned int), size);
}


int hal_mailbox_delete(hal_mailbox_t mailbox)
{
	return hal_queue_delete((hal_queue_t)mailbox);
}

int hal_mailbox_send_wait(hal_mailbox_t mailbox, unsigned int value, int timeout)
{
	return hal_queue_send_wait((hal_queue_t)mailbox, &value, timeout);
}

int hal_mailbox_send(hal_mailbox_t mailbox, unsigned int value)
{
	return hal_mailbox_send_wait(mailbox, value, 0);
}

int hal_mailbox_recv(hal_mailbox_t mailbox, unsigned int *value, int timeout)
{
	return hal_queue_recv((hal_queue_t)mailbox, value, timeout);
}

int hal_is_mailbox_empty(hal_mailbox_t mailbox)
{
	return hal_is_queue_empty((hal_queue_t)mailbox);
}

