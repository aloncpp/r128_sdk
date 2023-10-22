/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief      OS driver implementation.
 *
 *  Copyright (c) 2020-2021 Xradio, Inc.
 *
 *  Copyright (c) 2018-2019 Arm Ltd. All Rights Reserved.
 *
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

#include "pal_os.h"
#include "kernel/os/os.h"
#include "pm_task.h"

#ifdef BT_RESOURCE_TRACE
int g_bt_mutex_cnt = 0;
int g_bt_sem_cnt = 0;
int g_bt_thread_cnt = 0;
int g_bt_queue_cnt = 0;
int g_bt_timer_cnt = 0;
#endif

/**************************************************************************************************
  Functions: Initialization
**************************************************************************************************/
PalOsStatus_t PalOsThreadCreate(PalOsThread_t *thread, const char *name,
                                PalOsTheadEntry_t entry, void *argu,
                                PALOS_PRIORITY priority, uint32_t stackSize)
{
	XR_OS_Status status;
	XR_OS_Thread_t tmp_thread;
	XR_OS_Priority os_prio;

	switch (priority) {
	case PALOS_PRIO_LOW:
		os_prio = XR_OS_PRIORITY_LOW;
		break;
	case PALOS_PRIO_BELOW_NORMAL:
		os_prio = XR_OS_PRIORITY_BELOW_NORMAL;
		break;
	case PALOS_PRIO_NORMAL:
		os_prio = XR_OS_PRIORITY_NORMAL;
		break;
	case PALOS_PRIO_ABOVE_NORMAL:
		os_prio = XR_OS_PRIORITY_ABOVE_NORMAL;
		break;
	case PALOS_PRIO_HIGH:
		os_prio = XR_OS_PRIORITY_HIGH;
		break;
	case PALOS_PRIO_REAL_TIME:
		os_prio = XR_OS_PRIORITY_REAL_TIME;
		break;
	default:
		os_prio = PALOS_PRIO_NORMAL;
		break;
	}

	tmp_thread.handle = NULL;

	status = XR_OS_ThreadCreate(&tmp_thread,
	        name,
	        (XR_OS_ThreadEntry_t)entry,
	        argu,
	        os_prio,
	        stackSize);

#ifdef BT_RESOURCE_TRACE
	g_bt_thread_cnt++;
#endif

	if (status == XR_OS_OK) {
		thread->handle = tmp_thread.handle;
	}

	return status;
}

PalOsStatus_t PalOsThreadDelete(PalOsThread_t *thread)
{
	XR_OS_Status status;
	XR_OS_Thread_t tmp_thread;

	tmp_thread.handle = (XR_OS_ThreadHandle_t)(thread->handle);

	status = XR_OS_ThreadDelete(&tmp_thread);

#ifdef BT_RESOURCE_TRACE
	g_bt_thread_cnt--;
#endif

	return status;
}

PalOsStatus_t PalOsQueueCreate(PalOsQueue_t *queue, uint32_t queueLen, uint32_t itemSize)
{
	XR_OS_Status status;
	XR_OS_Queue_t tmp_queue;

	tmp_queue.handle = NULL;

	status = XR_OS_QueueCreate(&tmp_queue, queueLen, itemSize);

	if (status == XR_OS_OK) {
		queue->handle = tmp_queue.handle;
	}

#ifdef BT_RESOURCE_TRACE
	g_bt_queue_cnt++;
#endif

	return status;
}

PalOsStatus_t PalOsQueueDelete(PalOsQueue_t *queue)
{
	XR_OS_Status status;
	XR_OS_Queue_t tmp_queue;

	tmp_queue.handle = (QueueHandle_t)(queue->handle);

	status = XR_OS_QueueDelete(&tmp_queue);

#ifdef BT_RESOURCE_TRACE
	g_bt_queue_cnt--;
#endif

	return status;
}

__intr_text
PalOsStatus_t PalOsQueueSend(PalOsQueue_t *queue, const void *item, PalOsTime_t waitMS)
{
	XR_OS_Status status;
	XR_OS_Queue_t tmp_queue;

	tmp_queue.handle = (QueueHandle_t)(queue->handle);

	status = XR_OS_QueueSend(&tmp_queue, item, (XR_OS_Time_t)waitMS);

	return status;
}

PalOsStatus_t PalOsQueueReceive(PalOsQueue_t *queue, void *item, PalOsTime_t waitMS)
{
	XR_OS_Status status;
	XR_OS_Queue_t tmp_queue;

	tmp_queue.handle = (QueueHandle_t)(queue->handle);

	status = XR_OS_QueueReceive(&tmp_queue, item, (XR_OS_Time_t)waitMS);

	return status;
}

PalOsStatus_t PalOsSemaphoreCreate(PalOsSemaphore_t *sem, uint32_t initCount, uint32_t maxCount)
{
	XR_OS_Status status;
	XR_OS_Semaphore_t tmp_semp;

	tmp_semp.handle = NULL;

	status = XR_OS_SemaphoreCreate(&tmp_semp, initCount, maxCount);

	if (status == XR_OS_OK) {
		sem->handle = tmp_semp.handle;
	}

#ifdef BT_RESOURCE_TRACE
	g_bt_sem_cnt++;
#endif

	return status;
}

PalOsStatus_t PalOsSemaphoreCreateBinary(PalOsSemaphore_t *sem)
{
	XR_OS_Status status;
	XR_OS_Semaphore_t tmp_semp;

	tmp_semp.handle = NULL;

	status = XR_OS_SemaphoreCreateBinary(&tmp_semp);

	if (status == XR_OS_OK) {
		sem->handle = tmp_semp.handle;
	}

#ifdef BT_RESOURCE_TRACE
	g_bt_sem_cnt++;
#endif

	return status;
}

PalOsStatus_t PalOsSemaphoreDelete(PalOsSemaphore_t *sem)
{
	XR_OS_Status status;
	XR_OS_Semaphore_t tmp_semp;

	tmp_semp.handle = (SemaphoreHandle_t)(sem->handle);

	status = XR_OS_SemaphoreDelete(&tmp_semp);

#ifdef BT_RESOURCE_TRACE
	g_bt_sem_cnt--;
#endif

	return status;
}

PalOsStatus_t PalOsSemaphoreWait(PalOsSemaphore_t *sem, PalOsTime_t waitMS)
{
	XR_OS_Status status;
	XR_OS_Semaphore_t tmp_semp;

	tmp_semp.handle = (SemaphoreHandle_t)(sem->handle);

	status = XR_OS_SemaphoreWait(&tmp_semp, (XR_OS_Time_t)waitMS);

	return status;
}

__intr_text
PalOsStatus_t PalOsSemaphoreRelease(PalOsSemaphore_t *sem)
{
	XR_OS_Status status;
	XR_OS_Semaphore_t tmp_semp;

	tmp_semp.handle = (SemaphoreHandle_t)(sem->handle);

	status = XR_OS_SemaphoreRelease(&tmp_semp);
	if (status != XR_OS_OK) {
		/* 1.Walkaround about print too long with 115200 but bb need
		 * a queue send each 625us.
		 * 2.Walkaround deadlock about sem release failed need wsf_trace
		 * print error but wsf_trace need sem release. */
		printf("s\n");
	}

	return status;
}

PalOsStatus_t PalOsSemaphoreIsValid(PalOsSemaphore_t *sem)
{
	return (sem->handle != NULL);
}

void PalOsSemaphoreSetInvalid(PalOsSemaphore_t *sem)
{
	sem->handle = NULL;
}

PalOsStatus_t PalOsMutexCreate(PalOsMutex_t *mutex)
{
	XR_OS_Status status;
	XR_OS_Mutex_t tmp_mutex;

	tmp_mutex.handle = NULL;

	status = XR_OS_RecursiveMutexCreate(&tmp_mutex);

	if (status == XR_OS_OK) {
		mutex->handle = tmp_mutex.handle;
	}

#ifdef BT_RESOURCE_TRACE
	g_bt_mutex_cnt++;
#endif

	return status;
}

PalOsStatus_t PalOsMutexDelete(PalOsMutex_t *mutex)
{
	XR_OS_Status status;
	XR_OS_Mutex_t tmp_mutex;

	tmp_mutex.handle = (SemaphoreHandle_t)(mutex->handle);

	status = XR_OS_RecursiveMutexDelete(&tmp_mutex);

#ifdef BT_RESOURCE_TRACE
	g_bt_mutex_cnt--;
#endif

	return status;
}

__intr_text
PalOsStatus_t PalOsMutexLock(PalOsMutex_t *mutex, PalOsTime_t waitMS)
{
	XR_OS_Status status;
	XR_OS_Mutex_t tmp_mutex;

	tmp_mutex.handle = (SemaphoreHandle_t)(mutex->handle);

	status = XR_OS_RecursiveMutexLock(&tmp_mutex, (XR_OS_Time_t)waitMS);

	return status;
}

__intr_text
PalOsStatus_t PalOsMutexUnlock(PalOsMutex_t *mutex)
{
	XR_OS_Status status;
	XR_OS_Mutex_t tmp_mutex;

	tmp_mutex.handle = (SemaphoreHandle_t)(mutex->handle);

	status = XR_OS_RecursiveMutexUnlock(&tmp_mutex);

	return status;
}

PalOsStatus_t PalOsTimerCreate(PalOsTimer_t *timer, PalOsTimerType type,
              PalOsTimerCallback_t cb, void *arg, PalOsTime_t periodMS)
{
	XR_OS_Status status;
	XR_OS_Timer_t tmp_timer;

	tmp_timer.handle = NULL;

	status = XR_OS_TimerCreate(&tmp_timer, (XR_OS_TimerType)type, (XR_OS_TimerCallback_t)cb, arg, (XR_OS_Time_t)periodMS);

	if (status == XR_OS_OK) {
		timer->handle = tmp_timer.handle;
	}

#ifdef BT_RESOURCE_TRACE
	g_bt_timer_cnt++;
#endif

	return status;
}

PalOsStatus_t PalOsTimerDelete(PalOsTimer_t *timer)
{
	XR_OS_Status status;
	XR_OS_Timer_t tmp_timer;

	tmp_timer.handle = (TimerHandle_t)(timer->handle);

	status = XR_OS_TimerDelete(&tmp_timer);

#ifdef BT_RESOURCE_TRACE
	g_bt_timer_cnt--;
#endif

	return status;
}

__intr_text
PalOsStatus_t PalOsTimerStart(PalOsTimer_t *timer)
{
	XR_OS_Status status;
	XR_OS_Timer_t tmp_timer;

	tmp_timer.handle = (TimerHandle_t)(timer->handle);

	status = XR_OS_TimerStart(&tmp_timer);

	return status;
}

PalOsStatus_t PalOsTimerChangePeriod(PalOsTimer_t *timer, PalOsTime_t periodMS)
{
	XR_OS_Status status;
	XR_OS_Timer_t tmp_timer;

	tmp_timer.handle = (TimerHandle_t)(timer->handle);

	status = XR_OS_TimerChangePeriod(&tmp_timer, (XR_OS_Time_t)periodMS);

	return status;
}

PalOsStatus_t PalOsTimerStop(PalOsTimer_t *timer)
{
	XR_OS_Status status;
	XR_OS_Timer_t tmp_timer;

	tmp_timer.handle = (TimerHandle_t)(timer->handle);

	status = XR_OS_TimerStop(&tmp_timer);

	return status;
}

extern void srand (unsigned int seed);
extern int rand (void);
PalOsStatus_t PalOsRand(uint8_t *rnd, uint32_t size)
{
	PalOsStatus_t status = -1;
	uint32_t idx = 0;
	static uint32_t initSrand = 0;

	if ((rnd != NULL) && (size != 0)) {
		srand(initSrand);
		for (idx = 0; idx < size; idx++) {
			rnd[idx] = (uint8_t)(rand() & 0xFF);
		}
		initSrand = rand();

		status = 0;
	}

	return status;
}

#ifdef BT_RESOURCE_TRACE
/* mem review by sys mem print log review, we use PalOsMalloc/XHAL_Malloc/
 * XR_OS_Malloc/WsfBufAlloc/malloc api.
 */
void bt_resource_info(void)
{
	printf(("<<< bt resource info >>>\n"));
	printf("g_bt_mutex_cnt      %d\n", g_bt_mutex_cnt);
	printf("g_bt_sem_cnt        %d\n", g_bt_sem_cnt);
	printf("g_bt_thread_cnt     %d\n", g_bt_thread_cnt);
	printf("g_bt_queue_cnt      %d\n", g_bt_queue_cnt);
	printf("g_blec_timer_cnt   %d\n", g_bt_timer_cnt);
}

#endif

