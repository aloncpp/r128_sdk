/*
 * Copyright (C) 2019 Allwinner.
 * weidonghui <weidonghui@allwinnertech.com>
 *
 * SUNXI AXP21  Driver
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <common.h>
#include <sunxi_power/pmu_axp1530.h>
#include <sunxi_power/axp.h>
#include <asm/arch/pmic_bus.h>

/*#include <power/sunxi/pmu.h>*/

#ifdef PMU_DEBUG
#define axp_info(fmt...) tick_printf("[axp][info]: " fmt)
#define axp_err(fmt...) tick_printf("[axp][err]: " fmt)
#else
#define axp_info(fmt...)
#define axp_err(fmt...) tick_printf("[axp][err]: " fmt)
#endif

typedef struct _axp_contrl_info {
	char name[16];

	u32 min_vol;
	u32 max_vol;
	u32 cfg_reg_addr;
	u32 cfg_reg_mask;

	u32 step0_val;
	u32 split1_val;
	u32 step1_val;
	u32 ctrl_reg_addr;

	u32 ctrl_bit_ofs;
	u32 step2_val;
	u32 split2_val;
} axp_contrl_info;

__attribute__((section(".data"))) axp_contrl_info pmu_axp1530_ctrl_tbl[] = {
	/*name,    min,  max, reg,  mask, step0,split1_val, step1,ctrl_reg,ctrl_bit */
	{ "dcdc1", 500, 3400, AXP1530_DC1OUT_VOL, 0x7f, 10, 1200, 20,
	  AXP1530_OUTPUT_POWER_ON_OFF_CTL, 0, 100, 1540 },
	{ "dcdc2", 500, 1540, AXP1530_DC2OUT_VOL, 0x7f, 10, 1200, 20,
	  AXP1530_OUTPUT_POWER_ON_OFF_CTL, 1 },
	{ "dcdc3", 500, 1840, AXP1530_DC3OUT_VOL, 0x7f, 10, 1200, 20,
	  AXP1530_OUTPUT_POWER_ON_OFF_CTL, 2 },
	{ "aldo1", 500, 3500, AXP1530_ALDO1OUT_VOL, 0x1f, 100, 0, 0,
	  AXP1530_OUTPUT_POWER_ON_OFF_CTL, 3 },
	{ "dldo1", 500, 3500, AXP1530_DLDO1OUT_VOL, 0x1f, 100, 0, 0,
	  AXP1530_OUTPUT_POWER_ON_OFF_CTL, 4 },

};

static axp_contrl_info *get_ctrl_info_from_tbl(char *name)
{
	int i    = 0;
	int size = ARRAY_SIZE(pmu_axp1530_ctrl_tbl);
	axp_contrl_info *p;

	for (i = 0; i < size; i++) {
		if (!strncmp(name, pmu_axp1530_ctrl_tbl[i].name,
			     strlen(pmu_axp1530_ctrl_tbl[i].name))) {
			break;
		}
	}
	if (i >= size) {
		axp_err("can't find %s from table\n", name);
		return NULL;
	}
	p = pmu_axp1530_ctrl_tbl + i;
	return p;
}

static int pmu_axp1530_necessary_reg_enable(void)
{
	__attribute__((unused)) u8 reg_value;
#ifdef CONFIG_AXP1530A_NECESSARY_REG_ENABLE
	if (pmic_bus_read(AXP1530_RUNTIME_ADDR, AXP1530_WRITE_LOCK, &reg_value))
		return -1;
	reg_value |= 0x5;
	if (pmic_bus_write(AXP1530_RUNTIME_ADDR, AXP1530_WRITE_LOCK, reg_value))
		return -1;

	if (pmic_bus_read(AXP1530_RUNTIME_ADDR, AXP1530_ERROR_MANAGEMENT, &reg_value))
		return -1;
	reg_value |= 0x8;
	if (pmic_bus_write(AXP1530_RUNTIME_ADDR, AXP1530_ERROR_MANAGEMENT, reg_value))
		return -1;

	if (pmic_bus_read(AXP1530_RUNTIME_ADDR, AXP1530_DCDC_DVM_PWM_CTL, &reg_value))
		return -1;
	reg_value |= (0x1 << 5);
	if (pmic_bus_write(AXP1530_RUNTIME_ADDR, AXP1530_DCDC_DVM_PWM_CTL, reg_value))
		return -1;
#endif
	return 0;
}

static int pmu_axp1530_probe(void)
{
	u8 pmu_chip_id;
	if (pmic_bus_init(AXP1530_DEVICE_ADDR, AXP1530_RUNTIME_ADDR)) {
		tick_printf("%s pmic_bus_init fail\n", __func__);
		return -1;
	}
	if (pmic_bus_read(AXP1530_RUNTIME_ADDR, AXP1530_VERSION, &pmu_chip_id)) {
		tick_printf("%s pmic_bus_read fail\n", __func__);
		return -1;
	}
	pmu_chip_id &= 0XCF;
	if (pmu_chip_id == AXP1530_CHIP_ID || pmu_chip_id == AXP313A_CHIP_ID || pmu_chip_id == AXP313B_CHIP_ID) {
		/*pmu type AXP1530*/
		pmu_axp1530_necessary_reg_enable();
		tick_printf("PMU: AXP1530\n");
		return 0;
	}
	return -1;
}

static int pmu_axp1530_get_info(char *name, unsigned char *chipid)
{
	strncpy(name, "axp1530", sizeof("axp1530"));
	*chipid = AXP1530_CHIP_ID;
	return 0;
}


static int pmu_axp1530_set_voltage(char *name, uint set_vol, uint onoff)
{
	u8 reg_value;
	axp_contrl_info *p_item = NULL;
	u8 base_step		= 0;

	p_item = get_ctrl_info_from_tbl(name);
	if (!p_item) {
		return -1;
	}

	axp_info(
		"name %s, min_vol %dmv, max_vol %d, cfg_reg 0x%x, cfg_mask 0x%x \
		step0_val %d, split1_val %d, step1_val %d, ctrl_reg_addr 0x%x, ctrl_bit_ofs %d\n",
		p_item->name, p_item->min_vol, p_item->max_vol,
		p_item->cfg_reg_addr, p_item->cfg_reg_mask, p_item->step0_val,
		p_item->split1_val, p_item->step1_val, p_item->ctrl_reg_addr,
		p_item->ctrl_bit_ofs);

	if ((set_vol > 0) && (p_item->min_vol)) {
		if (set_vol < p_item->min_vol) {
			set_vol = p_item->min_vol;
		} else if (set_vol > p_item->max_vol) {
			set_vol = p_item->max_vol;
		}
		if (pmic_bus_read(AXP1530_RUNTIME_ADDR, p_item->cfg_reg_addr,
				  &reg_value)) {
			return -1;
		}

		reg_value &= ~p_item->cfg_reg_mask;
		if (p_item->split2_val && (set_vol > p_item->split2_val)) {
			base_step = (p_item->split2_val - p_item->split1_val) /
				    p_item->step1_val;

			base_step += (p_item->split1_val - p_item->min_vol) /
				     p_item->step0_val;
			reg_value |= (base_step +
				      (set_vol - p_item->split2_val/p_item->step2_val*p_item->step2_val) /
					      p_item->step2_val);
		} else if (p_item->split1_val &&
			   (set_vol > p_item->split1_val)) {
			if (p_item->split1_val < p_item->min_vol) {
				axp_err("bad split val(%d) for %s\n",
					p_item->split1_val, name);
			}

			base_step = (p_item->split1_val - p_item->min_vol) /
				    p_item->step0_val;
			reg_value |= (base_step +
				      (set_vol - p_item->split1_val) /
					      p_item->step1_val);
		} else {
			reg_value |=
				(set_vol - p_item->min_vol) / p_item->step0_val;
		}
		if (pmic_bus_write(AXP1530_RUNTIME_ADDR, p_item->cfg_reg_addr,
				   reg_value)) {
			axp_err("unable to set %s\n", name);
			return -1;
		}
	}

	if (onoff < 0) {
		return 0;
	}
	if (pmic_bus_read(AXP1530_RUNTIME_ADDR, p_item->ctrl_reg_addr,
			  &reg_value)) {
		return -1;
	}
	if (onoff == 0) {
		reg_value &= ~(1 << p_item->ctrl_bit_ofs);
	} else {
		reg_value |= (1 << p_item->ctrl_bit_ofs);
	}
	if (pmic_bus_write(AXP1530_RUNTIME_ADDR, p_item->ctrl_reg_addr,
			   reg_value)) {
		axp_err("unable to onoff %s\n", name);
		return -1;
	}
	return 0;
}

static int pmu_axp1530_get_voltage(char *name)
{
	u8 reg_value;
	axp_contrl_info *p_item = NULL;
	u8 base_step;
	int vol;

	p_item = get_ctrl_info_from_tbl(name);
	if (!p_item) {
		return -1;
	}

	if (pmic_bus_read(AXP1530_RUNTIME_ADDR, p_item->ctrl_reg_addr,
			  &reg_value)) {
		return -1;
	}
	if (!(reg_value & (0x01 << p_item->ctrl_bit_ofs))) {
		return 0;
	}

	if (pmic_bus_read(AXP1530_RUNTIME_ADDR, p_item->cfg_reg_addr,
			  &reg_value)) {
		return -1;
	}
	reg_value &= p_item->cfg_reg_mask;
	if (p_item->split2_val) {
		u32 base_step2;
		base_step = (p_item->split1_val - p_item->min_vol) /
				     p_item->step0_val;

		base_step2 = base_step + (p_item->split2_val - p_item->split1_val) /
			    p_item->step1_val;

		if (reg_value >= base_step2) {
			vol = ALIGN(p_item->split2_val, p_item->step2_val) +
			      p_item->step2_val * (reg_value - base_step2);
		} else if (reg_value >= base_step) {
			vol = p_item->split1_val +
			      p_item->step1_val * (reg_value - base_step);
		} else {
			vol = p_item->min_vol + p_item->step0_val * reg_value;
		}
	} else if (p_item->split1_val) {
		base_step = (p_item->split1_val - p_item->min_vol) /
			    p_item->step0_val;
		if (reg_value > base_step) {
			vol = p_item->split1_val +
			      p_item->step1_val * (reg_value - base_step);
		} else {
			vol = p_item->min_vol + p_item->step0_val * reg_value;
		}
	} else {
		vol = p_item->min_vol + p_item->step0_val * reg_value;
	}
	return vol;
}

static int pmu_axp1530_set_power_off(void)
{
	u8 reg_value;
	if (pmic_bus_read(AXP1530_RUNTIME_ADDR, AXP1530_POWER_DOMN_SEQUENCE, &reg_value)) {
		return -1;
	}
	reg_value |= (1<<7);
	if (pmic_bus_write(AXP1530_RUNTIME_ADDR, AXP1530_POWER_DOMN_SEQUENCE, reg_value)) {
		return -1;
	}
	return 0;
}



static int pmu_axp1530_get_key_irq(void)
{
	u8 reg_value;
	if (pmic_bus_read(AXP1530_RUNTIME_ADDR, AXP1530_IRQ_STATUS, &reg_value)) {
		return -1;
	}
	reg_value &= (0x03 << 4);
	if (reg_value) {
		if (pmic_bus_write(AXP1530_RUNTIME_ADDR, AXP1530_IRQ_STATUS,
				   reg_value)) {
			return -1;
		}
	}
	return (reg_value >> 4) & 3;
}

unsigned char pmu_axp1530_get_reg_value(unsigned char reg_addr)
{
	u8 reg_value;
	if (pmic_bus_read(AXP1530_RUNTIME_ADDR, reg_addr, &reg_value)) {
		return -1;
	}
	return reg_value;
}

unsigned char pmu_axp1530_set_reg_value(unsigned char reg_addr, unsigned char reg_value)
{
	unsigned char reg;
	if (pmic_bus_write(AXP1530_RUNTIME_ADDR, reg_addr, reg_value)) {
		return -1;
	}
	if (pmic_bus_read(AXP1530_RUNTIME_ADDR, reg_addr, &reg)) {
		return -1;
	}
	return reg;
}


U_BOOT_AXP_PMU_INIT(pmu_axp1530) = {
	.pmu_name	  = "pmu_axp1530",
	.get_info		= pmu_axp1530_get_info,
	.probe		   = pmu_axp1530_probe,
	.set_voltage       = pmu_axp1530_set_voltage,
	.get_voltage       = pmu_axp1530_get_voltage,
	.set_power_off     = pmu_axp1530_set_power_off,
	/*.set_sys_mode      = pmu_axp1530_set_sys_mode,*/
	/*.get_sys_mode      = pmu_axp1530_get_sys_mode,*/
	.get_key_irq       = pmu_axp1530_get_key_irq,
	/*.set_bus_vol_limit = pmu_axp1530_set_bus_vol_limit,*/
	.get_reg_value	   = pmu_axp1530_get_reg_value,
	.set_reg_value	   = pmu_axp1530_set_reg_value,
};
