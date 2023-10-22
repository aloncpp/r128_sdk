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
#ifndef __OMX_BASE_H
#define __OMX_BASE_H

#include <assert.h>
#include <errno.h>

#include <hal_mutex.h>
#include <FreeRTOS.h>
#include <OmxCore.h>

#include "FreeRTOSConfig.h"
#include "portmacro.h"


#define OMX_IL_VERSION "OMX-IL-V0.0.7"

//#define RB_DEBUG

#ifndef TAG
#define TAG	"OMX"
#endif

#ifndef unlikely
#define unlikely(x)             __builtin_expect ((x), 0)
#endif

#define LOG_COLOR_NONE		"\e[0m"
#define LOG_COLOR_GREEN		"\e[32m"
#define LOG_COLOR_BLUE		"\e[34m"
#define LOG_COLOR_RED		"\e[31m"
#define OSCL_IMPORT_REF
#define OSCL_EXPORT_REF

#if 1
#define omx_alloc(size)		calloc(1, size)
#define omx_free(ptr)		free(ptr)
#else
static inline void *as_alloc(size)
{
	printf("alloc %u bytes\n", size);
	return calloc(1, size);
}

static inline void as_free(void *ptr)
{
	free(ptr);
}
#endif

#if defined(CONFIG_ARCH_ARM_CORTEX_A7)
#define OMX_CORE "A7-"
#elif defined(CONFIG_ARCH_ARM_CORTEX_M33)
#define OMX_CORE "M33-"
#elif defined(CONFIG_ARCH_RISCV)
#define OMX_CORE "RV-"
#elif defined(CONFIG_ARCH_DSP)
#define OMX_CORE "DSP-"
#else
#define OMX_CORE ""
#endif

extern int g_omx_debug_mask;
#define omx_debug(fmt, args...) \
do { \
	if (unlikely(g_omx_debug_mask)) \
		printf(LOG_COLOR_GREEN "[%s%s-DBG][%s](%d) " fmt "\n" \
			LOG_COLOR_NONE, OMX_CORE, TAG, __func__, __LINE__, ##args); \
} while (0)


#define omx_info(fmt, args...)	\
    printf(LOG_COLOR_BLUE "[%s%s-INF][%s](%d) " fmt "\n" LOG_COLOR_NONE, \
			OMX_CORE, TAG, __func__, __LINE__, ##args)

#define omx_err(fmt, args...)	\
    printf(LOG_COLOR_RED "[%s%s-ERR][%s](%d) " fmt "\n" LOG_COLOR_NONE, \
			OMX_CORE, TAG, __func__, __LINE__, ##args)

#if 1
#define fatal(msg) \
do {\
	printf("[%s%s-FATAL][%s](%d) %s \n", OMX_CORE, TAG, __func__, __LINE__,msg);\
	assert(0);\
} while (0)
#else
#define fatal(msg) \
	assert(0);
#endif


#ifndef configAPPLICATION_OMX_PRIORITY
#define configAPPLICATION_OMX_PRIORITY	(configMAX_PRIORITIES > 20 ? configMAX_PRIORITIES - 8 : configMAX_PRIORITIES - 3)
#endif
#define configAPPLICATION_OMX_HIGH_PRIORITY	(configAPPLICATION_OMX_PRIORITY + 2)

#define OMX_MAYBE_UNUSED(v) 	(void)(v)

/* for omx thread */
#define SCHED_FIFO        0 /**< First in-first out (FIFO) scheduling policy. */
#define SCHED_RR          1 /**< Round robin scheduling policy. */
#define SCHED_SPORADIC    2 /**< Sporadic server scheduling policy. */
#define SCHED_OTHER       3 /**< Another scheduling policy. */
/**@} */

/**
 * @brief Scheduling parameters required for implementation of each supported
 * scheduling policy.
 */
struct omx_sched_param
{
    int sched_priority; /**< Process or thread execution scheduling priority. */
};

#define omx_threadDETACH_STATE_MASK 0x8000
#define omx_threadSCHED_PRIORITY_MASK 0x7FFF
#define omx_threadDETACH_STATE_SHIFT 15
#define omx_threadGET_SCHED_PRIORITY( var ) ( (var)  & (omx_threadSCHED_PRIORITY_MASK) )
#define omx_threadIS_JOINABLE( var ) ( ( (var) & (omx_threadDETACH_STATE_MASK) ) == omx_threadDETACH_STATE_MASK )
#define OMXTHREAD_CREATE_DETACHED    0       /**< Detached. */
#define OMXTHREAD_CREATE_JOINABLE    1       /**< Joinable (default). */

#define OMX_THREAD_STACK_MIN	 configMINIMAL_STACK_SIZE * sizeof(StackType_t) /**< Minimum size in bytes of thread stack storage. */
#define Config_OMXTHREAD_TASK_NAME    "omxthread"

typedef struct omx_thread_attr {
	uint32_t ulpthreadAttrStorage;
} OMXthreadAttrType_t;

typedef OMXthreadAttrType_t omx_thread_attr_t;

typedef void * omx_thread_t;

typedef struct omx_thread_attr_internal
{
    uint16_t usStackSize;                 /**< Stack size. */
    uint16_t usSchedPriorityDetachState;  /**< Schedule priority 15 bits (LSB) Detach state: 1 bits (MSB) */
} omx_thread_attr_internal_t;

typedef struct omx_thread_internal
{
    omx_thread_attr_internal_t xAttr;        /**< Thread attributes. */
    void * ( *pvStartRoutine )( void * ); /**< Application thread function. */
    void * xTaskArg;                      /**< Arguments for application thread function. */
    TaskHandle_t xTaskHandle;             /**< FreeRTOS task handle. */
    StaticSemaphore_t xJoinBarrier;       /**< Synchronizes the two callers of pthread_join. */
    StaticSemaphore_t xJoinMutex;         /**< Ensures that only one other thread may join this thread. */
    void * xReturn;                       /**< Return value of pvStartRoutine. */
} omx_thread_internal_t;

int omx_thread_attr_init(omx_thread_attr_t * attr);

int omx_thread_attr_setdetachstate( omx_thread_attr_t * attr,
                                 int detachstate );

int omx_thread_attr_setschedparam( omx_thread_attr_t * attr,
                                const struct omx_sched_param * param );

int omx_thread_attr_setstacksize( omx_thread_attr_t * attr,
                               int stacksize );

int omx_thread_setschedparam( omx_thread_t thread,
			                       int policy,
			                       const struct omx_sched_param * param );

int omx_thread_setname_np(omx_thread_t thread, const char * name);

int omx_thread_getname_np(omx_thread_t thread, char * name, int len);


int omx_thread_create(omx_thread_t * thread,
                    const omx_thread_attr_t * attr,
                    void *( *startroutine )( void * ),
                    void * arg);

int omx_thread_equal(omx_thread_t t1,
                   omx_thread_t t2 );

int omx_thread_join(omx_thread_t pthread,
                  void ** retval);

omx_thread_t omx_thread_self(void);


/* for omx thread mutex lock */
#ifndef OMX_THREAD_MUTEX_NORMAL
    #define OMX_THREAD_MUTEX_NORMAL        0                        /**< Non-robust, deadlock on relock, does not remember owner. */
#endif
#ifndef OMX_THREAD_MUTEX_ERRORCHECK
    #define OMX_THREAD_MUTEX_ERRORCHECK    1                        /**< Non-robust, error on relock,  remembers owner. */
#endif
#ifndef OMX_THREAD_MUTEX_RECURSIVE
    #define OMX_THREAD_MUTEX_RECURSIVE     2                        /**< Non-robust, recursive relock, remembers owner. */
#endif
#ifndef OMX_THREAD_MUTEX_DEFAULT
    #define OMX_THREAD_MUTEX_DEFAULT       OMX_THREAD_MUTEX_NORMAL     /**< PTHREAD_MUTEX_NORMAL (default). */
#endif

typedef struct omx_thread_mutexattr_internal
{
	int iType; /**< Mutex type. */
} omx_thread_mutexattr_internal_t;

typedef struct omx_thread_mutexattr {
	uint32_t		ulpthreadMutexAttrStorage;
} omx_threadMutexAttrType_t;

typedef omx_threadMutexAttrType_t  omx_thread_mutexattr_t;

typedef struct omx_thread_mutex_internal
{
	BaseType_t xIsInitialized;			/**< Set to pdTRUE if this mutex is initialized, pdFALSE otherwise. */
	StaticSemaphore_t xMutex;			/**< FreeRTOS mutex. */
	TaskHandle_t xTaskOwner;			/**< Owner; used for deadlock detection and permission checks. */
	omx_thread_mutexattr_internal_t xAttr; /**< Mutex attributes. */
} omx_thread_mutex_internal_t;

#define OMX_MUTEX_INITIALIZER \
	( ( ( omx_thread_mutex_internal_t )	 \
	{									 \
		.xIsInitialized = pdFALSE,		 \
		.xMutex = { { 0 } },			 \
		.xTaskOwner = NULL, 			 \
		.xAttr = { .iType = 0 } 		 \
	}									 \
	  ) 								 \
	)

typedef omx_thread_mutex_internal_t  omx_thread_mutex_t;
int omx_thread_mutex_init( omx_thread_mutex_t * mutex,
                        const omx_thread_mutexattr_t * attr );

int omx_thread_mutex_destroy( omx_thread_mutex_t * mutex );
int omx_thread_mutex_lock( omx_thread_mutex_t * mutex );

int omx_thread_mutex_timedlock( omx_thread_mutex_t * mutex,
                             long milliSecondsDelay );
int omx_thread_mutex_unlock( omx_thread_mutex_t * mutex );



/* for omx thread cond mutex lock */
typedef hal_mutex_t omx_mutex_t;
omx_mutex_t omx_mutex_init(void);
int omx_mutex_lock_timeout(omx_mutex_t mutex, long ms);
int omx_mutex_lock(omx_mutex_t mutex);
int omx_mutex_unlock(omx_mutex_t mutex);
void omx_mutex_destroy(omx_mutex_t mutex);

typedef struct omx_thread_cond_internal {
	BaseType_t xIsInitialized;            /**< Set to pdTRUE if this condition variable is initialized, pdFALSE otherwise. */
	StaticSemaphore_t xCondMutex;         /**< Prevents concurrent accesses to iWaitingThreads. */
	StaticSemaphore_t xCondWaitSemaphore; /**< Threads block on this semaphore in pthread_cond_wait. */
	int iWaitingThreads;                  /**< The number of threads currently waiting on this condition variable. */
} omx_thread_cond_internal_t;

typedef  omx_thread_cond_internal_t  omx_thread_cond_t;

/** @@ Modified code
 * added some queue functions for deinterlace case.
 */

/** Maximum number of elements in a queue
 */
#define MAX_QUEUE_ELEMENTS 32
/** Output port queue element. Contains an OMX buffer header type
 */
 
typedef struct qelem_t qelem_t;
struct qelem_t{
  qelem_t* q_forw;
  void* data;
};

/** This structure contains the queue
 */
typedef struct queue_t{
  qelem_t* first; /**< Output buffer queue head */
  qelem_t* last; /**< Output buffer queue tail */
  int nelem; /**< Number of elements in the queue */
  omx_mutex_t mutex;
} queue_t;

/** Initialize a queue descriptor
 *
 * @param queue The queue descriptor to initialize.
 * The user needs to allocate the queue
 *
 * @return -1 if the resources are not enough and the allocation cannot be performed
 */
int queue_init(queue_t* queue);

/** Deinitialize a queue descriptor
 * flushing all of its internal data
 *
 * @param queue the queue descriptor to dump
 */
void queue_deinit(queue_t* queue);

/** Enqueue an element to the given queue descriptor
 *
 * @param queue the queue descritpor where to queue data
 *
 * @param data the data to be enqueued
 *
 * @return -1 if the queue is full
 */
int queue(queue_t* queue, void* data);

/** Dequeue an element from the given queue descriptor
 *
 * @param queue the queue descriptor from which to dequeue the element
 *
 * @return the element that has bee dequeued. If the queue is empty
 *  a NULL value is returned
 */
void* dequeue(queue_t* queue);

/** Returns the number of elements hold in the queue
 *
 * @param queue the requested queue
 *
 * @return the number of elements in the queue
 */
int getquenelem(queue_t* queue);

/** peek queue*/
int peekqueue(queue_t* queue, void** data);

/** erase one elem from queue*/
int queue_erase(queue_t* queue, void* data);

/** queue one elem as first elem*/
int queue_first(queue_t* queue, void* data);

/** @@ Modified code
 * fixed a bug in omx_sem_timed_down function.
 * added function for deinterlace case.
 **/

/** The structure contains the semaphore value, mutex and green light flag
 */
typedef struct omx_sem_t{
  omx_thread_cond_t condition;
  omx_mutex_t mutex;
  unsigned int semval;
}omx_sem_t;

/** Initializes the semaphore at a given value
 *
 * @param omx_sem the semaphore to initialize
 *
 * @param val the initial value of the semaphore
 */
OSCL_IMPORT_REF int omx_sem_init(omx_sem_t* omx_sem, unsigned int val);

/** Destroy the semaphore
 *
 * @param omx_sem the semaphore to destroy
 */
OSCL_IMPORT_REF void omx_sem_deinit(omx_sem_t* omx_sem);

/** Decreases the value of the semaphore. Blocks if the semaphore
 * value is zero.
 *
 * @param omx_sem the semaphore to decrease
 */
OSCL_IMPORT_REF void omx_sem_down(omx_sem_t* omx_sem);

/** Decreases the value of the semaphore. Blocks if the semaphore
 * value is zero. If the timeout is reached the function exits with
 * error ETIMEDOUT
 *
 * @param omx_sem the semaphore to decrease
 * @param timevalue the value of delay for the timeout
 */
OSCL_EXPORT_REF int omx_sem_timed_down(omx_sem_t* omx_sem, long milliSecondsDelay);

/** Increases the value of the semaphore
 *
 * @param omx_sem the semaphore to increase
 */
OSCL_IMPORT_REF void omx_sem_up(omx_sem_t* omx_sem);

/** Increases the value of the semaphore to one
 *
 * @param omx_sem the semaphore to increase
 */
OSCL_IMPORT_REF void omx_sem_up_to_one(omx_sem_t* omx_sem);

/** Reset the value of the semaphore
 *
 * @param omx_sem the semaphore to reset
 */
OSCL_IMPORT_REF void omx_sem_reset(omx_sem_t* omx_sem);

/** Wait on the condition.
 *
 * @param omx_sem the semaphore to wait
 */
OSCL_IMPORT_REF void omx_sem_wait(omx_sem_t* omx_sem);

/** Signal the condition,if waiting
 *
 * @param omx_sem the semaphore to signal
 */
OSCL_IMPORT_REF void omx_sem_signal(omx_sem_t* omx_sem);

unsigned int omx_sem_get_semval(omx_sem_t* omx_sem);

char *OmxerrorName(OMX_ERRORTYPE error);
void omx_msleep(unsigned int ms);
int omx_usleep(unsigned long usec);

#endif /* __OMX_BASE_H */
