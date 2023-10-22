#include "armv7a.h"
#include "mmu_cache.h"
#include <aw_types.h>
#include <io.h>
#include <hal_dma.h>

void FlushTLB(void)
{
	ARMV7_FLUSH_TLB_ALL();
}

void FlushDcacheAll(void)
{
	ARMV7_FLUSH_DCACHE_ALL();
}

void FlushIcacheAll(void)
{
	ARMV7_FLUSH_ICACHE_ALL();
}

void FlushCacheAll(void)
{
	ARMV7_FLUSH_CACHE_ALL();
}


void InvalidIcacheRegion(unsigned long start, unsigned int size)
{
	ARMV7_FLUSH_ICACHE_REGION(start, (void *)((unsigned int)start + size));
}

void FlushDcacheRegion(unsigned long start, unsigned int size)
{
	ARMV7_FLUSH_DCACHE_REGION(start, (void *)((unsigned int)start + size));
}


void InvalidDcacheRegion(unsigned long start, unsigned int size)
{
	ARMV7_INVALID_DCACHE_REGION(start, (void *)((unsigned int)start + size));
}

void InvalidDcache(void)
{
    ARMV7_INVALIDATE_DCACHE();
}

void * dma_map_area(void * addr, size_t size, enum dma_transfer_direction dir)
{
    void * v7_dma_map_area(void *addr, size_t size, enum dma_transfer_direction dir);
    v7_dma_map_area(addr, size, dir);
    return addr;
}

void dma_unmap_area(void * addr, size_t size, enum dma_transfer_direction dir)
{
    void v7_dma_unmap_area(void *addr, size_t size, enum dma_transfer_direction dir);
    v7_dma_unmap_area(addr, size, dir);
}
