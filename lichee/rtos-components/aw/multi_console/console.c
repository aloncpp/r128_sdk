#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <hal_uart.h>
#include <hal_mem.h>
#include <console.h>

#include "multi_console_internal.h"
#include "porting.h"
#include "shell.h"

#define CLI_CONSOLE_DEBUG_BUF_SIZE				512

static LIST_HEAD(gCliConsolelist);

static cli_console *default_console = NULL;
static cli_console *global_console = NULL;

static hal_spinlock_t console_lock;


/*
 * dump target console information
 * @console: target console
 * */
static void dump_cli_console_info(cli_console *console)
{
	wraper_task *pos, *q;
	int i = 0;

	if (console == NULL)
		return;

	printf("name : [%s]\n", console->name);

	list_for_each_entry_safe(pos, q, &console->task_list, node) {
		printf("task[%d] : %s\n", i, cli_port_get_task_name(pos->task));
		i++;
	}
}

/*
 * dump all cli console information
 */
int dump_all_cli_console_info(void)
{
	cli_console *pos, *q;

	list_for_each_entry_safe(pos, q, &gCliConsolelist, i_list)
		dump_cli_console_info(pos);

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(dump_all_cli_console_info, console_dump, dum all cli console info);

int register_cli_console(cli_console *console)
{
	unsigned long flags_cpsr;

	if (console == NULL)
		return -EINVAL;

	INIT_LIST_HEAD(&console->i_list);
	flags_cpsr = hal_spin_lock_irqsave(&console_lock);
	list_add(&console->i_list, &gCliConsolelist);
	hal_spin_unlock_irqrestore(&console_lock, flags_cpsr);

	return 0;
}

int unregister_cli_console(cli_console *console)
{
	unsigned long flags_cpsr;

	if (console == NULL)
		return -EINVAL;

	flags_cpsr = hal_spin_lock_irqsave(&console_lock);
	list_del(&console->i_list);
	hal_spin_unlock_irqrestore(&console_lock, flags_cpsr);

	return 0;
}

#if 0
static cli_console *lookup_cli_console(const char *name)
{
	cli_console *pos, *q;
	unsigned long flags_cpsr;

	if (name == NULL)
		return NULL;

	flags_cpsr = hal_spin_lock_irqsave(&console_lock);
	list_for_each_entry_safe(pos, q, &gCliConsolelist, i_list) {
		if (!strcmp(name, pos->name)) {
			hal_spin_unlock_irqrestore(&console_lock, flags_cpsr);
			return pos;
		}
	}
	hal_spin_unlock_irqrestore(&console_lock, flags_cpsr);

	return NULL;
}
#endif

static int cli_console_check_is_in_list(cli_console *console)
{
	cli_console *pos, *q;
	int ret = 0;
	unsigned long flags_cpsr;

	if (console == NULL)
		return ret;

	flags_cpsr = hal_spin_lock_irqsave(&console_lock);
	list_for_each_entry_safe(pos, q, &gCliConsolelist, i_list) {
		if (console == pos)
			ret = 1;
	}
	hal_spin_unlock_irqrestore(&console_lock, flags_cpsr);

	return ret;
}

int get_alive_console_num(void)
{
	cli_console *pos, *q;
	int alive_console_n = 0;
	unsigned long flags_cpsr;

	flags_cpsr = hal_spin_lock_irqsave(&console_lock);
	list_for_each_entry_safe(pos, q, &gCliConsolelist, i_list) {
		if (pos->alive)
			alive_console_n++;
	}
	hal_spin_unlock_irqrestore(&console_lock, flags_cpsr);

	return alive_console_n;
}

cli_console *get_default_console(void)
{
	unsigned long flags_cpsr;
	cli_console *tmp;

	if (!default_console)
		return NULL;

	flags_cpsr = hal_spin_lock_irqsave(&console_lock);
	if (default_console->exit || !default_console->init)
		/* TODO: return virtual null console */
		tmp = NULL;
	else
		tmp = default_console;
	hal_spin_unlock_irqrestore(&console_lock, flags_cpsr);

	return tmp;
}

cli_console *set_default_console(void *console)
{
	unsigned long flags_cpsr;
	cli_console *last_console;

	flags_cpsr = hal_spin_lock_irqsave(&console_lock);
	last_console = default_console;
	default_console = console;
	hal_spin_unlock_irqrestore(&console_lock, flags_cpsr);

	return last_console;
}

cli_console *get_global_console(void)
{
	unsigned long flags_cpsr;
	cli_console *tmp;

	flags_cpsr = hal_spin_lock_irqsave(&console_lock);
	tmp = global_console;
	hal_spin_unlock_irqrestore(&console_lock, flags_cpsr);

	return tmp;
}

cli_console *set_global_console(void *console)
{
	unsigned long flags_cpsr;
	cli_console *last_console;

	flags_cpsr = hal_spin_lock_irqsave(&console_lock);
	last_console = global_console;
	global_console = console;
	hal_spin_unlock_irqrestore(&console_lock, flags_cpsr);

	return last_console;
}

cli_console *get_current_console(void)
{
	cli_console *console = NULL;

	console = get_global_console();
	if (console)
		return console;

	console = cli_port_get_thread_data(cli_port_current_task());
	if (console && !console->exit && console->init)
		return console;

	return get_default_console();
}

cli_console *set_current_console(cli_console *console)
{
	cli_console *last_console = NULL;
	void *task = cli_port_current_task();

	last_console = cli_port_get_thread_data(task);
	if (last_console)
		cli_console_clear_task_console(last_console, task);

	cli_console_set_task_console(console, task);

	if (last_console && !last_console->exit && last_console->init)
		return last_console;

	return get_default_console();
}

/*
 * get real console from cli task
 * @return cli task console
 * */
cli_console *get_clitask_console(void)
{
	return cli_port_get_thread_data(cli_port_current_task());
}

void cli_console_set_task_console(void *current_console, void *task)
{
	cli_console *console = (cli_console *)(current_console);
	wraper_task *wraper_task_node;
	unsigned long flags_cpsr;

	if (!console || !task)
	    return;

	wraper_task_node = hal_malloc(sizeof(wraper_task));
	if (!wraper_task_node)
		return;

	memset(wraper_task_node, 0, sizeof(wraper_task));

	flags_cpsr = hal_spin_lock_irqsave(&console->lock);
	wraper_task_node->task = task;
	INIT_LIST_HEAD(&wraper_task_node->node);
	list_add(&wraper_task_node->node, &console->task_list);
	hal_spin_unlock_irqrestore(&console->lock, flags_cpsr);

	cli_port_set_thread_data(task, console);
}

void cli_console_clear_task_console(cli_console *console, void *task)
{
	wraper_task *pos, *q;
	unsigned long flags_cpsr;

	if (!console)
		return;

	flags_cpsr = hal_spin_lock_irqsave(&console->lock);
	list_for_each_entry_safe(pos, q, &console->task_list, node) {
		if (pos && pos->task == task) {
			cli_port_set_thread_data(task, get_default_console());
			list_del(&pos->node);
			free(pos);
			break;
		}
	}
	hal_spin_unlock_irqrestore(&console->lock, flags_cpsr);
}

static void cli_console_clear_console(cli_console *console)
{
	wraper_task *pos, *q;
	unsigned long flags_cpsr;

	if (console == NULL)
		return;

	flags_cpsr = hal_spin_lock_irqsave(&console->lock);
	list_for_each_entry_safe(pos, q, &console->task_list, node) {
		if (pos && pos->task) {
			cli_port_set_thread_data(pos->task, get_default_console());
			list_del(&pos->node);
			free(pos);
		}
	}
	hal_spin_unlock_irqrestore(&console->lock, flags_cpsr);
}

int cli_console_read(cli_console *console, void *buf, size_t nbytes)
{
	int rbytes = 0;

	if (!cli_console_check_is_in_list(console))
		console = get_current_console();

	if (console == NULL)
		console = get_current_console();

	if (console->exit)
		return 0;

	if (console && console->dev_ops->read)
		return rbytes = console->dev_ops->read(buf, nbytes, console->dev_ops->priv, -1);

	return 0;
}

int cli_console_read_timeout(cli_console *console, void *buf, size_t nbytes, uint32_t timeout)
{
	int rbytes = 0;

	if (console && !cli_console_check_is_in_list(console))
		console = get_current_console();

	if (console == NULL)
		console = get_current_console();

	if (console->exit)
		return 0;

	if (!nbytes)
		return 0;

	if (console && console->dev_ops->read)
		return rbytes = console->dev_ops->read(buf, nbytes, console->dev_ops->priv, timeout);

	return -EINVAL;
}

int cli_console_write(cli_console *console, const void *buf, size_t nbytes)
{
	int wbytes = 0;

	if (!console)
		console = get_current_console();
	if (!console)
		console = get_default_console();

	if (!console)
		return 0;

	if (console->exit)
		return 0;

	if (!nbytes)
		return 0;

	if (console && console->dev_ops->write)
		return wbytes = console->dev_ops->write(buf, nbytes, console->dev_ops->priv);

	return -EINVAL;
}

static int cli_console_init(cli_console *console)
{
	int ret = 0;

	if (console && console->init)
		return -EACCES;

	INIT_LIST_HEAD(&console->task_list);
	register_cli_console(console);
	hal_spin_lock_init(&console->lock);

	if (console->dev_ops && console->dev_ops->init) {
		ret = console->dev_ops->init(console->dev_ops->priv);
		if (!ret) {
			console->exit = 0;
			console->init = 1;
		}
	}

	return ret;
}

static int cli_console_deinit(cli_console *console)
{
	int ret = 0;
	unsigned long flags_cpsr;

	/* set default console */
	if (global_console == console)
		/* TODO: set global_console to virtual null console */
		set_global_console(NULL);

	if (default_console == console)
		/* TODO: set global_console to virtual null console */
		set_global_console(NULL);

	if (console && !console->init)
		return -EACCES;

	flags_cpsr = hal_spin_lock_irqsave(&console->lock);
	console->exit = 1;
	hal_spin_unlock_irqrestore(&console->lock, flags_cpsr);

	if (console->dev_ops && console->dev_ops->deinit) {
		ret = console->dev_ops->deinit(console->dev_ops->priv);
		if (ret == 0) {
			console->exit = 1;
			console->init = 0;
		}
	}
	hal_spin_lock_deinit(&console->lock);

	unregister_cli_console(console);

	return ret;
}

/* create a cli console task
 * @param console: the console which is attached t the new task
 * @param stack_size: the stack size of the new cli task
 * @param priority: the priority of the new cli task
 * */
static int cli_console_task_create(cli_console *console, uint32_t stack_size, uint32_t priority)
{
	if (!cli_console_check_invalid(console))
		return -EINVAL;

	console->alive = 1;

	console->shell_task = cli_port_thread_cteate(cli_console_get_name(console),
					shell_thread_entry, stack_size, priority, console);
	if (!console->shell_task)
		return -EFAULT;

	return 0;
}

/*
 * destory cli console task
 * @param console: cli_console need to be destoryed
 * @return 0
 * */
static int cli_console_task_destory(void *task)
{
	cli_port_thread_destory(task);
	return 0;
}

/*
 * create new cli_console
 * @param dev_ops: the device console is attached to the new cli console
 * @param name: the name of new cli console
 */
cli_console *cli_console_create(cli_dev_ops *dev_ops, const char *name)
{
	cli_console *console;
	int ret = 0;

	console_debug("create console %s\r\n", name);

	if (!dev_ops)
		return NULL;

	console = hal_malloc(sizeof(*console));
	if (!console)
		return NULL;
	memset(console, 0, sizeof(*console));

	console->init = 0;
	console->exit = 0;
	console->dev_ops = dev_ops;
	memcpy(console->name, name, CLI_CONSOLE_MAX_NAME_LEN - 1 > strlen(name) ? strlen(name) : CLI_CONSOLE_MAX_NAME_LEN - 1);

	console_debug("init console %s\r\n", name);
	ret = cli_console_init(console);
	if (ret)
		goto _err_init;

	if (console->dev_ops->task) {
		console_debug("create console %s thread\r\n", name);
		ret = cli_console_task_create(console, CLI_CONSOLE_DEFAULT_STACK_SIZE,
						CLI_CONSOLE_DEFAULT_PRIORITY);
		if (ret)
			goto _err_creat_task;
	}

	return console;

_err_init:
	console_debug("err: init console %s\r\n", name);
	hal_free(console);
_err_creat_task:
	console_debug("err: create console task %s\r\n", name);
	cli_console_deinit(console);

	return NULL;
}

int cli_console_destory(cli_console *console)
{
	void *task = NULL;

	if (!console || !console->alive)
		return -EINVAL;

	if (console->dev_ops->task)
		task = console->shell_task;

	cli_console_deinit(console);
	cli_console_clear_console(console);

	console->exit = 1;
	console->alive = 0;

	hal_free(console);

	if (task)
		cli_console_task_destory(task);

	return 0;
}

/* get target console name
 * @param console: the target console
 * @return the name of target console
 * */
char *cli_console_get_name(cli_console *console)
{
	char *name = NULL;

	if (console)
		name = console->name;
	if (!name && default_console)
		name = default_console->name;

	return name;
}


int cli_console_check_invalid(cli_console *console)
{
	if (console == NULL)
		return 0;
	return 1;
}

void cli_console_current_task_destory(void)
{
	cli_console *console = get_current_console();
	void *task = NULL;

	if (!console || !console->alive)
		return;

	if (console->dev_ops->task)
		task = console->shell_task;

	cli_console_deinit(console);
	cli_console_clear_console(console);

	console->exit = 1;
	console->alive = 0;
	hal_free(console);

	if (task)
		cli_console_task_destory(task);
}

/*
 * check cli_console exit
 * @return 0/1, 0:the task should be alive; 1:the task should be deleted
 * */
int cli_console_task_check_exit(void)
{
	int exit = 0;
	cli_console *console;

	console = cli_port_get_thread_data(cli_port_current_task());

	if (console)
		exit = console->exit;
	else
		exit = 0;

	return exit;
}

void cli_console_set_exit(cli_console *console)
{
	if (console)
		console->exit = 1;
}

void check_console_task_exit(void)
{
	cli_console *console = get_clitask_console();
	if (console && console->exit)
		cli_port_thread_destory(console->shell_task);
}

int console_core_init(void)
{
	return hal_spin_lock_init(&console_lock);
}
