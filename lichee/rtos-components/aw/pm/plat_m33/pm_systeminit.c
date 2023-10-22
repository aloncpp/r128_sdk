#ifdef CONFIG_DRIVERS_DMA
#include <hal_dma.h>
#endif
#include <io.h>
#include <sunxi_hal_rcosc_cali.h>

static void pm_SetupHardware(void)
{
	/* rcosc_init PRCM
	RCOCALI_InitParam cal_para;
	cal_para.cnt_n = 8192;
	cal_para.out_clk = 32000;
	HAL_RcoscCali_Init(&cal_para);
	HAL_RcoscCali_Start();
	*/
}

void pm_systeminit(void)
{
#ifdef CONFIG_ARM_MPU
	extern void init_mpu(void);
	init_mpu();
#endif

#if 0
/* restore in standby.bin */
#ifdef CONFIG_ARCH_HAVE_ICACHE
	extern void hal_icache_init(void)
	hal_icache_init();
#endif

#ifdef CONFIG_ARCH_HAVE_DCACHE
	extern void hal_dcache_init(void)
	hal_dcache_init();
#endif
#endif

	pm_SetupHardware();
}
