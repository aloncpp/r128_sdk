#ifndef _MULTI_CONSOLE_PORTING_H
#define _MULTI_CONSOLE_PORTING_H

#include <stdlib.h>
#include <stdio.h>

void *cli_port_current_task(void);
void *cli_port_get_thread_data(void *task);
void *cli_port_set_thread_data(void *task, void *data);
void *cli_port_thread_cteate(const char *name, console_thread_t entry,
				uint32_t stack_size, uint32_t priority, void *param);
int cli_port_thread_destory(void *task);
int cli_port_is_in_irq(void);
char *cli_port_get_task_name(void *tcb);

int cli_port_parse_string(const char *input, char *output, size_t len);

#endif
