#include "cpufreq.h"
#include <hal_clk.h>

typedef struct cpu_freq_setting
{
	const uint32_t freq;
	int div_hw_cnt;
	hal_clk_type_t parent_clk_type;
	hal_clk_id_t parent_clk_id;
	uint32_t voltage; //unit: mV
} cpu_freq_setting_t;

static cpu_freq_setting_t cpu_freq_table[] =
{
	{ 480000000, 2, HAL_SUNXI_AON_CCU, CLK_CKPLL_C906_SEL, 1100},
	{ 640000000, 2, HAL_SUNXI_AON_CCU, CLK_CKPLL_C906_SEL, 1200},
};

static inline uint32_t __get_available_cpu_freq(uint32_t freq_index)
{
	return cpu_freq_table[freq_index].freq;
}

static inline uint32_t __get_available_cpu_voltage(uint32_t freq_index)
{
	return cpu_freq_table[freq_index].voltage;
}

uint32_t get_available_cpu_freq_num(void)
{
	return sizeof(cpu_freq_table)/sizeof(cpu_freq_table[0]);
}

int get_available_cpu_freq(uint32_t freq_index, uint32_t *cpu_freq)
{
	if (freq_index < 0 || freq_index >= get_available_cpu_freq_num())
		return -1;

	*cpu_freq = __get_available_cpu_freq(freq_index);
	return 0;
}

int get_available_cpu_freq_info(uint32_t freq_index, uint32_t *cpu_freq, uint32_t *cpu_voltage)
{
	if (freq_index < 0 || freq_index >= get_available_cpu_freq_num())
		return -1;

	*cpu_freq = __get_available_cpu_freq(freq_index);
	*cpu_voltage = __get_available_cpu_voltage(freq_index);
	return 0;
}

#define GPRCM_BASE_ADDR 0x40050000
#define APP_LDO_CTRL_REG_OFFSET 0x44
#define MAX_OUTPUT_VOLTAGE 1375
#define MIN_OUTPUT_VOLTAGE 600
#define VOLTAGE_STEP 25
#define MAX_LDO_VOL_FIELD_VALUE 0x1F
static int set_cpu_voltage(uint32_t vol)
{
	uint32_t reg_data, reg_addr, ldo_vol_field;

	if ((vol < MIN_OUTPUT_VOLTAGE) || (vol > MAX_OUTPUT_VOLTAGE))
		return -1;

	ldo_vol_field = ((vol - MIN_OUTPUT_VOLTAGE) + (VOLTAGE_STEP - 1))/VOLTAGE_STEP;

	if (ldo_vol_field > MAX_LDO_VOL_FIELD_VALUE)
	{
		return -2;
	}

	reg_addr = GPRCM_BASE_ADDR + APP_LDO_CTRL_REG_OFFSET;
	reg_data = readl(reg_addr);
	reg_data &= ~(MAX_LDO_VOL_FIELD_VALUE << 9);
	reg_data |= (ldo_vol_field & MAX_LDO_VOL_FIELD_VALUE) << 9;
	writel(reg_data, reg_addr);

	return 0;
}

int get_cpu_voltage(uint32_t *cpu_voltage)
{
	uint32_t reg_data, reg_addr, ldo_vol_field;

	reg_addr = GPRCM_BASE_ADDR + APP_LDO_CTRL_REG_OFFSET;
	reg_data = readl(reg_addr);
	ldo_vol_field = (reg_data >> 9) & MAX_LDO_VOL_FIELD_VALUE;

	*cpu_voltage = MIN_OUTPUT_VOLTAGE + VOLTAGE_STEP * ldo_vol_field;

	return 0;
}

static int set_first_div_freq(uint32_t target_freq, hal_clk_t next_clk)
{
	int ret = 0;
	hal_clk_t clk_dpll1_rv_div;

	clk_dpll1_rv_div = hal_clock_get(HAL_SUNXI_AON_CCU, CLK_CK1_C906);
	if (!clk_dpll1_rv_div)
	{
		ret = -1;
		goto err_get_clk_dpll3_div;
	}

	ret = hal_clk_set_rate(clk_dpll1_rv_div, target_freq);
	if (HAL_CLK_STATUS_OK != ret)
	{
		ret = -2;
		goto err_set_dpll3_div_rate;
	}

	ret = hal_clk_set_parent(next_clk, clk_dpll1_rv_div);
	if (HAL_CLK_STATUS_OK != ret)
	{
		ret = -3;
		goto err_set_first_mux_parent;
	}

err_set_first_mux_parent:
err_set_dpll3_div_rate:
	hal_clock_put(clk_dpll1_rv_div);

err_get_clk_dpll3_div:
	return ret;
}

int set_cpu_freq(uint32_t target_freq)
{
	int ret = 0;
	uint32_t i = 0, size = get_available_cpu_freq_num();
	int is_increase_freq = 0;
	uint32_t current_clk_freq = 0;
	cpu_freq_setting_t *freq_setting;
	hal_clk_t clk_rv_mux, clk_rv_div;
	hal_clk_t pclk;

	for (i = 0; i < size; i++)
	{
		if (cpu_freq_table[i].freq == target_freq)
			break;
	}

	if (i == size)
		return -1;

	freq_setting = &cpu_freq_table[i];

	if (freq_setting->freq == 0)
		return -2;

	ret = get_cpu_freq(&current_clk_freq);
	if (ret)
	{
		return -3;
	}

	if (current_clk_freq == target_freq)
	{
		return 0;
	}

	if (current_clk_freq < target_freq)
	{
		is_increase_freq = 1;
	}

	if (is_increase_freq)
	{
		ret = set_cpu_voltage(freq_setting->voltage);
		if (ret)
		{
			return -4;
		}
	}

	pclk = hal_clock_get(freq_setting->parent_clk_type, freq_setting->parent_clk_id);
	if (!pclk)
	{
		ret = -5;
		goto err_get_pclk;
	}

	if (freq_setting->div_hw_cnt > 1)
	{
		ret = set_first_div_freq(target_freq, pclk);
		if (ret < 0)
		{
			ret = -6;
			goto err_set_first_div_freq;
		}
	}

	clk_rv_mux = hal_clock_get(HAL_SUNXI_CCU, CLK_RISCV_SEL);
	if (!clk_rv_mux)
	{
		ret = -7;
		goto err_get_hifi5_mux;
	}

	ret = hal_clk_set_parent(clk_rv_mux, pclk);
	if (ret)
	{
		ret = -8;
		goto err_set_parent;
	}

	clk_rv_div = hal_clock_get(HAL_SUNXI_CCU, CLK_RISCV_DIV);
	if (!clk_rv_div)
	{
		ret = -9;
		goto err_get_clk_div;
	}

	ret = hal_clk_set_rate(clk_rv_div, target_freq);
	if (HAL_CLK_STATUS_OK != ret)
	{
		ret = -10;
		goto err_set_rv_div_freq;
	}

	if (!is_increase_freq)
	{
		ret = set_cpu_voltage(freq_setting->voltage);
		if (ret)
		{
			ret = -11;
		}
	}

err_set_rv_div_freq:
	hal_clock_put(clk_rv_div);

err_get_clk_div:

err_set_parent:
	hal_clock_put(clk_rv_mux);

err_get_hifi5_mux:

err_set_first_div_freq:
	hal_clock_put(pclk);

err_get_pclk:
	return ret;
}

int get_cpu_freq(uint32_t *cpu_freq)
{
	hal_clk_t clk = NULL;
	uint32_t clk_freq;

	clk = hal_clock_get(HAL_SUNXI_CCU, CLK_RISCV_DIV);
	if (!clk)
	{
		return -1;
	}

	clk_freq = hal_clk_get_rate(clk);
	hal_clock_put(clk);

	if (clk_freq == 0)
	{
		return -2;
	}

	*cpu_freq = clk_freq;
	return 0;
}

