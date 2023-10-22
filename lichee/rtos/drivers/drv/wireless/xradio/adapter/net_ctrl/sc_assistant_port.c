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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kernel/os/os.h"
#include "kernel/os/os_time.h"
#include "lwip/netif.h"
#include "lwip/sockets.h"

#include "net/wlan/wlan_defs.h"
#include "smartlink/sc_assistant.h"

#include "sysinfo/sysinfo.h"
#include "net_ctrl.h"

#define SCA_DEBUG_LEVEL 1

#if (SCA_DEBUG_LEVEL >= 3)
#define SCA_LOGD(format, args...) printf(format, ##args)
#else
#define SCA_LOGD(x...)
#endif
#if (SCA_DEBUG_LEVEL >= 2)
#define SCA_LOGN(format, args...) printf(format, ##args)
#else
#define SCA_LOGN(x...)
#endif

#if (SCA_DEBUG_LEVEL >= 1)
#define SCA_LOGW(format, args...) printf("SCA_WAR:"format, ##args)
#define SCA_LOGE(format, args...) printf("SCA_ERR:"format, ##args)
#else
#define SCA_LOGW(x...)
#define SCA_LOGE(x...)
#endif

#define SCA_BUG_ON(v) do {if(v) {printf("BUG at %s:%d!\n", __func__, __LINE__); \
                                 while (1);}} while (0)
#define SCA_WARN_ON(v) do {if(v) {printf("WARN at %s:%d!\n", __func__, __LINE__);} \
                          } while (0)

#define SCA_CONNECTING_AP               (1 << 0)
#define SCA_STOP_CONNECTING_AP          (1 << 1)

static uint8_t stop_connect_ap = 0;

static uint8_t *__sc_assistant_get_mac(uint8_t *mac_hex)
{
	struct sysinfo *sysinfo;

	SCA_BUG_ON(!mac_hex);

	sysinfo = sysinfo_get();
	memcpy(mac_hex, sysinfo->mac_addr, IEEE80211_ADDR_LEN);

	return mac_hex;
}

static void __sc_assistant_open_monitor(void)
{
	enum wlan_mode mode = wlan_if_get_mode(g_wlan_netif);

	if (mode == WLAN_MODE_HOSTAP) {
		wlan_ap_disable();
	}

	net_switch_mode(WLAN_MODE_MONITOR);
}

/* exit monitor mode, enter station mode */
static void __sc_assistant_close_monitor(void)
{
	net_switch_mode(WLAN_MODE_STA);
}

/* channel:1-13 */
static void __sc_assistant_switch_channel(char channel)
{
	wlan_monitor_set_channel(g_wlan_netif, channel);
}

static int __sc_assistant_send_raw_frame(int type, uint8_t *buffer, int len)
{
	return wlan_send_raw_frame(g_wlan_netif, type, buffer, len);
}

static struct netif *__sc_assistant_open_sta(void)
{
	net_switch_mode(WLAN_MODE_STA);

	return g_wlan_netif;
}

/* set psk to NULL if no psk.
 */
static sc_assistant_connect_status __sc_assistant_connect_ap(uint8_t *ssid, int ssid_len,
                                                             uint8_t *psk, unsigned int timeout_ms)
{
	struct netif *nif = g_wlan_netif;
	uint32_t timeout = XR_OS_GetTicks() + XR_OS_TicksToMSecs(timeout_ms);
	sc_assistant_connect_status ret = SCA_CONNECT_FAIL;

	net_switch_mode(WLAN_MODE_STA);
	wlan_sta_disable();

	SCA_LOGD("connect AP:%s psk:%s\n", ssid, psk ? psk : (uint8_t *)(""));
	if (wlan_sta_set((uint8_t *)ssid, ssid_len, psk)) {
		goto err;
	}

	XR_OS_ThreadSuspendScheduler();
	stop_connect_ap |= SCA_CONNECTING_AP;
	XR_OS_ThreadResumeScheduler();

	wlan_sta_enable();
	while (!(stop_connect_ap & SCA_STOP_CONNECTING_AP) &&
	       !(nif && netif_is_up(nif) && netif_is_link_up(nif)) &&
	       XR_OS_TimeBeforeEqual(XR_OS_GetTicks(), timeout)) {
		XR_OS_MSleep(20);
	}

	if (stop_connect_ap & SCA_STOP_CONNECTING_AP) {
		ret = SCA_CONNECT_STOP;
	} else if (nif && netif_is_up(nif) && netif_is_link_up(nif)) {
		ret = SCA_CONNECT_SUCCESS;
	} else if (XR_OS_TimeAfterEqual(XR_OS_GetTicks(), timeout)) {
		ret = SCA_CONNECT_TIMEOUT;
	}

	XR_OS_ThreadSuspendScheduler();
	stop_connect_ap = 0;
	XR_OS_ThreadResumeScheduler();
	return ret;

err:
	SCA_LOGE("connect ap failed\n");
	return ret;
}

static void __sc_assistant_stop_connect_ap(void)
{
	XR_OS_ThreadSuspendScheduler();
	stop_connect_ap |= SCA_STOP_CONNECTING_AP;
	XR_OS_ThreadResumeScheduler();
	while (stop_connect_ap & SCA_CONNECTING_AP)
		XR_OS_MSleep(5);

	//todo: delete it
	XR_OS_ThreadSuspendScheduler();
	stop_connect_ap = 0;
	XR_OS_ThreadResumeScheduler();
}

static uint8_t __sc_assistant_is_connecting_ap(void)
{
	if (stop_connect_ap & SCA_CONNECTING_AP)
		return 1;
	else
		return 0;
}

static int32_t __sc_assistant_get_ip(char *ip_str, const char *ifname)
{
	struct netif *nif = g_wlan_netif;

	if (nif && netif_is_up(nif) && netif_is_link_up(nif)) {
		strncpy(ip_str, inet_ntoa(nif->ip_addr), 16); /* add '\0' */
		return 0;
	}

	return -1;
}

void sc_assistant_get_fun(sc_assistant_fun_t *fun)
{
	if (!fun)
		return ;

	fun->get_mac = __sc_assistant_get_mac;
	fun->open_monitor = __sc_assistant_open_monitor;
	fun->close_monitor = __sc_assistant_close_monitor;
	fun->switch_channel = __sc_assistant_switch_channel;
	fun->send_raw_frame = __sc_assistant_send_raw_frame;
	fun->open_sta = __sc_assistant_open_sta;
	fun->connect_ap = __sc_assistant_connect_ap;
	fun->stop_connect_ap = __sc_assistant_stop_connect_ap;
	fun->is_connecting_ap = __sc_assistant_is_connecting_ap;
	fun->get_ip = __sc_assistant_get_ip;
}
