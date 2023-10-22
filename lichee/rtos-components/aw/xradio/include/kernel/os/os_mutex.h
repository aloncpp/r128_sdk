/*
 * Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of XRADIO TECHNOLOGY CO., LTD. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _KERNEL_XR_OS_MUTEX_H_
#define _KERNEL_XR_OS_MUTEX_H_

#include "kernel/os/os_common.h"
#include "kernel/os/os_thread.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Mutex handle definition
 */
typedef XR_OS_Handle_t XR_OS_MutexHandle_t;

/**
 * @brief Mutex object definition
 */
typedef struct XR_OS_Mutex {
	XR_OS_MutexHandle_t handle;
} XR_OS_Mutex_t;

/**
 * @brief Create and initialize a mutex object
 * @note A mutex can only be locked by a single thread at any given time.
 * @param[in] mutex Pointer to the mutex object
 * @retval XR_OS_Status, XR_OS_OK on success
 */
XR_OS_Status XR_OS_MutexCreate(XR_OS_Mutex_t *mutex);

/**
 * @brief Delete the mutex object
 * @param[in] mutex Pointer to the mutex object
 * @retval XR_OS_Status, XR_OS_OK on success
 */
XR_OS_Status XR_OS_MutexDelete(XR_OS_Mutex_t *mutex);

/**
 * @brief Lock the mutex object
 * @note A mutex can only be locked by a single thread at any given time. If
 *       the mutex is already locked, the caller will be blocked for the
 *       specified time duration.
 * @param[in] mutex Pointer to the mutex object
 * @param[in] waitMS The maximum amount of time (in millisecond) the thread
 *                   should remain in the blocked state to wait for the mutex
 *                   to become unlocked.
 *                   XR_OS_WAIT_FOREVER for waiting forever, zero for no waiting.
 * @retval XR_OS_Status, XR_OS_OK on success
 */
XR_OS_Status XR_OS_MutexLock(XR_OS_Mutex_t *mutex, XR_OS_Time_t waitMS);

/**
 * @brief Unlock the mutex object previously locked using XR_OS_MutexLock()
 * @note The mutex should be unlocked from the same thread context from which
 *       it was locked.
 * @param[in] mutex Pointer to the mutex object
 * @retval XR_OS_Status, XR_OS_OK on success
 */
XR_OS_Status XR_OS_MutexUnlock(XR_OS_Mutex_t *mutex);

/**
 * @brief Create and initialize a recursive mutex object
 * @note A recursive mutex can be locked repeatedly by one single thread.
 *       The mutex doesn't become available again until the owner has called
 *       XR_OS_RecursiveMutexUnlock() for each successful XR_OS_RecursiveMutexLock().
 * @param[in] mutex Pointer to the recursive mutex object
 * @retval XR_OS_Status, XR_OS_OK on success
 */
XR_OS_Status XR_OS_RecursiveMutexCreate(XR_OS_Mutex_t *mutex);

/**
 * @brief Delete the recursive mutex object
 * @param[in] mutex Pointer to the recursive mutex object
 * @retval XR_OS_Status, XR_OS_OK on success
 */
XR_OS_Status XR_OS_RecursiveMutexDelete(XR_OS_Mutex_t *mutex);

/**
 * @brief Lock the recursive mutex object
 * @note A recursive mutex can be locked repeatedly by one single thread.
 *       If the recursive mutex is already locked by other thread, the caller
 *       will be blocked for the specified time duration.
 * @param[in] mutex Pointer to the recursive mutex object
 * @param[in] waitMS The maximum amount of time (in millisecond) the thread
 *                   should remain in the blocked state to wait for the
 *                   recursive mutex to become unlocked.
 *                   XR_OS_WAIT_FOREVER for waiting forever, zero for no waiting.
 * @retval XR_OS_Status, XR_OS_OK on success
 */
XR_OS_Status XR_OS_RecursiveMutexLock(XR_OS_Mutex_t *mutex, XR_OS_Time_t waitMS);

/**
 * @brief Unlock the recursive mutex object previously locked using
 *        XR_OS_RecursiveMutexLock()
 * @note The recursive mutex should be unlocked from the same thread context
 *       from which it was locked.
 * @param[in] mutex Pointer to the mutex object
 * @retval XR_OS_Status, XR_OS_OK on success
 */
XR_OS_Status XR_OS_RecursiveMutexUnlock(XR_OS_Mutex_t *mutex);

/**
 * @brief Get the owner of the mutex object
 * @param[in] mutex Pointer to the mutex object
 * @return Thread handle which is the owner of the mutex object
 */
XR_OS_ThreadHandle_t XR_OS_MutexGetOwner(XR_OS_Mutex_t *mutex);

/**
 * @brief Check whether the mutex object is valid or not
 * @param[in] mutex Pointer to the mutex object
 * @return 1 on valid, 0 on invalid
 */
static __always_inline int XR_OS_MutexIsValid(XR_OS_Mutex_t *mutex)
{
	return (mutex->handle != XR_OS_INVALID_HANDLE);
}

/**
 * @brief Set the mutex object to invalid state
 * @param[in] mutex Pointer to the mutex object
 * @return None
 */
static __always_inline void XR_OS_MutexSetInvalid(XR_OS_Mutex_t *mutex)
{
	mutex->handle = XR_OS_INVALID_HANDLE;
}

#ifdef __cplusplus
}
#endif

#endif /* _KERNEL_XR_OS_MUTEX_H_ */
