#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <ktimer.h>

void udelay(uint32_t us)
{
    int64_t t0 = ktime_get();
    int64_t t1 = t0 + us * 1000;
    while (ktime_get() < t1);
}

void mdelay(uint32_t ms)
{
    udelay(ms * 1000);
}

void sdelay(uint32_t sec)
{
    mdelay(sec * 1000);
}
