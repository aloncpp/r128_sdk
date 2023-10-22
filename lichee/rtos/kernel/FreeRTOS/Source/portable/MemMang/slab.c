/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * File      : slab.c
 *
 * Change Logs:
 * Date           Author       Notes
 * 2008-07-12     Bernard      the first version
 * 2010-07-13     Bernard      fix RT_ALIGN issue found by kuronca
 * 2010-10-23     yi.qiu       add module memory allocator
 * 2010-12-18     yi.qiu       fix zone release bug
 */

/*
 * KERN_SLABALLOC.C - Kernel SLAB memory allocator
 *
 * Copyright (c) 2003,2004 The DragonFly Project.  All rights reserved.
 *
 * This code is derived from software contributed to The DragonFly Project
 * by Matthew Dillon <dillon@backplane.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of The DragonFly Project nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific, prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
#include "slab_rtt_system.h"
#include <spinlock.h>
#include <stdio.h>
#include <console.h>

#if defined (RT_USING_HEAP) && defined (RT_USING_SLAB)
/* some statistical variable */
#ifdef RT_MEM_STATS
static rt_size_t used_mem, max_mem;
#endif

#ifdef RT_USING_HOOK
static void (*rt_malloc_hook)(void *ptr, rt_size_t size);
static void (*rt_free_hook)(void *ptr);
static void (*rt_page_alloc_hook)(void *ptr, rt_size_t npages);
static void (*rt_page_free_hook)(void *ptr, rt_size_t npages);
static void (*rt_malloc_large_hook)(void *ptr, rt_size_t size);
static void (*rt_free_large_hook)(void *ptr, rt_size_t size);
static void (*rt_malloc_small_hook)(void *ptr, rt_size_t size);
static void (*rt_free_small_hook)(void *ptr, rt_size_t size);
static void (*rt_realloc_small_hook)(void *ptr, rt_size_t size);

/**
 * @addtogroup Hook
 */

/**@{*/

/**
 * This function will set a hook function, which will be invoked when a memory
 * block is allocated from heap memory.
 *
 * @param hook the hook function
 */
void rt_malloc_sethook(void (*hook)(void *ptr, rt_size_t size))
{
    rt_malloc_hook = hook;
}

/**
 * This function will set a hook function, which will be invoked when a memory
 * block is released to heap memory.
 *
 * @param hook the hook function
 */
void rt_free_sethook(void (*hook)(void *ptr))
{
    rt_free_hook = hook;
}
void rt_page_alloc_sethook(void (*hook)(void *ptr, rt_size_t npages))
{
    rt_page_alloc_hook = hook;
}

void rt_page_free_sethook(void (*hook)(void *ptr, rt_size_t npages))
{
    rt_page_free_hook = hook;
}

void rt_malloc_large_sethook(void (*hook)(void *ptr, rt_size_t size))
{
    rt_malloc_large_hook = hook;
}

void rt_free_large_sethook(void (*hook)(void *ptr, rt_size_t size))
{
    rt_free_large_hook = hook;
}

void rt_malloc_small_sethook(void (*hook)(void *ptr, rt_size_t size))
{
    rt_malloc_small_hook = hook;
}

void rt_free_small_sethook(void (*hook)(void *ptr, rt_size_t size))
{
    rt_free_small_hook = hook;
}

void rt_realloc_small_sethook(void (*hook)(void *ptr, rt_size_t size))
{
    rt_realloc_small_hook = hook;
}


/**@}*/

#endif

/*
 * slab allocator implementation
 *
 * A slab allocator reserves a ZONE for each chunk size, then lays the
 * chunks out in an array within the zone.  Allocation and deallocation
 * is nearly instantanious, and fragmentation/overhead losses are limited
 * to a fixed worst-case amount.
 *
 * The downside of this slab implementation is in the chunk size
 * multiplied by the number of zones.  ~80 zones * 128K = 10MB of VM per cpu.
 * In a kernel implementation all this memory will be physical so
 * the zone size is adjusted downward on machines with less physical
 * memory.  The upside is that overhead is bounded... this is the *worst*
 * case overhead.
 *
 * Slab management is done on a per-cpu basis and no locking or mutexes
 * are required, only a critical section.  When one cpu frees memory
 * belonging to another cpu's slab manager an asynchronous IPI message
 * will be queued to execute the operation.   In addition, both the
 * high level slab allocator and the low level zone allocator optimize
 * M_ZERO requests, and the slab allocator does not have to pre initialize
 * the linked list of chunks.
 *
 * XXX Balancing is needed between cpus.  Balance will be handled through
 * asynchronous IPIs primarily by reassigning the z_Cpu ownership of chunks.
 *
 * XXX If we have to allocate a new zone and M_USE_RESERVE is set, use of
 * the new zone should be restricted to M_USE_RESERVE requests only.
 *
 *  Alloc Size  Chunking        Number of zones
 *  0-127       8               16
 *  128-255     16              8
 *  256-511     32              8
 *  512-1023    64              8
 *  1024-2047   128             8
 *  2048-4095   256             8
 *  4096-8191   512             8
 *  8192-16383  1024            8
 *  16384-32767 2048            8
 *  (if RT_MM_PAGE_SIZE is 4K the maximum zone allocation is 16383)
 *
 *  Allocations >= zone_limit go directly to kmem.
 *
 *          API REQUIREMENTS AND SIDE EFFECTS
 *
 *    To operate as a drop-in replacement to the FreeBSD-4.x malloc() we
 *    have remained compatible with the following API requirements:
 *
 *    + small power-of-2 sized allocations are power-of-2 aligned (kern_tty)
 *    + all power-of-2 sized allocations are power-of-2 aligned (twe)
 *    + malloc(0) is allowed and returns non-RT_NULL (ahc driver)
 *    + ability to allocate arbitrarily large chunks of memory
 */

#define rt_hw_interrupt_enable vPortExitCritical
#define rt_hw_interrupt_disable vPortEnterCritical

#ifdef CONFIG_SLAB_DEBUG

#ifdef CONFIG_KASAN
void kasan_enable_report(void);
void kasan_disable_report(void);
#endif

#define CHUNK_CHECK_MAGIC 0x23233232

#ifdef CONFIG_DEBUG_BACKTRACE
#include <backtrace.h>
#include <stdarg.h>

static int slab_printf(const char *fmt, ...)
{
    int ret = 0;

    va_list ap;
    va_start(ap, fmt);
    printf(fmt, ap);
    va_end(ap);

    return ret;
}
#endif

/*
 * The chunk allocated information for slab zone.
 * Neon need 8 bytes aligned buffer, so the structure
 * must be aligned to 8 bytes.
 */
typedef struct slab_chunk_debug_info
{
    /* 0:no allocate; 1:allocated */
    rt_int32_t chunk_alloc_flag: 2;
    /* the user truely need size, if use malloc(xx), need_size is set as xx */
    rt_uint32_t need_size: 30;
    rt_thread_t alloc_thread;
#ifdef CONFIG_DEBUG_BACKTRACE
    void *backtrace[CONFIG_CHUNK_BACKTRACE_LEVEL];
#endif
    struct slab_chunk_debug_info *next_debug_info;
} __attribute__((aligned(8))) slab_chunk_debug_info;

/*
 * The chunk debug magic will fill the first 8 bytes and the last 8 bytes of chunk.
 * Neon need 8 bytes aligned buffer, so the structure must be aligned to 8 bytes.
 */
typedef struct slab_chunk_debug_magic
{
    char redzone[8];
} __attribute__((aligned(8))) slab_chunk_debug_magic;

static slab_chunk_debug_magic chunk_debug_magic __attribute__((aligned(8))) =
{
#ifdef CONFIG_KASAN
    .redzone = {0},
#else
    .redzone = {0x23, 0x23, 0x32, 0x32, 0x23, 0x23, 0x32, 0x32},
#endif
};

#endif
#ifdef CONFIG_CPU_PRIVATE_DATA
const unsigned long cpu_private_data_paddr = CONFIG_VIRTUAL_DRAM_ADDR \
											+ CONFIG_TOTAL_HEAP_SIZE \
											- 0x100000 \
											+ (CONFIG_CPU_PRIVATE_DATA_ADDR & 0xFF000);
const unsigned long cpu_private_data_vaddr = CONFIG_CPU_PRIVATE_DATA_ADDR;
const unsigned long cpu_private_data_size = RT_ALIGN(CONFIG_CPU_PRIVATE_DATA_SIZE, RT_MM_PAGE_SIZE);
void *rt_page_alloc_addr(unsigned long addr, rt_size_t npages);
#endif
/*
 * Chunk structure for free elements
 */
typedef struct slab_chunk
{
    struct slab_chunk *c_next;
} slab_chunk;

/*
 * The IN-BAND zone header is placed at the beginning of each zone.
 */
typedef struct slab_zone
{
    rt_int32_t  z_magic;        /* magic number for sanity check */
    rt_int32_t  z_nfree;        /* total free chunks / ualloc space in zone */
    rt_int32_t  z_nused;        /* total used chunks / ualloc space in zone */
    rt_int32_t  z_nmax;         /* maximum free chunks */

    struct slab_zone *z_next;   /* zoneary[] link if z_nfree non-zero */
    rt_uint8_t  *z_baseptr;     /* pointer to start of chunk array */

    rt_int32_t  z_uindex;       /* current initial allocation index */
    rt_int32_t  z_chunksize;    /* chunk size for validation */

    rt_int32_t  z_zoneindex;    /* zone index */
    slab_chunk  *z_freechunk;   /* free chunk list */
#ifdef CONFIG_SLAB_DEBUG
    slab_chunk_debug_info *z_debug_info;
#endif
} slab_zone;

#define ZALLOC_SLAB_MAGIC       0x51ab51ab
#define ZALLOC_ZONE_LIMIT       (16 * 1024)     /* max slab-managed alloc */
#define ZALLOC_MIN_ZONE_SIZE    (32 * 1024)     /* minimum zone size */
#define ZALLOC_MAX_ZONE_SIZE    (128 * 1024)    /* maximum zone size */
#define NZONES                  72              /* number of zones */
#define ZONE_RELEASE_THRESH     2               /* threshold number of zones */

static slab_zone *zone_array[NZONES];   /* linked list of zones NFree > 0 */
static slab_zone *zone_free;            /* whole zones that have become free */

static int zone_free_cnt;
static int zone_size;
static int zone_limit;
static int zone_page_cnt;

#ifdef CONFIG_SLAB_DEBUG
static slab_zone *zone_outof[NZONES];   /* linked list of zones out of chunk */
static int check_memory_zone_chunk(slab_zone *z, void *ptr, rt_int32_t size);
#endif

/*
 * Misc constants.  Note that allocations that are exact multiples of
 * RT_MM_PAGE_SIZE, or exceed the zone limit, fall through to the kmem module.
 */
#define MIN_CHUNK_SIZE      8       /* in bytes */
#define MIN_CHUNK_MASK      (MIN_CHUNK_SIZE - 1)

/*
 * Array of descriptors that describe the contents of each page
 */
#define PAGE_TYPE_FREE      0x00
#define PAGE_TYPE_SMALL     0x01
#define PAGE_TYPE_LARGE     0x02
#define PAGE_TYPE_SHORT     0x03
struct memusage
{
    rt_uint32_t type: 2 ;       /* page type */
    rt_uint32_t size: 30;       /* pages allocated or offset from zone */
#ifdef CONFIG_SLAB_DEBUG
#ifdef CONFIG_DEBUG_BACKTRACE
    void *backtrace[CONFIG_CHUNK_BACKTRACE_LEVEL];
#endif
    /* next allocated page buffer */
    struct memusage *next;
    /* user truely need size  */
    rt_uint32_t need_size;
    /* page buffer pointer  */
    void *ptr;
    rt_thread_t alloc_thread;
#endif
};
static struct memusage *memusage = RT_NULL;
#define btokup(addr)    \
    (&memusage[((rt_ubase_t)(addr) - heap_start) >> RT_MM_PAGE_BITS])

static rt_ubase_t heap_start, heap_end;

/* page allocator */
struct rt_page_head
{
    struct rt_page_head *next;      /* next valid page */
    rt_size_t page;                 /* number of page  */

    /* dummy */
    char dummy[RT_MM_PAGE_SIZE - (sizeof(struct rt_page_head *) + sizeof(rt_size_t))];
};
static struct rt_page_head *rt_page_list;

#ifdef CONFIG_SLAB_DEBUG
static struct memusage *allocated_list; /* linked list of allocated pages */
static int check_memory_page_chunk(struct memusage *kup);
#endif

static rt_ubase_t heap_lock(void)
{
    rt_ubase_t interrupt_level;
    interrupt_level = rt_hw_interrupt_disable();
    return interrupt_level;
}

static void heap_unlock(rt_ubase_t level)
{
    rt_hw_interrupt_enable(level);
}

static void heap_lock_init(void)
{
}

#ifdef CONFIG_KASAN
static void *rt_memset(void *s, int c, size_t count)
{
    char *xs = (char *)s;

    while (count--)
    {
        *xs++ = c;
    }

    return s;
}

static void *rt_memcpy(void *dst, const void *src, size_t count)
{
    char *tmp = (char *)dst, *s = (char *)src;
    int len;

    if (tmp <= s || tmp > (s + count))
    {
        while (count--)
        {
            *tmp ++ = *s ++;
        }
    }
    else
    {
        for (len = count; len > 0; len --)
        {
            tmp[len - 1] = s[len - 1];
        }
    }

    return dst;
}

static int32_t rt_memcmp(const void *cs, const void *ct, size_t count)
{
    const unsigned char *su1, *su2;
    int res = 0;

    for (su1 = (const unsigned char *)cs, su2 = (const unsigned char *)ct; 0 < count; ++su1, ++su2, count--)
    {
        if ((res = *su1 - *su2) != 0)
        {
            break;
        }
	}
    return res;
}

#endif

static void *__rt_page_alloc(rt_size_t npages)
{
    struct rt_page_head *b, *n;
    struct rt_page_head **prev;

    rt_ubase_t lock_value;

    if (npages == 0)
    {
        return RT_NULL;
    }

    /* lock heap */
    lock_value = heap_lock();
    for (prev = &rt_page_list; (b = *prev) != RT_NULL; prev = &(b->next))
    {
        if (b->page > npages)
        {
            /* splite pages */
            n       = b + npages;
            n->next = b->next;
            n->page = b->page - npages;
            *prev   = n;
            break;
        }

        if (b->page == npages)
        {
            /* this node fit, remove this node */
            *prev = b->next;
            break;
        }
    }

    /* unlock heap */
    heap_unlock(lock_value);

    return b;
}

void *rt_page_alloc(rt_size_t npages)
{
    struct memusage *kup;
    void *ptr;
    rt_ubase_t lock_value;

#ifdef CONFIG_SLAB_DEBUG
    rt_ubase_t interrupt_level;
    npages += 2;
#endif

    ptr = __rt_page_alloc(npages);
    if (ptr == RT_NULL)
    {
        return RT_NULL;
    }

    lock_value = heap_lock();

#ifdef CONFIG_SLAB_DEBUG
    /* need to disable interrrupt, because rt_schedule will be called in irq handler */
    interrupt_level = rt_hw_interrupt_disable();
#ifdef CONFIG_KASAN
    kasan_disable_report();
#endif
#endif

    kup = btokup(ptr);
    kup->type = PAGE_TYPE_SHORT;
    kup->size = npages;

#ifdef CONFIG_SLAB_DEBUG
    kup->ptr = ptr;
    kup->need_size = (npages - 2) * RT_MM_PAGE_SIZE;
    kup->next = allocated_list;
    kup->alloc_thread = xTaskGetCurrentTaskHandle();
    allocated_list = kup;

    /* copy debug magic to the first 8 bytes and the last 8 bytes.  */
    rt_memcpy((rt_uint8_t *)ptr + RT_MM_PAGE_SIZE - sizeof(slab_chunk_debug_magic), &chunk_debug_magic, sizeof(slab_chunk_debug_magic));
    rt_memcpy((rt_uint8_t *)ptr + RT_MM_PAGE_SIZE + kup->need_size,
              &chunk_debug_magic,
              sizeof(slab_chunk_debug_magic));
    ptr = (void *)((rt_uint8_t *)ptr + RT_MM_PAGE_SIZE);
#ifdef CONFIG_DEBUG_BACKTRACE
    rt_memset(kup->backtrace, 0, sizeof(kup->backtrace));
    /* get backtrace information */
    backtrace(RT_NULL, kup->backtrace, CONFIG_CHUNK_BACKTRACE_LEVEL, 0, RT_NULL);
#endif
#ifdef CONFIG_KASAN
    kasan_enable_report();
#endif
    npages -= 2;
    RT_OBJECT_HOOK_CALL(rt_page_alloc_hook, ((char *)ptr, npages));
    rt_hw_interrupt_enable(interrupt_level);
#endif

    heap_unlock(lock_value);

    return ptr;
}


static void __rt_page_free(void *addr, rt_size_t npages)
{
    struct rt_page_head *b, *n;
    struct rt_page_head **prev;
    rt_ubase_t lock_value;

    RT_ASSERT(addr != RT_NULL);
    RT_ASSERT((rt_ubase_t)addr % RT_MM_PAGE_SIZE == 0);
    RT_ASSERT(npages != 0);

    n = (struct rt_page_head *)addr;

    /* lock heap */
    lock_value = heap_lock();

    for (prev = &rt_page_list; (b = *prev) != RT_NULL; prev = &(b->next))
    {
        RT_ASSERT(b->page > 0);
        RT_ASSERT(b > n || b + b->page <= n);

        if (b + b->page == n)
        {
            if (b + (b->page += npages) == b->next)
            {
                b->page += b->next->page;
                b->next  = b->next->next;
            }

            goto _return;
        }

        if (b == n + npages)
        {
            n->page = b->page + npages;
            n->next = b->next;
            *prev   = n;

            goto _return;
        }

        if (b > n + npages)
        {
            break;
        }
    }

    n->page = npages;
    n->next = b;
    *prev   = n;

_return:
    /* unlock heap */
    heap_unlock(lock_value);
}



void rt_page_free(void *addr, rt_size_t npages)
{
    struct memusage *kup;
    rt_ubase_t lock_value;

    if (addr == RT_NULL || npages == 0)
    {
        return;
    }

#ifdef CONFIG_SLAB_DEBUG
    rt_ubase_t interrupt_level;
    /* the true page num  */
    npages += 2;
    addr = (void *)((rt_uint8_t *)addr - RT_MM_PAGE_SIZE);
#endif

    lock_value = heap_lock();

#ifdef CONFIG_SLAB_DEBUG
    interrupt_level = rt_hw_interrupt_disable();
#endif

    kup = btokup((rt_ubase_t)addr & ~RT_MM_PAGE_MASK);
    RT_ASSERT(kup->type == PAGE_TYPE_SHORT);
    kup->type = PAGE_TYPE_FREE;
    kup->size = 0;

#ifdef CONFIG_SLAB_DEBUG
    if (check_memory_page_chunk(kup))
    {
        rt_kprintf("free: meet corrupted chunk! chunk = %p, size = %d\n", kup->ptr, kup->need_size);
#ifdef CONFIG_DEBUG_BACKTRACE
        backtrace(RT_NULL, RT_NULL, 0, 0, slab_printf);
#endif
        while (1);
    }

    kup->alloc_thread = RT_NULL;
    kup->ptr = RT_NULL;
    kup->need_size = 0;
    {
        /* remove the free page buffer from allocated_list  */
        struct memusage *next, *prev;
        next = allocated_list;
        prev = next;
        for (; kup != next && next != RT_NULL;)
        {
            prev = next;
            next = next->next;
        }

        if (next == allocated_list)
        {
            allocated_list = kup->next;
        }
        else if (prev)
        {
            prev->next = kup->next;
        }
    }
    RT_OBJECT_HOOK_CALL(rt_page_free_hook, (addr, npages));
    rt_hw_interrupt_enable(interrupt_level);
#endif

    heap_unlock(lock_value);

    __rt_page_free(addr, npages);
}

/*
 * Initialize the page allocator
 */
static void rt_page_init(void *addr, rt_size_t npages)
{
    RT_ASSERT(addr != RT_NULL);
    RT_ASSERT(npages != 0);

    rt_page_list = RT_NULL;
    __rt_page_free(addr, npages);
}

/**
 * @ingroup SystemInit
 *
 * This function will init system heap
 *
 * @param begin_addr the beginning address of system page
 * @param end_addr the end address of system page
 */
void rt_system_heap_init(void *begin_addr, void *end_addr)
{
    rt_uint32_t limsize, npages;
    struct memusage *kup;

    RT_DEBUG_NOT_IN_INTERRUPT;

    /* align begin and end addr to page */
    heap_start = RT_ALIGN((rt_ubase_t)begin_addr, RT_MM_PAGE_SIZE);
    heap_end   = RT_ALIGN_DOWN((rt_ubase_t)end_addr, RT_MM_PAGE_SIZE);

    if (heap_start >= heap_end)
    {
        rt_kprintf("rt_system_heap_init, wrong address[0x%x - 0x%x]\n",
                   (rt_ubase_t)begin_addr, (rt_ubase_t)end_addr);

        return;
    }

    limsize = heap_end - heap_start;
    npages  = limsize / RT_MM_PAGE_SIZE;

    /* initialize heap semaphore */
    heap_lock_init();
    //heap_sem = xSemaphoreCreateCountingStatic(10, 1, &static_heap_sem);

    RT_DEBUG_LOG(RT_DEBUG_SLAB, ("heap[0x%x - 0x%x], size 0x%x, 0x%x pages\n",
                                 heap_start, heap_end, limsize, npages));

    /* init pages */
    rt_page_init((void *)heap_start, npages);

    /* calculate zone size */
    zone_size = ZALLOC_MIN_ZONE_SIZE;
    while (zone_size < ZALLOC_MAX_ZONE_SIZE && (zone_size << 1) < (limsize / 1024))
    {
        zone_size <<= 1;
    }

    zone_limit = zone_size / 4;
    if (zone_limit > ZALLOC_ZONE_LIMIT)
    {
        zone_limit = ZALLOC_ZONE_LIMIT;
    }

    zone_page_cnt = zone_size / RT_MM_PAGE_SIZE;

    RT_DEBUG_LOG(RT_DEBUG_SLAB, ("zone size 0x%x, zone page count 0x%x\n",
                                 zone_size, zone_page_cnt));

    /* allocate memusage array */
    limsize  = npages * sizeof(struct memusage);
    limsize  = RT_ALIGN(limsize, RT_MM_PAGE_SIZE);
    memusage = __rt_page_alloc(limsize / RT_MM_PAGE_SIZE);
    if (memusage == RT_NULL)
    {
        configASSERT(0);
    }

    rt_memset(memusage, 0x00, limsize);
    /* set kup */
    if (limsize > zone_size)
    {
        kup = btokup(memusage);
        kup->type = PAGE_TYPE_LARGE;
        kup->size = limsize >> RT_MM_PAGE_BITS;
    }
    else
    {
        rt_int32_t off;

        for (off = 0, kup = btokup(memusage); off < limsize / RT_MM_PAGE_SIZE; off ++)
        {
            kup->type = PAGE_TYPE_SMALL;
            kup->size = off;
            kup ++;
        }
    }

    RT_DEBUG_LOG(RT_DEBUG_SLAB, ("memusage 0x%x, size 0x%x\n",
                                 (rt_ubase_t)memusage, limsize));
#ifdef CONFIG_CPU_PRIVATE_DATA
	{
		/* alloc private data memory of each cpu */
		for(int i = 0;i<configNR_CPUS;i++){
			unsigned long pptr = cpu_private_data_paddr - i * (1024*1024);
			if( NULL == rt_page_alloc_addr(pptr, cpu_private_data_size/RT_MM_PAGE_SIZE) ){
				extern int printk(const char *fmt, ...);
				printk("failed, addr:%08lx, size:%d, npages:%d\r\n",
						pptr,
						cpu_private_data_size,
						cpu_private_data_size/RT_MM_PAGE_SIZE);
				while(1);
			}
			rt_memset((void *)pptr, 0, cpu_private_data_size);
		}
		extern void vInitCpuPrivateReent( void *paddr, void *vaddr  );
		extern void vInitIrqReent( void );
		vInitIrqReent();
		for(int i = 0;i<configNR_CPUS;i++){
			vInitCpuPrivateReent( (void *)(cpu_private_data_paddr - (unsigned long)(i * (1024*1024))), (void *)cpu_private_data_vaddr);
		}
	}
#endif
}

/*
 * Calculate the zone index for the allocation request size and set the
 * allocation request size to that particular zone's chunk size.
 */
rt_inline int zoneindex(rt_size_t *bytes)
{
    /* unsigned for shift opt */
    rt_ubase_t n = (rt_ubase_t)(*bytes);

    if (n < 128)
    {
        *bytes = n = (n + 7) & ~7;

        /* 8 byte chunks, 16 zones */
        return (n / 8 - 1);
    }
    if (n < 256)
    {
        *bytes = n = (n + 15) & ~15;

        return (n / 16 + 7);
    }
    if (n < 8192)
    {
        if (n < 512)
        {
            *bytes = n = (n + 31) & ~31;

            return (n / 32 + 15);
        }
        if (n < 1024)
        {
            *bytes = n = (n + 63) & ~63;

            return (n / 64 + 23);
        }
        if (n < 2048)
        {
            *bytes = n = (n + 127) & ~127;

            return (n / 128 + 31);
        }
        if (n < 4096)
        {
            *bytes = n = (n + 255) & ~255;

            return (n / 256 + 39);
        }
        *bytes = n = (n + 511) & ~511;

        return (n / 512 + 47);
    }
    if (n < 16384)
    {
        *bytes = n = (n + 1023) & ~1023;

        return (n / 1024 + 55);
    }

    rt_kprintf("Unexpected byte count %d", n);

    return 0;
}

/**
 * @addtogroup MM
 */

/**@{*/

/**
 * This function will allocate a block from system heap memory.
 * - If the nbytes is less than zero,
 * or
 * - If there is no nbytes sized memory valid in system,
 * the RT_NULL is returned.
 *
 * @param size the size of memory to be allocated
 *
 * @return the allocated memory
 */
void *__internal_malloc(rt_size_t size)
{
    slab_zone *z;
    rt_int32_t zi;
    slab_chunk *chunk;
    struct memusage *kup;
    rt_ubase_t lock_value;

#ifdef CONFIG_SLAB_DEBUG
    rt_size_t need_size = size;
    rt_ubase_t interrupt_level;
#endif

    /* zero size, return RT_NULL */
    if (size == 0)
    {
        return RT_NULL;
    }

#ifdef CONFIG_SLAB_DEBUG
    /* add two sizeof(slab_chunk_debug_magic) for debug */
    size += 2 * sizeof(slab_chunk_debug_magic);
#endif

    /*
     * Handle large allocations directly.  There should not be very many of
     * these so performance is not a big issue.
     */
    if (size >= zone_limit)
    {
        size = RT_ALIGN(size, RT_MM_PAGE_SIZE);
#ifdef CONFIG_SLAB_DEBUG
        size += 2 * RT_MM_PAGE_SIZE;
#endif

        chunk = __rt_page_alloc(size >> RT_MM_PAGE_BITS);
        if (chunk == RT_NULL)
        {
            return RT_NULL;
        }

        /* set kup */
        kup = btokup(chunk);
        kup->type = PAGE_TYPE_LARGE;
        kup->size = size >> RT_MM_PAGE_BITS;

        RT_DEBUG_LOG(RT_DEBUG_SLAB,
                     ("malloc a large memory 0x%x, page cnt %d, kup %d\n",
                      size,
                      size >> RT_MM_PAGE_BITS,
                      ((rt_ubase_t)chunk - heap_start) >> RT_MM_PAGE_BITS));

        /* lock heap */
        lock_value = heap_lock();

#ifdef CONFIG_SLAB_DEBUG
        interrupt_level = rt_hw_interrupt_disable();
#ifdef CONFIG_KASAN
        kasan_disable_report();
#endif

        kup->alloc_thread = xTaskGetCurrentTaskHandle();
        kup->ptr = chunk;
        kup->need_size = need_size;
        kup->next = allocated_list;
        allocated_list = kup;

        /* copy debug magic to the first 8 bytes and the last 8 bytes.  */
        rt_memcpy((rt_uint8_t *)chunk + RT_MM_PAGE_SIZE - sizeof(slab_chunk_debug_magic), &chunk_debug_magic, sizeof(slab_chunk_debug_magic));
        rt_memcpy((rt_uint8_t *)chunk + RT_MM_PAGE_SIZE + RT_ALIGN(kup->need_size, 8),
                  &chunk_debug_magic,
                  sizeof(slab_chunk_debug_magic));
        chunk = (void *)((rt_uint8_t *)chunk + RT_MM_PAGE_SIZE);

#ifdef CONFIG_DEBUG_BACKTRACE
        rt_memset(kup->backtrace, 0, sizeof(kup->backtrace));
        backtrace(RT_NULL, kup->backtrace, CONFIG_CHUNK_BACKTRACE_LEVEL, 0, RT_NULL);
#endif
#endif

#ifdef RT_MEM_STATS
        used_mem += size;
        if (used_mem > max_mem)
        {
            max_mem = used_mem;
        }
#endif

#ifdef CONFIG_SLAB_DEBUG
        size = kup->need_size;
        RT_OBJECT_HOOK_CALL(rt_malloc_large_hook, ((char *)chunk, kup->need_size));
        goto done1;
#else
        goto done;
#endif
    }

    /* lock heap */
    lock_value = heap_lock();

#ifdef CONFIG_SLAB_DEBUG
    interrupt_level = rt_hw_interrupt_disable();
#ifdef CONFIG_KASAN
    kasan_disable_report();
#endif
#endif

    /*
     * Attempt to allocate out of an existing zone.  First try the free list,
     * then allocate out of unallocated space.  If we find a good zone move
     * it to the head of the list so later allocations find it quickly
     * (we might have thousands of zones in the list).
     *
     * Note: zoneindex() will panic of size is too large.
     */
    zi = zoneindex(&size);
    RT_ASSERT(zi < NZONES);

    RT_DEBUG_LOG(RT_DEBUG_SLAB, ("try to malloc 0x%x on zone: %d\n", size, zi));

    if ((z = zone_array[zi]) != RT_NULL)
    {
        RT_ASSERT(z->z_nfree > 0);

        /* Remove us from the zone_array[] when we become empty */
        if (--z->z_nfree == 0)
        {
            zone_array[zi] = z->z_next;
            z->z_next = RT_NULL;
#ifdef CONFIG_SLAB_DEBUG
            z->z_next = zone_outof[zi];
            zone_outof[zi] = z;
#endif
        }

        z->z_nused ++;
        RT_ASSERT((z->z_nused + z->z_nfree) == z->z_nmax);
        /*
         * No chunks are available but nfree said we had some memory, so
         * it must be available in the never-before-used-memory area
         * governed by uindex.  The consequences are very serious if our zone
         * got corrupted so we use an explicit rt_kprintf rather then a KASSERT.
         */
        if (z->z_uindex + 1 != z->z_nmax)
        {
            z->z_uindex = z->z_uindex + 1;
            chunk = (slab_chunk *)(z->z_baseptr + z->z_uindex * size);
        }
        else
        {
            /* find on free chunk list */
            chunk = z->z_freechunk;

            /* remove this chunk from list */
            z->z_freechunk = z->z_freechunk->c_next;
        }

#ifdef RT_MEM_STATS
        used_mem += z->z_chunksize;
        if (used_mem > max_mem)
        {
            max_mem = used_mem;
        }
#endif

#ifdef CONFIG_SLAB_DEBUG
        {
            rt_int32_t index = ((rt_uint8_t *)chunk - z->z_baseptr) / size;
            z->z_debug_info[index].chunk_alloc_flag = 1;
            z->z_debug_info[index].need_size = need_size;
            z->z_debug_info[index].alloc_thread = xTaskGetCurrentTaskHandle();
#ifdef CONFIG_DEBUG_BACKTRACE
            rt_memset(z->z_debug_info[z->z_uindex].backtrace, 0, sizeof(z->z_debug_info[z->z_uindex].backtrace));
            backtrace(RT_NULL, z->z_debug_info[z->z_uindex].backtrace, CONFIG_CHUNK_BACKTRACE_LEVEL, 0, RT_NULL);
#endif
        }
#endif
        goto done;
    }

    /*
     * If all zones are exhausted we need to allocate a new zone for this
     * index.
     *
     * At least one subsystem, the tty code (see CROUND) expects power-of-2
     * allocations to be power-of-2 aligned.  We maintain compatibility by
     * adjusting the base offset below.
     */
    {
        rt_int32_t off;

        if ((z = zone_free) != RT_NULL)
        {
            /* remove zone from free zone list */
            zone_free = z->z_next;
            -- zone_free_cnt;
        }
        else
        {
#ifdef CONFIG_SLAB_DEBUG
            rt_hw_interrupt_enable(interrupt_level);
#ifdef CONFIG_KASAN
            kasan_enable_report();
#endif
#endif
            /* unlock heap, since page allocator will think about lock */
            heap_unlock(lock_value);

            /* allocate a zone from page */
            z = __rt_page_alloc(zone_size / RT_MM_PAGE_SIZE);
            if (z == RT_NULL)
            {
                chunk = RT_NULL;
                goto __exit;
            }

            /* lock heap */
            lock_value = heap_lock();

#ifdef CONFIG_SLAB_DEBUG
            interrupt_level = rt_hw_interrupt_disable();
#ifdef CONFIG_KASAN
            kasan_disable_report();
#endif
#endif

            RT_DEBUG_LOG(RT_DEBUG_SLAB, ("alloc a new zone: 0x%x\n",
                                         (rt_ubase_t)z));

            /* set message usage */
            for (off = 0, kup = btokup(z); off < zone_page_cnt; off ++)
            {
                kup->type = PAGE_TYPE_SMALL;
                kup->size = off;

                kup ++;
            }
        }

        /* clear to zero */
        rt_memset(z, 0, sizeof(slab_zone));

        /* offset of slab zone struct in zone */
        off = sizeof(slab_zone);

        /*
         * Guarentee power-of-2 alignment for power-of-2-sized chunks.
         * Otherwise just 8-byte align the data.
         */
        if ((size | (size - 1)) + 1 == (size << 1))
        {
            off = (off + size - 1) & ~(size - 1);
        }
        else
        {
            off = (off + MIN_CHUNK_MASK) & ~MIN_CHUNK_MASK;
        }

        z->z_magic     = ZALLOC_SLAB_MAGIC;
        z->z_zoneindex = zi;
#ifndef CONFIG_SLAB_DEBUG
        z->z_nmax      = (zone_size - off) / size;
#else
        z->z_nmax      = (zone_size - off) / (size + sizeof(slab_chunk_debug_info));
#endif
        z->z_nfree     = z->z_nmax - 1;
        z->z_nused     = 1;
#ifndef CONFIG_SLAB_DEBUG
        z->z_baseptr   = (rt_uint8_t *)z + off;
#else
        z->z_baseptr   = (rt_uint8_t *)z + off + z->z_nmax * sizeof(slab_chunk_debug_info);
#endif
        z->z_uindex    = 0;
        z->z_chunksize = size;

        RT_ASSERT((z->z_nused + z->z_nfree) == z->z_nmax);

        chunk = (slab_chunk *)(z->z_baseptr + z->z_uindex * size);

        /* link to zone array */
        z->z_next = zone_array[zi];
        zone_array[zi] = z;

#ifdef CONFIG_SLAB_DEBUG
        z->z_debug_info = (slab_chunk_debug_info *)((rt_uint8_t *)z + off);
        rt_memset(z->z_debug_info, 0, z->z_nmax * sizeof(slab_chunk_debug_info));
        z->z_debug_info[z->z_uindex].chunk_alloc_flag = 1;
        z->z_debug_info[z->z_uindex].need_size = need_size;
        z->z_debug_info[z->z_uindex].alloc_thread = xTaskGetCurrentTaskHandle();
#ifdef CONFIG_DEBUG_BACKTRACE
        backtrace(RT_NULL, z->z_debug_info[z->z_uindex].backtrace, CONFIG_CHUNK_BACKTRACE_LEVEL, 0, RT_NULL);
#endif
#endif

#ifdef RT_MEM_STATS
        used_mem += z->z_chunksize;
        if (used_mem > max_mem)
        {
            max_mem = used_mem;
        }
#endif
    }

done:
#ifdef CONFIG_SLAB_DEBUG
    {
        rt_int32_t align_size = RT_ALIGN(need_size, 8);
        rt_memcpy(chunk, &chunk_debug_magic, sizeof(slab_chunk_debug_magic));
        rt_memcpy((rt_uint8_t *)chunk + sizeof(slab_chunk_debug_magic) + align_size,
                  &chunk_debug_magic,
                  sizeof(slab_chunk_debug_magic));
        chunk = (slab_chunk *)((rt_uint8_t *)chunk + sizeof(slab_chunk_debug_magic));
        size = need_size;
        RT_OBJECT_HOOK_CALL(rt_malloc_small_hook, ((char *)chunk, need_size));
    }

done1:
#ifdef CONFIG_KASAN
    kasan_enable_report();
#endif
    rt_hw_interrupt_enable(interrupt_level);
#endif

    heap_unlock(lock_value);
    RT_OBJECT_HOOK_CALL(rt_malloc_hook, ((char *)chunk, size));

__exit:
    return chunk;
}


/**
 * This function will change the size of previously allocated memory block.
 *
 * @param ptr the previously allocated memory block
 * @param size the new size of memory block
 *
 * @return the allocated memory
 */
void *rt_realloc(void *ptr, rt_size_t size)
{
    void *nptr;
    slab_zone *z;
    struct memusage *kup;

    if (ptr == RT_NULL)
    {
        return rt_malloc(size);
    }
    if (size == 0)
    {
        rt_free(ptr);

        return RT_NULL;
    }

#ifdef CONFIG_SLAB_DEBUG
    rt_size_t need_size = size;
    ptr = (void *)((unsigned long)ptr - sizeof(slab_chunk_debug_magic));
#endif

    /*
     * Get the original allocation's zone.  If the new request winds up
     * using the same chunk size we do not have to do anything.
     */
    kup = btokup((rt_ubase_t)ptr & ~RT_MM_PAGE_MASK);
    if (kup->type == PAGE_TYPE_LARGE)
    {
        rt_size_t osize;

        osize = kup->size << RT_MM_PAGE_BITS;
        if ((nptr = rt_malloc(size)) == RT_NULL)
        {
            return RT_NULL;
        }
#ifdef CONFIG_SLAB_DEBUG
        ptr = (void *)((unsigned long)ptr + sizeof(slab_chunk_debug_magic));
        osize = kup->need_size;
#endif
        rt_memcpy(nptr, ptr, size > osize ? osize : size);
        rt_free(ptr);

        return nptr;
    }
    else if (kup->type == PAGE_TYPE_SMALL)
    {
        z = (slab_zone *)(((rt_ubase_t)ptr & ~RT_MM_PAGE_MASK) -
                          kup->size * RT_MM_PAGE_SIZE);
        RT_ASSERT(z->z_magic == ZALLOC_SLAB_MAGIC);

#ifdef CONFIG_SLAB_DEBUG
        size += 2 * sizeof(slab_chunk_debug_magic);
        ptr = (void *)((unsigned long)ptr + sizeof(slab_chunk_debug_magic));
#endif
        zoneindex(&size);
        if (z->z_chunksize == size)
        {
#ifdef CONFIG_SLAB_DEBUG
            rt_int32_t same_index = ((rt_uint8_t *)ptr - sizeof(slab_chunk_debug_magic) - z->z_baseptr) / (z->z_chunksize);
            z->z_debug_info[same_index].need_size = need_size;
            z->z_debug_info[same_index].alloc_thread = xTaskGetCurrentTaskHandle();
#ifndef CONFIG_KASAN
            rt_int32_t align_size = RT_ALIGN(need_size, 8);
            rt_memcpy((rt_uint8_t *)ptr - sizeof(slab_chunk_debug_magic), &chunk_debug_magic, sizeof(slab_chunk_debug_magic));
            rt_memcpy((rt_uint8_t *)ptr + align_size, &chunk_debug_magic, sizeof(slab_chunk_debug_magic));
#endif
            RT_OBJECT_HOOK_CALL(rt_realloc_small_hook, ((char *)ptr, need_size));
#endif
            return (ptr); /* same chunk */
        }

#ifdef CONFIG_SLAB_DEBUG
        size = need_size;
#endif
        /*
         * Allocate memory for the new request size.  Note that zoneindex has
         * already adjusted the request size to the appropriate chunk size, which
         * should optimize our bcopy().  Then copy and return the new pointer.
         */
        if ((nptr = rt_malloc(size)) == RT_NULL)
        {
            return RT_NULL;
        }

#ifdef CONFIG_SLAB_DEBUG
        rt_int32_t index = ((rt_uint8_t *)ptr - sizeof(slab_chunk_debug_magic) - z->z_baseptr) / (z->z_chunksize);
        rt_int32_t old_need_size = z->z_debug_info[index].need_size;

        rt_memcpy(nptr, ptr, (size > old_need_size) ? old_need_size : size);
#else
        rt_memcpy(nptr, ptr, size > z->z_chunksize ? z->z_chunksize : size);
#endif
        rt_free(ptr);

        return nptr;
    }

    return RT_NULL;
}


/**
 * This function will contiguously allocate enough space for count objects
 * that are size bytes of memory each and returns a pointer to the allocated
 * memory.
 *
 * The allocated memory is filled with bytes of value zero.
 *
 * @param count number of objects to allocate
 * @param size size of the objects to allocate
 *
 * @return pointer to allocated memory / NULL pointer if there is an error
 */
void *rt_calloc(rt_size_t count, rt_size_t size)
{
    void *p;

    /* allocate 'count' objects of size 'size' */
    p = rt_malloc(count * size);

    /* zero the memory */
    if (p)
    {
        rt_memset(p, 0, count * size);
    }

    return p;
}


/**
 * This function will release the previous allocated memory block by rt_malloc.
 * The released memory block is taken back to system heap.
 *
 * @param ptr the address of memory which will be released
 */
void __internal_free(void *ptr)
{
    slab_zone *z;
    slab_chunk *chunk;
    struct memusage *kup;
    rt_ubase_t lock_value;

    /* free a RT_NULL pointer */
    if (ptr == RT_NULL)
    {
        return ;
    }

    RT_OBJECT_HOOK_CALL(rt_free_hook, (ptr));

#ifdef CONFIG_SLAB_DEBUG
    rt_ubase_t interrupt_level;
    ptr = (void *)((unsigned long)ptr - sizeof(slab_chunk_debug_magic));
#endif

    /* get memory usage */
#if RT_DEBUG_SLAB
    {
        rt_ubase_t addr = ((rt_ubase_t)ptr & ~RT_MM_PAGE_MASK);
        RT_DEBUG_LOG(RT_DEBUG_SLAB,
                     ("free a memory 0x%x and align to 0x%x, kup index %d\n",
                      (rt_ubase_t)ptr,
                      (rt_ubase_t)addr,
                      ((rt_ubase_t)(addr) - heap_start) >> RT_MM_PAGE_BITS));
    }
#endif

    kup = btokup((rt_ubase_t)ptr & ~RT_MM_PAGE_MASK);
    /* release large allocation */
    if (kup->type == PAGE_TYPE_LARGE)
    {
        rt_ubase_t size;

        /* lock heap */
        lock_value = heap_lock();

#ifdef CONFIG_SLAB_DEBUG
        interrupt_level = rt_hw_interrupt_disable();
#ifdef CONFIG_KASAN
        kasan_disable_report();
#endif
        RT_OBJECT_HOOK_CALL(rt_free_large_hook, ((char *)ptr + sizeof(slab_chunk_debug_magic), kup->need_size));

        ptr = (void *)((unsigned long)ptr + sizeof(slab_chunk_debug_magic) - RT_MM_PAGE_SIZE);

        if (check_memory_page_chunk(kup))
        {
            rt_kprintf("free: meet corrupted chunk! chunk = %p, size = %d\n", kup->ptr, kup->need_size);
#ifdef CONFIG_DEBUG_BACKTRACE
            backtrace(RT_NULL, RT_NULL, 0, 0, slab_printf);
#endif
            while (1);
        }
#endif

        /* clear page counter */
        size = kup->size;
        kup->size = 0;

#ifdef CONFIG_SLAB_DEBUG
        kup->ptr = RT_NULL;
        kup->need_size = 0;
        kup->alloc_thread = RT_NULL;
        {
            struct memusage *next, *prev;
            next = allocated_list;
            prev = next;
            for (; kup != next && next != RT_NULL;)
            {
                prev = next;
                next = next->next;
            }
            if (next == allocated_list)
            {
                allocated_list = kup->next;
            }
            else if (prev)
            {
                prev->next = kup->next;
            }
        }
#endif

#ifdef RT_MEM_STATS
        used_mem -= size * RT_MM_PAGE_SIZE;
#endif

#ifdef CONFIG_SLAB_DEBUG
        rt_hw_interrupt_enable(interrupt_level);
#ifdef CONFIG_KASAN
        kasan_enable_report();
#endif
#endif
        heap_unlock(lock_value);

        RT_DEBUG_LOG(RT_DEBUG_SLAB,
                     ("free large memory block 0x%x, page count %d\n",
                      (rt_ubase_t)ptr, size));

        /* free this page */
        __rt_page_free(ptr, size);

        return;
    }

    if (kup->type == PAGE_TYPE_FREE)
    {
        return;
    }

    /* lock heap */
    lock_value = heap_lock();

#ifdef CONFIG_SLAB_DEBUG
    interrupt_level = rt_hw_interrupt_disable();
#ifdef CONFIG_KASAN
    kasan_disable_report();
#endif
#endif

    /* zone case. get out zone. */
    z = (slab_zone *)(((rt_ubase_t)ptr & ~RT_MM_PAGE_MASK) -
                      kup->size * RT_MM_PAGE_SIZE);

    RT_ASSERT(z->z_magic == ZALLOC_SLAB_MAGIC);

#ifdef CONFIG_SLAB_DEBUG
    {
        rt_int32_t index = ((rt_uint8_t *)ptr - z->z_baseptr) / z->z_chunksize;
        z->z_debug_info[index].chunk_alloc_flag = 0;
        rt_int32_t need_size = z->z_debug_info[index].need_size;
        z->z_debug_info[index].need_size = 0;
        z->z_debug_info[index].alloc_thread = RT_NULL;
        if (check_memory_zone_chunk(z, ptr, RT_ALIGN(need_size, 8)))
        {
            rt_kprintf("free: meet corrupted chunk! chunk = %p, size = %d\n", ptr, need_size);
#ifdef CONFIG_DEBUG_BACKTRACE
            backtrace(RT_NULL, RT_NULL, 0, 0, slab_printf);
#endif
            while (1);
        }
        rt_memset(&z->z_debug_info[index], 0, sizeof(slab_chunk_debug_info));
        RT_OBJECT_HOOK_CALL(rt_free_small_hook, ((char *)ptr + sizeof(slab_chunk_debug_magic), need_size));
    }
#endif

    chunk          = (slab_chunk *)ptr;
    chunk->c_next  = z->z_freechunk;
    z->z_freechunk = chunk;

#ifdef RT_MEM_STATS
    used_mem -= z->z_chunksize;
#endif

    /*
     * Bump the number of free chunks.  If it becomes non-zero the zone
     * must be added back onto the appropriate list.
     */
    if (z->z_nfree++ == 0)
    {
#ifdef CONFIG_SLAB_DEBUG
        {
            slab_zone * next, *prev;
            next = zone_outof[z->z_zoneindex];
            prev = next;
            for (; z != next && next != RT_NULL;)
            {
                prev = next;
                next = next->z_next;
            }
            if (next == zone_outof[z->z_zoneindex])
            {
                zone_outof[z->z_zoneindex] = z->z_next;
            }
            else if (prev)
            {
                prev->z_next = z->z_next;
            }
        }
        z->z_next = RT_NULL;
#endif
        z->z_next = zone_array[z->z_zoneindex];
        zone_array[z->z_zoneindex] = z;
    }

    if (z->z_nfree > z->z_nmax)
    {
        rt_kprintf("slab error: nfree %d, nmax %d,chunksz %d, freeptr 0x%08x.\n", z->z_nfree, z->z_nmax, z->z_chunksize, (rt_uint32_t)ptr);
        z->z_nfree = z->z_nmax;
        z->z_nused = 0;
        heap_unlock(lock_value);
        return;
    }
    z->z_nused --;

    RT_ASSERT((z->z_nused + z->z_nfree) == z->z_nmax);
    /*
     * If the zone becomes totally free, and there are other zones we
     * can allocate from, move this zone to the FreeZones list.  Since
     * this code can be called from an IPI callback, do *NOT* try to mess
     * with kernel_map here.  Hysteresis will be performed at malloc() time.
     */
    if (z->z_nfree == z->z_nmax &&
        (z->z_next || zone_array[z->z_zoneindex] != z))
    {
        slab_zone **pz;

        RT_ASSERT(z->z_nused == 0);
        RT_DEBUG_LOG(RT_DEBUG_SLAB, ("free zone 0x%x, index=%d\n",
                                     (rt_ubase_t)z, z->z_zoneindex));

        /* remove zone from zone array list */
        for (pz = &zone_array[z->z_zoneindex]; z != *pz; pz = &(*pz)->z_next)
            ;
        *pz = z->z_next;

        /* reset zone */
        z->z_magic = -1;

        /* insert to free zone list */
        z->z_next = zone_free;
        zone_free = z;

        ++ zone_free_cnt;

        /* release zone to page allocator */
        if (zone_free_cnt > ZONE_RELEASE_THRESH)
        {
            register rt_base_t i;

            z         = zone_free;
            zone_free = z->z_next;
            -- zone_free_cnt;

            /* set message usage */
            for (i = 0, kup = btokup(z); i < zone_page_cnt; i ++)
            {
                kup->type = PAGE_TYPE_FREE;
                kup->size = 0;
                kup ++;
            }

#ifdef CONFIG_SLAB_DEBUG
            rt_hw_interrupt_enable(interrupt_level);
#ifdef CONFIG_KASAN
            kasan_enable_report();
#endif
#endif

            /* unlock heap */
            heap_unlock(lock_value);

            /* release pages */
            __rt_page_free(z, zone_size / RT_MM_PAGE_SIZE);

            return;
        }
    }

#ifdef CONFIG_SLAB_DEBUG
    rt_hw_interrupt_enable(interrupt_level);
#ifdef CONFIG_KASAN
    kasan_enable_report();
#endif
#endif

    /* unlock heap */
    heap_unlock(lock_value);
}

#ifdef RT_MEM_STATS
void rt_memory_info(rt_uint32_t *total,
                    rt_uint32_t *used,
                    rt_uint32_t *max_used)
{
    if (total != RT_NULL)
    {
        *total = heap_end - heap_start;
    }

    if (used  != RT_NULL)
    {
        *used = used_mem;
    }

    if (max_used != RT_NULL)
    {
        *max_used = max_mem;
    }
}

#ifdef CONFIG_SLAB_DEBUG

static int check_memory_zone_chunk(slab_zone *z, void *ptr, rt_int32_t size)
{
#ifndef CONFIG_KASAN
    slab_chunk_debug_magic *left_magic;
    slab_chunk_debug_magic *right_magic;

    left_magic = (slab_chunk_debug_magic *)(ptr);
    right_magic = (slab_chunk_debug_magic *)(ptr + size + sizeof(chunk_debug_magic));

    if (rt_memcmp(&(left_magic->redzone), &(chunk_debug_magic.redzone), sizeof(chunk_debug_magic.redzone)))
    {
        rt_kprintf("left corrupted\r\n");
        rt_kprintf("redzone:0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x\r\n",
            left_magic->redzone[0],
            left_magic->redzone[1],
            left_magic->redzone[2],
            left_magic->redzone[3],
            left_magic->redzone[4],
            left_magic->redzone[5],
            left_magic->redzone[6],
            left_magic->redzone[7]);
        return -1;
    }

    if (rt_memcmp(&(right_magic->redzone),
                  &(chunk_debug_magic.redzone),
                  sizeof(chunk_debug_magic.redzone)))
    {
        rt_kprintf("right corrupted: size=%d, ptr=0x%p\r\n", size, ptr);
        rt_kprintf("redzone:0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x\r\n",
            right_magic->redzone[0],
            right_magic->redzone[1],
            right_magic->redzone[2],
            right_magic->redzone[3],
            right_magic->redzone[4],
            right_magic->redzone[5],
            right_magic->redzone[6],
            right_magic->redzone[7]);
        return -1;
    }
#endif
    return 0;
}


static int check_memory_page_chunk(struct memusage *kup)
{
#ifndef CONFIG_KASAN
    rt_uint8_t *ptr = kup->ptr;
    rt_uint32_t size = RT_ALIGN(kup->need_size, 8);

    slab_chunk_debug_magic *left_magic;
    slab_chunk_debug_magic *right_magic;

    left_magic = (slab_chunk_debug_magic *)(ptr + RT_MM_PAGE_SIZE - sizeof(chunk_debug_magic));
    right_magic = (slab_chunk_debug_magic *)(ptr + RT_MM_PAGE_SIZE + size);

    if (rt_memcmp(&(left_magic->redzone), &(chunk_debug_magic.redzone), sizeof(chunk_debug_magic.redzone)))
    {
        rt_kprintf("left page corrupted\r\n");
        return -1;
    }

    if (rt_memcmp(&(right_magic->redzone),
                  &(chunk_debug_magic.redzone),
                  sizeof(chunk_debug_magic.redzone)))
    {
        rt_kprintf("right page corrupted\r\n");
        return -1;
    }
#endif
    return 0;
}

#ifdef CONFIG_KASAN
int print_memory_address_description(void *addr)
{
    int i, j, m;

    slab_chunk *chunk;
    slab_zone *z;
    int corrupted_chunk = 0;
    int corrupted_page = 0;
    struct memusage *next, *prev;
    struct memusage *kup;
    rt_uint8_t *left_bound;
    rt_uint8_t *right_bound;
    rt_thread_t alloc_thread;

    kup = allocated_list;
    for (; kup != RT_NULL;)
    {
        left_bound = (rt_uint8_t *)kup->ptr;
        right_bound = (rt_uint8_t *)kup->ptr + kup->size * RT_MM_PAGE_SIZE;
        if ((left_bound < (rt_uint8_t *)addr) && (right_bound > (rt_uint8_t *)addr))
        {
            alloc_thread = kup->alloc_thread;
            rt_kprintf("page chunk check corrupted!, thread(%s), chunk = %p, size = %d\n",
                       (alloc_thread != RT_NULL) ? pcTaskGetName(alloc_thread) : "unknown",
                       (rt_uint8_t *)kup->ptr - sizeof(slab_chunk_debug_magic),
                       kup->need_size);
            rt_kprintf("Allocator Backtrace:\n");
#ifdef CONFIG_DEBUG_BACKTRACE
            for (m = 0; m < CONFIG_CHUNK_BACKTRACE_LEVEL; m++)
            {
                if (kup->backtrace[m] == RT_NULL)
                {
                    break;
                }
                rt_kprintf("backtrace : %p\n", kup->backtrace[m]);
            }
            rt_kprintf("\n");
#endif
            goto exit;
        }
        kup = kup->next;
    }

    for (i = 0; i < NZONES; i++)
    {
        z = zone_outof[i];
        while (z)
        {
            for (j = 0; j < (z->z_nmax - 1); j++)
            {
                chunk = (slab_chunk *)(z->z_baseptr + j * z->z_chunksize);
                left_bound = (rt_uint8_t *)chunk;
                right_bound = (rt_uint8_t *)chunk + z->z_chunksize;
                if ((left_bound < (rt_uint8_t *)addr) && (right_bound > (rt_uint8_t *)addr))
                {
                    alloc_thread = z->z_debug_info[j].alloc_thread;
                    rt_kprintf("%d: thread(%s) zone chunk check failed!, chunk = %p, size = %d\n",
                               __LINE__, (alloc_thread != RT_NULL) ? pcTaskGetName(alloc_thread) : "unknown",
                               (rt_uint8_t *)chunk + sizeof(slab_chunk_debug_magic),
                               z->z_debug_info[j].need_size);
                    rt_kprintf("Allocator Backtrace:\n");
#ifdef CONFIG_DEBUG_BACKTRACE
                    for (m = 0; m < CONFIG_CHUNK_BACKTRACE_LEVEL; m++)
                    {
                        if (z->z_debug_info[j].backtrace[m] == RT_NULL)
                        {
                            break;
                        }
                        rt_kprintf("backtrace : %p\n", z->z_debug_info[j].backtrace[m]);
                    }
                    rt_kprintf("\n");
#endif
                    goto exit;
                }
            }
            z = z->z_next;
        }
    }

    for (i = 0; i < NZONES; i++)
    {
        z = zone_array[i];
        while (z)
        {
            for (j = 0; j < (z->z_nmax - 1); j++)
            {
                if (z->z_debug_info[j].chunk_alloc_flag == 1)
                {
                    chunk = (slab_chunk *)(z->z_baseptr + j * z->z_chunksize);
                    left_bound = (rt_uint8_t *)chunk;
                    right_bound = (rt_uint8_t *)chunk + z->z_chunksize;
                    if ((left_bound < (rt_uint8_t *)addr) && (right_bound > (rt_uint8_t *)addr))
                    {
                        alloc_thread = z->z_debug_info[j].alloc_thread;
                        rt_kprintf("%d: thread(%s) zone chunk check failed!, chunk = %p, size = %d\n",
                                   __LINE__, (alloc_thread != RT_NULL) ?  pcTaskGetName(alloc_thread) : "unknown",
                                   (rt_uint8_t *)chunk + sizeof(slab_chunk_debug_magic),
                                   z->z_debug_info[j].need_size);
                        rt_kprintf("Allocator Backtrace:\n");
#ifdef CONFIG_DEBUG_BACKTRACE
                        for (m = 0; m < CONFIG_CHUNK_BACKTRACE_LEVEL; m++)
                        {
                            if (z->z_debug_info[j].backtrace[m] == RT_NULL)
                            {
                                break;
                            }
                            rt_kprintf("backtrace : %p\n", z->z_debug_info[j].backtrace[m]);
                        }
                        rt_kprintf("\n");
#endif
                        goto exit;
                    }
                }
            }
            z = z->z_next;
        }
    }
exit:
    return 0;
}
#endif

int check_memory_corrupt_info(void)
{
    int i, j, m;

    slab_chunk *chunk;
    slab_zone *z;
    int corrupted_chunk = 0;
    int corrupted_page = 0;
    struct memusage *next, *prev;
    struct memusage *kup;
    rt_thread_t alloc_thread;

#ifdef CONFIG_KASAN
    return 0;
#endif

    kup = allocated_list;
    for (; kup != RT_NULL;)
    {
        if (check_memory_page_chunk(kup))
        {
            alloc_thread = kup->alloc_thread;
            rt_kprintf("page chunk check corrupted!, thread(%s), chunk = %p, size = %d\n",
                       (alloc_thread != RT_NULL) ? pcTaskGetName(alloc_thread) : "unknown",
                       (rt_uint8_t *)kup->ptr - sizeof(slab_chunk_debug_magic),
                       kup->need_size);
#ifdef CONFIG_DEBUG_BACKTRACE
            for (m = 0; m < CONFIG_CHUNK_BACKTRACE_LEVEL; m++)
            {
                if (kup->backtrace[m] == RT_NULL)
                {
                    break;
                }
                rt_kprintf("backtrace : %p\n", kup->backtrace[m]);
            }
            rt_kprintf("\n");
#endif
            corrupted_page++;
        }
        kup = kup->next;
    }

    for (i = 0; i < NZONES; i++)
    {
        z = zone_outof[i];
        while (z)
        {
            for (j = 0; j < (z->z_nmax - 1); j++)
            {
                chunk = (slab_chunk *)(z->z_baseptr + j * z->z_chunksize);
                rt_size_t need_size = z->z_debug_info[j].need_size;
                if (check_memory_zone_chunk(z, chunk, RT_ALIGN(need_size, 8)))
                {
                    alloc_thread = z->z_debug_info[j].alloc_thread;
                    rt_kprintf("%d: thread(%s) zone chunk check failed!, chunk = %p, size = %d\n",
                               __LINE__, (alloc_thread != RT_NULL) ? pcTaskGetName(alloc_thread) : "unknown",
                               (rt_uint8_t *)chunk + sizeof(slab_chunk_debug_magic),
                               need_size);
#ifdef CONFIG_DEBUG_BACKTRACE
                    for (m = 0; m < CONFIG_CHUNK_BACKTRACE_LEVEL; m++)
                    {
                        if (z->z_debug_info[j].backtrace[m] == RT_NULL)
                        {
                            break;
                        }
                        rt_kprintf("backtrace : %p\n", z->z_debug_info[j].backtrace[m]);
                    }
                    rt_kprintf("\n");
#endif
                    corrupted_chunk++;
                }
            }
            z = z->z_next;
        }
    }

    for (i = 0; i < NZONES; i++)
    {
        z = zone_array[i];
        while (z)
        {
            for (j = 0; j < (z->z_nmax - 1); j++)
            {
                if (z->z_debug_info[j].chunk_alloc_flag == 1)
                {
                    rt_size_t need_size = z->z_debug_info[j].need_size;
                    chunk = (slab_chunk *)(z->z_baseptr + j * z->z_chunksize);
                    if (check_memory_zone_chunk(z, chunk, RT_ALIGN(need_size, 8)))
                    {
                        alloc_thread = z->z_debug_info[j].alloc_thread;
                        rt_kprintf("%d: thread(%s) zone chunk check failed!, chunk = %p, size = %d\n",
                                   __LINE__, (alloc_thread != RT_NULL) ? pcTaskGetName(alloc_thread) : "unknown",
                                   (rt_uint8_t *)chunk + sizeof(slab_chunk_debug_magic),
                                   need_size);
#ifdef CONFIG_DEBUG_BACKTRACE
                        for (m = 0; m < CONFIG_CHUNK_BACKTRACE_LEVEL; m++)
                        {
                            if (z->z_debug_info[j].backtrace[m] == RT_NULL)
                            {
                                break;
                            }
                            rt_kprintf("backtrace : %p\n", z->z_debug_info[j].backtrace[m]);
                        }
                        rt_kprintf("\n");
#endif
                        corrupted_chunk++;
                    }
                }
            }
            z = z->z_next;
        }
    }

    if (corrupted_chunk || corrupted_page)
    {
        rt_kprintf("total corrupted zone chunks = %d, total corrupted page chunk = %d\n", corrupted_chunk, corrupted_page);
        return -1;
    }
    return 0;
}

int cmd_memory_corrupt_info(int argc, char *argv)
{
    int ret;
    rt_ubase_t lock_value;

    lock_value = heap_lock();

    ret = check_memory_corrupt_info();

    heap_unlock(lock_value);
    return ret;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_memory_corrupt_info, __cmd_memory_corrupt_info, list zone information)
#endif

#endif

void heap_init(void)
{
    extern uint32_t _heap_start, _heap_end;

    uint32_t heap_start = (uint32_t)&_heap_start;
    uint32_t heap_end = (uint32_t) &_heap_end;

    if(heap_end > CONFIG_VIRTUAL_DRAM_ADDR + CONFIG_DRAM_SIZE)
    {
            heap_end = CONFIG_VIRTUAL_DRAM_ADDR + CONFIG_DRAM_SIZE;
    }

    rt_system_heap_init((void *)heap_start, (void *)heap_end);
}

static void (*memleak_malloc_hook)(void *ptr, uint32_t size);
static void (*memleak_free_hook)(void *ptr);

void memleak_malloc_sethook(void (*hook)(void *ptr, uint32_t size))
{
    memleak_malloc_hook = hook;
}

void memleak_free_sethook(void (*hook)(void *ptr))
{
    memleak_free_hook = hook;
}

void rt_free(void *ptr)
{
    __internal_free(ptr);

    if(memleak_free_hook)
    {
        memleak_free_hook(ptr);
    }
}

void *rt_malloc(rt_size_t size)
{
    void * ptr = __internal_malloc(size);
    if(ptr && memleak_malloc_hook)
    {
        memleak_malloc_hook(ptr, size);
    }
    return ptr;
}

#ifdef CONFIG_CPU_PRIVATE_DATA
static void *__rt_page_alloc_addr(unsigned long addr, rt_size_t npages)
{
	void *ret_addr = NULL;
	struct rt_page_head *b, *n;
	struct rt_page_head **prev;

	rt_ubase_t lock_value;

	if (npages == 0)
        return RT_NULL;

	unsigned long addr_start = RT_ALIGN((rt_ubase_t)addr, RT_MM_PAGE_SIZE);
	if( addr_start!=addr )
		npages++;
    /* lock heap */
	lock_value = heap_lock();
	for (prev = &rt_page_list; (b = *prev) != RT_NULL; prev = &(b->next))
	{
		//printk("b:%p, b->page:%ld, b->next:%08lx, \n", b, (unsigned long)b->page, b->next);
		unsigned long p_start = (unsigned long)b;
		unsigned long addr_end = RT_MM_PAGE_SIZE * npages + (unsigned long)addr_start;
		unsigned long p_end = RT_MM_PAGE_SIZE * b->page + (unsigned long)b;
		if( p_start > addr_start || addr_end > p_end )
			continue;

		if( addr_end < p_end ){
			/* divide into two blocks */
			n = (struct rt_page_head *)addr_end;
			n->page = (p_end - addr_end) / RT_MM_PAGE_SIZE;
			n->next = b->next;
			b->next = n;
			b->page -= n->page;
		}

		if( p_start == addr_start ){
			*prev = b->next;
		}
		else{
			b->page -= npages;
		}
		ret_addr = (void *)addr_start;
	}

    /* unlock heap */
	heap_unlock(lock_value);
	return ret_addr;
}

void *rt_page_alloc_addr(unsigned long addr, rt_size_t npages)
{
	struct memusage *kup;
	void *ptr;
	rt_ubase_t lock_value;

#ifdef CONFIG_SLAB_DEBUG
	rt_ubase_t interrupt_level;
	npages += 2;
	if(addr < RT_MM_PAGE_SIZE)
		return NULL;
	addr = addr - RT_MM_PAGE_SIZE;
#endif

	ptr = __rt_page_alloc_addr(addr, npages);
	if (ptr == RT_NULL)
	{
		return RT_NULL;
	}

	lock_value = heap_lock();

#ifdef CONFIG_SLAB_DEBUG
	/* need to disable interrrupt, because rt_schedule will be called in irq handler */
	interrupt_level = rt_hw_interrupt_disable();
#ifdef CONFIG_KASAN
	kasan_disable_report();
#endif
#endif

	kup = btokup(ptr);
	kup->type = PAGE_TYPE_SHORT;
	kup->size = npages;

#ifdef CONFIG_SLAB_DEBUG
	kup->ptr = ptr;
	kup->need_size = (npages - 2) * RT_MM_PAGE_SIZE;
	kup->next = allocated_list;
	kup->alloc_thread = xTaskGetCurrentTaskHandle();
	allocated_list = kup;

	/* copy debug magic to the first 8 bytes and the last 8 bytes.  */
	rt_memcpy((rt_uint8_t *)ptr + RT_MM_PAGE_SIZE - sizeof(slab_chunk_debug_magic), &chunk_debug_magic, sizeof(slab_chunk_debug_magic));
	rt_memcpy((rt_uint8_t *)ptr + RT_MM_PAGE_SIZE + kup->need_size,
				&chunk_debug_magic,
				sizeof(slab_chunk_debug_magic));
	ptr = (void *)((rt_uint8_t *)ptr + RT_MM_PAGE_SIZE);
#ifdef CONFIG_DEBUG_BACKTRACE
	rt_memset(kup->backtrace, 0, sizeof(kup->backtrace));
	/* get backtrace information */
	backtrace(RT_NULL, kup->backtrace, CONFIG_CHUNK_BACKTRACE_LEVEL, 0, RT_NULL);
#endif
#ifdef CONFIG_KASAN
	kasan_enable_report();
#endif
	npages -= 2;
	RT_OBJECT_HOOK_CALL(rt_page_alloc_hook, ((char *)ptr, npages));
	rt_hw_interrupt_enable(interrupt_level);
#endif

	heap_unlock(lock_value);

	return ptr;
}
#endif

#endif
