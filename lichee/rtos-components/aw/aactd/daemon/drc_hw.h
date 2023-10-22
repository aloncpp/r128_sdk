#ifndef __AACTD_DRC_HW_H__
#define __AACTD_DRC_HW_H__

#include "aactd/communicate.h"

#define MAX_DRC_3_BAND_REG_NUM (78)

#define MAX_DRC_1_BAND_REG_NUM (26)


#define DRC1B_REG_MIN 	0x600
#define DRC1B_REG_MAX	0x660
#define DRC3B_REG_MIN 	0x700
#define DRC3B_REG_MAX	0x834

int drc_hw_local_init(void);
int drc_hw_local_release(void);

int drc_hw_write_com_to_local(const struct aactd_com *com);

#if defined(CONFIG_ARCH_SUN8IW18) || defined(CONFIG_ARCH_SUN8IW19) || defined(CONFIG_ARCH_SUN8IW20)

#define drchw_writel(value,reg)   (*(volatile uint32_t *)(reg) = (value))

#if defined(CONFIG_ARCH_SUN8IW18) || defined(CONFIG_ARCH_SUN8IW19)
#define DRC_HW_REG_BASE_ADDR 0x05096000

#define DAC_DRC_REG_MIN 0xF0
#define DAC_DRC_REG_MAX	0x1B0

#define ADC_DRC_REG_MIN 0xF8
#define ADC_DRC_REG_MAX	0x2B0

#endif

#ifdef CONFIG_ARCH_SUN8IW20
#define DRC_HW_REG_BASE_ADDR 0x02030000

#define DAC_DRC_REG_MIN 0xF0
#define DAC_DRC_REG_MAX	0x1B0

#define ADC_DRC_REG_MIN 0xF8
#define ADC_DRC_REG_MAX	0x2B0

#endif

#ifndef DRC_HW_REG_BASE_ADDR
#error "not define DRC_HW_REG_BASE_ADDR"
#endif

#endif

#endif /* ifndef __AACTD_DRC_HW_H__ */
