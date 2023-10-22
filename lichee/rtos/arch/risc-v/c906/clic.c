#include <stdio.h>
#include <stdlib.h>
#include <excep.h>
#include <csr.h>
#include <rv_io.h>
#include <irqflags.h>
#include <timex.h>

#ifdef CONFIG_COMPONENTS_PM
#include "pm_syscore.h"
#endif

#define CSR_PLIC_BASE            0xfc1
#define C910_PLIC_CLINT_OFFSET   0x04000000

static uint64_t mmode_peripheral_addr_base;
static uint64_t clic_int_control_addr_base;
static uint64_t clic_int_control_addr_comp;
static uint64_t clic_int_control_addr_mtime;

int clic_driver_init(void)
{
    mmode_peripheral_addr_base = csr_read(CSR_PLIC_BASE);
    if (mmode_peripheral_addr_base == 0)
    {
        printf("%s line %d, fatal error, peripheral address is null!\n", __func__, __LINE__);
        return -1;
    }

    clic_int_control_addr_base = mmode_peripheral_addr_base + C910_PLIC_CLINT_OFFSET;

    clic_int_control_addr_mtime = clic_int_control_addr_base + 0xbff8;
    clic_int_control_addr_comp = clic_int_control_addr_base + 0x4000;

    return 0;
}

uint64_t sunxi_clic_read_mtime(void)
{
    uint32_t lo, hi;
    unsigned long addr = clic_int_control_addr_mtime;

    do
    {
        hi = readl_relaxed((uint32_t *)addr + 1);
        lo = readl_relaxed((uint32_t *)addr);
    } while (hi != readl_relaxed((uint32_t *)addr + 1));

    return ((uint64_t)hi << 32) | (uint64_t)lo;
}

void sunxi_program_timer_next_event(unsigned long next_compare)
{
    unsigned int mask = -1U;
    unsigned long value = next_compare;
    unsigned long addr = clic_int_control_addr_comp;

    writel_relaxed(value & mask, (void *)(addr));
    writel_relaxed(value >> 32, (void *)(addr) + 0x04);
}

uint64_t sunxi_get_timer_next_event(void)
{
	uint64_t low, high;
	unsigned long addr = clic_int_control_addr_comp;

	low = readl_relaxed((void *)(addr));
	high = readl_relaxed((void *)(addr) + 0x04);

	return low | (high << 32);
}

#define DEFAULT_ARCH_TIMER_CLK_FREQ (40000000UL)
static unsigned long delta = DEFAULT_ARCH_TIMER_CLK_FREQ / CONFIG_HZ;
static uint64_t s_delta_after_wake_up = 0;

#ifdef CONFIG_ARCH_SUN20IW2P1
extern uint32_t arch_timer_get_cntfrq(void);
#endif

#ifdef CONFIG_COMPONENTS_PM
static int clic_suspend(void *data, suspend_mode_t mode)
{
	int ret = 0;

	csr_clear(mie, MIE_MTIE);

	uint64_t current = get_cycles64();
	uint64_t target_count_value = sunxi_get_timer_next_event();

	if (current < target_count_value)
	{
		s_delta_after_wake_up = target_count_value - current;
	}
	else
	{
		s_delta_after_wake_up = delta;
	}
	return ret;
}

static void clic_resume(void *data, suspend_mode_t mode)
{
	csr_set(mie, MIE_MTIE);
	sunxi_program_timer_next_event(get_cycles64() + s_delta_after_wake_up);
}

static struct syscore_ops clic_syscore_ops = {
	.name = "clic_syscore_ops",
	.suspend = clic_suspend,
	.resume = clic_resume,
};
#endif /* CONFIG_COMPONENTS_PM */

void system_tick_init(void)
{
#ifdef CONFIG_ARCH_SUN20IW2P1
    delta = arch_timer_get_cntfrq() / CONFIG_HZ;
#endif

    csr_set(mie, MIE_MTIE);
    sunxi_program_timer_next_event(get_cycles64() + delta);

#ifdef CONFIG_COMPONENTS_PM
	int ret;
	ret = pm_syscore_register(&clic_syscore_ops);
	if (ret)
		printf("WARNING: clic syscore ops registers failed\n");
#endif

}

void riscv_timer_interrupt(void)
{
    csr_clear(mie, MIE_MTIE);

    sunxi_program_timer_next_event(get_cycles64() + delta);
    csr_set(mie, MIE_MTIE);
}
