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

static TaskHandle_t cpu0_task;
static TaskHandle_t cpu1_task;

void cpu1_suspend_task(void *para)
{
    	while(1)
	{
		/*vTaskDelay(0xfffffff0);	*/
		SMP_DBG("pre.\n");
		vTaskSuspend(NULL);
		SMP_DBG("post.\n");
	}

	vTaskDelete(NULL);
   	return;
}

void cpu0_suspend_task(void *para)
{

    	while(1)
	{
		vTaskDelay(10);	
		vTaskSuspend(cpu1_task);
		vTaskDelay(100);	
		vTaskResume(cpu1_task);
		SMP_DBG("here.\n");
	}

	vTaskDelete(NULL);
   	return;
}

void cpu0_app_main(void *para)
{
	portBASE_TYPE ret;

	ret = xTaskCreateOnCore( cpu1_suspend_task, "Suspend", configMINIMAL_STACK_SIZE, NULL,configTIMER_TASK_PRIORITY /*mainQUEUE_RECEIVE_TASK_PRIORITY*/, &cpu1_task, 1);
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

	vTaskDelete(NULL);
   	return;
}
