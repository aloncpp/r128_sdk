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

void migrate_task1(void *para)
{
 	while(1)
	{
		SMP_DBG("\n");
		xTaskYieldToCore(0);
		SMP_DBG("\n");
		vTaskDelay(1000);
		SMP_DBG("\n");
		xTaskYieldToCore(1);
		SMP_DBG("\n");
		vTaskDelay(1000);
		SMP_DBG("\n");
	}

	vTaskDelete(NULL);
   	return;
}

void migrate_task0(void *para)
{
 	while(1)
	{
		SMP_DBG("\n");
		xTaskYieldToCore(1);
		SMP_DBG("\n");
		vTaskDelay(1000);
		SMP_DBG("\n");
		xTaskYieldToCore(0);
		SMP_DBG("\n");
		vTaskDelay(1000);
		SMP_DBG("\n");
	}

	vTaskDelete(NULL);
   	return;
}

void cpu0_app_main(void)
{
	portBASE_TYPE ret;
	int i = 0;

	for (i = 0; i < 5; i ++)
	{
	    ret = xTaskCreateOnCore( migrate_task0, "migrate0", 1024, "mig0",configTIMER_TASK_PRIORITY,  NULL, 0);
	    if(ret != pdPASS)
	    {
		    SMP_DBG("Create IPI Task Failure.\n");
		    soft_break();
		    return;
	    }

	    ret = xTaskCreateOnCore( migrate_task0, "migrate0", 1024, "mig1",configTIMER_TASK_PRIORITY,  NULL, 0);
	    if(ret != pdPASS)
	    {
		    SMP_DBG("Create IPI Task Failure.\n");
		    soft_break();
		    return;
	    }

	    ret = xTaskCreateOnCore( migrate_task1, "migrate1", 1024, "mig2",configTIMER_TASK_PRIORITY,  NULL, 1);
	    if(ret != pdPASS)
	    {
		    SMP_DBG("Create IPI Task Failure.\n");
		    soft_break();
		    return;
	    }

	    ret = xTaskCreateOnCore( migrate_task1, "migrate1", 1024, "mig3",configTIMER_TASK_PRIORITY,  NULL, 1);
	    if(ret != pdPASS)
	    {
		    SMP_DBG("Create IPI Task Failure.\n");
		    soft_break();
		    return;
	    }
	}
	
	vTaskDelete(NULL);
   	return;
}
