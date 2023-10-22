#ifndef _SUNXI_HAL_LSPSRAM_H_
#define _SUNXI_HAL_LSPSRAM_H_

#include "head.h" /* for standby.bin */

int platform_psram_chip_config(void);
void psram_suspend(struct psram_chip *lpsram_para);
int psram_resume(struct psram_chip *lpsram_para);

#endif
