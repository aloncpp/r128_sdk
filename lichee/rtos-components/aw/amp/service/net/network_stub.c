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
#include <reent.h>

#include "sunxi_amp.h"
#include <console.h>
#include "hal_cache.h"

#include "net/wlan/wlan.h"
#include "net/wlan/wlan_ext_req.h"
#include "net/wpa_supplicant/wpa_ctrl_req.h"
#include "sys/mbuf.h"

#ifdef CONFIG_ARCH_RISCV_RV64
int wpa_ctrl_request(enum wpa_ctrl_cmd cmd, void *data, int len, uint32_t flags)
{
	int ret;
	void *args[4] = {0};
	char *buf_ali = data;

	/* if args 'data' is a buf pointer, it should be amp align process,
	 * that is, use 'buf_ali' replaces 'data' as buf */
	if (ISBUF(flags)) {
		if (len) {
			buf_ali = amp_align_malloc(len);
			if (!buf_ali) {
				return -1;
			}
			/* if the 'data' buf need to be read in ser, we should
			 * get the real value from 'data' and clean cache in stub */
			if (STUB_CACHE_CLEAN(flags)) {
				memcpy(buf_ali, data, len);
				hal_dcache_clean((unsigned long)buf_ali, len);
			} else {
				/* note: the purpose of zeroing and cache clean is used to
				 * be compatible with the scenarios of bit width problems
				 * caused by WPA_CTRL_CMD_STA_STATE case */
				memset(buf_ali, 0, len);
				hal_dcache_clean((unsigned long)buf_ali, len);
			}
		}
	}

	/* RPC call to ser */
	args[0] = (void *)cmd;
	args[1] = buf_ali;
	args[2] = (void *)(unsigned long)len;
	args[3] = (void *)(unsigned long)flags;
	ret = func_stub(RPCCALL_NET(wpa_ctrl_request), 1, ARRAY_SIZE(args), args);

	/* if args 'data' is a buf pointer, free the buf_ali tmp buffer */
	if (ISBUF(flags)) {
		if (len) {
			/* if the 'data' buf has been written in ser, we should
			 * invalid our 'buf_ali' cache and copy the real value
			 * form 'buf_ali' in stub */
			if (STUB_CACHE_INVALID(flags)) {
				hal_dcache_invalidate((unsigned long)buf_ali, len);
				memcpy(data, buf_ali, len);
			}
			amp_align_free(buf_ali);
		}
	}

	return ret;
}

int wlan_start(struct netif *nif)
{
	void *args[1] = {0};

	/* ser do not use 'nif' data, so there is no need
	 * to flush 'nif' cache in stub */
	args[0] = nif;
	return func_stub(RPCCALL_NET(wlan_start), 1, ARRAY_SIZE(args), args);
}

int wlan_stop(struct netif *nif)
{
	void *args[1] = {0};

	/* ser do not use 'nif' data, so there is no need
	 * to flush 'nif' cache in stub */
	args[0] = nif;
	return func_stub(RPCCALL_NET(wlan_stop), 1, ARRAY_SIZE(args), args);
}

int wlan_get_mac_addr(struct netif *nif, uint8_t *buf, int buf_len)
{
	int ret = -1;
	char *buf_ali;
	void *args[3] = {0};
	buf_ali = amp_align_malloc(buf_len);
	if (!buf_ali) {
		return -1;
	}

	/* ser do not use 'nif' data, so there is no need
	 * to flush 'nif' cache in stub */
	args[0] = nif;
	args[1] = buf_ali;
	args[2] = (void *)(unsigned long)buf_len;

	/* when the stub does not use this buf, clear the dirty bit of the cacheline
	 * to prevent the stub from mistakenly flush the cache when the ser operates
	 * the buf again */
	hal_dcache_invalidate((unsigned long)buf_ali, buf_len);
	ret = func_stub(RPCCALL_NET(wlan_get_mac_addr), 1, ARRAY_SIZE(args), args);

	hal_dcache_invalidate((unsigned long)buf_ali, buf_len);
	memcpy(buf, buf_ali, buf_len);
	amp_align_free(buf_ali);

	return ret;
}

int wlan_set_mac_addr(struct netif *nif, uint8_t *mac_addr, int mac_len)
{
	int ret = -1;
	void *args[3] = {0};
	char *buf_ali;

	buf_ali = amp_align_malloc(mac_len);
	if (!buf_ali) {
		return -1;
	}
	memcpy(buf_ali, mac_addr, mac_len);
	hal_dcache_clean((unsigned long)buf_ali, mac_len);

	/* ser do not use 'nif' data, so there is no need
	 * to flush 'nif' cache in stub */
	args[0] = nif;
	args[1] = buf_ali;
	args[2] = (void *)(unsigned long)mac_len;
	ret = func_stub(RPCCALL_NET(wlan_set_mac_addr), 1, ARRAY_SIZE(args), args);

	amp_align_free(buf_ali);

	return ret;
}

int wlan_set_ip_addr(struct netif *nif, uint8_t *ip_addr, int ip_len)
{
	int ret = -1;
	void *args[3] = {0};
	char *buf_ali;

	buf_ali = amp_align_malloc(ip_len);
	if (!buf_ali) {
		return -1;
	}
	memcpy(buf_ali, ip_addr, ip_len);
	hal_dcache_clean((unsigned long)buf_ali, ip_len);

	/* ser do not use 'nif' data, so there is no need
	 * to flush 'nif' cache in stub */
	args[0] = nif;
	args[1] = buf_ali;
	args[2] = (void *)(unsigned long)ip_len;
	ret = func_stub(RPCCALL_NET(wlan_set_ip_addr), 1, ARRAY_SIZE(args), args);

	amp_align_free(buf_ali);

	return ret;
}

int wlan_set_ps_mode(struct netif *nif, int mode)
{
	void *args[2] = {0};

	/* ser do not use 'nif' data, so there is no need
	 * to flush 'nif' cache in stub */
	args[0] = nif;
	args[1] = (void *)(unsigned long)mode;
	return func_stub(RPCCALL_NET(wlan_set_ps_mode), 1, ARRAY_SIZE(args), args);
}

int wlan_set_appie(struct netif *nif, uint8_t type, uint8_t *ie, uint16_t ie_len)
{
	int ret = -1;
	void *args[4] = {0};
	char *buf_ali;

	buf_ali = amp_align_malloc(ie_len);
	if (!buf_ali) {
		return -1;
	}
	memcpy(buf_ali, ie, ie_len);
	hal_dcache_clean((unsigned long)buf_ali, ie_len);

	/* ser do not use 'nif' data, so there is no need
	 * to flush 'nif' cache in stub */
	args[0] = nif;
	args[1] = (void *)(unsigned long)type;
	args[2] = buf_ali;
	args[3] = (void *)(unsigned long)ie_len;
	ret = func_stub(RPCCALL_NET(wlan_set_appie), 1, ARRAY_SIZE(args), args);

	amp_align_free(buf_ali);

	return ret;
}

int net80211_monitor_enable_rx(int enable)
{
	void *args[1] = {0};

	args[0] = (void *)(unsigned long)enable;
	return func_stub(RPCCALL_NET(net80211_monitor_enable_rx), 1, ARRAY_SIZE(args), args);
}

int wlan_set_channel(struct netif *nif, int16_t channel)
{
	void *args[2] = {0};

	/* ser do not use 'nif' data, so there is no need
	 * to flush 'nif' cache in stub */
	args[0] = nif;
	args[1] = (void *)(unsigned long)channel;
	return func_stub(RPCCALL_NET(wlan_set_channel), 1, ARRAY_SIZE(args), args);
}

int net80211_mail_init(void)
{
	return func_stub(RPCCALL_NET(net80211_mail_init), 1, 0, NULL);
}

int net80211_mail_deinit(void)
{
	return func_stub(RPCCALL_NET(net80211_mail_deinit), 1, 0, NULL);
}

int net80211_ifnet_setup(void)
{
	return func_stub(RPCCALL_NET(net80211_ifnet_setup), 1, 0, NULL);
}

int net80211_ifnet_release(void)
{
	return func_stub(RPCCALL_NET(net80211_ifnet_release), 1, 0, NULL);
}

int wlan_send_raw_frame(struct netif *netif, int type, uint8_t *buffer, int len)
{
	int ret = -1;
	void *args[4] = {0};
	char *buf_ali;

	buf_ali = amp_align_malloc(len);
	if (!buf_ali) {
		return -1;
	}
	memcpy(buf_ali, buffer, len);
	hal_dcache_clean((unsigned long)buf_ali, len);

	/* ser do not use 'netif' data, so there is no need
	 * to flush 'netif' cache in stub */
	args[0] = netif;
	args[1] = (void *)(unsigned long)type;
	args[2] = buf_ali;
	args[3] = (void *)(unsigned long)len;
	ret = func_stub(RPCCALL_NET(wlan_send_raw_frame), 1, ARRAY_SIZE(args), args);

	amp_align_free(buf_ali);

	return ret;
}

void *wlan_if_create(enum wlan_mode mode, struct netif *nif, const char *name)
{
	void * ret;
	void *args[4] = {0};
	char *buf_ali;
	int len = strlen(name) + 1;

	buf_ali = amp_align_malloc(len);
	if (!buf_ali) {
		return NULL;
	}
	memcpy(buf_ali, name, len);
	hal_dcache_clean((unsigned long)buf_ali, len);

	/* ser do not use 'nif' data, so there is no need
	 * to flush 'nif' cache in stub */
	args[0] = (void *)mode;
	args[1] = nif;
	args[2] = buf_ali;
	args[3] = (void *)(uintptr_t) len;
	ret = (void *)func_stub(RPCCALL_NET(wlan_if_create), 1, ARRAY_SIZE(args), args);

	amp_align_free(buf_ali);

	return ret;
}

int wlan_if_delete(void *ifp)
{
	void *args[1] = {0};

	/* RV do not modify 'ifp', M33 only use its own 'ifp',
	 * so RV needn't to flush cache */
	args[0] = ifp;
	return func_stub(RPCCALL_NET(wlan_if_delete), 1, ARRAY_SIZE(args), args);
}

int wlan_linkoutput(struct netif *nif, struct mbuf *m)
{
	struct mbuf *_m = m;
	void *args[2] = {0};

	/* note: mbuf has been cacheline algin in mb_get/mb_free
	 * so, nothing need to do here */
	while (_m) {
		hal_dcache_clean((unsigned long)_m, sizeof(struct mbuf) + _m->m_len + _m->m_headspace + _m->m_tailspace);
		_m = m->m_nextpkt;
	}

	/* ser do not use 'nif' data, so there is no need
	 * to flush 'nif' cache in stub */
	args[0] = nif;
	args[1] = m;
	return func_stub(RPCCALL_NET(wlan_linkoutput), 1, ARRAY_SIZE(args), args);
}

int wlan_ext_request(struct netif *nif, wlan_ext_cmd_t cmd, uint32_t param)
{
	int ret;
	void *args[5] = {0};
	char *buf_ali = (char *)(unsigned long)param;
	char *buf_ali_freq;
	void *freq;
	uint32_t flags, len;

	switch (cmd) {
	case WLAN_EXT_CMD_GET_PM_DTIM: {
		len = sizeof(wlan_ext_pm_dtim_t);
		flags = FLAG_IS_BUF | FLAG_SER_WR;
		break;
	}
	case WLAN_EXT_CMD_SET_PS_CFG: {
		len = sizeof(wlan_ext_ps_cfg_t);
		flags = FLAG_IS_BUF | FLAG_SER_RD;
		break;
	}
	case WLAN_EXT_CMD_GET_BCN_STATUS: {
		len = sizeof(wlan_ext_bcn_status_t);
		flags = FLAG_IS_BUF | FLAG_SER_WR;
		break;
	}
	case WLAN_EXT_CMD_SET_PHY_PARAM: {
		len = sizeof(wlan_ext_phy_param_t);
		flags = FLAG_IS_BUF | FLAG_SER_RD;
		break;
	}
	case WLAN_EXT_CMD_SET_SCAN_PARAM: {
		len = sizeof(wlan_ext_scan_param_t);
		flags = FLAG_IS_BUF | FLAG_SER_RD;
		break;
	}
	case WLAN_EXT_CMD_SET_AUTO_SCAN: {
		len = sizeof(wlan_ext_auto_scan_param_t);
		flags = FLAG_IS_BUF | FLAG_SER_RD;
		break;
	}
	case WLAN_EXT_CMD_SET_P2P_SVR: {
		len = sizeof(wlan_ext_p2p_svr_set_t);
		flags = FLAG_IS_BUF | FLAG_SER_RD;
		break;
	}
	case WLAN_EXT_CMD_SET_P2P_WKP_CFG: {
		len = sizeof(wlan_ext_p2p_wkp_param_set_t);
		flags = FLAG_IS_BUF | FLAG_SER_RD;
		break;
	}
	case WLAN_EXT_CMD_SET_P2P_KPALIVE_CFG: {
		len = sizeof(wlan_ext_p2p_kpalive_param_set_t);
		flags = FLAG_IS_BUF | FLAG_SER_RD;
		break;
	}
	case WLAN_EXT_CMD_SET_BCN_WIN_CFG: {
		len = sizeof(wlan_ext_bcn_win_param_set_t);
		flags = FLAG_IS_BUF | FLAG_SER_RD;
		break;
	}
	case WLAN_EXT_CMD_SET_FORCE_B_RATE: {
		len = sizeof(wlan_ext_force_b_rate_set_t);
		flags = FLAG_IS_BUF | FLAG_SER_RD;
		break;
	}
	case WLAN_EXT_CMD_SET_RCV_SPECIAL_FRM: {
		len = sizeof(wlan_ext_rcv_spec_frm_param_set_t);
		flags = FLAG_IS_BUF | FLAG_SER_RD;
		break;
	}
	case WLAN_EXT_CMD_SET_SEND_RAW_FRM_CFG: {
		len = sizeof(wlan_ext_send_raw_frm_param_set_t);
		flags = FLAG_IS_BUF | FLAG_SER_RD;
		break;
	}
	case WLAN_EXT_CMD_SET_SNIFF_SYNC_CFG: {
		len = sizeof(wlan_ext_sniff_sync_param_set_t);
		flags = FLAG_IS_BUF | FLAG_SER_RD;
		break;
	}
	case WLAN_EXT_CMD_SET_RCV_FRM_FILTER_CFG: {
		len = sizeof(rcv_frm_filter_t);
		flags = FLAG_IS_BUF | FLAG_SER_RD;
		break;
	}
	case WLAN_EXT_CMD_SET_POWER_LEVEL_TAB: {
		len = sizeof(wlan_ext_power_level_tab_set_t);
		flags = FLAG_IS_BUF | FLAG_SER_RD;
		break;
	}
	case WLAN_EXT_CMD_GET_POWER_LEVEL_TAB: {
		len = sizeof(wlan_ext_power_level_tab_get_t);
		flags = FLAG_IS_BUF | FLAG_SER_RD | FLAG_SER_WR;
		break;
	}
	case WLAN_EXT_CMD_SET_SWITCH_CHN_CFG: {
		len = sizeof(wlan_ext_switch_channel_param_set_t);
		flags = FLAG_IS_BUF | FLAG_SER_RD;
		break;
	}
	case WLAN_EXT_CMD_GET_CURRENT_CHN: {
		len = sizeof(int);
		flags = FLAG_IS_BUF | FLAG_SER_WR;
		break;
	}
	case WLAN_EXT_CMD_SET_SNIFF_KP_ACTIVE: {
		len = sizeof(wlan_ext_sniff_kp_active_set_t);
		flags = FLAG_IS_BUF | FLAG_SER_RD;
		break;
	}
	case WLAN_EXT_CMD_SET_FRM_FILTER: {
		len = sizeof(wlan_ext_frm_filter_set_t);
		flags = FLAG_IS_BUF | FLAG_SER_RD;
		break;
	}
	case WLAN_EXT_CMD_SET_TEMP_FRM: {
		len = sizeof(wlan_ext_temp_frm_set_t);
		flags = FLAG_IS_BUF | FLAG_SER_RD;
		break;
	}
	case WLAN_EXT_CMD_SET_UPDATE_TEMP_IE: {
		len = sizeof(uint8_t) * IE_NUM;
		flags = FLAG_IS_BUF | FLAG_SER_RD;
		break;
	}
	case WLAN_EXT_CMD_SET_SYNC_FRM_SEND: {
		len = sizeof(wlan_ext_send_sync_frm_set_t);
		flags = FLAG_IS_BUF | FLAG_SER_RD;
		break;
	}
	case WLAN_EXT_CMD_SET_SNIFF_EXTERN_CFG: {
		len = sizeof(wlan_ext_sniff_extern_param_set_t);
		flags = FLAG_IS_BUF | FLAG_SER_RD;
		break;
	}
	case WLAN_EXT_CMD_GET_SNIFF_STAT: {
		len = sizeof(wlan_ext_bcn_status_t);
		flags = FLAG_IS_BUF | FLAG_SER_WR;
		break;
	}
	case WLAN_EXT_CMD_GET_TEMP_VOLT: {
		len = sizeof(wlan_ext_temp_volt_get_t);
		flags = FLAG_IS_BUF | FLAG_SER_WR;
		break;
	}
	case WLAN_EXT_CMD_SET_CHANNEL_FEC: {
		len = sizeof(wlan_ext_channel_fec_set_t);
		flags = FLAG_IS_BUF | FLAG_SER_RD;
		break;
	}
	case WLAN_EXT_CMD_SET_TEMP_VOLT_AUTO_UPLOAD: {
		len = sizeof(wlan_ext_temp_volt_auto_upload_set_t);
		flags = FLAG_IS_BUF | FLAG_SER_RD;
		break;
	}
	case WLAN_EXT_CMD_SET_TEMP_VOLT_THRESH: {
		len = sizeof(wlan_ext_temp_volt_thresh_set_t);
		flags = FLAG_IS_BUF | FLAG_SER_RD;
		break;
	}
	case WLAN_EXT_CMD_SET_EDCA_PARAM: {
		len = sizeof(wlan_ext_edca_param_t);
		flags = FLAG_IS_BUF | FLAG_SER_RD;
		break;
	}
	case WLAN_EXT_CMD_GET_EDCA_PARAM: {
		len = sizeof(wlan_ext_edca_param_t);
		flags = FLAG_IS_BUF | FLAG_SER_WR;
		break;
	}
	case WLAN_EXT_CMD_SET_LFCLOCK_PARAM: {
		len = sizeof(wlan_ext_lfclock_param_t);
		flags = FLAG_IS_BUF | FLAG_SER_RD;
		break;
	}
	case WLAN_EXT_CMD_SET_SDD_FREQ_OFFSET: {
		len = sizeof(uint32_t);
		flags = FLAG_IS_BUF | FLAG_SER_RD;
		break;
	}
	case WLAN_EXT_CMD_GET_SDD_FREQ_OFFSET: {
		len = sizeof(uint32_t);
		flags = FLAG_IS_BUF | FLAG_SER_WR;
		break;
	}
	case WLAN_EXT_CMD_SET_SDD_POWER: {
		len = sizeof(uint16_t) * SDD_IE_POWER_NUM;
		flags = FLAG_IS_BUF | FLAG_SER_RD;
		break;
	}
	case WLAN_EXT_CMD_GET_SDD_POWER: {
		len = sizeof(uint16_t) * SDD_IE_POWER_NUM;
		flags = FLAG_IS_BUF | FLAG_SER_WR;
		break;
	}
	case WLAN_EXT_CMD_GET_SDD_FILE: {
		len = sizeof(int);
		/* the param is a buf need to be read */
		flags = FLAG_IS_BUF | FLAG_SER_RD;
		break;
	}
	case WLAN_EXT_CMD_GET_TX_RATE: {
		len = sizeof(int);
		flags = FLAG_IS_BUF | FLAG_SER_WR;
		break;
	}
	case WLAN_EXT_CMD_GET_SIGNAL: {
		len = sizeof(wlan_ext_signal_t);
		flags = FLAG_IS_BUF | FLAG_SER_WR;
		break;
	}
	case WLAN_EXT_CMD_SET_MIXRATE: {
		len = sizeof(int);
		flags = FLAG_IS_BUF | FLAG_SER_RD;
		break;
	}
	case WLAN_EXT_CMD_SET_MBUF_LIMIT: {
		len = sizeof(wlan_ext_mbuf_limit_t);
		flags = FLAG_IS_BUF | FLAG_SER_RD;
		break;
	}
	case WLAN_EXT_CMD_SET_AMPDU_REORDER_AGE: {
		len = sizeof(uint16_t);
		flags = FLAG_IS_BUF | FLAG_SER_RD;
		break;
	}
	case WLAN_EXT_CMD_SET_SCAN_FREQ: {
		wlan_ext_scan_freq_t *p = (wlan_ext_scan_freq_t *)(unsigned long)param;
		freq = (void *)p->freq_list;
		if (p->freq_num) {
			buf_ali_freq = amp_align_malloc(p->freq_num * sizeof(int32_t));
			if (!buf_ali_freq) {
				return -1;
			}
			p->freq_list = (int32_t *)buf_ali_freq;
			memcpy(buf_ali_freq, freq, sizeof(int32_t) * p->freq_num);
			hal_dcache_clean((unsigned long)buf_ali_freq, sizeof(int32_t) * p->freq_num);
		}
		len = sizeof(wlan_ext_scan_freq_t);
		flags = FLAG_IS_BUF | FLAG_SER_RD;
		break;
	}
	case WLAN_EXT_CMD_SET_AMRR_PARAM: {
		len = sizeof(wlan_ext_amrr_param_t);
		flags = FLAG_IS_BUF | FLAG_SER_RD;
		break;
	}
	case WLAN_EXT_CMD_AUTO_BCN_OFFSET_SET: {
		len = sizeof(wlan_ext_bcn_auto_offset_t);
		flags = FLAG_IS_BUF | FLAG_SER_RD;
		break;
	}
	case WLAN_EXT_CMD_AUTO_BCN_OFFSET_READ: {
		len = sizeof(wlan_ext_bcn_auto_offset_t);
		flags = FLAG_IS_BUF | FLAG_SER_WR;
		break;
	}
	case WLAN_EXT_CMD_SET_SNIFFER: {
		len = sizeof(wlan_ext_sniffer_param_t);
		flags = FLAG_IS_BUF | FLAG_SER_RD;
		break;
	}
	case WLAN_EXT_CMD_SET_MIMO_PARAM: {
		len = sizeof(MIB_SET_RX_EXT_FRAME);
		flags = FLAG_IS_BUF | FLAG_SER_RD;
		break;
	}
	case WLAN_EXT_CMD_SET_PS_POLICY: {
		len = sizeof(wlan_ext_ps_policy_t);
		flags = FLAG_IS_BUF | FLAG_SER_RD;
		break;
	}
	case WLAN_EXT_CMD_SET_PRE_RX_BCN: {
		len = sizeof(wlan_ext_pre_rx_bcn_t);
		flags = FLAG_IS_BUF | FLAG_SER_RD;
		break;
	}
	case WLAN_EXT_CMD_SET_BCN_WITHOUT_DATA: {
		len = sizeof(wlan_ext_chk_bcn_without_data_set_t);
		flags = FLAG_IS_BUF | FLAG_SER_RD;
		break;
	}
	case WLAN_EXT_CMD_SET_BCN_TIM_NO_DATA_TMO: {
		len = sizeof(wlan_ext_bcn_tim_no_data_tmo_set_t);
		flags = FLAG_IS_BUF | FLAG_SER_RD;
		break;
	}
	case WLAN_EXT_CMD_GET_BCN_RSSI_DBM: {
		len = sizeof(wlan_ext_bcn_rssi_dbm_get_t);
		flags = FLAG_IS_BUF | FLAG_SER_WR;
		break;
	}
	case WLAN_EXT_CMD_GET_STATS_CODE: {
		len = sizeof(wlan_ext_stats_code_get_t);
		flags = FLAG_IS_BUF | FLAG_SER_WR;
		break;
	}
	case WLAN_EXT_CMD_SET_BSS_LOSS_THOLD: {
		len = sizeof(wlan_ext_bss_loss_thold_set_t);
		flags = FLAG_IS_BUF | FLAG_SER_RD;
		break;
	}
	case WLAN_EXT_CMD_SET_AUTH_TMO_AND_TRIES: {
		len = sizeof(wlan_ext_mgmt_timeout_and_tries_set_t);
		flags = FLAG_IS_BUF | FLAG_SER_RD;
		break;
	}
	case WLAN_EXT_CMD_SET_ASSOC_TMO_AND_TRIES: {
		len = sizeof(wlan_ext_mgmt_timeout_and_tries_set_t);
		flags = FLAG_IS_BUF | FLAG_SER_RD;
		break;
	}
	case WLAN_EXT_CMD_SET_AUTO_POWER: {
		len = sizeof(wlan_ext_auto_power_t);
		flags = FLAG_IS_BUF | FLAG_SER_RD;
		break;
	}
	case WLAN_EXT_CMD_SET_BCN_LOST_COMP: {
		len = sizeof(wlan_ext_bcn_lost_comp_set_t);
		flags = FLAG_IS_BUF | FLAG_SER_RD;
		break;
	}
	case WLAN_EXT_CMD_SET_ARP_KPALIVE: {
		len = sizeof(wlan_ext_arp_kpalive_set_t);
		flags = FLAG_IS_BUF | FLAG_SER_RD | FLAG_SER_WR;
		break;
	}
	default:
		len = 0;
		flags = FLAG_NONE;
		break;
	}

	if (ISBUF(flags)) {
		buf_ali = amp_align_malloc(len);
		if (!buf_ali) {
			switch (cmd) {
			case WLAN_EXT_CMD_SET_SCAN_FREQ: {
				amp_align_free(buf_ali_freq);
				return -1;
			}
			default:
				return -1;
			}
		}
		/* if the 'buf_ali' buf need to be read in ser, we should
		 * get the real value from 'data' and clean cache in stub */
		if (STUB_CACHE_CLEAN(flags)) {
			memcpy(buf_ali, (void *)(unsigned long)param, len);
			hal_dcache_clean((unsigned long)buf_ali, len);
		}
	}
	args[0] = nif;
	args[1] = (void *)cmd;
	args[2] = buf_ali;
	args[3] = (void *)(unsigned long)len;
	args[4] = (void *)(unsigned long)flags;

	if (ISBUF(flags)) {
		/* if the 'buf_ali' need to be write in ser, we also should
		 * clear the dirty bit of the cacheline to prevent the stub
		 * from mistakenly flush the cache when the ser operates
		 * the buf at the same time */
		if (STUB_CACHE_INVALID(flags)) {
			hal_dcache_invalidate((unsigned long)buf_ali, len);
		}
	}

	ret = func_stub(RPCCALL_NET(wlan_ext_request), 1, ARRAY_SIZE(args), args);

	if (ISBUF(flags)) {
		if (STUB_CACHE_INVALID(flags)) {
			hal_dcache_invalidate((unsigned long)buf_ali, len);
			memcpy((void *)(unsigned long)param, buf_ali, len);
		}
		amp_align_free(buf_ali);

		switch (cmd) {
		case WLAN_EXT_CMD_SET_SCAN_FREQ: {
			wlan_ext_scan_freq_t *p = (wlan_ext_scan_freq_t *)(unsigned long)param;
			if (p->freq_num) {
				p->freq_list = (int32_t *)freq;
				amp_align_free(buf_ali_freq);
			}
			break;
		}
		default:
			break;
		}
	}

	return ret;
}

int wlan_ext_low_power_param_set_default(struct netif *nif, uint32_t dtim)
{
	void *args[2] = {0};

	/* ser do not use 'nif' data, so there is no need
	 * to flush 'nif' cache in stub */
	args[0] = nif;
	args[1] = (void *)(unsigned long)dtim;
	return func_stub(RPCCALL_NET(wlan_ext_low_power_param_set_default), 1, ARRAY_SIZE(args), args);
}


#define LMCA_DRV_CMD_MAX_LEN 128 /* lmac drv only support 50, use 128 */

int xradio_drv_cmd(int argc, char *argv[])
{
	int ret = -1;
	void *args[2] = {0};
	char *strbuf_ali, *buf_ali;
	uint32_t strbuf_len, array_len;

	strbuf_len = argv[argc - 1] - argv[0] + strlen(argv[argc - 1]) + 1;
	if ((argc < 1) || strbuf_len > LMCA_DRV_CMD_MAX_LEN) {
		return -1;
	}

	strbuf_ali = amp_align_malloc(strbuf_len);
	if (!strbuf_ali) {
		return -1;
	}
	memcpy(strbuf_ali, argv[0], strbuf_len);

	array_len = sizeof(uint32_t) * argc; /* to CPU 32 bit */
	buf_ali = amp_align_malloc(array_len);
	if (!buf_ali) {
		amp_align_free(strbuf_ali);
		return -1;
	}

	/* copy str address array to buf_ali, for cpu 32bit and cacheline align */
	for (int i = 0; i < argc; i++) {
		*(uint32_t *)(buf_ali + (i * sizeof(uint32_t))) =
		(uint32_t)(unsigned long)((char *)strbuf_ali + (argv[i] - argv[0]));
	}

	args[0] = (void *)(unsigned long)argc;
	args[1] = buf_ali;
	hal_dcache_clean((unsigned long)strbuf_ali, strbuf_len);
	hal_dcache_clean((unsigned long)buf_ali, array_len);

	ret = func_stub(RPCCALL_NET(xradio_drv_cmd), 1, ARRAY_SIZE(args), args);

	amp_align_free(buf_ali);
	amp_align_free(strbuf_ali);

	return ret;
}

struct mbuf *mb_get(int len, int tx)
{
	struct mbuf *m;
	void *args[2] = {0};

	args[0] = (void *)(unsigned long)len;
	args[1] = (void *)(unsigned long)tx;
	m = (void *)func_stub(RPCCALL_NET(mb_get), 1, ARRAY_SIZE(args), args);

	if (m) {
		int32_t tot_len = sizeof(struct mbuf) + len;
		if (tx) {
			tot_len += MBUF_HEAD_SPACE + MBUF_TAIL_SPACE;
		}
		hal_dcache_invalidate((unsigned long)m, tot_len);
	}

	return m;
}

int mb_free(struct mbuf *m)
{
	void *args[1] = {0};

	args[0] = m;
	hal_dcache_clean((unsigned long)m,
	    (sizeof(struct mbuf) + m->m_len + m->m_headspace + m->m_tailspace));
	return func_stub(RPCCALL_NET(mb_free), 1, ARRAY_SIZE(args), args);
}

#elif (defined CONFIG_ARCH_ARM_ARMV8M)
const wlan_ap_default_conf_t *wlan_ap_get_default_conf(void)
{
	const wlan_ap_default_conf_t *cfg = NULL;

	cfg = (const wlan_ap_default_conf_t *)func_stub(RPCCALL_NET(wlan_ap_get_default_conf), 1, 0, NULL);
	hal_dcache_invalidate((unsigned long)cfg, sizeof(wlan_ap_default_conf_t));

	return cfg;
}

int wlan_event_notify(uint32_t param0, uint32_t param1)
{
	void *args[2] = {0};

	args[0] = (void *)param0;
	args[1] = (void *)param1;

	return func_stub(RPCCALL_NET(wlan_event_notify), 1, ARRAY_SIZE(args), args);
}

int wlan_monitor_input(struct netif *nif, uint8_t *data, uint32_t len, void *info)
{
	int ret = -1;
	void *args[4] = {0};
	char *buf_ali;
	char *inf_ali;

	buf_ali = amp_align_malloc(len);
	if (!buf_ali) {
		return -1;
	}
	memcpy(buf_ali, data, len);
	inf_ali = amp_align_malloc(sizeof(struct frame_info));
	if (!inf_ali) {
		amp_align_free(buf_ali);
		return -1;
	}
	memcpy(inf_ali, info, sizeof(struct frame_info));

	/* M33 do not modify 'nif' data, so there is no need
	 * to flush 'nif' cache in M33 */
	args[0] = nif;
	args[1] = buf_ali;
	args[2] = (void *)len;
	args[3] = inf_ali;
	hal_dcache_clean((unsigned long)buf_ali, len);
	hal_dcache_clean((unsigned long)inf_ali, sizeof(struct frame_info));

	ret = func_stub(RPCCALL_NET(wlan_monitor_input), 1, ARRAY_SIZE(args), args);

	amp_align_free(buf_ali);
	amp_align_free(inf_ali);

	return ret;
}

u16_t lwip_htons(u16_t n)
{
	void *args[1] = {0};

	args[0] = (void *)(unsigned long)n;
	return func_stub(RPCCALL_NET(lwip_htons), 1, ARRAY_SIZE(args), args);
}
#if (LWIP_MBUF_SUPPORT == 0)
err_t ethernetif_raw_input(struct netif *nif, uint8_t *data, u16_t len)
{
	int ret = -1;
	void *args[3] = {0};
	char *buf_ali;

	buf_ali = amp_align_malloc(len);
	if (!buf_ali) {
		return -1;
	}
	memcpy(buf_ali, data, len);

	/* M33 do not modify 'nif' data, so there is no need
	 * to flush 'nif' cache in M33 */
	args[0] = nif;
	args[1] = buf_ali;
	args[2] = (void *)(unsigned long)len;
	hal_dcache_clean((unsigned long)buf_ali, len);

	ret = func_stub(RPCCALL_NET(ethernetif_raw_input), 1, ARRAY_SIZE(args), args);
	amp_align_free(buf_ali);
	return ret;
}
#endif

enum wlan_mode ethernetif_get_mode(struct netif *nif)
{
	void *args[1] = {0};

	/* M33 do not modify 'nif' data, so there is no need
	 * to flush 'nif' cache in M33 */
	args[0] = nif;
	return func_stub(RPCCALL_NET(ethernetif_get_mode), 1, ARRAY_SIZE(args), args);
}

void *ethernetif_get_state(struct netif *nif)
{
	void *args[1] = {0};

	/* M33 do not modify 'nif' data, so there is no need
	 * to flush 'nif' cache in M33 */
	args[0] = nif;
	return (void *)func_stub(RPCCALL_NET(ethernetif_get_state), 1, ARRAY_SIZE(args), args);
}

int wlan_ext_temp_volt_event_input(wlan_ext_temp_volt_event_data_t *data)
{
	int ret = -1;
	void *args[1] = {0};
	char *buf_ali;
	uint32_t len = sizeof(wlan_ext_temp_volt_event_data_t);

	buf_ali = amp_align_malloc(len);
	if (!buf_ali) {
		return -1;
	}
	memcpy(buf_ali, data, len);

	args[0] = buf_ali;
	hal_dcache_clean((unsigned long)buf_ali, len);

	ret = func_stub(RPCCALL_NET(wlan_ext_temp_volt_event_input), 1, ARRAY_SIZE(args), args);
	amp_align_free(buf_ali);
	return ret;
}
#endif
