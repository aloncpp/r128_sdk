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
#include "event_groups.h"
#include <math.h>

static SemaphoreHandle_t mutex_test;

void sdelay(uint32_t sec);
void mdelay(uint32_t ms);
//priority: A < B < C
//step 1: Task A running, Task B and C Sleep.
void cpu0_task_A(void *para)
{
 	while(1)
	{
again:
		SMP_DBG("%s, get mutex before, %d.\n", para, uxSemaphoreGetCount( mutex_test  ));
		if(xSemaphoreTakeRecursive( mutex_test,  portMAX_DELAY) !=  pdPASS)
		{
			SMP_DBG("%s, get mutex failure.\n", para);
			goto again;
		}
		SMP_DBG("%s, get mutex success, %d.\n", para, uxSemaphoreGetCount( mutex_test  ));
		sdelay(5);
		xSemaphoreGiveRecursive( mutex_test);
		SMP_DBG("%s, release mutex success.\n", para);

		vTaskDelay(100);
	}

	vTaskDelete(NULL);
   	return;
}

void cpu0_task_B(void *para)
{
 	while(1)
	{
		// yield out for task A get the sem.
    		vTaskDelay(100);
		SMP_DBG("%s, intrude in.\n", para);
		sdelay(1);
		SMP_DBG("%s, intrude out.\n", para);
	}
	
	vTaskDelete(NULL);
   	return;
}

void cpu0_task_C(void *para)
{
 	while(1)
	{
		// yield out for task A get the sem.
    		vTaskDelay(100);
again:
		SMP_DBG("%s, get mutex before, %d.\n", para, uxSemaphoreGetCount( mutex_test  ));
		if(xSemaphoreTakeRecursive( mutex_test,  portMAX_DELAY) !=  pdPASS)
		{
			SMP_DBG("%s, get mutex failure.\n", para);
			goto again;
		}
		SMP_DBG("%s, get mutex success, %d.\n", para, uxSemaphoreGetCount( mutex_test  ));
		sdelay(3);
		xSemaphoreGiveRecursive( mutex_test);

		vTaskDelay(250);
	}

	vTaskDelete(NULL);
   	return;
}

void cpu0_app_main(void)
{
	portBASE_TYPE ret;

	mutex_test = xSemaphoreCreateRecursiveMutex();
	configASSERT( uxSemaphoreGetCount( mutex_test  ) == 1  );
	configASSERT( mutex_test);

	ret = xTaskCreateOnCore( cpu0_task_A, "task_A", 1024, "low",configTIMER_TASK_PRIORITY - 5 , NULL, 1);
	if(ret != pdPASS)
	{
		SMP_DBG("Create Rx Task Failure.\n");
		soft_break();
		return;
	}

	ret = xTaskCreateOnCore( cpu0_task_B, "task_B", 1024, "mid",configTIMER_TASK_PRIORITY -2 , NULL, 1);
	if(ret != pdPASS)
	{
		SMP_DBG("Create Rx Task Failure.\n");
		soft_break();
		return;
	}

	ret = xTaskCreateOnCore( cpu0_task_C, "task_C", 1024, "hih",configTIMER_TASK_PRIORITY , NULL, 1);
	if(ret != pdPASS)
	{
		SMP_DBG("Create Rx Task Failure.\n");
		soft_break();
		return;
	}

	vTaskDelete(NULL);
   	return;
}
