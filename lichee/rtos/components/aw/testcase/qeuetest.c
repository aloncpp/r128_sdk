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

static xQueueHandle msg_queue = NULL;

static void cpu1_send_task(void *p)
{
    int                 cnt;
    unsigned char       msg;

    msg = 0; 

    while(1){
	msg ++;
	printf("%s line %d. sending message %d.\n", __func__, __LINE__, msg);
	xQueueSend(msg_queue, &msg, portMAX_DELAY);
	vTaskDelay(100);
    } 	
}

void cpu0_app_main(void* para)
{
	unsigned char msg;
	portBASE_TYPE ret;

	msg_queue = xQueueCreate(5, sizeof(unsigned char));
	if(msg_queue == NULL)
	{
		printf("fatal error, create queue failure.\n");
		while(1);
	}

	ret = xTaskCreateOnCore(cpu1_send_task, (signed portCHAR *) "cpu1_send_task", 1024, NULL, configTIMER_TASK_PRIORITY - 1, NULL, 1);
	if (ret != pdPASS) {
		printf("error creating task, status was %d\n", ret);
		while(1);
	}

	while(1){
		ret = xQueueReceive(msg_queue, &msg, portMAX_DELAY);
		if(ret == pdPASS){ 
			printf("%s line %d, receving msg = %d.\n" ,__func__, __LINE__, msg);
		} else {
			printf("%s line %d, fatal error.\n" ,__func__, __LINE__);
			while(1);
		}  
		vTaskDelay(100);
	}


	vTaskDelete(NULL);
   	return;
}
