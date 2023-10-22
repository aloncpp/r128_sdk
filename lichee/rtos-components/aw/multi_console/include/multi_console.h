#ifndef AW_MULTI_CONSOLE_H
#define AW_MULTI_CONSOLE_H

#include <aw_list.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include <hal_atomic.h>

#define CLI_CONSOLE_MAX_NAME_LEN					(32)
#define CLI_CONSOLE_MAX_INPUT_SIZE					(256)
#define CLI_CONSOLE_MAX_OUTPUT_SIZE					(configCOMMAND_INT_MAX_OUTPUT_SIZE)

struct _wraper_task;
struct _device_ops;
struct _cli_console;

typedef struct _wraper_task wraper_task;
typedef struct _device_ops cli_dev_ops;
typedef struct _cli_console cli_console;

void cli_console_set_task_console(void *current_console, void *task);
void cli_console_clear_task_console(cli_console *console, void *task);

int cli_console_read(cli_console *console, void *buf, size_t nbytes);
int cli_console_read_timeout(cli_console *console, void *buf, size_t nbytes, uint32_t timeout);
int cli_console_write(cli_console *console, const void *buf, size_t nbytes);

cli_console *get_current_console(void);
cli_console *set_current_console(cli_console *console);
cli_console *get_clitask_console(void);

cli_console *get_default_console(void);
cli_console *set_default_console(void *console);

cli_console *get_global_console(void);
cli_console *set_global_console(void *console);

cli_console *cli_console_create(cli_dev_ops *dev_ops, const char *name);
int cli_console_destory(cli_console * console);
void cli_console_current_task_destory(void);
char *cli_console_get_name(cli_console *console);

int cli_console_check_invalid(cli_console *console);
int cli_console_task_check_exit(void);
void cli_console_set_exit(cli_console *console);
void check_console_task_exit(void);

int multiple_console_init(void);

#endif
