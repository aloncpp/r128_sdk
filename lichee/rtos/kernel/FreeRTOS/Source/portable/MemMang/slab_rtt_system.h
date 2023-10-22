
#ifndef SLAB_RTT_SYSTEM_H
#define SLAB_RTT_SYSTEM_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <FreeRTOS.h>
#include <portmacro.h>
#include <FreeRTOSConfig.h>
#include <task.h>
#include <port_misc.h>
#include <queue.h>
#include <semphr.h>

typedef portBASE_TYPE rt_err_t;
typedef portBASE_TYPE rt_base_t;
typedef uint32_t rt_ubase_t;
typedef uint32_t rt_uint32_t;
typedef int32_t  rt_int32_t;
typedef uint8_t  rt_uint8_t;
typedef size_t   rt_size_t;
typedef int8_t   rt_bool_t;
typedef TaskHandle_t rt_thread_t;

#define RT_TRUE 1
#define RT_FALSE 0
#define RT_NULL NULL

#define RT_ALIGN_SIZE 4

#define RT_ASSERT(x) configASSERT(x)
#define rt_kprintf printf
#ifndef CONFIG_KASAN
#define rt_memset  memset
#define rt_memcpy  memcpy
#define rt_memcmp  memcmp
#endif

#define rt_interrupt_get_nest uGetInterruptNest

void *rt_realloc(void *ptr, rt_size_t size);
void *rt_malloc(rt_size_t size);
void *rt_calloc(rt_size_t count, rt_size_t size);
void rt_free(void * ptr);
void *rt_malloc_align(size_t size, size_t align);
void rt_free_align(void *ptr);

#define rt_inline static __inline

/**
 * @ingroup BasicDef
 *
 * @def RT_ALIGN(size, align)
 * Return the most contiguous size aligned at specified width. RT_ALIGN(13, 4)
 * would return 16.
 */
#define RT_ALIGN(size, align)           (((size) + (align) - 1) & ~((align) - 1))

/**
 * @ingroup BasicDef
 *
 * @def RT_ALIGN_DOWN(size, align)
 * Return the down number of aligned at specified width. RT_ALIGN_DOWN(13, 4)
 * would return 12.
 */
#define RT_ALIGN_DOWN(size, align)      ((size) & ~((align) - 1))

#define RT_USING_HOOK 1
#define RT_MEM_STATS 1
#define RT_USING_SLAB 1
#define RT_USING_HEAP 1

#define RT_DEBUG_SLAB 0
#define RT_DEBUG_LOG(type, message)                                           \
do                                                                            \
{                                                                             \
    if (type)                                                                 \
        rt_kprintf message;                                                   \
}                                                                             \
while (0)

#define RT_DEBUG_NOT_IN_INTERRUPT                                             \
do                                                                            \
{                                                                             \
    if (rt_interrupt_get_nest() != 0)                                         \
    {                                                                         \
        rt_kprintf("Function[%s] shall not be used in ISR\n", __FUNCTION__);  \
        RT_ASSERT(0)                                                          \
    }                                                                         \
}                                                                             \
while (0)

#define RT_MM_PAGE_SIZE                 4096
#define RT_MM_PAGE_MASK                 (RT_MM_PAGE_SIZE - 1)
#define RT_MM_PAGE_BITS                 12

#ifdef RT_USING_HOOK
#define RT_OBJECT_HOOK_CALL(func, argv) \
    do { if ((func) != RT_NULL) func argv; } while (0)
#else
#define RT_OBJECT_HOOK_CALL(func, argv)
#endif

#endif  /*SLAB_RTT_SYSTEM_H*/
