/*
 * sound\common\snd_sunxi_rxsync.c
 * (C) Copyright 2021-2025
 * AllWinner Technology Co., Ltd. <www.allwinnertech.com>
 * caiyongheng <caiyongheng@allwinnertech.com>
 * Dby <dby@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <sound/snd_core.h>
#include "snd_sunxi_rxsync.h"

#define HLOG		"RXSYNC"

#define RX_SYNC_DEV_MAX	8

typedef void (*enable_func)(void *data, bool enable);

typedef struct {
	void *data;
	/*enable_func route_enable;*/
	enable_func rx_enable;
} rx_sync_dev_t;

static struct {
	int total_count[RX_SYNC_DOMAIN_CNT];
	int enabled_count[RX_SYNC_DOMAIN_CNT];
	int total_ctl_count[RX_SYNC_DOMAIN_CNT];

	rx_sync_dev_t dev_info[RX_SYNC_DOMAIN_CNT][RX_SYNC_DEV_MAX];
	rx_sync_dev_t *last_enabled_dev[RX_SYNC_DOMAIN_CNT];
} rx_sync_data;

static hal_spinlock_t rx_sync_lock;

/* Return the rx_sync id. The id is unique in its own domain. */
int sunxi_rx_sync_probe(rx_sync_domain_t domain)
{
	int ret = 0;
	uint32_t __cpsr;

	__cpsr = hal_spin_lock_irqsave(&rx_sync_lock);
	rx_sync_data.total_count[domain]++;
	snd_err("domain=%d, total_count=%d\n",
		      domain, rx_sync_data.total_count[domain]);
	if (rx_sync_data.total_count[domain] > RX_SYNC_DEV_MAX) {
		snd_err("domain=%d, too many rx_sync devices (current=%d, max=%d)\n",
			    domain, rx_sync_data.total_count[domain], RX_SYNC_DEV_MAX);
		ret = -EINVAL;
		goto unlock;
	}
	/* use total_count to define rx_sync id */
	ret = rx_sync_data.total_count[domain] - 1;
unlock:
	hal_spin_unlock_irqrestore(&rx_sync_lock, __cpsr);
	return ret;
}

void sunxi_rx_sync_remove(rx_sync_domain_t domain)
{
	uint32_t __cpsr;

	__cpsr = hal_spin_lock_irqsave(&rx_sync_lock);
	rx_sync_data.total_count[domain]--;
	snd_print("domain=%d, total_count=%d\n",
		      domain, rx_sync_data.total_count[domain]);
	if (rx_sync_data.total_count[domain] < 0)
		rx_sync_data.total_count[domain] = 0;
	hal_spin_unlock_irqrestore(&rx_sync_lock, __cpsr);
}

void sunxi_rx_sync_startup(rx_sync_domain_t domain, int id,
			   void *data, void (*rx_enable)(void *data, bool enable))
{
	uint32_t __cpsr;

	if (id < 0 || id >= RX_SYNC_DEV_MAX) {
		snd_err("unsupport rx sync id\n");
		return;
	}
	if (!rx_enable || !data) {
		snd_print("card dev or data err.\n");
		return;
	}

	hal_spin_lock_irqsave(&rx_sync_lock);
	rx_sync_data.dev_info[domain][id].data = data;
	rx_sync_data.dev_info[domain][id].rx_enable = rx_enable;

	rx_sync_data.total_ctl_count[domain]++;

	snd_info("total_ctl_count=%d, domain=%d, id=%d\n",
		rx_sync_data.total_ctl_count[domain], domain, id);
	hal_spin_unlock_irqrestore(&rx_sync_lock, __cpsr);
}

void sunxi_rx_sync_shutdown(rx_sync_domain_t domain, int id)
{
	uint32_t __cpsr;

	if (id < 0 || id >= RX_SYNC_DEV_MAX) {
		snd_err("unsupport rx sync id\n");
		return;
	}
	snd_print("domian=%d, id=%d, enabled_count=%d\n",
		      domain, id, rx_sync_data.enabled_count[domain]);

	hal_spin_lock_irqsave(&rx_sync_lock);
	rx_sync_data.total_ctl_count[domain]--;

	snd_print("domian=%d, id=%d, reset rx_sync_data\n", domain, id);
	memset(&(rx_sync_data.dev_info[domain][id]), 0, sizeof(rx_sync_dev_t));

	hal_spin_unlock_irqrestore(&rx_sync_lock, __cpsr);
}

void sunxi_rx_sync_control(rx_sync_domain_t domain, int id, bool enable)
{
	uint32_t __cpsr;
	int i;
	int total_ctl_count = 0;
	int enabled_count = 0;

	snd_print("domain=%d, id=%d, enable=%d, enabled_count=%d\n",
		      domain, id, enable, rx_sync_data.enabled_count[domain]);
	if (id < 0 || id >= RX_SYNC_DEV_MAX) {
		snd_err("unsupport rx sync id\n");
		return;
	}

	hal_spin_lock_irqsave(&rx_sync_lock);
	if (enable) {
		rx_sync_data.enabled_count[domain]++;
		/* store the pointer of last enabled device. */
		if (rx_sync_data.enabled_count[domain] == rx_sync_data.total_ctl_count[domain]) {
			rx_sync_data.last_enabled_dev[domain] = &rx_sync_data.dev_info[domain][id];
			snd_print("domain=%d, id=%d, last_enabled_dev=0x%p\n",
				      domain, id, rx_sync_data.last_enabled_dev[domain]);
		} else if (rx_sync_data.enabled_count[domain] > \
			   rx_sync_data.total_ctl_count[domain]) {
			snd_err("domain=%d, enabled_count(%d) is more than "
				    "total_ctl_count(%d). %s was called incorrectly?\n",
				    domain, rx_sync_data.enabled_count[domain],
				    rx_sync_data.total_ctl_count[domain], __func__);
			goto unlock;
		}

		/* Check all devices in all domains. Only if the last device in
		 * all domains runs here, we will run rx_enable(1) in only
		 * the last device in each domain.
		 */
		for (i = 0; i < RX_SYNC_DOMAIN_CNT; i++) {
			total_ctl_count += rx_sync_data.total_ctl_count[i];
			enabled_count += rx_sync_data.enabled_count[i];
		}
		if (enabled_count < total_ctl_count) {
			goto unlock;
		} else if (enabled_count > total_ctl_count) {
			snd_err("sum of enabled_count(%d) is more than "
				    "total_ctl_count(%d). %s was called incorrectly?\n",
				    enabled_count, total_ctl_count, __func__);
			goto unlock;
		}
		for (i = 0; i < RX_SYNC_DOMAIN_CNT; i++) {
			rx_sync_dev_t *dev = rx_sync_data.last_enabled_dev[i];
			if (!dev)
				continue;
			snd_info("domain=%d, rx enable\n", i);
			dev->rx_enable(dev->data, 1);
		}
	} else {
		rx_sync_data.enabled_count[domain]--;

		if (rx_sync_data.enabled_count[domain] > 0) {
			goto unlock;
		} else if (rx_sync_data.enabled_count[domain] < 0) {
			snd_err("domain=%d, enabled_count(%d) is less than 0. "
				    "%s was called incorrectly?\n",
				    domain, rx_sync_data.enabled_count[domain], __func__);
			goto unlock;
		}

		/* Check all devices in all domains. Only if the last device in
		 * all domains runs here, we will run rx_enable(0) in only
		 * the last device in each domain.
		 */
		for (i = 0; i < RX_SYNC_DOMAIN_CNT; i++) {
			enabled_count += rx_sync_data.enabled_count[i];
		}
		if (enabled_count > 0) {
			goto unlock;
		} else if (enabled_count < 0) {
			snd_err("sum of enabled_count(%d) is "
				    "less than 0.%s was called incorrectly?\n",
				    enabled_count, __func__);
			goto unlock;
		}
		for (i = 0; i < RX_SYNC_DOMAIN_CNT; i++) {
			rx_sync_dev_t *dev = rx_sync_data.last_enabled_dev[i];
			if (!dev)
				continue;
			snd_print("domain=%d, rx disable\n", i);
			dev->rx_enable(dev->data, 0);
		}
	}
unlock:
	hal_spin_unlock_irqrestore(&rx_sync_lock, __cpsr);
	return;
}

