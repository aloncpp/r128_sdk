/*
 * gpadc power
 */
#include <sunxi_hal_power.h>
#include <sunxi_hal_power_private.h>
#include "gpadc_power.h"
#include "type.h"
#ifdef CONFIG_COMPONENTS_PM
#include <pm_devops.h>
#endif
#include <hal_interrupt.h>
#include <hal_atomic.h>
#include <hal_clk.h>
#include <hal_sem.h>
#include <hal_timer.h>
#include <sunxi_hal_gpadc.h>

#define GPADC_POWER_CHANNEL 8

static inline float votage_2_capacity(float votage)
{
    static float res = 0;
    const float coeff[2][12] = {
        { 3.00,3.45,3.68,3.74,3.77,3.79,3.82,3.87,3.92,3.98,4.06,4.20 },
        { 0.0,0.05,0.1,0.2,0.3,0.4,0.5,0.6,0.7,0.8,0.9,1.0 }
    };
    if (votage >= 3.0 && votage <= 4.2) {
        int i;
        for (i = 11; i > 0 && votage < coeff[0][i]; i--) {
        }
        res = coeff[1][i];
    }
    return res;
}

/* battery ops */
static int gpadc_get_rest_cap(struct power_dev *rdev)
{
    uint32_t vol_data;
    vol_data = gpadc_read_channel_data(GPADC_POWER_CHANNEL) * 3;
    float cap = votage_2_capacity((float)vol_data / 1000.0f);
    // pr_info("channel %d vol data is %u cap=%f\n", GPADC_POWER_CHANNEL, vol_data, cap);
    // 0-100
    return (int)(cap*100.0);
}

static int gpadc_get_bat_present(struct power_dev *rdev )
{
	return 1; // battery detected
}

static int gpadc_get_direction(struct power_dev *rdev)
{
    return 1; // battery discharge
}

static int gpadc_get_bat_status(struct power_dev *rdev)
{
	bool bat_det;
	unsigned int rest_cap;

	bat_det = gpadc_get_bat_present(rdev);

	rest_cap = gpadc_get_rest_cap(rdev);

	if (bat_det) {
		if (rest_cap == 100)
			return POWER_SUPPLY_STATUS_FULL;
    }
    return POWER_SUPPLY_STATUS_UNKNOWN;
}

static int gpadc_get_bat_health(struct power_dev *rdev)
{
	return POWER_SUPPLY_HEALTH_GOOD;
}

static int gpadc_get_vbat(struct power_dev *rdev)
{
    uint32_t vol_data;
    vol_data = gpadc_read_channel_data(GPADC_POWER_CHANNEL) * 3;
    // pr_info("channel %d vol data is %u\n", GPADC_POWER_CHANNEL, vol_data);
    return (int)vol_data;
}

static int gpadc_get_bat_temp(struct power_dev *rdev)
{
	return 26;
}

/* usb ops */
static int gpadc_read_vbus_state(struct power_dev *rdev)
{
	return 0; //charger usb offline
}

struct bat_power_ops gpadc_bat_power_ops = {
	.get_rest_cap         = gpadc_get_rest_cap, //0-100
	.get_coulumb_counter  = NULL,
	.get_bat_present      = gpadc_get_bat_present, // use batery on[1] off[0]
	.get_bat_online       = gpadc_get_direction,
	.get_bat_status       = gpadc_get_bat_status, // ok
	.get_bat_health       = gpadc_get_bat_health, // ok
	.get_vbat             = gpadc_get_vbat, // 3300 - 4200mV
	.get_ibat             = NULL,
	.get_disibat          = NULL,
	.get_temp             = gpadc_get_bat_temp, // 26°
	.get_temp_ambient     = gpadc_get_bat_temp, // 26°
	.set_chg_cur          = NULL,
	.set_chg_vol          = NULL,
	.set_batfet           = NULL,
};

struct usb_power_ops gpadc_usb_power_ops = {
	.get_usb_status  = gpadc_read_vbus_state,
	.get_usb_ihold   = NULL,
	.get_usb_vhold   = NULL,
	.set_usb_ihold   = NULL,
	.set_usb_vhold   = NULL,

#if defined(CONFIG_AXP2585_TYPE_C)
	.get_cc_status     = NULL,
#endif
};

/* init chip & power setting*/
int gpadc_init_power(struct power_dev *rdev)
{
    int ret;
    ret = hal_gpadc_init();
    if (ret) {
        pr_err("pmu type simple gpadc!\n");
        return -1;
    }
    hal_gpadc_channel_init(GPADC_POWER_CHANNEL);
	pr_info("gpadc init finished !\n");

	return 0;
}

/* get power ops*/
int gpadc_get_power(struct power_dev *rdev)
{
	int err;
	u8 chip_id = 0;
	static int pmu_type = -1;

	if (pmu_type == -1) {
        pmu_type = AXP2585_UNKNOWN;
        pr_info("pmu type simple gpadc!\n");
	}

	// rdev->config->pmu_version = pmu_type;

	rdev->bat_ops = &gpadc_bat_power_ops;
	rdev->usb_ops = &gpadc_usb_power_ops;

	return GPADC_POWER;
}
