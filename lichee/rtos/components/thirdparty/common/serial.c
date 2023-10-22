#include "serial.h"
#include <stdarg.h>
#include <hal_uart.h>
#include <uart_drv.h>
#include "vsprintf.h"

#if defined(CONFIG_ARCH_SUN20IW2)
void serial_init (void)
{
#ifdef CONFIG_DRIVERS_UART
#if defined(CONFIG_ARCH_ARM)
	int i;
	int init = 0;

	init |= (1 << CONFIG_CLI_UART_PORT);

#ifdef CONFIG_SUNXI_UART_REGISTER_UART0
	init |= (1 << 0);
#endif

#ifdef CONFIG_SUNXI_UART_REGISTER_UART1
	init |= (1 << 1);
#endif

#ifdef CONFIG_SUNXI_UART_REGISTER_UART2
	init |= (1 << 2);
#endif

#ifdef CONFIG_SUNXI_UART_REGISTER_UART3
	init |= (1 << 3);
#endif
	for (i = 0; i < 8; i++)
	{
		if (init & (1 << i))
			hal_uart_init(i);
	}
#endif

#ifdef CONFIG_COMPONENTS_AW_DEVFS
	sunxi_driver_uart_init();
#endif

#endif
}
#else
void serial_init (void)
{
#ifdef CONFIG_DRIVERS_UART
	int i;
	int init = 0;

	init |= (1 << CONFIG_CLI_UART_PORT);

#ifdef CONFIG_SUNXI_UART_REGISTER_UART0
	init |= (1 << 0);
#endif

#ifdef CONFIG_SUNXI_UART_REGISTER_UART1
	init |= (1 << 1);
#endif

#ifdef CONFIG_SUNXI_UART_REGISTER_UART2
	init |= (1 << 2);
#endif

#ifdef CONFIG_SUNXI_UART_REGISTER_UART3
	init |= (1 << 3);
#endif
	for (i = 0; i < 8; i++)
	{
		if (init & (1 << i))
			hal_uart_init(i);
	}

#ifdef CONFIG_COMPONENTS_AW_DEVFS
	sunxi_driver_uart_init();
#endif
#endif
}
#endif