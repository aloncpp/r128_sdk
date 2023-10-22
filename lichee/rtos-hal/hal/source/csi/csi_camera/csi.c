
#include "csi.h"
#include "hal_mutex.h"

typedef struct {
	uint8_t cap_mode;	/* 0:still, 1:video */
	uint8_t cap_start;
	hal_mutex_t lock;
	struct sensor_function sensor_func;
	struct sensor_config sensor_cfg;
	struct csi_cfg_param csi_cfg;
} csi_private;

static csi_private *gcsi_private;

static csi_private *csi_getpriv()
{
	if (gcsi_private == NULL) {
		csi_err("csi not probe");
		return NULL;
	} else
		return gcsi_private;
}

static void csi_setpriv(csi_private *csi_priv)
{
	gcsi_private = csi_priv;
}

#define CSI_PINCTL_SEL 0

#ifdef CONFIG_DRIVER_SYSCONFIG
#include <hal_cfg.h>
#include <script.h>

typedef struct csi_cfg {
	uint8_t twi_id;
	int reset_pin;
	int pwdn_pin;
} csi_dev_cfg_t;

static csi_dev_cfg_t csi_dev_cfg;

static HAL_Status csi_dev_cfg_parse(void)
{
	int ret = -1, value = 0;
	user_gpio_set_t csi_dev_pin = {0};

	ret = hal_cfg_get_keyvalue(SET_CSI0, KEY_CSI_DEV_TWI_ID, (int32_t *)&value, 1);
	csi_dbg("csi twid:%d, ret:%d\n", value, ret);
	if (ret) {
		csi_err("script_parser_fetch %s %s fail \n", SET_CSI0, KEY_CSI_DEV_TWI_ID);
		return -1;
	}

	csi_dev_cfg.twi_id = value;

	ret = hal_cfg_get_keyvalue(SET_CSI0, KEY_CSI_DEV_RESET_GPIO, (int32_t *)&csi_dev_pin,
				  sizeof(user_gpio_set_t) >> 2);
	if (ret) {
		csi_err("script_parser_fetch %s %s fail \n", SET_CSI0, KEY_CSI_DEV_RESET_GPIO);
		return -1;
	}
	csi_dev_cfg.reset_pin = (csi_dev_pin.port - 1) * PINS_PER_BANK + csi_dev_pin.port_num;
	csi_dbg("%s pin number:%d\n", csi_dev_pin.name, csi_dev_cfg.reset_pin);

	ret = hal_cfg_get_keyvalue(SET_CSI0, KEY_CSI_DEV_PWDN_GPIO, (int32_t *)&csi_dev_pin,
				  sizeof(user_gpio_set_t) >> 2);
	if (ret) {
		csi_err("script_parser_fetch %s %s fail \n", SET_CSI0, KEY_CSI_DEV_PWDN_GPIO);
		return -1;
	}

	csi_dev_cfg.pwdn_pin = (csi_dev_pin.port - 1) * PINS_PER_BANK + csi_dev_pin.port_num;
	csi_dbg("%s pin number:%d\n", csi_dev_pin.name, csi_dev_cfg.pwdn_pin);

	return ret;
}

static HAL_Status csi_sys_pinctrl_config(uint8_t init)
{
	char csi_name[16];
	user_gpio_set_t gpio_cfg[SYS_GPIO_NUM] = {0};
	int count, i, ret;
	gpio_pin_t csi_pin;

	sprintf(csi_name, "csi%d", CSI_PINCTL_SEL);
	count = hal_cfg_get_gpiosec_keycount(csi_name);
	if (!count) {
		csi_err("[csi%d] not support in sys_config\n",
				CSI_PINCTL_SEL);
		return -1;
	}

	ret = hal_cfg_get_gpiosec_data(csi_name, gpio_cfg, count);
	if (ret < 0) {
		csi_err("[csi%d] not seting in sys_config\n", CSI_PINCTL_SEL);
		return -1;
	}

	for (i = 0; i < count; i++) {
		csi_dbg("[csi%d] port:%d,port_num:%d,mul_sel:%d,pull:%d,driv:%d\n",
				CSI_PINCTL_SEL, gpio_cfg[i].port, gpio_cfg[i].port_num,
				gpio_cfg[i].mul_sel,gpio_cfg[i].pull,gpio_cfg[i].drv_level);

		csi_pin = (gpio_cfg[i].port - 1) * PINS_PER_BANK + gpio_cfg[i].port_num;

		if (init) {

			ret = hal_gpio_pinmux_set_function(csi_pin, gpio_cfg[i].mul_sel);
			if (ret) {
				csi_err("[csi%d] pin%d set function failed\n",
						CSI_PINCTL_SEL, csi_pin);
	            return -1;
			}

			ret = hal_gpio_set_driving_level(csi_pin, gpio_cfg[i].drv_level);
	        if (ret) {
	            csi_err("[csi %d] pin%d set driving level failed!\n",
						CSI_PINCTL_SEL, csi_pin);
	            return -1;
	        }

	        ret = hal_gpio_set_pull(csi_pin, gpio_cfg[i].pull);
			if (ret) {
	            csi_err("[csi %d] pin%d set pull failed!\n",
						CSI_PINCTL_SEL, csi_pin);
	            return -1;
	        }
		} else {
			ret = hal_gpio_pinmux_set_function(csi_pin, GPIO_MUXSEL_DISABLED);
			if (ret) {
				csi_err("[csi%d] pin%d set function failed\n",
						CSI_PINCTL_SEL, csi_pin);
	            return -1;
			}
		}
	}

	return ret;
}
#else
struct csi_pinctl gl_csi_pinctls[2][CSI_PIN_NUM] = {
 	{
 		{.csi_gpio = GPIO_PA18, .func = 7,}, //hsync
 		{.csi_gpio = GPIO_PA19, .func = 7,}, //vsync
 		{.csi_gpio = GPIO_PA20, .func = 7,}, //pclk
 		{.csi_gpio = GPIO_PA21, .func = 7,}, //mclk
 		{.csi_gpio = GPIO_PA22, .func = 8,}, //d0
 		{.csi_gpio = GPIO_PA23, .func = 8,}, //d1
 		{.csi_gpio = GPIO_PA24, .func = 8,}, //d6
 		{.csi_gpio = GPIO_PA25, .func = 8,}, //d5
 		{.csi_gpio = GPIO_PA26, .func = 8,}, //d3
 		{.csi_gpio = GPIO_PA27, .func = 8,}, //d2
 		{.csi_gpio = GPIO_PA28, .func = 8,}, //d7
		{.csi_gpio = GPIO_PA29, .func = 8,}, //d4
 	},
 	{
		{.csi_gpio = GPIO_PB0,  .func = 8,}, //hsync
		{.csi_gpio = GPIO_PA1,  .func = 8,}, //vsync
		{.csi_gpio = GPIO_PB14, .func = 8,}, //pclk
		{.csi_gpio = GPIO_PB15, .func = 8,}, //mclk
		{.csi_gpio = GPIO_PA22, .func = 8,}, //d0
		{.csi_gpio = GPIO_PA23, .func = 8,}, //d1
		{.csi_gpio = GPIO_PA24, .func = 8,}, //d6
		{.csi_gpio = GPIO_PA25, .func = 8,}, //d5
		{.csi_gpio = GPIO_PA26, .func = 8,}, //d3
		{.csi_gpio = GPIO_PA27, .func = 8,}, //d2
		{.csi_gpio = GPIO_PA28, .func = 8,}, //d7
		{.csi_gpio = GPIO_PA29, .func = 8,}, //d4
 	},
 };

#endif
static HAL_Status csi_pinctrl_config()
{
#if CONFIG_DRIVER_SYSCONFIG
	return csi_sys_pinctrl_config(1);
#else
	uint8_t i;
	struct csi_pinctl csi_pinctls[CSI_PIN_NUM];

	memcpy(csi_pinctls, gl_csi_pinctls[CSI_PINCTL_SEL], sizeof(struct csi_pinctl) * CSI_PIN_NUM);

	for (i = 0; i < CSI_PIN_NUM; i++) {

		csi_dbg("pin :%d func:%d\n", csi_pinctls[i].csi_gpio, csi_pinctls[i].func);

	    if (hal_gpio_pinmux_set_function(csi_pinctls[i].csi_gpio, csi_pinctls[i].func)) {
		csi_err("[csi%d] PIN set function failed!\n", i);
		return -1;
	    }

	    if (hal_gpio_set_driving_level(csi_pinctls[i].csi_gpio, 3)) {
		csi_err("[csi%d] PIN set driving failed!\n", i);
		return -1;
	    }

	    if (hal_gpio_set_pull(csi_pinctls[i].csi_gpio, GPIO_PULL_DOWN_DISABLED)) {
		csi_err("[csi%d] PIN set driving failed!\n", i);
		return -1;
	   }
	}

	csi_dbg("csi pinctrl config\n");
#endif
	 return 0;
}

static HAL_Status csi_pinctrl_release()
{
#if CONFIG_DRIVER_SYSCONFIG
	return csi_sys_pinctrl_config(0);
#else
	uint8_t i;
	struct csi_pinctl csi_pinctls[CSI_PIN_NUM];

	memcpy(csi_pinctls, gl_csi_pinctls[CSI_PINCTL_SEL], sizeof(struct csi_pinctl) * CSI_PIN_NUM);

	for (i = 0; i < CSI_PIN_NUM; i++) {
	    if (hal_gpio_pinmux_set_function(csi_pinctls[i].csi_gpio, GPIO_MUXSEL_DISABLED)) {
		csi_err("[csi%d] PIN exit function failed\n!", i);
		return -1;
	    }
	}
#endif
	csi_dbg("csi pinctrl release\n");

	return 0;
}

HAL_Status hal_sensor_s_stream(unsigned int on)
{
	csi_private *csi_priv = csi_getpriv();
	HAL_Status ret;

	if (on)
		ret = csi_priv->sensor_func.init(&csi_priv->sensor_cfg);
	else {
		ret = csi_priv->sensor_func.deinit(&csi_priv->sensor_cfg);
//		ret = 0;
	}

	return ret;
}

void __csi_set_fmt_hw()
{
	csi_private *csi_priv = csi_getpriv();
	struct sensor_win_size *sensor_win_sizes;
	unsigned int flags;

	csi_priv->csi_cfg.yuv420_mask = CSI_YUV420_MASK_UV_ODD;
	csi_priv->csi_cfg.yuv420_line_order = CSI_LINE_ORDER_Y_YC_Y_YC;

	csi_priv->sensor_func.g_mbus_config(&flags);
	if (IS_FLAG(flags, MBUS_SEPARATE_SYNCS))
		csi_priv->csi_cfg.sync_type = CSI_SYNC_SEPARARE;
	else
		csi_priv->csi_cfg.sync_type = CSI_SYNC_CCIR656;
	if (IS_FLAG(flags, MBUS_VSYNC_ACTIVE_LOW))
		csi_priv->csi_cfg.vref_pol = CSI_POL_NEGATIVE;
	else
		csi_priv->csi_cfg.vref_pol = CSI_POL_POSITIVE;
	if (IS_FLAG(flags, MBUS_HSYNC_ACTIVE_LOW))
		csi_priv->csi_cfg.href_pol = CSI_POL_NEGATIVE;
	else
		csi_priv->csi_cfg.href_pol = CSI_POL_POSITIVE;
	if (IS_FLAG(flags, MBUS_PCLK_SAMPLE_FALLING))
		csi_priv->csi_cfg.clk_pol = CSI_ACTIVE_FALLING;
	else
		csi_priv->csi_cfg.clk_pol = CSI_ACTIVE_RISING;

	sensor_win_sizes = csi_priv->sensor_func.sensor_win_sizes;
	if (sensor_win_sizes->mbus_code == MEDIA_BUS_FMT_YUYV8_2X8) {
		csi_priv->csi_cfg.input_seq = CSI_IN_SEQ_YUYV;
		csi_priv->csi_cfg.out_mode = CSI_OUT_MODE_YUV422_TO_YUV420;
		csi_priv->csi_cfg.input_fmt = CSI_IN_FMT_YUV422;
	} else if (sensor_win_sizes->mbus_code == MEDIA_BUS_FMT_UYVY8_2X8) {
		csi_priv->csi_cfg.input_seq = CSI_IN_SEQ_UYVY;
		csi_priv->csi_cfg.out_mode = CSI_OUT_MODE_YUV422_TO_YUV420;
		csi_priv->csi_cfg.input_fmt = CSI_IN_FMT_YUV422;
	} else if (sensor_win_sizes->mbus_code == MEDIA_BUS_FMT_VYUY8_2X8) {
		csi_priv->csi_cfg.input_seq = CSI_IN_SEQ_VYUY;
		csi_priv->csi_cfg.out_mode = CSI_OUT_MODE_YUV422_TO_YUV420;
		csi_priv->csi_cfg.input_fmt = CSI_IN_FMT_YUV422;
	} else if (sensor_win_sizes->mbus_code == MEDIA_BUS_FMT_YVYU8_2X8) {
		csi_priv->csi_cfg.input_seq = CSI_IN_SEQ_YVYU;
		csi_priv->csi_cfg.out_mode = CSI_OUT_MODE_YUV422_TO_YUV420;
		csi_priv->csi_cfg.input_fmt = CSI_IN_FMT_YUV422;
	} else {
		csi_err("sensor mbus_code is no support\n");
	}

}

HAL_Status hal_csi_s_stream(unsigned int on)
{
	csi_private *csi_priv = csi_getpriv();

	if (on) {
		__csi_set_fmt_hw();
		csi_top_enable();
		csi_ini_enable(CSI_INT_STA_FRM_END_PD);
		csi_set_cfg(&csi_priv->csi_cfg);
	} else {
		csi_ini_clear_status(CSI_IRQ_ALL);
		csi_ini_disable(CSI_IRQ_ALL);
		csi_top_disable();
	}

	return 0;
}

HAL_Status hal_csi_set_fmt(struct csi_fmt *csi_output_fmt)
{
	csi_private *csi_priv = csi_getpriv();
	struct sensor_win_size *sensor_win_sizes;

	sensor_win_sizes = csi_priv->sensor_func.sensor_win_sizes;

	if (csi_output_fmt->width > sensor_win_sizes->width || csi_output_fmt->height > sensor_win_sizes->height) {
		csi_err("output size morn than sensor size, sensor:%d*%d, output:%d*%d\n",
					sensor_win_sizes->width, sensor_win_sizes->height,
					csi_output_fmt->width, csi_output_fmt->height);
		csi_output_fmt->width = sensor_win_sizes->width;
		csi_output_fmt->height = sensor_win_sizes->height;
	}

	csi_priv->csi_cfg.hor_len = csi_output_fmt->width;
	csi_priv->csi_cfg.hor_start = sensor_win_sizes->hoffset + (sensor_win_sizes->width - csi_output_fmt->width) / 2;
	csi_priv->csi_cfg.ver_len = csi_output_fmt->height;
	csi_priv->csi_cfg.ver_start = sensor_win_sizes->voffset + (sensor_win_sizes->height - csi_output_fmt->height) / 2;

	csi_dbg("csi output size is %d*%d, h/voffset is %d/%d\n",
					csi_priv->csi_cfg.hor_len, csi_priv->csi_cfg.ver_len,
					csi_priv->csi_cfg.hor_start, csi_priv->csi_cfg.ver_start);
}


int hal_csi_probe(void)
{
	csi_private *csi_priv;

	csi_priv = malloc(sizeof(csi_private));
	if (!csi_priv) {
		csi_err("%s malloc faild\n", __func__);
		return -1;
	} else {
		memset(csi_priv, 0, sizeof(csi_private));
		csi_setpriv(csi_priv);
	}

	csi_priv->lock = hal_mutex_create();
	if (!csi_priv->lock) {
		csi_err("mutex create fail\n");
		goto emalloc;
	}

	csi_pinctrl_config();

#ifdef CONFIG_DRIVER_SYSCONFIG
	if(csi_dev_cfg_parse()) {
		csi_err("csi dev config parse failed\n");
		goto emalloc;
	}

	csi_priv->sensor_cfg.twi_port = csi_dev_cfg.twi_id;
	csi_priv->sensor_cfg.pwcfg.pwdn_pin = csi_dev_cfg.pwdn_pin;
	csi_priv->sensor_cfg.pwcfg.reset_pin = csi_dev_cfg.reset_pin;
#else
	csi_priv->sensor_cfg.twi_port = SENSOR_I2C_ID;
	csi_priv->sensor_cfg.pwcfg.pwdn_pin = SENSOR_POWERDOWN_PIN;
	csi_priv->sensor_cfg.pwcfg.reset_pin = SENSOR_RESET_PIN;
#endif
	csi_priv->sensor_func.init = SENSOR_FUNC_INIT;
	csi_priv->sensor_func.deinit = SENSOR_FUNC_DEINIT;
	csi_priv->sensor_func.ioctl = SENSOR_FUNC_IOCTL;
	csi_priv->sensor_func.g_mbus_config = SENSOR_FUNC_MBUD_CFG;
	csi_priv->sensor_func.sensor_win_sizes = &SENSOR_FUNC_WIN_SIZE;

	return 0;
emalloc:
	free(csi_priv);
	csi_setpriv(NULL);

	return -1;
}

void hal_csi_remove(void)
{
	csi_private *csi_priv = csi_getpriv();
	if (!csi_priv) {
		csi_err("csi_priv not init\n");
		return;
	}

	csi_pinctrl_release();
	hal_mutex_delete(csi_priv->lock);
	free(csi_priv);
	csi_setpriv(NULL);
}

