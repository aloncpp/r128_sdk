/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief      OS driver definition.
 *
 *  Copyright (c) 2020-2021 Xradio, Inc.
 *  Copyright (c) 2019-2020 Packetcraft, Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
/*************************************************************************************************/

#ifndef PAL_OS_H
#define PAL_OS_H

#include "pal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*! \addtogroup PAL_OS_H
 *  \{ */

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! \brief      PAL OS defien. */

typedef int32_t PalOsStatus_t;
typedef uint32_t PalOsTime_t;

typedef void *PalOsThreadHandle_t;
typedef struct PalOsThreadTag {
	PalOsThreadHandle_t handle;
} PalOsThread_t;

typedef void *PalOsQueueHandle_t;
typedef struct PalOsQueueTag {
	PalOsQueueHandle_t handle;
} PalOsQueue_t;

typedef void *PalOsSemaphoreHandle_t;
typedef struct PalOsSemaphoreTag {
	PalOsSemaphoreHandle_t handle;
} PalOsSemaphore_t;

typedef void *PalOsMutexHandle_t;
typedef struct PalOsMutexTag {
	PalOsMutexHandle_t handle;
} PalOsMutex_t;

typedef void *PalOsTimerHandle_t;
typedef struct PalOsTimerTag {
	PalOsTimerHandle_t handle;
} PalOsTimer_t;

typedef void (*PalOsTheadEntry_t)(void *);
typedef void (*PalOsTimerCallback_t)(void *);

typedef enum {
	PALOS_TIMER_ONCE       = 0, /* one shot timer */
	PALOS_TIMER_PERIODIC   = 1  /* periodic timer */
} PalOsTimerType;

typedef enum {
	PALOS_PRIO_LOW           = 1,
	PALOS_PRIO_BELOW_NORMAL  = 2,
	PALOS_PRIO_NORMAL        = 3,
	PALOS_PRIO_ABOVE_NORMAL  = 4,
	PALOS_PRIO_HIGH          = 5,
	PALOS_PRIO_REAL_TIME     = 6
} PALOS_PRIORITY;

#define PAL_OS_WAIT_FOREVER         0xffffffffU /* Wait forever timeout value */

#define PAL_OS_OK                   0

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

PalOsStatus_t PalOsThreadCreate(PalOsThread_t *thread, const char *name,
                          PalOsTheadEntry_t entry, void *argu, PALOS_PRIORITY priority, uint32_t stackSize);
PalOsStatus_t PalOsThreadDelete(PalOsThread_t *thread);

PalOsStatus_t PalOsQueueCreate(PalOsQueue_t *queue, uint32_t queueLen, uint32_t itemSize);
PalOsStatus_t PalOsQueueDelete(PalOsQueue_t *queue);
PalOsStatus_t PalOsQueueSend(PalOsQueue_t *queue, const void *item, PalOsTime_t waitMS);
PalOsStatus_t PalOsQueueReceive(PalOsQueue_t *queue, void *item, PalOsTime_t waitMS);

PalOsStatus_t PalOsSemaphoreCreate(PalOsSemaphore_t *sem, uint32_t initCount, uint32_t maxCount);
PalOsStatus_t PalOsSemaphoreCreateBinary(PalOsSemaphore_t *sem);
PalOsStatus_t PalOsSemaphoreDelete(PalOsSemaphore_t *sem);
PalOsStatus_t PalOsSemaphoreWait(PalOsSemaphore_t *sem, PalOsTime_t waitMS);
PalOsStatus_t PalOsSemaphoreRelease(PalOsSemaphore_t *sem);
PalOsStatus_t PalOsSemaphoreIsValid(PalOsSemaphore_t *sem);
void PalOsSemaphoreSetInvalid(PalOsSemaphore_t *sem);

PalOsStatus_t PalOsMutexCreate(PalOsMutex_t *mutex);
PalOsStatus_t PalOsMutexDelete(PalOsMutex_t *mutex);
PalOsStatus_t PalOsMutexLock(PalOsMutex_t *mutex, PalOsTime_t waitMS);
PalOsStatus_t PalOsMutexUnlock(PalOsMutex_t *mutex);

PalOsStatus_t PalOsTimerCreate(PalOsTimer_t *timer, PalOsTimerType type,
                         PalOsTimerCallback_t cb, void *arg, PalOsTime_t periodMS);
PalOsStatus_t PalOsTimerDelete(PalOsTimer_t *timer);
PalOsStatus_t PalOsTimerStart(PalOsTimer_t *timer);
PalOsStatus_t PalOsTimerChangePeriod(PalOsTimer_t *timer, PalOsTime_t periodMS);
PalOsStatus_t PalOsTimerStop(PalOsTimer_t *timer);

PalOsStatus_t PalOsRand(uint8_t *rnd, uint32_t size);

#define PalOsMalloc(size)                   malloc(size)

#define PalOsFree(ptr)                      free(ptr)

#ifdef BT_RESOURCE_TRACE
void bt_resource_info(void);
#endif

/*! \} */    /* PAL_OS_H */

#ifdef __cplusplus
};
#endif

#endif /* PAL_OS_H */

