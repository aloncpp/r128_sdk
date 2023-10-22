/******************************************************************************
 *
 *  Copyright (C) 2015 Google, Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

#include "osi/mutex.h"
#include <string.h>
#include "common/bt_trace.h"

#define OS_MutexLock XR_OS_MutexLock
#define OS_MutexUnlock XR_OS_MutexUnlock
#define OS_MutexIsValid XR_OS_MutexIsValid
#define OS_RecursiveMutexUnlock XR_OS_RecursiveMutexUnlock
#define OS_RecursiveMutexLock XR_OS_RecursiveMutexLock
#define OS_RecursiveMutexCreate XR_OS_RecursiveMutexCreate
#define OS_MutexDelete XR_OS_MutexDelete
#define OS_MutexCreate XR_OS_MutexCreate

/* static section */
static osi_mutex_t gl_mutex; /* Recursive Type */


/** Create a new mutex
 * @param mutex pointer to the mutex to create
 * @return mutex status, 0 on success, -1 on fail*/
int osi_mutex_new(osi_mutex_t *mutex)
{
    assert(mutex != NULL);

    return OS_MutexCreate(mutex);
}

/** Lock a mutex
 * @param mutex the mutex to lock
 * @return 0 on success, -1 on fail*/
int osi_mutex_lock(osi_mutex_t *mutex, uint32_t timeout)
{
    assert(mutex != NULL);

    return OS_MutexLock(mutex, timeout);
}

/** Unlock a mutex
 * @param mutex the mutex to unlock */
void osi_mutex_unlock(osi_mutex_t *mutex)
{
    assert(mutex != NULL);

    OS_MutexUnlock(mutex);
}

/** Delete a semaphore
 * @param mutex the mutex to delete */
void osi_mutex_free(osi_mutex_t *mutex)
{
    if (mutex == NULL)
        return;

    OS_MutexDelete(mutex);

    memset(mutex, 0, sizeof(osi_mutex_t));
}

/* return 0 on success, -1 on fail */
int osi_mutex_global_init(void)
{
    memset(&gl_mutex, 0, sizeof(osi_mutex_t));

    return OS_RecursiveMutexCreate(&gl_mutex);
}

void osi_mutex_global_deinit(void)
{
    OS_MutexDelete(&gl_mutex);

    memset(&gl_mutex, 0, sizeof(osi_mutex_t));
}

void osi_mutex_global_lock(void)
{
    OS_RecursiveMutexLock(&gl_mutex, OSI_MUTEX_MAX_TIMEOUT);
}

void osi_mutex_global_unlock(void)
{
    OS_RecursiveMutexUnlock(&gl_mutex);
}

/** Check whether the mutex object is valid or not.
 * @param mutex Pointer to the mutex object
 * @return 1 on valid, 0 on invalid
 */
int osi_mutex_is_valid(osi_mutex_t *mutex)
{
    if (mutex == NULL)
        return 0;

    return OS_MutexIsValid(mutex);
}
