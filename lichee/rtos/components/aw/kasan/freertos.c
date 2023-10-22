#include "kasan.h"

#include <port_misc.h>
#include <mmu_cache.h>

char *get_current_thread_name(void)
{
    TaskHandle_t task = NULL;

    if (uGetInterruptNest() == 0)
    {
        task = xTaskGetCurrentTaskHandle();
        if (task)
        {
            return pcTaskGetName(task);
        }
    }
    else
    {
        return "irq-context!";
    }
	return "null";
}

int get_current_thread_id(void)
{
    return 0;
}

void rt_page_alloc_func_hook(void *ptr, unsigned int npages)
{
    kasan_alloc_pages(ptr, npages);
}

void rt_page_free_func_hook(void *ptr, unsigned int npages)
{
    kasan_free_pages(ptr, npages);
}

void rt_malloc_large_func_hook(void *ptr, unsigned int size)
{
    kasan_malloc_large(ptr, size);
}

void rt_free_large_func_hook(void *ptr, unsigned int size)
{
    kasan_free_large(ptr, size);
}

void rt_malloc_small_func_hook(void *ptr, unsigned int size)
{
    kasan_malloc_small(ptr, size);
}

void rt_free_small_func_hook(void *ptr, unsigned int size)
{
    kasan_free_small(ptr, size);
}

void rt_realloc_small_func_hook(void *ptr, unsigned int size)
{
    kasan_realloc_small(ptr, size);
}

void rt_free_func_hook(void *ptr)
{
    kasan_double_free_check(ptr);
}
