/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Configuration settings for the Allwinner A64 (sun50i) CPU
 */

#ifndef __CONFIG_H
#define __CONFIG_H

/* sram layout*/

#define SUNXI_SRAM_A1_BASE		(0x4000000L)
#define SUNXI_SRAM_A1_SIZE		(1 * 1024 *1024)
/*#define SUNXI_SRAM_A1_SIZE		(768 *1024)*/

#define SUNXI_SYS_SRAM_BASE		SUNXI_SRAM_A1_BASE
#define SUNXI_SYS_SRAM_SIZE		(SUNXI_SRAM_A1_SIZE)

#define CONFIG_SYS_BOOTM_LEN 0x2000000
#define PHOENIX_PRIV_DATA_ADDR      (SUNXI_SYS_SRAM_BASE + 0x9400)

#ifdef CONFIG_SUNXI_USE_SRAM
#define SUNXI_SYS_MALLOC_LEN    (256 * 1024)
#else
#define SUNXI_SYS_MALLOC_LEN	(2560 * 1024)
#endif

/*#define CONFIG_SYS_HZ_CLOCK		24000000*/	/* Timer is clocked at 24MHz */
/*
 * Include common sunxi configuration where most the settings are
 */
#include <configs/sunxi-common.h>

#endif /* __CONFIG_H */
