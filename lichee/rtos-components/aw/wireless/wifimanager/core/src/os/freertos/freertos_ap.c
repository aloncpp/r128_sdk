#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <wpa_ctrl.h>
#include <udhcpc.h>
#include <utils.h>
#include <wifi_log.h>
#include <wmg_ap.h>
#include <unistd.h>
#include <freertos_ap.h>
#include <freertos_common.h>
#include "wlan.h"
#include "net_ctrl.h"
#include "net_init.h"
#include "sysinfo.h"
#include "cmd_util.h"

#define SLEEP_TIMES 3

static wmg_ap_inf_object_t ap_inf_object;
static observer_base *net_ob;

/*
	"wlan connected",
	"wlan disconnected",
	"wlan scan success",
	"wlan scan failed",
	"wlan 4way handshake failed",
	"wlan ssid not found",
	"wlan auth timeout",
	"wlan associate timeout",
	"wlan connect failed",
	"wlan connect loss",
	"wlan associate failed",
	"wlan SAE auth-commit failed",
	"wlan SAE auth-confirm failed",
	"wlan dev hang",
	"wlan ap-sta connected",
	"wlan ap-sta disconnected",
	"network up",
	"network down",
*/
//cu  ->  uncare
static const char * const net_ctrl_msg_str[] = {
	"connected",
	"disconnected",
	"cu",
	"cu",
	"cu",
	"cu",
	"cu",
	"cu",
	"cu",
	"cu",
	"cu",
	"cu",
	"cu",
	"cu",
	"ap-sta connected",
	"ap-sta disconnected",
	"cu",
	"cu",
};

static void ap_event_notify_to_ap_dev(wifi_ap_event_t event)
{
    if (ap_inf_object.ap_event_cb) {
        ap_inf_object.ap_event_cb(event);
    }
}

static void freertos_ap_net_msg_process(uint32_t event, uint32_t data, void *arg)
{
	int need_config = 0;
	struct netif *nif = g_wlan_netif;
	uint16_t type = EVENT_SUBTYPE(event);

	WMG_DEBUG("msg <%s>\n", net_ctrl_msg_str[type]);

	WMG_DEBUG("event : %d \n", type);

	switch (type) {
	case NET_CTRL_MSG_WLAN_CONNECTED:
		ap_event_notify_to_ap_dev(WIFI_AP_ENABLED);
		break;
	case NET_CTRL_MSG_WLAN_DISCONNECTED:
		ap_event_notify_to_ap_dev(WIFI_AP_DISABLED);
		break;
//	case NET_CTRL_MSG_WLAN_SCAN_SUCCESS:
//		break;
//	case NET_CTRL_MSG_WLAN_SCAN_FAILED:
//		break;
//	case NET_CTRL_MSG_WLAN_4WAY_HANDSHAKE_FAILED:
//		break;
//	case NET_CTRL_MSG_WLAN_SSID_NOT_FOUND:
//		break;
//	case NET_CTRL_MSG_WLAN_AUTH_TIMEOUT:
//		break;
//	case NET_CTRL_MSG_WLAN_ASSOC_TIMEOUT:
//		break;
//	case NET_CTRL_MSG_WLAN_CONNECT_FAILED:
//		break;
//	case NET_CTRL_MSG_WLAN_DEV_HANG:
//		break;
//	case NET_CTRL_MSG_WLAN_CONNECTION_LOSS:
//		break;
//	case NET_CTRL_MSG_WLAN_ASSOC_FAILED:
//		break;
//	case NET_CTRL_MSG_WLAN_SAE_COMMIT_FAILED:
//		break;
//	case NET_CTRL_MSG_WLAN_SAE_CONFIRM_FAILED:
//		break;
	case NET_CTRL_MSG_WLAN_AP_STA_DISCONNECTED:
		ap_event_notify_to_ap_dev(WIFI_AP_STA_DISCONNECTED);
		break;
	case NET_CTRL_MSG_WLAN_AP_STA_CONNECTED:
		ap_event_notify_to_ap_dev(WIFI_AP_STA_CONNECTED);
		break;
//	case NET_CTRL_MSG_NETWORK_UP:
//		break;
//	case NET_CTRL_MSG_NETWORK_DOWN:
//		break;
	default:
		ap_event_notify_to_ap_dev(WIFI_AP_UNKNOWN);
		WMG_WARNG("unknown msg (%u, %u)\n", type, data);
		break;
	}
}

//Use ip address to determine whether ap mode has been started
static wmg_status_t freertos_ap_try_get_ip_flag(void)
{
	struct netif *nif = g_wlan_netif;

	if (nif == NULL) {
		return WMG_STATUS_FAIL;
	}

	if (NET_IS_IP4_VALID(nif) && netif_is_link_up(nif)) {
		return WMG_STATUS_SUCCESS;
	}

	return WMG_STATUS_SUCCESS;
}

static wmg_status_t freertos_ap_mode_init(ap_event_cb_t ap_event_cb)
{
	WMG_DEBUG("ap mode init\n");
	if (wlan_get_init_status() == WLAN_STATUS_NO_INIT) {
		net_core_init();
		wlan_set_init_status(WLAN_STATUS_INITED);
	}
	if (ap_event_cb != NULL){
		ap_inf_object.ap_event_cb = ap_event_cb;
	}
	return WMG_STATUS_SUCCESS;
}

static wmg_status_t freertos_ap_mode_enable(void *para)
{
	WMG_DEBUG("ap mode enable\n");
	net_ob = sys_callback_observer_create(CTRL_MSG_TYPE_NETWORK, NET_CTRL_MSG_ALL, freertos_ap_net_msg_process, NULL);
	if (net_ob == NULL) {
		WMG_ERROR("freertos callback observer create failed\n");
		return WMG_STATUS_FAIL;
	}
	if (sys_ctrl_attach(net_ob) != 0) {
		WMG_ERROR("freertos callback observer attach failed\n");
		sys_ctrl_detach(net_ob);
		sys_callback_observer_destroy(net_ob);
		return WMG_STATUS_FAIL;
	} else {
	//use sta_private_data to save observer_base point
		ap_inf_object.ap_private_data = net_ob;
	}
	net_switch_mode(WLAN_MODE_HOSTAP);
	return WMG_STATUS_SUCCESS;;
}

static wmg_status_t freertos_ap_mode_disable(void *para)
{
	wlan_ap_disable();
	if(net_ob != NULL) {
		sys_ctrl_detach(net_ob);
		sys_callback_observer_destroy(net_ob);
		ap_inf_object.ap_private_data = NULL;
	}
	return WMG_STATUS_SUCCESS;
}

static wmg_status_t freertos_ap_mode_deinit(void *para)
{
	WMG_DEBUG("ap mode deinit\n");
	if(g_wlan_netif != NULL) {
		net_close(g_wlan_netif);
		g_wlan_netif = NULL;
	}
	return WMG_STATUS_SUCCESS;
}

static wmg_status_t freertos_ap_enable(wifi_ap_config_t *ap_config)
{
	WMG_DEBUG("ap enable\n");
	wlan_ap_config_t xradio_config;

	int ret = 0;
	int sleep_times = SLEEP_TIMES;
	wlan_ap_disable();
	if(ap_config != NULL) {
		switch (ap_config->sec)
		{
			case WIFI_SEC_NONE:
				if(wlan_ap_set((uint8_t *)ap_config->ssid, strlen(ap_config->ssid), NULL)) {
					WMG_ERROR("creat ap failed\n");
					return WMG_STATUS_FAIL;
				}
				break;
			case WIFI_SEC_WPA_PSK:
			case WIFI_SEC_WPA2_PSK:
				if(wlan_ap_set((uint8_t *)ap_config->ssid, strlen(ap_config->ssid), (uint8_t *)ap_config->psk)) {
					WMG_ERROR("creat ap failed\n");
					return WMG_STATUS_FAIL;
				}
				break;
			case WIFI_SEC_WEP:
			default:
				WMG_ERROR("unsupport sec type\n");
				return WMG_STATUS_FAIL;
		}

		xradio_config.field = WLAN_AP_FIELD_CHANNEL;
		xradio_config.u.channel = ap_config->channel;
		wlan_ap_set_config(&xradio_config);
	}
	wlan_ap_enable();
	while(sleep_times) {
		//Use ip address to determine whether ap mode has been started
		if(!freertos_ap_try_get_ip_flag()){
			return WMG_STATUS_SUCCESS;
		}
		sleep_times--;
		usleep(1000 * 1000);
	}

	return WMG_STATUS_SUCCESS;
}

static wmg_status_t freertos_ap_disable()
{
	// freertos not need
	return WMG_STATUS_SUCCESS;
}

static wmg_status_t freertos_ap_scan_sta_test(get_scan_results_para_t *sta_scan_results_para)
{
	WMG_DEBUG("ap scan\n");
	int ret, i;
	int size = 20;
	wlan_sta_scan_results_t results;
	ret = wlan_ap_scan_once();
	if (ret) {
		WMG_ERROR("scan once failed!\n");
		return WMG_STATUS_FAIL;
	}
	usleep(1000*1000*2);
	results.ap = cmd_malloc(size * sizeof(wlan_sta_ap_t));
	if (results.ap == NULL) {
		WMG_ERROR("no mem\n");
		return WMG_STATUS_FAIL;
	}
	results.size = size;
	ret = wlan_ap_scan_result(&results);
	if (ret == 0) {
		*(sta_scan_results_para->bss_num) = results.num;
		for(i = 0; i < results.num; ++i) {
			memcpy(sta_scan_results_para->scan_results[i].bssid, results.ap[i].bssid, (sizeof(uint8_t) * 6));
			memcpy(sta_scan_results_para->scan_results[i].ssid, results.ap[i].ssid.ssid, SSID_MAX_LEN);
			sta_scan_results_para->scan_results[i].ssid[SSID_MAX_LEN] = '\0';
			sta_scan_results_para->scan_results[i].freq = (uint32_t)(results.ap[i].freq);
			sta_scan_results_para->scan_results[i].rssi = results.ap[i].rssi;
		}
		cmd_free(results.ap);
		return WMG_STATUS_SUCCESS;
	} else {
		cmd_free(results.ap);
		return WMG_STATUS_FAIL;
	}
}

static wmg_status_t freertos_ap_get_config(wifi_ap_config_t *ap_config)
{
	WMG_DEBUG("ap get config\n");
	int ret = 0;
	int i = 0;
	wlan_ap_config_t config;
	wlan_ap_stas_t stas;
	struct sysinfo *sysinfo = sysinfo_get();
	if (sysinfo == NULL) {
		WMG_ERROR("failed to get sysinfo %p\n", sysinfo);
		return WMG_STATUS_FAIL;
	}
	/*get ap mode infomation*/
	config.field = WLAN_AP_FIELD_SSID;
	if (wlan_ap_get_config(&config) != 0) {
		WMG_ERROR("get ssid failed\n");
		return WMG_STATUS_FAIL;
	}
	memcpy(ap_config->ssid, config.u.ssid.ssid, config.u.ssid.ssid_len);

	config.field = WLAN_AP_FIELD_PSK;
	if (wlan_ap_get_config(&config) != 0) {
		WMG_ERROR("get psk failed\n");
		return WMG_STATUS_FAIL;
	}
	memcpy(ap_config->psk, config.u.psk, 48);
	config.field = WLAN_AP_FIELD_KEY_MGMT;
	if (wlan_ap_get_config(&config) != 0) {
		WMG_ERROR("get key failed\n");
		return WMG_STATUS_FAIL;
	}
	ap_config->key_mgmt = config.u.key_mgmt;

	config.field = WLAN_AP_FIELD_CHANNEL;
	if (wlan_ap_get_config(&config) != 0) {
		WMG_ERROR("get channel failed\n");
		return WMG_STATUS_FAIL;
	}
	ap_config->channel = config.u.channel;

	ap_config->ip_addr[0] = (sysinfo->netif_ap_param.ip_addr.addr & 0xff);
	ap_config->ip_addr[1] = ((sysinfo->netif_ap_param.ip_addr.addr >> 8) & 0xff);
	ap_config->ip_addr[2] = ((sysinfo->netif_ap_param.ip_addr.addr >> 16) & 0xff);
	ap_config->ip_addr[3] = ((sysinfo->netif_ap_param.ip_addr.addr >> 24) & 0xff);

	ap_config->gw_addr[0] = (sysinfo->netif_ap_param.gateway.addr & 0xff);
	ap_config->gw_addr[1] = ((sysinfo->netif_ap_param.gateway.addr >> 8) & 0xff);
	ap_config->gw_addr[2] = ((sysinfo->netif_ap_param.gateway.addr >> 16) & 0xff);
	ap_config->gw_addr[3] = ((sysinfo->netif_ap_param.gateway.addr >> 24) & 0xff);

	/*In ap mode, get connect sta's infomation*/
	config.field = WLAN_AP_FIELD_MAX_NUM_STA;
	if (wlan_ap_get_config(&config) != 0) {
		WMG_ERROR("get sta num failed\n");
		return WMG_STATUS_FAIL;
	}
	ap_config->sta_num = config.u.max_num_sta;

	stas.sta = (wlan_ap_sta_t *)cmd_malloc(4 * sizeof(wlan_ap_sta_t));
	if (stas.sta == NULL) {
		return WMG_STATUS_FAIL;
	}
	stas.size = 4;
	ret = wlan_ap_sta_info(&stas);
	if (ret == 0) {
		for (i = 0; i < stas.num; i++) {
			sprintf(ap_config->dev_list[i],"%02x:%02x:%02x:%02x:%02x:%02x",
				stas.sta[i].addr[0], stas.sta[i].addr[1],
				stas.sta[i].addr[2], stas.sta[i].addr[3],
				stas.sta[i].addr[4], stas.sta[i].addr[5]);
				ap_config->dev_list[i][17] = '\0';
		}
	}else {
		WMG_ERROR("ap mode get sta's info failed!\n");
		cmd_free(stas.sta);
		return WMG_STATUS_FAIL;
	}

	cmd_free(stas.sta);
	return WMG_STATUS_SUCCESS;
}

static wmg_status_t freertos_platform_extension(int cmd, void* cmd_para,int *erro_code)
{
	WMG_DEBUG("platform extension\n");
	switch (cmd) {
		case AP_CMD_ENABLE:
			return freertos_ap_enable((wifi_ap_config_t *)cmd_para);
		case AP_CMD_DISABLE:
			return freertos_ap_disable();
		case AP_CMD_GET_CONFIG:
			return freertos_ap_get_config((wifi_ap_config_t *)cmd_para);
		case AP_CMD_SET_SCAN_PARAM:
			return WMG_STATUS_FAIL;
		case AP_CMD_GET_SCAN_RESULTS:
			return freertos_ap_scan_sta_test((get_scan_results_para_t *)cmd_para);
		default:
			return WMG_STATUS_FAIL;
	}
	return WMG_STATUS_FAIL;
}

static wmg_ap_inf_object_t ap_inf_object = {
	.ap_init_flag = WMG_FALSE,
	.sta_num = 0,
	.ap_event_cb = NULL,
	.ap_private_data = NULL,

	.ap_inf_init = freertos_ap_mode_init,
	.ap_inf_deinit = freertos_ap_mode_deinit,
	.ap_inf_enable = freertos_ap_mode_enable,
	.ap_inf_disable = freertos_ap_mode_disable,
	.ap_platform_extension = freertos_platform_extension,
};

wmg_ap_inf_object_t * ap_rtos_inf_object_register(void)
{
	return &ap_inf_object;
}
