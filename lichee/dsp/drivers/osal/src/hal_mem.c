#include <stdint.h>
#include <stdlib.h>

#include <FreeRTOS.h>

void *hal_malloc(uint32_t size)
{
	/* return malloc(size); */
	return pvPortMalloc(size);
}

void hal_free(void *p)
{
	/* free(p); */
	vPortFree(p);
}

unsigned long hal_virt_to_phys(unsigned long virtaddr)
{
	return virtaddr;
}

unsigned long hal_phys_to_virt(unsigned long phyaddr)
{
	return phyaddr;
}
