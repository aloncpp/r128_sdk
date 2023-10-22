#include <stddef.h>
#include "slab_rtt_system.h"

void rt_memory_info(uint32_t *total, uint32_t *used, uint32_t *max_used);

void *pvPortMalloc( size_t xWantedSize )
{
    return rt_malloc(xWantedSize);
}

void vPortFree( void *pv )
{
    rt_free(pv);
}

void *pvPortCalloc( size_t count, size_t size )
{
    return rt_calloc(count, size);
}

void *pvPortRealloc( void * ptr, size_t size )
{
    return rt_realloc(ptr, size);
}

void *pvPortMallocAlign( size_t size, size_t align )
{
    return rt_malloc_align(size, align);
}

void vPortFreeAlign( void *pv )
{
    rt_free_align(pv);
}

size_t xPortGetFreeHeapSize( void )
{
    uint32_t total = 0;
    uint32_t used = 0;
    uint32_t max_used = 0;

    rt_memory_info(&total, &used, &max_used);
    return total - used;
}

size_t xPortGetMinimumEverFreeHeapSize( void )
{
    uint32_t total = 0;
    uint32_t used = 0;
    uint32_t max_used = 0;

    rt_memory_info(&total, &used, &max_used);
    return total - max_used;
}

size_t xPortGetTotalHeapSize( void )
{
    uint32_t total = 0;
    uint32_t used = 0;
    uint32_t max_used = 0;

    rt_memory_info(&total, &used, &max_used);
    return total;
}
