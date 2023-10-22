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

static uint32_t ulCounter;
static TaskHandle_t cpu0_task;
static TaskHandle_t cpu1_task;
static void CPU1_vContinuousIncrementTask( void * pvParameters )
{       
unsigned long *pulCounter;
unsigned portBASE_TYPE uxOurPriority;
        
        /* Take a pointer to the shared variable from the parameters passed into
        the task. */
        pulCounter = ( unsigned long * ) pvParameters;
        
        /* Query our priority so we can raise it when exclusive access to the 
        shared variable is required. */
        uxOurPriority = uxTaskPriorityGet( NULL );
        
        for( ;; )
        {
#if 0
        	uxOurPriority = uxTaskPriorityGet( NULL );
		/*SMP_DBG("pri  = %d.\n", uxOurPriority);*/
                /* Raise our priority above the controller task to ensure a context
                switch does not occur while we are accessing this variable. */
                vTaskPrioritySet( NULL, (uxOurPriority + 1)%32);
                        ( *pulCounter )++;              
		SMP_DBG("pri  = %d.\n", uxOurPriority);
        	uxOurPriority = uxTaskPriorityGet( NULL );
		/*SMP_DBG("pri  = %d.\n", uxOurPriority);*/
                vTaskPrioritySet( NULL, uxOurPriority );
        
		taskYIELD();
#else
		SMP_DBG("pre sus.\n");
		vTaskSuspend(NULL);
		SMP_DBG("pos sus.\n");
        	uxOurPriority = uxTaskPriorityGet( NULL );
		SMP_DBG("pri  = %d.\n", uxOurPriority);
#endif
        }
	vTaskDelete(NULL);
   	return;
}  

static void cpu0_suspend_task(void *para)
{
unsigned portBASE_TYPE uxOurPriority;

	while(1)
	{
		uxOurPriority = uxTaskPriorityGet( cpu1_task );
		SMP_DBG("uxOurPriority = %d.\n", uxOurPriority);
		vTaskDelay(100);	
    		vTaskPrioritySet( cpu1_task, (uxOurPriority + 1)%32);
		SMP_DBG("resume cpu1\n");
		vTaskResume(cpu1_task);
	}

	vTaskDelete(NULL);
   	return;
}

void cpu0_app_main(void *para)
{
	portBASE_TYPE ret;

	ret = xTaskCreateOnCore( CPU1_vContinuousIncrementTask, "increment", configMINIMAL_STACK_SIZE, &ulCounter,0 /*mainQUEUE_RECEIVE_TASK_PRIORITY*/, &cpu1_task, 1);
	if(ret != pdPASS)
	{
		SMP_DBG("Create Rx Task Failure.\n");
		soft_break();
		return;
	}

	ret = xTaskCreateOnCore( cpu0_suspend_task, "Resume", configMINIMAL_STACK_SIZE, NULL,configTIMER_TASK_PRIORITY /*mainQUEUE_SEND_TASK_PRIORITY*/, &cpu0_task , 0);
	if(ret != pdPASS)
	{
		SMP_DBG("Create Rx Task Failure.\n");
		soft_break();
		return;
	}
	
	if(!cpu1_task || !cpu0_task)
	{
		soft_break();
	}

	vTaskDelete(NULL);
   	return;
}
