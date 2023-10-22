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

#ifndef __SEMAPHORE_H__
#define __SEMAPHORE_H__

#include "kernel/os/os_semaphore.h"
#define OS_Semaphore_t XR_OS_Semaphore_t
#define OS_WAIT_FOREVER XR_OS_WAIT_FOREVER

#define OSI_SEM_MAX_TIMEOUT (uint32_t)OS_WAIT_FOREVER

typedef OS_Semaphore_t osi_sem_t;

int osi_sem_is_valid(osi_sem_t *sem);
//void osi_sem_set_invalid(osi_sem_t *sem);

int osi_sem_new(osi_sem_t *sem, uint32_t max_count, uint32_t init_count);

void osi_sem_free(osi_sem_t *sem);

int osi_sem_take(osi_sem_t *sem, uint32_t timeout);

void osi_sem_give(osi_sem_t *sem);

#endif /* __SEMAPHORE_H__ */
