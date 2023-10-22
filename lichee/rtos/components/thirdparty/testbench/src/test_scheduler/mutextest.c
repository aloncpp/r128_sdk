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

static SemaphoreHandle_t xSemaphore;
uint32_t g_tickets = 0;
unsigned int mdelay(unsigned int counter);
int cur_cpu_id(void);
void cpu0_mutex_task(void *unused)
{
    while(1)
    {
retry:
	if(xSemaphoreTakeRecursive( xSemaphore, 100) !=  pdPASS)
	{
		printf("%s line %d wait timeout.\n", __func__, __LINE__);
		goto retry;
	}
	g_tickets ++;
	printf("cpu%d tickets %d.\n",cur_cpu_id(), g_tickets);
	mdelay(3000000000);
	xSemaphoreGiveRecursive( xSemaphore);
	vTaskDelay(100);
    }
}

void cpu1_mutex_task(void *unused)
{
    while(1)
    {
retry:
	if(xSemaphoreTakeRecursive( xSemaphore, 100) !=  pdPASS)
	{
		printf("%s line %d wait timeout.\n", __func__, __LINE__);
		goto retry;
	}
	g_tickets ++;
	printf("cpu%d tickets %d.\n", cur_cpu_id(), g_tickets);
	mdelay(3000000000);
	xSemaphoreGiveRecursive( xSemaphore);
	vTaskDelay(100);
    }
}

void test_case1_mutex_cpu0_taskcreate(void)
{
	portBASE_TYPE ret;

	ret = xTaskCreate(cpu0_mutex_task, (signed portCHAR *) "cpu0_mutex_task", 4096, NULL, configTIMER_TASK_PRIORITY - 1, NULL);
	if (ret != pdPASS) {
		printf("Error creating task, status was %d\n", ret);
		return;
	}

	return;
}

void cpu0_app_main(void* para)
{
	portBASE_TYPE ret;

	xSemaphore = xSemaphoreCreateRecursiveMutex();
	configASSERT( uxSemaphoreGetCount( xSemaphore  ) == 1  );
	configASSERT( xSemaphore);

	test_case1_mutex_cpu0_taskcreate();

	ret = xTaskCreateOnCore(cpu1_mutex_task, (signed portCHAR *) "cpu1_mutex_task", 1024, NULL, configTIMER_TASK_PRIORITY - 1, NULL, 1);
	if (ret != pdPASS) {
		printf("Error creating task, status was %d\n", ret);
		return;
	}
	vTaskDelete(NULL);
}
