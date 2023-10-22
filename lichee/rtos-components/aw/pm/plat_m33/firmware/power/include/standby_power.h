#ifndef __STANDBY_POWER_H__
#define __STANDBY_POWER_H__

#include "head.h"
#include "platform.h"

#define CPU_BOOT_FLAG_REG			(GPRCM_BASE + 0x01c0)
#define BOOT_FALG_COLD_RESET			(0x429b0000)
#define BOOT_FALG_DEEP_SLEEP			(0x429b0001)

/* pmu */
#define SYS_LOW_POWER_CTRL_REG			(PMU_BASE + 0x0100)
#define DSP_WUP_EN_MASK				(0x1 << 12)
#define RV_WUP_EN_MASK				(0x1 << 8)
#define STANDBY_HIBERNATION_MODE_SEL_MASK	(0x1 << 0)

#define SYS_LOW_POWER_STATUS_REG		(PMU_BASE + 0x0104)
#define DSP_SLEEP				(0x1 << 13)
#define DSP_ALIVE				(0x1 << 12)
#define RV_SLEEP				(0x1 << 9)
#define RV_ALIVE				(0x1 << 8)

/* gprcm */
#define EXT_LDO_CTRL_REG			(GPRCM_BASE + 0x0024)
#define EXT_LDO_EN_MASK				(0x3 << 0)
#define EXT_LDO_EN_BY_PMU			(0x1 << 0)

#define SYS_LFCLK_CTRL				(GPRCM_BASE + 0x0080)
#define SYS_32K_SEL_MASK			(0x1 << 28)

#define BLE_RCOSC_CALIB_REG0			(GPRCM_BASE + 0x0144)
#define RCO_WUP_TIME_EN_MASK			(0x1 << 16)
#define RCO_WUP_TIME_EN				(0x1 << 16)

#define BLE_CLK32K_SW_REG0			(GPRCM_BASE + 0x014c)
#define CLK32K_AUTO_SW_EN_MASK			(0x1 << 0)

/* analog and adc path */
#define AC_POWER_CTRL_REG			(ANALOG_AND_ADC_BASE + 0x0000)
#define ALDO_EN_MASK				(0x1 << 18)

void hibernation_mode_set(void);
void hibernation_mode_restore(void);
void ldo_disable(standby_head_t *head);
void ldo_enable(standby_head_t *head);

#endif /* __STANDBY_POWER_H__ */
