#include <xtensa/hal.h>
#include <xtensa/tie/xt_externalregisters.h>
#include <xtensa/config/core.h>
#include <xtensa/config/core-matmap.h>
#include <xtensa/xtruntime.h>

#include <platform.h>
#include <irqs.h>
#include <aw_io.h>
#include <console.h>
#include <string.h>
#include <stdio.h>
#include <components/aw/linux_debug/debug_common.h>
#include "spinlock.h"
#include <sunxi_hal_common.h>
#ifdef CONFIG_DRIVERS_SOUND
#include <snd_core.h>
#endif
#ifdef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM
#include <AudioSystem.h>
#endif
#ifdef CONFIG_DRIVERS_CCMU
#include <hal_clk.h>
#endif
#ifdef CONFIG_COMPONENTS_PM
#include <pm_init.h>
#endif

#ifdef CONFIG_COMPONENTS_XTENSA_HIFI5_NNLIB_LIBRARY
#include <xa_nnlib_standards.h>
#endif

#ifdef CONFIG_COMPONENTS_XTENSA_HIFI5_VFPU_LIBRARY
#include <NatureDSP_Signal_id.h>
#endif

#ifdef CONFIG_COMPONENTS_OMX_SYSTEM
#include <rpdata_common_interface.h>
#endif

extern int amp_init(void);

int32_t console_uart = UART_UNVALID;

void cache_config(void)
{
	/* 0x0~0x20000000-1 is cacheable */
	xthal_set_region_attribute((void *)0x00000000, 0x20000000, XCHAL_CA_WRITEBACK, 0);

	/* 0x20000000~0x40000000-1 is non-cacheable */
	xthal_set_region_attribute((void *)0x20000000, 0x40000000, XCHAL_CA_BYPASS, 0);

	/* 0x4000000~0x80000000-1 is non-cacheable */
	xthal_set_region_attribute((void *)0x40000000, 0x40000000, XCHAL_CA_BYPASS, 0);

	/* 0x80000000~0xC0000000-1 is non-cacheable */
	xthal_set_region_attribute((void *)0x80000000, 0x40000000, XCHAL_CA_BYPASS, 0);

	/* 0xC0000000~0xFFFFFFFF is  non-cacheable */
	xthal_set_region_attribute((void *)0xC0000000, 0x40000000, XCHAL_CA_BYPASS, 0);

	/* set prefetch level */
	xthal_set_cache_prefetch(XTHAL_PREFETCH_BLOCKS(8) |XTHAL_DCACHE_PREFETCH_HIGH | XTHAL_ICACHE_PREFETCH_HIGH |XTHAL_DCACHE_PREFETCH_L1);
}

#define DEFAULT_DSP_CORE_CLK_FREQ 400000000
static uint32_t s_dsp_core_clk_freq = 0;

static int get_dsp_core_clk_freq(uint32_t *dsp_clk_freq)
{
#ifdef CONFIG_DRIVERS_CCMU
	hal_clk_t dsp_div_clk;
	dsp_div_clk = hal_clock_get(HAL_SUNXI_CCU, CLK_DSP_DIV);
	if (!dsp_div_clk)
		return -1;

	*dsp_clk_freq = hal_clk_recalc_rate(dsp_div_clk);
	return 0;
#else
	printf("Warning: no CCMU driver support, use default frequency!\n");
	*dsp_clk_freq = DEFAULT_DSP_CORE_CLK_FREQ;
	return 0;
#endif
}

unsigned int xtbsp_clock_freq_hz(void)
{
	if (s_dsp_core_clk_freq)
		return s_dsp_core_clk_freq;

	uint32_t dsp_clk_freq = 0;
	int ret = get_dsp_core_clk_freq(&dsp_clk_freq);
	if (ret)
	{
		printf("get_dsp_core_clk_freq failed, ret: %d\n", ret);
		dsp_clk_freq = DEFAULT_DSP_CORE_CLK_FREQ;
	}

	if (!dsp_clk_freq)
	{
		printf("Warning: invalid DSP core clock frequency, use default frequency!\n");
		dsp_clk_freq = DEFAULT_DSP_CORE_CLK_FREQ;
	}

	s_dsp_core_clk_freq = dsp_clk_freq;

	return s_dsp_core_clk_freq;
}

void print_banner(void)
{
	/* remove, print banner after start schedule */
}

void print_banner_after_schedule(void)
{
	printf("\r\n"
	       " *******************************************\r\n"
	       " **     Welcome to R128 DSP FreeRTOS      **\r\n"
	       " ** Copyright (C) 2019-2022 AllwinnerTech **\r\n"
	       " **                                       **\r\n"
	       " **      starting xtensa FreeRTOS V0.8    **\r\n"
	       " **    Date:%s, Time:%s    **\r\n"
	       " *******************************************\r\n"
	       "\r\n", __DATE__, __TIME__);
}

void app_init(void)
{
	printf("DSP core clock frequency: %uHz\n", s_dsp_core_clk_freq);
	print_banner_after_schedule();

#ifdef CONFIG_COMPONENTS_XTENSA_HIFI5_NNLIB_LIBRARY
	const char *name = xa_nnlib_get_lib_name_string();
	const char *aver = xa_nnlib_get_lib_api_version_string();
	printf("Name:%s API version:%s \n",name,aver);
#endif

#ifdef CONFIG_COMPONENTS_XTENSA_HIFI5_VFPU_LIBRARY
	char nature_lib_ver[32] = {0};
	char natrre_api_ver[32] = {0};
	NatureDSP_Signal_get_library_version(nature_lib_ver);
	NatureDSP_Signal_get_library_api_version(natrre_api_ver);
	printf("VFPU lib version:%s  API version:%s\n",nature_lib_ver,natrre_api_ver);
#endif

#ifdef CONFIG_COMPONENTS_AMP
	amp_init();
#endif
#ifdef CONFIG_COMPONENTS_PM
	pm_init(1, NULL);
#endif
#ifdef CONFIG_DRIVERS_SOUND
	sunxi_soundcard_init();
#endif
#ifdef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM
	AudioSystemInit();
#endif
#ifdef CONFIG_COMPONENTS_OMX_SYSTEM
	rpdata_ctrl_init();
#endif

}

