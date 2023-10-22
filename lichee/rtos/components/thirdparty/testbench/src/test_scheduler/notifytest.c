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

static TaskHandle_t cpu0_task;
static TaskHandle_t cpu1_task;

/*#define portMAX_DELAY 200*/
static void cpu1_notify_task(void* para)
{
    while(1)
    {
	ulTaskNotifyTake( pdTRUE, portMAX_DELAY  );
	printf("cpu%d.%s line %d,take success.\n" ,cur_cpu_id(),__func__, __LINE__);
	vTaskDelay(100);
	xTaskNotifyGive(cpu0_task);
	printf("cpu%d.%s line %d,give.\n" ,cur_cpu_id(),__func__, __LINE__);
    }

}

unsigned int mdelay(unsigned int counter);
static void cpu0_notify_task(void* para)
{
    uint32_t sr_level;
    while(1)
    {
	//xport_cpu_send_ipi(IPI_SCHEDU);
	vTaskDelay(100);
	xTaskNotifyGive(cpu1_task);
	printf("cpu%d.%s line %d, give.\n" ,cur_cpu_id(),__func__, __LINE__);
	ulTaskNotifyTake( pdTRUE, portMAX_DELAY  );
	printf("cpu%d.%s line %d, take success.\n" ,cur_cpu_id(),__func__, __LINE__);
    }
}

void cpu0_app_main(void* para)
{
	unsigned char msg;
	portBASE_TYPE ret;

	ret = xTaskCreateOnCore(cpu1_notify_task, (signed portCHAR *) "cpu1_notify_task", 1024, NULL, configTIMER_TASK_PRIORITY - 1, &cpu1_task, 1);
	if (ret != pdPASS) {
		printf("error creating task, status was %d\n", ret);
		while(1);
	}

	ret = xTaskCreateOnCore(cpu0_notify_task, (signed portCHAR *) "cpu0_notify_task", 1024, NULL, configTIMER_TASK_PRIORITY - 1, &cpu0_task, 0);
	if (ret != pdPASS) {
		printf("error creating task, status was %d\n", ret);
		while(1);
	}

	vTaskDelete(NULL);
   	return;
}

