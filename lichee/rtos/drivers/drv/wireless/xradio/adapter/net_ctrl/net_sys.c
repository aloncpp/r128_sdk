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

#include "prj_config.h"

#include <string.h>
#include <stdlib.h>
#include "kernel/os/os.h"
#include "net/wlan/wlan.h"
#include "net/wlan/wlan_defs.h"
#include "lwip/tcpip.h"
#if PRJCONF_NET_DHCP_SERVER
#include "usr_dhcpd.h"
#endif

#include "sysinfo/sysinfo.h"
#include "net_ctrl.h"
#include "net_ctrl_debug.h"

struct netif *g_wlan_netif = NULL;

static void netif_tcpip_init_done(void *arg)
{
//	NET_DBG("%s()\n", __func__);
}

void net_sys_init(void)
{
	tcpip_init(netif_tcpip_init_done, NULL); /* Init lwip module */
}

#if LWIP_XR_DEINIT
void net_sys_deinit(void)
{
	tcpip_deinit(); /* DeInit lwip module */
}
#endif

static int _net_sys_start(enum wlan_mode mode)
{
	struct sysinfo *sysinfo = sysinfo_get();
	if (sysinfo == NULL) {
		NET_ERR("failed to get sysinfo %p\n", sysinfo);
		return -1;
	}

	if (wlan_sys_init() != 0) {
		NET_ERR("net system start failed\n");
		return -1;
	}

#ifndef CONFIG_ETF_CLI
	wlan_set_mac_addr(NULL, sysinfo->mac_addr, SYSINFO_MAC_ADDR_LEN);
	wlan_attach(net_ctrl_msg_send);
	g_wlan_netif = net_open(mode);
#endif /* CONFIG_ETF_CLI */
	return 0;
}

#ifdef CONFIG_ARCH_SUN20IW2
#include "prcm.h"
#endif

static int _net_sys_stop(void)
{
	if (g_wlan_netif && wlan_if_get_mode(g_wlan_netif) == WLAN_MODE_HOSTAP) {
#if PRJCONF_NET_DHCP_SERVER
		dhcp_server_stop();
#else
		NET_WRN("dhcp_server_stop() not support\n");
#endif
	}

	net_close(g_wlan_netif);
	g_wlan_netif = NULL;
	wlan_detach();

#ifdef CONFIG_ARCH_SUN20IW2
	while (HAL_PMU_IsWLANAliveStatus())
#endif
		XR_OS_MSleep(10);

	if (wlan_sys_deinit()) {
		NET_ERR("net system stop failed\n");
		return -1;
	}

	return 0;
}

static net_sys_status g_net_sys_status = NET_SYS_STATUS_STOP;

net_sys_status net_sys_status_get(void)
{
	return g_net_sys_status;
}

void net_sys_status_set(net_sys_status status)
{
	g_net_sys_status = status;
}

int net_sys_start(enum wlan_mode mode)
{
	if (net_sys_status_get() != NET_SYS_STATUS_STOP) {
		NET_WRN("net system has started!\n");
		return -1;
	}

	_net_sys_start(mode);
	//pm_register_wlan_power_onoff(net_sys_onoff, PRJCONF_NET_PM_MODE);
	net_sys_status_set(NET_SYS_STATUS_START);

	return 0;
}

int net_sys_stop(void)
{
	if (net_sys_status_get() != NET_SYS_STATUS_START) {
		NET_WRN("net system has stopped!\n");
		return -1;
	}

	//pm_unregister_wlan_power_onoff();
	_net_sys_stop();
	net_sys_status_set(NET_SYS_STATUS_STOP);

	return 0;
}

int net_sys_onoff(unsigned int enable)
{
	struct sysinfo *sysinfo = sysinfo_get();
	if (sysinfo == NULL) {
		NET_ERR("failed to get sysinfo %p\n", sysinfo);
		return -1;
	}

	printf("%s set net to power%s\n", __func__, enable?"on":"off");

	if (enable) {
		_net_sys_start(sysinfo->wlan_mode);
#ifdef CONFIG_AUTO_RECONNECT_AP
		net_ctrl_connect_ap(NULL);
#endif
	} else {
#ifdef CONFIG_AUTO_RECONNECT_AP
		net_ctrl_disconnect_ap(NULL, 1);
#endif
		_net_sys_stop();
	}

	return 0;
}
