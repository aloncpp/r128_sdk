/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

/* lwIP includes. */

#include <pthread.h>
#include "lwip/debug.h"
#include "lwip/def.h"
#include "lwip/sys.h"
#include "lwip/mem.h"
#include "arch/sys_arch.h"
#include "lwip/stats.h"
#include <stdlib.h>

/* This is the number of threads that can be started with sys_thread_new() */
#define SYS_THREAD_MAX 4
#define TAG "lwip_arch"

static sys_mutex_t g_lwip_protect_mutex = NULL;

#if 0
void hex_dump(char *pref, int width, unsigned char *buf, int len)
{
       int i,n;
    for (i = 0, n = 1; i < len; i++, n++){
        if (n == 1)
            printf("%s ", pref);
        printf("%2.2X ", buf[i]);
        if (n == width) {
            printf("\n");
            n = 0;
        }
    }
    if (i && n!=1)
        printf("\n");
}
#endif

/* static pthread_key_t sys_thread_sem_key;*/
static void sys_thread_sem_free(void* data);

#if !LWIP_COMPAT_MUTEX
/** Create a new mutex
 * @param mutex pointer to the mutex to create
 * @return a new mutex */
err_t
sys_mutex_new(sys_mutex_t *pxMutex)
{
  err_t xReturn = ERR_MEM;

  *pxMutex = xSemaphoreCreateMutex();

  if (*pxMutex != NULL) {
    xReturn = ERR_OK;
  }

  /*LWIP_DEBUGF(AW_LWIP_DEBUG, ("sys_mutex_new: m=%p\n", *pxMutex));*/

  return xReturn;
}

/** Lock a mutex
 * @param mutex the mutex to lock */
void sys_mutex_lock(sys_mutex_t *pxMutex)
{
  while (xSemaphoreTake(*pxMutex, portMAX_DELAY) != pdPASS);
}

err_t sys_mutex_trylock(sys_mutex_t *pxMutex)
{
  if (xSemaphoreTake(*pxMutex, 0) == pdPASS) return 0;
  else return -1;
}

/** Unlock a mutex
 * @param mutex the mutex to unlock */
void sys_mutex_unlock(sys_mutex_t *pxMutex)
{
  xSemaphoreGive(*pxMutex);
}

/** Delete a semaphore
 * @param mutex the mutex to delete */
void sys_mutex_free(sys_mutex_t *pxMutex)
{
  LWIP_DEBUGF(AW_LWIP_DEBUG, ("sys_mutex_free: m=%p\n", *pxMutex));
  vQueueDelete(*pxMutex);
}
#endif

/*-----------------------------------------------------------------------------------*/
//  Creates and returns a new semaphore. The "count" argument specifies
//  the initial state of the semaphore. TBD finish and test
err_t sys_sem_new(sys_sem_t *sem, u8_t count)
{
  err_t xReturn = ERR_MEM;
  vSemaphoreCreateBinary(*sem);

  if ((*sem) != NULL) {
    if (count == 0) {	// Means it can't be taken
      xSemaphoreTake(*sem, 1);
    }

    xReturn = ERR_OK;
  } else {
    ;	// TBD need assert
  }

  return xReturn;
}

/*-----------------------------------------------------------------------------------*/
// Signals a semaphore
void sys_sem_signal(sys_sem_t *sem)
{
    xSemaphoreGive(*sem);
}

/*-----------------------------------------------------------------------------------*/
// Signals a semaphore (from ISR)
int sys_sem_signal_isr(sys_sem_t *sem)
{
    BaseType_t woken = pdFALSE;
    BaseType_t ret;
    ret = xSemaphoreGiveFromISR(*sem, &woken);
    if (ret == pdPASS) {
        portYIELD_FROM_ISR(woken);
    }
    return woken == pdTRUE;
}

/*-----------------------------------------------------------------------------------*/
/*
  Blocks the thread while waiting for the semaphore to be
  signaled. If the "timeout" argument is non-zero, the thread should
  only be blocked for the specified time (measured in
  milliseconds).

  If the timeout argument is non-zero, the return value is the number of
  milliseconds spent waiting for the semaphore to be signaled. If the
  semaphore wasn't signaled within the specified time, the return value is
  SYS_ARCH_TIMEOUT. If the thread didn't have to wait for the semaphore
  (i.e., it was already signaled), the function may return zero.

  Notice that lwIP implements a function with a similar name,
  sys_sem_wait(), that uses the sys_arch_sem_wait() function.
*/
u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout)
{
  portTickType StartTime, EndTime, Elapsed;
  unsigned long ulReturn;

  StartTime = xTaskGetTickCount();

  if (timeout != 0) {
    if (xSemaphoreTake(*sem, timeout / portTICK_PERIOD_MS) == pdTRUE) {
      EndTime = xTaskGetTickCount();
      Elapsed = (EndTime - StartTime) * portTICK_PERIOD_MS;

      if (Elapsed == 0) {
        Elapsed = 1;
      }

      ulReturn = Elapsed;
    } else {
      ulReturn = SYS_ARCH_TIMEOUT;
    }
  } else { // must block without a timeout
    while (xSemaphoreTake(*sem, portMAX_DELAY) != pdTRUE);

    EndTime = xTaskGetTickCount();
    Elapsed = (EndTime - StartTime) * portTICK_PERIOD_MS;

    if (Elapsed == 0) {
      Elapsed = 1;
    }

    ulReturn = Elapsed;
  }

  return ulReturn ; // return time blocked
}

/*-----------------------------------------------------------------------------------*/
// Deallocates a semaphore
void
sys_sem_free(sys_sem_t *sem)
{
  vSemaphoreDelete(*sem);
}

/*-----------------------------------------------------------------------------------*/
//  Creates an empty mailbox.
err_t
sys_mbox_new(sys_mbox_t *mbox, int size)
{
#if PTR_SYS_MBOX
  *mbox = malloc(sizeof(struct sys_mbox_s));
  if (*mbox == NULL){
    LWIP_DEBUGF(AW_LWIP_DEBUG, ("fail to new *mbox\n"));
    return ERR_MEM;
  }

  (*mbox)->os_mbox = xQueueCreate(size, sizeof(void *));

  if ((*mbox)->os_mbox == NULL) {
    LWIP_DEBUGF(AW_LWIP_DEBUG, ("fail to new *mbox->os_mbox\n"));
    free(*mbox);
    return ERR_MEM;
  }

  /*LWIP_DEBUGF(AW_LWIP_DEBUG, ("new *mbox ok mbox=%p os_mbox=%p\n", *mbox, (*mbox)->os_mbox));*/
  return ERR_OK;
#else
	err_t xReturn = ERR_MEM;
	*mbox = xQueueCreate(size, sizeof(void *));
	if(*mbox != NULL) {
		xReturn = ERR_OK;
#if SYS_STATS
		SYS_STATS_INC_USED( mbox );
#endif
	}
	return xReturn;
#endif
}

/*-----------------------------------------------------------------------------------*/
//   Posts the "msg" to the mailbox.
void sys_mbox_post(sys_mbox_t *mbox, void *msg)
{
#if PTR_SYS_MBOX
  while (xQueueSendToBack((*mbox)->os_mbox, &msg, portMAX_DELAY) != pdTRUE);
#else
  while (xQueueSendToBack(*mbox, &msg, portMAX_DELAY) != pdTRUE);
#endif
}

extern volatile uint32_t ulPortInterruptNesting[];
extern int cur_cpu_id(void);

/*-----------------------------------------------------------------------------------*/
err_t sys_mbox_trypost(sys_mbox_t *mbox, void *msg)
{
  err_t xReturn;
#if PTR_SYS_MBOX
  if (xQueueSend((*mbox)->os_mbox, &msg, (portTickType)0) == pdPASS) {
    xReturn = ERR_OK;
  } else {
    LWIP_DEBUGF(AW_LWIP_DEBUG, ("trypost mbox=%p fail\n", (*mbox)->os_mbox));
    xReturn = ERR_MEM;
  }
#else
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
	if(ulPortInterruptNesting[cur_cpu_id()] != 0) {
		xReturn = xQueueSendFromISR(*mbox,&msg,&xHigherPriorityTaskWoken);
		if (xReturn == pdPASS) {
			portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
		}
	}else {
		xReturn = xQueueSend(*mbox,&msg,(TickType_t)0);
	}
	if(xReturn == pdPASS) {
		xReturn = ERR_OK;
	}else {
		xReturn = ERR_MEM;
		SYS_STATS_INC(mbox.err);
	}
#endif
  return xReturn;
}

/*-----------------------------------------------------------------------------------*/
/*
  Blocks the thread until a message arrives in the mailbox, but does
  not block the thread longer than "timeout" milliseconds (similar to
  the sys_arch_sem_wait() function). The "msg" argument is a result
  parameter that is set by the function (i.e., by doing "*msg =
  ptr"). The "msg" parameter maybe NULL to indicate that the message
  should be dropped.

  The return values are the same as for the sys_arch_sem_wait() function:
  Number of milliseconds spent waiting or SYS_ARCH_TIMEOUT if there was a
  timeout.

  Note that a function with a similar name, sys_mbox_fetch(), is
  implemented by lwIP.
*/
u32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout)
{
  void *dummyptr;
  portTickType StartTime, EndTime, Elapsed;
  unsigned long ulReturn;

  StartTime = xTaskGetTickCount();
  if (msg == NULL) {
    msg = &dummyptr;
  }

  if (*mbox == NULL){
    *msg = NULL;
    return -1;
  }

  if (timeout == 0) {
    timeout = portMAX_DELAY;
  } else {
    timeout = timeout / portTICK_PERIOD_MS;
  }

  *msg = NULL;
#if PTR_SYS_MBOX
  if (pdTRUE == xQueueReceive((*mbox)->os_mbox, &(*msg), timeout)) {
#else
  if (pdTRUE == xQueueReceive(*mbox, &(*msg), timeout)) {
#endif
    EndTime = xTaskGetTickCount();
    Elapsed = (EndTime - StartTime) * portTICK_PERIOD_MS;

    if (Elapsed == 0) {
      Elapsed = 1;
    }

    ulReturn = Elapsed;
  } else { // timed out blocking for message
    ulReturn = SYS_ARCH_TIMEOUT;
  }

  return ulReturn ; // return time blocked TBD test
}

/*-----------------------------------------------------------------------------------*/
u32_t
sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg)
{
  void *pvDummy;
  unsigned long ulReturn;

  if (msg == NULL) {
    msg = &pvDummy;
  }
#if PTR_SYS_MBOX
  if (pdTRUE == xQueueReceive((*mbox)->os_mbox, &(*msg), 0)) {
    ulReturn = ERR_OK;
  } else {
    ulReturn = SYS_MBOX_EMPTY;
  }
#else
	err_t xReturn;
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
	if(ulPortInterruptNesting[cur_cpu_id()] != 0) {
		xReturn = xQueueReceiveFromISR(*mbox,&(*msg),&xHigherPriorityTaskWoken);
		if (xReturn == pdPASS) {
			portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
		}
	}else {
		xReturn = xQueueReceive(*mbox, &(*msg),0UL);
	}
	if(xReturn == pdPASS) {
		ulReturn = ERR_OK;
	}else{
		ulReturn = SYS_MBOX_EMPTY;
	}
#endif
  return ulReturn;
}

void
sys_mbox_free(sys_mbox_t *mbox)
{
  uint32_t mbox_message_num = 0;

  if ( (NULL == mbox) || (NULL == *mbox) ) {
      return;
  }
#if PTR_SYS_MBOX
  mbox_message_num = uxQueueMessagesWaiting((*mbox)->os_mbox);

  LWIP_DEBUGF(AW_LWIP_DEBUG, ("mbox free: mbox=%p os_mbox=%p owner=%p msg_num=%d\n",
              *mbox, (*mbox)->os_mbox, (*mbox)->owner, mbox_message_num));

  vQueueDelete((*mbox)->os_mbox);
  free(*mbox);
  *mbox = NULL;
#else
	mbox_message_num = uxQueueMessagesWaiting(*mbox);
#if SYS_STATS
	if(mbox_message_num != 0UL) {
		SYS_STATS_INC(mbox.err);
	}
	SYS_STATS_DEC(mbox.used);
#endif
	vQueueDelete(*mbox);
	*mbox = NULL;
#endif
}

/*-----------------------------------------------------------------------------------*/
/*
  Starts a new thread with priority "prio" that will begin its execution in the
  function "thread()". The "arg" argument will be passed as an argument to the
  thread() function. The id of the new thread is returned. Both the id and
  the priority are system dependent.
*/
static xTaskHandle created_task;
sys_thread_t
sys_thread_new(const char *name, lwip_thread_fn thread, void *arg, int stacksize, int prio)
{
#if LWIP_FREERTOS_ORIG
  portBASE_TYPE result;

  result = xTaskCreate(thread, name, (stacksize / sizeof(StackType_t)), arg, prio, &created_task);

  if (result != pdPASS) {
    return NULL;
  }

  return created_task;
#else
   portBASE_TYPE ret;
   ret = xTaskCreateOnCore(thread, name, (stacksize / sizeof(StackType_t)), arg,
		   prio, &created_task, 1);
   if(ret != pdPASS) {
		return NULL;
	}
   return created_task;
#endif
}

/*-----------------------------------------------------------------------------------*/
// Initialize sys arch
void
sys_init(void)
{
    if (ERR_OK != sys_mutex_new(&g_lwip_protect_mutex)) {
        LWIP_ERROR("sys_init: failed to init lwip protect mutex",0, while(1));
    }
#if 0
    // Create the pthreads key for the per-thread semaphore storage
    pthread_key_create(&sys_thread_sem_key, sys_thread_sem_free);

    esp_vfs_lwip_sockets_register();
#endif
}
void sys_deinit(void)
{
	sys_mutex_free(&g_lwip_protect_mutex);
}
/*-----------------------------------------------------------------------------------*/
u32_t
sys_jiffies(void)
{
  return xTaskGetTickCount();
}

/*-----------------------------------------------------------------------------------*/
u32_t
sys_now(void)
{
  return (xTaskGetTickCount()*portTICK_PERIOD_MS);
}
/*
  This optional function does a "fast" critical region protection and returns
  the previous protection level. This function is only called during very short
  critical regions. An embedded system which supports ISR-based drivers might
  want to implement this function by disabling interrupts. Task-based systems
  might want to implement this by using a mutex or disabling tasking. This
  function should support recursive calls from the same task or interrupt. In
  other words, sys_arch_protect() could be called while already protected. In
  that case the return value indicates that it is already protected.

  sys_arch_protect() is only required if your port is supporting an operating
  system.
*/
sys_prot_t
sys_arch_protect(void)
{
  sys_mutex_lock(&g_lwip_protect_mutex);
  return (sys_prot_t) 1;
}

/*-----------------------------------------------------------------------------------*/
/*
  This optional function does a "fast" set of critical region protection to the
  value specified by pval. See the documentation for sys_arch_protect() for
  more information. This function is only required if your port is supporting
  an operating system.
*/
void
sys_arch_unprotect(sys_prot_t pval)
{
  sys_mutex_unlock(&g_lwip_protect_mutex);
}

#if 0
/*
 * get per thread semphore
 */
sys_sem_t* sys_thread_sem_get(void)
{
  sys_sem_t *sem = pthread_getspecific(sys_thread_sem_key);

  if (!sem) {
      sem = sys_thread_sem_init();
  }
  LWIP_DEBUGF(AW_LWIP_DEBUG, ("sem_get s=%p\n", sem));
  return sem;
}

static void sys_thread_sem_free(void* data) // destructor for TLS semaphore
{
  sys_sem_t *sem = (sys_sem_t*)(data);

  if (sem && *sem){
    LWIP_DEBUGF(AW_LWIP_DEBUG, ("sem del, sem=%p\n", *sem));
    vSemaphoreDelete(*sem);
  }

  if (sem) {
    LWIP_DEBUGF(AW_LWIP_DEBUG, ("sem pointer del, sem_p=%p\n", sem));
    free(sem);
  }
}

sys_sem_t* sys_thread_sem_init(void)
{
  sys_sem_t *sem = (sys_sem_t*)mem_malloc(sizeof(sys_sem_t*));

  if (!sem){
    ESP_LOGE(TAG, "thread_sem_init: out of memory");
    return 0;
  }

  *sem = xSemaphoreCreateBinary();
  if (!(*sem)){
    free(sem);
    ESP_LOGE(TAG, "thread_sem_init: out of memory");
    return 0;
  }

  pthread_setspecific(sys_thread_sem_key, sem);
  return sem;
}

void sys_thread_sem_deinit(void)
{
  sys_sem_t *sem = pthread_getspecific(sys_thread_sem_key);
  if (sem != NULL) {
    sys_thread_sem_free(sem);
    pthread_setspecific(sys_thread_sem_key, NULL);
  }
}

void sys_delay_ms(uint32_t ms)
{
  vTaskDelay(ms / portTICK_PERIOD_MS);
}
#endif

