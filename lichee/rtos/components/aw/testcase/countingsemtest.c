#include "serial.h"
#include <stdio.h>
#include "serial.h"
#include <stdint.h>    
#include <FreeRTOS.h>
#include <string.h>
#include <task.h>
#include "processor.h"
#include "interrupt.h"
#include "stdio.h"
#include "semphr.h"
#include "timers.h"

uint32_t freert_cpus_lock(void);
void freert_cpus_unlock(uint32_t);
#define countNUM_TEST_TASKS  		( 2 )
#define countDONT_BLOCK                 ( 0 )
/* The maximum count value that the semaphore used for the demo can hold. */
#define countMAX_COUNT_VALUE    	( 200 )
/* The structure that is passed into the task as the task parameter. */

/* Constants used to indicate whether or not the semaphore should have been
created with its maximum count value, or its minimum count value.  These
numbers are used to ensure that the pointers passed in as the task parameters
are valid. */                                                        
#define countSTART_AT_MAX_COUNT 	( 0xaa )
#define countSTART_AT_ZERO              ( 0x55 )
typedef struct COUNT_SEM_STRUCT
{      
        /* The semaphore to be used for the demo. */
        SemaphoreHandle_t xSemaphore;
       
        /* Set to countSTART_AT_MAX_COUNT if the semaphore should be created with
        its count value set to its max count value, or countSTART_AT_ZERO if it
        should have been created with its count value set to 0. */
        UBaseType_t uxExpectedStartCount;
       
        /* Incremented on each cycle of the demo task.  Used to detect a stalled
        task. */
        volatile UBaseType_t uxLoopCounter;
} xCountSemStruct; 

/* Two structures are defined, one is passed to each test task. */
static xCountSemStruct xParameters[ countNUM_TEST_TASKS ];

static void prvIncrementSemaphoreCount( SemaphoreHandle_t xSemaphore, volatile UBaseType_t *puxLoopCounter )
{            
UBaseType_t ux;
             
        /* If the semaphore count is zero then we should not be able to 'take'
        the semaphore. */
        if( xSemaphoreTake( xSemaphore, countDONT_BLOCK ) == pdPASS )
        {    
		SMP_DBG("fatal error.\n");
		soft_break();
        }    
             
        /* We should be able to 'give' the semaphore countMAX_COUNT_VALUE times. */
        for( ux = 0; ux < countMAX_COUNT_VALUE; ux++ )
        {    
                configASSERT( uxSemaphoreGetCount( xSemaphore ) == ux );
             
                if( xSemaphoreGive( xSemaphore ) != pdPASS )
                {
                        /* We expected to be able to take the semaphore. */
			SMP_DBG("fatal error.\n");
			soft_break();
                }
             
                ( *puxLoopCounter )++;
        }    
             
        /* If the semaphore count is at its maximum then we should not be able to
        'give' the semaphore. */
        if( xSemaphoreGive( xSemaphore ) == pdPASS )
        {    
		SMP_DBG("fatal error.\n");
		soft_break();
        }    
	uint32_t cpusr;
	cpusr = freert_cpus_lock();
	SMP_DBG("success.\n");
	freert_cpus_unlock(cpusr);
} 

static void prvDecrementSemaphoreCount( SemaphoreHandle_t xSemaphore, volatile UBaseType_t *puxLoopCounter )
{       
UBaseType_t ux;
        
        /* If the semaphore count is at its maximum then we should not be able to
        'give' the semaphore. */
        if( xSemaphoreGive( xSemaphore ) == pdPASS )
        {   
		SMP_DBG("fatal error.\n");
		soft_break();
        }   
        
        /* We should be able to 'take' the semaphore countMAX_COUNT_VALUE times. */
        for( ux = 0; ux < countMAX_COUNT_VALUE; ux++ )
        {   
                configASSERT( uxSemaphoreGetCount( xSemaphore ) == ( countMAX_COUNT_VALUE - ux ) );
        
                if( xSemaphoreTake( xSemaphore, countDONT_BLOCK ) != pdPASS )
                {
                        /* We expected to be able to take the semaphore. */
			SMP_DBG("fatal error.\n");
			soft_break();
                }
        
                ( *puxLoopCounter )++;
        }   
        
        /* If the semaphore count is zero then we should not be able to 'take'
        the semaphore. */
        configASSERT( uxSemaphoreGetCount( xSemaphore ) == 0 );
        if( xSemaphoreTake( xSemaphore, countDONT_BLOCK ) == pdPASS )
        {   
		SMP_DBG("fatal error.\n");
		soft_break();
        }
	uint32_t cpusr;
	cpusr = freert_cpus_lock();
	SMP_DBG("success.\n");
	freert_cpus_unlock(cpusr);
}

static void prvCountingSemaphoreTask( void *pvParameters )
{                           
xCountSemStruct *pxParameter;
                            
        /* The semaphore to be used was passed as the parameter. */
        pxParameter = ( xCountSemStruct * ) pvParameters;
                            
        /* Did we expect to find the semaphore already at its max count value, or
        at zero? */                    
        if( pxParameter->uxExpectedStartCount == countSTART_AT_MAX_COUNT )
        {                   
                prvDecrementSemaphoreCount( pxParameter->xSemaphore, &( pxParameter->uxLoopCounter ) );
        }                   
                            
        /* Now we expect the semaphore count to be 0, so this time there is an
        error if we can take the semaphore. */
        if( xSemaphoreTake( pxParameter->xSemaphore, 0 ) == pdPASS )
        {                   
		SMP_DBG("fatal error.\n");
		soft_break();
        }                   
                            
        for( ;; )           
        {                              
                prvIncrementSemaphoreCount( pxParameter->xSemaphore, &( pxParameter->uxLoopCounter ) );
                prvDecrementSemaphoreCount( pxParameter->xSemaphore, &( pxParameter->uxLoopCounter ) );
        }                   

	vTaskDelete(NULL);
   	return;
} 

void cpu0_app_main(void)
{
        /* Create the semaphores that we are going to use for the test/demo.  The
        first should be created such that it starts at its maximum count value,
        the second should be created such that it starts with a count value of zero. */
        xParameters[ 0 ].xSemaphore = xSemaphoreCreateCounting( countMAX_COUNT_VALUE, countMAX_COUNT_VALUE );
        xParameters[ 0 ].uxExpectedStartCount = countSTART_AT_MAX_COUNT;
        xParameters[ 0 ].uxLoopCounter = 0;
        xParameters[ 1 ].xSemaphore = xSemaphoreCreateCounting( countMAX_COUNT_VALUE, 0 );
        xParameters[ 1 ].uxExpectedStartCount = 0;
        xParameters[ 1 ].uxLoopCounter = 0;
                                       
        /* Were the semaphores created? */
        if( ( xParameters[ 0 ].xSemaphore != NULL ) || ( xParameters[ 1 ].xSemaphore != NULL ) )
        {      
                /* vQueueAddToRegistry() adds the semaphore to the registry, if one is
                in use.  The registry is provided as a means for kernel aware
                debuggers to locate semaphores and has no purpose if a kernel aware
                debugger is not being used.  The call to vQueueAddToRegistry() will be
                removed by the pre-processor if configQUEUE_REGISTRY_SIZE is not
                defined or is defined to be less than 1. */
                vQueueAddToRegistry( ( QueueHandle_t ) xParameters[ 0 ].xSemaphore, "Counting_Sem_1" );
                vQueueAddToRegistry( ( QueueHandle_t ) xParameters[ 1 ].xSemaphore, "Counting_Sem_2" );
        
                /* Create the demo tasks, passing in the semaphore to use as the parameter. */
                xTaskCreateOnCore( prvCountingSemaphoreTask, "CNT1", configMINIMAL_STACK_SIZE, ( void * ) &( xParameters[ 0 ] ), tskIDLE_PRIORITY, NULL, 1);
                xTaskCreateOnCore( prvCountingSemaphoreTask, "CNT2", configMINIMAL_STACK_SIZE, ( void * ) &( xParameters[ 1 ] ), tskIDLE_PRIORITY, NULL, 1);
        }

	vTaskDelete(NULL);
   	return;
}
