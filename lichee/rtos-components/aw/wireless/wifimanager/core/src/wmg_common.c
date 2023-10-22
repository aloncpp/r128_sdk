/*
 * Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
 */

#include <wifimg.h>
#include <wmg_common.h>
#include <wifi_log.h>
#include <wmg_sta.h>
#include <wmg_ap.h>
#include <wmg_monitor.h>
#include <wmg_p2p.h>
#include <linkd.h>
#include <expand_cmd.h>

#define UNK_BITMAP        0x0
#define STA_BITMAP        0x1
#define AP_BITMAP         0x2
#define MONITOR_BITMAP    0x4
#define P2P_BITMAP        0x8

#define STA_MODE_NUM      0
#define AP_MODE_NUM       1
#define MONITOR_MODE_NUM  2
#define P2P_MODE_NUM      3

#define CHECK_MODE_STATUS(x)                \
	*cb_status = __check_mode_status(x);    \
	if(*cb_status != WMG_STATUS_SUCCESS) {  \
		return *cb_status;                  \
	}

static wifimg_object_t wifimg_object;
act_handle_t wmg_act_handle;
static char UNKNOWN_char[] = "UNKNOWN";
static char station_char[] = "sta";
static char ap_char[] = "ap";
static char monitor_char[] = "mon";
static char p2p_char[] = "p2p";

const char *wmg_wifi_mode_to_str(uint8_t mode)
{
	switch (mode) {
	case WIFI_STATION:
		return station_char;
	case WIFI_AP:
		return ap_char;
	case WIFI_MONITOR:
		return monitor_char;
	case WIFI_P2P:
		return p2p_char;
	default:
		return UNKNOWN_char;
	}
}

int __wifi_on(void **call_argv,void **cb_argv)
{
	wmg_status_t ret;
	wifimg_object_t* wifimg_object = get_wifimg_object();

	wifi_mode_t *mode = (wifi_mode_t *)call_argv[0];
	wmg_status_t *cb_status = (wmg_status_t *)cb_argv[0];

	if(!wifimg_object->is_init()){
		ret = wifimg_object->init();
		if (ret != WMG_STATUS_SUCCESS) {
			return ret;
		}
	}

	ret = wifimg_object->switch_mode(*mode);
	if ((ret == WMG_STATUS_UNHANDLED) || (ret == WMG_STATUS_SUCCESS)) {
		WMG_DEBUG("switch wifi mode success\n");
		WMG_INFO("wifi mode %s on success\n",wmg_wifi_mode_to_str(*mode));
		ret = WMG_STATUS_SUCCESS;
	} else {
		WMG_ERROR("failed to switch wifi mode\n");
		WMG_ERROR("wifi mode %s on failed\n",wmg_wifi_mode_to_str(*mode));
	}

	*cb_status = ret;
	return ret;
}

int __wifi_off(void **call_argv,void **cb_argv)
{
	wifimg_object_t* wifimg_object = get_wifimg_object();

	wmg_status_t *cb_status = (wmg_status_t *)cb_argv[0];

	if(wifimg_object->is_init()){
		wifimg_object->deinit();
	} else {
		WMG_DEBUG("wifimg is already deinit\n");
	}

	*cb_status = WMG_STATUS_SUCCESS;
	return WMG_STATUS_SUCCESS;
}

static wmg_status_t __check_mode_status(uint8_t mode_bitmap)
{
	wifimg_object_t* wifimg_object = get_wifimg_object();

	if(!wifimg_object->is_init()){
		WMG_ERROR("wifi manager is not running\n");
		return WMG_STATUS_NOT_READY;
	}

	if((!(wifimg_object->current_mode_bitmap & mode_bitmap)) && (mode_bitmap != 0xf)) {
		WMG_ERROR("wifi %s mode is not enabled\n",
				wmg_wifi_mode_to_str(mode_bitmap));
		return WMG_STATUS_NOT_READY;
	}

	return WMG_STATUS_SUCCESS;
}

#ifdef SUPPORT_STA_MODE
int __wifi_sta_connect(void **call_argv,void **cb_argv)
{
	wifimg_object_t* wifimg_object = get_wifimg_object();
	wifi_sta_cn_para_t *cn_para = (wifi_sta_cn_para_t *)call_argv[0];
	wmg_status_t *cb_status = (wmg_status_t *)cb_argv[0];

	CHECK_MODE_STATUS(STA_BITMAP);

	if (cn_para == NULL) {
		WMG_ERROR("invalid para: cn_para is NULL\n");
		*cb_status = WMG_STATUS_INVALID;
		return WMG_STATUS_INVALID;
	} else {
		if((cn_para->ssid != NULL) && (strlen(cn_para->ssid) > SSID_MAX_LEN)) {
			WMG_ERROR("invalid para: ssid is longer than %d\n", SSID_MAX_LEN);
			*cb_status = WMG_STATUS_INVALID;
			return WMG_STATUS_INVALID;
		}
		if((cn_para->password != NULL) && (strlen(cn_para->password) > PSK_MAX_LEN)) {
			WMG_ERROR("invalid para: psk is longer than %d\n", PSK_MAX_LEN);
			*cb_status = WMG_STATUS_INVALID;
			return WMG_STATUS_INVALID;
		}
	}

	*cb_status = wifimg_object->sta_connect(cn_para);
	if (*cb_status != WMG_STATUS_SUCCESS) {
		WMG_ERROR("wifi sta connect fail\n");
	}

	return *cb_status;
}

int __wifi_sta_disconnect(void **call_argv,void **cb_argv)
{
	wifimg_object_t* wifimg_object = get_wifimg_object();
	wmg_status_t *cb_status = (wmg_status_t *)cb_argv[0];

	CHECK_MODE_STATUS(STA_BITMAP);

	*cb_status = wifimg_object->sta_disconnect();
	if((*cb_status != WMG_STATUS_SUCCESS) && (*cb_status != WMG_STATUS_UNHANDLED)) {
		WMG_ERROR("wifi sta disconnect fail\n");
	}

	return *cb_status;
}

int __wifi_sta_auto_reconnect(void **call_argv,void **cb_argv)
{
	wifimg_object_t* wifimg_object = get_wifimg_object();
	wmg_bool_t *enable = (wmg_bool_t *)call_argv[0];
	wmg_status_t *cb_status = (wmg_status_t *)cb_argv[0];

	CHECK_MODE_STATUS(STA_BITMAP);

	*cb_status = wifimg_object->sta_auto_reconnect(*enable);
	if (*cb_status != WMG_STATUS_SUCCESS) {
		WMG_ERROR("wifi sta auto reconnect fail\n");
	}

	return *cb_status;
}

int __wifi_sta_get_info(void **call_argv,void **cb_argv)
{
	wifimg_object_t* wifimg_object = get_wifimg_object();
	wifi_sta_info_t *sta_info = (wifi_sta_info_t *)call_argv[0];
	wmg_status_t *cb_status = (wmg_status_t *)cb_argv[0];

	CHECK_MODE_STATUS(STA_BITMAP);

	if (sta_info == NULL) {
		WMG_ERROR("invalid sta info para\n");
		*cb_status = WMG_STATUS_INVALID;
		return WMG_STATUS_INVALID;
	}

	*cb_status = wifimg_object->sta_get_info(sta_info);
	if (*cb_status != WMG_STATUS_SUCCESS) {
		WMG_ERROR("wifi sta get info fail\n");
	}

	return *cb_status;
}

int __wifi_sta_list_networks(void **call_argv,void **cb_argv)
{
	wifimg_object_t* wifimg_object = get_wifimg_object();
	wifi_sta_list_t *sta_list_networks = (wifi_sta_list_t *)call_argv[0];
	wmg_status_t *cb_status = (wmg_status_t *)cb_argv[0];

	CHECK_MODE_STATUS(STA_BITMAP);

	if (sta_list_networks == NULL) {
		WMG_ERROR("invalid list networks para\n");
		*cb_status = WMG_STATUS_INVALID;
		return WMG_STATUS_INVALID;
	}

	*cb_status = wifimg_object->sta_list_networks(sta_list_networks);
	if (*cb_status != WMG_STATUS_SUCCESS) {
		WMG_ERROR("wifi sta list network fail\n");
	}

	return *cb_status;
}

int __wifi_sta_remove_networks(void **call_argv,void **cb_argv)
{
	wifimg_object_t* wifimg_object = get_wifimg_object();
	char *ssid = (char *)call_argv[0];
	wmg_status_t *cb_status = (wmg_status_t *)cb_argv[0];

	CHECK_MODE_STATUS(STA_BITMAP);

	*cb_status = wifimg_object->sta_remove_networks(ssid);
	if (*cb_status != WMG_STATUS_SUCCESS) {
		WMG_ERROR("wifi sta remove network fail\n");
	}

	return *cb_status;
}
#endif

#ifdef SUPPORT_AP_MODE
int __wifi_ap_enable(void **call_argv,void **cb_argv)
{
	wifimg_object_t* wifimg_object = get_wifimg_object();
	wifi_ap_config_t *ap_config = (wifi_ap_config_t *)call_argv[0];
	wmg_status_t *cb_status = (wmg_status_t *)cb_argv[0];

	CHECK_MODE_STATUS(AP_BITMAP);

	if(ap_config == NULL) {
		WMG_ERROR("invalid ap config para\n");
		*cb_status = WMG_STATUS_INVALID;
		return WMG_STATUS_INVALID;
	} else {
		if((ap_config->ssid != NULL) && (strlen(ap_config->ssid) > SSID_MAX_LEN)) {
			WMG_ERROR("invalid ap config: ssid is longer than %d\n", SSID_MAX_LEN);
			*cb_status = WMG_STATUS_INVALID;
			return WMG_STATUS_INVALID;
		}
		if((ap_config->psk != NULL) && (strlen(ap_config->psk) > PSK_MAX_LEN)) {
			WMG_ERROR("invalid ap config: psk is longer than %d\n", PSK_MAX_LEN);
			*cb_status = WMG_STATUS_INVALID;
			return WMG_STATUS_INVALID;
		}
	}

	*cb_status = wifimg_object->ap_enable(ap_config);
	if (*cb_status != WMG_STATUS_SUCCESS) {
		WMG_ERROR("wifi ap enable fail\n");
	}

	return *cb_status;
}

int __wifi_ap_disable(void **call_argv,void **cb_argv)
{
	wifimg_object_t* wifimg_object = get_wifimg_object();
	wmg_status_t *cb_status = (wmg_status_t *)cb_argv[0];

	CHECK_MODE_STATUS(AP_BITMAP);

	*cb_status = wifimg_object->ap_disable();
	if((*cb_status != WMG_STATUS_SUCCESS) && (*cb_status != WMG_STATUS_UNHANDLED)) {
		WMG_ERROR("wifi ap disenable fail\n");
	}

	return *cb_status;
}

int __wifi_ap_get_config(void **call_argv,void **cb_argv)
{
	wifimg_object_t* wifimg_object = get_wifimg_object();
	wifi_ap_config_t *ap_config = (wifi_ap_config_t *)call_argv[0];
	wmg_status_t *cb_status = (wmg_status_t *)cb_argv[0];

	CHECK_MODE_STATUS(AP_BITMAP);

	if(ap_config == NULL) {
		WMG_ERROR("invalid ap config,config is NULL\n");
		*cb_status = WMG_STATUS_INVALID;
		return WMG_STATUS_INVALID;
	}

	*cb_status = wifimg_object->ap_get_config(ap_config);
	if (*cb_status != WMG_STATUS_SUCCESS) {
		WMG_ERROR("wifi ap get config fail\n");
	}

	return *cb_status;
}
#endif

#ifdef SUPPORT_MONITOR_MODE
int __wifi_monitor_enable(void **call_argv,void **cb_argv)
{
	wifimg_object_t* wifimg_object = get_wifimg_object();
	uint8_t *channel = (uint8_t *)call_argv[0];
	wmg_status_t *cb_status = (wmg_status_t *)cb_argv[0];

	CHECK_MODE_STATUS(MONITOR_BITMAP);

	if(*channel > 14) {
		WMG_ERROR("invalid mon mode channel(%d), range is(1 ~ 13)\n", *channel);
		*cb_status = WMG_STATUS_INVALID;
		return WMG_STATUS_INVALID;
	}

	*cb_status = wifimg_object->monitor_enable(*channel);
	if(*cb_status != WMG_STATUS_SUCCESS) {
		WMG_ERROR("wifi mon enable fail\n");
	}

	return *cb_status;
}

int __wifi_monitor_set_channel(void **call_argv,void **cb_argv)
{
	wifimg_object_t* wifimg_object = get_wifimg_object();
	uint8_t *channel = (uint8_t *)call_argv[0];
	wmg_status_t *cb_status = (wmg_status_t *)cb_argv[0];

	CHECK_MODE_STATUS(MONITOR_BITMAP);

	if(*channel > 14) {
		WMG_ERROR("invalid mon mode channel(%d), range is(1 ~ 13)\n", *channel);
		*cb_status = WMG_STATUS_INVALID;
		return WMG_STATUS_INVALID;
	}

	*cb_status = wifimg_object->monitor_set_channel(*channel);
	if (*cb_status != WMG_STATUS_SUCCESS) {
		WMG_ERROR("wifi mon set channel fail\n");
	}

	return *cb_status;
}

int __wifi_monitor_disable(void **call_argv,void **cb_argv)
{
	wifimg_object_t* wifimg_object = get_wifimg_object();
	wmg_status_t *cb_status = (wmg_status_t *)cb_argv[0];

	CHECK_MODE_STATUS(MONITOR_BITMAP);

	*cb_status = wifimg_object->monitor_disable();
	if((*cb_status != WMG_STATUS_SUCCESS) && (*cb_status != WMG_STATUS_UNHANDLED)){
		WMG_ERROR("wifi mon disable fail\n");
	}

	return *cb_status;
}
#endif

#ifdef SUPPORT_P2P_MODE
int __wifi_p2p_enable(void **call_argv,void **cb_argv)
{
	wifimg_object_t* wifimg_object = get_wifimg_object();
	wifi_p2p_config_t *p2p_config = (wifi_p2p_config_t *)call_argv[0];
	wmg_status_t *cb_status = (wmg_status_t *)cb_argv[0];

	CHECK_MODE_STATUS(P2P_BITMAP);

	if(p2p_config == NULL) {
		WMG_ERROR("invalid p2p config, p2p config is NULL\n");
		*cb_status = WMG_STATUS_INVALID;
		return WMG_STATUS_INVALID;
	}

	*cb_status = wifimg_object->p2p_enable(p2p_config);
	if(*cb_status != WMG_STATUS_SUCCESS){
		WMG_ERROR("wifi p2p enable fail\n");
	}

	return *cb_status;
}

int __wifi_p2p_disable(void **call_argv,void **cb_argv)
{
	wifimg_object_t* wifimg_object = get_wifimg_object();
	wmg_status_t *cb_status = (wmg_status_t *)cb_argv[0];

	CHECK_MODE_STATUS(P2P_BITMAP);

	*cb_status = wifimg_object->p2p_disable();
	if((*cb_status != WMG_STATUS_SUCCESS) && (*cb_status != WMG_STATUS_UNHANDLED)){
		WMG_ERROR("wifi p2p disable fail\n");
	}

	return *cb_status;
}

int __wifi_p2p_find(void **call_argv,void **cb_argv)
{
	wifimg_object_t* wifimg_object = get_wifimg_object();
	wifi_p2p_peers_t *p2p_peers = (wifi_p2p_peers_t *)call_argv[0];
	uint8_t *find_second = (uint8_t *)call_argv[1];
	wmg_status_t *cb_status = (wmg_status_t *)cb_argv[0];

	CHECK_MODE_STATUS(P2P_BITMAP);

	*cb_status = wifimg_object->p2p_find(p2p_peers, *find_second);
	if(*cb_status != WMG_STATUS_SUCCESS){
		WMG_ERROR("wifi p2p disable fail\n");
	}

	return *cb_status;
}

int __wifi_p2p_connect(void **call_argv,void **cb_argv)
{
	wifimg_object_t* wifimg_object = get_wifimg_object();
	uint8_t *p2p_mac_addr = (uint8_t *)call_argv[0];
	wmg_status_t *cb_status = (wmg_status_t *)cb_argv[0];

	CHECK_MODE_STATUS(P2P_BITMAP);

	if(p2p_mac_addr == NULL) {
		WMG_ERROR("invalid p2p mac addr, p2p_mac_addr is NULL\n");
		*cb_status = WMG_STATUS_INVALID;
		return WMG_STATUS_INVALID;
	}

	*cb_status = wifimg_object->p2p_connect(p2p_mac_addr);
	if(*cb_status != WMG_STATUS_SUCCESS){
		WMG_ERROR("wifi p2p connect fail\n");
	}

	return *cb_status;
}

int __wifi_p2p_disconnect(void **call_argv,void **cb_argv)
{
	wifimg_object_t* wifimg_object = get_wifimg_object();
	uint8_t *p2p_mac_addr = (uint8_t *)call_argv[0];
	wmg_status_t *cb_status = (wmg_status_t *)cb_argv[0];

	CHECK_MODE_STATUS(P2P_BITMAP);

	*cb_status = wifimg_object->p2p_disconnect(p2p_mac_addr);
	if((*cb_status != WMG_STATUS_SUCCESS) && (*cb_status != WMG_STATUS_UNHANDLED)){
		WMG_ERROR("wifi p2p disconnect fail\n");
	}

	return *cb_status;
}

int __wifi_p2p_get_info(void **call_argv,void **cb_argv)
{
	wifimg_object_t* wifimg_object = get_wifimg_object();
	wifi_p2p_info_t *p2p_info = (wifi_p2p_info_t *)call_argv[0];
	wmg_status_t *cb_status = (wmg_status_t *)cb_argv[0];

	CHECK_MODE_STATUS(P2P_BITMAP);

	if(p2p_info == NULL) {
		WMG_ERROR("invalid p2p_info, p2p_info is NULL\n");
		return WMG_STATUS_INVALID;
	}

	*cb_status = wifimg_object->p2p_get_info(p2p_info);
	if(*cb_status != WMG_STATUS_SUCCESS){
		WMG_ERROR("wifi p2p get info fail\n");
	}

	return *cb_status;
}
#endif

int __wifi_register_msg_cb(void **call_argv,void **cb_argv)
{
	wifimg_object_t* wifimg_object = get_wifimg_object();
	wifi_msg_cb_t *msg_cb = (wifi_msg_cb_t *)call_argv[0];
	wmg_status_t *cb_status = (wmg_status_t *)cb_argv[0];
	void *msg_cb_arg = cb_argv[1];

	CHECK_MODE_STATUS((STA_BITMAP | AP_BITMAP | MONITOR_BITMAP | P2P_BITMAP));

	if (msg_cb == NULL) {
		WMG_ERROR("invalid para, msg_cb is NULL\n");
		*cb_status = WMG_STATUS_INVALID;
		return WMG_STATUS_INVALID;
	}

	*cb_status = wifimg_object->register_msg_cb(*msg_cb, msg_cb_arg);
	if(*cb_status != WMG_STATUS_SUCCESS) {
		WMG_ERROR("failed to register msg cb\n");
	}

	return *cb_status;
}

int __wifi_set_scan_param(void **call_argv,void **cb_argv)
{
	wmg_status_t *cb_status = (wmg_status_t *)cb_argv[0];

	*cb_status = WMG_STATUS_FAIL;
	return WMG_STATUS_FAIL;
}

int __wifi_get_scan_results(void **call_argv,void **cb_argv)
{
	wifimg_object_t* wifimg_object = get_wifimg_object();
	wifi_scan_result_t *scan_results = (wifi_scan_result_t *)call_argv[0];
	uint32_t *bss_num = (uint32_t *)call_argv[1];
	uint32_t *arr_size = (uint32_t *)call_argv[2];
	wmg_status_t *cb_status = (wmg_status_t *)cb_argv[0];

	CHECK_MODE_STATUS((STA_BITMAP | AP_BITMAP));

	if (scan_results == NULL || arr_size == 0 || bss_num == NULL) {
		WMG_ERROR("invalid para\n");
		*cb_status = WMG_STATUS_INVALID;
		return WMG_STATUS_INVALID;
	}

	*cb_status = wifimg_object->get_scan_results(scan_results, bss_num, *arr_size);
	if(*cb_status != WMG_STATUS_SUCCESS) {
		WMG_ERROR("failed to get scan results\n");
	}

	return *cb_status;
}

int __wifi_set_mac(void **call_argv,void **cb_argv)
{
	wifimg_object_t* wifimg_object = get_wifimg_object();
	char *ifname = (char *)call_argv[0];
	uint8_t *mac_addr = (uint8_t *)call_argv[1];
	wmg_status_t *cb_status = (wmg_status_t *)cb_argv[0];
	char expand_cmd[128] = {0};
	char mac_addr_char[18] = {0};

#ifdef OS_NET_LINUX_OS
	strcat(expand_cmd, "linux: smac: ");
#endif
#ifdef OS_NET_XRLINK_OS
	strcat(expand_cmd, "xrlink: smac: ");
#endif
#ifdef OS_NET_FREERTOS_OS
	strcat(expand_cmd, "freertos: smac: ");
#endif

	if(ifname == NULL) {
		strcat(expand_cmd, WMG_DEFAULT_INF);
	} else {
		if(strlen(ifname) > 32){
			WMG_ERROR("infname longer than 32\n");
			*cb_status = WMG_STATUS_FAIL;
			return WMG_STATUS_FAIL;
		} else {
			strcat(expand_cmd, ifname);
		}
	}
	strcat(expand_cmd, ": ");

	sprintf(mac_addr_char,"%02x:%02x:%02x:%02x:%02x:%02x",
		mac_addr[0], mac_addr[1], mac_addr[2],
		mac_addr[3], mac_addr[4], mac_addr[5]);
	mac_addr_char[17] = '\0';

	strcat(expand_cmd, mac_addr_char);

	*cb_status = wifimg_object->send_expand_cmd(expand_cmd, NULL);
	if(*cb_status != WMG_STATUS_SUCCESS) {
		WMG_ERROR("wifi send expand cmd fail\n");
	}

	return *cb_status;
}

int __wifi_get_mac(void **call_argv,void **cb_argv)
{
	wifimg_object_t* wifimg_object = get_wifimg_object();
	char *ifname = (char *)call_argv[0];
	uint8_t *mac_addr = (uint8_t *)call_argv[1];
	wmg_status_t *cb_status = (wmg_status_t *)cb_argv[0];
	char expand_cmd[128] = {0};

#ifdef OS_NET_LINUX_OS
	strcat(expand_cmd, "linux: gmac: ");
#endif
#ifdef OS_NET_XRLINK_OS
	strcat(expand_cmd, "xrlink: gmac: ");
#endif
#ifdef OS_NET_FREERTOS_OS
	strcat(expand_cmd, "freertos: gmac: ");
#endif

	if(ifname == NULL) {
		strcat(expand_cmd, WMG_DEFAULT_INF);
	} else {
		if(strlen(ifname) > 32){
			WMG_ERROR("infname longer than 32\n");
			*cb_status = WMG_STATUS_FAIL;
			return WMG_STATUS_FAIL;
		} else {
			strcat(expand_cmd, ifname);
		}
	}

	*cb_status = wifimg_object->send_expand_cmd(expand_cmd, mac_addr);
	if(*cb_status != WMG_STATUS_SUCCESS) {
		WMG_ERROR("wifi get mac addr fail\n");
	}

	return *cb_status;
}

int __wifi_send_80211_raw_frame(void **call_argv,void **cb_argv)
{
	wmg_status_t *cb_status = (wmg_status_t *)cb_argv[0];
	*cb_status = WMG_STATUS_UNSUPPORTED;
	return WMG_STATUS_UNSUPPORTED;
}

int __wifi_set_country_code(void **call_argv,void **cb_argv)
{
	wmg_status_t *cb_status = (wmg_status_t *)cb_argv[0];
	*cb_status = WMG_STATUS_UNSUPPORTED;
	return WMG_STATUS_UNSUPPORTED;
}

int __wifi_get_country_code(void **call_argv,void **cb_argv)
{
	wmg_status_t *cb_status = (wmg_status_t *)cb_argv[0];
	*cb_status = WMG_STATUS_UNSUPPORTED;
	return WMG_STATUS_UNSUPPORTED;
}

int __wifi_set_ps_mode(void **call_argv,void **cb_argv)
{
	wmg_status_t *cb_status = (wmg_status_t *)cb_argv[0];
	*cb_status = WMG_STATUS_UNSUPPORTED;
	return WMG_STATUS_UNSUPPORTED;
}

#ifdef SUPPORT_LINKD
int __wifi_linkd_protocol(void **call_argv,void **cb_argv)
{
	wmg_status_t ret = WMG_STATUS_FAIL;
	wifimg_object_t* wifimg_object = get_wifimg_object();

	wifi_linkd_mode_t *mode = (wifi_linkd_mode_t *)call_argv[0];
	wifi_ap_config_t *params = call_argv[1];
	int second = *(int *)call_argv[2];
	wifi_linkd_result_t *linkd_result = (wifi_linkd_result_t *)call_argv[3];
	wmg_status_t *cb_status = (wmg_status_t *)cb_argv[0];

#ifdef SUPPORT_AP_MODE
	wifi_ap_config_t *ap_config = params;

	if(*mode == WMG_LINKD_MODE_SOFTAP) {
		if(ap_config == NULL) {
			WMG_ERROR("linkd softap config is null\n");
			ret = WMG_STATUS_INVALID;
			goto erro;
		}

		if(!wifimg_object->is_init()){
			ret = wifimg_object->init();
			if (ret != WMG_STATUS_SUCCESS) {
				*cb_status = ret;
				return ret;
			}
		}

		ret = wifimg_object->switch_mode(WIFI_AP);
		if ((ret == WMG_STATUS_UNHANDLED) || (ret == WMG_STATUS_SUCCESS)) {
			WMG_DEBUG("switch wifi ap mode for config network success\n");
			ret = WMG_STATUS_SUCCESS;
		} else {
			WMG_ERROR("failed to switch wifi ap mode for config network\n");
			*cb_status = ret;
			return ret;
		}

		ret = wifimg_object->ap_enable(ap_config);
		if (ret == WMG_STATUS_SUCCESS) {
			WMG_DEBUG("wifi ap enable success\n");
			ret = WMG_STATUS_SUCCESS;
		} else {
			WMG_ERROR("wifi ap enable failed\n");
			*cb_status = ret;
			return ret;
		}
	}
#else
	if(*mode == WMG_LINKD_MODE_SOFTAP) {
		*cb_status = WMG_STATUS_UNSUPPORTED;
		return ret;
	}
#endif

	ret = wifimg_object->linkd_protocol(*mode, params, second, linkd_result);
	if(!ret) {
		WMG_DEBUG("Get ssid(%s) and psk(%s) success\n", linkd_result->ssid, linkd_result->psk);
	}

#ifdef SUPPORT_AP_MODE
	if(*mode == WMG_LINKD_MODE_SOFTAP) {
		ret = wifimg_object->ap_disable();
		if (ret == WMG_STATUS_SUCCESS) {
			WMG_DEBUG("wifi ap disable success\n");
			ret = WMG_STATUS_SUCCESS;
		} else {
			WMG_ERROR("wifi ap disable failed\n");
			*cb_status = ret;
			return ret;
		}
	}
#endif

erro:
	*cb_status = ret;
	return ret;
}
#endif

int __wifi_get_wmg_state(void **call_argv,void **cb_argv)
{
	wmg_status_t ret;
	wifimg_object_t* wifimg_object = get_wifimg_object();
	wmg_status_t *cb_status = (wmg_status_t *)cb_argv[0];

	CHECK_MODE_STATUS((STA_BITMAP | AP_BITMAP | MONITOR_BITMAP | P2P_BITMAP));

	wifi_wmg_state_t *wmg_state = (wifi_wmg_state_t *)call_argv[0];
	wmg_state->support_mode = wifimg_object->support_mode_bitmap;
	wmg_state->current_mode = wifimg_object->current_mode_bitmap;

	if (wifimg_object->current_mode_bitmap & STA_BITMAP) {
		if (wifimg_object->mode_object[STA_MODE_NUM]->mode_opt->mode_ctl(WMG_STA_CMD_GET_STATE, (void *)wmg_state, cb_status)) {
			WMG_ERROR("wifi sta mode get state faile\n");
			ret = WMG_STATUS_FAIL;
			goto erro;
		} else {
			if (*cb_status == WMG_STATUS_SUCCESS) {
				WMG_DEBUG("wifi sta mode get state success\n");
			} else {
				WMG_ERROR("wifi sta mode get state faile\n");
				ret = WMG_STATUS_FAIL;
				goto erro;
			}
		}
	}
	if (wifimg_object->current_mode_bitmap & AP_BITMAP) {
		if (wifimg_object->mode_object[AP_MODE_NUM]->mode_opt->mode_ctl(WMG_AP_CMD_GET_STATE, (void *)wmg_state, cb_status)) {
			WMG_ERROR("wifi ap mode get state faile\n");
			ret = WMG_STATUS_FAIL;
			goto erro;
		} else {
			if (*cb_status == WMG_STATUS_SUCCESS) {
				WMG_DEBUG("wifi ap mode get state success\n");
			} else {
				WMG_ERROR("wifi ap mode get state faile\n");
				ret = WMG_STATUS_FAIL;
				goto erro;
			}
		}
	}
	if (wifimg_object->current_mode_bitmap & MONITOR_BITMAP) {
		if (wifimg_object->mode_object[MONITOR_MODE_NUM]->mode_opt->mode_ctl(WMG_MONITOR_CMD_GET_STATE, (void *)wmg_state, cb_status)) {
			WMG_DEBUG("wifi monitor mode get state faile\n");
			ret = WMG_STATUS_FAIL;
			goto erro;
		} else {
			if (*cb_status == WMG_STATUS_SUCCESS) {
				WMG_DEBUG("wifi mon mode get state success\n");
			} else {
				WMG_DEBUG("wifi mon mode get state faile\n");
				ret = WMG_STATUS_FAIL;
				goto erro;
			}
		}
	}
	if (wifimg_object->current_mode_bitmap & P2P_BITMAP) {
		if (wifimg_object->mode_object[P2P_MODE_NUM]->mode_opt->mode_ctl(WMG_P2P_CMD_GET_STATE, (void *)wmg_state, cb_status)) {
			WMG_DEBUG("wifi p2p mode get state faile\n");
			ret = WMG_STATUS_FAIL;
			goto erro;
		} else {
			if (*cb_status == WMG_STATUS_SUCCESS) {
				WMG_DEBUG("wifi p2p mode get state success\n");
			} else {
				WMG_DEBUG("wifi p2p mode get state faile\n");
				ret = WMG_STATUS_FAIL;
				goto erro;
			}
		}
	}

	ret = WMG_STATUS_SUCCESS;

erro:
	*cb_status = ret;
	return ret;
}

#ifdef OS_NET_XRLINK_OS
int __wifi_vendor_send_data(void **call_argv,void **cb_argv)
{
	wifimg_object_t* wifimg_object = get_wifimg_object();
	uint8_t *data = (uint8_t *)call_argv[0];
	uint32_t len = (uint32_t )call_argv[1];
	wmg_status_t *cb_status = (wmg_status_t *)cb_argv[0];

	CHECK_MODE_STATUS((STA_BITMAP | AP_BITMAP | MONITOR_BITMAP | P2P_BITMAP));

	*cb_status = wifimg_object->vendor_send_data(data, len);
	if(*cb_status != WMG_STATUS_SUCCESS) {
		WMG_ERROR("wifi vendor send data fail\n");
	}

	return *cb_status;
}

int __wifi_vendor_register_rx_cb(void **call_argv,void **cb_argv)
{
	wifimg_object_t* wifimg_object = get_wifimg_object();
	wifi_vend_cb_t cb = (wifi_vend_cb_t)call_argv[0];
	wmg_status_t *cb_status = (wmg_status_t *)cb_argv[0];

	CHECK_MODE_STATUS((STA_BITMAP | AP_BITMAP | MONITOR_BITMAP | P2P_BITMAP));

	*cb_status = wifimg_object->vendor_register_rx_cb(cb);
	if(*cb_status != WMG_STATUS_SUCCESS) {
		WMG_ERROR("wifi vendor register_rx_cb fail\n");
	}

	return *cb_status;
}
#endif

#ifdef SUPPORT_EXPAND
int __wifi_send_expand_cmd(void **call_argv,void **cb_argv)
{
	wifimg_object_t* wifimg_object = get_wifimg_object();
	char *expand_cmd = (char *)call_argv[0];
	void *expand_cb = (void *)call_argv[1];
	wmg_status_t *cb_status = (wmg_status_t *)cb_argv[0];

	if(expand_cmd) {
		*cb_status = wifimg_object->send_expand_cmd(expand_cmd, expand_cb);
		if(*cb_status != WMG_STATUS_SUCCESS) {
			WMG_ERROR("wifi send expand cmd fail\n");
		}
	} else {
		WMG_ERROR("expand_cmd is NULL\n");
		*cb_status = WMG_STATUS_INVALID;
	}

	return *cb_status;
}
#endif

static int __wifi_unspport_mode_function(void **call_argv,void **cb_argv)
{
	wmg_status_t *cb_status = (wmg_status_t *)cb_argv[0];

	WMG_WARNG("wifi unspport mode function\n");

	*cb_status = WMG_STATUS_UNSUPPORTED;

	return *cb_status;
}

act_func_t pla_action_table[] = {
	[WMG_PLA_ACT_WIFI_ON] = {__wifi_on, "__wifi_on"},
	[WMG_PLA_ACT_WIFI_OFF] = {__wifi_off, "__wifi_off"},
	[WMG_PLA_ACT_WIFI_REGISTER_MSG_CB] = {__wifi_register_msg_cb, "__wifi_register_msg_cb"},
	[WMG_PLA_ACT_WIFI_SEND_80211_RAW_FRAME] = {__wifi_send_80211_raw_frame, "__wifi_send_80211_raw_frame"},
	[WMG_PLA_ACT_WIFI_SET_COUNTRY_CODE] = {__wifi_set_country_code, "__wifi_set_country_code"},
	[WMG_PLA_ACT_WIFI_GET_COUNTRY_CODE] = {__wifi_get_country_code, "__wifi_get_country_code"},
	[WMG_PLA_ACT_WIFI_SET_PS_MODE] = {__wifi_set_ps_mode, "__wifi_set_ps_mode"},
#ifdef SUPPORT_LINKD
	[WMG_PLA_ACT_WIFI_LINKD_PROTOCOL] = {__wifi_linkd_protocol, "__wifi_linkd_protocol"},
#else
	[WMG_PLA_ACT_WIFI_LINKD_PROTOCOL] = {__wifi_unspport_mode_function, "__wifi_unspport_mode_function"},
#endif
	[WMG_PLA_ACT_SCAN_PARAM] = {__wifi_set_scan_param, "__wifi_set_scan_param"},
	[WMG_PLA_ACT_SCAN_RESULTS] = {__wifi_get_scan_results, "__wifi_get_scan_results"},
	[WMG_PLA_ACT_GET_WMG_STATE] = {__wifi_get_wmg_state, "__wifi_get_wmg_state"},
#ifdef OS_NET_XRLINK_OS
	[WMG_PLA_ACT_VENDOR_SEND_DATA] = {__wifi_vendor_send_data, "__wifi_vendor_send_data"},
	[WMG_PLA_ACT_VENDOR_REGISTER_RX_CB] = {__wifi_vendor_register_rx_cb, "__wifi_vendor_register_rx_cb"},
#else
	[WMG_PLA_ACT_VENDOR_SEND_DATA] = {__wifi_unspport_mode_function, "__wifi_unspport_mode_function"},
	[WMG_PLA_ACT_VENDOR_REGISTER_RX_CB] = {__wifi_unspport_mode_function, "__wifi_unspport_mode_function"},
#endif
#ifdef SUPPORT_EXPAND
	[WMG_PLA_ACT_WIFI_SET_MAC] = {__wifi_set_mac, "__wifi_set_mac"},
	[WMG_PLA_ACT_WIFI_GET_MAC] = {__wifi_get_mac, "__wifi_get_mac"},
	[WMG_PLA_ACT_SEND_EXPAND_CMD] = {__wifi_send_expand_cmd, "__wifi_send_expand_cmd"},
#else
	[WMG_PLA_ACT_WIFI_SET_MAC] = {__wifi_set_mac, "__wifi_unspport_mode_function"},
	[WMG_PLA_ACT_WIFI_GET_MAC] = {__wifi_get_mac, "__wifi_unspport_mode_function"},
	[WMG_PLA_ACT_SEND_EXPAND_CMD] = {__wifi_unspport_mode_function, "__wifi_unspport_mode_function"},
#endif
};

#ifdef SUPPORT_STA_MODE
act_func_t sta_action_table[] = {
	[WMG_STA_ACT_CONNECT] = {__wifi_sta_connect, "__wifi_sta_connect"},
	[WMG_STA_ACT_DISCONNECT] = {__wifi_sta_disconnect, "__wifi_sta_disconnect"},
	[WMG_STA_ACT_AUTO_RECONNECT] = {__wifi_sta_auto_reconnect, "__wifi_sta_auto_reconnect"},
	[WMG_STA_ACT_GET_INFO] = {__wifi_sta_get_info, "__wifi_sta_get_info"},
	[WMG_STA_ACT_LIST_NETWORKS] = {__wifi_sta_list_networks, "__wifi_sta_list_networks"},
	[WMG_STA_ACT_REMOVE_NETWORKS] = {__wifi_sta_remove_networks, "__wifi_sta_remove_networks"},
};
#else
act_func_t sta_action_table[] = {
	[WMG_STA_ACT_CONNECT] = {__wifi_unspport_mode_function, "__wifi_unspport_mode_function"},
	[WMG_STA_ACT_DISCONNECT] = {__wifi_unspport_mode_function, "__wifi_unspport_mode_function"},
	[WMG_STA_ACT_AUTO_RECONNECT] = {__wifi_unspport_mode_function, "__wifi_unspport_mode_function"},
	[WMG_STA_ACT_GET_INFO] = {__wifi_unspport_mode_function, "__wifi_unspport_mode_function"},
	[WMG_STA_ACT_LIST_NETWORKS] = {__wifi_unspport_mode_function, "__wifi_unspport_mode_function"},
	[WMG_STA_ACT_REMOVE_NETWORKS] = {__wifi_unspport_mode_function, "__wifi_unspport_mode_function"},
};
#endif

#ifdef SUPPORT_AP_MODE
act_func_t ap_action_table[] = {
	[WMG_AP_ACT_ENABLE] = {__wifi_ap_enable, "__wifi_ap_enable"},
	[WMG_AP_ACT_DISABLE] = {__wifi_ap_disable, "__wifi_ap_disable"},
	[WMG_AP_ACT_GET_CONFIG] = {__wifi_ap_get_config, "__wifi_ap_get_config"},
};
#else
act_func_t ap_action_table[] = {
	[WMG_AP_ACT_ENABLE] = {__wifi_unspport_mode_function, "__wifi_unspport_mode_function"},
	[WMG_AP_ACT_DISABLE] = {__wifi_unspport_mode_function, "__wifi_unspport_mode_function"},
	[WMG_AP_ACT_GET_CONFIG] = {__wifi_unspport_mode_function, "__wifi_unspport_mode_function"},
};
#endif

#ifdef SUPPORT_MONITOR_MODE
act_func_t monitor_action_table[] = {
	[WMG_MONITOR_ACT_ENABLE] = {__wifi_monitor_enable, "__wifi_monitor_enable"},
	[WMG_MONITOR_ACT_SET_CHANNEL] = {__wifi_monitor_set_channel, "__wifi_monitor_set_channel"},
	[WMG_MONITOR_ACT_DISABLE] = {__wifi_monitor_disable, "__wifi_monitor_disable"},
};
#else
act_func_t monitor_action_table[] = {
	[WMG_MONITOR_ACT_ENABLE] = {__wifi_unspport_mode_function, "__wifi_unspport_mode_function"},
	[WMG_MONITOR_ACT_SET_CHANNEL] = {__wifi_unspport_mode_function, "__wifi_unspport_mode_function"},
	[WMG_MONITOR_ACT_DISABLE] = {__wifi_unspport_mode_function, "__wifi_unspport_mode_function"},
};
#endif

#ifdef SUPPORT_P2P_MODE
act_func_t p2p_action_table[] = {
	[WMG_P2P_ACT_ENABLE] = {__wifi_p2p_enable, "__wifi_p2p_enable"},
	[WMG_P2P_ACT_DISABLE] = {__wifi_p2p_disable, "__wifi_p2p_disable"},
	[WMG_P2P_ACT_FIND] = {__wifi_p2p_find, "__wifi_p2p_find"},
	[WMG_P2P_ACT_CONNECT] = {__wifi_p2p_connect, "__wifi_p2p_connect"},
	[WMG_P2P_ACT_DISCONNECT] = {__wifi_p2p_disconnect, "__wifi_p2p_disconnect"},
	[WMG_P2P_ACT_GET_INFO] = {__wifi_p2p_get_info, "__wifi_p2p_get_info"},
};
#else
act_func_t p2p_action_table[] = {
	[WMG_P2P_ACT_ENABLE] = {__wifi_unspport_mode_function, "__wifi_unspport_mode_function"},
	[WMG_P2P_ACT_DISABLE] = {__wifi_unspport_mode_function, "__wifi_unspport_mode_function"},
	[WMG_P2P_ACT_FIND] = {__wifi_unspport_mode_function, "__wifi_unspport_mode_function"},
	[WMG_P2P_ACT_CONNECT] = {__wifi_unspport_mode_function, "__wifi_unspport_mode_function"},
	[WMG_P2P_ACT_DISCONNECT] = {__wifi_unspport_mode_function, "__wifi_unspport_mode_function"},
	[WMG_P2P_ACT_GET_INFO] = {__wifi_unspport_mode_function, "__wifi_unspport_mode_function"},
};
#endif

wmg_status_t __wifimanager_init(void)
{
	if(act_init(&wmg_act_handle,"wmg_act_handle",true) != OS_NET_STATUS_OK) {
		WMG_ERROR("act init failed.\n");
		return WMG_STATUS_FAIL;
	}

	act_register_handler(&wmg_act_handle,WMG_ACT_TABLE_PLA_ID,pla_action_table);
	act_register_handler(&wmg_act_handle,WMG_ACT_TABLE_STA_ID,sta_action_table);
	act_register_handler(&wmg_act_handle,WMG_ACT_TABLE_AP_ID,ap_action_table);
	act_register_handler(&wmg_act_handle,WMG_ACT_TABLE_MONITOR_ID,monitor_action_table);
	act_register_handler(&wmg_act_handle,WMG_ACT_TABLE_P2P_ID,p2p_action_table);

	return WMG_STATUS_SUCCESS;
}

wmg_status_t __wifimanager_deinit(void)
{
	act_deinit(&wmg_act_handle);
	return WMG_STATUS_SUCCESS;
}

mode_object_t* __attribute__((weak)) wmg_sta_register_object(void)
{
	WMG_DEBUG("wk:sta mode unsupport\n");
	return NULL;
}

mode_object_t* __attribute__((weak)) wmg_ap_register_object(void)
{
	WMG_DEBUG("wk:ap mode unsupport\n");
	return NULL;
}

mode_object_t* __attribute__((weak)) wmg_monitor_register_object(void)
{
	WMG_DEBUG("wk:mon mode unsupport\n");
	return NULL;
}

mode_object_t* __attribute__((weak)) wmg_p2p_register_object(void)
{
	WMG_DEBUG("wk:p2p mode unsupport\n");
	return NULL;
}

/* This is a mode bitmap
 * used to indicate a list of supported modes 
 * The table indicates that only 4 individual modes are supported by default and coexistence modes are not supported*/
static uint8_t support_list[4] = {1,2,4,8};
static mode_support_list_t mode_support_list = {
	.item_num = 4,
	.item_table = support_list,
};

mode_support_list_t* __attribute__((weak)) wmg_mode_support_list_register(void)
{
	return &mode_support_list;
}

static wmg_bool_t compared_mode_support_list(uint8_t mode_table)
{
	int i = wifimg_object.mode_support_list->item_num;
	uint8_t *table_p = wifimg_object.mode_support_list->item_table;
	int j = 0;

	for(; j < i; j++){
		if(mode_table == table_p[j]){
			return WMG_TRUE;
		}
	}

	return WMG_FALSE;
}

//Remove unsupported patterns
static void update_mode_support_list(void)
{
	int i = wifimg_object.mode_support_list->item_num;
	uint8_t *table_p = wifimg_object.mode_support_list->item_table;
	int j = 0;
	uint8_t tmp_table_support_mode = 0;

	for(; j < i; j++) {
		if(table_p[j] <= 8) {
			tmp_table_support_mode |= table_p[j];
		}
	}

	/* update support mode bitmap */
	wifimg_object.support_mode_bitmap &= tmp_table_support_mode;

	for(; j < i; j++){
		if(table_p[j] & (~wifimg_object.support_mode_bitmap)){
			table_p[j] = 0;
		}
	}
}

static void printf_mode_support_list(void)
{
	int i = wifimg_object.mode_support_list->item_num;
	uint8_t *table_p = wifimg_object.mode_support_list->item_table;
	int j = 0;

	for(; j < i; j++){
		if(table_p[j] != 0) {
			WMG_INFO("* ");
			if(table_p[j] & WIFI_STATION) {
				WMG_INFO_NONE("sta ");
			}
			if(table_p[j] & WIFI_AP) {
				WMG_INFO_NONE("ap ");
			}
			if(table_p[j] & WIFI_MONITOR) {
				WMG_INFO_NONE("mon ");
			}
			if(table_p[j] & WIFI_P2P) {
				WMG_INFO_NONE("p2p ");
			}
			if(((table_p[j] & WIFI_STATION) + ((table_p[j] & WIFI_AP) >> 1)
			+ ((table_p[j] & WIFI_MONITOR) >> 2) + ((table_p[j] & WIFI_P2P) >> 3)) > 1) {
				WMG_INFO_NONE("coexist\n");
			} else {
				WMG_INFO_NONE("alone\n");
			}
		}
	}
}

static void wmg_mode_object_register(int mode_num, uint8_t mode_bitmap, mode_object_t *mode_object)
{
	wifimg_object.mode_object[mode_num] = mode_object;
	if(wifimg_object.mode_object[mode_num] != NULL) {
		if(wifimg_object.mode_object[mode_num]->init != NULL) {
			if(wifimg_object.mode_object[mode_num]->init()){
				WMG_WARNG("%s mode register faile\n", wmg_wifi_mode_to_str(mode_bitmap));
			}
		}
		wifimg_object.support_mode_bitmap |= mode_bitmap;
	}
}

/* init wifimg */
static wmg_status_t wifimg_init(void)
{
	wmg_status_t ret = WMG_STATUS_FAIL;

	if(wifimg_object.init_flag == WMG_FALSE) {
		os_net_mutex_create(&wifimg_object.mutex_lock);

/* wifi driver support mode */
		wifimg_object.mode_support_list = wmg_mode_support_list_register();
/* wifimanager support mode */
		wmg_mode_object_register(STA_MODE_NUM, STA_BITMAP, wmg_sta_register_object());
		wmg_mode_object_register(AP_MODE_NUM, AP_BITMAP, wmg_ap_register_object());
		wmg_mode_object_register(MONITOR_MODE_NUM, MONITOR_BITMAP, wmg_monitor_register_object());
		wmg_mode_object_register(P2P_MODE_NUM, P2P_BITMAP, wmg_p2p_register_object());
/* actually support mode */
		update_mode_support_list();

		wifimg_object.init_flag = WMG_TRUE;
		WMG_INFO("**************************************\n");
		WMG_INFO("* %s\n", WMG_DECLARE);
		WMG_INFO("* version: %s\n", WMG_VERSION);
		WMG_INFO("* support mode:(%s | %s | %s | %s)\n",
		(wifimg_object.support_mode_bitmap & STA_BITMAP)?"sta":"---",
		(wifimg_object.support_mode_bitmap & AP_BITMAP)?"ap":"--",
		(wifimg_object.support_mode_bitmap & MONITOR_BITMAP)?"mon":"---",
		(wifimg_object.support_mode_bitmap & P2P_BITMAP)?"p2p":"---");
		WMG_INFO("* mode support list\n");
		printf_mode_support_list();
		WMG_INFO("**************************************\n");

		ret = WMG_STATUS_SUCCESS;
	} else {
		WMG_INFO("wifimg is already init\n");
		ret = WMG_STATUS_SUCCESS;
	}

	return ret;
}

static void wifimg_mode_enable(int mode_num, uint8_t mode_bitmap)
{
	int erro_code;
	uint8_t current_mode_bitmap = wifimg_object.current_mode_bitmap;
	char *mode_char = (char *)wmg_wifi_mode_to_str(mode_bitmap);
	if(!(current_mode_bitmap & mode_bitmap)){
		if(wifimg_object.mode_object[mode_num]->mode_opt->mode_enable(&erro_code)){
			WMG_WARNG("wifi %s mode enable faile\n", mode_char);
		} else {
			wifimg_object.current_mode_bitmap |= mode_bitmap;
			WMG_DEBUG("wifi %s mode enable success\n", mode_char);
		}
	}
}

static void wifimg_mode_disable(int mode_num, uint8_t mode_bitmap)
{
	int erro_code;
	uint8_t current_mode_bitmap = wifimg_object.current_mode_bitmap;
	char *mode_char = (char *)wmg_wifi_mode_to_str(mode_bitmap);
	if(current_mode_bitmap & mode_bitmap) {
		if (wifimg_object.mode_object[mode_num]->mode_opt->mode_disable(&erro_code)) {
			WMG_WARNG("wifi %s mode disable faile\n", mode_char);
		} else {
			wifimg_object.current_mode_bitmap &= (~mode_bitmap);
			WMG_DEBUG("wifi %s mode disable success\n", mode_char);
		}
	}
}

static void wifimg_mode_deinit(int mode_num, uint8_t mode_bitmap)
{
	wifimg_mode_disable(mode_num, mode_bitmap);
	if((wifimg_object.mode_object[mode_num] != NULL) &&
			(wifimg_object.mode_object[mode_num]->deinit != NULL)) {
		wifimg_object.mode_object[mode_num]->deinit();
	}
}

static void wifimg_deinit(void)
{
	if(wifimg_object.init_flag == WMG_TRUE) {
		WMG_INFO("wifimg deinit now\n");

		wifimg_mode_deinit(STA_MODE_NUM, STA_BITMAP);
		wifimg_mode_deinit(AP_MODE_NUM, AP_BITMAP);
		wifimg_mode_deinit(MONITOR_MODE_NUM, MONITOR_BITMAP);
		wifimg_mode_deinit(P2P_MODE_NUM, P2P_BITMAP);

		wifimg_object.mode_support_list = NULL;
		wifimg_object.init_flag = WMG_FALSE;
		wifimg_object.wifi_status = WLAN_STATUS_DOWN,
		wifimg_object.current_mode_bitmap = UNK_BITMAP,
		wifimg_object.support_mode_bitmap = UNK_BITMAP,

		WMG_INFO("wifimg deinit success\n");
		return;
	} else {
		WMG_INFO("wifimg already deinit\n");
		return;
	}
}

static wmg_status_t wifimg_switch_mode(wifi_mode_t switch_mode)
{
	uint8_t disable_mode_bitmap;
	uint8_t switch_mode_bitmap = (uint8_t) switch_mode;

	if(wifimg_object.init_flag == WMG_FALSE) {
		WMG_ERROR("wifimg is not init\n");
		return WMG_STATUS_NOT_READY;
	}

	if((wifimg_object.support_mode_bitmap & switch_mode_bitmap) == 0) {
		WMG_ERROR("switch %s mode fail(unsupport)\n", wmg_wifi_mode_to_str(switch_mode_bitmap));
		return WMG_STATUS_UNSUPPORTED;
	}

	WMG_DEBUG("switch_mode_bitmap:0x%x\ncurrent_mode_bitmap:0x%x\n", switch_mode_bitmap, wifimg_object.current_mode_bitmap);

	if(switch_mode_bitmap & wifimg_object.current_mode_bitmap) {
		WMG_DEBUG("Current mode 0x%x, no need to switch to mode 0x%x\n", wifimg_object.current_mode_bitmap, switch_mode_bitmap);
		return WMG_STATUS_UNHANDLED;
	}

	if(compared_mode_support_list((switch_mode_bitmap | wifimg_object.current_mode_bitmap))) {
		disable_mode_bitmap = ((~(switch_mode_bitmap | wifimg_object.current_mode_bitmap)) & wifimg_object.current_mode_bitmap);
	} else {
		disable_mode_bitmap = ((~switch_mode_bitmap) & wifimg_object.current_mode_bitmap);
	}
	WMG_DEBUG("disable_mode_bitmap:0x%x\n",disable_mode_bitmap);

	os_net_mutex_lock(&wifimg_object.mutex_lock);

	if (disable_mode_bitmap & STA_BITMAP) {
		wifimg_mode_disable(STA_MODE_NUM, STA_BITMAP);
	}
	if (disable_mode_bitmap & AP_BITMAP) {
		wifimg_mode_disable(AP_MODE_NUM, AP_BITMAP);
	}
	if (disable_mode_bitmap & MONITOR_BITMAP) {
		wifimg_mode_disable(MONITOR_MODE_NUM, MONITOR_BITMAP);
	}
	if (disable_mode_bitmap & P2P_BITMAP) {
		wifimg_mode_disable(P2P_MODE_NUM, P2P_BITMAP);
	}

	if (switch_mode_bitmap & STA_BITMAP) {
		wifimg_mode_enable(STA_MODE_NUM, STA_BITMAP);
	}
	if (switch_mode_bitmap & AP_BITMAP) {
		wifimg_mode_enable(AP_MODE_NUM, AP_BITMAP);
	}
	if (switch_mode_bitmap & MONITOR_BITMAP) {
		wifimg_mode_enable(MONITOR_MODE_NUM, MONITOR_BITMAP);
	}
	if (switch_mode_bitmap & P2P_BITMAP) {
		wifimg_mode_enable(P2P_MODE_NUM, P2P_BITMAP);
	}

	WMG_DEBUG("switch after current_mode_bitmap:0x%x\n",wifimg_object.current_mode_bitmap);

	os_net_mutex_unlock(&wifimg_object.mutex_lock);

	if(switch_mode_bitmap & wifimg_object.current_mode_bitmap){
		WMG_DEBUG("switch mode success\n");
		return WMG_STATUS_SUCCESS;
	} else {
		WMG_ERROR("switch mode faile\n");
		return WMG_STATUS_FAIL;
	}
}

static wmg_bool_t wifimg_is_init(void)
{
	return wifimg_object.init_flag;
}

static wmg_status_t wifimg_mode_ctl_cmd(int mode ,int cmd, void *para, void *cb_msg)
{
	return wifimg_object.mode_object[mode]->mode_opt->mode_ctl(cmd, para, cb_msg);
}

#ifdef SUPPORT_STA_MODE
static wmg_status_t wifimg_sta_connect(wifi_sta_cn_para_t *cn_para)
{
	return wifimg_mode_ctl_cmd(STA_MODE_NUM , WMG_STA_CMD_CONNECT, cn_para, NULL);
}

static wmg_status_t wifimg_sta_disconnect(void)
{
	return wifimg_mode_ctl_cmd(STA_MODE_NUM , WMG_STA_CMD_DISCONNECT, NULL, NULL);
}

static wmg_status_t wifimg_sta_auto_reconnect(wmg_bool_t enable)
{
	return wifimg_mode_ctl_cmd(STA_MODE_NUM , WMG_STA_CMD_AUTO_RECONNECT, &enable, NULL);
}

static wmg_status_t wifimg_sta_get_info(wifi_sta_info_t *sta_info)
{
	return wifimg_mode_ctl_cmd(STA_MODE_NUM , WMG_STA_CMD_GET_INFO, (void *)sta_info, NULL);
}

static wmg_status_t wifimg_sta_list_networks(wifi_sta_list_t *sta_list)
{
	return wifimg_mode_ctl_cmd(STA_MODE_NUM , WMG_STA_CMD_LIST_NETWORKS, (void *)sta_list, NULL);
}

static wmg_status_t wifimg_sta_remove_networks(char *ssid)
{
	return wifimg_mode_ctl_cmd(STA_MODE_NUM , WMG_STA_CMD_REMOVE_NETWORKS, (void *)ssid, NULL);
}
#endif

#ifdef SUPPORT_AP_MODE
static wmg_status_t wifimg_ap_enable(wifi_ap_config_t *ap_config)
{
	return wifimg_mode_ctl_cmd(AP_MODE_NUM , WMG_AP_CMD_ENABLE, (void *)ap_config, NULL);
}

static wmg_status_t wifimg_ap_disable(void)
{
	return wifimg_mode_ctl_cmd(AP_MODE_NUM , WMG_AP_CMD_DISABLE, NULL, NULL);
}

static wmg_status_t wifimg_ap_get_config(wifi_ap_config_t *ap_config)
{
	return wifimg_mode_ctl_cmd(AP_MODE_NUM , WMG_AP_CMD_GET_CONFIG, (void *)ap_config, NULL);
}
#endif

#ifdef SUPPORT_MONITOR_MODE
static wmg_status_t wifimg_monitor_enable(uint8_t channel)
{
	return wifimg_mode_ctl_cmd(MONITOR_MODE_NUM , WMG_MONITOR_CMD_ENABLE, (void *)&channel, NULL);
}

static wmg_status_t wifimg_monitor_set_channel(uint8_t channel)
{
	return wifimg_mode_ctl_cmd(MONITOR_MODE_NUM , WMG_MONITOR_CMD_SET_CHANNEL, (void *)&channel, NULL);
}

static wmg_status_t wifimg_monitor_disable(void)
{
	return wifimg_mode_ctl_cmd(MONITOR_MODE_NUM , WMG_MONITOR_CMD_DISABLE, NULL, NULL);
}
#endif

#ifdef SUPPORT_P2P_MODE
static wmg_status_t wifimg_p2p_enable(wifi_p2p_config_t *p2p_config)
{
	return wifimg_mode_ctl_cmd(P2P_MODE_NUM , WMG_P2P_CMD_ENABLE, p2p_config, NULL);
}

static wmg_status_t wifimg_p2p_disable(void)
{
	return wifimg_mode_ctl_cmd(P2P_MODE_NUM , WMG_P2P_CMD_DISABLE, NULL, NULL);
}

static wmg_status_t wifimg_p2p_find(wifi_p2p_peers_t *p2p_peers, uint8_t find_second)
{
	wmg_status_t ret, cb_msg;
	find_results_para_t cmd_para = {
		.peers = p2p_peers,
		.find_second = find_second,
	};

	return wifimg_mode_ctl_cmd(P2P_MODE_NUM , WMG_P2P_CMD_FIND, &cmd_para, NULL);
}

static wmg_status_t wifimg_p2p_connect(uint8_t *p2p_mac_addr)
{
	return wifimg_mode_ctl_cmd(P2P_MODE_NUM , WMG_P2P_CMD_CONNECT, p2p_mac_addr, NULL);
}

static wmg_status_t wifimg_p2p_disconnect(void)
{
	return wifimg_mode_ctl_cmd(P2P_MODE_NUM , WMG_P2P_CMD_DISCONNECT, NULL, NULL);
}

static wmg_status_t wifimg_p2p_get_info(wifi_p2p_info_t *p2p_info)
{
	return wifimg_mode_ctl_cmd(P2P_MODE_NUM , WMG_P2P_CMD_GET_INFO, p2p_info, NULL);
}
#endif

static wmg_status_t wifimg_register_msg_cb(wifi_msg_cb_t msg_cb ,void *msg_cb_arg)
{
	common_msg_cb_t msg_cb_attr;

	if(wifimg_object.wmg_msg_cb != NULL) {
		WMG_DEBUG("wifimg register new msg cb\n");
	}

	msg_cb_attr.msg_cb = msg_cb;
	msg_cb_attr.msg_cb_arg = msg_cb_arg;

	if((wifimg_object.mode_object[STA_MODE_NUM] != NULL) && (wifimg_object.current_mode_bitmap & STA_BITMAP)) {
		if(wifimg_mode_ctl_cmd(STA_MODE_NUM , WMG_STA_CMD_REGISTER_MSG_CB, (void *)&msg_cb_attr, NULL)) {
			WMG_WARNG("sta register cb fail\n");
			return WMG_STATUS_FAIL;
		}
	}
	if((wifimg_object.mode_object[AP_MODE_NUM] != NULL) && (wifimg_object.current_mode_bitmap & AP_BITMAP)) {
		if(wifimg_mode_ctl_cmd(AP_MODE_NUM , WMG_AP_CMD_REGISTER_MSG_CB, (void *)&msg_cb_attr, NULL)) {
			WMG_WARNG("ap register cb fail\n");
			return WMG_STATUS_FAIL;
		}
	}
	if((wifimg_object.mode_object[MONITOR_MODE_NUM] != NULL) && (wifimg_object.current_mode_bitmap & MONITOR_BITMAP)) {
		if(wifimg_mode_ctl_cmd(MONITOR_MODE_NUM , WMG_MONITOR_CMD_REGISTER_MSG_CB, (void *)&msg_cb_attr, NULL)) {
			WMG_WARNG("mon register cb fail\n");
			return WMG_STATUS_FAIL;
		}
	}
	if((wifimg_object.mode_object[P2P_MODE_NUM] != NULL) && (wifimg_object.current_mode_bitmap & P2P_BITMAP)) {
		if(wifimg_mode_ctl_cmd(P2P_MODE_NUM , WMG_P2P_CMD_REGISTER_MSG_CB, (void *)&msg_cb_attr, NULL)) {
			WMG_WARNG("p2p register cb fail\n");
			return WMG_STATUS_FAIL;
		}
	}
	return WMG_STATUS_SUCCESS;
}

static wmg_status_t wifimg_set_scan_param(wifi_scan_param_t *scan_param)
{
	return WMG_STATUS_FAIL;
}

static wmg_status_t wifimg_get_scan_results(wifi_scan_result_t *wifi_scan_results, uint32_t *wifi_bss_num, uint32_t wifi_arr_size)
{
	wmg_status_t ret;

	get_scan_results_para_t cmd_para = {
		.scan_results = wifi_scan_results,
		.bss_num = wifi_bss_num,
		.arr_size = wifi_arr_size,
	};

	if (wifimg_object.current_mode_bitmap & STA_BITMAP) {
		ret = wifimg_mode_ctl_cmd(STA_MODE_NUM , WMG_STA_CMD_SCAN_RESULTS, (void *)&cmd_para, NULL);
	} else if (wifimg_object.current_mode_bitmap & AP_BITMAP) {
		ret = wifimg_mode_ctl_cmd(AP_MODE_NUM , WMG_AP_CMD_SCAN_RESULTS, (void *)&cmd_para, NULL);
	} else {
		WMG_ERROR("current mode unsupport get scan results\n");
		return WMG_STATUS_FAIL;
	}

	return ret;
}

#ifdef SUPPORT_LINKD
static wmg_status_t wifimg_linkd_protocol(wifi_linkd_mode_t mode, void *params, int second, wifi_linkd_result_t *linkd_result)
{
	return wmg_linkd_protocol(mode, params, second, linkd_result);
}
#endif

#ifdef OS_NET_XRLINK_OS
static wmg_status_t wifimg_vendor_send_data(uint8_t *data, uint32_t len)
{
	wmg_status_t ret;

	vendor_send_data_para_t cmd_para = {
		.data = data,
		.len = len,
	};

	if (wifimg_object.current_mode_bitmap & STA_BITMAP) {
		ret = wifimg_mode_ctl_cmd(STA_MODE_NUM , WMG_STA_CMD_VENDOR_SEND_DATA, (void *)&cmd_para, NULL);
	} else if (wifimg_object.current_mode_bitmap & AP_BITMAP) {
		ret = wifimg_mode_ctl_cmd(AP_MODE_NUM , WMG_AP_CMD_VENDOR_SEND_DATA, (void *)&cmd_para, NULL);
	} else if (wifimg_object.current_mode_bitmap & MONITOR_BITMAP) {
		ret = wifimg_mode_ctl_cmd(MONITOR_MODE_NUM , WMG_MONITOR_CMD_VENDOR_SEND_DATA, (void *)&cmd_para, NULL);
	} else {
		WMG_ERROR("current mode unsupport vendor send data\n");
		return WMG_STATUS_FAIL;
	}

	return ret;
}

static wmg_status_t wifimg_vendor_register_rx_cb(wifi_vend_cb_t cb)
{
	wmg_status_t ret;

	if (wifimg_object.current_mode_bitmap & STA_BITMAP) {
		ret = wifimg_mode_ctl_cmd(STA_MODE_NUM , WMG_STA_CMD_VENDOR_REGISTER_RX_CB, (void *)cb, NULL);
	} else if (wifimg_object.current_mode_bitmap & AP_BITMAP) {
		ret = wifimg_mode_ctl_cmd(AP_MODE_NUM , WMG_AP_CMD_VENDOR_REGISTER_RX_CB, (void *)cb, NULL);
	} else if (wifimg_object.current_mode_bitmap & MONITOR_BITMAP) {
		ret = wifimg_mode_ctl_cmd(MONITOR_MODE_NUM , WMG_MONITOR_CMD_VENDOR_REGISTER_RX_CB, (void *)cb, NULL);
	} else {
		WMG_ERROR("current mode unsupport vendor register\n");
		return WMG_STATUS_FAIL;
	}

	return ret;
}
#endif

#ifdef SUPPORT_EXPAND
static wmg_status_t wifimg_send_expand_cmd(char *expand_cmd, void *expand_cb)
{
	return wmg_send_expand_cmd(expand_cmd, expand_cb);
}
#endif

static wifimg_object_t wifimg_object = {
	.init_flag = WMG_FALSE,
	.wifi_status = WLAN_STATUS_DOWN,
	.current_mode_bitmap = UNK_BITMAP,
	.support_mode_bitmap = UNK_BITMAP,
	.mode_object = {NULL, NULL, NULL, NULL},
	.wmg_msg_cb = NULL,
	.mode_support_list = NULL,

	.init = wifimg_init,
	.deinit = wifimg_deinit,
	.switch_mode = wifimg_switch_mode,

	.is_init = wifimg_is_init,

#ifdef SUPPORT_STA_MODE
	.sta_connect =  wifimg_sta_connect,
	.sta_disconnect =  wifimg_sta_disconnect,
	.sta_auto_reconnect =  wifimg_sta_auto_reconnect,
	.sta_get_info =  wifimg_sta_get_info,
	.sta_list_networks =  wifimg_sta_list_networks,
	.sta_remove_networks =  wifimg_sta_remove_networks,
#endif

#ifdef SUPPORT_AP_MODE
	.ap_enable = wifimg_ap_enable,
	.ap_disable = wifimg_ap_disable,
	.ap_get_config = wifimg_ap_get_config,
#endif

#ifdef SUPPORT_MONITOR_MODE
	.monitor_enable = wifimg_monitor_enable,
	.monitor_set_channel = wifimg_monitor_set_channel,
	.monitor_disable = wifimg_monitor_disable,
#endif

#ifdef SUPPORT_P2P_MODE
	.p2p_enable = wifimg_p2p_enable,
	.p2p_disable = wifimg_p2p_disable,
	.p2p_find = wifimg_p2p_find,
	.p2p_connect = wifimg_p2p_connect,
	.p2p_disconnect = wifimg_p2p_disconnect,
	.p2p_get_info = wifimg_p2p_get_info,
#endif

	.register_msg_cb = wifimg_register_msg_cb,
	.set_scan_param = wifimg_set_scan_param,
	.get_scan_results =  wifimg_get_scan_results,
#ifdef OS_NET_XRLINK_OS
	.vendor_send_data = wifimg_vendor_send_data,
	.vendor_register_rx_cb = wifimg_vendor_register_rx_cb,
#endif
#ifdef SUPPORT_EXPAND
	.send_expand_cmd = wifimg_send_expand_cmd,
#endif

#ifdef SUPPORT_LINKD
	.linkd_protocol = wifimg_linkd_protocol,
#endif
};

wifimg_object_t* get_wifimg_object(void)
{
	return &wifimg_object;
}
