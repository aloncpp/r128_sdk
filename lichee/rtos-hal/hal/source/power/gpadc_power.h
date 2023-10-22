/*
 * (C) Copyright 2010-2016
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Pannan <pannan@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */
#ifndef GPADC_POWER_H_
#define GPADC_POWER_H_

#include "ffs.h"
#include <stddef.h>
#include <sunxi_hal_power.h>
#include <sunxi_hal_power_private.h>

#define GPADC_MANUFACTURER  "xpower,gpadc"

int gpadc_init_power(struct power_dev *rdev);

int gpadc_get_power(struct power_dev *rdev);

#endif /* GPADC_POWER_H_ */
