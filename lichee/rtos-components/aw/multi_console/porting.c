#include <stdlib.h>
#include <stdio.h>
#include <FreeRTOS.h>
#include <task.h>
#include <hal_thread.h>
#include <hal_interrupt.h>

#include "multi_console_internal.h"

#define LOCAL_STORAGE_CONSOLE_INDEX (1)

void *cli_port_current_task(void)
{
	return xTaskGetCurrentTaskHandle();
}

void *cli_port_get_thread_data(void *task)
{
	if (!task)
		return NULL;
	return pvTaskGetThreadLocalStoragePointer(task, LOCAL_STORAGE_CONSOLE_INDEX);
}

void *cli_port_set_thread_data(void *task, void *console)
{
	void *last = NULL;

	if (!task)
		return NULL;

	last = pvTaskGetThreadLocalStoragePointer(task, LOCAL_STORAGE_CONSOLE_INDEX);
	vTaskSetThreadLocalStoragePointer(task, LOCAL_STORAGE_CONSOLE_INDEX, console);

	return last;
}

void *cli_port_thread_cteate(const char *name, console_thread_t entry,
				uint32_t stack_size, uint32_t priority, void *param)
{
	return hal_thread_create(entry, param, name, stack_size, priority);
}

int cli_port_thread_destory(void *task)
{
	return hal_thread_stop(task);
}

int cli_port_is_in_irq(void)
{
	return in_interrupt();
}

char *cli_port_get_task_name(void *tcb)
{
	return pcTaskGetName(tcb);
}

extern BaseType_t FreeRTOS_CLIProcessCommand( const char *pcCommandInput, char *pcWriteBuffer, size_t xWriteBufferLen);

int cli_port_parse_string(const char *input, char *output, size_t len)
{
	return FreeRTOS_CLIProcessCommand(input, output, len);
}
