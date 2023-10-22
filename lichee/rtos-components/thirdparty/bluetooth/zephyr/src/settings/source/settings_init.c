/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include <errno.h>

#include "settings/settings.h"
#include "include/settings/settings_file.h"
#include <zephyr.h>

#define SETTING_NO_DEV_REGISTER 0
#define SETTING_ONE_DEV_REGISTER 1

uint8_t settings_subsys_initialized;

void settings_init(void);
#if defined(CONFIG_BT_DEINIT)
int settings_deinit(void);
#endif

int settings_backend_init(void);

#if defined(CONFIG_BT_DEINIT)
int settings_backend_deinit(void);
#endif

int settings_subsys_init(void)
{

	int err = 0;

	if (settings_subsys_initialized) {
		settings_subsys_initialized++;
		return 0;
	}

	settings_init();

	err = settings_backend_init(); /* func rises kernel panic once error */

	if (!err) {
		settings_subsys_initialized++;
	}

	return err;
}

#if defined(CONFIG_BT_DEINIT)
int settings_subsys_deinit(void)
{
	int err = 0;

	if (settings_subsys_initialized == SETTING_NO_DEV_REGISTER) {
		return -EPERM;
	} else if (settings_subsys_initialized > SETTING_ONE_DEV_REGISTER) {
		settings_subsys_initialized--;
		return 0;
	}

	err = settings_backend_deinit();
	if (err) {
		return err;
	}

	err = settings_deinit();
	if (err) {
		return err;
	}

	settings_subsys_initialized--;

	return 0;
}
#endif

