#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>

void vApplicationMallocFailedHook(void)
{
    printf("Malloc failed\n");
    for (;;);
}

void vApplicationStackOverflowHook(xTaskHandle pxTask, char *pcTaskName)
{
    (void) pcTaskName;
    (void) pxTask;

    printf("Stack overflow in task (handle 0x%x, name '%s')\n", pxTask, pcTaskName);
    for (;;);
}

void vApplicationIdleHook(void)
{
    __asm__ volatile("wfi":::"memory", "cc");
}

void vApplicationTickHook(void)
{
}
