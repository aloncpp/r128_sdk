#include <stdlib.h>
#include <stdio.h>
#include <cli_console.h>
#include <console.h>
#include <port_misc.h>
#include <hal_atomic.h>

#define LOCAL_STORAGE_CONSOLE_INDEX (0)

void *cli_current_task(void)
{
    return xTaskGetCurrentTaskHandle();
}

void *cli_console_thread_entry(void *param)
{
	cli_task_set_console(cli_current_task(), (cli_console *)param);
#ifdef CONFIG_COMPONENT_FINSH_CLI
    void finsh_thread_entry(void *parameter);
    finsh_thread_entry(param);
#else
    void prvUARTCommandConsoleTask( void *pvParameters );
    prvUARTCommandConsoleTask(NULL);
#endif
    return NULL;
}

void *cli_task_get_console(void *task_handle)
{
    if (!task_handle)
    {
        return NULL;
    }
    return pvTaskGetThreadLocalStoragePointer(task_handle, LOCAL_STORAGE_CONSOLE_INDEX);
}

void *cli_task_set_console(void *task_handle, void *console)
{
    void *last_console = NULL;

    if (task_handle)
    {
        last_console = pvTaskGetThreadLocalStoragePointer(task_handle, LOCAL_STORAGE_CONSOLE_INDEX);
        vTaskSetThreadLocalStoragePointer(task_handle, LOCAL_STORAGE_CONSOLE_INDEX, console);

        if (last_console)
        {
            cli_console_remove_task_list_node(last_console, task_handle);
        }

        if (console)
        {
            cli_console_add_task_list_node(console, task_handle);
        }
    }
    return last_console;
}

int cli_console_is_irq_context(void)
{
    return hal_thread_is_in_critical_context();
}

int cli_task_clear_console(void *task_handle)
{
    if (!task_handle)
    {
        return 0;
    }
    vTaskSetThreadLocalStoragePointer(task_handle, LOCAL_STORAGE_CONSOLE_INDEX, NULL);
    return 0;
}

cpu_cpsr_t cli_console_lock(cli_console_spinlock_t *lock)
{
    cpu_cpsr_t flags_cpsr = 0;
    flags_cpsr = hal_spin_lock_irqsave(&lock->lock);
    return flags_cpsr;
}

void cli_console_unlock(cli_console_spinlock_t *lock, cpu_cpsr_t flags_cpsr)
{
    hal_spin_unlock_irqrestore(&lock->lock, flags_cpsr);
}

int cli_console_lock_init(cli_console_spinlock_t *lock)
{
    hal_spin_lock_init(&lock->lock);
    return 0;
}

char *get_task_name(void *tcb)
{
    return pcTaskGetName(tcb);
}

#ifdef CONFIG_MULTI_CONSOLE_DEBUG
int cmd_dump_console(int argc, char *argv)
{
    dump_all_cli_console_info();
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_dump_console, dump_console, Dump Console Debug Information);
#endif

#ifdef CONFIG_MULTI_CONSOLE_REDIRECT_CMD
static void *find_task(char *name)
{
    return xTaskGetHandle(name);
}

static int cmd_redirect(int argc, char **argv)
{
    cli_console *current_console = NULL;
    void *task = NULL;

    if (argc < 2)
    {
        printf("Usage:\n");
        printf("\tredirect all      : redirect all logs to current console\n");
        printf("\tredirect stop     : stop redirect all logs to current console\n");
        printf("\tredirect taskname : redirect the target task logs to current console\n");
        return 0;
    }

    current_console = get_clitask_console();

    if (!strcmp(argv[1], "all"))
    {
        set_global_console(current_console);
        return 0;
    }

    if (!strcmp(argv[1], "stop"))
    {
        set_global_console(NULL);
        return 0;
    }

    task = find_task(argv[1]);

    if (task != NULL)
    {
        cli_task_set_console(task, current_console);
    }
    else
    {
        printf("can not find the task\n");
    }

    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_redirect, redirect, redirect logs to current console);
#endif
