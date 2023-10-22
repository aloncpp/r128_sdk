#include <stdint.h>
#include <xtensa/hal.h>

void hal_dcache_clean(unsigned long vaddr_start, unsigned long size)
{
	xthal_dcache_region_writeback((void *)vaddr_start, (uint32_t)size);
}

void hal_dcache_invalidate(unsigned long vaddr_start, unsigned long size)
{
	xthal_dcache_region_invalidate((void *)vaddr_start, (uint32_t)size);
}

void hal_dcache_clean_invalidate(unsigned long vaddr_start, unsigned long size)
{
	xthal_dcache_region_writeback_inv((void *)vaddr_start, (uint32_t)size);
}

void hal_icache_invalidate_all(void)
{
	xthal_icache_all_invalidate();
}

void hal_icache_invalidate(unsigned long vaddr_start, unsigned long size)
{
	xthal_icache_region_invalidate((void *)vaddr_start, (uint32_t)size);
}

void hal_dcache_invalidate_all(void)
{
	xthal_dcache_all_invalidate();
}

void hal_dcache_clean_all(void)
{
	xthal_dcache_all_writeback();
}
