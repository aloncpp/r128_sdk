#include <stdint.h>
#include <spinlock.h>

#include "multi_console_internal.h"

static cli_dev_ops null_console_ops = {
	.write = NULL,
	.read = NULL,
	.init = NULL,
	.deinit = NULL,
	.priv = NULL,
	.prefix = NULL,
	.echo = 0,
	.task = 0,
};
static cli_console *console;

int null_multi_console_register(void)
{
	console = cli_console_create(&null_console_ops, "null");
	if (!console)
		return -ENOMEM;

	set_default_console(console);
	return 0;
}

void null_multi_console_unregister(void)
{
	return;
}
