#include <string.h>

#include <ktimer.h>
#include <hal_time.h>
#include <hal_status.h>

extern void sleep(int seconds);
extern int usleep(int usecs);
extern void udelay(unsigned int us);

int msleep(unsigned int msecs)
{
    usleep(msecs * 1000);

    return HAL_OK;
}

int hal_sleep(unsigned int secs)
{
    sleep(secs);
    return HAL_OK;
}

int hal_usleep(unsigned int usecs)
{
    usleep(usecs);
    return HAL_OK;
}

int hal_msleep(unsigned int msecs)
{
    msleep(msecs);
    return HAL_OK;
}

void hal_udelay(unsigned int us)
{
    udelay(us);
}

void hal_mdelay(unsigned int ms)
{
    hal_udelay(ms * 1000);
}

void hal_sdelay(unsigned int s)
{
    hal_mdelay(s * 1000);
}

uint64_t hal_gettime_ns(void)
{
    struct timespec64 timeval;
    memset(&timeval, 0, sizeof(struct timespec64));
    do_gettimeofday(&timeval);
    return timeval.tv_sec * MSEC_PER_SEC * USEC_PER_MSEC * 1000LL + timeval.tv_nsec;
}
