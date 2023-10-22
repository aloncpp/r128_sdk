#include <FreeRTOS.h>
#include <semphr.h>

#include <hal_hwspinlock.h>
#include <hal_interrupt.h>
#include <hal_thread.h>

#include <xstdio.h>

static void *custom_prout(void *fp, const char *buffer, size_t len)
{
	fwrite(buffer, 1, len, fp);
	return fp;
}

//The function '_Printf' is in libc, it's prototype is below:
//extern int _Printf(void *(*)(void *, const char *, size_t), void *, const char *, va_list, int);

static SemaphoreHandle_t s_printf_mutex;

int printf(const char *format, ...)
{
	va_list va;

	if (!hal_thread_is_in_critical_context())
	{
		if (!s_printf_mutex)
			s_printf_mutex = xSemaphoreCreateMutex();
		xSemaphoreTake(s_printf_mutex, portMAX_DELAY);
	}

	if (!hal_thread_is_in_critical_context())
		hal_hwspin_lock(SPINLOCK_CLI_UART_LOCK_BIT);

	int temp = 0;
	va_start(va, format);
	int ret = _Printf(custom_prout, stdout, format, va, (int)&temp);
	va_end(va);

	if (!hal_thread_is_in_critical_context())
		hal_hwspin_unlock(SPINLOCK_CLI_UART_LOCK_BIT);

	if (!hal_thread_is_in_critical_context())
		xSemaphoreGive(s_printf_mutex);
	return ret;
}
