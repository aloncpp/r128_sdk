#include <cli_console.h>
#include <hal_uart.h>
#include <stdint.h>
#include <hal_atomic.h>

#ifdef CONFIG_ARCH_SUN8IW20
#define CONFIG_IRQ_CLI_USE_SPINLOCK
#endif

int uart_console_write_no_lock(const void *buffer, size_t len, void *privata_data)
{
	const char *buf = buffer;
#ifndef CONFIG_DISABLE_ALL_UART_LOG
	hal_uart_send(CONFIG_CLI_UART_PORT, buf, len);
#endif
    return 0;
}

#if defined(CONFIG_UART_CLI_USE_SPINLOCK)
static freert_spinlock_t uart_twlock = {
	.owner = 0,
	.counter = 0,
	.spin_lock = {
		.slock = 0,
	},
};
#elif defined(CONFIG_UART_CLI_USE_MUTEX)
static pthread_mutex_t uart_tw_mutex = FREERTOS_POSIX_MUTEX_INITIALIZER;
#endif

int uart_console_write(const void *buffer, size_t len, void *privata_data)
{
	int ret;

	extern int hal_thread_is_in_critical_context(void);
#if defined(CONFIG_UART_CLI_USE_SPINLOCK)
	hal_spin_lock(&uart_twlock);
#elif defined(CONFIG_UART_CLI_USE_MUTEX)
    if (!hal_thread_is_in_critical_context())
        pthread_mutex_lock(&uart_tw_mutex);
#endif

	ret = uart_console_write_no_lock(buffer, len, privata_data);

#if defined(CONFIG_UART_CLI_USE_SPINLOCK)
	hal_spin_unlock(&uart_twlock);
#elif defined(CONFIG_UART_CLI_USE_MUTEX)
    if (!hal_thread_is_in_critical_context())
	    pthread_mutex_unlock(&uart_tw_mutex);
#endif

    return ret;
}

#if defined(CONFIG_IRQ_CLI_USE_SPINLOCK)
static freert_spinlock_t uart_iwlock = {
	.owner = 0,
	.counter = 0,
	.spin_lock = {
		.slock = 0,
	},
};
#endif

int uart_console_write_in_irq(const void *buffer, size_t len, void *privata_data)
{
	int ret;

#if defined(CONFIG_IRQ_CLI_USE_SPINLOCK)
	hal_spin_lock(&uart_iwlock);
#endif

	ret = uart_console_write_no_lock(buffer, len, privata_data);

#if defined(CONFIG_IRQ_CLI_USE_SPINLOCK)
	hal_spin_unlock(&uart_iwlock);
#endif
    return ret;
}

int uart_console_read(void *buf, size_t len, void *privata_data)
{
#ifndef CONFIG_DISABLE_ALL_UART_LOG
    return hal_uart_receive(CONFIG_CLI_UART_PORT, (uint8_t *)buf, len);
#else
    return 0;
#endif
}

static int uart_console_init(void *private_data)
{
#ifndef CONFIG_DISABLE_ALL_UART_LOG
    hal_uart_init(CONFIG_CLI_UART_PORT);
#endif
}

static int uart_console_deinit(void *private_data)
{
    return 1;
}

static device_console uart_console =
{
    .name = "uart-console",
    .write = uart_console_write,
    .read = uart_console_read,
    .init = uart_console_init,
    .deinit = uart_console_deinit,
};

cli_console cli_uart_console =
{
    .name = "cli-uart-console",
    .dev_console = &uart_console,
    .init_flag = 0,
    .exit_flag = 0,
    .alive = 1,
    .private_data = NULL,
};
