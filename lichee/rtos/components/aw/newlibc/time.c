#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <reent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/reent.h>
#include <time.h>
#include <sys/times.h>
#include <sys/lock.h>
#include <string.h>
#include <stdio.h>

#include <FreeRTOSConfig.h>
#include <portmacro.h>
#include <portable.h>

#define CYC_PER_TICK   (24000000 / configTICK_RATE_HZ)

int clock_gettime (clockid_t clock_id, struct timespec *tp)
{
    if (tp == NULL) {
        errno = EINVAL;
        return -1;
    }

    struct timeval tv;
    _gettimeofday_r(NULL, &tv, NULL);
    unsigned long monotonic_time_ms = 0;

    switch (clock_id) {
        case CLOCK_REALTIME:
            tp->tv_sec = tv.tv_sec;
            tp->tv_nsec = tv.tv_usec * 1000L;
            break;
        case CLOCK_MONOTONIC:
            monotonic_time_ms = xTaskGetTickCount() / CYC_PER_TICK * (1000 / configTICK_RATE_HZ);
            tp->tv_sec = monotonic_time_ms / 1000;
            tp->tv_nsec = (monotonic_time_ms % 1000) * 1000000L;
            break;
        default:
            errno = EINVAL;
            return -1;
    }
    return 0;
}

int clock_settime (clockid_t clock_id, const struct timespec *tp)
{
    if (tp == NULL) {
        errno = EINVAL;
        return -1;
    }
    struct timeval tv;
    switch (clock_id) {
        case CLOCK_REALTIME:
            tv.tv_sec = tp->tv_sec;
            tv.tv_usec = tp->tv_nsec / 1000L;
            settimeofday(&tv, NULL);
            break;
        default:
            printf("Do not support set monotonic time by clock_settime\n");
            errno = EINVAL;
            return -1;
    }
    return 0;
}

#if 0
int usleep(useconds_t us)
{
    const int us_per_tick = portTICK_PERIOD_MS * 1000;
    if (us < us_per_tick) {
        //ets_delay_us((uint32_t) us);
        vTaskDelay(1);
    } else {
        /* since vTaskDelay(1) blocks for anywhere between 0 and portTICK_PERIOD_MS,
         * round up to compensate.
         */
        vTaskDelay((us + us_per_tick - 1) / us_per_tick);
    }
    return 0;
}

unsigned int sleep(unsigned int seconds)
{
    usleep(seconds*1000000UL);
    return 0;
}

int adjtime(const struct timeval *delta, struct timeval *outdelta)
{
  return 0;
}
#endif

int gettimeready(void)
{
    int ret = 0;
    unsigned long long DIFF_YEAR = 60 * 60 * 24 * 365;

    struct timeval sys_time = {0};
    gettimeofday(&sys_time, NULL);

    if(sys_time.tv_sec > 10 * DIFF_YEAR)
    {
        ret = 1;
    }
    return ret;
}
