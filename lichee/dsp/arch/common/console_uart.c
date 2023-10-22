#include <string.h>
#include <platform.h>
#include <console.h>
#include <sunxi_hal_common.h>
#include <hal_uart.h>

void console_uart_init(void)
{
	int val = CONSOLE_UART;
	console_uart = val;
	hal_uart_init(val);
}
