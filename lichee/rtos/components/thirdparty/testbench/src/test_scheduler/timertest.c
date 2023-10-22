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

static TimerHandle_t xTimer = NULL;
static QueueHandle_t xQueue = NULL;

#define mainQUEUE_RECEIVE_TASK_PRIORITY         (tskIDLE_PRIORITY + 2)
#define mainQUEUE_SEND_TASK_PRIORITY            (tskIDLE_PRIORITY + 1)
#define mainQUEUE_LENGTH        		(2)
#define mainTASK_SEND_FREQUENCY_MS              (200UL)
#define mainVALUE_SENT_FROM_TASK                (100UL)
#define mainVALUE_SENT_FROM_TIMER               (200UL)

static void prvQueueReceiveTask(void *para)
{
	uint32_t ulReceivedValue;
   	
   	for( ;; )  
   	{       
   		/* Wait until something arrives in the queue - this task will block
   		indefinitely provided INCLUDE_vTaskSuspend is set to 1 in
   		FreeRTOSConfig.h.  It will not use any CPU time while it is in the
   		Blocked state. */              
   		xQueueReceive( xQueue, &ulReceivedValue, portMAX_DELAY );
   		
   		/* To get here something must have been received from the queue, but
   		is it an expected value?  Normally calling printf() from a task is not
   		a good idea.  Here there is lots of stack space and only one task is
   		using console IO so it is ok.  However, note the comments at the top of
   		this file about the risks of making Windows system calls (such as 
   		console output) from a FreeRTOS task. */
   		if( ulReceivedValue == mainVALUE_SENT_FROM_TASK )
   		{  
   			printf( "Message received from task\r\n" );
   		}  
   		else if( ulReceivedValue == mainVALUE_SENT_FROM_TIMER )
   		{  
   			printf( "Message received from software timer\r\n" );
   		}  
   		else
   		{               
   			printf( "Unexpected message\r\n" );
   		}
   	}

	vTaskDelete(NULL);
   	return;
}

static void prvQueueSendTask(void *para)
{
TickType_t xNextWakeTime;
const TickType_t xBlockTime = mainTASK_SEND_FREQUENCY_MS;
const uint32_t ulValueToSend = mainVALUE_SENT_FROM_TASK;
                
        /* Initialise xNextWakeTime - this only needs to be done once. */
        xNextWakeTime = xTaskGetTickCount();
                
        for( ;; )
        {       
                /* Place this task in the blocked state until it is time to run again.
                The block time is specified in ticks, pdMS_TO_TICKS() was used to
                convert a time specified in milliseconds into a time specified in ticks.
                While in the Blocked state this task will not consume any CPU time. */
                vTaskDelayUntil( &xNextWakeTime, xBlockTime );
                
                /* Send to the queue - causing the queue receive task to unblock and
                write to the console.  0 is used as the block time so the send operation                                                                                                       
                will not block - it shouldn't need to block as the queue should always
                have at least one space at this point in the code. */
                xQueueSend( xQueue, &ulValueToSend, 0U );
        } 
	vTaskDelete(NULL);
   	return;
}

static void prvQueueSendTimerCallback( TimerHandle_t xTimerHandle )
{            
const uint32_t ulValueToSend = mainVALUE_SENT_FROM_TIMER;

	/* This is the software timer callback function.  The software timer has a
	period of two seconds and is reset each time a key is pressed.  This
	callback function will execute if the timer expires, which will only happen
	if a key is not pressed for two seconds. */
	                                        
	/* Avoid compiler warnings resulting from the unused parameter. */
	( void ) xTimerHandle;                  
	                                        
	/* Send to the queue - causing the queue receive task to unblock and
	write out a message.  This function is called from the timer/daemon task, so
	must not block.  Hence the block time is set to 0. */
	xQueueSend( xQueue, &ulValueToSend, 0U );
	
	return;
}   

void cpu0_app_main(void *para)
{
	portBASE_TYPE ret;

	/*soft_break();*/
	xQueue = xQueueCreate( mainQUEUE_LENGTH, sizeof( uint32_t ) );
	if(xQueue == NULL)
	{
		SMP_DBG("Create Queue Failure.\n");
		soft_break();
		return;
	}

    	xTimer = xTimerCreate( "Timer", 100, pdTRUE, NULL, prvQueueSendTimerCallback );
	if(xTimer == NULL)
	{
		SMP_DBG("Create Timer Failure.\n");
		soft_break();
		return;
	}
	else
	{
		xTimerStart( xTimer, 0 );	
	}

#if 1
	ret = xTaskCreateOnCore( prvQueueReceiveTask, "Rx", configMINIMAL_STACK_SIZE, NULL,configTIMER_TASK_PRIORITY /*mainQUEUE_RECEIVE_TASK_PRIORITY*/, NULL, 1);
#else
	ret = xTaskCreateOnCore( prvQueueReceiveTask, "Rx", configMINIMAL_STACK_SIZE, NULL,configTIMER_TASK_PRIORITY /*mainQUEUE_RECEIVE_TASK_PRIORITY*/, NULL, 0);
#endif
	if(ret != pdPASS)
	{
		SMP_DBG("Create Rx Task Failure.\n");
		soft_break();
		return;
	}

#if 0
	ret = xTaskCreateOnCore( prvQueueSendTask, "Tx", configMINIMAL_STACK_SIZE, NULL,configTIMER_TASK_PRIORITY /*mainQUEUE_SEND_TASK_PRIORITY*/, NULL , 1);
#else
	ret = xTaskCreateOnCore( prvQueueSendTask, "Tx", configMINIMAL_STACK_SIZE, NULL,configTIMER_TASK_PRIORITY /*mainQUEUE_SEND_TASK_PRIORITY*/, NULL , 0);
#endif
	if(ret != pdPASS)
	{
		SMP_DBG("Create Rx Task Failure.\n");
		soft_break();
		return;
	}


	vTaskDelete(NULL);
   	return;
}
