#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "start_task.h"

int main(void)
{
	start_task();
	vTaskStartScheduler();
	/* If we got here then scheduler failed. */
	printf( "vTaskStartScheduler FAILED!\n" );
	return 0;
}
