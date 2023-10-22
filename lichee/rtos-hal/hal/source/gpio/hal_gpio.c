/*
 * Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
 *
 * Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
 * the the People's Republic of China and other countries.
 * All Allwinner Technology Co.,Ltd. trademarks are used with permission.
 *
 * DISCLAIMER
 * THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
 * IF YOU NEED TO INTEGRATE THIRD PARTY'S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
 * IN ALLWINNERS'SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
 * ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
 * ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
 * COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
 * YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY��S TECHNOLOGY.
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
#include <stdio.h>
#include <string.h>
/* #include <init.h> */

#include <hal_mem.h>
#include <hal_gpio.h>
#include <hal_log.h>
#include <hal_cfg.h>
#include "gpio.h"
#include <sunxi_hal_common.h>
#include <interrupt.h>
#ifdef CONFIG_COMPONENTS_PM
#include <pm_wakesrc.h>
#include <pm_syscore.h>
#include <pm_debug.h>
#endif
#ifdef CONFIG_AMP_SHARE_IRQ
#include <openamp/openamp_share_irq.h>
#endif
#ifdef CONFIG_DRIVERS_GPIO_SHARE_IRQ
#include <hal_cpu.h>
#endif
#ifdef CONFIG_DRIVERS_GPIO_EX_AW9523
#include "aw9523.h"
#endif

static const struct gpio_desc **g_gpio_desc = NULL;

/*
 * The following inlines stuffs a configuration parameter and data value
 * into and out of an unsigned long argument, as used by the generic pin config
 * system. We put the parameter in the lower 8 bits and the argument in the
 * upper 24 bits.
 */

static inline pin_config_param_t pinconf_to_config_param(unsigned long config)
{
	return (pin_config_param_t)(config & 0xffUL);
}

static inline gpio_pin_t pinconf_to_config_argument(unsigned long config)
{
	return (uint32_t)((config >> 8) & 0xffffffUL);
}

static inline uint64_t pinconf_to_config_packed(pin_config_param_t param,
		unsigned long argument)
{
	return GPIO_CFG_PACK(param, argument);
}

/*
 * The sunXi PIO registers are organized as is:
 * 0x00 - 0x0c  Muxing values.
 *      8 pins per register, each pin having a 4bits value
 * 0x10     Pin values
 *      32 bits per register, each pin corresponding to one bit
 * 0x14 - 0x18  Drive level
 *      16 pins per register, each pin having a 2bits value
 * 0x1c - 0x20  Pull-Up values
 *      16 pins per register, each pin having a 2bits value
 *
 * This is for the first bank. Each bank will have the same layout,
 * with an offset being a multiple of 0x24.
 *
 * The following functions calculate from the pin number the register
 * and the bit offset that we should access.
 */
static inline uint32_t gpio_mux_reg(gpio_pin_t pin)
{
	pin %= BANK_BOUNDARY;
	uint32_t bank = pin / PINS_PER_BANK;
	uint32_t offset = bank * BANK_MEM_SIZE;
	offset += MUX_REGS_OFFSET;
	offset += pin % PINS_PER_BANK / MUX_PINS_PER_REG * 0x04;
	return round_down(offset, 4);
}

static inline uint32_t gpio_mux_offset(gpio_pin_t pin)
{
	gpio_pin_t pin_num = pin % MUX_PINS_PER_REG;
	return pin_num * MUX_PINS_BITS;
}

static inline uint32_t gpio_data_reg(gpio_pin_t pin)
{
	pin %= BANK_BOUNDARY;
	uint32_t bank = pin / PINS_PER_BANK;
	uint32_t offset = bank * BANK_MEM_SIZE;
	offset += DATA_REGS_OFFSET;
	offset += pin % PINS_PER_BANK / DATA_PINS_PER_REG * 0x04;
	return round_down(offset, 4);
}

static inline uint32_t gpio_data_offset(gpio_pin_t pin)
{
	gpio_pin_t pin_num = pin % DATA_PINS_PER_REG;
	return pin_num * DATA_PINS_BITS;
}

static inline uint32_t gpio_dlevel_reg(gpio_pin_t pin)
{
	pin %= BANK_BOUNDARY;
	uint32_t bank = pin / PINS_PER_BANK;
	uint32_t offset = bank * BANK_MEM_SIZE;
	offset += DLEVEL_REGS_OFFSET;
	offset += pin % PINS_PER_BANK / DLEVEL_PINS_PER_REG * 0x04;
	return round_down(offset, 4);
}

static inline uint32_t gpio_dlevel_offset(gpio_pin_t pin)
{
	gpio_pin_t pin_num = pin % DLEVEL_PINS_PER_REG;
	return pin_num * DLEVEL_PINS_BITS;
}

static inline uint32_t gpio_pull_reg(gpio_pin_t pin)
{
	pin %= BANK_BOUNDARY;
	uint32_t bank = pin / PINS_PER_BANK;
	uint32_t offset = bank * BANK_MEM_SIZE;
	offset += PULL_REGS_OFFSET;
	offset += pin % PINS_PER_BANK / PULL_PINS_PER_REG * 0x04;
	return round_down(offset, 4);
}

static inline uint32_t gpio_pull_offset(gpio_pin_t pin)
{
	gpio_pin_t pin_num = pin % PULL_PINS_PER_REG;
	return pin_num * PULL_PINS_BITS;
}

static inline uint32_t gpio_irq_ctrl_reg_from_bank(u8 bank, unsigned bank_base)
{
	return IRQ_CTRL_REG + (bank_base + bank) * IRQ_MEM_SIZE;
}

static inline uint32_t gpio_irq_ctrl_reg(uint32_t irq, unsigned bank_base)
{
	uint32_t bank = irq / IRQ_PER_BANK;
	return gpio_irq_ctrl_reg_from_bank(bank, bank_base);
}

static inline uint32_t gpio_irq_ctrl_offset(uint32_t irq)
{
	uint32_t offset = irq % IRQ_CTRL_IRQ_PER_REG;
	return offset * IRQ_CTRL_IRQ_BITS;
}

static inline uint32_t gpio_get_pin_base_from_bank(u8 bank, unsigned bank_base)
{
	return (bank_base + bank) * IRQ_MEM_SIZE;
}

static inline uint32_t gpio_irq_status_reg_from_bank(u8 bank, unsigned bank_base)
{
	return IRQ_STATUS_REG + (bank_base + bank) * IRQ_MEM_SIZE;
}

static inline uint32_t gpio_irq_status_reg(uint32_t irq, unsigned bank_base)
{
	uint32_t bank = irq / IRQ_PER_BANK;
	return gpio_irq_status_reg_from_bank(bank, bank_base);
}

static inline uint32_t gpio_irq_debounce_from_bank(u8 bank, unsigned bank_base)
{
	return IRQ_DEBOUNCE_REG + (bank_base + bank) * IRQ_MEM_SIZE;
}

static inline uint32_t gpio_irq_debounce_reg(uint32_t irq, unsigned bank_base)
{
	uint32_t bank = irq / IRQ_PER_BANK;
	return gpio_irq_debounce_from_bank(bank, bank_base);
}

static inline uint32_t gpio_irq_status_offset(uint32_t irq)
{
	uint32_t index = irq % IRQ_STATUS_IRQ_PER_REG;
	return index * IRQ_STATUS_IRQ_BITS;
}

static inline uint32_t gpio_irq_cfg_reg(uint32_t irq, unsigned bank_base)
{
	uint32_t bank = irq / IRQ_PER_BANK;
	uint32_t reg = (irq % IRQ_PER_BANK) / IRQ_CFG_IRQ_PER_REG * 0x04;

	return IRQ_CFG_REG + (bank_base + bank) * IRQ_MEM_SIZE + reg;
}

static inline uint32_t gpio_irq_cfg_offset(uint32_t irq)
{
	uint32_t index = irq % IRQ_CFG_IRQ_PER_REG;
	return index * IRQ_CFG_IRQ_BITS;
}

static int gpio_pconf_reg(gpio_pin_t pin, pin_config_param_t param,
		uint32_t *offset, uint32_t *shift, uint32_t *mask)
{
	switch (param)
	{
		case GPIO_TYPE_DRV:
			*offset = gpio_dlevel_reg(pin);
			*shift = gpio_dlevel_offset(pin);
			*mask = DLEVEL_PINS_MASK;
			break;

		case GPIO_TYPE_PUD:
			*offset = gpio_pull_reg(pin);
			*shift = gpio_pull_offset(pin);
			*mask = PULL_PINS_MASK;
			break;

		case GPIO_TYPE_DAT:
			*offset = gpio_data_reg(pin);
			*shift = gpio_data_offset(pin);
			*mask = DATA_PINS_MASK;
			break;

		case GPIO_TYPE_FUNC:
			*offset = gpio_mux_reg(pin);
			*shift = gpio_mux_offset(pin);
			*mask = MUX_PINS_MASK;
			break;

		default:
			GPIO_ERR("Invalid mux type");
			return -1;
	}
	return 0;
}

#if defined(CONFIG_DRIVERS_GPIO_SHARE_IRQ)
static uint32_t *gpio_banks_irq_map = NULL;

int sysconfig_gpio_map_init(const struct gpio_desc **g_gpio_desc)
{
	int i;
	int ret;
	int banks = 0;
	struct gpio_desc *gpio_desc = NULL;

	for (i = 0; g_gpio_desc[i] != NULL; i++)
	{
		gpio_desc = (struct gpio_desc *)g_gpio_desc[i];
		banks += gpio_desc->banks;
	}

	gpio_banks_irq_map = hal_malloc(banks * sizeof(uint32_t));
	if( !gpio_banks_irq_map ) {
		return -1;
	}
	memset(gpio_banks_irq_map, 0, banks * sizeof(uint32_t));

	for(i = 0; i < banks; i++) {
		int num;
		for(num = 0; num < 2; num++) {
			int32_t config_value = 0;
			uint32_t init_value = 0;
			char name[32] = {0};
			int k = num * 16;
			snprintf(name, sizeof(name) - 1, "gpio_%c_%c", '1' + i, '1' + num);
			ret = hal_cfg_get_keyvalue("gpio_irq_map", name, &config_value, 1);
			if (!ret) {
				int j;
				for(j = 0; j < 32; j += 2) {
					uint8_t id = (config_value >> j) & 0x3;
					if (id == (hal_cpu_get_id() + 1)) {
						init_value |= (1 << k);
					}
					k++;
				}
				*(gpio_banks_irq_map + i) |= init_value;
			}
		}
	}
	return 0;
}

int sysconfig_gpio_map_deinit(void)
{
	hal_free(gpio_banks_irq_map);
	gpio_banks_irq_map = NULL;
	return 0;
}

int sunxi_get_banks_mask(uint32_t hw_irq)
{
	int i;
	int j;
	int pos = 0;
	struct gpio_desc *gpio_desc = NULL;

	for (i = 0; g_gpio_desc[i] != NULL; i++)
	{
		gpio_desc = (struct gpio_desc *)g_gpio_desc[i];
		for(j = 0; j < gpio_desc->irq_arry_size; j++) {
			if (gpio_desc->irq[j] == hw_irq) {
				break;
			}
			pos++;
		}
	}
	return *(gpio_banks_irq_map + pos);
}
#endif



static uint32_t count_gpio_bank_mask(void)
{
	uint32_t max_bank = (uint32_t)GPIO_MAX_BANK;
	uint32_t mask = 0;
	do
	{
		mask |= 1 << (max_bank / PINS_PER_BANK);
		max_bank -= PINS_PER_BANK;
		if (max_bank == 0)
		{
			mask |= 1;
		}
	} while (max_bank);
	return mask;
}

static struct gpio_desc *pin_to_gpio_desc(gpio_pin_t pin)
{
	if (pin < BANK_BOUNDARY) /* CPUX domain */
	{
		return (struct gpio_desc *)g_gpio_desc[0];
	}
	else /* CPUS domain */
	{
		return (struct gpio_desc *)g_gpio_desc[1];
	}
}

static struct gpio_desc *irq_to_gpio_desc(uint32_t irq)
{
	int i, j;
	struct gpio_desc *gpio_desc;
	for (i = 0; g_gpio_desc[i] != NULL; i++)
	{
		gpio_desc = (struct gpio_desc *)g_gpio_desc[i];
		for (j = 0; j < gpio_desc->irq_arry_size; j++)
		{
			if (gpio_desc->irq[j] == irq)
			{
				return gpio_desc;
			}
		}
	}
	GPIO_ERR("gpio to irq error!");
	return NULL;
}

static struct gpio_desc *virq_to_gpio_desc(uint32_t irq)
{
	int i, j;
	struct gpio_desc *gpio_desc;
	for (i = 0; g_gpio_desc[i] != NULL; i++)
	{
		gpio_desc = (struct gpio_desc *)g_gpio_desc[i];
		for (j = 0; j < gpio_desc->irq_banks * IRQ_PER_BANK; j++)
		{
			if (gpio_desc->irq_desc[j].virq == irq)
			{
				return gpio_desc;
			}
		}
	}
	GPIO_ERR("gpio to virq error!");
	return NULL;
}

static void gpio_irq_ack(struct gpio_desc *gpio_desc, int i)
{
	struct gpio_irq_desc *dirq = &gpio_desc->irq_desc[i];
	uint32_t hw_irq = dirq->virq - gpio_desc->virq_offset - GPIO_IRQ_START;
	unsigned bank_base = gpio_desc->irq_bank_base[hw_irq / IRQ_PER_BANK];
	uint32_t reg = gpio_irq_status_reg(hw_irq, bank_base);
	uint32_t status_idx = gpio_irq_status_offset(hw_irq);

	/* clear the pending */
	hal_writel(1 << status_idx, gpio_desc->membase + reg);
}

static hal_irqreturn_t bad_gpio_irq_handle(void *data)
{
	GPIO_INFO("No irq registered handler for this calling !!");
	return 0;
}

static void gpio_irq_set_type(struct gpio_desc *gpio_desc, int irq_num, unsigned long type)
{
	struct gpio_irq_desc *dirq = &gpio_desc->irq_desc[irq_num];
	uint32_t hw_irq = dirq->virq - gpio_desc->virq_offset - GPIO_IRQ_START;
	unsigned bank_base = gpio_desc->irq_bank_base[hw_irq / IRQ_PER_BANK];
	uint32_t reg = gpio_irq_cfg_reg(hw_irq, bank_base);
	uint32_t index = gpio_irq_cfg_offset(hw_irq);
	uint32_t mode, regval;

	switch (type)
	{
		case IRQ_TYPE_EDGE_RISING:
			mode = IRQ_EDGE_RISING;
			break;
		case IRQ_TYPE_EDGE_FALLING:
			mode = IRQ_EDGE_FALLING;
			break;
		case IRQ_TYPE_EDGE_BOTH:
			mode = IRQ_EDGE_BOTH;
			break;
		case IRQ_TYPE_LEVEL_HIGH:
			mode = IRQ_LEVEL_HIGH;
			break;
		case IRQ_TYPE_LEVEL_LOW:
			mode = IRQ_LEVEL_LOW;
			break;
		default:
			mode = IRQ_EDGE_RISING;
			break;
	}
	/*should use spin lock protect here*/
	regval = hal_readl(gpio_desc->membase + reg);
	regval &= ~(IRQ_CFG_IRQ_MASK << index);
	hal_writel(regval | (mode << index), gpio_desc->membase + reg);

	//regval = hal_readl(gpio_desc->membase + reg);
	//GPIO_ERR("gpio_desc->membase + reg: 0x%x\n", regval);

}

static hal_irqreturn_t gpio_irq_handle(void *data)
{
	uint32_t hwirq = *((uint32_t *)data);
	uint32_t bank, reg, val, base_bank;
	struct gpio_desc *gpio_desc = irq_to_gpio_desc(hwirq);

	if (gpio_desc == NULL)
	{
		return 0;
	}

	for (bank = 0; bank < gpio_desc->irq_banks; bank ++)
	{
		if (hwirq == gpio_desc->irq[bank])
		{
			break;
		}
	}

	if (bank == gpio_desc->irq_banks)
	{
		return 0;
	}

	base_bank = gpio_desc->irq_bank_base[bank];
	reg = gpio_irq_status_reg_from_bank(bank, base_bank);
#if defined(CONFIG_AMP_SHARE_IRQ) || defined(CONFIG_DRIVERS_GPIO_SHARE_IRQ)
	uint32_t banks_mask = sunxi_get_banks_mask(gpio_desc->irq[bank]);
	val = hal_readl(gpio_desc->membase + reg) & banks_mask;
#else
	val = hal_readl(gpio_desc->membase + reg);
#endif
	GPIO_INFO("hwirq = %ld, gpio_desc address is 0x%lx.", hwirq, gpio_desc->membase);
	GPIO_INFO("base_bank is %ld, hwirq is %ld, val is %ld.", base_bank, hwirq, val);
	if (val)
	{
		uint32_t irqoffset;
		uint32_t irq_pin;
		int i;
		for (irqoffset = 0; irqoffset < IRQ_PER_BANK; irqoffset++)
		{
			if ((1 << irqoffset) & val)
			{
				break;
			}
		}

		if (irqoffset >= IRQ_PER_BANK)
		{
			GPIO_INFO("return");
			return 0;
		}
		irq_pin = ((base_bank + bank) * IRQ_PER_BANK) + irqoffset + gpio_desc->virq_offset;

		for (i = 0; i < gpio_desc->irq_desc_size; i++)
		{
			if (irq_pin == gpio_desc->irq_desc[i].pin)
			{
				break;
			}
		}
		if (i >= gpio_desc->irq_desc_size)
		{
			return 0;
		}
		gpio_desc->irq_desc[i].handle_irq(gpio_desc->irq_desc[i].data);
		gpio_irq_ack(gpio_desc, i);
	}
	return 0;
}

bool hal_gpio_check_valid(gpio_pin_t pin)
{
	uint32_t bank = pin / PINS_PER_BANK;
	uint32_t mask = count_gpio_bank_mask();
	if (!((1 << bank) & mask))
	{
		return false;
	}
	return true;
}

static int gpio_conf_set(gpio_pin_t pin, unsigned long *gpio_config)
{
	struct gpio_desc *gpio_desc = pin_to_gpio_desc(pin);
	if (gpio_desc == NULL)
	{
		GPIO_ERR("gpio_desc is not inited");
		return -1;
	}
	unsigned long config = (unsigned long)gpio_config;
	uint32_t offset, shift, mask, reg;
	uint32_t arg;
	pin_config_param_t param;
	int ret;

	param = pinconf_to_config_param(config);
	arg = pinconf_to_config_argument(config);

	ret = gpio_pconf_reg(pin, param, &offset, &shift, &mask);
	if (ret < 0)
	{
		GPIO_ERR("can't get reg for pin %u", pin);
		return -1;
	}
	/* fix me: shuold we keep spin_lock to protect here?*/
	reg = hal_readl(gpio_desc->membase + offset);
	reg &= ~(mask << shift);
	hal_writel(reg | arg << shift, gpio_desc->membase + offset);
	return 0;
}

static int gpio_conf_get(gpio_pin_t pin, unsigned long *gpio_config)
{
	struct gpio_desc *gpio_desc = pin_to_gpio_desc(pin);
	if (gpio_desc == NULL)
	{
		GPIO_ERR("gpio_desc is not inited");
		return -1;
	}
	uint32_t offset, shift, mask;
	uint32_t arg, val;
	pin_config_param_t param = pinconf_to_config_param(*gpio_config);
	int ret = 0;

	ret = gpio_pconf_reg(pin, param, &offset, &shift, &mask);
	if (ret < 0)
	{
		GPIO_ERR("can't get reg for pin %u", pin);
		return -1;
	}

	val = (hal_readl(gpio_desc->membase + offset) >> shift) & mask;
	switch (param)
	{
		case GPIO_TYPE_DRV:
		case GPIO_TYPE_DAT:
		case GPIO_TYPE_PUD:
		case GPIO_TYPE_FUNC:
			arg = val;
			break;
		default:
			ret = -1;
			GPIO_ERR("Invalid mux type");
			return -1;
	}
	if (!ret)
	{
		*gpio_config = pinconf_to_config_packed(param, arg);
	}
	return ret;
}

int hal_gpio_get_data(gpio_pin_t pin, gpio_data_t *data)
{
	unsigned long config;
	int ret = 0;

#ifdef CONFIG_DRIVERS_GPIO_EX_AW9523
	if(pin >= EXA_BASE) {
		return aw9523_gpio_get_data(pin, data);
	} else {
#endif
	if (NULL == data)
	{
		ret = -1;
		GPIO_ERR("Invalid parameter!");
		return ret;
	}

	config = GPIO_CFG_PACK(GPIO_TYPE_DAT, 0xffffff);
	ret = gpio_conf_get(pin, &config);
	if (ret < 0)
	{
		GPIO_ERR("get conf error!");
		return ret;
	}

	*data = GPIO_CFG_UNPACK_VALUE(config);

	return ret;
#ifdef CONFIG_DRIVERS_GPIO_EX_AW9523
	}
#endif
}

int hal_gpio_set_data(gpio_pin_t pin, gpio_data_t data)
{
	unsigned long config;
	int ret = 0;
#ifdef CONFIG_DRIVERS_GPIO_EX_AW9523
	if(pin >= EXA_BASE) {
		return aw9523_gpio_set_data(pin, data);
	} else {
#endif
	config = GPIO_CFG_PACK(GPIO_TYPE_DAT, data);
	ret = gpio_conf_set(pin, (unsigned long *)config);
	if (ret < 0)
	{
		GPIO_ERR("set conf error!");
		return ret;
	}
	return ret;
#ifdef CONFIG_DRIVERS_GPIO_EX_AW9523
	}
#endif
}

int hal_gpio_set_direction(gpio_pin_t pin, gpio_direction_t direction)
{
	unsigned long config;
	int ret = 0;

#ifdef CONFIG_DRIVERS_GPIO_EX_AW9523
	if(pin >= EXA_BASE) {
		return aw9523_gpio_set_direction(pin, direction);
	} else {
#endif
	config = GPIO_CFG_PACK(GPIO_TYPE_FUNC, direction);
	ret = gpio_conf_set(pin, (unsigned long *)config);
	if (ret < 0)
	{
		GPIO_ERR("set conf error!");
		return ret;
	}
	return ret;
#ifdef CONFIG_DRIVERS_GPIO_EX_AW9523
	}
#endif
}

int hal_gpio_get_direction(gpio_pin_t pin, gpio_direction_t *direction)
{
	unsigned long config;
	int ret = 0;

#ifdef CONFIG_DRIVERS_GPIO_EX_AW9523
	if(pin >= EXA_BASE) {
		return aw9523_gpio_get_direction(pin, direction);
	} else {
#endif
	if (NULL == direction)
	{
		ret = -1;
		GPIO_ERR("Invalid parameter!");
		return ret;
	}
	config = GPIO_CFG_PACK(GPIO_TYPE_FUNC, 0xffffff);
	ret = gpio_conf_get(pin, &config);
	if (ret < 0)
	{
		GPIO_ERR("get conf error!");
		return ret;
	}

	*direction = GPIO_CFG_UNPACK_VALUE(config);

	return ret;
#ifdef CONFIG_DRIVERS_GPIO_EX_AW9523
	}
#endif
}

int hal_gpio_set_pull(gpio_pin_t pin, gpio_pull_status_t pull)
{
	unsigned long config;
	int ret = 0;

#ifdef CONFIG_DRIVERS_GPIO_EX_AW9523
	if(pin >= EXA_BASE) {
		return 0;
	}
#endif

	config = GPIO_CFG_PACK(GPIO_TYPE_PUD, pull);
	ret = gpio_conf_set(pin, (unsigned long *)config);
	if (ret < 0)
	{
		GPIO_ERR("set conf error!");
		return ret;
	}
	return ret;
}

int hal_gpio_get_pull(gpio_pin_t pin, gpio_pull_status_t *pull)
{
	unsigned long config;
	int ret = 0;

	if (NULL == pull)
	{
		ret = -1;
		GPIO_ERR("Invalid parameter!");
		return ret;
	}

#ifdef CONFIG_DRIVERS_GPIO_EX_AW9523
	if(pin >= EXA_BASE) {
		*pull = GPIO_PULL_DOWN_DISABLED;
		return 0;
	}
#endif

	config = GPIO_CFG_PACK(GPIO_TYPE_PUD, 0xffffff);
	ret = gpio_conf_get(pin, &config);
	if (ret < 0)
	{
		GPIO_ERR("get conf error!");
		return ret;
	}

	*pull = GPIO_CFG_UNPACK_VALUE(config);

	return ret;

}

int hal_gpio_set_driving_level(gpio_pin_t pin, gpio_driving_level_t level)
{
	unsigned long config;
	int ret = 0;

#ifdef CONFIG_DRIVERS_GPIO_EX_AW9523
	if(pin >= EXA_BASE) {
		return 0;
	}
#endif

	config = GPIO_CFG_PACK(GPIO_TYPE_DRV, level);
	ret = gpio_conf_set(pin, (unsigned long *)config);
	if (ret < 0)
	{
		GPIO_ERR("set conf error!");
		return ret;
	}
	return ret;
}

int hal_gpio_get_driving_level(gpio_pin_t pin, gpio_driving_level_t *level)
{
	unsigned long config;
	int ret = 0;

	if (NULL == level)
	{
		ret = -1;
		GPIO_ERR("Invalid parameter!");
		return ret;
	}

#ifdef CONFIG_DRIVERS_GPIO_EX_AW9523
	if(pin >= EXA_BASE) {
		*level = GPIO_DRIVING_LEVEL3;
		return 0;
	}
#endif

	config = GPIO_CFG_PACK(GPIO_TYPE_DRV, 0xffffff);
	ret = gpio_conf_get(pin, &config);
	if (ret < 0)
	{
		GPIO_ERR("get conf error!");
		return ret;
	}

	*level = GPIO_CFG_UNPACK_VALUE(config);

	return ret;
}

int hal_gpio_pinmux_set_function(gpio_pin_t pin, gpio_muxsel_t function_index)
{
	unsigned long config;
	int ret = 0;

#ifdef CONFIG_DRIVERS_GPIO_EX_AW9523
	if(pin >= EXA_BASE) {
		if (function_index == GPIO_MUXSEL_OUT) {
			ret = aw9523_gpio_set_direction(pin, GPIO_DIRECTION_OUTPUT);
		} else if (function_index == GPIO_MUXSEL_IN) {
			ret = aw9523_gpio_set_direction(pin, GPIO_DIRECTION_INPUT);
		} else {
			GPIO_ERR("set ex pin mux error!");
			ret = -1;
		}
		return ret;
	}
#endif

	config = GPIO_CFG_PACK(GPIO_TYPE_FUNC, function_index);
	ret = gpio_conf_set(pin, (unsigned long *)config);
	if (ret < 0)
	{
		GPIO_ERR("set pin mux error!");
		return ret;
	}
	return ret;
}

int hal_gpio_pinmux_get_function(gpio_pin_t pin, gpio_muxsel_t *function_index)
{
	unsigned long config;
	int ret = 0;

	if (NULL == function_index)
	{
		ret = -1;
		GPIO_ERR("Invalid parameter!");
		return ret;
	}

#ifdef CONFIG_DRIVERS_GPIO_EX_AW9523
	if(pin >= EXA_BASE) {
		gpio_direction_t dir = GPIO_DIRECTION_OUTPUT;

		if (ret = aw9523_gpio_get_direction(pin, &dir)) {
			GPIO_ERR("get ex pin mux error!");
			return ret;
		}

		*function_index = (dir == GPIO_DIRECTION_OUTPUT) ? GPIO_MUXSEL_OUT : GPIO_MUXSEL_IN;
		return 0;
	}
#endif

	config = GPIO_CFG_PACK(GPIO_TYPE_FUNC, 0xffffff);
	ret = gpio_conf_get(pin, &config);
	if (ret < 0)
	{
		GPIO_ERR("get conf error!");
		return ret;
	}

	*function_index = GPIO_CFG_UNPACK_VALUE(config);

	return ret;
}

/*
 * 1.8 -> 3.3:
 *	1. Increase withstand
 *	2. Increase power voltage
 * 3.3 -> 1.8:
 *	1. Decrease power voltage
 *	2. Decrease withstand
 */
static void hal_gpio_power_switch_pf(struct gpio_desc *gpio_desc, u32 tar_vol, u32 pow_sel)
{
	u32 cur_vol;

	cur_vol = hal_readl(gpio_desc->membase + POWER_VOL_SEL) & BIT(0);
	tar_vol &= BIT(0);

	if (cur_vol < tar_vol) {
		hal_writel(pow_sel, gpio_desc->membase + POWER_MODE_SEL);
		hal_writel(tar_vol, gpio_desc->membase + POWER_VOL_SEL);
	} else if (cur_vol > tar_vol) {
		hal_writel(tar_vol, gpio_desc->membase + POWER_VOL_SEL);
		hal_writel(pow_sel, gpio_desc->membase + POWER_MODE_SEL);
	} else {
		hal_writel(pow_sel, gpio_desc->membase + POWER_MODE_SEL);
	}
}

int hal_gpio_sel_vol_mode(gpio_pin_t pin, gpio_power_mode_t  pm_sel)
{
	uint32_t bank, temp;
	struct gpio_desc *gpio_desc;

	gpio_desc = pin_to_gpio_desc(pin);

	if (gpio_desc == NULL)
	{
		return -1;
	}

	bank = (pin - gpio_desc->pin_base) / PINS_PER_BANK;
	temp = hal_readl(gpio_desc->membase + POWER_MODE_SEL);
	temp &= ~(1 << bank);
	temp |= (pm_sel << bank);

	if (bank == 5)
		hal_gpio_power_switch_pf(gpio_desc, ~pm_sel, temp);
	else
		hal_writel(temp, gpio_desc->membase + POWER_MODE_SEL);

	return 0;
}

int hal_gpio_set_debounce(gpio_pin_t pin, unsigned value)
{
	uint32_t irq, hw_irq, reg, reg_val;
	struct gpio_desc *gpio_desc;
	unsigned bank_base;
	unsigned int val_clk_select, val_clk_per_scale;
	int ret = 0;

	gpio_desc = pin_to_gpio_desc(pin);

	if (gpio_desc == NULL)
	{
		return -1;
	}

	ret = hal_gpio_to_irq(pin, &irq);

	if (ret < 0)
	{
		GPIO_ERR("gpio to irq error");
		return -1;
	}

	hw_irq = irq - gpio_desc->virq_offset - GPIO_IRQ_START;
	bank_base = gpio_desc->irq_bank_base[hw_irq / IRQ_PER_BANK];
	reg = gpio_irq_debounce_reg(hw_irq, bank_base);

	reg_val = hal_readl(gpio_desc->membase + reg);
	val_clk_select = value & 1;
	val_clk_per_scale = (value >> 4) & 0x07;

	/*set debounce pio interrupt clock select */
	reg_val &= ~(1 << 0);
	reg_val |= val_clk_select;

	/* set debounce clock pre scale */
	reg_val &= ~(7 << 4);
	reg_val |= val_clk_per_scale << 4;
	hal_writel(reg_val, gpio_desc->membase + reg);

	return 0;
}


int hal_gpio_to_irq(gpio_pin_t pin, uint32_t *irq)
{
	int i = 0;
	struct gpio_desc *gpio_desc = pin_to_gpio_desc(pin);

	for (i = 0; i < gpio_desc->irq_banks * IRQ_PER_BANK; i++)
	{
		if (pin != gpio_desc->irq_desc[i].pin)
		{
			continue;
		}
		GPIO_INFO("gpio %lu to irq %lu succeed!", pin, gpio_desc->irq_desc[i].virq);
		*irq = gpio_desc->irq_desc[i].virq;
		return 0;
	}

	return -1;
}


int hal_gpio_irq_request(uint32_t irq, hal_irq_handler_t hdle, unsigned long flags, void *data)
{
	struct gpio_desc *gpio_desc = virq_to_gpio_desc(irq);
	GPIO_INFO("[%s]gpio_desc address is 0x%lx.", __func__, gpio_desc->membase);
	int irq_max_num = gpio_desc->irq_desc_size + GPIO_IRQ_START;
	int ret = 0;
#if defined(CONFIG_AMP_SHARE_IRQ) || defined(CONFIG_DRIVERS_GPIO_SHARE_IRQ)
	uint32_t hw_irq = irq - gpio_desc->virq_offset - GPIO_IRQ_START;
	uint32_t bank_mask;
#endif
	irq -= gpio_desc->virq_offset;

#if defined(CONFIG_AMP_SHARE_IRQ) || defined(CONFIG_DRIVERS_GPIO_SHARE_IRQ)
	bank_mask = sunxi_get_banks_mask(gpio_desc->irq[hw_irq / IRQ_PER_BANK]);
	bank_mask &= (1 << hw_irq);
	if (!bank_mask) {
		GPIO_ERR("irq%d not belong to this chip,hwirq %d, mask=0x%x\n", irq,
						gpio_desc->irq[hw_irq / IRQ_PER_BANK],
						(uint32_t)sunxi_get_banks_mask(gpio_desc->irq[hw_irq / IRQ_PER_BANK]));
		return -1;
	}
#endif
	if (irq >= GPIO_IRQ_START && irq < irq_max_num)
	{
		if (hdle && gpio_desc->irq_desc[irq - GPIO_IRQ_START].handle_irq == bad_gpio_irq_handle)
		{
			gpio_desc->irq_desc[irq - GPIO_IRQ_START].handle_irq = hdle;
			gpio_desc->irq_desc[irq - GPIO_IRQ_START].flags = flags;
			gpio_desc->irq_desc[irq - GPIO_IRQ_START].data = data;
		}
		/*set irq tpye*/
		gpio_irq_set_type(gpio_desc, irq - GPIO_IRQ_START, flags);

		/*set pin mux*/
		ret = hal_gpio_pinmux_set_function(gpio_desc->irq_desc[irq - GPIO_IRQ_START].pin, GPIO_MUXSEL_EINT);

		if (ret < 0)
		{
			GPIO_ERR("set pin mux error!");
			return -1;
		}
		GPIO_INFO("request irq %lu succeed!", irq);
		return irq;
	}

	GPIO_ERR("Wrong irq NO.(%u) to request !!", (unsigned int)irq);
	return -1;
}

int hal_gpio_irq_free(uint32_t irq)
{
	struct gpio_desc *gpio_desc = virq_to_gpio_desc(irq);
	int irq_max_num = gpio_desc->irq_desc_size + GPIO_IRQ_START;
	irq -= gpio_desc->virq_offset;
	if (irq >= GPIO_IRQ_START && irq < irq_max_num)
	{
		gpio_desc->irq_desc[irq - GPIO_IRQ_START].handle_irq = bad_gpio_irq_handle;
		gpio_desc->irq_desc[irq - GPIO_IRQ_START].flags = 0;
		gpio_desc->irq_desc[irq - GPIO_IRQ_START].data = NULL;
		GPIO_INFO("free irq %lu succeed!", irq);
		return irq;
	}

	GPIO_ERR("Wrong irq NO.(%u) to free !!", (unsigned int)irq);
	return -1;
}

int hal_gpio_irq_enable(uint32_t irq)
{
	struct gpio_desc *gpio_desc = virq_to_gpio_desc(irq);
	GPIO_INFO("[%s]gpio_desc address is 0x%lx.", __func__, gpio_desc->membase);
	int irq_max_num = gpio_desc->irq_desc_size + GPIO_IRQ_START;
	uint32_t hw_irq = irq - gpio_desc->virq_offset - GPIO_IRQ_START;
	unsigned bank_base = gpio_desc->irq_bank_base[hw_irq / IRQ_PER_BANK];
	uint32_t reg = gpio_irq_ctrl_reg(hw_irq, bank_base);
	uint32_t index = gpio_irq_ctrl_offset(hw_irq);
	uint32_t val = 0;

	irq -= gpio_desc->virq_offset;

	if (irq < GPIO_IRQ_START || irq >= irq_max_num)
	{
		GPIO_ERR("Wrong irq NO.(%u) to enable !!", (unsigned int)irq);
		return -1;
	}

	/*clear pending*/
	gpio_irq_ack(gpio_desc, hw_irq);

	/*unmask the irq,should keep spin lock to protect*/
	val = hal_readl(gpio_desc->membase + reg);
	hal_writel(val | (1 << index), gpio_desc->membase + reg);
	return 0;
}
#ifdef CONFIG_STANDBY
struct gpio_pm_reg_cache gpio_pm_reg;

static int gpio_pm_alloc_mem(uint32_t desc_index, uint32_t mem_size)
{
	if (desc_index > 1)
	{
		GPIO_ERR("index[%d] exceed desc_index range!", desc_index);
		return -1;
	}

	gpio_pm_reg.reg_dump[desc_index] = hal_malloc(mem_size);
	if (gpio_pm_reg.reg_dump[desc_index] == NULL)
	{
		GPIO_ERR("malloc reg_mem[%d] error!", desc_index);
		return -1;
	}

	gpio_pm_reg.reg_dump_size[desc_index] = mem_size;

	return 0;
}

int hal_gpio_suspend()
{
	int i;
	void *mem = NULL;
	uint32_t mem_size;
	uint32_t flags;
	struct gpio_desc *gpio_desc = NULL;

	GPIO_INFO("gpio suspend\n");

	flags = hal_interrupt_disable_irqsave();
	for (i = 0; g_gpio_desc[i] != NULL; i++) {
		gpio_desc = (struct gpio_desc *)g_gpio_desc[i];
		mem = gpio_pm_reg.reg_dump[i];
		mem_size = gpio_pm_reg.reg_dump_size[i];
		if (mem != NULL)
			memcpy(mem, (uint32_t *)gpio_desc->membase, mem_size);
	}
	hal_interrupt_enable_irqrestore(flags);

	return 0;
}

int hal_gpio_resume()
{
	int i;
	void *mem = NULL;
	uint32_t mem_size;
	uint32_t flags;
	struct gpio_desc *gpio_desc = NULL;

	flags = hal_interrupt_disable_irqsave();
	for (i = 0; g_gpio_desc[i] != NULL; i++) {
		gpio_desc = (struct gpio_desc *)g_gpio_desc[i];
		mem = gpio_pm_reg.reg_dump[i];
		mem_size = gpio_pm_reg.reg_dump_size[i];
		if (gpio_pm_reg.reg_dump[i] != NULL)
			memcpy((uint32_t *)gpio_desc->membase, mem, mem_size);
	}
	hal_interrupt_enable_irqrestore(flags);

	GPIO_INFO("gpio resume");

	return 0;
}
#endif
#ifdef CONFIG_COMPONENTS_PM
struct gpio_pm_reg_cache gpio_pm_reg;

static int gpio_pm_alloc_mem(uint32_t desc_index, uint32_t mem_size)
{
	if (desc_index > 1)
	{
		GPIO_ERR("index[%d] exceed desc_index range!", desc_index);
		return -1;
	}

	gpio_pm_reg.reg_dump[desc_index] = hal_malloc(mem_size);
	if (gpio_pm_reg.reg_dump[desc_index] == NULL)
	{
		GPIO_ERR("malloc reg_mem[%d] error!", desc_index);
		return -1;
	}

	gpio_pm_reg.reg_dump_size[desc_index] = mem_size;

	return 0;
}

static int hal_gpio_suspend(void *data, suspend_mode_t mode)
{
	int i;
	int j;
	uint32_t *mem = NULL;
	uint32_t mem_size;
	uint32_t flags;
	struct gpio_desc *gpio_desc = NULL;

	flags = hal_interrupt_disable_irqsave();
	for (i = 0; g_gpio_desc[i] != NULL; i++) {
		gpio_desc = (struct gpio_desc *)g_gpio_desc[i];
		mem = gpio_pm_reg.reg_dump[i];
		mem_size = gpio_pm_reg.reg_dump_size[i];
		if (mem != NULL) {
			for(j = 0; j < (mem_size >> 2); j++) {
				mem[j] = *(volatile unsigned int *)((unsigned long)gpio_desc->membase + j * 4);
			}
		}
	}
	hal_interrupt_enable_irqrestore(flags);

	pm_inf("gpio suspend end\n");
	return 0;
}

static void hal_gpio_resume(void *data, suspend_mode_t mode)
{
	int i;
	int j;
	uint32_t *mem = NULL;
	uint32_t mem_size;
	uint32_t flags;
	struct gpio_desc *gpio_desc = NULL;

	flags = hal_interrupt_disable_irqsave();
	for (i = 0; g_gpio_desc[i] != NULL; i++) {
		gpio_desc = (struct gpio_desc *)g_gpio_desc[i];
		mem = gpio_pm_reg.reg_dump[i];
		mem_size = gpio_pm_reg.reg_dump_size[i];
		if (gpio_pm_reg.reg_dump[i] != NULL) {
			for(j = 0; j < (mem_size >> 2); j++) {
				*(volatile unsigned int *)((unsigned long)gpio_desc->membase + j * 4) = mem[j];
			}
		}
	}
	hal_interrupt_enable_irqrestore(flags);

	pm_inf("gpio resume end\n");
}

static struct syscore_ops gpio_syscore_ops = {
	.name = "gpio_syscore_ops",
	.suspend = hal_gpio_suspend,
	.resume = hal_gpio_resume,
	.common_syscore = COMMON_SYSCORE,
};
#endif

int hal_gpio_irq_disable(uint32_t irq)
{
	struct gpio_desc *gpio_desc = virq_to_gpio_desc(irq);
	GPIO_INFO("[%s]gpio_desc address is 0x%lx.", __func__, gpio_desc->membase);
	int irq_max_num = gpio_desc->irq_desc_size + GPIO_IRQ_START;
	uint32_t hw_irq = irq - gpio_desc->virq_offset - GPIO_IRQ_START;
	unsigned bank_base = gpio_desc->irq_bank_base[hw_irq / IRQ_PER_BANK];
	uint32_t reg = gpio_irq_ctrl_reg(hw_irq, bank_base);
	uint32_t index = gpio_irq_ctrl_offset(hw_irq);
	uint32_t val = 0;
	irq -= gpio_desc->virq_offset;
	if (irq < GPIO_IRQ_START || irq >= irq_max_num)
	{
		GPIO_ERR("Wrong irq NO.(%u) to enable !!", (unsigned int)irq);
		return -1;
	}

	/*mask the irq,should keep spin lock to protect*/
	val = hal_readl(gpio_desc->membase + reg);
	hal_writel(val & ~(1 << index), gpio_desc->membase + reg);
	return 0;
}

int hal_gpio_init(void)
{
	int i, j, ret;
	struct gpio_desc *gpio_desc = NULL;
	struct gpio_irq_desc *irq_desc = NULL;
	int irq_desc_array_size = 0;
	char irqname[32] = {0};

	/* initialize g_gpio_desc */
	g_gpio_desc = gpio_get_platform_desc();
	if (g_gpio_desc == NULL)
	{
		GPIO_ERR("initialize global platform desc failed!");
		return -1;
	}

#if defined(CONFIG_DRIVERS_GPIO_SHARE_IRQ)
	sysconfig_gpio_map_init(g_gpio_desc);
#endif

	for (j = 0; g_gpio_desc[j] != NULL; j++)
	{
		gpio_desc = (struct gpio_desc *)g_gpio_desc[j];
		irq_desc_array_size = gpio_desc->irq_banks * IRQ_PER_BANK;
		gpio_desc->irq_desc_size = irq_desc_array_size;

#ifdef CONFIG_STANDBY
		ret = gpio_pm_alloc_mem(j, gpio_desc->resource_size);
		if (ret)
		{
			GPIO_ERR("gpio[%d] pm alloc mem err!", j);
			return ret;
		}
#endif

		irq_desc = (struct gpio_irq_desc *)hal_malloc(irq_desc_array_size * sizeof(struct gpio_irq_desc));
		if (irq_desc == NULL)
		{
			GPIO_ERR("alloc memory failed!");
			return -1;
		}

		memset(irq_desc, 0, irq_desc_array_size * sizeof(struct gpio_irq_desc));
		for (i = 0; i < irq_desc_array_size; i++)
		{
			unsigned int n = i / IRQ_PER_BANK;
			unsigned int k = i % IRQ_PER_BANK;
			unsigned bank_base = gpio_desc->irq_bank_base[n];
			irq_desc[i].pin = gpio_get_pin_base_from_bank(n, bank_base) + gpio_desc->virq_offset + k;
			irq_desc[i].virq =  GPIO_IRQ_START + gpio_desc->virq_offset + i;
			irq_desc[i].handle_irq = bad_gpio_irq_handle;
		}

		gpio_desc->irq_desc = irq_desc;

		for (i = 0; i < gpio_desc->irq_banks; i++)
		{
			/* mask all irq */
			unsigned bank_base = gpio_desc->irq_bank_base[i];
#if defined(CONFIG_AMP_SHARE_IRQ) || defined(CONFIG_DRIVERS_GPIO_SHARE_IRQ)
			uint32_t bank_mask, ctrl_val, sta_val;
			unsigned long ctrl_reg, sta_reg;

			ctrl_reg = gpio_desc->membase + gpio_irq_ctrl_reg_from_bank(i, bank_base);
			sta_reg = gpio_desc->membase + gpio_irq_status_reg_from_bank(i, bank_base);

			/* get gpio bank mask from share interrupt table */
			bank_mask = sunxi_get_banks_mask(gpio_desc->irq[i]);
			ctrl_val = hal_readl(ctrl_reg) & (~bank_mask);
			sta_val = 0xffffffff & bank_mask;

			hal_writel(ctrl_val, ctrl_reg);
			hal_writel(sta_val, sta_reg);
#else
			hal_writel(0, gpio_desc->membase +
					gpio_irq_ctrl_reg_from_bank(i, bank_base));
			/* clear pending flags */
			hal_writel(0xffffffff, gpio_desc->membase +
					gpio_irq_status_reg_from_bank(i, bank_base));
#endif
		}

		/* request irq */
		for (i = 0; i < gpio_desc->irq_arry_size; i++)
		{
#if defined(CONFIG_AMP_SHARE_IRQ) || defined(CONFIG_DRIVERS_GPIO_SHARE_IRQ)
			uint32_t bank_mask = sunxi_get_banks_mask(gpio_desc->irq[i]);
			if (!bank_mask)
				continue;
#endif
			snprintf(irqname, 32, "gpio-ctl%d%d", j, i);
			ret = hal_request_irq(gpio_desc->irq[i], gpio_irq_handle, irqname, (void *)&gpio_desc->irq[i]);
		}

		/* enable irq */
		for (i = 0; i < gpio_desc->irq_arry_size; i++)
		{
#if defined(CONFIG_AMP_SHARE_IRQ) || defined(CONFIG_DRIVERS_GPIO_SHARE_IRQ)
			uint32_t bank_mask = sunxi_get_banks_mask(gpio_desc->irq[i]);
			if (!bank_mask)
				continue;
#endif
			hal_enable_irq(gpio_desc->irq[i]);
		}

#ifdef CONFIG_COMPONENTS_PM
		ret = gpio_pm_alloc_mem(j, gpio_desc->resource_size);
		if (ret)
		{
			GPIO_ERR("gpio[%d] pm alloc mem err!", j);
			return ret;
		}
		pm_syscore_register(&gpio_syscore_ops);
#ifdef CONFIG_PM_WAKESRC_GPIO
		for (i = 0; i < gpio_desc->irq_arry_size; i++) {
			if (pm_wakesrc_register(gpio_desc->irq[i], "GPIO", PM_WAKESRC_MIGHT_WAKEUP))
				GPIO_ERR("irq %d register wakesrc failed", gpio_desc->irq[i]);
			if (pm_set_wakeirq(gpio_desc->irq[i]))
				GPIO_ERR("irq %d enable wakesrc failed", gpio_desc->irq[i]);
		}
#endif
#endif
	}
	GPIO_INFO("gpio init success!");
	return 0;
}

int hal_gpio_r_irq_disable(uint32_t irq)
{
	int i = 0;
	struct gpio_desc *gpio_desc = irq_to_gpio_desc(irq);
	if (gpio_desc == NULL)
	{
		GPIO_ERR("initialize global platform desc failed!");
		return -1;
	}

	for (i = 0; i < gpio_desc->irq_arry_size; i++)
	{
		if (gpio_desc->irq[i] == irq)
		{
			hal_disable_irq(gpio_desc->irq[i]);
		}
	}
	return 0;
}

int hal_gpio_r_irq_enable(uint32_t irq)
{
	int i = 0;
	struct gpio_desc *gpio_desc = irq_to_gpio_desc(irq);
	if (gpio_desc == NULL)
	{
		GPIO_ERR("initialize global platform desc failed!");
		return -1;
	}

	for (i = 0; i < gpio_desc->irq_arry_size; i++)
	{
		if (gpio_desc->irq[i] == irq)
		{
			hal_enable_irq(gpio_desc->irq[i]);
		}
	}
	return 0;
}

int hal_gpio_r_all_irq_disable(void)
{
	int i = 0, j = 0;
	struct gpio_desc *gpio_desc = NULL;
	for (j = 0; g_gpio_desc[j] != NULL; j++)
	{
		gpio_desc = (struct gpio_desc *)g_gpio_desc[j];
		for (i = 0; i < gpio_desc->irq_arry_size; i++)
		{
			hal_disable_irq(gpio_desc->irq[i]);
		}
	}
	return 0;
}

