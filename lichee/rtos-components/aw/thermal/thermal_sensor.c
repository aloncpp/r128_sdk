#include <stdint.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <io.h>
#include <string.h>
#include <sunxi_hal_efuse.h>
#include <hal_time.h>
#include <hal_atomic.h>

#include "thermal.h"
#include "thermal_sensor.h"

#ifdef CONFIG_ARCH_SUN20IW2P1
#include "sunxi_hal_gpadc.h"
#endif

/* the maximum of all platform thermal_sensor_chip */
#define THERMAL_MAX_SENSOR_NUM	(1)

struct thermal_sensor_chip {
	char *name;
	char **zone_name;

	int sensor_num;
	int slope;
	int intercept;
	int temp_conpensation;

	int (*init)(struct thermal_sensor_chip *ths);
	int (*deinit)(struct thermal_sensor_chip *ths);
	int (*calibrate)(struct thermal_sensor_chip *ths);
	int (*get_temp)(struct thermal_sensor_chip *ths, int *temp);

	struct ths_device *ths_dev;
};

struct ths_device {
	struct thermal_sensor_chip *ths;
	struct thermal_zone_device *tzd[THERMAL_MAX_SENSOR_NUM];
};

static struct ths_device *thsdev;

#ifdef CONFIG_ARCH_SUN20IW2
/* ths_sun20i */
#define SUN20I_THS_CCMU_AON_BASE		(0x4004c400)
#define SUN20I_THS_MODULE_RST_REG		(SUN20I_THS_CCMU_AON_BASE + 0xc8)
#define SUN20I_THS_GPADC_RSTN_MASK		(0x1 << 2)
#define SUN20I_THS_MODULE_CLK_EN_REG		(SUN20I_THS_CCMU_AON_BASE + 0xcc)
#define SUN20I_THS_GPADC_CLK_GATING_MASK	(0x1 << 2)
#define SUN20I_THS_GPADC_CLK_CRTL_REG		(SUN20I_THS_CCMU_AON_BASE + 0xd8)
#define SUN20I_THS_GPADC_DIV_N_MASK		(0x3 << 16)

#define SUN20I_THS_GPADC_BASE			(0x4004a000)
#define SUN20I_THS_GPADC_SAMPLE_RATE_REG	(SUN20I_THS_GPADC_BASE + 0x00)
#define SUN20I_THS_GPADC_FS_DIV_MASK		(0xffff << 16)
#define SUN20I_THS_GPADC_TACQ_MASK		(0xffff << 0)
#define SUN20I_THS_GPADC_CTRL_REG		(SUN20I_THS_GPADC_BASE + 0x04)
#define SUN20I_THS_GPADC_WORK_MODE_MASK		(0x3 << 18)
#define SUN20I_THS_GPADC_ADC_CALI_EN_MASK	(0x1 << 17)
#define SUN20I_THS_GPADC_ADC_EN_MASK		(0x1 << 16)
#define SUN20I_THS_GPADC_LP_TEMPSENS_MASK	(0x1 << 7)
#define SUN20I_THS_GPADC_EN_TEMPSENS_MASK	(0x1 << 6)
#define SUN20I_THS_GPADC_EN_VBAT_MASK		(0x1 << 5)
#define SUN20I_THS_GPADC_VRF_MODE_SEL_MASK	(0x7 << 1)
#define SUN20I_THS_GPADC_ADC_LDO_EN_MASK	(0x1 << 0)
#define SUN20IW2_THS_CHAN			(12)
#define SUN20I_GPADC_20M_MASK			(0x18f << 16)

static int ths_init_sun20i(struct thermal_sensor_chip *ths)
{
	int ret = 0;

	ret = hal_gpadc_init();
	if (ret) {
		thermal_err("hal gpadc init failed, return %d\n", ret);
		return ret;
	}

	if ((readl(SUN20I_THS_GPADC_SAMPLE_RATE_REG) & SUN20I_GPADC_20M_MASK) != SUN20I_GPADC_20M_MASK) {
		/* div_n = 2. Set to 20M. the ths clk can not exceed 24M */
		writel(readl(SUN20I_THS_MODULE_CLK_EN_REG) & ~SUN20I_THS_GPADC_CLK_GATING_MASK,
			SUN20I_THS_MODULE_CLK_EN_REG);
		writel(readl(SUN20I_THS_MODULE_RST_REG) & ~SUN20I_THS_GPADC_RSTN_MASK,
			SUN20I_THS_MODULE_RST_REG);

		writel((readl(SUN20I_THS_GPADC_CLK_CRTL_REG) & ~SUN20I_THS_GPADC_DIV_N_MASK)
			| (0x1 << 16),
			SUN20I_THS_GPADC_CLK_CRTL_REG);

		writel(readl(SUN20I_THS_MODULE_RST_REG) | SUN20I_THS_GPADC_RSTN_MASK,
			SUN20I_THS_MODULE_RST_REG);
		writel(readl(SUN20I_THS_MODULE_CLK_EN_REG) | SUN20I_THS_GPADC_CLK_GATING_MASK,
			SUN20I_THS_MODULE_CLK_EN_REG);

		/* sample rate = 50KHz */
		writel((readl(SUN20I_THS_GPADC_SAMPLE_RATE_REG) & ~SUN20I_THS_GPADC_FS_DIV_MASK)
			| (0x18f << 16),
			SUN20I_THS_GPADC_SAMPLE_RATE_REG);
		/* ADC acquire time.
		 * HOSC = 40KHz, tacq = 0x4f
		 * HOSC = 20KHz, tacq = 0x27
		 */
		writel((readl(SUN20I_THS_GPADC_SAMPLE_RATE_REG) & ~SUN20I_THS_GPADC_TACQ_MASK)
			| (0x27 << 0),
			SUN20I_THS_GPADC_SAMPLE_RATE_REG);
	}

	/* adc_en = 0 */
	writel(readl(SUN20I_THS_GPADC_CTRL_REG) & ~SUN20I_THS_GPADC_ADC_EN_MASK,
		SUN20I_THS_GPADC_CTRL_REG);

	/* vref_mode_sel = 1, 2.5V from BG, chosen when vdd is 3V~3.6V */
	writel((readl(SUN20I_THS_GPADC_CTRL_REG) & ~SUN20I_THS_GPADC_VRF_MODE_SEL_MASK)
		| (0x1 << 1),
		SUN20I_THS_GPADC_CTRL_REG);

	/* lp_tempsens = 0, using in low power mode */
	writel(readl(SUN20I_THS_GPADC_CTRL_REG) & ~SUN20I_THS_GPADC_LP_TEMPSENS_MASK,
		SUN20I_THS_GPADC_CTRL_REG);
	/* en_tempsens = 1, using in the normal mode */
	writel(readl(SUN20I_THS_GPADC_CTRL_REG) | SUN20I_THS_GPADC_EN_TEMPSENS_MASK,
		SUN20I_THS_GPADC_CTRL_REG);
	writel(readl(SUN20I_THS_GPADC_CTRL_REG) | SUN20I_THS_GPADC_EN_VBAT_MASK,
		SUN20I_THS_GPADC_CTRL_REG);

	/* auto calibration one time. calibrate by calibration callback in this case.
	writel(readl(SUN20I_THS_GPADC_CTRL_REG) | SUN20I_THS_GPADC_ADC_CALI_EN_MASK,
		SUN20I_THS_GPADC_CTRL_REG);
	*/

	/* work mode = continuous conversion */
	writel((readl(SUN20I_THS_GPADC_CTRL_REG) & ~SUN20I_THS_GPADC_WORK_MODE_MASK)
		| (0x2 << 18),
		SUN20I_THS_GPADC_CTRL_REG);

	/* adc_ldo_en = 1 */
	writel(readl(SUN20I_THS_GPADC_CTRL_REG) | SUN20I_THS_GPADC_ADC_LDO_EN_MASK,
		SUN20I_THS_GPADC_CTRL_REG);

	/* adc_en = 1 */
	writel(readl(SUN20I_THS_GPADC_CTRL_REG) | SUN20I_THS_GPADC_ADC_EN_MASK,
		SUN20I_THS_GPADC_CTRL_REG);

	return ret;
}

static int ths_deinit_sun20i(struct thermal_sensor_chip *ths)
{
	int ret = 0;

	return ret;
}

#define SUN20I_THS_GPADC_CDATA_REG		(SUN20I_THS_GPADC_BASE + 0x18)
#define SUN20I_THS_THS1_START			(453)
#define SUN20I_THS_THS_ROOM_START		(505)
#define SUN20I_THS_GPADC_OFFSET_START		(487)
#define SUN20I_THS_THS1_BIT_LEN			(12)
#define SUN20I_THS_THS_ROOM_BIT_LEN		(7)
#define SUN20I_THS_GPADC_OFFSET_BIT_LEN		(8)
uint32_t ths1 = 0;
uint32_t ths_room = 0;
uint32_t gpadc_offset = 0;
static int ths_calibrate_sun20i(struct thermal_sensor_chip *ths)
{
	int ret = 0;
	int i;
	int ths1_data_len = (SUN20I_THS_THS1_BIT_LEN + 7) / 8;
	int ths_room_data_len = (SUN20I_THS_THS_ROOM_BIT_LEN + 7) / 8;
	int gpadc_offset_len = (SUN20I_THS_GPADC_OFFSET_BIT_LEN + 7) / 8;
	uint8_t ths1_data[(SUN20I_THS_THS1_BIT_LEN + 7) / 8];
	uint8_t ths_room_data[(SUN20I_THS_THS_ROOM_BIT_LEN + 7) / 8];
	uint8_t gpadc_offset_data[(SUN20I_THS_GPADC_OFFSET_BIT_LEN + 7) / 8];

	/* wait gpadc calibrate end, and write tmp calibrate code */
	hal_msleep(10);

	memset(ths1_data, 0, ths1_data_len);
	memset(ths_room_data, 0, ths_room_data_len);
	memset(gpadc_offset_data, 0, gpadc_offset_len);

	ret = hal_efuse_read_ext(SUN20I_THS_GPADC_OFFSET_START, SUN20I_THS_GPADC_OFFSET_BIT_LEN, gpadc_offset_data);
	if (ret) {
		thermal_err("read gpadc offset failed, return %d\n", ret);
		return -EFAULT;
	}

	for (i = 0; i < gpadc_offset_len; i++) {
		thermal_dbg("read gpadc offset byte %d: 0x%x\n", i, gpadc_offset_data[i]);
		gpadc_offset |= ((uint32_t)gpadc_offset_data[i]) << (i * 8);
	}
	if (gpadc_offset == 0) {
		thermal_warn("gpadc offset does not exist, read temperature with large deviation\n");
		writel((readl(SUN20I_THS_GPADC_CDATA_REG) & ~(0xfff)) | 0x800, SUN20I_THS_GPADC_CDATA_REG);
	} else {
		if (gpadc_offset & (0x1 << (SUN20I_THS_GPADC_OFFSET_BIT_LEN - 1)))
			gpadc_offset = 0x800 + (gpadc_offset & ~(0x1 << (SUN20I_THS_GPADC_OFFSET_BIT_LEN - 1)));
		else
			gpadc_offset = 0x800 - gpadc_offset;

		thermal_dbg("get gpadc_offset: %d\n", gpadc_offset);
		writel((readl(SUN20I_THS_GPADC_CDATA_REG) & ~(0xfff)) | gpadc_offset, SUN20I_THS_GPADC_CDATA_REG);
	}

	ret = hal_efuse_read_ext(SUN20I_THS_THS1_START, SUN20I_THS_THS1_BIT_LEN, ths1_data);
	if (ret) {
		thermal_err("read ths1 failed, return %d\n", ret);
		return -EFAULT;
	}

	for (i = 0; i < ths1_data_len; i++) {
		thermal_dbg("read ths1_data byte %d: 0x%x\n", i, ths1_data[i]);
		ths1 |= ((uint32_t)ths1_data[i]) << (i * 8);
	}
	if (ths1 == 0)
		thermal_err("get ths1 failed\n");
	else {
		ths1 = ((ths1 * 2500 / 4096) - ths->intercept) * ths->slope;
		thermal_dbg("get ths1: %d\n", ths1);
	}

	ret = hal_efuse_read_ext(SUN20I_THS_THS_ROOM_START, SUN20I_THS_THS_ROOM_BIT_LEN, ths_room_data);
	if (ret) {
		thermal_err("read ths_room failed, return %d\n", ret);
		return -EFAULT;
	}

	for (i = 0; i < ths_room_data_len; i++) {
		thermal_dbg("read ths_room_data byte %d: 0x%x\n", i, ths_room_data[i]);
		ths_room |= ((uint32_t)ths_room_data[i]) << (i * 8);
	}
	if (ths_room == 0)
		thermal_err("get ths_room failed\n");
	else {
		ths_room = ths_room * 250 + 10000;
		thermal_dbg("get ths_room: %d, \n", ths_room);
	}

	return ret;
}

#define SUN20I_THS_GPADC_CHn_DATA_REG		(SUN20I_THS_GPADC_BASE + 0x80 + (4 * SUN20IW2_THS_CHAN))
#define SUN20I_THS_GPADC_FIFO_INTC		(SUN20I_THS_GPADC_BASE + 0x0c)
#define LAST_TEMP_INITIALIZATION		((-1) * 273150)
#define READ_SENSOR_DATA_RETRY			(3)
static volatile int get_temp_refcnt = 0;
static int last_temp = LAST_TEMP_INITIALIZATION;

static hal_spinlock_t ths_lock = {
	.owner = 0,
	.counter = 0,
	.spin_lock = {
		.slock = 0,
	},
};

static int ths_get_temp_sun20i(struct thermal_sensor_chip *ths, int *temp)
{
	int ret = 0;
	int vout;
	int t0;
	uint32_t reg_val = 0;
	uint32_t code = 0;
	uint8_t retry = 0;

	/* initialize channel only when it is in use to aviod pointless irq handler of gpadc for sun20ip1 */
	hal_spin_lock(&ths_lock);
	get_temp_refcnt += 1;
	hal_spin_unlock(&ths_lock);
	if (get_temp_refcnt == 1) {
		ret = hal_gpadc_channel_init(SUN20IW2_THS_CHAN);
		if (ret) {
			thermal_err("hal gpadc channal %d init failed, return %d\n", SUN20IW2_THS_CHAN, ret);
			return ret;
		}
		/* clear fifo, aviod get 0 or err data */
		reg_val = readl(SUN20I_THS_GPADC_FIFO_INTC);
		writel(reg_val | (0x1 << 4), SUN20I_THS_GPADC_FIFO_INTC);
		hal_msleep(1);
		if (readl(SUN20I_THS_GPADC_FIFO_INTC) & (0x1 << 4)) {
			thermal_err("thermal clear gpadc fifo failed\n");
			ret = -EFAULT;
			goto channel_exit;
		}
	}

	while (code == 0) {
		code = readl(SUN20I_THS_GPADC_CHn_DATA_REG);
		if (code == 0) {
			hal_msleep(5);
			retry++;
			if (retry == READ_SENSOR_DATA_RETRY)
				break;
		}
	}

	if (code == 0) {
		/* thermal_warn("readl Tsensor data: %d\n", code); */
		if (last_temp == LAST_TEMP_INITIALIZATION) {
			thermal_err("get temp failed\n");
			ret = -EFAULT;
			goto channel_exit;
		}
		/* thermal_warn("return last temperature\n"); */
		*temp = last_temp;
		ret = 0;
		goto channel_exit;
	} else
		vout = code * 2500 / 4096;

	/* vout = k * temp + intercept.
	 * temp is in uses milli-degrees Celsius.
	 * as a result:
	 * temp = (vout - intercept) * 1000 / k;
	 * slope = 1000 / k.
	 */
	t0 = (vout - ths->intercept) * ths->slope;

	if ((ths1 == 0) && (ths_room == 0)) {
		*temp = t0;
	} else
		*temp = t0 - (ths1 - ths_room) + ths->temp_conpensation;

	last_temp = *temp;

channel_exit:
	hal_spin_lock(&ths_lock);
	get_temp_refcnt -= 1;
	hal_spin_unlock(&ths_lock);
	/* initialize channel only when it is in use to aviod pointless irq handler of gpadc for sun20ip1 */
	if (get_temp_refcnt == 0) {
		ret = hal_gpadc_channel_exit(SUN20IW2_THS_CHAN);
		if (ret) {
			thermal_err("hal gpadc channal %d init failed, return %d\n", SUN20IW2_THS_CHAN, ret);
			return ret;
		}
	}

	return ret;
}

char *ths_sun20i_zone_name[] = {"cpu_zone", ""};
static struct thermal_sensor_chip ths_sun20i = {
	.name = "ths_sun20i",
	.zone_name = ths_sun20i_zone_name,
	.sensor_num = 1,
	.slope = -129,
	.intercept = 1694,
	.temp_conpensation = 3000,
	.init = ths_init_sun20i,
	.deinit = ths_deinit_sun20i,
	.calibrate = ths_calibrate_sun20i,
	.get_temp = ths_get_temp_sun20i,
};
#endif /* CONFIG_ARCH_SUN20IW2 */

#ifdef CONFIG_ARCH_SUN20IW2
/* temporarily replace the shuttdown interface, which is not orderly. */
#define THERMAL_GPRACM_DCDC_CTRL0			(0x40050004)
#define THERMAL_EXT_LDO_CTRL_REG			(0x40050024)
#define THERMAL_EXT_LDO_EN_MASK				(0x3 << 0)
#define THERMAL_EXT_LDO_EN_BY_PMU			(0x1 << 0)
static void dcdc_soft_shutdown(void)
{
	uint32_t val;

	/* ext_ldo keep off. */
	/* writel((readl(THERMAL_EXT_LDO_CTRL_REG) & ~THERMAL_EXT_LDO_EN_MASK), THERMAL_EXT_LDO_CTRL_REG); */

	/* dcdc off */
	val = readl(THERMAL_GPRACM_DCDC_CTRL0);
	writel(val | 0x6, THERMAL_GPRACM_DCDC_CTRL0);
	while(1);
}
#else
static void dcdc_soft_shutdown(void)
{

}
#endif

/* thermal zone device */
static int ths_get_temp(void *data, int *temp)
{
	int ret;
	struct ths_device *ths_dev = data;

	ret = ths_dev->ths->get_temp(ths_dev->ths, temp);
	if (ret)
		thermal_err("sensor get temp failed, return %d\n", ret);

	return ret;
}

static struct thermal_zone_device_ops ths_ops = {
	.get_temp = ths_get_temp,
	.shutdown = dcdc_soft_shutdown,
};

/* thermal sensor init */
static struct thermal_sensor_chip *find_ths[] = {
	&ths_sun20i,
};

static struct thermal_sensor_chip *get_match_ths(const char *name)
{
	int i;
	struct thermal_sensor_chip *ths;

	for (i = 0; i < ARRAY_SIZE(find_ths); i++) {
		ths = find_ths[i];
		if (ths && ths->name) {
			if (!strcmp(ths->name, name))
				return ths;
		}
		thermal_dbg("thermal sensor get chip loop: %d\n", i);
	}

	thermal_err("thermal sensor chip don't match\n");

	return NULL;
}

static int thermal_sensor_resource_init(struct ths_device *ths_dev)
{
	int ret = 0;
	struct thermal_sensor_chip *ths;
	char find_name[] = "ths_sun20i";

	ths = get_match_ths(find_name);
	ths->ths_dev = ths_dev;
	ths_dev->ths = ths;

	ret = ths->init(ths);
	if (ret) {
		thermal_err("thermal sensor init failed, return %d\n", ret);
		return ret;
	}

	ret = ths->calibrate(ths);
	if (ret) {
		thermal_err("thermal sensor calibrate failed, return %d\n", ret);
		return ret;
	}

	return ret;
}

static int thermal_sensor_register(struct ths_device *ths_dev)
{
	int ret;
	int i;

	if (!ths_dev->ths)
		return -EINVAL;

	for (i = 0; i < ths_dev->ths->sensor_num; i++) {
		ths_dev->tzd[i] = thermal_zone_device_register(ths_dev->ths->zone_name[i], &ths_ops, ths_dev);
		if (!ths_dev->tzd[i]) {
			thermal_err("thermal sensor register failed, sensor number: %d\n", i);
			goto tzd_unregister;
		}

	}

	return 0;

tzd_unregister:
	for (i = 0; i < ths_dev->ths->sensor_num; i++) {
		if (ths_dev->tzd[i]) {
			ret = thermal_zone_device_unregister(ths_dev->tzd[i]);
			if (ret)
				thermal_err("thermal sensor unregister failed, return %d\n", ret);
		}
	}

	return -EFAULT;

}

int thermal_sensor_init(void)
{
	int ret = 0;

	thsdev = malloc(sizeof(struct ths_device));
	if (!thsdev) {
		thermal_err("ths device malloc failed\n");
		return -ENOMEM;
	}

	ret = thermal_sensor_resource_init(thsdev);
	if (ret) {
		thermal_err("thermal sensor resource init failed, return %d\n", ret);
		goto free_ths_dev;
	}

	ret = thermal_sensor_register(thsdev);
	if (ret) {
		thermal_err("thermal sensor register failed, return %d\n", ret);
		goto free_ths_dev;
	}

	return ret;

free_ths_dev:
	free(thsdev);
	return ret;

}

/* thermal sensor deinit */
static void thermal_sensor_resource_deinit(struct ths_device *ths_dev)
{
	int ret;

	ret = ths_dev->ths->deinit(ths_dev->ths);
	if (ret)
		thermal_err("thermal sensor resourece deinit failed\n");
}

static void thermal_sensor_unregister(struct ths_device *ths_dev)
{
	int ret;
	int i;

	for (i = 0; i < ths_dev->ths->sensor_num; i++) {
		if (ths_dev->tzd[i]) {
			ret = thermal_zone_device_unregister(ths_dev->tzd[i]);
			if (ret)
				thermal_err("thermal sensor unregister failed, return %d\n", ret);
		}
	}
}

void thermal_sensor_deinit(void)
{
	thermal_sensor_unregister(thsdev);
	thermal_sensor_resource_deinit(thsdev);
	free(thsdev);
}
