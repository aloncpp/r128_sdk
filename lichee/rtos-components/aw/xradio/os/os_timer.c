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

#include <kernel/os/os_time.h>
#include <kernel/os/os_timer.h>
#include "kernel/os/os_debug.h"
#include <stdbool.h>
#include <stdio.h>
#include <FreeRTOS.h>
#include <timers.h>
#include <kernel/os/os_util.h>

/* TODO: what block time should be used ? */
#define XR_OS_TIMER_WAIT_FOREVER	portMAX_DELAY
#define XR_OS_TIMER_WAIT_NONE		0

#define XR_OS_TIMER_USE_FREERTOS_ORIG_CALLBACK	1

#if XR_OS_TIMER_USE_FREERTOS_ORIG_CALLBACK

/* Timer private data definition */
typedef struct XR_OS_TimerPriv {
    TimerHandle_t       handle;   /* Timer handle */
    XR_OS_TimerCallback_t  callback; /* Timer expire callback function */
    void               *argument; /* Argument of timer expire callback function */
} XR_OS_TimerPriv_t;

static void XR_OS_TimerPrivCallback(TimerHandle_t xTimer)
{
	XR_OS_TimerPriv_t *priv;

	priv = pvTimerGetTimerID(xTimer);
	if (priv && priv->callback) {
		priv->callback(priv->argument);
	} else {
		XR_OS_WRN("Invalid timer callback\n");
	}
}

XR_OS_Status XR_OS_TimerCreate(XR_OS_Timer_t *timer, XR_OS_TimerType type,
                               XR_OS_TimerCallback_t cb, void *arg, uint32_t periodMS)
{
	XR_OS_TimerPriv_t *priv;

	XR_OS_HANDLE_ASSERT(!XR_OS_TimerIsValid(timer), timer->handle);

	priv = XR_OS_Malloc(sizeof(XR_OS_TimerPriv_t));
	if (priv == NULL) {
		return XR_OS_E_NOMEM;
	}

	priv->callback = cb;
	priv->argument = arg;
	priv->handle = xTimerCreate("",
	                            XR_OS_MSecsToTicks(periodMS),
	                            type == XR_OS_TIMER_PERIODIC ? pdTRUE : pdFALSE,
	                            priv,
	                            XR_OS_TimerPrivCallback);
	if (priv->handle == NULL) {
		XR_OS_ERR("err %"XR_OS_HANDLE_F"\n", priv->handle);
		XR_OS_Free(priv);
		return XR_OS_FAIL;
	}
	timer->handle = priv;

	return XR_OS_OK;
}

static __inline TimerHandle_t XR_OS_TimerGetKernelHandle(XR_OS_Timer_t *timer)
{
	XR_OS_TimerPriv_t *priv = timer->handle;
	return priv->handle;
}

#else /* XR_OS_TIMER_USE_FREERTOS_ORIG_CALLBACK */

XR_OS_Status XR_OS_TimerCreate(XR_OS_Timer_t *timer, XR_OS_TimerType type,
                               XR_OS_TimerCallback_t cb, void *arg, uint32_t periodMS)
{
	XR_OS_HANDLE_ASSERT(!XR_OS_TimerIsValid(timer), timer->handle);

	timer->handle = xTimerCreate("",
	                             XR_OS_MSecsToTicks(periodMS),
	                             type == XR_OS_TIMER_PERIODIC ? pdTRUE : pdFALSE,
	                             arg,
	                             cb);
	if (timer->handle == NULL) {
		XR_OS_ERR("err %"XR_OS_HANDLE_F"\n", timer->handle);
		return XR_OS_FAIL;
	}
	return XR_OS_OK;
}

static __inline TimerHandle_t XR_OS_TimerGetKernelHandle(XR_OS_Timer_t *timer)
{
	return timer->handle;
}

#endif /* XR_OS_TIMER_USE_FREERTOS_ORIG_CALLBACK */

XR_OS_Status XR_OS_TimerDelete(XR_OS_Timer_t *timer)
{
	TimerHandle_t handle;
	BaseType_t ret;

	XR_OS_HANDLE_ASSERT(XR_OS_TimerIsValid(timer), timer->handle);

	handle = XR_OS_TimerGetKernelHandle(timer);
	ret = xTimerDelete(handle, XR_OS_TIMER_WAIT_FOREVER);
	if (ret != pdPASS) {
		XR_OS_ERR("err %"XR_OS_BASETYPE_F"\n", ret);
		return XR_OS_FAIL;
	}

#if XR_OS_TIMER_USE_FREERTOS_ORIG_CALLBACK
	XR_OS_TimerPriv_t *priv = timer->handle;
#endif
	XR_OS_TimerSetInvalid(timer);
#if XR_OS_TIMER_USE_FREERTOS_ORIG_CALLBACK
	XR_OS_Free(priv);
#endif
	return XR_OS_OK;
}

XR_OS_Status XR_OS_TimerStart(XR_OS_Timer_t *timer)
{
	TimerHandle_t handle;
	BaseType_t ret;
	BaseType_t taskWoken;

	XR_OS_HANDLE_ASSERT(XR_OS_TimerIsValid(timer), timer->handle);

	handle = XR_OS_TimerGetKernelHandle(timer);

	if (XR_OS_IsISRContext()) {
		taskWoken = pdFALSE;
		ret = xTimerStartFromISR(handle, &taskWoken);
		if (ret != pdPASS) {
			XR_OS_ERR("err %"XR_OS_BASETYPE_F"\n", ret);
			return XR_OS_FAIL;
		}
		portEND_SWITCHING_ISR(taskWoken);
	} else {
		ret = xTimerStart(handle, XR_OS_TIMER_WAIT_NONE);
		if (ret != pdPASS) {
			XR_OS_ERR("err %"XR_OS_BASETYPE_F"\n", ret);
			return XR_OS_FAIL;
		}
	}

	return XR_OS_OK;
}

XR_OS_Status XR_OS_TimerChangePeriod(XR_OS_Timer_t *timer, XR_OS_Time_t periodMS)
{
	TimerHandle_t handle;
	BaseType_t ret;
	BaseType_t taskWoken;

	XR_OS_HANDLE_ASSERT(XR_OS_TimerIsValid(timer), timer->handle);

	handle = XR_OS_TimerGetKernelHandle(timer);

	if (XR_OS_IsISRContext()) {
		taskWoken = pdFALSE;
		ret = xTimerChangePeriodFromISR(handle, periodMS, &taskWoken);
		if (ret != pdPASS) {
			XR_OS_ERR("err %"XR_OS_BASETYPE_F"\n", ret);
			return XR_OS_FAIL;
		}
		portEND_SWITCHING_ISR(taskWoken);
	} else {
		ret = xTimerChangePeriod(handle, periodMS, XR_OS_TIMER_WAIT_NONE);
		if (ret != pdPASS) {
			XR_OS_ERR("err %"XR_OS_BASETYPE_F"\n", ret);
			return XR_OS_FAIL;
		}
	}

	return XR_OS_OK;
}

XR_OS_Status XR_OS_TimerStop(XR_OS_Timer_t *timer)
{
	TimerHandle_t handle;
	BaseType_t ret;
	BaseType_t taskWoken;

	XR_OS_HANDLE_ASSERT(XR_OS_TimerIsValid(timer), timer->handle);

	handle = XR_OS_TimerGetKernelHandle(timer);

	if (XR_OS_IsISRContext()) {
		taskWoken = pdFALSE;
		ret = xTimerStopFromISR(handle, &taskWoken);
		if (ret != pdPASS) {
			XR_OS_ERR("err %"XR_OS_BASETYPE_F"\n", ret);
			return XR_OS_FAIL;
		}
		portEND_SWITCHING_ISR(taskWoken);
	} else {
		ret = xTimerStop(handle, XR_OS_TIMER_WAIT_FOREVER);
		if (ret != pdPASS) {
			XR_OS_ERR("err %"XR_OS_BASETYPE_F"\n", ret);
			return XR_OS_FAIL;
		}
	}

	return XR_OS_OK;
}

int XR_OS_TimerIsActive(XR_OS_Timer_t *timer)
{
	TimerHandle_t handle;

	if (!XR_OS_TimerIsValid(timer)) {
		return 0;
	}

	handle = XR_OS_TimerGetKernelHandle(timer);

	return (xTimerIsTimerActive(handle) != pdFALSE);
}
