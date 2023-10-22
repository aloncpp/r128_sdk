/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the people's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.
*
*
* THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
* PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
* WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
* THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
* OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
* IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
* NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
* OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <string.h>
#include <sound/snd_core.h>
#include <hal_gpio.h>
#include <hal_time.h>
#ifdef CONFIG_DRIVER_SYSCONFIG
#include <script.h>
#include <hal_cfg.h>
#endif
#if defined(CONFIG_DRIVERS_GPIO_EX_AW9523)
#include <gpio/aw9523.h>
#endif

#include "snd_sunxi_common.h"

/* for reg labels */
int snd_sunxi_save_reg(struct audio_reg_label *reg_labels, void *data,
		       unsigned int (*snd_read_func)(void *data, unsigned int reg))
{
	int i = 0;

	snd_print("\n");

	while (reg_labels[i].name != NULL) {
		reg_labels[i].value = snd_read_func(data, reg_labels[i].address);
		i++;
	}

	snd_print("save reg end\n");
	return i;
}

int snd_sunxi_echo_reg(struct audio_reg_label *reg_labels, void *data,
		       void (*snd_write_func)(void *data, unsigned int reg, unsigned int val))
{
	int i = 0;

	snd_print("\n");

	while (reg_labels[i].name != NULL) {
		snd_write_func(data, reg_labels[i].address, reg_labels[i].value);
		i++;
	}

	snd_print("echo reg end\n");
	return i;
}

/* for pa config */
int snd_sunxi_pa_enable(struct sunxi_pa_config *pa_cfg)
{
	snd_print("\n");

	int ret;

	hal_msleep(pa_cfg->pa_msleep_time);

	ret = hal_gpio_pinmux_set_function(pa_cfg->gpio, pa_cfg->mul_sel);
	if (ret) {
		snd_err("pin%d set function failed\n", pa_cfg->gpio);
		return -1;
	}

	ret = hal_gpio_set_driving_level(pa_cfg->gpio, pa_cfg->drv_level);
	if (ret) {
		snd_err("pin%d set driving_level failed\n", pa_cfg->gpio);
		return -1;
	}

	ret = hal_gpio_set_data(pa_cfg->gpio, pa_cfg->data);
	if (ret) {
		snd_err("pin%d set data failed\n", pa_cfg->gpio);
		return -1;
	}

	return 0;
}

void snd_sunxi_pa_disable(struct sunxi_pa_config *pa_cfg)
{
	snd_print("\n");

	hal_gpio_pinmux_set_function(pa_cfg->gpio, pa_cfg->mul_sel);
	hal_gpio_set_driving_level(pa_cfg->gpio, pa_cfg->drv_level);
	hal_gpio_set_data(pa_cfg->gpio, !pa_cfg->data);
}

int snd_sunxi_pa_init(struct sunxi_pa_config *pa_cfg,
		      struct sunxi_pa_config *default_pa_cfg,
		      char *secname)
{
#ifdef CONFIG_DRIVER_SYSCONFIG
	user_gpio_set_t gpio_cfg;
	int ret;
	int32_t val;

#endif

	if (!default_pa_cfg) {
		snd_err("No default pa config!\n");
		return -1;
	}

#ifdef CONFIG_DRIVER_SYSCONFIG
	memset(&gpio_cfg, 0, sizeof(gpio_cfg));
	ret = hal_cfg_get_keyvalue(secname, "pa_pin", (int32_t *)&gpio_cfg,
				  sizeof(user_gpio_set_t) >> 2);
	if (ret) {
		snd_print("%s get pa_pin failed, using default config.\n", secname);
		pa_cfg->gpio = default_pa_cfg->gpio;
		pa_cfg->mul_sel = default_pa_cfg->mul_sel;
		pa_cfg->drv_level = default_pa_cfg->drv_level;
		pa_cfg->data = default_pa_cfg->data;
	} else {
#if defined(CONFIG_DRIVERS_GPIO_EX_AW9523)
		if (gpio_cfg.port >= 21) { //PU
			pa_cfg->gpio = aw9523_gpio_get_num(gpio_cfg.port_num);
		} else {
			pa_cfg->gpio = (gpio_cfg.port - 1) * 32 + gpio_cfg.port_num;
		}
#else
		pa_cfg->gpio = (gpio_cfg.port - 1) * 32 + gpio_cfg.port_num;
#endif
		pa_cfg->mul_sel = gpio_cfg.mul_sel;
		pa_cfg->drv_level = gpio_cfg.drv_level;
		pa_cfg->data = gpio_cfg.data;
	}

	ret = hal_cfg_get_keyvalue(secname, "pa_pin_msleep", &val, 1);
	if (ret) {
		snd_print("%s get pa_msleep_time failed, using default config\n", secname);
		pa_cfg->pa_msleep_time = default_pa_cfg->pa_msleep_time;
	} else
		pa_cfg->pa_msleep_time = val;
#else
	*pa_cfg = *default_pa_cfg;
#endif

	return 0;
}
