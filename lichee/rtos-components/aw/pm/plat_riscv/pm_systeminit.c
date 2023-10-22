#include <osal/hal_interrupt.h>
#ifdef CONFIG_DRIVERS_DMA
#include <hal_dma.h>
#endif
#include <io.h>
#include "pm_systeminit.h"

static void pm_SetupHardware(void)
{

}

void pm_systeminit(void)
{
	/* regs retore, do not need to set?
	extern void cache_init(void);
	cache_init();
	*/

#ifdef CONFIG_ARCH_HAVE_ICACHE
	extern void hal_icache_init(void);
	hal_icache_init();
#endif

#ifdef CONFIG_ARCH_HAVE_DCACHE
	extern void hal_dcache_init(void);
	hal_dcache_init();
#endif

	pm_SetupHardware();
}
