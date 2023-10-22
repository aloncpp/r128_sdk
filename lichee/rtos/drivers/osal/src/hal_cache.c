#include <hal_osal.h>
#include <mmu_cache.h>

#ifdef CONFIG_CACHE_ALIGN_CHECK
#include <sunxi_hal_common.h>

#ifdef CONFIG_DEBUG_BACKTRACE
/*#if 0*/
#include <backtrace.h>
#include <assert.h>
#define CACHELINE_CHECK(option) \
{ \
	if (!option) { \
		printf("[%s] cacheline check failed\n", __func__); \
		backtrace(NULL, NULL, 0, 0, printf); \
		assert(0); \
	} \
} while (0)

#else
#define CACHELINE_CHECK(option) \
{ \
	assert(option); \
} while (0)

#endif /* CONFIG_DEBUG_BACKTRACE */
#endif /* CONFIG_CACHE_ALIGN_CHECK */


void hal_dcache_clean(unsigned long vaddr_start, unsigned long size)
{
#ifdef CONFIG_ARCH_HAVE_DCACHE
#ifdef CONFIG_CACHE_ALIGN_CHECK
	CACHELINE_CHECK(!(vaddr_start & (CACHELINE_LEN - 1)));
	/*CACHELINE_CHECK((size % CACHELINE_LEN) == 0);*/
#endif
	FlushDcacheRegion(vaddr_start, size);
#endif
}

void hal_dcache_invalidate(unsigned long vaddr_start, unsigned long size)
{
#ifdef CONFIG_ARCH_HAVE_DCACHE
#ifdef CONFIG_CACHE_ALIGN_CHECK
	CACHELINE_CHECK(!(vaddr_start & (CACHELINE_LEN - 1)));
	/*CACHELINE_CHECK((size % CACHELINE_LEN) == 0);*/
#endif
	InvalidDcacheRegion(vaddr_start, size);
#endif
}

void hal_dcache_clean_invalidate(unsigned long vaddr_start, unsigned long size)
{
#ifdef CONFIG_ARCH_HAVE_DCACHE
	FlushDcacheRegion(vaddr_start, (unsigned int)size);
	InvalidDcacheRegion(vaddr_start, (unsigned int)size);
#endif
}

void hal_icache_invalidate_all(void)
{
#ifdef CONFIG_ARCH_HAVE_ICACHE
	FlushIcacheAll();
#endif
}

void hal_dcache_invalidate_all(void)
{
#ifdef CONFIG_ARCH_HAVE_DCACHE
    InvalidDcache();
#endif
}

void hal_dcache_clean_all(void)
{
#ifdef CONFIG_ARCH_HAVE_DCACHE
	FlushDcacheAll();
#endif
}

void hal_icache_invalidate(unsigned long vaddr_start, unsigned long size)
{
#ifdef CONFIG_ARCH_HAVE_ICACHE
    InvalidIcacheRegion(vaddr_start, size);
#endif
}
