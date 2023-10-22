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

#ifndef __MUTEX_H__
#define __MUTEX_H__

#include "kernel/os/os_mutex.h"
#define OS_Mutex_t XR_OS_Mutex_t
#define OS_WAIT_FOREVER XR_OS_WAIT_FOREVER

#define OSI_MUTEX_MAX_TIMEOUT (uint32_t)OS_WAIT_FOREVER

typedef OS_Mutex_t osi_mutex_t;

int osi_mutex_is_valid(osi_mutex_t *mutex);
//#define osi_mutex_set_invalid( x ) ( ( *x ) = NULL )

int osi_mutex_new(osi_mutex_t *mutex);

int osi_mutex_lock(osi_mutex_t *mutex, uint32_t timeout);

void osi_mutex_unlock(osi_mutex_t *mutex);

void osi_mutex_free(osi_mutex_t *mutex);

/* Just for a global mutex */
int osi_mutex_global_init(void);

void osi_mutex_global_deinit(void);

void osi_mutex_global_lock(void);

void osi_mutex_global_unlock(void);

#endif /* __MUTEX_H__ */

