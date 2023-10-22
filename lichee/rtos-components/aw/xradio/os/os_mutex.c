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
#include <kernel/os/os_mutex.h>
#include <stdio.h>
#include <FreeRTOS.h>
#include <semphr.h>
#include <kernel/os/os_util.h>

XR_OS_Status XR_OS_MutexCreate(XR_OS_Mutex_t *mutex)
{
	XR_OS_HANDLE_ASSERT(!XR_OS_MutexIsValid(mutex), mutex->handle);

	mutex->handle = xSemaphoreCreateMutex();
	if (mutex->handle == NULL) {
		XR_OS_ERR("err %"XR_OS_HANDLE_F"\n", mutex->handle);
		return XR_OS_FAIL;
	}

	return XR_OS_OK;
}

XR_OS_Status XR_OS_MutexDelete(XR_OS_Mutex_t *mutex)
{
	XR_OS_HANDLE_ASSERT(XR_OS_MutexIsValid(mutex), mutex->handle);

	vSemaphoreDelete(mutex->handle);
	XR_OS_MutexSetInvalid(mutex);
	return XR_OS_OK;
}

XR_OS_Status XR_OS_MutexLock(XR_OS_Mutex_t *mutex, XR_OS_Time_t waitMS)
{
	BaseType_t ret;

	XR_OS_HANDLE_ASSERT(XR_OS_MutexIsValid(mutex), mutex->handle);

	ret = xSemaphoreTake(mutex->handle, XR_OS_CalcWaitTicks(waitMS));
	if (ret != pdPASS) {
		XR_OS_DBG("%s() fail @ %d, %"XR_OS_TIME_F" ms\n", __func__, __LINE__, waitMS);
		return XR_OS_FAIL;
	}

	return XR_OS_OK;
}

XR_OS_Status XR_OS_MutexUnlock(XR_OS_Mutex_t *mutex)
{
	BaseType_t ret;

	XR_OS_HANDLE_ASSERT(XR_OS_MutexIsValid(mutex), mutex->handle);

	ret = xSemaphoreGive(mutex->handle);
	if (ret != pdPASS) {
		XR_OS_DBG("%s() fail @ %d\n", __func__, __LINE__);
		return XR_OS_FAIL;
	}

	return XR_OS_OK;
}

XR_OS_Status XR_OS_RecursiveMutexCreate(XR_OS_Mutex_t *mutex)
{
	XR_OS_HANDLE_ASSERT(!XR_OS_MutexIsValid(mutex), mutex->handle);

	mutex->handle = xSemaphoreCreateRecursiveMutex();
	if (mutex->handle == NULL) {
		XR_OS_ERR("err %"XR_OS_HANDLE_F"\n", mutex->handle);
		return XR_OS_FAIL;
	}

	return XR_OS_OK;
}

XR_OS_Status XR_OS_RecursiveMutexDelete(XR_OS_Mutex_t *mutex)
{
	return XR_OS_MutexDelete(mutex);
}

XR_OS_Status XR_OS_RecursiveMutexLock(XR_OS_Mutex_t *mutex, XR_OS_Time_t waitMS)
{
	BaseType_t ret;

	XR_OS_HANDLE_ASSERT(XR_OS_MutexIsValid(mutex), mutex->handle);

	ret = xSemaphoreTakeRecursive(mutex->handle, XR_OS_CalcWaitTicks(waitMS));
	if (ret != pdPASS) {
		XR_OS_DBG("%s() fail @ %d, %"XR_OS_TIME_F" ms\n", __func__, __LINE__, waitMS);
		return XR_OS_FAIL;
	}

	return XR_OS_OK;
}

XR_OS_Status XR_OS_RecursiveMutexUnlock(XR_OS_Mutex_t *mutex)
{
	BaseType_t ret;

	XR_OS_HANDLE_ASSERT(XR_OS_MutexIsValid(mutex), mutex->handle);

	ret = xSemaphoreGiveRecursive(mutex->handle);
	if (ret != pdPASS) {
		XR_OS_DBG("%s() fail @ %d\n", __func__, __LINE__);
		return XR_OS_FAIL;
	}

	return XR_OS_OK;
}

XR_OS_ThreadHandle_t XR_OS_MutexGetOwner(XR_OS_Mutex_t *mutex)
{
	if (!XR_OS_MutexIsValid(mutex)) {
		return XR_OS_INVALID_HANDLE;
	}

	return (XR_OS_ThreadHandle_t)xSemaphoreGetMutexHolder(mutex->handle);
}
