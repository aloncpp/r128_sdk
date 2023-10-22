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

#include <string.h>
#include "kernel/os/os.h"
#include "sys_ctrl/sys_ctrl.h"
#include "net_ctrl.h"
#include "net_init.h"
#include "sysinfo/sysinfo.h"
#include "net_ctrl_debug.h"

#ifdef CONFIG_WLAN_AP

#ifdef CONFIG_ARCH_SUN20IW2P1
/* note: please force 64Byte cacheline alignment in R128 dual core mode */
__weak const wlan_ap_default_conf_t __attribute__((aligned(CACHELINE_LEN)))
#else
__weak const wlan_ap_default_conf_t
#endif
g_wlan_ap_default_conf = {
	.ssid = "AP-XRADIO",
	.ssid_len = 9,
	.psk = "123456789",
	.hw_mode = WLAN_AP_HW_MODE_IEEE80211G,
	.ieee80211n = 1,
	.key_mgmt = WPA_KEY_MGMT_PSK,
	.wpa_pairwise_cipher = WPA_CIPHER_TKIP,
	.rsn_pairwise_cipher = WPA_CIPHER_CCMP,
	.proto = WPA_PROTO_WPA | WPA_PROTO_RSN,
	.auth_alg = WPA_AUTH_ALG_OPEN,
	.group_rekey = 3600,
	.gmk_rekey = 86400,
	.ptk_rekey = 0,
	.strict_rekey = 1,
	.channel = 1,
	.beacon_int = 100,
	.dtim = 1,
	.max_num_sta = 4,
	.country = {'C', 'N', ' '},
};

#endif /* CONFIG_WLAN_AP */

static wlan_init_status g_wifi_init_flg = WLAN_STATUS_NO_INIT;

wlan_init_status wlan_get_init_status(void)
{
	return g_wifi_init_flg;
}

void wlan_set_init_status(wlan_init_status status)
{
	g_wifi_init_flg = status;
}

void net_core_init(void)
{
	int ret;

	ret = sys_ctrl_create();
	if (ret) {
		NET_ERR("sys create failed.\n");
		return;
	}

	ret = sysinfo_init();
	if (ret) {
		NET_ERR("sys info init failed.\n");
		goto end;
	}
	ret = net_ctrl_init();
	if (ret) {
		NET_ERR("wifi event init failed.\n");
		goto end;
	}

	//pm_mode_platform_select(PRJCONF_PM_MODE);

/*
#if (CONFIG_WPA_HEAP_MODE == 1)
	wpa_set_heap_fn(psram_malloc, psram_realloc, psram_free);
#endif
#if (CONFIG_UMAC_HEAP_MODE == 1)
	umac_set_heap_fn(psram_malloc, psram_free);
#endif
#if (CONFIG_LMAC_HEAP_MODE == 1)
	lmac_set_heap_fn(psram_malloc, psram_free);
#endif
#if (CONFIG_MBUF_HEAP_MODE == 1)
	wlan_ext_request(NULL, WLAN_EXT_CMD_SET_RX_QUEUE_SIZE, 256);
#endif
*/

#ifdef CONFIG_WLAN_AP
	wlan_ap_set_default_conf(&g_wlan_ap_default_conf);
#endif

#ifndef CONFIG_ETF
	/* init tcpip stack in cpu0_app_entry(), because the
	 * APP may only use lwip socket without WLAN */
	//net_sys_init();
#endif

	struct sysinfo *sysinfo = sysinfo_get();
	net_sys_start(sysinfo->wlan_mode);
end:
	//net_ctrl_deinit();
	//sys_ctrl_destroy();
	return;
}

int xr_wlan_on(int mode)
{
	int ret = 0;

	NET_INF("starting mode: %d\n", mode);
	if (wlan_get_init_status() == WLAN_STATUS_NO_INIT) {
		net_core_init();
		wlan_set_init_status(WLAN_STATUS_INITED);
		ret = net_switch_mode(mode);
	} else {
		ret = net_sys_start(mode);
		/* if ret err, we try switch mode directly */
		if (ret != 0) {
			NET_WRN("%s() err, we try switch mode(%d) directly!\n", __func__, mode);
			ret = net_switch_mode(mode);
		}
	}

	/* if (mode == WLAN_MODE_MONITOR) */
	/* wlan_monitor_set_rx_cb(g_wlan_netif, monitor_rx_cb); */

	return ret;
}

int xr_wlan_off(void)
{
	return net_sys_stop();
}
