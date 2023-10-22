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


#include <errno.h>
#include "OMX_Base.h"

#include <semphr.h>
#include <task.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>


char *OmxerrorName(OMX_ERRORTYPE error) {

char *nameString;

    switch(error) {
    case 0:
        nameString = "OMX_ErrorNone";
        break;
    case 0x80001000:
        nameString = "OMX_ErrorInsufficientResources";
        break;
    case 0x80001001:
        nameString = "OMX_ErrorUndefined";
        break;
    case 0x80001002:
        nameString = "OMX_ErrorInvalidComponentName";
        break;
    case 0x80001003:
        nameString = "OMX_ErrorComponentNotFound";
        break;
    case 0x80001004:
        nameString = "OMX_ErrorInvalidComponent";
        break;
    case 0x80001005:
        nameString = "OMX_ErrorBadParameter";
        break;
    case 0x80001006:
        nameString = "OMX_ErrorNotImplemented";
        break;
    case 0x80001007:
        nameString = "OMX_ErrorUnderflow";
        break;
    case 0x80001008:
        nameString = "OMX_ErrorOverflow";
        break;
    case 0x80001009:
        nameString = "OMX_ErrorHardware";
        break;
    case 0x8000100A:
        nameString = "OMX_ErrorInvalidState";
        break;
    case 0x8000100B:
        nameString = "OMX_ErrorStreamCorrupt";
        break;
    case 0x8000100C:
        nameString = "OMX_ErrorPortsNotCompatible";
        break;
    case 0x8000100D:
        nameString = "OMX_ErrorResourcesLost";
        break;
    case 0x8000100E:
        nameString = "OMX_ErrorNoMore";
        break;
    case 0x8000100F:
        nameString = "OMX_ErrorVersionMismatch";
        break;
    case 0x80001010:
        nameString = "OMX_ErrorNotReady";
        break;
    case 0x80001011:
        nameString = "OMX_ErrorTimeout";
        break;
    case 0x80001012:
        nameString = "OMX_ErrorSameState";
        break;
    case 0x80001013:
        nameString = "OMX_ErrorResourcesPreempted";
        break;
    case 0x80001014:
        nameString = "OMX_ErrorPortUnresponsiveDuringAllocation";
        break;
    case 0x80001015:
        nameString = "OMX_ErrorPortUnresponsiveDuringDeallocation";
        break;
    case 0x80001016:
        nameString = "OMX_ErrorPortUnresponsiveDuringStop";
        break;
    case 0x80001017:
        nameString = "OMX_ErrorIncorrectStateTransition";
        break;
    case 0x80001018:
        nameString = "OMX_ErrorIncorrectStateOperation";
        break;
    case 0x80001019:
        nameString = "OMX_ErrorUnsupportedSetting";
        break;
    case 0x8000101A:
        nameString = "OMX_ErrorUnsupportedIndex";
        break;
    case 0x8000101B:
        nameString = "OMX_ErrorBadPortIndex";
        break;
    case 0x8000101C:
        nameString = "OMX_ErrorPortUnpopulated";
        break;
    case 0x8000101D:
        nameString = "OMX_ErrorComponentSuspended";
        break;
    case 0x8000101E:
        nameString = "OMX_ErrorDynamicResourcesUnavailable";
        break;
    case 0x8000101F:
        nameString = "OMX_ErrorMbErrorsInFrame";
        break;
    case 0x80001020:
        nameString = "OMX_ErrorFormatNotDetected";
        break;
    case 0x80001021:
        nameString = "OMX_ErrorContentPipeOpenFailed";
        break;
    case 0x80001022:
        nameString = "OMX_ErrorContentPipeCreationFailed";
        break;
    case 0x80001023:
        nameString = "OMX_ErrorSeperateTablesUsed";
        break;
    case 0x80001024:
        nameString = "OMX_ErrorTunnelingUnsupported";
        break;
    default:
        nameString = '\0';
    }
    return nameString;
}

void omx_msleep(unsigned int ms)
{
	vTaskDelay(pdMS_TO_TICKS(ms));
}

int omx_usleep(unsigned long usec)
{
    /* To avoid delaying for less than usec, always round up. */
    const int us_per_tick = portTICK_PERIOD_MS * 1000;
    if (usec < us_per_tick) {
        vTaskDelay(1);
    } else {
        /* since vTaskDelay(1) blocks for anywhere between 0 and portTICK_PERIOD_MS,
         * round up to compensate.
         */
        vTaskDelay((usec + us_per_tick - 1) / us_per_tick);
    }

    return 0;
}


static const omx_thread_attr_internal_t OmxDefaultThreadAttributes =
{
    .usStackSize   = OMX_THREAD_STACK_MIN,
    .usSchedPriorityDetachState = ( ( uint16_t ) (configAPPLICATION_OMX_PRIORITY) & omx_threadSCHED_PRIORITY_MASK) | ( OMXTHREAD_CREATE_JOINABLE << omx_threadDETACH_STATE_SHIFT ),
};

static void OmxprvExitThread(void)
{
    omx_thread_internal_t * pxThread = (omx_thread_internal_t *) omx_thread_self();

    /* If this thread is joinable, wait for a call to pthread_join. */
    if(omx_threadIS_JOINABLE( pxThread->xAttr.usSchedPriorityDetachState ) )
    {
        (void) xSemaphoreGive((SemaphoreHandle_t ) &pxThread->xJoinBarrier);

        /* Suspend until the call to pthread_join. The caller of pthread_join
         * will perform cleanup. */
        vTaskSuspend(NULL);
    }
    else
    {
        /* For a detached thread, perform cleanup of thread object. */
        vPortFree( pxThread );
        vTaskDelete(NULL);
    }
}


static void OmxprvRunThread( void * pxArg )
{
    omx_thread_internal_t * pxThread = ( omx_thread_internal_t *) pxArg;

    /* Run the thread routine. */
    pxThread->xReturn = pxThread->pvStartRoutine((void *) pxThread->xTaskArg);

    /* Exit once finished. This function does not return. */
    OmxprvExitThread();
}

static inline int omx_sched_get_priority_max( int policy )
{
    /* Silence warnings about unused parameters. */
    ( void ) policy;

    return configMAX_PRIORITIES - 1;
}

int omx_thread_attr_init(omx_thread_attr_t * attr)
{
    int iStatus = 0;

    /* Check if the attribute is NULL. */
    if( attr == NULL )
    {
        iStatus = EINVAL;
    }

    /* Copy the default values into the new thread attributes object. */
    if( iStatus == 0 )
    {
        *( ( omx_thread_attr_internal_t * ) ( attr ) ) = OmxDefaultThreadAttributes;
    }

    return iStatus;
}

int omx_thread_attr_setdetachstate( omx_thread_attr_t * attr,
                                 int detachstate )
{
    int iStatus = 0;
    omx_thread_attr_internal_t * pxAttr = ( omx_thread_attr_internal_t * ) ( attr );

    if((detachstate != OMXTHREAD_CREATE_DETACHED) && (detachstate != OMXTHREAD_CREATE_JOINABLE))
    {
        iStatus = EINVAL;
    }
    else
    {
        /* clear and then set msb bit to detachstate) */
        pxAttr->usSchedPriorityDetachState &= ~omx_threadSCHED_PRIORITY_MASK;
        pxAttr->usSchedPriorityDetachState |= ( (uint16_t) detachstate << omx_threadDETACH_STATE_SHIFT );
    }

    return iStatus;
}


int omx_thread_attr_setschedparam( omx_thread_attr_t * attr,
                                const struct omx_sched_param * param )
{
    int iStatus = 0;
    omx_thread_attr_internal_t * pxAttr = ( omx_thread_attr_internal_t * ) ( attr );

    /* Check for NULL param. */
    if( param == NULL )
    {
        iStatus = EINVAL;
    }

    /* Ensure that param.sched_priority is valid. */
    if( ( iStatus == 0 ) &&
        ( ( param->sched_priority > omx_sched_get_priority_max( SCHED_OTHER ) ) ||
          ( param->sched_priority < 0 ) ) )
    {
        iStatus = ENOTSUP;
    }

    /* Set the sched_param. */
    if( iStatus == 0 )
    {
        /* clear and then set  15 LSB to schedule priority) */
        pxAttr->usSchedPriorityDetachState &= ~omx_threadSCHED_PRIORITY_MASK;
        pxAttr->usSchedPriorityDetachState |= ( ( uint16_t) param->sched_priority );
    }

    return iStatus;
}

/*-----------------------------------------------------------*/

int omx_thread_attr_setstacksize( omx_thread_attr_t * attr,
                               int stacksize )
{
    int iStatus = 0;
    omx_thread_attr_internal_t * pxAttr = ( omx_thread_attr_internal_t * ) ( attr );

    if( stacksize < OMX_THREAD_STACK_MIN )
    {
        iStatus = EINVAL;
    }
    else
    {
        pxAttr->usStackSize = ( uint16_t ) stacksize;
    }

    return iStatus;
}

int omx_thread_create( omx_thread_t * thread,
                    const omx_thread_attr_t * attr,
                    void *( *startroutine )( void * ),
                    void * arg )
{
    int iStatus = 0;
    omx_thread_internal_t * pxThread = NULL;
    struct omx_sched_param xSchedParam  = { .sched_priority = tskIDLE_PRIORITY };

    /* Allocate memory for new thread object. */
    pxThread = (omx_thread_internal_t *) pvPortMalloc(sizeof(omx_thread_internal_t));

    if( pxThread == NULL )
    {
        /* No memory. */
        iStatus = EAGAIN;
    }

    if( iStatus == 0 )
    {
        /* No attributes given, use default attributes. */
        if( attr == NULL )
        {
            pxThread->xAttr = OmxDefaultThreadAttributes;
        }
        /* Otherwise, use provided attributes. */
        else
        {
            pxThread->xAttr = *((omx_thread_attr_internal_t *) (attr));
        }

        /* Get priority from attributes */
        xSchedParam.sched_priority =  ( int )omx_threadGET_SCHED_PRIORITY( pxThread->xAttr.usSchedPriorityDetachState);

        /* Set argument and start routine. */
        pxThread->xTaskArg = arg;
        pxThread->pvStartRoutine = startroutine;

        /* If this thread is joinable, create the synchronization mechanisms for
         * pthread_join. */

        if(omx_threadIS_JOINABLE( pxThread->xAttr.usSchedPriorityDetachState ) )
        {
            /* These calls will not fail when their arguments aren't NULL. */
            ( void ) xSemaphoreCreateMutexStatic(&pxThread->xJoinMutex);
            ( void ) xSemaphoreCreateBinaryStatic(&pxThread->xJoinBarrier);
        }
    }

    if(iStatus == 0)
    {
        /* Suspend all tasks to create a critical section. This ensures that
         * the new thread doesn't exit before a tag is assigned. */
        vTaskSuspendAll();

        /* Create the FreeRTOS task that will run the pthread. */
        if(xTaskCreate(OmxprvRunThread,
                         Config_OMXTHREAD_TASK_NAME,
                         ( uint16_t ) ( pxThread->xAttr.usStackSize / sizeof( StackType_t ) ),
                         ( void * ) pxThread,
                         xSchedParam.sched_priority,
                         &pxThread->xTaskHandle ) != pdPASS)
        {
            /* Task creation failed, no memory. */
            vPortFree(pxThread);
            iStatus = EAGAIN;
        }
        else
        {
            /* Store the pointer to the thread object in the task tag. */
            vTaskSetApplicationTaskTag(pxThread->xTaskHandle, ( TaskHookFunction_t ) pxThread);

            /* Set the thread object for the user. */
            *thread = (omx_thread_t) pxThread;
        }

        /* End the critical section. */
        xTaskResumeAll();
    }

    return iStatus;
}


int omx_thread_equal( omx_thread_t t1,
                   omx_thread_t t2 )
{
    int iStatus = 0;

    /* Compare the thread IDs. */
    if((t1 != NULL) && (t2 != NULL))
    {
        iStatus = (t1 == t2);
    }

    return iStatus;
}

int omx_thread_join( omx_thread_t pthread,
                  void ** retval )
{
    int iStatus = 0;
    omx_thread_internal_t * pxThread = ( omx_thread_internal_t * ) pthread;

    /* Make sure pthread is joinable. Otherwise, this function would block
     * forever waiting for an unjoinable thread. */
    if (!omx_threadIS_JOINABLE( pxThread->xAttr.usSchedPriorityDetachState))
    {
        iStatus = EDEADLK;
    }

    /* Only one thread may attempt to join another. Lock the join mutex
     * to prevent other threads from calling pthread_join on the same thread. */
    if(iStatus == 0)
    {
        if(xSemaphoreTake((SemaphoreHandle_t )&pxThread->xJoinMutex, 0) != pdPASS)
        {
            /* Another thread has already joined the requested thread, which would
             * cause this thread to wait forever. */
            iStatus = EDEADLK;
        }
    }

    /* Attempting to join the calling thread would cause a deadlock. */
    if(iStatus == 0)
    {
        if(omx_thread_equal(omx_thread_self(), pthread ) != 0)
        {
            iStatus = EDEADLK;
        }
    }

    if(iStatus == 0)
    {
        /* Wait for the joining thread to finish. Because this call waits forever,
         * it should never fail. */
        ( void ) xSemaphoreTake((SemaphoreHandle_t) &pxThread->xJoinBarrier, portMAX_DELAY);

        /* Create a critical section to clean up the joined thread. */
		taskENTER_CRITICAL();


        /* Release xJoinBarrier and delete it. */
        ( void )xSemaphoreGive((SemaphoreHandle_t) &pxThread->xJoinBarrier);
        vSemaphoreDelete((SemaphoreHandle_t) &pxThread->xJoinBarrier);

        /* Release xJoinMutex and delete it. */
        ( void )xSemaphoreGive((SemaphoreHandle_t) &pxThread->xJoinMutex);
        vSemaphoreDelete((SemaphoreHandle_t) &pxThread->xJoinMutex);

        /* Delete the FreeRTOS task that ran the thread. */
		taskEXIT_CRITICAL();

		vTaskDelete(pxThread->xTaskHandle);

		taskENTER_CRITICAL();

        /* Set the return value. */
        if(retval != NULL)
        {
            *retval = pxThread->xReturn;
        }

        /* Free the thread object. */
        vPortFree(pxThread);

        /* End the critical section. */

		taskEXIT_CRITICAL();

    }

    return iStatus;
}

omx_thread_t omx_thread_self( void )
{
    /* Return a reference to this pthread object, which is stored in the
     * FreeRTOS task tag. */
    return ( omx_thread_t ) xTaskGetApplicationTaskTag( NULL );
}

int omx_thread_setschedparam( omx_thread_t thread,
			                       int policy,
			                       const struct omx_sched_param * param )
{
    int iStatus = 0;

    omx_thread_internal_t * pxThread = ( omx_thread_internal_t * ) thread;

    /* Silence compiler warnings about unused parameters. */
    ( void ) policy;

    /* Copy the given sched_param. */
    iStatus = omx_thread_attr_setschedparam( ( omx_thread_attr_t * ) &pxThread->xAttr, param );

    if ( iStatus == 0 )
    {
        /* Change the priority of the FreeRTOS task. */
        vTaskPrioritySet( pxThread->xTaskHandle, param->sched_priority );
    }

    return iStatus;
}

int omx_thread_setname_np(omx_thread_t thread, const char * name)
{
    int iStatus = 0;

    omx_thread_internal_t * pxThread = ( omx_thread_internal_t * ) thread;

    vTaskSetName(pxThread->xTaskHandle, name);

    return iStatus;
}

int omx_thread_getname_np(omx_thread_t thread, char * name, int len)
{
    int iStatus = 0;
    char * in_name;

    if(len > configMAX_TASK_NAME_LEN)
    {
		return -1;
    }

    omx_thread_internal_t * pxThread = ( omx_thread_internal_t * ) thread;

    in_name = pcTaskGetName(pxThread->xTaskHandle);
    memcpy(name, in_name, len);

    return iStatus;
}

/**
 * @brief Default pthread_mutexattr_t.
 */
static const omx_thread_mutexattr_internal_t OmXDefaultMutexAttributes =
{
    .iType = OMX_THREAD_MUTEX_DEFAULT,
};


static void OmxprvInitializeStaticMutex( omx_thread_mutex_internal_t * pxMutex )
{
    /* Check if the mutex needs to be initialized. */
    if( pxMutex->xIsInitialized == pdFALSE )
    {
        /* Mutex initialization must be in a critical section to prevent two threads
         * from initializing it at the same time. */
        taskENTER_CRITICAL();

        /* Check again that the mutex is still uninitialized, i.e. it wasn't
         * initialized while this function was waiting to enter the critical
         * section. */
        if( pxMutex->xIsInitialized == pdFALSE )
        {
            /* Set the mutex as the default type. */
            pxMutex->xAttr.iType = OMX_THREAD_MUTEX_DEFAULT;

            /* Call the correct FreeRTOS mutex initialization function based on
             * the mutex type. */
            #if OMX_THREAD_MUTEX_DEFAULT == OMX_THREAD_MUTEX_RECURSIVE
                ( void ) xSemaphoreCreateRecursiveMutexStatic( &pxMutex->xMutex );
            #else
                ( void ) xSemaphoreCreateMutexStatic( &pxMutex->xMutex );
            #endif

            pxMutex->xIsInitialized = pdTRUE;
        }

        /* Exit the critical section. */
        taskEXIT_CRITICAL();

    }
}

int omx_thread_mutex_init( omx_thread_mutex_t * mutex,
                        const omx_thread_mutexattr_t * attr )
{
    int iStatus = 0;
    omx_thread_mutex_internal_t * pxMutex = ( omx_thread_mutex_internal_t* ) mutex;

    if( pxMutex == NULL )
    {
        /* No memory. */
        iStatus = ENOMEM;
    }

    if( iStatus == 0 )
    {
        *pxMutex = OMX_MUTEX_INITIALIZER;

        /* No attributes given, use default attributes. */
        if( attr == NULL )
        {
            pxMutex->xAttr = OmXDefaultMutexAttributes;
        }
        /* Otherwise, use provided attributes. */
        else
        {
            pxMutex->xAttr = *((omx_thread_mutexattr_internal_t * ) (attr));
        }

        /* Call the correct FreeRTOS mutex creation function based on mutex type. */
        if( pxMutex->xAttr.iType == OMX_THREAD_MUTEX_RECURSIVE)
        {
            /* Recursive mutex. */
            (void) xSemaphoreCreateRecursiveMutexStatic( &pxMutex->xMutex );
        }
        else
        {
            /* All other mutex types. */
            (void) xSemaphoreCreateMutexStatic( &pxMutex->xMutex );
        }

        /* Mutex successfully created. */
        pxMutex->xIsInitialized = pdTRUE;
    }

    return iStatus;
}

/*-----------------------------------------------------------*/

int omx_thread_mutex_destroy( omx_thread_mutex_t * mutex )
{
    omx_thread_mutex_internal_t * pxMutex = (omx_thread_mutex_internal_t *) ( mutex );

    /* Free resources in use by the mutex. */
    if( pxMutex->xTaskOwner == NULL )
    {
        vSemaphoreDelete((SemaphoreHandle_t) &pxMutex->xMutex);
    }
    return 0;
}


int omx_thread_mutex_lock( omx_thread_mutex_t * mutex )
{
    return omx_thread_mutex_timedlock( mutex, 0);
}


int omx_thread_mutex_timedlock( omx_thread_mutex_t * mutex,
                             long milliSecondsDelay )
{
    int iStatus = 0;
    omx_thread_mutex_internal_t * pxMutex = ( omx_thread_mutex_internal_t * ) ( mutex );
    TickType_t xDelay = portMAX_DELAY;
    BaseType_t xFreeRTOSMutexTakeStatus = pdFALSE;

    /* If mutex in uninitialized, perform initialization. */
    OmxprvInitializeStaticMutex( pxMutex );

	if (pxMutex->xIsInitialized != pdTRUE)
		omx_err("xIsInitialized %d.\n", pxMutex->xIsInitialized);

    assert( pxMutex->xIsInitialized == pdTRUE );

    /* Convert abstime to a delay in TickType_t if provided. */
#if 0
    if( abstime != NULL )
    {
        struct timespec xCurrentTime = { 0 };

        /* Get current time */
        if( clock_gettime( CLOCK_REALTIME, &xCurrentTime ) != 0 )
        {
            iStatus = EINVAL;
        }
        else
        {
            iStatus = UTILS_AbsoluteTimespecToDeltaTicks( abstime, &xCurrentTime, &xDelay );
        }

        /* If abstime was in the past, still attempt to lock the mutex without
         * blocking, per POSIX spec. */
        if( iStatus == ETIMEDOUT )
        {
            xDelay = 0;
            iStatus = 0;
        }
    }
#endif
	if(milliSecondsDelay != 0) {
		xDelay = milliSecondsDelay / portTICK_PERIOD_MS;
	}
    /* Check if trying to lock a currently owned mutex. */
    if( ( iStatus == 0 ) &&
        ( pxMutex->xAttr.iType == 1 ) &&  /* Only PTHREAD_MUTEX_ERRORCHECK type detects deadlock. */
        ( pxMutex->xTaskOwner == xTaskGetCurrentTaskHandle() ) ) /* Check if locking a currently owned mutex. */
    {
        iStatus = EDEADLK;
    }

    if( iStatus == 0 )
    {
        /* Call the correct FreeRTOS mutex take function based on mutex type. */
        if( pxMutex->xAttr.iType == OMX_THREAD_MUTEX_RECURSIVE )
        {
            xFreeRTOSMutexTakeStatus = xSemaphoreTakeRecursive( ( SemaphoreHandle_t ) &pxMutex->xMutex, xDelay );
        }
        else
        {
            xFreeRTOSMutexTakeStatus = xSemaphoreTake( ( SemaphoreHandle_t ) &pxMutex->xMutex, xDelay );
        }

        /* If the mutex was successfully taken, set its owner. */
        if( xFreeRTOSMutexTakeStatus == pdPASS )
        {
            pxMutex->xTaskOwner = xTaskGetCurrentTaskHandle();
        }
        /* Otherwise, the mutex take timed out. */
        else
        {
            iStatus = ETIMEDOUT;
        }
    }

    return iStatus;
}

/*-----------------------------------------------------------*/

int omx_thread_mutex_unlock( omx_thread_mutex_t * mutex )
{
    int iStatus = 0;
    omx_thread_mutex_internal_t * pxMutex = ( omx_thread_mutex_internal_t * ) ( mutex );

    /* If mutex in uninitialized, perform initialization. */
    OmxprvInitializeStaticMutex( pxMutex );

    /* Check if trying to unlock an unowned mutex. */
    if( ( ( pxMutex->xAttr.iType == OMX_THREAD_MUTEX_ERRORCHECK ) ||
          ( pxMutex->xAttr.iType == OMX_THREAD_MUTEX_RECURSIVE ) ) &&
        ( pxMutex->xTaskOwner != xTaskGetCurrentTaskHandle() ) )
    {
        iStatus = EPERM;
    }

    if( iStatus == 0 )
    {
		taskENTER_CRITICAL();
        /* Call the correct FreeRTOS mutex unlock function based on mutex type. */
        if( pxMutex->xAttr.iType == OMX_THREAD_MUTEX_RECURSIVE )
        {
			taskEXIT_CRITICAL();
			( void ) xSemaphoreGiveRecursive( ( SemaphoreHandle_t ) &pxMutex->xMutex );
			taskENTER_CRITICAL();

        }
        else
        {
            taskEXIT_CRITICAL();
            ( void ) xSemaphoreGive( ( SemaphoreHandle_t ) &pxMutex->xMutex );
			taskENTER_CRITICAL();
        }

        /* Update the owner of the mutex. A recursive mutex may still have an
         * owner, so it should be updated with xSemaphoreGetMutexHolder. */
        pxMutex->xTaskOwner = xSemaphoreGetMutexHolder( ( SemaphoreHandle_t ) &pxMutex->xMutex );
        taskEXIT_CRITICAL();
    }

    return iStatus;
}

omx_mutex_t omx_mutex_init(void)
{
	return xSemaphoreCreateMutex();
}

int omx_mutex_lock_timeout(omx_mutex_t mutex, long ms)
{
	BaseType_t ret;
	const TickType_t timeout = ms / portTICK_PERIOD_MS;

	if (!mutex) {
		omx_err("mutex is null.\n");
		return -EFAULT;
	}
	omx_debug("\n");
	ret = xSemaphoreTake(mutex, timeout);
	if (ret == pdPASS)
		return 0;
	omx_err("mutex lock failed.\n");
	return -EFAULT;
}

int omx_mutex_lock(omx_mutex_t mutex)
{
	BaseType_t ret;

	if (!mutex) {
		omx_err("mutex is null.\n");
		return -EFAULT;
	}
	omx_debug("\n");
	ret = xSemaphoreTake(mutex, portMAX_DELAY);
	if (ret == pdPASS)
		return 0;
	omx_err("mutex lock failed.\n");
	return -EFAULT;
}

int omx_mutex_unlock(omx_mutex_t mutex)
{
	BaseType_t ret;

	if (!mutex) {
		omx_err("mutex is null.\n");
		return -EFAULT;
	}
	omx_debug("\n");
	ret = xSemaphoreGive(mutex);
	if (ret == pdPASS)
		return 0;
	omx_err("mutex unlock failed.\n");
	return -EFAULT;
}

void omx_mutex_destroy(omx_mutex_t mutex)
{
	if (!mutex) {
		omx_err("mutex is null.\n");
		return;
	}

	vSemaphoreDelete(mutex);
}


/*-----------------------------------------------------------*/

static void prvInitializeStaticCond( omx_thread_cond_internal_t * pxCond )
{
    /* Check if the condition variable needs to be initialized. */
    if( pxCond->xIsInitialized == pdFALSE )
    {
        /* Cond initialization must be in a critical section to prevent two threads
         * from initializing it at the same time. */
		taskENTER_CRITICAL();

        /* Check again that the cond is still uninitialized, i.e. it wasn't
         * initialized while this function was waiting to enter the critical
         * section. */
        if( pxCond->xIsInitialized == pdFALSE )
        {
            /* Set the members of the cond. The semaphore create calls will never fail
             * when their arguments aren't NULL. */
            pxCond->xIsInitialized = pdTRUE;
            ( void ) xSemaphoreCreateMutexStatic( &pxCond->xCondMutex );
            ( void ) xSemaphoreCreateCountingStatic( INT_MAX, 0U, &pxCond->xCondWaitSemaphore );
            pxCond->iWaitingThreads = 0;
        }

        /* Exit the critical section. */

		taskEXIT_CRITICAL();

    }
}

/*-----------------------------------------------------------*/

int omx_thread_cond_init( omx_thread_cond_t * cond,
                       const void * attr )
{
    int iStatus = 0;
    omx_thread_cond_internal_t * pxCond = ( omx_thread_cond_internal_t * ) cond;

    /* Silence warnings about unused parameters. */
    ( void ) attr;

    if( pxCond == NULL )
    {
        iStatus = ENOMEM;
    }

    if( iStatus == 0 )
    {
        /* Set the members of the cond. The semaphore create calls will never fail
         * when their arguments aren't NULL. */
        pxCond->xIsInitialized = pdTRUE;
        ( void ) xSemaphoreCreateMutexStatic( &pxCond->xCondMutex );
        ( void ) xSemaphoreCreateCountingStatic( INT_MAX, 0U, &pxCond->xCondWaitSemaphore );
        pxCond->iWaitingThreads = 0;
    }

    return iStatus;
}

/*-----------------------------------------------------------*/

int omx_thread_cond_signal( omx_thread_cond_t * cond )
{
    omx_thread_cond_internal_t * pxCond = ( omx_thread_cond_internal_t * ) ( cond );

    /* If the cond is uninitialized, perform initialization. */
    prvInitializeStaticCond( pxCond );

    /* Check that at least one thread is waiting for a signal. */
    if( pxCond->iWaitingThreads > 0 )
    {
        /* Lock xCondMutex to protect access to iWaitingThreads.
         * This call will never fail because it blocks forever. */
        ( void ) xSemaphoreTake( ( SemaphoreHandle_t ) &pxCond->xCondMutex, portMAX_DELAY );

        /* Check again that at least one thread is waiting for a signal after
         * taking xCondMutex. If so, unblock it. */
        if( pxCond->iWaitingThreads > 0 )
        {
            ( void ) xSemaphoreGive( ( SemaphoreHandle_t ) &pxCond->xCondWaitSemaphore );

            /* Decrease the number of waiting threads. */
            pxCond->iWaitingThreads--;
        }

        /* Release xCondMutex. */
        ( void ) xSemaphoreGive( ( SemaphoreHandle_t ) &pxCond->xCondMutex );
    }

    return 0;
}

/*-----------------------------------------------------------*/

int omx_thread_cond_timedwait( omx_thread_cond_t * cond,
                            omx_mutex_t mutex,
                            long milliSecondsDelay)
{
    int iStatus = 0;
    omx_thread_cond_internal_t * pxCond = ( omx_thread_cond_internal_t * ) ( cond );
    TickType_t xDelay = portMAX_DELAY;

    /* If the cond is uninitialized, perform initialization. */
    prvInitializeStaticCond( pxCond );

    /* Convert abstime to a delay in TickType_t if provided.
    if( abstime != NULL )
    {
        struct timespec xCurrentTime = { 0 };

        if( clock_gettime( CLOCK_REALTIME, &xCurrentTime ) != 0 )
        {
            iStatus = EINVAL;
        }
        else
        {
            iStatus = UTILS_AbsoluteTimespecToDeltaTicks( abstime, &xCurrentTime, &xDelay );
        }
    }*/
	if(milliSecondsDelay != 0) {
		xDelay = milliSecondsDelay / portTICK_PERIOD_MS;
	}

    /* Increase the counter of threads blocking on condition variable, then
     * unlock mutex. */
    if( iStatus == 0 )
    {
        ( void ) xSemaphoreTake( ( SemaphoreHandle_t ) &pxCond->xCondMutex, portMAX_DELAY );
        pxCond->iWaitingThreads++;
        ( void ) xSemaphoreGive( ( SemaphoreHandle_t ) &pxCond->xCondMutex );

        iStatus = omx_mutex_unlock(mutex);
    }

    /* Wait on the condition variable. */
    if( iStatus == 0 )
    {
        if( xSemaphoreTake( ( SemaphoreHandle_t ) &pxCond->xCondWaitSemaphore,
                            xDelay ) == pdPASS )
        {
            /* When successful, relock mutex. */
            iStatus = omx_mutex_lock(mutex);
        }
        else
        {
            /* Timeout. Relock mutex and decrement number of waiting threads. */
            iStatus = ETIMEDOUT;
            ( void ) omx_mutex_lock( mutex );

            ( void ) xSemaphoreTake( ( SemaphoreHandle_t ) &pxCond->xCondMutex, portMAX_DELAY );
            pxCond->iWaitingThreads--;
            ( void ) xSemaphoreGive( ( SemaphoreHandle_t ) &pxCond->xCondMutex );
        }
    }

    return iStatus;
}

/*-----------------------------------------------------------*/

int omx_thread_cond_wait( omx_thread_cond_t * cond,
                       omx_mutex_t  mutex)
{
    return omx_thread_cond_timedwait( cond, mutex, 0);
}

int omx_thread_cond_destroy( omx_thread_cond_t * cond )
{
    omx_thread_cond_internal_t * pxCond = ( omx_thread_cond_internal_t * ) ( cond );

    /* Free all resources in use by the cond. */
    vSemaphoreDelete( ( SemaphoreHandle_t ) &pxCond->xCondMutex );
    vSemaphoreDelete( ( SemaphoreHandle_t ) &pxCond->xCondWaitSemaphore );

    return 0;
}

/** @@ Modified code
 * added some queue functions for deinterlace case.
 */

/** Initialize a queue descriptor
 *
 * @param queue The queue descriptor to initialize.
 * The user needs to allocate the queue
 */
int queue_init(queue_t* queue) {

	int i;
    qelem_t* newelem;
    qelem_t* current;

    queue->mutex = omx_mutex_init();

    queue->first = omx_alloc(sizeof(qelem_t));
    if (!(queue->first)) {
        return -1;
    }

    memset(queue->first, 0, sizeof(qelem_t));

    current = queue->last = queue->first;

    queue->nelem = 0;

    for (i = 0; i< MAX_QUEUE_ELEMENTS - 2; i++) {
        newelem = omx_alloc(sizeof(qelem_t));
        if (!newelem) {
            // the memory is not enough. Free all
            while(queue->first!=NULL) {
                current = queue->first->q_forw;
                omx_free(queue->first);
                queue->first = current;
            }
            return -1;
        }
        memset(newelem, 0, sizeof(qelem_t));
        current->q_forw = newelem;
        current = newelem;
    }
    current->q_forw = queue->first;
    return 0;
}

/** Deinitialize a queue descriptor
 * flushing all of its internal data
 *
 * @param queue the queue descriptor to dump
 */
void queue_deinit(queue_t* queue) {

    int i;
    qelem_t* current;
    current = queue->first;

    for (i = 0; i< MAX_QUEUE_ELEMENTS - 2; i++) {
        if (current != NULL) {
            current = current->q_forw;
            omx_free(queue->first);
            queue->first = current;
        }
    }
    if(queue->first) {
        omx_free(queue->first);
        queue->first = NULL;
    }
    omx_mutex_destroy(queue->mutex);
}

/** Enqueue an element to the given queue descriptor
 *
 * @param queue the queue descriptor where to queue data
 *
 * @param data the data to be enqueued
 *
 * @return -1 if the queue is full
 */
int queue(queue_t* queue, void* data) {

    if (queue->last->data != NULL) {
        return -1;
    }

    omx_mutex_lock(queue->mutex);

    queue->last->data = data;
    queue->last = queue->last->q_forw;
    queue->nelem++;

    omx_mutex_unlock(queue->mutex);

    return 0;
}

/** Dequeue an element from the given queue descriptor
 *
 * @param queue the queue descriptor from which to dequeue the element
 *
 * @return the element that has bee dequeued. If the queue is empty
 *  a NULL value is returned
 */
void* dequeue(queue_t* queue) {

    void* data;

    if (queue->first->data == NULL) {
        return NULL;
    }

    omx_mutex_lock(queue->mutex);

    data = queue->first->data;
    queue->first->data = NULL;
    queue->first = queue->first->q_forw;
    queue->nelem--;

    omx_mutex_unlock(queue->mutex);

    return data;
}

/** Returns the number of elements hold in the queue
 *
 * @param queue the requested queue
 *
 * @return the number of elements in the queue
 */
int getquenelem(queue_t* queue) {

    int qelem;

    omx_mutex_lock(queue->mutex);
    qelem = queue->nelem;
    omx_mutex_unlock(queue->mutex);

    return qelem;
}

/** Peek the queue
 *
 *  @param queue the requested queue
 *  @data  allocated externally
 *  @size  size of external data
 *
 *  Add by Alan Wang
 */
int peekqueue(queue_t* queue, void** data) {

    if (queue == NULL) {
        *data = NULL;
        return -1;
    }

    omx_mutex_lock(queue->mutex);
    /* Queue is empty */
    if (queue->nelem == 0)
    {
        *data = NULL;
        omx_mutex_unlock(queue->mutex);
        return -1;
    }

    *data = queue->first->data;
    omx_mutex_unlock(queue->mutex);

    return 0;
}

int queue_erase(queue_t* queue, void* data) {

    qelem_t* current;
    qelem_t* previous;
    int find = 0;

    if (queue == NULL)
		return -1;

    omx_mutex_lock(queue->mutex);
    /* Queue is empty */
    if (queue->first->data == NULL)
    {
        omx_debug("return -1\n");
        omx_mutex_unlock(queue->mutex);
        return -1;
    }

    current = queue->first;
    if (current->data == data) {
        queue->first->data = NULL;
        queue->first = queue->first->q_forw;
        queue->nelem--;
        omx_mutex_unlock(queue->mutex);
        return 0;
    }

    while (current != queue->last) {
        if (current->data == data) {
            find = 1;
            break;
        }
        previous = current;
        current = current->q_forw;
    }

    if (!find) {
        omx_debug("not found, return -1\n");
        omx_mutex_unlock(queue->mutex);
        return -1;
    }

    while (current != queue->last) {
        current->data = current->q_forw->data;
        previous = current;
        current = current->q_forw;
    }
    queue->last = previous;
    queue->nelem--;
    omx_mutex_unlock(queue->mutex);

    return 0;
}

int queue_first(queue_t* queue, void* data) {

    qelem_t* current;

    if (queue == NULL)
		return -1;

    omx_mutex_lock(queue->mutex);
    if (queue->first->data == NULL) {
        queue->first->data = data;
        queue->last = queue->last->q_forw;
        queue->nelem++;
        omx_mutex_unlock(queue->mutex);
        return 0;
    }

    current = queue->last;
    while (current->q_forw !=  queue->first) {
        current = current->q_forw;
    }

    if (current->data != NULL) {
        omx_debug("not found pre elem of first, return -1\n");
        omx_mutex_unlock(queue->mutex);
        return -1;
    }

    current->data = data;
    queue->first = current;
    queue->nelem++;
    omx_mutex_unlock(queue->mutex);

    return 0;
}

/** @@ Modified code
 * fixed a bug in omx_sem_timed_down function.
 * added function for deinterlace case.
 **/

/** Initializes the semaphore at a given value
 *
 * @param omx_sem the semaphore to initialize
 * @param val the initial value of the semaphore
 *
 */
OSCL_EXPORT_REF int omx_sem_init(omx_sem_t* omx_sem, unsigned int val) {

    int ret = -1;

    ret = omx_thread_cond_init(&omx_sem->condition, NULL);
    if (ret != 0) {
        return -1;
    }

	omx_sem->mutex = omx_mutex_init();

    omx_sem->semval = val;

    return 0;
}

/** Destroy the semaphore
 *
 * @param omx_sem the semaphore to destroy
 */
OSCL_EXPORT_REF void omx_sem_deinit(omx_sem_t* omx_sem) {

    omx_thread_cond_destroy(&omx_sem->condition);
    omx_mutex_destroy(omx_sem->mutex);

}

/** Decreases the value of the semaphore. Blocks if the semaphore
 * value is zero. If the timeout is reached the function exits with
 * error ETIMEDOUT
 *
 * @param omx_sem the semaphore to decrease
 * @param timevalue the value of delay for the timeout
 */
OSCL_EXPORT_REF int omx_sem_timed_down(omx_sem_t* omx_sem, long milliSecondsDelay) {

    int err = 0;
    //struct timespec final_time;
	//struct timespec xWaitTime = { 0 };
    //struct timeval currentTime;
    //long int microdelay;

	//( void ) clock_gettime( CLOCK_REALTIME, &xWaitTime );

    /** convert timeval to timespec and add delay in milliseconds for the timeout
    microdelay = ((milliSecondsDelay * 1000 + xWaitTime.tv_usec));
    final_time.tv_sec = xWaitTime.tv_sec + (microdelay / 1000000);
    final_time.tv_nsec = (microdelay % 1000000) * 1000;
    */
	//( void ) UTILS_TimespecAddNanoseconds(&xWaitTime, milliSecondsDelay * 1000, &final_time);

    omx_mutex_lock(omx_sem->mutex);
    while (omx_sem->semval == 0) {
        err = omx_thread_cond_timedwait(&omx_sem->condition, omx_sem->mutex, milliSecondsDelay);
        if (err != 0) {
            /** @@ Modified code
             * fixed a bug in omx_sem_timed_down function.
             **/
            omx_sem->semval++;
        }
    }
    omx_sem->semval--;
    omx_mutex_unlock(omx_sem->mutex);

    return err;
}

/** Decreases the value of the semaphore. Blocks if the semaphore
 * value is zero.
 *
 * @param omx_sem the semaphore to decrease
 */
OSCL_EXPORT_REF void omx_sem_down(omx_sem_t* omx_sem) {

    omx_mutex_lock(omx_sem->mutex);

    while (omx_sem->semval == 0) {
        omx_thread_cond_wait(&omx_sem->condition, omx_sem->mutex);
    }
    omx_sem->semval--;

    omx_mutex_unlock(omx_sem->mutex);

}

/** Increases the value of the semaphore
 *
 * @param omx_sem the semaphore to increase
 */
OSCL_EXPORT_REF void omx_sem_up(omx_sem_t* omx_sem) {

    omx_mutex_lock(omx_sem->mutex);
    omx_sem->semval++;
    omx_thread_cond_signal(&omx_sem->condition);
    omx_mutex_unlock(omx_sem->mutex);

}

/** Increases the value of the semaphore to one
 *
 * @param omx_sem the semaphore to increase
 */
OSCL_EXPORT_REF void omx_sem_up_to_one(omx_sem_t* omx_sem) {

    omx_mutex_lock(omx_sem->mutex);
    if (omx_sem->semval == 0) {
        omx_sem->semval++;
        omx_thread_cond_signal(&omx_sem->condition);
    }
    omx_mutex_unlock(omx_sem->mutex);

}

/** Reset the value of the semaphore
 *
 * @param omx_sem the semaphore to reset
 */
OSCL_EXPORT_REF void omx_sem_reset(omx_sem_t* omx_sem) {

    omx_mutex_lock(omx_sem->mutex);
    omx_sem->semval=0;
    omx_mutex_unlock(omx_sem->mutex);

}

/** Wait on the condition.
 *
 * @param omx_sem the semaphore to wait
 */
OSCL_EXPORT_REF void omx_sem_wait(omx_sem_t* omx_sem) {

    omx_mutex_lock(omx_sem->mutex);
    omx_thread_cond_wait(&omx_sem->condition, omx_sem->mutex);
    omx_mutex_unlock(omx_sem->mutex);

}

/** Signal the condition,if waiting
 *
 * @param omx_sem the semaphore to signal
 */
OSCL_EXPORT_REF void omx_sem_signal(omx_sem_t* omx_sem) {

    omx_mutex_lock(omx_sem->mutex);
    omx_thread_cond_signal(&omx_sem->condition);
    omx_mutex_unlock(omx_sem->mutex);

}

unsigned int omx_sem_get_semval(omx_sem_t* omx_sem) {

    return omx_sem->semval;

}

