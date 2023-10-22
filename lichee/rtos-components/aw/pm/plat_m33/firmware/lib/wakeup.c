#include <stdint.h>
#include <type.h>
#include <wakeup.h>
#include <platform.h>

#define isb(option)             __asm__ __volatile__ ("isb " #option : : : "memory")

#define RTC_WUPTIMER_CTRL_REG	(0x40051508)
#define RTC_WUPTIMER_VAL_REG	(0x4005150c)

#define RTC_FREE_COUNTER_LOW_OFFSET (0x60)
#define RTC_FREE_COUNTER_HIGH_OFFSET (0x64)

#define MSEC_PER_SEC    1000L
#define USEC_PER_MSEC   1000L
#define NSEC_PER_SEC    1000000000L

#define DEFAULT_NS_PER_TICK 31250
static uint32_t s_ns_per_tick = DEFAULT_NS_PER_TICK;

struct timespec64
{
    uint64_t  tv_sec;                 /* seconds */
    uint32_t  tv_nsec;                /* nanoseconds */
};

int wakeup_check_callback(void)
{
	return 0;
}

void enable_wuptimer(unsigned int time_ms)
{
	unsigned int val;

	if (time_ms > 131071)
		time_ms = 131071;

	val = (unsigned int)((time_ms * 32768) / 1000);
	writel(val, RTC_WUPTIMER_VAL_REG);
	writel((0x1 << 31), RTC_WUPTIMER_CTRL_REG);
}

static inline uint64_t pm_arch_counter_get_cntpct(void)
{
    uint64_t cval = 0ULL;

    isb();
    uint64_t lval = readl(RTC_BASE + RTC_FREE_COUNTER_LOW_OFFSET);
    uint64_t hval = readl(RTC_BASE + RTC_FREE_COUNTER_HIGH_OFFSET);
    cval = lval | (hval << 32);
    isb();
    return cval;
}

static uint64_t pm_ktime_get(void)
{
    uint64_t arch_counter = 0ULL;

    arch_counter = pm_arch_counter_get_cntpct();

    return arch_counter * s_ns_per_tick;
}

uint64_t pm_gettime_ns(void)
{
    return pm_ktime_get();
}
