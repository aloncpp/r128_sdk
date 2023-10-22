/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the people's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.
*
*
* THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
* PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
* WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
* THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
* OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
* IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
* NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
* OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <fcntl.h>
#include <time.h>
#include <stdio.h>
#include "kernel/os/os_debug.h"
#include <kernel/os/os_queue.h>
#include <kernel/os/os_util.h>

XR_OS_Status XR_OS_QueueCreate(XR_OS_Queue_t *queue, uint32_t queueLen, uint32_t itemSize)
{
//	XR_OS_HANDLE_ASSERT(!XR_OS_QueueIsValid(queue), queue->handle);

	queue->handle = xQueueCreate(queueLen, itemSize);
	if (queue->handle == NULL) {
		XR_OS_ERR("err %"XR_OS_HANDLE_F"\n", queue->handle);
		return XR_OS_FAIL;
	}

	return XR_OS_OK;
}

XR_OS_Status XR_OS_QueueDelete(XR_OS_Queue_t *queue)
{
	UBaseType_t ret;

	XR_OS_HANDLE_ASSERT(XR_OS_QueueIsValid(queue), queue->handle);

	ret = uxQueueMessagesWaiting(queue->handle);
	if (ret > 0) {
		XR_OS_ERR("queue %"XR_OS_HANDLE_F" is not empty\n", queue->handle);
		return XR_OS_FAIL;
	}

	vQueueDelete(queue->handle);
	XR_OS_QueueSetInvalid(queue);
	return XR_OS_OK;
}

XR_OS_Status XR_OS_QueueSend(XR_OS_Queue_t *queue, const void *item, XR_OS_Time_t waitMS)
{
	BaseType_t ret;
	BaseType_t taskWoken;

	XR_OS_HANDLE_ASSERT(XR_OS_QueueIsValid(queue), queue->handle);

	if (XR_OS_IsISRContext()) {
		taskWoken = pdFALSE;
		ret = xQueueSendFromISR(queue->handle, item, &taskWoken);
		if (ret != pdPASS) {
			XR_OS_DBG("%s() fail @ %d\n", __func__, __LINE__);
			return XR_OS_FAIL;
		}
		portEND_SWITCHING_ISR(taskWoken);
	} else {
		ret = xQueueSend(queue->handle, item, XR_OS_CalcWaitTicks(waitMS));
		if (ret != pdPASS) {
			XR_OS_DBG("%s() fail @ %d, %"XR_OS_TIME_F" ms\n", __func__, __LINE__, waitMS);
			return XR_OS_FAIL;
		}
	}

	return XR_OS_OK;
}

XR_OS_Status XR_OS_QueueReceive(XR_OS_Queue_t *queue, void *item, XR_OS_Time_t waitMS)
{
	BaseType_t ret;
	BaseType_t taskWoken;

	XR_OS_HANDLE_ASSERT(XR_OS_QueueIsValid(queue), queue->handle);

	if (XR_OS_IsISRContext()) {
		taskWoken = pdFALSE;
		ret = xQueueReceiveFromISR(queue->handle, item, &taskWoken);
		if (ret != pdPASS) {
			XR_OS_DBG("%s() fail @ %d\n", __func__, __LINE__);
			return XR_OS_FAIL;
		}
		portEND_SWITCHING_ISR(taskWoken);
	} else {
		ret = xQueueReceive(queue->handle, item, XR_OS_CalcWaitTicks(waitMS));
		if (ret != pdPASS) {
			XR_OS_DBG("%s() fail @ %d, %"XR_OS_TIME_F" ms\n", __func__, __LINE__, waitMS);
			return XR_OS_FAIL;
		}
	}

	return XR_OS_OK;
}
