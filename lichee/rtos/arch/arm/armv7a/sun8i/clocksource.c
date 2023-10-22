#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#include <barrier.h>
#include <ktimer.h>

#define GENERNIC_TIMRE_REQ	(24000000)

static inline uint32_t arch_timer_get_cntfrq(void)
{
    uint32_t val;
    asm volatile("mrc p15, 0, %0, c14, c0, 0" : "=r"(val));
    return val;
}

static inline uint64_t arch_counter_get_cntvct(void)
{
    uint64_t cval;

    isb();
    asm volatile("mrrc p15, 1, %Q0, %R0, c14" : "=r"(cval));
    return cval;
}

static inline uint64_t arch_counter_get_cntpct(void)
{
    uint64_t cval;

    isb();
    asm volatile("mrrc p15, 0, %Q0, %R0, c14" : "=r"(cval));
    return cval;
}

uint64_t read_cntpct(void)
{
    uint64_t cval;

    isb();
    asm volatile("mrrc p15, 0, %Q0, %R0, c14" : "=r"(cval));
    return cval;
}

void udelay(uint32_t us)
{
    uint64_t start, target;
    uint64_t frequency;

    frequency = arch_timer_get_cntfrq();

    if(frequency != GENERNIC_TIMRE_REQ)
    {
        soft_break();
    }

    start = arch_counter_get_cntpct();
    target = frequency / 1000000ULL * us;

    while (arch_counter_get_cntpct() - start <= target);
}

void mdelay(uint32_t ms)
{
    udelay(ms * 1000);
}

void sdelay(uint32_t sec)
{
    mdelay(sec * 1000);
}


uint64_t (*arch_timer_read_counter)(void) = arch_counter_get_cntpct;

static cycle_t arch_counter_read(struct clocksource *cs)
{
    return arch_timer_read_counter();
}

static struct clocksource arch_timer_clocksource =
{
    .name   = "arch_sys_counter",
    .read   = arch_counter_read,
    .mask   = CLOCKSOURCE_MASK(56),
};

int64_t ktime_get(void)
{
    struct clocksource *cs = &arch_timer_clocksource;

    double ns_per_ticks = cs->mult / (1 << cs->shift);

    //~41.6666667 nano seconds per tick.
    ns_per_ticks = NS_PER_TICK;
    uint64_t arch_counter = cs->read(cs);

    return arch_counter * ns_per_ticks;
}

int do_gettimeofday(struct timespec64 *ts)
{
    if (ts == NULL)
        return 0;

    int64_t nsecs = ktime_get();

    ts->tv_sec  = nsecs / NSEC_PER_SEC;
    ts->tv_nsec = nsecs % NSEC_PER_SEC;

    return 0;
}
