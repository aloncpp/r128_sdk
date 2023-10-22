#include <stdio.h>
#include <console.h>
#include <FreeRTOS.h>
#include <hal_time.h>
#include <sunxi_hal_power.h>
#include <sunxi_hal_watchdog.h>
#include "aw_types.h"

/* #define HEALTHD_DEBUG */

#ifdef HEALTHD_DEBUG
#define DMSG(fmt, arg...) printf("[HEALTHD]:"fmt"", ##arg)
#else
#define DMSG(fmt, arg...)
#endif

#define DWARN(fmt, arg...) printf("Warning![HEALTHD]:"fmt"", ##arg)
#define DERR(fmt, arg...) printf("Error![HEALTHD]:"fmt"", ##arg)

static struct power_dev rdev;

struct battrey_properties {
	bool charger_usb_online;
	bool battery_cap_warn1;
	bool battery_cap_warn2;
	int battery_present;
	int battery_level;
	int battery_voltage;
	int battery_temp_ambient;
};

static int battery_update(struct battrey_properties *props)
{
	DMSG("before get healthd data: usb-%d bat-:%d level-%d vol-%d tide-%d\n",
			props->charger_usb_online, props->battery_present,
			props->battery_level, props->battery_voltage,
			props->battery_temp_ambient);

	if (props->battery_present && !props->charger_usb_online) {
		/* battery low capacity warning */
		if (props->battery_level <= 15 && props->battery_cap_warn1 == false) {
			DWARN("battery level is lower than level2-15%\n");
			props->battery_cap_warn1 = true;
		}
		if (props->battery_level <= 5 && props->battery_cap_warn2 == false) {
			DWARN("battery level is lower than level1-5%\n");
			props->battery_cap_warn2 = true;
		}
		if (props->battery_level > 15)
			props->battery_cap_warn1 = false;
		if (props->battery_level > 5)
			props->battery_cap_warn2 = false;

		/* battery low voltage reboot */
		if (props->battery_voltage <= 3300) {
			DWARN("battery voltage is %d, lower than 3.3V, system reboot\n", props->battery_voltage);
			hal_watchdog_restart();
		}
	}
	if (props->battery_temp_ambient >= 110) {
		DWARN("ic temp is %d, over than 110, close BATFET and poweroff\n", props->battery_temp_ambient);
		hal_power_set_batfet(&rdev, 1);
	} else if (props->battery_temp_ambient <= -10) {
		DWARN("ic temp is %d, lower than -10, close BATFET and poweroff\n", props->battery_temp_ambient);
		hal_power_set_batfet(&rdev, 1);
	}

	return 0;
}

static void healthd_work(void *arg)
{
	struct battrey_properties props;

	hal_power_get(&rdev);

	props.battery_cap_warn1 = false;
	props.battery_cap_warn2 = false;

	while (1) {
		props.charger_usb_online = false;
		props.battery_present = 0;
		props.battery_level = 0;
		props.battery_voltage = 0;
		props.battery_temp_ambient = 0;

		props.charger_usb_online = hal_power_get_usb_status(&rdev);
		DMSG("usb online:%d 0:[off] 1:[on]\n", props.charger_usb_online);

		props.battery_present = hal_power_get_bat_present(&rdev);
		DMSG("battery present:%d 0:[off] 1:[on]\n", props.battery_present);

		props.battery_level = hal_power_get_bat_cap(&rdev);
		DMSG("battery level:%d\n", props.battery_level);

		props.battery_voltage = hal_power_get_vbat(&rdev);
		DMSG("battery voltage:%d\n", props.battery_voltage);

		props.battery_temp_ambient = hal_power_get_temp_ambient(&rdev);
		DMSG("ic temp:%d\n", props.battery_temp_ambient);

		battery_update(&props);

		DMSG("\n");

		hal_msleep(3 * 1000);
	}
}

static int cmd_healthd(void)
{
	int ret= 0;
	ret = xTaskCreate(healthd_work, "healthd",
			4096, NULL, 6, NULL);
	if (ret != pdPASS) {
		printf("Error: failed to start healthd xTask!\n");
		return -1;
	}

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_healthd, healthd, healthd for rtos);

static int cmd_get_capacity(void)
{
	int ret;
	struct power_dev dev;

	hal_power_get(&dev);

	ret = hal_power_get_bat_cap(&dev);
	if (ret < 0) {
		DERR("healthd get battery capacity failed\n");
		return -1;
	} else {
		printf("battery capacity is %d\n", ret);
	}

	hal_power_put(&dev);
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_get_capacity, healthd_get_capacity, get battery capacity);

static int cmd_set_chgcur(int argc, const char **argv)
{
	struct power_dev rdev;
	int cur = 0, ret = 0;

	if (argv[1] == NULL) {
		DERR("healthd: please input mA\n");
		return -1;
	}

	cur = strtol(argv[1], NULL, 0);
	if (cur > 3072)
		DERR("more than 3072mA, set charing current to 3072mA\n");

	ret = hal_power_set_chg_cur(&rdev, cur);
	if (ret < 0) {
		DERR("set chg cur failed\n");
		return -1;
	}

	hal_power_put(&rdev);
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_set_chgcur, healthd_set_chgcur, set charing current);
