#include <hal_osal.h>

#include <portable.h>

void *hal_malloc(uint32_t size)
{
	return malloc(size);
}

void hal_free(void *p)
{
	return free(p);
}

void *hal_malloc_align(uint32_t size, int align)
{
    return pvPortMallocAlign(size, align);
}

void hal_free_align(void *p)
{
    return vPortFreeAlign(p);
}

unsigned long hal_virt_to_phys(unsigned long virtaddr)
{
    return virtaddr;
}

unsigned long hal_phys_to_virt(unsigned long phyaddr)
{
    return phyaddr;
}
