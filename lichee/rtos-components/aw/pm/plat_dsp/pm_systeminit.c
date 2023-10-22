#ifdef CONFIG_DRIVERS_DMA
#include <hal_dma.h>
#endif
#include "pm_systeminit.h"

static void pm_SetupHardware(void)
{

}

void pm_systeminit(void)
{
	pm_SetupHardware();
}
