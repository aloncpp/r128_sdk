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


extern uint32_t xport_interrupt_disable(void);
extern void xport_interrupt_enable(uint32_t);

uint32_t freert_cpus_lock(void);
void freert_cpus_unlock(uint32_t cpu_sr);
void test_case1_irq_disable_enable(void)
{
    uint32_t sr_level;
    uint32_t count1, count2;
    
    sr_level  = xport_interrupt_disable();
    count1 = xTaskGetTickCount(); 
    
    sdelay(10);

    count2 = xTaskGetTickCount(); 
    xport_interrupt_enable(sr_level);
	
    vTaskDelay(configTICK_RATE_HZ);

    uint32_t cpusr;
    cpusr = freert_cpus_lock();
    if(count1 != count2)
	SMP_DBG("failure\n");
    else
	SMP_DBG("pass\n");
    freert_cpus_unlock(cpusr);
}

void test_case2_schedule_in_irq(void)
{
    uint32_t sr_level;
    uint32_t count1, count2;
    
    sr_level  = xport_interrupt_disable();
    vTaskDelay(configTICK_RATE_HZ);
    xport_interrupt_enable(sr_level);

    uint32_t cpusr;
    cpusr = freert_cpus_lock();
    SMP_DBG("pass\n");
    freert_cpus_unlock(cpusr);
}

void cpu0_task(void *para)
{
    	sdelay(10);
    	while(1)
	{
	    test_case1_irq_disable_enable();
	}

	vTaskDelete(NULL);
   	return;
}

void cpu1_task(void *para)
{
    	sdelay(10);
    	while(1)
	{
		test_case2_schedule_in_irq();
	}

	vTaskDelete(NULL);
   	return;
}

void cpu0_app_main(void *para)
{
	portBASE_TYPE ret;

	ret = xTaskCreateOnCore( cpu1_task, "cpu1_lock", configMINIMAL_STACK_SIZE, NULL,configTIMER_TASK_PRIORITY , NULL, 1);
	if(ret != pdPASS)
	{
		SMP_DBG("Create Rx Task Failure.\n");
		soft_break();
		return;
	}

	ret = xTaskCreateOnCore( cpu0_task, "cpu0_lock", configMINIMAL_STACK_SIZE, NULL,configTIMER_TASK_PRIORITY,  NULL, 0);
	if(ret != pdPASS)
	{
		SMP_DBG("Create Rx Task Failure.\n");
		soft_break();
		return;
	}
	
	vTaskDelete(NULL);
   	return;
}
