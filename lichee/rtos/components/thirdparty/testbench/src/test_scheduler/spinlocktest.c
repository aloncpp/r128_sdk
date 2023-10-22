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

volatile freert_spinlock_t usr_lock;
static uint32_t g_usr_counter = 0;
static void cpu1_spin_lock_task(void *para)
{
    	while(1)
	{
	    freert_spin_lock(&usr_lock);
	    g_usr_counter ++;
	    SMP_DBG("counter %d.\n", g_usr_counter);
	    freert_spin_unlock(&usr_lock);
	}
	vTaskDelete(NULL);
   	return;
}

static void cpu0_spin_lock_task(void *para)
{
    	while(1)
	{
	    freert_spin_lock(&usr_lock);
	    g_usr_counter ++;
	    SMP_DBG("counter %d.\n", g_usr_counter);
	    freert_spin_unlock(&usr_lock);
	}

	vTaskDelete(NULL);
   	return;
}

void cpu0_app_main(void)
{
	portBASE_TYPE ret;

	ret = xTaskCreateOnCore( cpu1_spin_lock_task, "cpu1_lock", configMINIMAL_STACK_SIZE, NULL,configTIMER_TASK_PRIORITY , NULL, 1);
	if(ret != pdPASS)
	{
		SMP_DBG("Create Rx Task Failure.\n");
		soft_break();
		return;
	}

	ret = xTaskCreateOnCore( cpu0_spin_lock_task, "cpu0_lock", configMINIMAL_STACK_SIZE, NULL,configTIMER_TASK_PRIORITY,  NULL, 0);
	if(ret != pdPASS)
	{
		SMP_DBG("Create Rx Task Failure.\n");
		soft_break();
		return;
	}
	
	vTaskDelete(NULL);
   	return;
}
