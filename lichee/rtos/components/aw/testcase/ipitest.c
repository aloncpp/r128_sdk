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

void send_ipi_interrupt(int ipino, unsigned int targetlist, unsigned int filter, unsigned char ns);
unsigned int mdelay(unsigned int counter);
void cpu0_ipi_task0(void *para)
{
 	while(1)
	{
		SMP_DBG("%s, sending ipi.\n", para);
		send_ipi_interrupt(IPI_SCHEDU, 2, 0x0, 1);
		vTaskDelay(100);
	}

	vTaskDelete(NULL);
   	return;
}

void cpu1_ipi_task0(void *para)
{
 	while(1)
	{
		SMP_DBG("%s, sending ipi.\n", para);
		send_ipi_interrupt(IPI_SCHEDU, 1, 0x0, 1);
		vTaskDelay(100);
	}

	vTaskDelete(NULL);
   	return;
}

void cpu0_app_main(void)
{
	portBASE_TYPE ret;

	ret = xTaskCreateOnCore( cpu0_ipi_task0, "cpu0ipi", 1024, "ipi",configTIMER_TASK_PRIORITY,  NULL, 0);
	if(ret != pdPASS)
	{
		SMP_DBG("Create IPI Task Failure.\n");
		soft_break();
		return;
	}

	ret = xTaskCreateOnCore( cpu1_ipi_task0, "cpu1ipi", 1024, "ipi",configTIMER_TASK_PRIORITY,  NULL, 1);
	if(ret != pdPASS)
	{
		SMP_DBG("Create IPI Task Failure.\n");
		soft_break();
		return;
	}
	
	
	vTaskDelete(NULL);
   	return;
}
