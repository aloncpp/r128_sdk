#include <stdlib.h>
#include <stdio.h>
#include "console.h"
#include <cli_console.h>
#include <FreeRTOS.h>
#include <FreeRTOS_CLI.h>
#include <task.h>

void run_xip_test(void *param)
{
	const portTickType xDelay = pdMS_TO_TICKS(1000);
	for(;;){
		printf("%s, %d\n", __func__, __LINE__);
		vTaskDelay( xDelay );
	}
}

int cmd_xip_thread(int argc, char ** argv)
{
    portBASE_TYPE ret;
    ret = xTaskCreate(run_xip_test, (signed portCHAR *) "xip-thread", 1024, NULL, 31, NULL);
    if (ret != pdPASS)
    {
        printf("Error creating task, status was %d\n", ret);
    }
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_xip_thread, xip_thread, List all registered commands);

int cmd_run_xip(int argc, char ** argv)
{
    printf("%s, %d\n", __func__, __LINE__);
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_run_xip, run_xip, List all registered commands);
