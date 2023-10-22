#include <xtensa/hal.h>
#include <xtensa/tie/xt_externalregisters.h>
#include <xtensa/config/core.h>
#include <xtensa/config/core-matmap.h>
#include <xtensa/xtruntime.h>
#include <string.h>
#include "spinlock.h"
#ifdef CONFIG_DRIVERS_CCMU
#include <hal_clk.h>
#endif
#ifdef CONFIG_DRIVERS_INTC
#include <hal_intc.h>
#endif
#ifdef CONFIG_DRIVERS_GPIO
#include <hal_gpio.h>
#endif
#ifdef CONFIG_DRIVERS_HWSPINLOCK
#include <hal_hwspinlock.h>
#endif
#include <hal_uart.h>
#include <console.h>
#ifdef CONFIG_COMPONENTS_PM
#include <pm_init.h>
#endif
#ifdef CONFIG_COMPONENTS_AW_SYS_CONFIG_SCRIPT
#include <hal_cfg.h>
#endif
extern void cache_config(void);
extern void _xt_tick_divisor_init(void);
extern void console_uart_init(void);
extern unsigned int xtbsp_clock_freq_hz(void);
extern void set_arch_timer_ticks_per_us(uint32_t dsp_clk_freq);

__attribute__((weak)) void heap_init(void)
{

}

static void set_rtos_tick(void)
{
	unsigned int lock;

	/* system tick my use glabal var */
	spin_lock_irqsave(lock);
	_xt_tick_divisor_init();
	spin_unlock_irqrestore(lock);
}

void board_init(void)
{
	heap_init();

	/* cache configuration */
	cache_config();

#ifdef CONFIG_COMPONENTS_AW_SYS_CONFIG_SCRIPT
	hal_cfg_init();
#endif

#ifdef CONFIG_COMPONENTS_PM
	pm_wakecnt_init();
	pm_wakesrc_init();
	pm_syscore_init();
	pm_devops_init();
#endif

#ifdef CONFIG_DRIVERS_CCMU
	/* ccmu init */
	hal_clock_init();
#endif

#ifdef CONFIG_DRIVERS_INTC
	/* intc init */
	hal_intc_init(SUNXI_DSP_IRQ_R_INTC);
#endif

#if defined(CONFIG_DRIVERS_GPIO) && !defined(CONFIG_DRIVER_BOOT_DTS)
	/* gpio init */
	hal_gpio_init();
	hal_gpio_r_all_irq_disable();
#endif

#ifdef CONFIG_DRIVERS_HWSPINLOCK
	hal_hwspinlock_init();
#endif

#if defined(CONFIG_ARCH_SUN20IW2)
	console_uart = CONSOLE_UART;
#if defined(CONFIG_COMPONENTS_FREERTOS_CLI)
	hal_uart_init_for_amp_cli(CONSOLE_UART);
#endif
#else
#if defined(CONFIG_COMPONENTS_FREERTOS_CLI) && !defined(CONFIG_DRIVER_BOOT_DTS)
	console_uart_init();
#endif
#endif

	/* prepare for time API and rtos tick */
	set_arch_timer_ticks_per_us(xtbsp_clock_freq_hz());

	/* set rtos tick */
	set_rtos_tick();
}

