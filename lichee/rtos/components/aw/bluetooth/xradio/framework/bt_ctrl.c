/*
 * Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of XRADIO TECHNOLOGY CO., LTD. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <string.h>
#include "errno.h"
#include "kernel/os/os.h"
#include "bt_ctrl.h"
#include "bt_lib.h"

#ifdef CONFIG_BLEHOST
#include "hci_driver.h"
#endif

#if CONFIG_COMPONENTS_BT_PM
#include "bt_pm.h"
#endif

#if defined(CONFIG_DRIVERS_TRNG) && defined(CONFIG_ARCH_SUN20IW2)
#include <sunxi_hal_trng.h>
#endif

#define BT_CTRL_ERR(fmt, arg...)    printf("BT_CTRL<E>: " fmt, ##arg)
#define BT_CTRL_WARN(fmt, arg...)   printf("BT_CTRL<W>: " fmt, ##arg)
#define BT_CTRL_INFO(fmt, arg...)   printf("BT_CTRL<I>: " fmt, ##arg)

#define CTRL_NO_DEV_REGISTER        0
#define CTRL_ONE_DEV_REGISTER       1

static uint8_t bt_ctrl_ready = 0;

#ifdef CONFIG_BLEHOST
extern const struct bt_hci_driver * bt_ctrl_get_zephyr_interface(void);

int bt_zephyr_adapter_register(void)
{
	const struct bt_hci_driver *drv = bt_ctrl_get_zephyr_interface();

	int ret = bt_hci_driver_register(drv);

	if (!(ret == -EALREADY || ret == 0))
		return -EINVAL;

	return 0;
}

int bt_zephyr_adapter_unregister(void)
{
	const struct bt_hci_driver *drv = bt_ctrl_get_zephyr_interface();

	int ret = 0;
#if defined(CONFIG_BT_DEINIT)
	ret = bt_hci_driver_unregister(drv);

	if (!(ret == -EALREADY || ret == 0))
		return -EINVAL;
#endif

	return ret;
}
#endif /* CONFIG_BLEHOST */

const struct vendor_t *bt_ctrl_get_bluedroid_interface(void);
int bt_bluedroid_adapter_register(void)
{
	const struct vendor_t *drv = bt_ctrl_get_bluedroid_interface();

#ifdef CONFIG_COMPONENTS_BLUEDROID
	void vendor_set_interface(const void *vendor);
	vendor_set_interface((void *)drv);
#endif
	return 0;
}

int bt_bluedroid_adapter_unregister(void)
{
	return 0;
}

static inline int bt_adapter_register(void)
{
#ifdef CONFIG_BLEHOST
	bt_zephyr_adapter_register();
#endif

	bt_bluedroid_adapter_register();

	return 0;
}

static inline int bt_adapter_unregister(void)
{
	bt_bluedroid_adapter_unregister();

#ifdef CONFIG_BLEHOST
	bt_zephyr_adapter_unregister();
#endif

	return 0;
}

void bt_ctrl_set_mac(uint8_t *mac)
{
	const bt_lib_interface_t *bt_lib_interface;
	int i;
	uint8_t mac_addr[6];
	if (!bt_ctrl_ready)
		return;
	bt_lib_interface = bt_lib_get_interface();

	if (mac == NULL) {
#if defined(CONFIG_DRIVERS_TRNG) && defined(CONFIG_ARCH_SUN20IW2)
	uint32_t rand[4];
	HAL_TRNG_Extract(0, rand);
	for (i = 0; i < 6; i++) {
		mac_addr[i] = *((uint8_t *)rand + i);
	}
#else
	for (i = 0; i < 6; i++) {
		srand((unsigned int)XR_OS_GetTicks());
		mac_addr[i] = rand() % 255 + 1;
	}
#endif
	} else {
		memcpy(mac_addr, mac, 6);
	}

	if (mac_addr[5] == 0x9E && mac_addr[4] == 0x8B) {
		if (mac_addr[3] >= 0x00 && mac_addr[3] <= 0x3F) {
			mac_addr[3] = 0xFF - mac_addr[3];
		}
	}
	BT_CTRL_INFO("mac is: %02x:%02x:%02x:%02x:%02x:%02x\n", mac_addr[0], mac_addr[1],
						mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);

	bt_lib_interface->hci_ctrl_ops->set_mac(mac_addr);
}

int bt_ctrl_enable(void)
{
	const bt_lib_interface_t *bt_lib_interface;

	if (bt_ctrl_ready) {
		bt_ctrl_ready++;
		return 0;
	}

#ifdef CONFIG_COMPONENTS_BT_PM
	bt_pm_init();
#endif

#ifdef CONFIG_BT_DRIVERS_LIB
	if ((bt_lib_interface = bt_lib_get_interface()) &&
		                   bt_lib_interface->init) {
		bt_lib_interface->init();
	} else {
		BT_CTRL_ERR("BT LIB Interface not exist!\n");
		return -1;
	}
#else
	#error "should enable CONFIG_BT_DRIVERS_LIB!"
#endif

	bt_ctrl_ready++;

	return 0;
}

int bt_ctrl_disable(void)
{
	if (bt_ctrl_ready == CTRL_NO_DEV_REGISTER) {
		return -EPERM;
	} else if (bt_ctrl_ready > CTRL_ONE_DEV_REGISTER) {
		bt_ctrl_ready--;
		return 0;
	}

	const bt_lib_interface_t *bt_lib_interface = bt_lib_get_interface();
	bt_lib_interface->deinit();

#if CONFIG_COMPONENTS_BT_PM
	bt_pm_deinit();
#endif
	bt_ctrl_ready = 0;

	return 0;
}
