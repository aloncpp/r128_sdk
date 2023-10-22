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

static uint64_t share_resource;
static freert_rwlock_t user_lock;

unsigned int mdelay(unsigned int counter);
void cpu1_read_task0(void *para)
{
 	while(1)
	{
    		freert_read_lock(&user_lock);
		SMP_DBG("%s, share_resource = %lld.\n", para, share_resource);
    		freert_read_unlock(&user_lock);
		/* keek not sync with timer. */
		mdelay(1000);
	}
	vTaskDelete(NULL);
   	return;
}

void cpu0_write_task0(void *para)
{
 	while(1)
	{
    		freert_write_lock(&user_lock);
		share_resource ++;
		SMP_DBG("%s, share_resource = %lld.\n", para, share_resource);
    		freert_write_unlock(&user_lock);
		mdelay(1000);
	}
	vTaskDelete(NULL);
   	return;
}

void cpu0_app_main(void)
{
	portBASE_TYPE ret;

	ret = xTaskCreateOnCore( cpu1_read_task0, "cpu1t0", 1024, "cpu1t0",configTIMER_TASK_PRIORITY , NULL, 1);
	if(ret != pdPASS)
	{
		SMP_DBG("Create Rx Task Failure.\n");
		soft_break();
		return;
	}

	ret = xTaskCreateOnCore( cpu1_read_task0, "cpu1t1", 1024, "cpu1t1",configTIMER_TASK_PRIORITY , NULL, 1);
	if(ret != pdPASS)
	{
		SMP_DBG("Create Rx Task Failure.\n");
		soft_break();
		return;
	}

	ret = xTaskCreateOnCore( cpu1_read_task0, "cpu1t2", 1024, "cpu1t2",configTIMER_TASK_PRIORITY , NULL, 1);
	if(ret != pdPASS)
	{
		SMP_DBG("Create Rx Task Failure.\n");
		soft_break();
		return;
	}

	vTaskSuspendAll();
	ret = xTaskCreateOnCore( cpu0_write_task0, "cpu0t0", 1024, "cpu0t0",configTIMER_TASK_PRIORITY,  NULL, 0); if(ret != pdPASS)
	{
		SMP_DBG("Create Rx Task Failure.\n");
		soft_break();
		return;
	}

	ret = xTaskCreateOnCore( cpu0_write_task0, "cpu0t0", 1024, "cpu0t1",configTIMER_TASK_PRIORITY,  NULL, 0); if(ret != pdPASS)
	{
		SMP_DBG("Create Rx Task Failure.\n");
		soft_break();
		return;
	}

	ret = xTaskCreateOnCore( cpu0_write_task0, "cpu0t0", 1024, "cpu0t2",configTIMER_TASK_PRIORITY,  NULL, 0); if(ret != pdPASS)
	{
		SMP_DBG("Create Rx Task Failure.\n");
		soft_break();
		return;
	}
	xTaskResumeAll();

	vTaskDelete(NULL);
   	return;
}
