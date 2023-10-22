#include <hal_time.h>
#include <delay.h>
#include <hal_status.h>

int hal_sleep(unsigned int secs)
{
    msleep(secs * 1000);
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

