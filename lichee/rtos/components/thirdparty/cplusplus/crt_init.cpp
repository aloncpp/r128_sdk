/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
* Change Logs:
* Date           Author       Notes
* 2014-12-03     Bernard      Add copyright header.
* 2014-12-29     Bernard      Add cplusplus initialization for ARMCC.
* 2016-06-28     Bernard      Add _init/_fini routines for GCC.
* 2016-10-02     Bernard      Add WEAK for cplusplus_system_init routine.
*/
#include <stdint.h>
#include <FreeRTOS.h>
#include <FreeRTOSConfig.h>

#define RT_WEAK __attribute__((weak))

#ifdef __cplusplus
extern "C"{
#endif

#if defined(__CC_ARM) || defined(__CLANG_ARM)
extern void $Super$$__cpp_initialize__aeabi_(void);
/* we need to change the cpp_initialize order */
RT_WEAK void $Sub$$__cpp_initialize__aeabi_(void)
{
    /* empty */
}
#elif defined(__GNUC__) && !defined(__CS_SOURCERYGXX_MAJ__)
/* The _init()/_fini() routines has been defined in codesourcery g++ lite */
RT_WEAK void _init()
{
}

RT_WEAK void _fini()
{
}

RT_WEAK void *__dso_handle = 0;

#endif

RT_WEAK int cplusplus_system_init(void)
{
#if defined(__CC_ARM) || defined(__CLANG_ARM)
    /* If there is no SHT$$INIT_ARRAY, calling
     * $Super$$__cpp_initialize__aeabi_() will cause fault. At least until Keil5.12
     * the problem still exists. So we have to initialize the C++ runtime by ourself.
     */
    typedef void PROC();
    extern const unsigned long SHT$$INIT_ARRAY$$Base[];
    extern const unsigned long SHT$$INIT_ARRAY$$Limit[];

    const unsigned long *base = SHT$$INIT_ARRAY$$Base;
    const unsigned long *lim  = SHT$$INIT_ARRAY$$Limit;

    for (; base != lim; base++)
    {
        PROC *proc = (PROC *)((const char *)base + *base);
        (*proc)();
    }
#elif defined(__GNUC__)
    typedef void(*pfunc)();
    extern pfunc __init_array_start[];
    extern pfunc __init_array_end[];

    pfunc *p;

    for (p = __init_array_start; (unsigned long)p < (unsigned long)__init_array_end; p++)
        (*p)();
#endif

    return 0;
}

#ifdef __cplusplus
}
#endif

int32_t rt_tick_from_millisecond(int32_t ms)
{
    int32_t tick;

    if (ms < 0)
    {
        tick = portMAX_DELAY;
    }
    else
    {
        tick = configTICK_RATE_HZ * (ms / 1000);
        tick += (configTICK_RATE_HZ * (ms % 1000) + 999) / 1000;
    }

    /* return the calculated tick */
    return tick;
}
