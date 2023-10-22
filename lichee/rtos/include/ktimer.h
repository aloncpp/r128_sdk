#ifndef KTIMER_H
#define KTIMER_H

#include <sys/time.h>
#include <bits.h>

#include <hal_time.h>

#ifdef CONFIG_ARCH_SUN20IW2
#define NS_PER_TICK     (25)
#else
#define NS_PER_TICK     (41.66666666666667f)
#endif
#define CLOCKSOURCE_MASK(bits) RTOS_GENMASK_ULL((bits) - 1, 0)

typedef uint64_t        cycle_t;

struct timespec64
{
    int64_t  tv_sec;                 /* seconds */
    int32_t  tv_nsec;                /* nanoseconds */
};

struct clocksource
{
    cycle_t (*read)(struct clocksource *cs);
    cycle_t  mask;
    uint32_t mult;
    uint32_t shift;
    uint32_t arch_timer_rate;
    const char *name;
};

int64_t ktime_get(void);
int do_gettimeofday(struct timespec64 *ts);

static inline uint64_t ktime_get_ns(void)
{
    return (uint64_t)ktime_get();
}

static inline struct timespec timespec64_to_timespec(const struct timespec64 ts64)
{
    struct timespec ret;

    ret.tv_sec = (time_t)ts64.tv_sec;
    ret.tv_nsec = ts64.tv_nsec;
    return ret;
}

static inline struct timespec64 timespec_to_timespec64(const struct timespec ts)
{
    struct timespec64 ret;

    ret.tv_sec = ts.tv_sec;
    ret.tv_nsec = ts.tv_nsec;
    return ret;
}

static inline void ktime_get_ts(struct timespec *ts)
{
    struct timespec64 _ts;
    do_gettimeofday(&_ts);
    *ts = timespec64_to_timespec(_ts);
}

/**
* ns_to_timespec - Convert nanoseconds to timespec
* @nsec:       the nanoseconds value to be converted
*
* Returns the timespec representation of the nsec parameter.
*/
static inline struct timespec ns_to_timespec(const int64_t nsec)
{
    struct timespec ts;
    int32_t rem;

    if (!nsec)
        return (struct timespec) { 0, 0 };

    ts.tv_sec = nsec / NSEC_PER_SEC;
    ts.tv_nsec = (long)(nsec % NSEC_PER_SEC);

    return ts;
}

/**
 * ns_to_timeval - Convert nanoseconds to timeval
 * @nsec:       the nanoseconds value to be converted
 *
 * Returns the timeval representation of the nsec parameter.
 */
static inline struct timeval ns_to_timeval(const int64_t nsec)
{
    struct timespec ts = ns_to_timespec(nsec);
    struct timeval tv;

    tv.tv_sec = ts.tv_sec;
    tv.tv_usec = (suseconds_t) ts.tv_nsec / 1000;

    return tv;
}

static inline int64_t timeval_to_ns(const struct timeval *tv)
{
    return ((int64_t) tv->tv_sec * NSEC_PER_SEC) +
           tv->tv_usec * NSEC_PER_USEC;
}
#endif  /*KTIMER_H*/
