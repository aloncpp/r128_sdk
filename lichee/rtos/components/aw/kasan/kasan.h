#ifndef __MM_KASAN_KASAN_H
#define __MM_KASAN_KASAN_H

#include <stdbool.h>
#include "kasan_rtos.h"

#define KASAN_ABI_VERSION 6

#define KASAN_SHADOW_SCALE_SIZE (1UL << KASAN_SHADOW_SCALE_SHIFT)
#define KASAN_SHADOW_MASK       (KASAN_SHADOW_SCALE_SIZE - 1)

#define KASAN_FREE_PAGE         0xFF  /* page was freed */
#define KASAN_PAGE_REDZONE      0xFE  /* redzone for kmalloc_large allocations */
#define KASAN_KMALLOC_REDZONE   0xFC  /* redzone inside slub object */
#define KASAN_KMALLOC_FREE      0xFB  /* object was freed (kmem_cache_free/kfree) */
#define KASAN_GLOBAL_REDZONE    0xFA  /* redzone for global variable */

/*
 * Stack redzone shadow values
 * (Those are compiler's ABI, don't change them)
 */
#define KASAN_STACK_LEFT        0xF1
#define KASAN_STACK_MID         0xF2
#define KASAN_STACK_RIGHT       0xF3
#define KASAN_STACK_PARTIAL     0xF4
#define KASAN_USE_AFTER_SCOPE   0xF8

/*
 * alloca redzone shadow values
 */
#define KASAN_ALLOCA_LEFT   0xCA
#define KASAN_ALLOCA_RIGHT  0xCB

#define KASAN_ALLOCA_REDZONE_SIZE   32

/* Don't break randconfig/all*config builds */
#ifndef KASAN_ABI_VERSION
#define KASAN_ABI_VERSION 1
#endif

struct kasan_access_info
{
    const void *access_addr;
    const void *first_bad_addr;
    size_t access_size;
    bool is_write;
    unsigned long ip;
};

/* The layout of struct dictated by compiler */
struct kasan_source_location
{
    const char *filename;
    int line_no;
    int column_no;
};

/* The layout of struct dictated by compiler */
struct kasan_global
{
    const void *beg;        /* Address of the beginning of the global variable. */
    size_t size;            /* Size of the global variable. */
    size_t size_with_redzone;   /* Size of the variable + size of the red zone. 32 bytes aligned */
    const void *name;
    const void *module_name;    /* Name of the module where the global variable is declared. */
    unsigned long has_dynamic_init; /* This needed for C++ */
#if KASAN_ABI_VERSION >= 4
    struct kasan_source_location *location;
#endif
#if KASAN_ABI_VERSION >= 5
    char *odr_indicator;
#endif
};

static inline const void *kasan_shadow_to_mem(const void *shadow_addr)
{
    return (void *)(((unsigned long)shadow_addr - KASAN_SHADOW_OFFSET)
                    << KASAN_SHADOW_SCALE_SHIFT);
}

void kasan_report(unsigned long addr, size_t size,
                  bool is_write, unsigned long ip);
void kasan_report_double_free(unsigned int size, void *object,
                              void *ip);
void kasan_alloc_pages(void *page, unsigned int npages);
void kasan_free_pages(void *page, unsigned int npages);
void kasan_malloc_large(void *page, unsigned int size);
void kasan_free_large(void *page, unsigned int size);
void kasan_malloc_small(void *page, unsigned int size);
void kasan_free_small(void *page, unsigned int size);
void kasan_realloc_small(void *page, unsigned int size);
void kasan_double_free_check(void *page);

/*
 * Exported functions for interfaces called from assembly or from generated
 * code. Declarations here to avoid warning about missing declarations.
 */
void __asan_register_globals(struct kasan_global *globals, size_t size);
void __asan_unregister_globals(struct kasan_global *globals, size_t size);
void __asan_loadN(unsigned long addr, size_t size);
void __asan_storeN(unsigned long addr, size_t size);
void __asan_handle_no_return(void);
void __asan_poison_stack_memory(const void *addr, size_t size);
void __asan_unpoison_stack_memory(const void *addr, size_t size);
void __asan_alloca_poison(unsigned long addr, size_t size);
void __asan_allocas_unpoison(const void *stack_top, const void *stack_bottom);

void __asan_load1(unsigned long addr);
void __asan_store1(unsigned long addr);
void __asan_load2(unsigned long addr);
void __asan_store2(unsigned long addr);
void __asan_load4(unsigned long addr);
void __asan_store4(unsigned long addr);
void __asan_load8(unsigned long addr);
void __asan_store8(unsigned long addr);
void __asan_load16(unsigned long addr);
void __asan_store16(unsigned long addr);

void __asan_load1_noabort(unsigned long addr);
void __asan_store1_noabort(unsigned long addr);
void __asan_load2_noabort(unsigned long addr);
void __asan_store2_noabort(unsigned long addr);
void __asan_load4_noabort(unsigned long addr);
void __asan_store4_noabort(unsigned long addr);
void __asan_load8_noabort(unsigned long addr);
void __asan_store8_noabort(unsigned long addr);
void __asan_load16_noabort(unsigned long addr);
void __asan_store16_noabort(unsigned long addr);

void __asan_set_shadow_00(const void *addr, size_t size);
void __asan_set_shadow_f1(const void *addr, size_t size);
void __asan_set_shadow_f2(const void *addr, size_t size);
void __asan_set_shadow_f3(const void *addr, size_t size);
void __asan_set_shadow_f5(const void *addr, size_t size);
void __asan_set_shadow_f8(const void *addr, size_t size);

#endif
