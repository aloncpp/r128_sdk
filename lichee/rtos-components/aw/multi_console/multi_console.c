#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <hal_uart.h>
#include <hal_mem.h>

#include "multi_console_internal.h"

int console_printk(const char *fmt, ...)
{
#if defined(CONFIG_DRIVERS_UART) && defined(CONFIG_CLI_UART_PORT)
#define CONSOLEBUF_SIZE 256
	va_list args;
	size_t length;
	int i = 0;
	char log_buf[CONSOLEBUF_SIZE];

	memset(&log_buf, 0, CONSOLEBUF_SIZE);

	va_start(args, fmt);
	length = vsnprintf(log_buf, sizeof(log_buf) - 1, fmt, args);
	if (length > CONSOLEBUF_SIZE - 1)
		length = CONSOLEBUF_SIZE - 1;
	while (length--)
		hal_uart_put_char(CONFIG_CLI_UART_PORT, log_buf[i++]);

	va_end(args);
#endif

	return 0;
}

int multiple_console_early_init(void)
{
	int ret;

	extern int console_core_init(void);
	ret = console_core_init();
	if (ret)
		return ret;

	console_debug("register null console device.\r\n");
	extern int null_multi_console_register(void);
	ret = null_multi_console_register();
	if (ret)
		return ret;

#ifdef CONFIG_UART_MULTI_CONSOLE
	console_debug("register uart console device.\r\n");
	extern int uart_multi_console_register(void);
	ret = uart_multi_console_register();
	if (ret)
		return ret;
#endif

	return 0;
}
int multiple_console_init(void)
{
#ifdef CONFIG_RPMSG_MULTI_CONSOLE
	int ret;
	console_debug("register rpmsg console device.\r\n");
	extern int rpmsg_multi_console_register(void);
	ret = rpmsg_multi_console_register();
	if (ret)
		return ret;
#endif

	return 0;
}
