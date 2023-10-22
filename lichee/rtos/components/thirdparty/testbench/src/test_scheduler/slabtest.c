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

freert_spinlock_t usr_lock;
void cpu1_malloc_task0(void *para)
{
    	void* ptr;
 	while(1)
	{
		ptr = pvPortMalloc(1024);     	
		if(ptr == NULL)
		{
			SMP_DBG("malloc failure.\n");
			soft_break();
		}
		else
		{
			vPortFree(ptr);     	
		}
	
		freert_spin_lock(&usr_lock);
		SMP_DBG("slab test task,%s %p.\n",para, ptr);
		freert_spin_unlock(&usr_lock);
		ptr = NULL;
		taskYIELD();
	}

	vTaskDelete(NULL);
   	return;
}

void cpu0_malloc_task0(void *para)
{
    	void *ptr;
 	while(1)
	{
		ptr = pvPortMalloc(1024 * 1024);     	
		if(ptr == NULL)
		{
			SMP_DBG("malloc failure.\n");
			soft_break();
		}
		else
		{
			vPortFree(ptr);     	
		}
	
		freert_spin_lock(&usr_lock);
		SMP_DBG("slab test task,%s %p.\n",para, ptr);
		freert_spin_unlock(&usr_lock);
		ptr = NULL;
		taskYIELD();
	}

	vTaskDelete(NULL);
   	return;
}


void cpu0_app_main(void)
{
	portBASE_TYPE ret;

	ret = xTaskCreateOnCore( cpu1_malloc_task0, "cpu1t0", 1024, "cpu1t0",configTIMER_TASK_PRIORITY , NULL, 1);
	if(ret != pdPASS)
	{
		SMP_DBG("Create Malloc/Free Task Failure.\n");
		soft_break();
		return;
	}

	ret = xTaskCreateOnCore( cpu1_malloc_task0, "cpu1t1", 1024, "cpu1t1",configTIMER_TASK_PRIORITY , NULL, 1);
	if(ret != pdPASS)
	{
		SMP_DBG("Create Malloc/Free Task Failure.\n");
		soft_break();
		return;
	}

	ret = xTaskCreateOnCore( cpu1_malloc_task0, "cpu1t2", 1024, "cpu1t2",configTIMER_TASK_PRIORITY , NULL, 1);
	if(ret != pdPASS)
	{
		SMP_DBG("Create Malloc/Free Task Failure.\n");
		soft_break();
		return;
	}

	ret = xTaskCreateOnCore( cpu1_malloc_task0, "cpu1t3", 1024, "cpu1t3",configTIMER_TASK_PRIORITY , NULL, 1);
	if(ret != pdPASS)
	{
		SMP_DBG("Create Malloc/Free Task Failure.\n");
		soft_break();
		return;
	}

	vTaskSuspendAll();
	ret = xTaskCreateOnCore( cpu0_malloc_task0, "cpu0t0", 1024, "cpu0m0",configTIMER_TASK_PRIORITY,  NULL, 0);
	if(ret != pdPASS)
	{
		SMP_DBG("Create Malloc/Free Task Failure.\n");
		soft_break();
		return;
	}

	ret = xTaskCreateOnCore( cpu0_malloc_task0, "cpu0t1", 1024, "cpu0m1",configTIMER_TASK_PRIORITY,  NULL, 0); 
	if(ret != pdPASS)
	{
		SMP_DBG("Create Malloc/Free Task Failure.\n");
		soft_break();
		return;
	}

	ret = xTaskCreateOnCore( cpu0_malloc_task0, "cpu0t2", 1024, "cpu0m2",configTIMER_TASK_PRIORITY,  NULL, 0); 
	if(ret != pdPASS)
	{
		SMP_DBG("Create Malloc/Free Task Failure.\n");
		soft_break();
		return;
	}

	ret = xTaskCreateOnCore( cpu0_malloc_task0, "cpu0t3", 1024, "cpu0m3",configTIMER_TASK_PRIORITY,  NULL, 0); 
	if(ret != pdPASS)
	{
		SMP_DBG("Create Malloc/Free Task Failure.\n");
		soft_break();
		return;
	}
	xTaskResumeAll();

	vTaskDelete(NULL);
   	return;
}
