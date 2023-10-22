/**
 * drivers/usb/host/sunxi_hci.c
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * yangnaitian, 2011-5-24, create this file
 * javen, 2011-7-18, add clock and power switch
 *
 * sunxi HCI Driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include "usb_os_platform.h"
#include "sunxi-hci.h"
#include "usb_phy.h"

//static u64 sunxi_hci_dmamask = DMA_BIT_MASK(64);
#ifdef CONFIG_USB_SUNXI_USB_MANAGER
int usb_otg_id_status(void);
#endif

static void USBC_SelectPhyToHci(struct sunxi_hci_hcd *sunxi_hci)
{
	int reg_value = 0;

	reg_value = USBC_Readl(sunxi_hci->otg_vbase + SUNXI_OTG_PHY_CFG);
	reg_value &= ~(0x01);
	USBC_Writel(reg_value, (sunxi_hci->otg_vbase + SUNXI_OTG_PHY_CFG));
}

#if defined(CONFIG_ARCH_SUN20IW2)
static void USBC_Enable_Bias(void)
{
	int reg_value = 0;

	reg_value = USBC_Readl(SUNXI_GPRCM_BASE + USB_BIAS_CTRL);
	reg_value |= USB_BIAS_CTRL_EN;
	USBC_Writel(reg_value, SUNXI_GPRCM_BASE + USB_BIAS_CTRL);
}
#endif

static void USBC_Clean_SIDDP(struct sunxi_hci_hcd *sunxi_hci)
{
	int reg_value = 0;

	reg_value = USBC_Readl(sunxi_hci->usb_vbase + SUNXI_HCI_PHY_CTRL);
	reg_value &= ~(0x01 << SUNXI_HCI_PHY_CTRL_SIDDQ);
	USBC_Writel(reg_value, (sunxi_hci->usb_vbase + SUNXI_HCI_PHY_CTRL));
}

/*
 * Low-power mode USB standby helper functions.
 */
#ifdef SUNXI_USB_STANDBY_LOW_POW_MODE
void sunxi_hci_set_siddq(struct sunxi_hci_hcd *sunxi_hci, int is_on)
{
	int reg_value = 0;

	reg_value = USBC_Readl(sunxi_hci->usb_vbase + SUNXI_HCI_PHY_CTRL);

	if (is_on)
		reg_value |= 0x01 << SUNXI_HCI_PHY_CTRL_SIDDQ;
	else
		reg_value &= ~(0x01 << SUNXI_HCI_PHY_CTRL_SIDDQ);

	USBC_Writel(reg_value, (sunxi_hci->usb_vbase + SUNXI_HCI_PHY_CTRL));
}

void sunxi_hci_set_wakeup_ctrl(struct sunxi_hci_hcd *sunxi_hci, int is_on)
{
	int reg_value = 0;

	reg_value = USBC_Readl(sunxi_hci->usb_vbase + SUNXI_HCI_CTRL_3);

	if (is_on)
		reg_value |= 0x01 << SUNXI_HCI_CTRL_3_REMOTE_WAKEUP;
	else
		reg_value &= ~(0x01 << SUNXI_HCI_CTRL_3_REMOTE_WAKEUP);

	USBC_Writel(reg_value, (sunxi_hci->usb_vbase + SUNXI_HCI_CTRL_3));
}

void sunxi_hci_set_rc_clk(struct sunxi_hci_hcd *sunxi_hci, int is_on)
{
	int reg_value = 0;

	reg_value = USBC_Readl(sunxi_hci->usb_vbase + SUNXI_USB_PMU_IRQ_ENABLE);

	if (is_on)
		reg_value |= 0x01 << SUNXI_HCI_RC16M_CLK_ENBALE;
	else
		reg_value &= ~(0x01 << SUNXI_HCI_RC16M_CLK_ENBALE);

	USBC_Writel(reg_value, (sunxi_hci->usb_vbase + SUNXI_USB_PMU_IRQ_ENABLE));
}

#if defined(CONFIG_ARCH_SUN50IW9)
void sunxi_hci_set_standby_irq(struct sunxi_hci_hcd *sunxi_hci, int is_on)
{
	int reg_value = 0;

	reg_value = USBC_Readl(sunxi_hci->usb_vbase + SUNXI_USB_EHCI_TIME_INT);

	if (is_on)
		reg_value |= 0x01 << SUNXI_USB_EHCI_STANDBY_IRQ;
	else
		reg_value &= ~(0x01 << SUNXI_USB_EHCI_STANDBY_IRQ);

	USBC_Writel(reg_value, (sunxi_hci->usb_vbase + SUNXI_USB_EHCI_TIME_INT));
}
#endif

void sunxi_hci_clean_standby_irq(struct sunxi_hci_hcd *sunxi_hci)
{
	int reg_value = 0;

	reg_value = USBC_Readl(sunxi_hci->usb_vbase + SUNXI_USB_EHCI_TIME_INT);
	reg_value |= 0x01 << SUNXI_USB_EHCI_STANDBY_IRQ_STATUS;
	USBC_Writel(reg_value, (sunxi_hci->usb_vbase + SUNXI_USB_EHCI_TIME_INT));
}
#endif

int hci_clock_init(struct sunxi_hci_hcd *sunxi_hci, struct platform_usb_config *hci_table)
{
	if (sunxi_hci->usbc_no < USB_MAX_CONTROLLER_COUNT) {
		sunxi_hci->bus_clk_id    = hci_table[sunxi_hci->usbc_no].usb_clk;
		sunxi_hci->reset_bus_clk = hci_table[sunxi_hci->usbc_no].usb_rst;
		sunxi_hci->phy_clk_id    = hci_table[sunxi_hci->usbc_no].phy_clk;
		sunxi_hci->reset_phy_clk = hci_table[sunxi_hci->usbc_no].phy_rst;
		if (strstr(sunxi_hci->hci_name, "ohci")) {
			sunxi_hci->ohci_clk_id = hci_table[sunxi_hci->usbc_no].ohci_clk;
		}

		return 0;
	} else {
		hal_log_err("hci_clock_init failed, invalied\n");
		return -1;
	}
}

int open_clock(struct sunxi_hci_hcd *sunxi_hci)
{
	hal_reset_type_t reset_type = HAL_SUNXI_RESET;
	hal_clk_type_t clk_type = HAL_SUNXI_CCU;
	hal_clk_status_t ret;
	// mutex_lock(&usb_clock_lock);

	sunxi_hci->reset_phy = hal_reset_control_get(reset_type, sunxi_hci->reset_phy_clk);
	hal_reset_control_deassert(sunxi_hci->reset_phy);
	hal_reset_control_put(sunxi_hci->reset_phy);

	sunxi_hci->reset_hci = hal_reset_control_get(reset_type, sunxi_hci->reset_bus_clk);
	hal_reset_control_deassert(sunxi_hci->reset_hci);
	hal_reset_control_put(sunxi_hci->reset_hci);

	sunxi_hci->phy_clk = hal_clock_get(clk_type, sunxi_hci->phy_clk_id);
	ret = hal_clock_enable(sunxi_hci->phy_clk);
	if (ret) {
		hal_log_err("couldn't enable usb_clk!\n");
		return -1;
	}

	sunxi_hci->bus_clk = hal_clock_get(clk_type, sunxi_hci->bus_clk_id);
	ret = hal_clock_enable(sunxi_hci->bus_clk);
	if (ret) {
		hal_log_err("couldn't enable hci_clk!\n");
		return -1;
	}

	sunxi_hci->ohci_clk = hal_clock_get(clk_type, sunxi_hci->ohci_clk_id);
	ret = hal_clock_enable(sunxi_hci->ohci_clk);
	if (ret) {
		hal_log_err("couldn't enable ohci_clk!\n");
		return -1;
	}

#if defined(CONFIG_ARCH_SUN20IW2)
	/* USB BIAS enable, set 0 if usb disable to save power at sleep */
	USBC_Enable_Bias();
#endif
	USBC_Clean_SIDDP(sunxi_hci);

	/* otg and hci0 Controller Shared phy in SUN50I */
	if (sunxi_hci->usbc_no == HCI0_USBC_NO)
		USBC_SelectPhyToHci(sunxi_hci);

	// hal_log_info("--open_clock 0x810 = 0x%x",
	// 	     USBC_Readl(sunxi_hci->usb_vbase + SUNXI_HCI_PHY_CTRL));
	// mutex_unlock(&usb_clock_lock);

	usb_phy_init(sunxi_hci->usb_vbase + SUNXI_USB_PHY_BASE_OFFSET, sunxi_hci->usbc_no);

	return 0;
}

int close_clock(struct sunxi_hci_hcd *sunxi_hci)
{
	hal_reset_type_t reset_type = HAL_SUNXI_RESET;
	hal_clk_type_t clk_type = HAL_SUNXI_CCU;
	hal_clk_status_t ret;
	// if (sunxi_hci->ahb &&
	//		sunxi_hci->mod_usbphy &&
	//		sunxi_hci->clk_is_open) {
	//	sunxi_hci->clk_is_open = 0;
	//	clk_disable_unprepare(sunxi_hci->mod_usbphy);

	//	clk_disable_unprepare(sunxi_hci->ahb);
	//	udelay(10);
	//} else {
	//	DMSG_PANIC("[%s]: wrn: open clock failed, (0x%p, 0x%p, %d, 0x%p)\n",
	//		sunxi_hci->hci_name,
	//		sunxi_hci->ahb,
	//		sunxi_hci->mod_usbphy,
	//		sunxi_hci->clk_is_open,
	//		sunxi_hci->mod_usb);
	//}
	sunxi_hci->reset_phy = hal_reset_control_get(reset_type, sunxi_hci->reset_phy_clk);
	ret = hal_reset_control_assert(sunxi_hci->reset_phy);
	if (ret) {
		hal_log_err("couldn't disable hci_reset_phy!\n");
		return -1;
	}
	hal_reset_control_put(sunxi_hci->reset_phy);

	sunxi_hci->reset_hci = hal_reset_control_get(reset_type, sunxi_hci->reset_bus_clk);
	ret = hal_reset_control_assert(sunxi_hci->reset_hci);
	if (ret) {
		hal_log_err("couldn't disable hci_reset_bus!\n");
		return -1;
	}
	hal_reset_control_put(sunxi_hci->reset_hci);

	sunxi_hci->phy_clk = hal_clock_get(clk_type, sunxi_hci->phy_clk_id);
	ret = hal_clock_disable(sunxi_hci->phy_clk);
	if (ret) {
		hal_log_err("couldn't disable phy_clk!\n");
		return -1;
	}

	sunxi_hci->bus_clk = hal_clock_get(clk_type, sunxi_hci->bus_clk_id);
	ret = hal_clock_disable(sunxi_hci->bus_clk);
	if (ret) {
		hal_log_err("couldn't disable bus_clk!\n");
		return -1;
	}

	sunxi_hci->ohci_clk = hal_clock_get(clk_type, sunxi_hci->ohci_clk_id);
	ret = hal_clock_disable(sunxi_hci->ohci_clk);
	if (ret) {
		hal_log_err("couldn't disable ohci_clk!\n");
		return -1;
	}
	return 0;
}

static int usb_get_hsic_phy_ctrl(int value, int enable)
{
	if (enable) {
		value |= (0x07 << 8);
		value |= (0x01 << 1);
		value |= (0x01 << 0);
		value |= (0x01 << 16);
		value |= (0x01 << 20);
	} else {
		value &= ~(0x07 << 8);
		value &= ~(0x01 << 1);
		value &= ~(0x01 << 0);
		value &= ~(0x01 << 16);
		value &= ~(0x01 << 20);
	}

	return value;
}

void usb_passby(struct sunxi_hci_hcd *sunxi_hci, u32 enable)
{
	unsigned long flags;
	hal_spinlock_t passby_lock = {0};
	unsigned long reg_value = 0;

	flags = hal_spin_lock_irqsave(&passby_lock);

	reg_value = USBC_Readl(sunxi_hci->usb_vbase + SUNXI_USB_PMU_IRQ_ENABLE);
	if (enable) {
		reg_value |= (1 << 10); /* AHB Master interface INCR8 enable */
		reg_value |= (1 << 9);	/* AHB Master interface burst type INCR4 enable */
		reg_value |= (1 << 8);	/* AHB Master interface INCRX align enable */
		reg_value |= (1 << 0);	/* ULPI bypass enable */
		// hal_log_info("reg_value = 0x%x", reg_value);
	} else if (!enable) {
		reg_value &= ~(1 << 10); /* AHB Master interface INCR8 disable */
		reg_value &= ~(1 << 9);	 /* AHB Master interface burst type INCR4 disable */
		reg_value &= ~(1 << 8);	 /* AHB Master interface INCRX align disable */
		reg_value &= ~(1 << 0);	 /* ULPI bypass disable */
	}
	USBC_Writel(reg_value, (sunxi_hci->usb_vbase + SUNXI_USB_PMU_IRQ_ENABLE));

	// hal_log_info("---usb_passby 0x800 = 0x%x",
	// 	     USBC_Readl(sunxi_hci->usb_vbase + SUNXI_USB_PMU_IRQ_ENABLE));

	hal_spin_unlock_irqrestore(&passby_lock, flags);
}

static int init_pin(struct sunxi_hci_hcd *sunxi_hci)
{
	// if (strncmp(sunxi_hci->drv_vbus_name, "gpio", 4) == 0) {
	//	sunxi_hci->drv_vbus_gpio = usb_drvvbus[sunxi_hci->usbc_no];
	//	sunxi_hci->drv_vbus_gpio_valid = 1;
	//} else
	//	sunxi_hci->drv_vbus_gpio_valid = 0;

	// if (sunxi_hci->drv_vbus_gpio_valid) {
	//	if (hal_gpio_pinmux_set_function(sunxi_hci->drv_vbus_gpio, 0)) {
	//		DMSG_PANIC("ERR: %s set drvvbus gpio function failed\n",
	// sunxi_hci->hci_name); 		return;
	//	}

	//	if (hal_gpio_set_pull(sunxi_hci->drv_vbus_gpio, 1)) {
	//		DMSG_PANIC("ERR: %s pull gpio failed\n", sunxi_hci->hci_name);
	//		return;
	//	}
	//}

	return 0;
}

static void free_pin(struct sunxi_hci_hcd *sunxi_hci)
{
	// if (sunxi_hci->drv_vbus_gpio_valid) {
	//	gpio_free(sunxi_hci->drv_vbus_gpio_set.gpio);
	//	sunxi_hci->drv_vbus_gpio_valid = 0;
	//}

	// if (sunxi_hci->hsic_flag) {
	//	/* Marvell 4G HSIC ctrl */
	//	if (sunxi_hci->usb_host_hsic_rdy_valid) {
	//		gpio_free(sunxi_hci->usb_host_hsic_rdy.gpio);
	//		sunxi_hci->usb_host_hsic_rdy_valid = 0;
	//	}

	//	/* SMSC usb3503 HSIC HUB ctrl */
	//	if (sunxi_hci->usb_hsic_usb3503_flag) {
	//		if (sunxi_hci->usb_hsic_hub_connect_valid) {
	//			gpio_free(sunxi_hci->usb_hsic_hub_connect.gpio);
	//			sunxi_hci->usb_hsic_hub_connect_valid = 0;
	//		}

	//		if (sunxi_hci->usb_hsic_int_n_valid) {
	//			gpio_free(sunxi_hci->usb_hsic_int_n.gpio);
	//			sunxi_hci->usb_hsic_int_n_valid = 0;
	//		}

	//		if (sunxi_hci->usb_hsic_reset_n_valid) {
	//			gpio_free(sunxi_hci->usb_hsic_reset_n.gpio);
	//			sunxi_hci->usb_hsic_reset_n_valid = 0;
	//		}
	//	}
	//}
}

void sunxi_set_host_vbus(struct sunxi_hci_hcd *sunxi_hci, int is_on)
{
//	int ret;
//
//	if (sunxi_hci->supply) {
//		if (is_on) {
//			ret = regulator_enable(sunxi_hci->supply);
//			if (ret)
//				DMSG_PANIC("ERR: %s regulator enable failed\n",
//					sunxi_hci->hci_name);
//		} else {
//			ret = regulator_disable(sunxi_hci->supply);
//			if (ret)
//				DMSG_PANIC("ERR: %s regulator force disable failed\n",
//					sunxi_hci->hci_name);
//		}
//	}
//	if (sunxi_hci->drv_vbus_type == USB_DRV_VBUS_TYPE_GIPO) {
//		if (sunxi_hci->drv_vbus_gpio_valid)
//			__gpio_set_value(sunxi_hci->drv_vbus_gpio_set.gpio,
//					is_on);
//	} else if (sunxi_hci->drv_vbus_type == USB_DRV_VBUS_TYPE_AXP) {
//#if defined(CONFIG_AW_AXP)
//		axp_usb_vbus_output(is_on);
//#endif
//	}
//#endif
}
// EXPORT_SYMBOL(sunxi_set_host_vbus);

extern unsigned char sunxi_ehci_status_get(void);
void sunxi_set_vbus(struct sunxi_hci_hcd *sunxi_hci, int is_on)
{
	DMSG_DEBUG("[%s]: sunxi_set_vbus cnt.\n", sunxi_hci->hci_name);

#if defined(CONFIG_SOC_SUN20IW1)
	/* only for f133 because usb0 and usb1 share the same drvbus gpio
	 * close drvbus when there are no device need to use power*/
	if (!is_on && sunxi_ehci_status_get() != 0)
		return;
#endif

	if (sunxi_hci->drv_vbus_type == USB_DRV_VBUS_TYPE_GIPO) {
		// hal_gpio_set_direction(sunxi_hci->drv_vbus_gpio_set, is_on);
		if (sunxi_hci->drv_vbus_gpio_set != -1) {
			hal_gpio_set_data(sunxi_hci->drv_vbus_gpio_set, is_on);
		}
	} else if (sunxi_hci->drv_vbus_type == USB_DRV_VBUS_TYPE_AXP) {
#if defined(CONFIG_AW_AXP)
		axp_usb_vbus_output(is_on);
#endif
	}
}

void sunxi_hci_get_config_param(struct sunxi_hci_hcd *sunxi_hci)
{
#ifdef CONFIG_DRIVER_SYSCONFIG
	int ret = -1, pin_type = 0, value = 0;
	char hci_name[10] = {0};
	user_gpio_set_t pin_value = {0};

	sprintf(hci_name, "usbc%1d", sunxi_hci->usbc_no);

	ret = hal_cfg_get_keyvalue(hci_name, KEY_USB_DRVVBUS_TYPE, (int32_t *)&pin_type, 1);
	if (ret) {
		hal_log_err("get %s fail!", KEY_USB_DRVVBUS_TYPE);
		sunxi_hci->drv_vbus_type = USB_DRV_VBUS_TYPE_NULL;
		sunxi_hci->drv_vbus_gpio_set = -1;
	} else {
		sunxi_hci->drv_vbus_type = pin_type;
		ret = hal_cfg_get_keyvalue(hci_name, KEY_USB_DRVVBUS_GPIO, (int32_t *)&pin_value,
					  (sizeof(user_gpio_set_t) + 3) / sizeof(int));
		if (ret) {
			hal_log_err("get %s fail!", KEY_USB_DRVVBUS_GPIO);
			sunxi_hci->drv_vbus_gpio_set = -1;
		} else {
			sunxi_hci->drv_vbus_gpio_set
			    = (pin_value.port - 1) * PINS_PER_BANK + pin_value.port_num;
			hal_gpio_set_direction(sunxi_hci->drv_vbus_gpio_set, GPIO_DIRECTION_OUTPUT);
		}
	}

	ret = hal_cfg_get_keyvalue(hci_name, KEY_USB_DRIVER_LEVEL, (int32_t *)&value, 1);
	if (ret) {
		hal_log_err("get %s fail!!", KEY_USB_DRIVER_LEVEL);
		value = 0xf;
	}
	if (value > 0xf || value < 0x0) {
		value = 0xf;
	}
	sunxi_hci->usb_driver_level = value;

	ret = hal_cfg_get_keyvalue(hci_name, KEY_USB_IRQ_FLAG, (int32_t *)&value, 1);
	if (ret) {
		hal_log_err("get %s fail!!", KEY_USB_IRQ_FLAG);
		value = 0x0;
	}
	sunxi_hci->usb_irq_flag = value;
#else
	struct platform_usb_port_config *port_table = platform_get_port_table();
	sunxi_hci->drv_vbus_gpio_valid = port_table->drv_vbus[sunxi_hci->usbc_no].valid;
	sunxi_hci->drv_vbus_gpio_set = port_table->drv_vbus[sunxi_hci->usbc_no].gpio;

	if (sunxi_hci->drv_vbus_gpio_valid) {
		hal_log_info("[%s] usbc_no:%d , set %s %d\n",
					 sunxi_hci->hci_name, sunxi_hci->usbc_no,
					 KEY_USB_DRVVBUS_GPIO, sunxi_hci->drv_vbus_gpio_set);
		sunxi_hci->drv_vbus_type = 1;
		hal_gpio_set_direction(sunxi_hci->drv_vbus_gpio_set, GPIO_DIRECTION_OUTPUT);
		sunxi_hci->usb_driver_level = 5;
		sunxi_hci->usb_irq_flag = 0;
	}
#endif
}

static int sunxi_get_ohci_clock_src(struct platform_device *pdev,
				    struct sunxi_hci_hcd *sunxi_hci)
{
//	struct device_node *np = pdev->dev.of_node;
//
//	sunxi_hci->clk_usbohci12m = of_clk_get(np, 2);
//	if (IS_ERR(sunxi_hci->clk_usbohci12m)) {
//		sunxi_hci->clk_usbohci12m = NULL;
//		DMSG_INFO("%s get usb clk_usbohci12m clk failed.\n",
//			sunxi_hci->hci_name);
//	}
//
//	sunxi_hci->clk_hoscx2 = of_clk_get(np, 3);
//	if (IS_ERR(sunxi_hci->clk_hoscx2)) {
//		sunxi_hci->clk_hoscx2 = NULL;
//		DMSG_INFO("%s get usb clk_hoscx2 clk failed.\n",
//			sunxi_hci->hci_name);
//	}
//
//	sunxi_hci->clk_hosc = of_clk_get(np, 4);
//	if (IS_ERR(sunxi_hci->clk_hosc)) {
//		sunxi_hci->clk_hosc = NULL;
//		DMSG_INFO("%s get usb clk_hosc failed.\n",
//			sunxi_hci->hci_name);
//	}
//
//	sunxi_hci->clk_losc = of_clk_get(np, 5);
//	if (IS_ERR(sunxi_hci->clk_losc)) {
//		sunxi_hci->clk_losc = NULL;
//		DMSG_INFO("%s get usb clk_losc clk failed.\n",
//			sunxi_hci->hci_name);
//	}
//
	return 0;
}

int exit_sunxi_hci(struct sunxi_hci_hcd *sunxi_hci)
{
	// release_usb_regulator_io(sunxi_hci);
	// free_pin(sunxi_hci);
	return 0;
}
// EXPORT_SYMBOL(exit_sunxi_hci);

// int init_sunxi_hci(int usbc_type, int hci_num)
//{
//	struct sunxi_hci_hcd *sunxi_hci = NULL;
//
//	if (usbc_type == SUNXI_USB_EHCI) {
//		sunxi_hci = g_sunxi_ehci[hci_num];
//	} else if (usbc_type == SUNXI_USB_OHCI) {
//		sunxi_hci = g_sunxi_ohci[hci_num];
//	} else {
//		ehci_err("usbc type is error! n");
//		return -1;
//	}
//
//	ret = sunxi_get_hci_resource(sunxi_hci, hci_num, usbc_type);
//
//#if 0
//	if (usbc_type == SUNXI_USB_OHCI)
//		ret = sunxi_get_ohci_clock_src(pdev, sunxi_hci);
//#endif
//	return ret;
//}

// static int __parse_hci_str(const char *buf, size_t size)
//{
//	int ret = 0;
//	unsigned long val;
//
//	if (!buf) {
//		pr_err("%s()%d invalid argument\n", __func__, __LINE__);
//		return -1;
//	}
//
//	ret = kstrtoul(buf, 10, &val);
//	if (ret) {
//		pr_err("%s()%d failed to transfer\n", __func__, __LINE__);
//		return -1;
//	}
//
//	return val;
//}

// static int __init init_sunxi_hci_class(void)
//{
//	int ret = 0;
//
//	ret = class_register(&hci_class);
//	if (ret) {
//		DMSG_PANIC("%s()%d register class fialed\n", __func__, __LINE__);
//		return -1;
//	}
//
//	return 0;
//}
//
// static void __exit exit_sunxi_hci_class(void)
//{
//	class_unregister(&hci_class);
//}

// late_initcall(init_sunxi_hci_class);
// module_exit(exit_sunxi_hci_class);

// MODULE_LICENSE("GPL");
