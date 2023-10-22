#include <xtensa/hal.h>

#include <awlog.h>

void __attribute__((weak)) NMI_Handler(void)
{
	printfFromISR("Sample NMI Handler\n");
	xthal_set_intclear(1<<XCHAL_NMILEVEL);
}
