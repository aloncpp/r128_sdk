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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "sunxi_amp.h"
#include "lwip/err.h"
#include "hal_cache.h"

#include "net/wlan/wlan.h"
#include "net/wlan/wlan_ext_req.h"
#include "net/wpa_supplicant/wpa_ctrl_req.h"

#if (CONFIG_MBUF_IMPL_MODE == 0)
#include "sys/mbuf_0.h"
#endif

#ifdef CONFIG_ARCH_ARM_ARMV8M
static int _wpa_ctrl_request(enum wpa_ctrl_cmd cmd, void *data, int len, uint32_t flags)
{
	int ret;

	if (ISBUF(flags)) {
		if (len) {
			if (SER_CACHE_INVALID(flags)) {
				hal_dcache_invalidate((unsigned long)data, len);
			}
		}
		switch (cmd) {
		/* these case indicates that the 'data' passed in is a buf also contains
		 * another buf, and it needs to be read or written in ser */
		case WPA_CTRL_CMD_STA_SCAN_RESULTS: {
			wlan_sta_scan_results_t *p = (wlan_sta_scan_results_t *)data;
			hal_dcache_invalidate((unsigned long)p->ap, p->size * sizeof(wlan_sta_ap_t));
			break;
		}
		case WPA_CTRL_CMD_STA_BSS_GET: {
			wlan_sta_bss_info_t *p = (wlan_sta_bss_info_t *)data;
			hal_dcache_invalidate((unsigned long)p->bss, p->size);
			break;
		}
		case WPA_CTRL_CMD_STA_BSS_SET: {
			wlan_sta_bss_info_t *p = (wlan_sta_bss_info_t *)data;
			hal_dcache_invalidate((unsigned long)p->bss, p->size);
			break;
		}
		case WPA_CTRL_CMD_AP_STA_INFO: {
			wlan_ap_stas_t *p = (wlan_ap_stas_t *)data;
			hal_dcache_invalidate((unsigned long)p->sta, p->size * sizeof(wlan_ap_sta_t));
			break;
		}
		case WPA_CTRL_CMD_AP_SCAN_RESULTS: {
			wlan_sta_scan_results_t *p = (wlan_sta_scan_results_t *)data;
			hal_dcache_invalidate((unsigned long)p->ap, p->size * sizeof(wlan_sta_ap_t));
			break;
		}
		default:
			break;
		}
	}

	ret = wpa_ctrl_request(cmd, data, len, flags);

	if (ISBUF(flags)) {
		switch (cmd) {
		/* these case indicates that the 'data' passed in is a buf also contains
		 * another buf, and it needs to be read or written in ser */
		case WPA_CTRL_CMD_STA_SCAN_RESULTS: {
			wlan_sta_scan_results_t *p = (wlan_sta_scan_results_t *)data;
			hal_dcache_clean((unsigned long)p->ap, p->size * sizeof(wlan_sta_ap_t));
			break;
		}
		case WPA_CTRL_CMD_STA_BSS_GET: {
			wlan_sta_bss_info_t *p = (wlan_sta_bss_info_t *)data;
			hal_dcache_clean((unsigned long)p->bss, p->size);
			break;
		}
		case WPA_CTRL_CMD_STA_BSS_SET: {
			break;
		}
		case WPA_CTRL_CMD_AP_STA_INFO: {
			wlan_ap_stas_t *p = (wlan_ap_stas_t *)data;
			hal_dcache_clean((unsigned long)p->sta, p->size * sizeof(wlan_ap_sta_t));
			break;
		}
		case WPA_CTRL_CMD_AP_SCAN_RESULTS: {
			wlan_sta_scan_results_t *p = (wlan_sta_scan_results_t *)data;
			hal_dcache_clean((unsigned long)p->ap, p->size * sizeof(wlan_sta_ap_t));
			break;
		}
		default:
			break;
		}
		if (len) {
			/* if ser has written the buf, we need cache clean it;
			 * if ser only read this buf, cacheline dirty bit is 0,
			 * nothing need to do */
			if (SER_CACHE_CLEAN(flags)) {
				hal_dcache_clean((unsigned long)data, len);
			}
		}
	}

	return ret;
}

#if (defined(CONFIG_WLAN_STA) && defined(CONFIG_WLAN_AP))
static int _wlan_start(struct netif *nif)
{
	/* ser will not use 'nif' data, and there is no need
	 * to invalidate 'nif' cache */
	return wlan_start(nif);
}

static int _wlan_stop(struct netif *nif)
{
	/* ser will not use 'nif' data, and there is no need
	 * to invalidate 'nif' cache */
	return wlan_stop(nif);
}
#elif (defined(CONFIG_WLAN_STA))
static int _wlan_start_sta(struct netif *nif)
{
	/* ser will not use 'nif' data, and there is no need
	 * to invalidate 'nif' cache */
	return wlan_start_sta(nif);
}

static int _wlan_stop_sta(struct netif *nif)
{
	/* ser will not use 'nif' data, and there is no need
	 * to invalidate 'nif' cache */
	return wlan_stop_sta(nif);
}
#elif (defined(CONFIG_WLAN_AP))
static int _wlan_start_hostap(struct netif *nif)
{
	/* ser will not use 'nif' data, and there is no need
	 * to invalidate 'nif' cache */
	return wlan_start_hostap(nif);
}

static int _wlan_stop_hostap(struct netif *nif)
{
	/* ser will not use 'nif' data, and there is no need
	 * to invalidate 'nif' cache */
	return wlan_stop_hostap(nif);
}
#endif

static int _wlan_get_mac_addr(struct netif *nif, uint8_t *buf, int buf_len)
{
	int ret = -1;

	/* ser will not use 'nif' data, and there is no need
	 * to invalidate 'nif' cache */
	hal_dcache_invalidate((unsigned long)buf, buf_len);

	ret = wlan_get_mac_addr(nif, buf, buf_len);

	hal_dcache_clean((unsigned long)buf, buf_len);

	return ret;
}

static int _wlan_set_mac_addr(struct netif *nif, uint8_t *mac_addr, int mac_len)
{
	/* ser will not use 'nif' data, and there is no need
	 * to invalidate 'nif' cache */
	hal_dcache_invalidate((unsigned long)mac_addr, mac_len);

	/* 'mac_addr' buf is only read by ser, cacheline dirty bit is 0,
	 * nothing need to do after use */
	return wlan_set_mac_addr(nif, mac_addr, mac_len);
}

static int _wlan_set_ip_addr(struct netif *nif, uint8_t *ip_addr, int ip_len)
{
	/* ser will not use 'nif' data, and there is no need
	 * to invalidate 'nif' cache */
	hal_dcache_invalidate((unsigned long)ip_addr, ip_len);

	/* 'ip_addr' buf is only read by ser, cacheline dirty bit is 0,
	 * nothing need to do after use */
	return wlan_set_ip_addr(nif, ip_addr, ip_len);
}

static int _wlan_set_ps_mode(struct netif *nif, int mode)
{
	/* ser will not use 'nif' data, and there is no need
	 * to invalidate 'nif' cache */
	return wlan_set_ps_mode(nif, mode);
}

static int _wlan_set_appie(struct netif *nif, uint8_t type, uint8_t *ie, uint16_t ie_len)
{
	int ret = -1;

	/* ser will not use 'nif' data, and there is no need
	 * to invalidate 'nif' cache */
	hal_dcache_invalidate((unsigned long)ie, ie_len);

	ret = wlan_set_appie(nif, type, ie, ie_len);

	/* todo: check if need */
	hal_dcache_clean_invalidate((unsigned long)ie, ie_len);

	return ret;
}

extern int wlan_set_channel(struct netif *nif, int16_t channel);
extern void net80211_monitor_enable_rx(int enable);
extern int net80211_mail_init(void);
extern void net80211_mail_deinit(void);
extern int net80211_ifnet_setup(void);
extern int net80211_ifnet_release(void);
extern int xradio_drv_cmd(int argc, const char **arg);

static int _wlan_set_channel(struct netif *nif, int16_t channel)
{
	/* ser will not use 'nif' data, and there is no need
	 * to invalidate 'nif' cache */
	return wlan_set_channel(nif, channel);
}

static int _net80211_monitor_enable_rx(int enable)
{
	net80211_monitor_enable_rx(enable);
	return 0;
}

static int _net80211_mail_init(void)
{
	return net80211_mail_init();
}

static int _net80211_mail_deinit(void)
{
	net80211_mail_deinit();
	return 0;
}

static int _net80211_ifnet_setup(void)
{
	return net80211_ifnet_setup();
}

static int _net80211_ifnet_release(void)
{
	return net80211_ifnet_release();
}

static int _wlan_send_raw_frame(struct netif *netif, int type, uint8_t *buffer, int len)
{
	/* ser will not use 'netif' data, and there is no need
	 * to invalidate 'netif' cache */
	hal_dcache_invalidate((unsigned long)buffer, len);

	/* 'buffer' is only read by ser, cacheline dirty bit is 0,
	 * nothing need to do after use */
	return wlan_send_raw_frame(netif, type, buffer, len);
}

static void *_wlan_if_create(enum wlan_mode mode, struct netif *nif, const char *name, int len)
{
	/* do not use strlen(name) to get len, because in ser,
	 * the name buffer may be stored in cache rather than from stub */
	hal_dcache_invalidate((unsigned long)name, len);

	/* 'name' is only read by ser, cacheline dirty bit is 0,
	 * nothing need to do after use */
	return wlan_if_create(mode, nif, name);
}

static int _wlan_if_delete(void *ifp)
{
	/* stub do not modify 'ifp', so ser needn't to
	 * invalid 'ifp' cache */
	return wlan_if_delete(ifp);
}

static int _wlan_linkoutput(struct netif *nif, struct mbuf *m)
{
	struct mbuf *_m = m;

	/* ser will not use 'nif' data, and there is no need
	 * to invalidate 'nif' cache */
	hal_dcache_invalidate((unsigned long)m, sizeof(struct mbuf));

	while (_m) {
		hal_dcache_invalidate((unsigned long)_m, sizeof(struct mbuf) + _m->m_len + _m->m_headspace + _m->m_tailspace);
		_m = m->m_nextpkt;
	}

	/* '_m' may will be used(written/read) for a long time by ser,
	 * and released by ser(xradio_skb_dtor) itself.
	 * here only stub(RV) provides an rpc mbuf for ser(M33), stub(RV) will
	 * never use this mbuf after return, so, in order not to reduce WLAN TX
	 * performance, we needn't to cache clean it to stub(RV) */
	return wlan_linkoutput(nif, m);
}

static int _wlan_ext_request(struct netif *nif, wlan_ext_cmd_t cmd, uint32_t param, uint32_t len, uint32_t flags)
{
	int ret;

	if (ISBUF(flags)) {
		if (len) {
			if (SER_CACHE_INVALID(flags)) {
				hal_dcache_invalidate((unsigned long)param, len);
			}
		}
		switch (cmd) {
		case WLAN_EXT_CMD_SET_SCAN_FREQ: {
			wlan_ext_scan_freq_t *p = (wlan_ext_scan_freq_t *)param;
			hal_dcache_invalidate((unsigned long)p->freq_list, sizeof(int32_t) * p->freq_num);
			break;
		}
		default:
			break;
		}
	}

	ret = wlan_ext_request(nif, cmd, param);

	if (ISBUF(flags)) {
		if (len) {
			/* if ser has written the buf, we need cache clean it;
			 * if ser only has read this buf, cacheline dirty bit is 0,
			 * nothing need to do */
			if (SER_CACHE_CLEAN(flags)) {
				hal_dcache_clean((unsigned long)param, len);
			}
		}
	}

	return ret;
}

static int _wlan_ext_low_power_param_set_default(struct netif *nif, uint32_t dtim)
{
	/* ser will not use 'nif' data, and there is no need
	 * to invalidate 'nif' cache */
	return wlan_ext_low_power_param_set_default(nif, dtim);
}

static int _xradio_drv_cmd(int argc, char *argv[])
{
	hal_dcache_invalidate((unsigned long)argv, argc * sizeof(char *));
	hal_dcache_invalidate((unsigned long)(argv[0]), (argv[argc - 1] - argv[0] + strlen(argv[argc - 1]) + 1));

	/* 'argv' is const, ser only has read this buf, cacheline dirty bit is 0,
	 * nothing need to do after use */
	return xradio_drv_cmd(argc, (const char **)argv);
}

static struct mbuf *_mb_get(int len, int tx)
{
	struct mbuf *m = mb_get(len, tx);

	if (m) {
		int32_t tot_len = sizeof(struct mbuf) + len;
		if (tx) {
			tot_len += MBUF_HEAD_SPACE + MBUF_TAIL_SPACE;
		}
		hal_dcache_clean((unsigned long)m, tot_len);
	}

	return m;
}

static int _mb_free(struct mbuf *m)
{
	hal_dcache_invalidate((unsigned long)m, sizeof(struct mbuf));
	hal_dcache_invalidate((unsigned long)m, sizeof(struct mbuf) + m->m_len + m->m_headspace);
	/* mbuf 'm' never used by stub free, nothing need to do after return */
	mb_free(m);

	return 0;
}

sunxi_amp_func_table net_table[] = {
	{.func = (void *)&_wpa_ctrl_request, .args_num = 4, .return_type = RET_POINTER},
#if (defined(CONFIG_WLAN_STA) && defined(CONFIG_WLAN_AP))
	{.func = (void *)&_wlan_start, .args_num = 1, .return_type = RET_POINTER},
	{.func = (void *)&_wlan_stop, .args_num = 1, .return_type = RET_POINTER},
#elif (defined(CONFIG_WLAN_STA))
	{.func = (void *)&wlan_start_sta, .args_num = 1, .return_type = RET_POINTER},
	{.func = (void *)&wlan_stop_sta, .args_num = 1, .return_type = RET_POINTER},
#elif (defined(CONFIG_WLAN_AP))
	{.func = (void *)&wlan_start_hostap, .args_num = 1, .return_type = RET_POINTER},
	{.func = (void *)&wlan_stop_hostap, .args_num = 1, .return_type = RET_POINTER},
#endif
	{.func = (void *)&_wlan_get_mac_addr, .args_num = 3, .return_type = RET_POINTER},
	{.func = (void *)&_wlan_set_mac_addr, .args_num = 3, .return_type = RET_POINTER},
	{.func = (void *)&_wlan_set_ip_addr, .args_num = 3, .return_type = RET_POINTER},
	{.func = (void *)&_wlan_set_ps_mode, .args_num = 2, .return_type = RET_POINTER},
	{.func = (void *)&_wlan_set_appie, .args_num = 4, .return_type = RET_POINTER},
	{.func = (void *)&_wlan_set_channel, .args_num = 2, .return_type = RET_POINTER},
	{.func = (void *)&_net80211_mail_init, .args_num = 0, .return_type = RET_POINTER},
	{.func = (void *)&_net80211_mail_deinit, .args_num = 0, .return_type = RET_POINTER},
	{.func = (void *)&_net80211_ifnet_setup, .args_num = 0, .return_type = RET_POINTER},
	{.func = (void *)&_net80211_ifnet_release, .args_num = 0, .return_type = RET_POINTER},
	{.func = (void *)&_net80211_monitor_enable_rx, .args_num = 1, .return_type = RET_POINTER},
	{.func = (void *)&_wlan_send_raw_frame, .args_num = 4, .return_type = RET_POINTER},
	{.func = (void *)&_wlan_if_create, .args_num = 4, .return_type = RET_POINTER},
	{.func = (void *)&_wlan_if_delete, .args_num = 1, .return_type = RET_POINTER},
	{.func = (void *)&_wlan_linkoutput, .args_num = 2, .return_type = RET_POINTER},
	{.func = (void *)&_wlan_ext_request, .args_num = 5, .return_type = RET_POINTER},
	{.func = (void *)&_wlan_ext_low_power_param_set_default, .args_num = 2, .return_type = RET_POINTER},
	{.func = (void *)&_xradio_drv_cmd, .args_num = 2, .return_type = RET_POINTER},
	{.func = (void *)&_mb_get, .args_num = 2, .return_type = RET_POINTER},
	{.func = (void *)&_mb_free, .args_num = 1, .return_type = RET_POINTER},
};
#elif (defined CONFIG_ARCH_RISCV_RV64)
static const wlan_ap_default_conf_t *_wlan_ap_get_default_conf(void)
{
	const wlan_ap_default_conf_t *cfg = NULL;

	cfg = wlan_ap_get_default_conf();
	hal_dcache_clean((unsigned long)cfg, sizeof(wlan_ap_default_conf_t));

	return cfg;
}

static int _wlan_event_notify(uint32_t param0, uint32_t param1)
{
	return wlan_event_notify(param0, param1);
}

static int _wlan_monitor_input(struct netif *nif, uint8_t *data, uint32_t len, void *info)
{
	int ret = -1;

	/* ser do not modify 'nif' data, so stub needn't to invalid 'nif' */
	hal_dcache_invalidate((unsigned long)data, len);
	hal_dcache_invalidate((unsigned long)info, sizeof(struct frame_info));

	ret = wlan_monitor_input(nif, data, len, info);

	/* 'data' and 'info' only use in ser(RV), and will be amp_align_free after
	 * return in stub(M33) RPC call internal. However, due to the uncertainty
	 * of the m_wlan_monitor_rx_cb() config by user, in order to prevent the
	 * customer writing this buf, we direct invalidate it when the stub(M33)
	 * does not need to use this buf after return */
	hal_dcache_invalidate((unsigned long)data, len);
	hal_dcache_invalidate((unsigned long)info, sizeof(struct frame_info));

	return ret;
}

#if BYTE_ORDER == LITTLE_ENDIAN
static u16_t _lwip_htons(u16_t n)
{
	return lwip_htons(n);
}
#endif /* lwip_htons */

static err_t _ethernetif_raw_input(struct netif *nif, uint8_t *data, u16_t len)
{
	/* ser do not modify 'nif' data, so stub needn't to invalid 'nif' */
	hal_dcache_invalidate((unsigned long)data, len);

	/* ser(RV) has only read and copy 'data', ser(RV) cacheline dirty bit is 0
	 * in ser, nothing need to do after return */
	return ethernetif_raw_input(nif, data, len);
}

static enum wlan_mode _ethernetif_get_mode(struct netif *nif)
{
	/* ser do not modify 'nif' data, so stub needn't to invalid 'nif' */
	return ethernetif_get_mode(nif);
}

static void *_ethernetif_get_state(struct netif *nif)
{
	/* ser do not modify 'nif' data, so stub needn't to invalid 'nif' */
	return ethernetif_get_state(nif);
}

static int _wlan_ext_temp_volt_event_input(wlan_ext_temp_volt_event_data_t *data)
{
	int ret = -1;
	uint32_t len = sizeof(wlan_ext_temp_volt_event_data_t);

	hal_dcache_invalidate((unsigned long)data, len);
	ret = wlan_ext_temp_volt_event_input(data);
	/* in order to prevent the customer writing this buf, we direct invalidate it
	 * when the stub(M33) does not need to use this buf after return */
	hal_dcache_invalidate((unsigned long)data, len);

	return ret;
}

sunxi_amp_func_table net_table[] = {
	{.func = (void *)&_wlan_ap_get_default_conf, .args_num = 0, .return_type = RET_POINTER},
	{.func = (void *)&_wlan_event_notify, .args_num = 2, .return_type = RET_POINTER},
	{.func = (void *)&_wlan_monitor_input, .args_num = 4, .return_type = RET_POINTER},
	{.func = (void *)&_lwip_htons, .args_num = 1, .return_type = RET_POINTER},
	{.func = (void *)&_ethernetif_raw_input, .args_num = 3, .return_type = RET_POINTER},
	{.func = (void *)&_ethernetif_get_mode, .args_num = 1, .return_type = RET_POINTER},
	{.func = (void *)&_ethernetif_get_state, .args_num = 1, .return_type = RET_POINTER},
	{.func = (void *)&_wlan_ext_temp_volt_event_input, .args_num = 1, .return_type = RET_POINTER},
};
#endif
