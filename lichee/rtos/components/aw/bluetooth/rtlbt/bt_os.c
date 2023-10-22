
#include "FreeRTOS.h"
#include <semphr.h>
#include "bt_os.h"

BTOS_Status BTOS_QueueCreate(BTOS_Queue_t *queue, uint32_t queueLen, uint32_t itemSize)
{

	queue->handle = xQueueCreate(queueLen, itemSize);
	if (queue->handle == NULL) {
		return BTOS_FAIL;
	}

	return BTOS_OK;
}

BTOS_Status BTOS_QueueDelete(BTOS_Queue_t *queue)
{
	UBaseType_t ret;

	ret = uxQueueMessagesWaiting(queue->handle);
	if (ret > 0) {
		return BTOS_FAIL;
	}

	vQueueDelete(queue->handle);
	return BTOS_OK;
}

BTOS_Status BTOS_QueueSend(BTOS_Queue_t *queue, const void *item, BTOS_Time_t waitMS)
{
	BaseType_t ret;
	BaseType_t taskWoken;

	ret = xQueueSend(queue->handle, item, waitMS);
	if (ret != pdPASS) {
		return BTOS_FAIL;
	}
	return BTOS_OK;
}

BTOS_Status BTOS_QueueReceive(BTOS_Queue_t *queue, void *item, BTOS_Time_t waitMS)
{
	BaseType_t ret;

	ret = xQueueReceive(queue->handle, item, pdMS_TO_TICKS(waitMS));
	if (ret != pdPASS) {
		return BTOS_FAIL;
	}

	return BTOS_OK;
}
