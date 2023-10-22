
#ifndef _FW_DEBUG_H_
#define _FW_DEBUG_H_


#define HAL_SYSLOG(fmt, ...)    do { while (1); } while (0)

#define HAL_ASSERT_PARAM(exp)                                           \
    do {                                                                \
        if (!(exp)) {                                                   \
            HAL_SYSLOG("Invalid param at %s:%d\n", __func__, __LINE__); \
        }                                                               \
    } while (0)





#endif


