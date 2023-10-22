/*
 * drivers/usb/host/ohci_sunxi.c
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * yangnaitian, 2011-5-24, create this file
 * javen, 2011-6-26, add suspend and resume
 * javen, 2011-7-18, move clock and power operations out from driver
 *
 * OHCI Driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include "ohci.h"
#include "sunxi-hci.h"

#ifdef CONFIG_COMPONENTS_PM
#include "pm_suspend.h"
#include "pm_wakelock.h"
#include "pm_devops.h"
static const char pm_ohci_name[16];
#endif

// #define   SUNXI_OHCI_NAME	"sunxi-ohci"
// static const char ohci_name[] = SUNXI_OHCI_NAME;

#ifdef CONFIG_USB_SUNXI_OHCI0
#define  SUNXI_OHCI0_OF_MATCH	"allwinner,sunxi-ohci0"
#else
#define SUNXI_OHCI0_OF_MATCH	"null"
#endif

#ifdef CONFIG_USB_SUNXI_OHCI1
#define  SUNXI_OHCI1_OF_MATCH	"allwinner,sunxi-ohci1"
#else
#define SUNXI_OHCI1_OF_MATCH	"null"
#endif

#ifdef CONFIG_USB_SUNXI_OHCI2
#define  SUNXI_OHCI2_OF_MATCH	"allwinner,sunxi-ohci2"
#else
#define SUNXI_OHCI2_OF_MATCH	"null"
#endif

#ifdef CONFIG_USB_SUNXI_OHCI3
#define  SUNXI_OHCI3_OF_MATCH	"allwinner,sunxi-ohci3"
#else
#define SUNXI_OHCI3_OF_MATCH	"null"
#endif

#if USB_MAX_CONTROLLER_COUNT
static struct sunxi_hci_hcd g_sunxi_ohci[USB_MAX_CONTROLLER_COUNT];
#else
static struct sunxi_hci_hcd *g_sunxi_ohci = 0;
#endif

static u32 ohci_first_probe[4] = {1, 1, 1, 1};
static u32 ohci_enable[4] = {1, 1, 1, 1};
// static atomic_t ohci_in_standby;

// #ifdef CONFIG_PM
// static void sunxi_ohci_resume_work(struct work_struct *work);
// #endif

// int sunxi_usb_disable_ohci(__u32 usbc_no);
// int sunxi_usb_enable_ohci(__u32 usbc_no);

// static ssize_t ohci_enable_show(struct device *dev,
// 		struct device_attribute *attr, char *buf)
// {
// 	struct sunxi_hci_hcd *sunxi_ohci = NULL;

// 	if (dev == NULL) {
// 		DMSG_PANIC("ERR: Argment is invalid\n");
// 		return 0;
// 	}

// 	sunxi_ohci = dev->platform_data;
// 	if (sunxi_ohci == NULL) {
// 		DMSG_PANIC("ERR: sw_ohci is null\n");
// 		return 0;
// 	}

// 	return sprintf(buf, "ohci:%d,probe:%u\n",
// 			sunxi_ohci->usbc_no, sunxi_ohci->probe);
// }

// static ssize_t ohci_enable_store(struct device *dev,
// 		struct device_attribute *attr,
// 		const char *buf, size_t count)
// {
// 	struct sunxi_hci_hcd *sunxi_ohci = NULL;
// 	int value = 0;
// 	int err;

// 	if (dev == NULL) {
// 		DMSG_PANIC("ERR: Argment is invalid\n");
// 		return 0;
// 	}

// 	sunxi_ohci = dev->platform_data;
// 	if (sunxi_ohci == NULL) {
// 		DMSG_PANIC("ERR: sw_ohci is null\n");
// 		return 0;
// 	}

// 	ohci_first_probe[sunxi_ohci->usbc_no] = 0;

// 	err = kstrtoint(buf, 10, &value);
// 	if (err != 0)
// 		return -EINVAL;
// 	if (value == 1) {
// 		ohci_enable[sunxi_ohci->usbc_no] = 0;
// 		sunxi_usb_enable_ohci(sunxi_ohci->usbc_no);

// 		if (sunxi_ohci->usbc_no == HCI0_USBC_NO)
// 			sunxi_set_host_vbus(sunxi_ohci, 1);

// 	} else if (value == 0) {
// 		ohci_enable[sunxi_ohci->usbc_no] = 1;
// 		sunxi_usb_disable_ohci(sunxi_ohci->usbc_no);
// 		ohci_enable[sunxi_ohci->usbc_no] = 0;
// 	} else {
// 		DMSG_INFO("unknown value (%d)\n", value);
// 	}

// 	return count;
// }

// static DEVICE_ATTR(ohci_enable, 0644, ohci_enable_show, ohci_enable_store);

static int open_ohci_clock(struct sunxi_hci_hcd *sunxi_ohci)
{
	return sunxi_ohci->open_clock(sunxi_ohci);
}

static int close_ohci_clock(struct sunxi_hci_hcd *sunxi_ohci)
{
	return sunxi_ohci->close_clock(sunxi_ohci);
}

static void sunxi_ohci_set_vbus(struct sunxi_hci_hcd *sunxi_ohci, int is_on)
{
	sunxi_ohci->set_power(sunxi_ohci, is_on);
}

static void sunxi_ohci_set_passby(struct sunxi_hci_hcd *sunxi_ohci, int is_on)
{
	sunxi_ohci->usb_passby(sunxi_ohci, is_on);
}

static void sunxi_start_ohci(struct sunxi_hci_hcd *sunxi_ohci)
{
	open_ohci_clock(sunxi_ohci);
	sunxi_ohci_set_passby(sunxi_ohci, 1);
	sunxi_ohci_set_vbus(sunxi_ohci, 1);
}

static void sunxi_stop_ohci(struct sunxi_hci_hcd *sunxi_ohci)
{
	sunxi_ohci_set_vbus(sunxi_ohci, 0);
	sunxi_ohci_set_passby(sunxi_ohci, 0);
	close_ohci_clock(sunxi_ohci);
}

static int sunxi_ohci_start(struct usb_hcd *hcd)
{
	struct ohci_hcd *ohci = hcd_to_ohci(hcd);
	int ret;

	ret = ohci_init(ohci);
	if (ret < 0)
		return ret;

	ret = ohci_run(ohci);
	if (ret < 0) {
		DMSG_PANIC("can't start %s", hcd->self.bus_name);
		ohci_stop(hcd);
		return ret;
	}

	return 0;
}

// static int sunxi_ohci_pm_notify(struct notifier_block *nb,
// 				unsigned long mode, void *_unused)
// {
// 	switch (mode) {
// 	case PM_HIBERNATION_PREPARE:
// 	case PM_RESTORE_PREPARE:
// 	case PM_SUSPEND_PREPARE:
// 		// atomic_set(&ohci_in_standby, 1);
// 		HAL_ATMOTIC_SET(&ohci_in_standby, 1);
// 		break;
// 	case PM_POST_HIBERNATION:
// 	case PM_POST_RESTORE:
// 	case PM_POST_SUSPEND:
// 		// atomic_set(&ohci_in_standby, 0);
// 		HAL_ATMOTIC_SET(&ohci_in_standby, 0);
// 		sunxi_hci_standby_completion(SUNXI_USB_OHCI);
// 		break;
// 	default:
// 		break;
// 	}

// 	return 0;
// }

// static struct notifier_block sunxi_ohci_pm_nb = {
// 	.notifier_call = sunxi_ohci_pm_notify,
// };

static const struct hc_driver sunxi_ohci_hc_driver = {
	.description	        = "ohci_hcd",
	.product_desc	        = "SW USB2.0 'Open' Host Controller (OHCI) Driver",
	.hcd_priv_size	        = sizeof(struct ohci_hcd),

	/*
	 * generic hardware linkage
	 */
	.irq			= NULL,//ohci_irq,
	// .flags		= HCD_USB11 | HCD_MEMORY,
	.flags			=  HC_DRIVER_FLAG_HCD_USB11,

	/*
	 * basic lifecycle operations
	 */
	.start			= sunxi_ohci_start,
	.stop			= ohci_stop,
	// .shutdown		= ohci_shutdown,

	/*
	 * managing i/o requests and associated device resources
	 */
	.urb_enqueue		= ohci_urb_enqueue,
	.urb_dequeue		= ohci_urb_dequeue,
	.endpoint_disable	= ohci_endpoint_disable,

	/*
	 * scheduling support
	 */
	.get_frame_number	= ohci_get_frame,

	/*
	 * root hub support
	 */
	.hub_status_data	= ohci_hub_status_data,
	.hub_control		= ohci_hub_control,

#ifdef CONFIG_PM
	.bus_suspend		= ohci_bus_suspend,
	.bus_resume		= ohci_bus_resume,
#endif
	// .start_port_reset	= ohci_start_port_reset,
	.start_port_reset	= NULL,
};

static int sunxi_insmod_ohci(struct sunxi_hci_hcd *sunxi_ohci)
{
	int ret;
	struct usb_hcd *hcd = NULL;

	/* if usb is disabled, can not probe */
	// if (usb_disabled()) {
	// 	DMSG_PANIC("ERR: usb hcd is disabled\n");
	// 	return -ENODEV;
	// }

	// sunxi_ohci = pdev->dev.platform_data;
	if (!sunxi_ohci) {
		DMSG_PANIC("ERR: sunxi_ohci is null\n");
		ret = -ENOMEM;
		goto ERR1;
	}

	printf("\n[ohci-usb%d] insmod host driver!\n", sunxi_ohci->usbc_no);

	// sunxi_ohci->pdev = pdev;
	// g_sunxi_ohci[sunxi_ohci->usbc_no] = sunxi_ohci;

	// DMSG_INFO("[%s%d]: probe, pdev->name: %s, sunxi_ohci: 0x%p\n",
	// 	ohci_name, sunxi_ohci->usbc_no, pdev->name, sunxi_ohci);

	sunxi_ohci->ohci_base = (uint32_t *)(long)(sunxi_ohci->usb_vbase + SUNXI_USB_OHCI_BASE_OFFSET);

	sunxi_ohci->ohci_reg_length = SUNXI_USB_OHCI_LEN;

	// /* not init ohci, when driver probe */
	// if (sunxi_ohci->usbc_no == HCI0_USBC_NO) {
	// 	if (sunxi_ohci->port_type != USB_PORT_TYPE_HOST) {
	// 		if (ohci_first_probe[sunxi_ohci->usbc_no]) {
	// 			ohci_first_probe[sunxi_ohci->usbc_no] = 0;
	// 			DMSG_INFO("[%s%d]: Not init ohci0\n",
	// 				  sunxi_ohci->hci_name, sunxi_ohci->usbc_no);
	// 			return 0;
	// 		}
	// 	}
	// }

	/* creat a usb_hcd for the ohci controller */
	hcd = usb_create_hc_gen_dev(&sunxi_ohci_hc_driver, sunxi_ohci->hci_name);
	if (!hcd) {
		DMSG_PANIC("ERR: usb_ohci_create_hcd failed\n");
		ret = -ENOMEM;
		goto ERR2;
	}

	// hcd->rsrc_start = sunxi_ohci->usb_base_res->start;
	// hcd->rsrc_len	= SUNXI_USB_OHCI_LEN;
	hcd->regs = (void *)sunxi_ohci->ohci_base;
	sunxi_ohci->hcd = hcd;

	if (hal_request_irq(sunxi_ohci->irq_no, ohci_irq_handler, "ohci", hcd) < 0) {
		hal_log_err("request irq error\n");
		return -1;
	}

	hal_enable_irq(sunxi_ohci->irq_no);

	/* ochi start to work */
	sunxi_start_ohci(sunxi_ohci);

	ohci_hcd_init(hcd_to_ohci(hcd));

	hcd->state = HC_GEN_DEV_STATE_RUNNING;

	ret = usb_add_hc_gen_dev(hcd, sunxi_ohci->irq_no, 0);
	if (ret != 0) {
		DMSG_PANIC("ERR: usb_add_hcd failed\n");
		ret = -ENOMEM;
		goto ERR3;
	}

	// device_wakeup_enable(hcd->self.controller);
	// platform_set_drvdata(pdev, hcd);

// #ifndef USB_SYNC_SUSPEND
// 	device_enable_async_suspend(&pdev->dev);
// #endif

// #ifdef CONFIG_PM
// 	if (!sunxi_ohci->wakeup_suspend)
// 		INIT_WORK(&sunxi_ohci->resume_work, sunxi_ohci_resume_work);
// #endif

	sunxi_ohci->probe = 1;

	return 0;

ERR3:
	sunxi_stop_ohci(sunxi_ohci);
	// usb_put_hcd(hcd);

ERR2:
	sunxi_ohci->hcd = NULL;

ERR1:
	return ret;
}

static int sunxi_rmmod_ohci(struct sunxi_hci_hcd *sunxi_ohci)
{
	// struct usb_hcd *hcd = NULL;
	// unsigned long time_left;

	// hcd = platform_get_drvdata(pdev);
	// if (hcd == NULL) {
	// 	DMSG_PANIC("ERR: hcd is null\n");
	// 	return -1;
	// }

	// sunxi_ohci = pdev->dev.platform_data;
	// if (sunxi_ohci == NULL) {
	// 	DMSG_PANIC("ERR: sunxi_ohci is null\n");
	// 	return -1;
	// }

	// if (atomic_read(&ohci_in_standby)) {
	// 	reinit_completion(&sunxi_ohci->standby_complete);
	// 	DMSG_INFO("INFO: sunxi_ohci disable, waiting until standby finish\n");
	// 	time_left = wait_for_completion_timeout(&sunxi_ohci->standby_complete,
	// 					msecs_to_jiffies(STANDBY_TIMEOUT));
	// 	if (time_left)
	// 		DMSG_INFO("INFO: sunxi_ohci disable time_left = %lu\n", time_left);
	// 	else
	// 		DMSG_PANIC("ERR: sunxi_ohci waiting standby failed, go on disable\n");

	// }

	if (sunxi_ohci->probe == 0) {
		hal_log_err("ERR: sunxi_ohci is disable, can not disable again\n");
		return -1;
	}
	sunxi_ohci->probe = 0;

	// DMSG_INFO("[%s%d]: remove, pdev->name: %s, sunxi_ohci: 0x%p\n",
	// 	ohci_name, sunxi_ohci->usbc_no, pdev->name, sunxi_ohci);
	printf("[ohci-usb%d] rmmod host driver!\n", sunxi_ohci->usbc_no);

	// usb_remove_hcd(hcd);
	usb_remove_hc_gen_dev(sunxi_ohci->hcd);

	if (sunxi_ohci->irq_no > 0)
		hal_free_irq(sunxi_ohci->irq_no);
	// usb_put_hcd(hcd);
	sunxi_stop_ohci(sunxi_ohci);

	sunxi_ohci->hcd = NULL;

	return 0;
}

// static int sunxi_ohci_hcd_probe(struct platform_device *pdev)
// {
// 	int ret = 0;
// // #if defined(CONFIG_ARCH_SUN50IW10)
// // 	int val = 0;
// // #endif
// 	struct sunxi_hci_hcd *sunxi_ohci = NULL;

// 	if (pdev == NULL) {
// 		DMSG_PANIC("ERR: %s, Argment is invalid\n", __func__);
// 		return -1;
// 	}

// 	/* if usb is disabled, can not probe */
// 	if (usb_disabled()) {
// 		DMSG_PANIC("ERR: usb hcd is disabled\n");
// 		return -ENODEV;
// 	}

// 	ret = init_sunxi_hci(pdev, SUNXI_USB_OHCI);
// 	if (ret != 0) {
// 		dev_err(&pdev->dev, "init_sunxi_hci is fail\n");
// 		return -1;
// 	}

// 	sunxi_insmod_ohci(pdev);

// 	sunxi_ohci = pdev->dev.platform_data;
// 	if (sunxi_ohci == NULL) {
// 		DMSG_PANIC("ERR: %s, sunxi_ohci is null\n", __func__);
// 		return -1;
// 	}

// 	// if (sunxi_ohci->usbc_no == HCI0_USBC_NO) {
// 	// 	ret = register_pm_notifier(&sunxi_ohci_pm_nb);
// 	// 	if (ret) {
// 	// 		DMSG_PANIC("ERR: %s, can not register suspend notifier\n", __func__);
// 	// 		return -1;
// 	// 	}
// 	// }

// 	// init_completion(&sunxi_ohci->standby_complete);

// /* keep common circuit configuration when usb0 enable only*/
// // #if defined(CONFIG_ARCH_SUN50IW10)
// // 	if (sunxi_ohci->usbc_no == HCI0_USBC_NO) {
// // 		/*phy reg, offset:0x10 bit3 set 0, enable siddq*/
// // 		val = USBC_Readl(sunxi_ohci->usb_common_phy_config + SUNXI_HCI_PHY_CTRL);
// // 		val &= ~(0x1 << SUNXI_HCI_PHY_CTRL_SIDDQ);
// // 		USBC_Writel(val, sunxi_ohci->usb_common_phy_config + SUNXI_HCI_PHY_CTRL);
// // 	}
// // #endif

// 	// if (ohci_enable[sunxi_ohci->usbc_no]) {
// 		// device_create_file(&pdev->dev, &dev_attr_ohci_enable);
// 		// ohci_enable[sunxi_ohci->usbc_no] = 0;
// 	// }

// 	return 0;
// }

// static int sunxi_ohci_hcd_remove(struct platform_device *pdev)
// {
// 	struct sunxi_hci_hcd *sunxi_ohci = NULL;

// 	int ret = 0;

// 	if (pdev == NULL) {
// 		DMSG_PANIC("ERR: %s, Argment is invalid\n", __func__);
// 		return -1;
// 	}

// 	sunxi_ohci = pdev->dev.platform_data;
// 	if (sunxi_ohci == NULL) {
// 		DMSG_PANIC("ERR: %s, sunxi_ohci is null\n", __func__);
// 		return -1;
// 	}

// 	// if (ohci_enable[sunxi_ohci->usbc_no] == 0)
// 		// device_remove_file(&pdev->dev, &dev_attr_ohci_enable);

// 	if (sunxi_ohci->probe == 1) {
// 		ret = sunxi_rmmod_ohci(pdev);
// 		if (ret == 0)
// 			exit_sunxi_hci(sunxi_ohci);

// 		// if (sunxi_ohci->usbc_no == HCI0_USBC_NO)
// 		// 	unregister_pm_notifier(&sunxi_ohci_pm_nb);

// 		return ret;
// 	} else
// 		return 0;
// }

// static void sunxi_ohci_hcd_shutdown(struct platform_device *pdev)
// {
// 	struct sunxi_hci_hcd *sunxi_ohci = NULL;

// 	sunxi_ohci = pdev->dev.platform_data;
// 	if (sunxi_ohci == NULL) {
// 		DMSG_PANIC("ERR: %s sunxi_ohci is null\n", __func__);
// 		return;
// 	}

// 	if (sunxi_ohci->probe == 0) {
// 		DMSG_INFO("%s, %s is disable, need not shutdown\n",
// 			__func__, sunxi_ohci->hci_name);
// 		return;
// 	}

// 	pr_debug("[%s]: ohci shutdown start\n", sunxi_ohci->hci_name);

// 	// usb_hcd_platform_shutdown(pdev);

// 	/**
// 	 * disable usb otg INTUSBE, to solve usb0 device mode
// 	 * catch audio udev on reboot system is fail.
// 	 */
// 	if (sunxi_ohci->usbc_no == 0)
// 		if (sunxi_ohci->otg_vbase) {
// 			writel(0, (sunxi_ohci->otg_vbase + SUNXI_USBC_REG_INTUSBE));
// 		}

// 	pr_debug("[%s]: ohci shutdown end\n", sunxi_ohci->hci_name);
// }

// #ifdef CONFIG_PM

// static int sunxi_ohci_hcd_suspend(struct device *dev)
// {
// 	struct sunxi_hci_hcd *sunxi_ohci = NULL;
// 	struct usb_hcd *hcd = NULL;
// 	struct ohci_hcd *ohci = NULL;
// 	int val = 0;

// 	if (dev == NULL) {
// 		DMSG_PANIC("ERR: Argment is invalid\n");
// 		return 0;
// 	}

// 	hcd = dev_get_drvdata(dev);
// 	if (hcd == NULL) {
// 		DMSG_PANIC("ERR: hcd is null\n");
// 		return 0;
// 	}

// 	sunxi_ohci = dev->platform_data;
// 	if (sunxi_ohci == NULL) {
// 		DMSG_PANIC("ERR: sunxi_ohci is null\n");
// 		return 0;
// 	}

// 	if (sunxi_ohci->no_suspend) {
// 		DMSG_INFO("[%s]:ohci is being enable, stop system suspend\n",
// 			sunxi_ohci->hci_name);
// 		return -1;
// 	}

// 	if (sunxi_ohci->probe == 0) {
// 		DMSG_INFO("[%s]: is disable, can not suspend\n", sunxi_ohci->hci_name);
// 		return 0;
// 	}

// 	ohci = hcd_to_ohci(hcd);
// 	if (ohci == NULL) {
// 		DMSG_PANIC("ERR: ohci is null\n");
// 		return 0;
// 	}

// 	if (sunxi_ohci->wakeup_suspend == USB_STANDBY) {
// 		DMSG_INFO("[%s] usb suspend\n", sunxi_ohci->hci_name);
// 		val = ohci_readl(ohci, &ohci->regs->control);
// 		val |= OHCI_CTRL_RWE;
// 		ohci_writel(ohci, val, &ohci->regs->control);

// 		val = ohci_readl(ohci, &ohci->regs->intrenable);
// 		val |= OHCI_INTR_RD;
// 		val |= OHCI_INTR_MIE;
// 		ohci_writel(ohci, val, &ohci->regs->intrenable);

// #ifdef SUNXI_USB_STANDBY_LOW_POW_MODE
// 		val = ohci_readl(ohci, &ohci->regs->control);
// 		val |= OHCI_USB_SUSPEND;
// 		ohci_writel(ohci, val, &ohci->regs->control);

// #ifdef SUNXI_USB_STANDBY_NEW_MODE
// 		/*phy reg, offset:0x0 bit2, set 1, enable RC16M CLK*/
// 		sunxi_hci_set_rc_clk(sunxi_ohci, 1);
// #endif
// 		/*phy reg, offset:0x08 bit3, set 1, remote enable*/
// 		sunxi_hci_set_wakeup_ctrl(sunxi_ohci, 1);
// 		/*phy reg, offset:0x10 bit3 set 1, clean siddq*/
// 		sunxi_hci_set_siddq(sunxi_ohci, 1);
// #endif

// #if defined(CONFIG_ARCH_SUN50IW10)
// 		/*phy reg, offset:0x0 bit31, set 1*/
// 		val = USBC_Readl(sunxi_ohci->usb_vbase + SUNXI_USB_PMU_IRQ_ENABLE);
// 		val |= (0x1 << 31);
// 		USBC_Writel(val, sunxi_ohci->usb_vbase + SUNXI_USB_PMU_IRQ_ENABLE);

// 		/*phy reg, offset:0x0 bit3, set 1*/
// 		val = USBC_Readl(sunxi_ohci->usb_vbase + SUNXI_USB_PMU_IRQ_ENABLE);
// 		val |= (0x1 << 3);
// 		USBC_Writel(val, sunxi_ohci->usb_vbase + SUNXI_USB_PMU_IRQ_ENABLE);

// 		/*prcm, offset:0x0 bit3, set 1*/
// 		val = USBC_Readl(sunxi_ohci->prcm);
// 		val |= (0x1 << 3);
// 		USBC_Writel(val, sunxi_ohci->prcm);
// #endif

// 		if (sunxi_ohci->clk_usbohci12m && sunxi_ohci->clk_losc)
// 			clk_set_parent(sunxi_ohci->clk_usbohci12m,
// 					sunxi_ohci->clk_losc);
// 	} else {
// 		DMSG_INFO("[%s]super suspend\n", sunxi_ohci->hci_name);
// 		atomic_add(1, &g_sunxi_usb_super_standby);

// 		/**
// 		 * Root hub was already suspended. Disable irq emission and
// 		 * mark HW unaccessible, bail out if RH has been resumed. Use
// 		 * the spinlock to properly synchronize with possible pending
// 		 * RH suspend or resume activity.
// 		 *
// 		 * This is still racy as hcd->state is manipulated outside of
// 		 * any locks =P But that will be a different fix.
// 		 */
// 		ohci_suspend(hcd, device_may_wakeup(dev));

// 		sunxi_stop_ohci(sunxi_ohci);
// 		sunxi_hci_set_siddq(sunxi_ohci, 1);

// 		cancel_work_sync(&sunxi_ohci->resume_work);
// 	}

// 	return 0;
// }

// static void sunxi_ohci_resume_work(struct work_struct *work)
// {
// 	struct sunxi_hci_hcd *sunxi_ohci = NULL;

// 	sunxi_ohci = container_of(work, struct sunxi_hci_hcd, resume_work);

// 	/* Waiting hci to resume. */
// 	msleep(5000);

// 	sunxi_ohci_set_vbus(sunxi_ohci, 1);
// 	atomic_sub(1, &g_sunxi_usb_super_standby);
// }

// static int sunxi_ohci_hcd_resume(struct device *dev)
// {
// 	struct sunxi_hci_hcd *sunxi_ohci = NULL;
// 	struct usb_hcd *hcd = NULL;
// 	struct ohci_hcd *ohci = NULL;
// #ifdef SUNXI_USB_STANDBY_LOW_POW_MODE
// 	int val = 0;
// #endif

// 	if (dev == NULL) {
// 		DMSG_PANIC("ERR: Argment is invalid\n");
// 		return 0;
// 	}

// 	hcd = dev_get_drvdata(dev);
// 	if (hcd == NULL) {
// 		DMSG_PANIC("ERR: hcd is null\n");
// 		return 0;
// 	}

// 	sunxi_ohci = dev->platform_data;
// 	if (sunxi_ohci == NULL) {
// 		DMSG_PANIC("ERR: sunxi_ohci is null\n");
// 		return 0;
// 	}

// 	if (sunxi_ohci->probe == 0) {
// 		DMSG_INFO("[%s]: is disable, can not resume\n", sunxi_ohci->hci_name);
// 		return 0;
// 	}

// 	ohci = hcd_to_ohci(hcd);
// 	if (ohci == NULL) {
// 		DMSG_PANIC("ERR: ohci is null\n");
// 		return 0;
// 	}

// 	if (sunxi_ohci->wakeup_suspend == USB_STANDBY) {
// 		DMSG_INFO("[%s]usb resume\n", sunxi_ohci->hci_name);

// 		if (sunxi_ohci->clk_usbohci12m && sunxi_ohci->clk_hoscx2)
// 			clk_set_parent(sunxi_ohci->clk_usbohci12m, sunxi_ohci->clk_hoscx2);

// #if defined(CONFIG_ARCH_SUN50IW10)
// 		/*prcm, offset:0x0 bit3, set 0*/
// 		val = USBC_Readl(sunxi_ohci->prcm);
// 		val &= ~(0x1 << 3);
// 		USBC_Writel(val, sunxi_ohci->prcm);

// 		/*phy reg, offset:0x0 bit3, set 0*/
// 		val = USBC_Readl(sunxi_ohci->usb_vbase + SUNXI_USB_PMU_IRQ_ENABLE);
// 		val &= ~(0x1 << 3);
// 		USBC_Writel(val, sunxi_ohci->usb_vbase + SUNXI_USB_PMU_IRQ_ENABLE);

// 		/*phy reg, offset:0x0 bit31, set 0*/
// 		val = USBC_Readl(sunxi_ohci->usb_vbase + SUNXI_USB_PMU_IRQ_ENABLE);
// 		val &= ~(0x1 << 31);
// 		USBC_Writel(val, sunxi_ohci->usb_vbase + SUNXI_USB_PMU_IRQ_ENABLE);
// #endif

// #ifdef SUNXI_USB_STANDBY_LOW_POW_MODE
// 		/*phy reg, offset:0x10 bit3 set 0, enable siddq*/
// 		sunxi_hci_set_siddq(sunxi_ohci, 0);
// 		/*phy reg, offset:0x08 bit3, set 0, remote disable*/
// 		sunxi_hci_set_wakeup_ctrl(sunxi_ohci, 0);

// #ifdef SUNXI_USB_STANDBY_NEW_MODE
// 		/*disable rc clk*/
// 		/*phy reg, offset:0x0 bit2, set 0, disable rc clk*/
// 		sunxi_hci_set_rc_clk(sunxi_ohci, 0);
// #endif

// 		val = ohci_readl(ohci, &ohci->regs->control);
// 		val &= ~(OHCI_USB_SUSPEND);
// 		ohci_writel(ohci, val, &ohci->regs->control);
// #endif
// 	} else {
// 		DMSG_INFO("[%s]super resume\n", sunxi_ohci->hci_name);
// 		sunxi_hci_set_siddq(sunxi_ohci, 0);
// 		open_ohci_clock(sunxi_ohci);
// 		sunxi_ohci_set_passby(sunxi_ohci, 1);
// 		set_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);
// 		ohci_resume(hcd, false);

// #if defined(CONFIG_ARCH_SUN50IW10)
// 		if (sunxi_ohci->usbc_no == HCI0_USBC_NO) {
// 			/*phy reg, offset:0x10 bit3 set 0, enable siddq*/
// 			val = USBC_Readl(sunxi_ohci->usb_common_phy_config + SUNXI_HCI_PHY_CTRL);
// 			val &= ~(0x1 << SUNXI_HCI_PHY_CTRL_SIDDQ);
// 			USBC_Writel(val, sunxi_ohci->usb_common_phy_config + SUNXI_HCI_PHY_CTRL);
// 		}
// #endif

// 		schedule_work(&sunxi_ohci->resume_work);
// 	}

// 	return 0;
// }

// static const struct dev_pm_ops sunxi_ohci_pmops = {
// 	.suspend	= sunxi_ohci_hcd_suspend,
// 	.resume		= sunxi_ohci_hcd_resume,
// };

// #define SUNXI_OHCI_PMOPS (&sunxi_ohci_pmops)

// #else

// #define SUNXI_OHCI_PMOPS NULL

// #endif

// static const struct of_device_id sunxi_ohci_match[] = {
// 	{.compatible = SUNXI_OHCI0_OF_MATCH, },
// 	{.compatible = SUNXI_OHCI1_OF_MATCH, },
// 	{.compatible = SUNXI_OHCI2_OF_MATCH, },
// 	{.compatible = SUNXI_OHCI3_OF_MATCH, },
// 	{},
// };
// MODULE_DEVICE_TABLE(of, sunxi_ohci_match);


// static struct platform_driver sunxi_ohci_hcd_driver = {
// 	.probe		= sunxi_ohci_hcd_probe,
// 	.remove		= sunxi_ohci_hcd_remove,
// 	.shutdown	= sunxi_ohci_hcd_shutdown,
// 	.driver		= {
// 		.name	= ohci_name,
// 		.owner	= THIS_MODULE,
// 		.pm	= SUNXI_OHCI_PMOPS,
// 		.of_match_table = sunxi_ohci_match,
// 	},
// };

#ifdef CONFIG_COMPONENTS_PM
static int sunxi_ohci_hcd_suspend(struct pm_device *dev, suspend_mode_t mode)
{
	struct sunxi_hci_hcd *sunxi_ohci = dev->data;
	struct usb_hcd *hcd = sunxi_ohci->hcd;
	bool device_may_wakeup = true;

	hal_log_info("%s suspend start, mode:%d\n", __func__, sunxi_ohci->hci_name, mode);

	switch (mode)
	{
		case PM_MODE_SLEEP:
		case PM_MODE_STANDBY:
		case PM_MODE_HIBERNATION:
			break;
		default:
			break;
	}
	ohci_suspend(hcd, device_may_wakeup);
	sunxi_stop_ohci(sunxi_ohci);
	sunxi_hci_set_siddq(sunxi_ohci, 1);

	hal_log_info("%s suspend end\n", sunxi_ohci->hci_name);

	return 0;
}

static int sunxi_ohci_hcd_resume(struct pm_device *dev, suspend_mode_t mode)
{
	struct sunxi_hci_hcd *sunxi_ohci = dev->data;
	struct usb_hcd *hcd = sunxi_ohci->hcd;

	hal_log_info("%s resume start, mode:%d\n", sunxi_ohci->hci_name, mode);

	sunxi_hci_set_siddq(sunxi_ohci, 0);
	open_ohci_clock(sunxi_ohci);
	sunxi_ohci_set_passby(sunxi_ohci, 1);
	ohci_resume(hcd, false);

	sunxi_ohci_set_vbus(sunxi_ohci, 1);

	hal_log_info("%s resume end\n", sunxi_ohci->hci_name);

	return 0;
}

static struct pm_devops sunxi_ohci_devops = {
	.suspend = sunxi_ohci_hcd_suspend,
	.resume = sunxi_ohci_hcd_resume,
};

static struct pm_device sunxi_ohci_dev = {
	.name = pm_ohci_name,
	.ops = &sunxi_ohci_devops,
	.data = NULL,
};
#endif

int sunxi_ohci_hcd_init(__u32 hci_num)
{
	int value = 0;
	char ohci_name[10] = {0};
	sprintf(ohci_name, "usbc%1d", hci_num);

#ifdef CONFIG_DRIVER_SYSCONFIG
	if (hal_cfg_get_keyvalue(ohci_name, KEY_USB_ENABLE, (int32_t *)&value, 1) == 0) {
		if (value == SUNXI_USB_DISABLE) {
			hal_log_err("ERR: ohci%d disable", hci_num);
			return -1;
		}
	}
#else
	value = 1;
#endif

	if (g_sunxi_ohci == 0) {
		hal_log_err("ERR: ohci%d NULL!", hci_num);
		return -1;
	}

	if (hci_num >= USB_MAX_CONTROLLER_COUNT) {
		hal_log_err("ERR: ohci%d oversize!", hci_num);
		return -1;
	}

	struct sunxi_hci_hcd *sunxi_ohci = &g_sunxi_ohci[hci_num];

	if (!sunxi_ohci->hcd) {
		sunxi_ohci->no_suspend = 1;

		struct platform_usb_config *ohci_table = platform_get_ohci_table();
		struct platform_usb_config *otg_table = platform_get_otg_table();

		sunxi_ohci->usbc_no = hci_num;
		sunxi_ohci->usb_vbase = ohci_table[hci_num].pbase;
		sunxi_ohci->irq_no = ohci_table[hci_num].irq;
		sunxi_ohci->otg_vbase = otg_table->pbase;

		sprintf(sunxi_ohci->hci_name, "%s", ohci_table[hci_num].name);

		hci_clock_init(sunxi_ohci, ohci_table);
		sunxi_hci_get_config_param(sunxi_ohci);

		sunxi_ohci->open_clock = open_clock;
		sunxi_ohci->close_clock = close_clock;
		sunxi_ohci->set_power = sunxi_set_vbus;
		sunxi_ohci->usb_passby = usb_passby;

		sunxi_insmod_ohci(sunxi_ohci);
		sunxi_ohci->no_suspend = 0;
		// DMSG_INFO("[%s]: sunxi_ohci_hcd_init\n", sunxi_ohci->hci_name);
#ifdef CONFIG_COMPONENTS_PM
		sprintf((char*)sunxi_ohci_dev.name, "sunxi_ohci%d", hci_num);
		sunxi_ohci_dev.data = sunxi_ohci;
		pm_devops_register(&sunxi_ohci_dev);
#endif
		return 0;
	} else {
		hal_log_err("ERR: ohci%d already exist!", hci_num);
	}

	return -1;
}

int sunxi_ohci_hcd_deinit(__u32 hci_num)
{
	struct sunxi_hci_hcd *sunxi_ohci = &g_sunxi_ohci[hci_num];

#ifdef CONFIG_COMPONENTS_PM
	pm_devops_unregister(&sunxi_ohci_dev);
#endif
	if (sunxi_ohci->hcd)
		return sunxi_rmmod_ohci(sunxi_ohci);
	else
		hal_log_err("ERR: ohci%d is NULL!", hci_num);

	return 0;
}
