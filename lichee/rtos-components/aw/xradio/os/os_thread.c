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

#include "kernel/os/os_debug.h"
#include "kernel/os/os_thread.h"
#include "kernel/os/os_time.h"
#include <FreeRTOS.h>
#include <task.h>
#include <hal_thread.h>

XR_OS_Status XR_OS_ThreadCreate(XR_OS_Thread_t *thread, const char *name,
                          XR_OS_ThreadEntry_t entry, void *arg,
                          XR_OS_Priority priority, uint32_t stackSize)
{
	BaseType_t ret;
	UBaseType_t prio;

	switch (priority) {
	case XR_OS_PRIORITY_IDLE:
		prio = HAL_THREAD_PRIORITY_LOWEST;
		break;
	case XR_OS_PRIORITY_LOW:
		prio = HAL_THREAD_PRIORITY_LOWEST + 1;
		break;
	case XR_OS_PRIORITY_BELOW_NORMAL:
		prio = HAL_THREAD_PRIORITY_MIDDLE - 1;
		break;
	case XR_OS_PRIORITY_NORMAL:
		prio = HAL_THREAD_PRIORITY_MIDDLE;
		break;
	case XR_OS_PRIORITY_ABOVE_NORMAL:
		prio = HAL_THREAD_PRIORITY_MIDDLE + 1;
		break;
	case XR_OS_PRIORITY_HIGH:
		prio = HAL_THREAD_PRIORITY_HIGHEST - 1;
		break;
	case XR_OS_PRIORITY_REAL_TIME:
		prio = HAL_THREAD_PRIORITY_HIGHEST;
		break;
	default:
		prio = HAL_THREAD_PRIORITY_MIDDLE;
		break;
	}

	XR_OS_HANDLE_ASSERT(!XR_OS_ThreadIsValid(thread), thread->handle);

	ret = xTaskCreate(entry, name, stackSize / sizeof(StackType_t), arg,
	                  prio, (TaskHandle_t * const)&thread->handle);
	if (ret != pdPASS) {
		XR_OS_ERR("err %"XR_OS_BASETYPE_F"\n", ret);
		XR_OS_ThreadSetInvalid(thread);
		return XR_OS_FAIL;
	}
	return XR_OS_OK;
}

XR_OS_Status XR_OS_ThreadDelete(XR_OS_Thread_t *thread)
{
	TaskHandle_t handle;
	TaskHandle_t curHandle;

	if (thread == NULL) {
		vTaskDelete(NULL); /* delete self */
		return XR_OS_OK;
	}

	XR_OS_HANDLE_ASSERT(XR_OS_ThreadIsValid(thread), thread->handle);

	handle = thread->handle;
	curHandle = xTaskGetCurrentTaskHandle();
	if (handle == curHandle) {
		/* delete self */
		XR_OS_ThreadSetInvalid(thread);
		vTaskDelete(NULL);
	} else {
		/* delete other thread */
		XR_OS_WRN("thread %"XR_OS_HANDLE_F" delete %"XR_OS_HANDLE_F"\n", curHandle, handle);
		vTaskDelete(handle);
		XR_OS_ThreadSetInvalid(thread);
	}

	return XR_OS_OK;
}

void XR_OS_ThreadSleep(XR_OS_Time_t msec)
{
    vTaskDelay((TickType_t)XR_OS_MSecsToTicks(msec));
}

void XR_OS_ThreadYield(void)
{
	taskYIELD();
}

XR_OS_ThreadHandle_t XR_OS_ThreadGetCurrentHandle(void)
{
	return (XR_OS_ThreadHandle_t)xTaskGetCurrentTaskHandle();
}

void XR_OS_ThreadStartScheduler(void)
{
	vTaskStartScheduler();
}

void XR_OS_ThreadSuspendScheduler(void)
{
	vTaskSuspendAll();
}

void XR_OS_ThreadResumeScheduler(void)
{
	xTaskResumeAll();
}

int XR_OS_ThreadIsSchedulerRunning(void)
{
	return (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING);
}

#if INCLUDE_uxTaskGetStackHighWaterMark
uint32_t XR_OS_ThreadGetStackMinFreeSize(XR_OS_Thread_t *thread)
{
	TaskHandle_t handle;

	if (thread != NULL) {
		if (XR_OS_ThreadIsValid(thread)) {
			handle = thread->handle;
		} else {
			return 0;
		}
	} else {
		handle = NULL;
	}

	return (uxTaskGetStackHighWaterMark(handle) * sizeof(StackType_t));
}

#endif
