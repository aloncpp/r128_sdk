#include "usb_manager_common.h"
#include "usb_hw_scan.h"
#include "usb_msg_center.h"

#ifdef CONFIG_ARCH_SUN20IW2
#define USB_SCAN_THREAD_STACK_SIZE 4096
#else
#define USB_SCAN_THREAD_STACK_SIZE 8192
#endif

static usb_cfg_t g_usb_cfg;
static uint32_t thread_run_flag = 1;
static int thread_stopped_flag = 1;
static hal_thread_t scan_thread_handle = NULL;

#ifdef CONFIG_KERNEL_FREERTOS
int thread_suspend_flag = 0;
#else
int thread_suspend_flag = 1;
static rt_err_t usb_manager_open(rt_device_t dev, rt_uint16_t oflag)
{
	return 0;
}

static rt_err_t usb_manager_close(rt_device_t dev)
{
	return 0;
}

static rt_err_t usb_manager_control(rt_device_t dev, int cmd, void *args)
{
	switch (cmd) {
	case DEV_IOC_USR_HWSC_ENABLE_MONITOR:
		thread_suspend_flag = 0;
		break;
	case DEV_IOC_USR_HWSC_DISABLE_MONITOR:
		thread_suspend_flag = 1;
		break;
	default:
		return 0;
	}
	return 0;
}

int usb_manager_dev_init(void)
{
	int ret = -1;
	struct rt_device *device;

	device = rt_device_create(RT_Device_Class_Block, 0);
	if (!device) {
		printf("%s %d rt_device_create fail!\n", __func__, __LINE__);
		return ret;
	}
	device->user_data = NULL;
	device->open = usb_manager_open;
	device->close = usb_manager_close;
	device->control = usb_manager_control;
	ret = rt_device_register(device, "hwsc", RT_DEVICE_FLAG_RDWR);
	if (ret != 0) {
		printf("%s %d rt_device_register fail!\n", __func__, __LINE__);
		free(device);
		return ret;
	}
	return 0;
}
#endif

static void usb_device_scan_thread(void *pArg)
{
	while (thread_run_flag) {
		hal_msleep(100); /* 1s */
		if (thread_suspend_flag)
			continue;
		hw_insmod_usb_device();
		usb_msg_center(&g_usb_cfg);
		thread_run_flag = 0;
	}
	thread_stopped_flag = 1;

#ifdef CONFIG_KERNEL_FREERTOS  // melis exit auto
	/*safe exit*/
	while (1) {
		if (!hal_thread_stop(scan_thread_handle))
			break;
		hal_msleep(3);
	}
#endif
}

static void usb_host_scan_thread(void *pArg)
{
	while (thread_run_flag) {
		hal_msleep(100); /* 1s */
		if (thread_suspend_flag)
			continue;
		hw_insmod_usb_host();
		usb_msg_center(&g_usb_cfg);
		thread_run_flag = 0;
	}
	thread_stopped_flag = 1;

#ifdef CONFIG_KERNEL_FREERTOS  // melis exit auto
	/*safe exit*/
	while (1) {
		if (!hal_thread_stop(scan_thread_handle))
			break;
		hal_msleep(3);
	}
#endif
}

static void usb_hardware_scan_thread(void *pArg)
{
	usb_cfg_t *cfg = pArg;

	while (thread_run_flag) {
		hal_msleep(100); /* 1s */
		if (thread_suspend_flag)
			continue;
		usb_hw_scan(cfg);
		usb_msg_center(cfg);
	}
	thread_stopped_flag = 1;

#ifdef CONFIG_KERNEL_FREERTOS  // melis exit auto
	/*safe exit*/
	while (1) {
		if (!hal_thread_stop(scan_thread_handle))
			break;
		hal_msleep(3);
	}
#endif
}

static hal_irqreturn_t usb_id_irq(void *parg)
{
	usb_cfg_t *cfg = parg;

	hal_msleep(100);

	/**
	 * rmmod usb device/host driver first,
	 * then insmod usb host/device driver.
	 */
	usb_hw_scan(cfg);
	usb_msg_center(cfg);

	usb_hw_scan(cfg);
	usb_msg_center(cfg);
	return HAL_IRQ_OK;
}

#if 0
static int usb_register_id_irq(void *parg)
{
    usb_cfg_t *cfg = parg;
    unsigned long irq_flags = 0;
    int ret = 0;
    int32_t Id_Irq = 0;

    /* delay for udc & hcd ready */
    hal_msleep(100);
    hw_rmmod_usb_host();
    hw_rmmod_usb_device();
    usb_msg_center(cfg);

    hw_insmod_usb_device();
    usb_msg_center(cfg);

    ret = drv_gpio_to_irq(cfg->port.id.gpio_set.gpio, &Id_Irq);
    if (ret < 0)
    {
	Usb_Manager_Err("gpio to irq error, error num: %d\n", ret);
	return -1;
    }

    irq_flags = IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING | IRQF_ONESHOT;
    ret = drv_gpio_irq_request(Id_Irq, usb_id_irq, irq_flags, NULL);
    if (ret < 0)
    {
	Usb_Manager_Err("request irq error, irq num:%lu error num: %d\n", Id_Irq, ret);
	return -1;
    }
    cfg->port.id_irq_num = Id_Irq;
    return 0;
}
#endif

static int32_t usb_script_parse(usb_cfg_t *cfg)
{
#ifdef CONFIG_DRIVER_SYSCONFIG
	int ret = -1, value = 0;
	user_gpio_set_t gpio_set = {0};

	/* usbc enable */
	ret = hal_cfg_get_keyvalue(SET_USB0, KEY_USB_ENABLE, (int32_t *)&value, 1);
	Usb_Manager_INFO("value:%d, ret:%d\n", value, ret);
	if (ret) {
		Usb_Manager_Err("script_parser_fetch %s %s fail \n", SET_USB0, KEY_USB_ENABLE);
		cfg->port.enable = 0;
		return -1;
	}
	cfg->port.enable = value;
	if (cfg->port.enable == 0) {
		Usb_Manager_INFO("%s not used!\n", SET_USB0);
		return -1;
	}

	/* usbc port type */
	ret = hal_cfg_get_keyvalue(SET_USB0, KEY_USB_PORT_TYPE, (int32_t *)&value, 1);
	Usb_Manager_INFO("port_type:%d, ret:%d\n", value, ret);
	if (ret) {
		Usb_Manager_Err("script_parser_fetch %s %s fail \n", SET_USB0, KEY_USB_PORT_TYPE);
		return -1;
	}
	cfg->port.port_type = value;

	/* usbc det type */
	ret = hal_cfg_get_keyvalue(SET_USB0, KEY_USB_DET_TYPE, (int32_t *)&value, 1);
	Usb_Manager_Debug("detect_type:%d, ret:%d\n", value, ret);
	if (ret) {
		Usb_Manager_Err("script_parser_fetch %s %s fail \n", SET_USB0, KEY_USB_DET_TYPE);
		return -1;
	}
	cfg->port.detect_type = value;

	if (cfg->port.detect_type == USB_DETECT_TYPE_VBUS_ID) {

		/* usbc id detect mode */
		ret = hal_cfg_get_keyvalue(SET_USB0, KEY_USB_DET_MODE, (int32_t *)&value, 1);
		Usb_Manager_Debug("detect_mode:%d, ret:%d\n", value, ret);
		if (ret) {
			Usb_Manager_Err("script_parser_fetch %s %s fail \n", SET_USB0, KEY_USB_DET_MODE);
			return -1;
		}
		cfg->port.detect_mode = value;

		/* get id gpio */
		memset(&gpio_set, 0x00, sizeof(gpio_set));
		ret = hal_cfg_get_keyvalue(SET_USB0, KEY_USB_ID_GPIO, (int32_t *)&gpio_set,
					  sizeof(user_gpio_set_t) >> 2);
		Usb_Manager_Debug("get %s, ret:%d\n", KEY_USB_ID_GPIO, ret);
		if (ret == 0) {
			cfg->port.id.valid = 1;
			cfg->port.id.gpio_set.gpio = (gpio_set.port - 1) * 32 + gpio_set.port_num;
			cfg->port.id.gpio_set.mul_sel = gpio_set.mul_sel;
			cfg->port.id.gpio_set.pull = gpio_set.pull;
			cfg->port.id.gpio_set.drv_level = gpio_set.drv_level;
			cfg->port.id.gpio_set.data = gpio_set.data;
			Usb_Manager_Debug("%s gpio %d data:%d\n", KEY_USB_ID_GPIO, cfg->port.id.gpio_set.gpio, cfg->port.id.gpio_set.data);
		} else {
			Usb_Manager_Err("script_parser_fetch %s %s fail \n", SET_USB0, KEY_USB_ID_GPIO);
		}

		/*get det vbus gpio */
		memset(&gpio_set, 0x00, sizeof(gpio_set));
		ret = hal_cfg_get_keyvalue(SET_USB0, KEY_USB_DETVBUS_GPIO, (int32_t *)&gpio_set,
					  sizeof(user_gpio_set_t) >> 2);
		Usb_Manager_Debug("get %s, ret:%d\n", KEY_USB_DETVBUS_GPIO, ret);
		if (ret == 0) {
			cfg->port.det_vbus.valid = 1;
			cfg->port.det_vbus.gpio_set.gpio = (gpio_set.port - 1) * 32 + gpio_set.port_num;
			cfg->port.det_vbus.gpio_set.mul_sel = gpio_set.mul_sel;
			cfg->port.det_vbus.gpio_set.pull = gpio_set.pull;
			cfg->port.det_vbus.gpio_set.drv_level = gpio_set.drv_level;
			cfg->port.det_vbus.gpio_set.data = gpio_set.data;
			Usb_Manager_Debug("%s gpio %d data:%d\n", KEY_USB_DETVBUS_GPIO, cfg->port.det_vbus.gpio_set.gpio, cfg->port.det_vbus.gpio_set.data);			
		} else {
			Usb_Manager_Err("script_parser_fetch %s %s fail \n", SET_USB0, KEY_USB_DETVBUS_GPIO);
		}

		/*get drvvbus gpio */
		memset(&gpio_set, 0x00, sizeof(gpio_set));
		ret = hal_cfg_get_keyvalue(SET_USB0, KEY_USB_DRVVBUS_GPIO, (int32_t *)&gpio_set,
					  sizeof(user_gpio_set_t) >> 2);
		Usb_Manager_Debug("get %s, ret:%d\n", KEY_USB_DRVVBUS_GPIO, ret);
		if (ret == 0) {
			cfg->port.drv_vbus.valid = 1;
			cfg->port.drv_vbus.gpio_set.gpio = (gpio_set.port - 1) * 32 + gpio_set.port_num;
			cfg->port.drv_vbus.gpio_set.mul_sel = gpio_set.mul_sel;
			cfg->port.drv_vbus.gpio_set.pull = gpio_set.pull;
			cfg->port.drv_vbus.gpio_set.drv_level = gpio_set.drv_level;
			cfg->port.drv_vbus.gpio_set.data = gpio_set.data;
			Usb_Manager_Debug("%s gpio %d data:%d\n", KEY_USB_DRVVBUS_GPIO, cfg->port.drv_vbus.gpio_set.gpio, gpio_set.data);
		} else {
			Usb_Manager_Err("script_parser_fetch %s %s fail \n", SET_USB0, KEY_USB_DRVVBUS_GPIO);
		}
	}
#else
	struct platform_usb_port_config *port_table = platform_get_port_table();

	cfg->port.enable = port_table->enable;
	cfg->port.port_type = port_table->port_type;
	cfg->port.detect_type = port_table->detect_type;
	cfg->port.detect_mode = port_table->detect_mode;

	cfg->port.id.valid = port_table->id.valid;
	cfg->port.id.gpio_set.gpio = port_table->id.gpio;
	cfg->port.id.gpio_set.mul_sel = 0;
	cfg->port.id.gpio_set.pull = 0;
	cfg->port.id.gpio_set.drv_level = 0;
	cfg->port.id.gpio_set.data = 0;

	cfg->port.det_vbus.valid = port_table->det_vbus.valid;
	cfg->port.det_vbus.gpio_set.gpio = port_table->det_vbus.gpio;
	cfg->port.det_vbus.gpio_set.mul_sel = 0;
	cfg->port.det_vbus.gpio_set.pull = 0;
	cfg->port.det_vbus.gpio_set.drv_level = 0;
	cfg->port.det_vbus.gpio_set.data = 0;

	cfg->port.drv_vbus.valid = port_table->drv_vbus[0].valid;
	cfg->port.drv_vbus.gpio_set.gpio = port_table->drv_vbus[0].gpio;
	cfg->port.drv_vbus.gpio_set.mul_sel = 0;
	cfg->port.drv_vbus.gpio_set.pull = 0;
	cfg->port.drv_vbus.gpio_set.drv_level = 0;
	cfg->port.drv_vbus.gpio_set.data = 0;
#endif
	return 0;
}

#if defined(CONFIG_COMPONENTS_PM)
static int usb_manager_suspend(struct pm_device *dev, suspend_mode_t mode)
{
	struct usb_port_info *port_info = NULL;
	usb_cfg_t *cfg = &g_usb_cfg;

	port_info = (struct usb_port_info *)&cfg->port;

	switch (mode)
	{
	case PM_MODE_SLEEP:
	case PM_MODE_STANDBY:
	case PM_MODE_HIBERNATION:
		break;
	default:
		break;
	}

	switch (port_info->port_type) {
	case USB_PORT_TYPE_DEVICE:
		if (port_info->detect_type == USB_DETECT_TYPE_VBUS_ID) {
			if (port_info->id.valid) {
				hal_gpio_pinmux_set_function(port_info->id.gpio_set.gpio, USB_IO_DISABLED);
				hal_gpio_set_pull(port_info->id.gpio_set.gpio, USB_IO_NOPULL);
			}
			if (port_info->det_vbus.valid) {
				hal_gpio_pinmux_set_function(port_info->det_vbus.gpio_set.gpio, USB_IO_DISABLED);
				hal_gpio_set_pull(port_info->det_vbus.gpio_set.gpio, USB_IO_NOPULL);
			}
		}
		break;
	case USB_PORT_TYPE_OTG:
		if (port_info->detect_type == USB_DETECT_TYPE_VBUS_ID) {
			if (port_info->id.valid) {
				hal_gpio_pinmux_set_function(port_info->id.gpio_set.gpio, USB_IO_DISABLED);
				hal_gpio_set_pull(port_info->id.gpio_set.gpio, USB_IO_NOPULL);
			}
			if (port_info->det_vbus.valid) {
				hal_gpio_pinmux_set_function(port_info->det_vbus.gpio_set.gpio, USB_IO_DISABLED);
				hal_gpio_set_pull(port_info->det_vbus.gpio_set.gpio, USB_IO_NOPULL);
			}
			if (port_info->drv_vbus.valid) {
				hal_gpio_pinmux_set_function(port_info->drv_vbus.gpio_set.gpio, USB_IO_DISABLED);
				hal_gpio_set_pull(port_info->drv_vbus.gpio_set.gpio, USB_IO_NOPULL);
			}
		}
		break;
	default:
		break;
	}
	return 0;
}

static int usb_manager_resume(struct pm_device *dev, suspend_mode_t mode)
{
	struct usb_port_info *port_info = NULL;
	usb_cfg_t *cfg = &g_usb_cfg;

	port_info = (struct usb_port_info *)&cfg->port;

	switch (port_info->port_type) {
	case USB_PORT_TYPE_DEVICE:
		if (port_info->detect_type == USB_DETECT_TYPE_VBUS_ID) {
			if (port_info->id.valid) {
				hal_gpio_pinmux_set_function(port_info->id.gpio_set.gpio, port_info->id.gpio_set.mul_sel);
				hal_gpio_set_pull(port_info->id.gpio_set.gpio, port_info->id.gpio_set.pull);
			}
			if (port_info->det_vbus.valid) {
				hal_gpio_pinmux_set_function(port_info->det_vbus.gpio_set.gpio, port_info->det_vbus.gpio_set.mul_sel);
				hal_gpio_set_pull(port_info->det_vbus.gpio_set.gpio, port_info->det_vbus.gpio_set.pull);
			}
		}
		break;
	case USB_PORT_TYPE_OTG:
		if (port_info->detect_type == USB_DETECT_TYPE_VBUS_ID) {
			if (port_info->id.valid) {
				hal_gpio_pinmux_set_function(port_info->id.gpio_set.gpio, port_info->id.gpio_set.mul_sel);
				hal_gpio_set_pull(port_info->id.gpio_set.gpio, port_info->id.gpio_set.pull);
			}

			if (port_info->det_vbus.valid) {
				hal_gpio_pinmux_set_function(port_info->det_vbus.gpio_set.gpio, port_info->det_vbus.gpio_set.mul_sel);
				hal_gpio_set_pull(port_info->det_vbus.gpio_set.gpio, port_info->det_vbus.gpio_set.pull);
			}

			if (port_info->drv_vbus.valid) {
				hal_gpio_pinmux_set_function(port_info->drv_vbus.gpio_set.gpio, port_info->drv_vbus.gpio_set.mul_sel);
				hal_gpio_set_pull(port_info->drv_vbus.gpio_set.gpio, port_info->drv_vbus.gpio_set.pull);
			}
		}
		break;
	default:
		break;
	}
	return 0;
}

static struct pm_devops usb_manager_devops = {
	.suspend = usb_manager_suspend,
	.resume = usb_manager_resume,
};

static struct pm_device usb_manager_dev = {
	.name = "usb_manager",
	.ops = &usb_manager_devops,
};
#endif

int hal_usb_manager_init(void)
{
	int ret = -1;

	Usb_Manager_Debug("init \n");
	memset(&g_usb_cfg, 0, sizeof(usb_cfg_t));
	usb_msg_center_init();
	ret = usb_script_parse(&g_usb_cfg);
	if (ret != 0) {
		Usb_Manager_Err("ERR: get_usb_cfg failed\n");
		return -1;
	}
	thread_run_flag = 1;
	thread_stopped_flag = 0;
#ifndef CONFIG_KERNEL_FREERTOS
	usb_manager_dev_init();
#endif

	if (g_usb_cfg.port.port_type == USB_PORT_TYPE_DEVICE) {
		usb_hw_scan_init(&g_usb_cfg);
		scan_thread_handle = hal_thread_create(usb_device_scan_thread,
						    &g_usb_cfg,
						    "usb-device-scan",
						    USB_SCAN_THREAD_STACK_SIZE,
						    HAL_THREAD_PRIORITY_SYS);
		if (scan_thread_handle == NULL) {
			Usb_Manager_Err("ERR: hal_thread_create failed\n");
			return -1;
		}
		hal_thread_start(scan_thread_handle);
	} else if (g_usb_cfg.port.port_type == USB_PORT_TYPE_HOST) {
		scan_thread_handle = hal_thread_create(usb_host_scan_thread,
						    &g_usb_cfg,
						    "usb-host-scan",
						    USB_SCAN_THREAD_STACK_SIZE,
						    HAL_THREAD_PRIORITY_SYS);
		if (scan_thread_handle == NULL) {
			Usb_Manager_Err("ERR: hal_thread_create failed\n");
			return -1;
		}
		hal_thread_start(scan_thread_handle);
	} else {
		usb_hw_scan_init(&g_usb_cfg);
		if (g_usb_cfg.port.detect_mode == USB_DETECT_MODE_THREAD) {
			scan_thread_handle = hal_thread_create(usb_hardware_scan_thread,
							    &g_usb_cfg,
							    "usb-hardware-scan",
							    USB_SCAN_THREAD_STACK_SIZE,
							    HAL_THREAD_PRIORITY_SYS);
			if (scan_thread_handle == NULL) {
				Usb_Manager_Err("ERR: hal_thread_create failed\n");
				return -1;
			}
		}
	// else if (g_usb_cfg.port.detect_mode == USB_DETECT_MODE_INTR)
	// {
	// 	if (usb_register_id_irq(&g_usb_cfg) != 0)
	// 	{
	// 	Usb_Manager_Err("ERR: usb_register_id_irq failed\n");
	// 	return -1;
	// 	}
	// }
		else {
			Usb_Manager_Err("ERR: usb detect mode isn't supported\n");
			return -1;
		}
		hal_thread_start(scan_thread_handle);
	}
#if defined(CONFIG_COMPONENTS_PM)
	pm_devops_register(&usb_manager_dev);
#endif
	return ret;
}

int hal_usb_manager_deinit(void)
{
#ifdef CONFIG_COMPONENTS_PM
	pm_devops_unregister(&usb_manager_dev);
#endif
	if (g_usb_cfg.port.port_type == USB_PORT_TYPE_OTG) {
		if (g_usb_cfg.port.detect_mode == USB_DETECT_MODE_THREAD) {
			thread_run_flag = 0;
			while (!thread_stopped_flag) {
				Usb_Manager_Err("waitting for usb_hardware_scan_thread stop\n");
				hal_msleep(20);
			}
		} else if (g_usb_cfg.port.detect_mode == USB_DETECT_MODE_INTR) {
			hal_free_irq(g_usb_cfg.port.id_irq_num);
		}
	}
	hw_rmmod_usb_host();
	hw_rmmod_usb_device();
	usb_msg_center(&g_usb_cfg);
	usb_msg_center_exit();
	return 0;
}
