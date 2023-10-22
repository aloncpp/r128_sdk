/*
 * Copyright (C) 2020 allwinnertech Ltd.
 */

#include <sunxi_hal_power.h>
#include <sunxi_hal_power_private.h>
#include "../gpadc_power.h"
#include <stdlib.h>

static int power_init = -1;
static struct power_dev *rdev;

int hal_power_init(void)
{
	int pmu_id;

	rdev = malloc(sizeof(struct power_dev));
	if (!rdev) {
		printf("%s:malloc rdev failed\n", __func__);
		return -1;
	}

	rdev->config = malloc(sizeof(struct power_supply));
	if (!rdev->config) {
		printf("%s:malloc rdev.config failed\n", __func__);
		return -1;
	}
	pmu_id = gpadc_get_power(rdev);
	if (pmu_id == GPADC_POWER) {
		gpadc_init_power(rdev);
		power_init = 1;
	} else {
		pr_err("hal power init gpadc failed\n");
		power_init = 0;
	}
	return 0;
}

void hal_power_deinit(void)
{
	free(rdev);
}

int hal_power_get(struct power_dev *rdev)
{
	if (power_init != 1) {
		pr_err("hal power init failed\n");
		return 0;
	}
	gpadc_get_power(rdev);
	rdev->config = malloc(sizeof(struct power_supply));
	if (!rdev->config) {
		printf("%s:malloc failed\n", __func__);
		return -1;
	}
	return 0;
}

int hal_power_put(struct power_dev *rdev)
{
	free(rdev->config);

	return 0;
}

