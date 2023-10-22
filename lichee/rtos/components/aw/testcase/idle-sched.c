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

unsigned int mdelay(unsigned int counter);
int cur_cpu_id(void);
void cpu0_mutex_task(void *unused)
{
    while(1)
    {
	printf("%s line %d.\n", __func__, __LINE__);
	vTaskDelay(100);
    }
    vTaskDelete(NULL);
}

void cpu1_prmeet_task0(void *unused)
{
    while(1)
    {
	printf("%s line %d.\n", __func__, __LINE__);
	taskYIELD();
	vTaskDelay(1);
    }
    vTaskDelete(NULL);
}

void cpu1_prmeet_task1(void *unused)
{
    while(1)
    {
	printf("%s line %d\n", __func__, __LINE__);
	taskYIELD();
	vTaskDelay(1);
    }
    vTaskDelete(NULL);
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
	test_case1_mutex_cpu0_taskcreate();

	ret = xTaskCreateOnCore(cpu1_prmeet_task0, (signed portCHAR *) "cpu1_p0", 1024, NULL, configTIMER_TASK_PRIORITY - 1, NULL, 1);
	if (ret != pdPASS) {
		printf("Error creating task, status was %d\n", ret);
		return;
	}

	ret = xTaskCreateOnCore(cpu1_prmeet_task1, (signed portCHAR *) "cpu1_p1", 1024, NULL, configTIMER_TASK_PRIORITY - 1, NULL, 1);
	if (ret != pdPASS) {
		printf("Error creating task, status was %d\n", ret);
		return;
	}

	vTaskDelete(NULL);
}
