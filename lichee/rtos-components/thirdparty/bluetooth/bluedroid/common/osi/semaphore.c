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


#include "osi/semaphore.h"
#include <string.h>
#include "common/bt_trace.h"

#define OS_SemaphoreIsValid XR_OS_SemaphoreIsValid
#define OS_SemaphoreDelete XR_OS_SemaphoreDelete
#define OS_SemaphoreWait XR_OS_SemaphoreWait
#define OS_SemaphoreRelease XR_OS_SemaphoreRelease
#define OS_SemaphoreCreate XR_OS_SemaphoreCreate

/*-----------------------------------------------------------------------------------*/
//  Create and initialize a counting semaphore object. The "init_count" argument specifies
//  the initial state of the semaphore, "max_count" specifies the maximum value
//  that can be reached.
//  @return 0 on success, -1 on fail
int osi_sem_new(osi_sem_t *sem, uint32_t max_count, uint32_t init_count)
{
    assert(sem != NULL);

    return OS_SemaphoreCreate(sem, init_count, max_count);
}

/*-----------------------------------------------------------------------------------*/
// Give a semaphore
void osi_sem_give(osi_sem_t *sem)
{
    assert(sem != NULL);

    OS_SemaphoreRelease(sem);
}

/*
  Blocks the thread while waiting for the semaphore to be
  signaled. If the "timeout" argument is non-zero, the thread should
  only be blocked for the specified time (measured in
  milliseconds).
  @return 0 on success
*/
int osi_sem_take(osi_sem_t *sem, uint32_t timeout)
{
    assert(sem != NULL);

    return OS_SemaphoreWait(sem, timeout);
}

// Deallocates a semaphore
void osi_sem_free(osi_sem_t *sem)
{
    if (sem == NULL)
        return;

    OS_SemaphoreDelete(sem);

    memset(sem, 0, sizeof(osi_sem_t));
}

/** Check whether the semaphore object is valid or not.
 * @param sem Pointer to the semaphore object
 * @return 1 on valid, 0 on invalid
 */
int osi_sem_is_valid(osi_sem_t *sem)
{
    if (sem == NULL)
        return 0;

    return OS_SemaphoreIsValid(sem);
}
