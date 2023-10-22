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
#include <math.h>

void cpu1_lock_task0(void *para)
{
	vTaskDelay(100);
    	uint32_t cpu_sr, cpu_sr1;
 	while(1)
	{
		taskENTER_CRITICAL(cpu_sr);
		taskENTER_CRITICAL(cpu_sr1);
		SMP_DBG("para = %s.\n",  para);
		vTaskDelay(100);
		taskEXIT_CRITICAL(cpu_sr1);
		taskEXIT_CRITICAL(cpu_sr);
		vTaskDelay(300);
	}

	vTaskDelete(NULL);
   	return;
}

void cpu0_lock_task0(void *para)
{
    	uint32_t cpu_sr;
 	while(1)
	{
		taskENTER_CRITICAL(cpu_sr);
		SMP_DBG("para = %s.\n",  para);
		taskEXIT_CRITICAL(cpu_sr);
		vTaskDelay(100);
	}

	vTaskDelete(NULL);
   	return;
}


void cpu0_app_main(void)
{
	portBASE_TYPE ret;

	ret = xTaskCreateOnCore( cpu1_lock_task0, "cpu1t0", 1024, "cpu1t0",configTIMER_TASK_PRIORITY , NULL, 1);
	if(ret != pdPASS)
	{
		SMP_DBG("Create Malloc/Free Task Failure.\n");
		soft_break();
		return;
	}

	ret = xTaskCreateOnCore( cpu1_lock_task0, "cpu1t1", 1024, "cpu1t1",configTIMER_TASK_PRIORITY , NULL, 1);
	if(ret != pdPASS)
	{
		SMP_DBG("Create Malloc/Free Task Failure.\n");
		soft_break();
		return;
	}

	ret = xTaskCreateOnCore( cpu1_lock_task0, "cpu1t2", 1024, "cpu1t2",configTIMER_TASK_PRIORITY , NULL, 1);
	if(ret != pdPASS)
	{
		SMP_DBG("Create Malloc/Free Task Failure.\n");
		soft_break();
		return;
	}

	ret = xTaskCreateOnCore( cpu1_lock_task0, "cpu1t3", 1024, "cpu1t3",configTIMER_TASK_PRIORITY , NULL, 1);
	if(ret != pdPASS)
	{
		SMP_DBG("Create Malloc/Free Task Failure.\n");
		soft_break();
		return;
	}

	ret = xTaskCreateOnCore( cpu0_lock_task0, "cpu0t0", 1024, "cpu0t0",configTIMER_TASK_PRIORITY , NULL, 0);
	if(ret != pdPASS)
	{
		SMP_DBG("Create Malloc/Free Task Failure.\n");
		soft_break();
		return;
	}

	vTaskDelete(NULL);
}
