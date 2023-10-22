#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>

#include <backtrace.h>

int cur_cpu_id(void);

void vAssertCalled(const char *pcFile, unsigned long ulLine)
{
#ifdef CONFIG_DEBUG_BACKTRACE
    backtrace(NULL, NULL, 0, 0, printf);
#endif
    printf("cpu%d: pcFile %s, uLine %d, task handle %p.\n", cur_cpu_id(), \
        pcFile, ulLine, xTaskGetCurrentTaskHandle());
    while (1);
}
