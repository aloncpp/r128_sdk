#include <hal_uart.h>
#include <stdint.h>
#include <spinlock.h>

#include "multi_console_internal.h"

int uart_console_write(const void *buf, size_t len, void *priv)
{
	int i;
	const char *pc = buf;
	for (i = 0; i < len; i++) {
		if (pc[i] == '\n')
			hal_uart_put_char(CONFIG_CLI_UART_PORT, '\r');
		hal_uart_put_char(CONFIG_CLI_UART_PORT, pc[i]);
	}

	return len;
}

int uart_console_read(void *buf, size_t len, void *priv, uint32_t timeout)
{
	return hal_uart_receive_no_block(CONFIG_CLI_UART_PORT, (uint8_t *)buf, len, timeout);
}

static int uart_console_init(void *private_data)
{
	//return hal_uart_init(CONFIG_CLI_UART_PORT);
	return 0;
}

static cli_dev_ops uart_console_ops = {
	.write = uart_console_write,
	.read = uart_console_read,
	.init = uart_console_init,
	.deinit = NULL,
	.priv = NULL,
#ifdef CONFIG_UART_MULTI_CONSOLE_AS_MAIN
	.echo = 1,
	.task = 1,
	.prefix = "uart>",
#else
	.echo = 0,
	.task = 0,
	.prefix = NULL,
#endif
};
static cli_console *console;

int uart_multi_console_register(void)
{
	console = cli_console_create(&uart_console_ops, "uart");
	if (!console)
		return -ENOMEM;

	set_default_console(console);
	return 0;
}

void uart_multi_console_unregister(void)
{
	return;
}
