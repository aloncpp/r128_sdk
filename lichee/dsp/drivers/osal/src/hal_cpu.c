#include <hal_cpu.h>

unsigned long hal_cpu_get_id(void)
{
#ifdef CONFIG_ARCH_SUN20IW2
	/* CPU ID
	 * ARM -- 0
	 * RV  -- 1
	 * DSP -- 2
	 */
	return 2L;
#endif
	return 0;
}
