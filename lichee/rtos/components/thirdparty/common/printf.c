#include <_ansi.h>
#include <reent.h>
#include <stdio.h>
#include <stdarg.h>
#include <hal_hwspinlock.h>
#include <spinlock.h>

#ifdef CONFIG_COMPONENTS_PM
static hal_spinlock_t pm_printf_lock = {
	.owner = 0,
	.counter = 0,
	.spin_lock = {
		.slock = 0,
	},
};

static uint32_t printf_lock_cnt = 0;
void printf_lock(void)
{
	hal_spin_lock(&pm_printf_lock);
	printf_lock_cnt += 1;
	hal_spin_unlock(&pm_printf_lock);
}

void printf_unlock(void)
{
	hal_spin_lock(&pm_printf_lock);
	if (printf_lock_cnt > 0)
		printf_lock_cnt -= 1;
	hal_spin_unlock(&pm_printf_lock);
}
#endif

int _printf_r(struct _reent *ptr,
              const char *__restrict fmt, ...)
{
    int ret;
    va_list ap;

    _REENT_SMALL_CHECK_INIT(ptr);
    va_start(ap, fmt);
    ret = _vfprintf_r(ptr, _stdout_r(ptr), fmt, ap);
    va_end(ap);
    return ret;
}

#ifndef _REENT_ONLY

#if defined(CONFIG_UART_CLI_USE_SPINLOCK)
static hal_spinlock_t io_lock = {
	.owner = 0,
	.counter = 0,
	.spin_lock = {
		.slock = 0,
	},
};
#elif defined(CONFIG_UART_CLI_USE_MUTEX)
static pthread_mutex_t io_mutex = FREERTOS_POSIX_MUTEX_INITIALIZER;
#endif

static unsigned long enter_io_critical(void)
{
#if defined(CONFIG_UART_CLI_USE_SPINLOCK)
	hal_spin_lock(&io_lock);
#elif defined(CONFIG_UART_CLI_USE_MUTEX)
    if (!hal_thread_is_in_critical_context())
        pthread_mutex_lock(&io_mutex);
#endif
    return 0;
}

static void exit_io_critical(unsigned long cpu_sr)
{
    (void)cpu_sr;
#if defined(CONFIG_UART_CLI_USE_SPINLOCK)
	hal_spin_unlock(&io_lock);
#elif defined(CONFIG_UART_CLI_USE_MUTEX)
    if (!hal_thread_is_in_critical_context())
	    pthread_mutex_unlock(&io_mutex);
#endif
}

int printf(const char *__restrict fmt, ...)
{
	int lock_ret = HWSPINLOCK_ERR;
#ifdef CONFIG_COMPONENTS_PM
	if (printf_lock_cnt)
		return 0;
#endif

    int ret;
    va_list ap;
    uint32_t flags;

    flags = enter_io_critical();
    struct _reent *ptr = _REENT;

#ifdef CONFIG_CLI_UART_PORT_LOCK
    if (!hal_thread_is_in_critical_context())
        lock_ret = hal_hwspin_lock_timeout(SPINLOCK_CLI_UART_LOCK_BIT, 100);
#endif

    _REENT_SMALL_CHECK_INIT(ptr);
    va_start(ap, fmt);
    ret = _vfprintf_r(ptr, _stdout_r(ptr), fmt, ap);
    va_end(ap);

#ifdef CONFIG_CLI_UART_PORT_LOCK
    if (lock_ret == HWSPINLOCK_OK)
        hal_hwspin_unlock(SPINLOCK_CLI_UART_LOCK_BIT);
#endif

    exit_io_critical(flags);

    return ret;
}

#ifdef _NANO_FORMATTED_IO
int _iprintf_r(struct _reent *, const char *, ...) _ATTRIBUTE((__alias__("_printf_r")));
int iprintf(const char *, ...) _ATTRIBUTE((__alias__("printf")));
#endif
#endif /* ! _REENT_ONLY */
