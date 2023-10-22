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
#include "kernel/os/os_semaphore.h"
#include "semphr.h"
#include <kernel/os/os_util.h>

XR_OS_Status XR_OS_SemaphoreCreate(XR_OS_Semaphore_t *sem, uint32_t initCount, XR_OS_Count_t maxCount)
{
//	XR_OS_HANDLE_ASSERT(!XR_OS_SemaphoreIsValid(sem), sem->handle);

	sem->handle = xSemaphoreCreateCounting(maxCount, initCount);
	if (sem->handle == NULL) {
		XR_OS_ERR("err %"XR_OS_HANDLE_F"\n", sem->handle);
		return XR_OS_FAIL;
	}

	return XR_OS_OK;
}

XR_OS_Status XR_OS_SemaphoreCreateBinary(XR_OS_Semaphore_t *sem)
{
//	XR_OS_HANDLE_ASSERT(!XR_OS_SemaphoreIsValid(sem), sem->handle);

	sem->handle = xSemaphoreCreateBinary();
	if (sem->handle == NULL) {
		XR_OS_ERR("err %"XR_OS_HANDLE_F"\n", sem->handle);
		return XR_OS_FAIL;
	}

	return XR_OS_OK;
}

XR_OS_Status XR_OS_SemaphoreDelete(XR_OS_Semaphore_t *sem)
{
	XR_OS_HANDLE_ASSERT(XR_OS_SemaphoreIsValid(sem), sem->handle);

	vSemaphoreDelete(sem->handle);
	XR_OS_SemaphoreSetInvalid(sem);
	return XR_OS_OK;
}

XR_OS_Status XR_OS_SemaphoreWait(XR_OS_Semaphore_t *sem, XR_OS_Time_t waitMS)
{
	BaseType_t ret;
	BaseType_t taskWoken;

	XR_OS_HANDLE_ASSERT(XR_OS_SemaphoreIsValid(sem), sem->handle);

	if (XR_OS_IsISRContext()) {
		if (waitMS != 0) {
			XR_OS_ERR("%s() in ISR, wait %u ms\n", __func__, waitMS);
			return XR_OS_E_ISR;
		}
		taskWoken = pdFALSE;
		ret = xSemaphoreTakeFromISR(sem->handle, &taskWoken);
		if (ret != pdPASS) {
			XR_OS_DBG("%s() fail @ %d\n", __func__, __LINE__);
			return XR_OS_E_TIMEOUT;
		}
		portEND_SWITCHING_ISR(taskWoken);
	} else {
		ret = xSemaphoreTake(sem->handle, XR_OS_CalcWaitTicks(waitMS));
		if (ret != pdPASS) {
			XR_OS_DBG("%s() fail @ %d, %"XR_OS_TIME_F" ms\n", __func__, __LINE__, waitMS);
			return XR_OS_E_TIMEOUT;
		}
	}

	return XR_OS_OK;
}

XR_OS_Status XR_OS_SemaphoreRelease(XR_OS_Semaphore_t *sem)
{
	BaseType_t ret;
	BaseType_t taskWoken;

	XR_OS_HANDLE_ASSERT(XR_OS_SemaphoreIsValid(sem), sem->handle);

	if (XR_OS_IsISRContext()) {
		taskWoken = pdFALSE;
		ret = xSemaphoreGiveFromISR(sem->handle, &taskWoken);
		if (ret != pdPASS) {
			XR_OS_DBG("%s() fail @ %d\n", __func__, __LINE__);
			return XR_OS_FAIL;
		}
		portEND_SWITCHING_ISR(taskWoken);
	} else {
		ret = xSemaphoreGive(sem->handle);
		if (ret != pdPASS) {
			XR_OS_DBG("%s() fail @ %d\n", __func__, __LINE__);
			return XR_OS_FAIL;
		}
	}

	return XR_OS_OK;
}
