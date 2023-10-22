/*
 * Copyright 2019 AllWinnertech  Co., Ltd
 * frank@allwinnertech.com
 */
#include <hal_clk.h>
#include <drivers/hal_cpufreq.h>
#include <drivers/hal_regulator.h>

/* SUN8IW18P1 */
#ifdef CONFIG_ARCH_SUN8IW18P1
static int sunxi_set_cpufreq(struct cpufreq_info *info,
			     unsigned int target_freq)
{
	int rc;

	rc = hal_clk_set_parent(HAL_CLK_BUS_C0_CPU, HAL_CLK_PLL_PERI0);
	if (rc != HAL_CLK_STATUS_OK)
		return HAL_CPUFREQ_STATUS_ERROR;

	rc = hal_clk_set_rate(HAL_CLK_PLL_CPUX_C0, target_freq);
	if (rc != HAL_CLK_STATUS_OK) {
		hal_clk_set_parent(HAL_CLK_BUS_C0_CPU, HAL_CLK_PLL_CPUX_C0);
		return HAL_CPUFREQ_STATUS_ERROR;
	}

	rc = hal_clk_set_parent(HAL_CLK_BUS_C0_CPU, HAL_CLK_PLL_CPUX_C0);
	if (rc != HAL_CLK_STATUS_OK)
		return HAL_CPUFREQ_STATUS_ERROR;

	return HAL_CPUFREQ_STATUS_OK;
}
#endif /* CONFIG_ARCH_SUN8IW18P1 */

static int sunxi_set_cpuvol(struct cpufreq_info *info,
			    unsigned int target_freq)
{
	struct cpufreq_frequency_table *freq_table = info->freq_table;
	int rc, target_uV = 0;

	for (; freq_table->target_uV; freq_table++) {
		if (freq_table->frequency != target_freq)
			continue;

		target_uV = freq_table->target_uV;
	}

	if (!target_uV)
		return HAL_CPUFREQ_STATUS_ERROR;

	rc = hal_regulator_set_voltage(info->rdev, target_uV);
	if (!rc)
		return HAL_CPUFREQ_STATUS_OK;

	return HAL_CPUFREQ_STATUS_ERROR;
}

static int cpufreq_target(struct cpufreq_info *info,
			  unsigned int target_freq)
{
	unsigned int cur_rate;
	int rc;

	rc = hal_clk_get_rate(info->clk);
	if ((rc == HAL_CLK_STATUS_INVALID_PARAMETER) ||
		(rc == HAL_CLK_RATE_UNINITIALIZED))
		return HAL_CPUFREQ_STATUS_ERROR;

	cur_rate = rc;

	if (target_freq > cur_rate) {
		if (info->rdev) {
			rc = sunxi_set_cpuvol(info, target_freq);
			vTaskDelay(1000/portTICK_RATE_MS);
			if (rc)
				return rc;
		}

		rc = sunxi_set_cpufreq(info, target_freq);
		if (rc)
			return rc;
	} else if (target_freq < cur_rate) {
		rc = sunxi_set_cpufreq(info, target_freq);
		if (rc)
			return rc;

		if (info->rdev) {
			rc = sunxi_set_cpuvol(info, target_freq);
			vTaskDelay(1000/portTICK_RATE_MS);
			if (rc)
				return rc;
		}
	}

	return HAL_CPUFREQ_STATUS_OK;
}

hal_cpufreq_status_t hal_cpufreq_info_get(unsigned int cpu,
					  struct cpufreq_info **info)
{
	int rc;

	rc = cpufreq_info_get(cpu, info);
	if (!rc)
		return HAL_CPUFREQ_STATUS_OK;

	return HAL_CPUFREQ_STATUS_ERROR;
}

void hal_cpufreq_regulator_set(struct cpufreq_info *info,
			       struct regulator_dev *rdev)
{
	info->rdev = rdev;
}

hal_cpufreq_status_t hal_cpufreq_target(struct cpufreq_info *info,
					unsigned int target_freq)
{
	int rc;

	rc = cpufreq_target(info, target_freq);
	if (!rc)
		return HAL_CPUFREQ_STATUS_OK;

	return HAL_CPUFREQ_STATUS_ERROR;
}
