#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <pthread.h>
#include <wpa_ctrl.h>
#include <freertos_sta.h>
#include <udhcpc.h>
#include <utils.h>
#include <wifi_log.h>
#include <unistd.h>
#include <wmg_sta.h>
#include <freertos/event.h>
#include <freertos/udhcpc.h>
#include <freertos/scan.h>
#include <freertos_common.h>
#include "wlan.h"
#include "net_ctrl.h"
#include "net_init.h"
#include "sysinfo.h"
#include "cmd_util.h"
#include "wlan_ext_req.h"
#include "lwip/tcpip.h"
#include "lwip/inet.h"
#include "lwip/dhcp.h"
#include "lwip/netifapi.h"

#define SLEEP_TIMES 10

static wmg_sta_inf_object_t sta_inf_object;
static int ip_flag = 0;
static int connect_status = 0;

/* Optimized for logging events
	"wlan disconnected",
	"wlan scan started",
	"wlan scan success",
	"wlan scan failed",
	"wlan scan results",
	"wlan network not found",
	"wlan password incorrect",
	"wlan authentiacation",
	"wlan auth reject",
	"wlan auth timeout",
	"wlan auth failed",
	"wlan deauth",
	"wlan associating",
	"wlan assoc reject",
	"wlan assocated",
	"wlan assoc timeout",
	"wlan assoc failed",
	"wlan disassoc",
	"wlan 4way handshake",
	"wlan 4way handshake failed",
	"wlan group handshake",
	"wlan group handshake done",
	"wlan connected",
	"wlan connect timeout",
	"wlan connect failed",
	"wlan connection loss",
	"wlan connect timeout",
	"wlan dev hang",
	"wlan dhcp start",
	"wlan dhcp timeout",
	"wlan dhcp success",
	"wlan SAE auth-commit failed",
	"wlan SAE auth-confirm failed",
	"wlan network up",
	"wlan network down",
	"wlan terminating",
	"wlan unknown",
*/
// uc -> uncare
static const char * const net_ctrl_msg_str[] = {
	"wlan disconnected",
	"uc",
	"wlan scan success",
	"wlan scan failed",
	"wlan scan results",
	"wlan network not found",
	"wlan password incorrect",
	"wlan authentiacation",
	"wlan auth reject",
	"wlan auth timeout",
	"wlan auth failed",
	"wlan deauth",
	"wlan associating",
	"wlan assoc reject",
	"wlan assocated",
	"wlan assoc timeout",
	"wlan assoc failed",
	"wlan disassoc",
	"uc",
	"wlan 4way handshake failed",
	"uc",
	"wlan group handshake done",
	"wlan connected",
	"wlan connect timeout",
	"wlan connect failed",
	"wlan connection loss",
	"wlan connect timeout",
	"uc",
	"wlan dhcp start",
	"wlan dhcp timeout",
	"wlan dhcp success",
	"uc",
	"uc",
	"wlan network up",
	"wlan network down",
	"uc",
	"wlan unknown",
};

static void sta_event_notify_to_sta_dev(wifi_sta_event_t event)
{
	if (sta_inf_object.sta_event_cb) {
		sta_inf_object.sta_event_cb(event);
	}
}

static void freertos_sta_net_msg_process(uint32_t event, uint32_t data, void *arg)
{
	int need_config = 0;
	struct netif *nif = g_wlan_netif;
	uint16_t type = EVENT_SUBTYPE(event);
	int ret = -1;
	WMG_DEBUG("msg <%s>\n", net_ctrl_msg_str[type]);

	WMG_DEBUG("event : %d \n", type);
	wlan_ext_stats_code_get_t param;
	switch (type) {
	case NET_CTRL_MSG_WLAN_CONNECTED:
		sta_event_notify_to_sta_dev(WIFI_CONNECTED);
		connect_status = 1;
		break;
	case NET_CTRL_MSG_WLAN_DISCONNECTED:
		sta_event_notify_to_sta_dev(WIFI_DISCONNECTED);
		connect_status = 0;
		break;
	case NET_CTRL_MSG_WLAN_SCAN_SUCCESS:
		sta_event_notify_to_sta_dev(WIFI_SCAN_RESULTS);
		break;
	case NET_CTRL_MSG_WLAN_SCAN_FAILED:
		sta_event_notify_to_sta_dev(WIFI_SCAN_FAILED);
		break;
	case NET_CTRL_MSG_WLAN_4WAY_HANDSHAKE_FAILED:
		sta_event_notify_to_sta_dev(WIFI_4WAY_HANDSHAKE_FAILED);
		break;
	case NET_CTRL_MSG_WLAN_SSID_NOT_FOUND:
		sta_event_notify_to_sta_dev(WIFI_NETWORK_NOT_FOUND);
		break;
	case NET_CTRL_MSG_WLAN_AUTH_TIMEOUT:
		sta_event_notify_to_sta_dev(WIFI_AUTH_TIMEOUT);
		break;
	case NET_CTRL_MSG_WLAN_ASSOC_TIMEOUT:
		sta_event_notify_to_sta_dev(WIFI_AUTH_FAILED);
		break;
	case NET_CTRL_MSG_WLAN_CONNECT_FAILED:
		sta_event_notify_to_sta_dev(WIFI_CONNECT_FAILED);
		break;
	case NET_CTRL_MSG_WLAN_DEV_HANG:
		break;
	case NET_CTRL_MSG_WLAN_CONNECTION_LOSS:
		break;
	case NET_CTRL_MSG_WLAN_ASSOC_FAILED:
		sta_event_notify_to_sta_dev(WIFI_ASSOC_FAILED);
		break;
	case NET_CTRL_MSG_WLAN_SAE_COMMIT_FAILED:
		break;
	case NET_CTRL_MSG_WLAN_SAE_CONFIRM_FAILED:
		break;
	case NET_CTRL_MSG_WLAN_AP_STA_DISCONNECTED:
		break;
	case NET_CTRL_MSG_WLAN_AP_STA_CONNECTED:
		break;
	case NET_CTRL_MSG_NETWORK_UP:
		ip_flag = 1;
		sta_event_notify_to_sta_dev(WIFI_DHCP_SUCCESS);
		break;
	case NET_CTRL_MSG_NETWORK_DOWN:
		ip_flag = 0;
		break;
	default:
		sta_event_notify_to_sta_dev(WIFI_UNKNOWN);
		WMG_WARNG("unknown msg (%u, %u)\n", type, data);
		break;
	}
}

//static __inline void wlan_print_ap(wlan_sta_ap_t *ap)
//{
//	printf("%02x:%02x:%02x:%02x:%02x:%02x  ssid=%-32.32s  "
//	    "beacon_int=%d  freq=%d  channel=%u  rssi=%d  level=%d  "
//	    "flags=%#010x  wpa_key_mgmt=%#010x  wpa_cipher=%#010x  "
//	    "wpa2_key_mgmt=%#010x  wpa2_cipher=%#010x\n",
//	    ap->bssid[0], ap->bssid[1],
//	    ap->bssid[2], ap->bssid[3],
//	    ap->bssid[4], ap->bssid[5],
//	    ap->ssid.ssid,
//	    ap->beacon_int,
//	    ap->freq,
//	    ap->channel,
//	    ap->rssi,
//	    ap->level,
//	    ap->wpa_flags,
//	    ap->wpa_key_mgmt,
//	    ap->wpa_cipher,
//	    ap->wpa2_key_mgmt,
//	    ap->wpa2_cipher);
//}

static wmg_status_t freertos_sta_mode_init(sta_event_cb_t sta_event_cb, void *para)
{
	WMG_DEBUG("sta mode init\n");
	if (wlan_get_init_status() == WLAN_STATUS_NO_INIT) {
		net_core_init();
		wlan_set_init_status(WLAN_STATUS_INITED);
	}
	if (sta_event_cb != NULL){
		sta_inf_object.sta_event_cb = sta_event_cb;
	}
	return WMG_STATUS_SUCCESS;
}

static wmg_status_t freertos_sta_mode_enable()
{
	WMG_DEBUG("sta mode enable\n");
	int ret = 0;
	ret = net_switch_mode(WLAN_MODE_STA);
	if (ret)
		return WMG_STATUS_FAIL;

	observer_base *ob = sys_callback_observer_create(CTRL_MSG_TYPE_NETWORK, NET_CTRL_MSG_ALL, freertos_sta_net_msg_process, NULL);
	if (ob == NULL) {
		WMG_ERROR("freertos callback observer create failed\n");
		return WMG_STATUS_FAIL;
	}
	if (sys_ctrl_attach(ob) != 0) {
		WMG_ERROR("freertos callback observer attach failed\n");
		sys_callback_observer_destroy(ob);
		return WMG_STATUS_FAIL;
	} else {
		//use sta_private_data to save observer_base point
		sta_inf_object.sta_private_data = ob;
	}

	return WMG_STATUS_SUCCESS;
}

static wmg_status_t freertos_sta_mode_disable()
{
	/* nothing to do in freertos */
	return WMG_STATUS_SUCCESS;
}

static wmg_status_t freertos_sta_mode_deinit(void *para)
{
	WMG_DEBUG("sta mode deinit\n");
	observer_base *ob = (observer_base *)sta_inf_object.sta_private_data;
	if(ob) {
		sys_ctrl_detach(ob);
		sys_callback_observer_destroy(ob);
		sta_inf_object.sta_private_data = NULL;
	}
	if(g_wlan_netif) {
		net_close(g_wlan_netif);
		g_wlan_netif = NULL;
	}
	return WMG_STATUS_SUCCESS;
}

static wmg_status_t freertos_sta_disconnect()
{
	/* disconnect ap and report WIFI_DISCONNECTED to wifimg */
	WMG_DEBUG("sta disconnect\n");
	wlan_sta_states_t state;
	struct netif *nif = g_wlan_netif;
	wlan_sta_disable();
	usleep(1000 * 1000);
	wlan_sta_state(&state);
	if(state) {
		WMG_DEBUG("disconnect failed\n");
		return WMG_STATUS_FAIL;
	}

	/* release ip address when disconnect */
	net_config(nif, 0);

	return WMG_STATUS_SUCCESS;
}
#if 0
static wmg_status_t freertos_sta_try_get_ip_flag(void)
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
#endif

int wifi_wep_connect(const char *ssid, const char *passwd)
{
    uint8_t ssid_len;
	int sleep_times = SLEEP_TIMES;
    wlan_sta_config_t config;
    WMG_DEBUG("%s,ssid %s,passwd, %s\n", __func__, ssid, passwd);

    if (ssid)
        ssid_len = strlen(ssid);
    else
        goto err;


    if (ssid_len > WLAN_SSID_MAX_LEN)
        ssid_len = WLAN_SSID_MAX_LEN;

    memset(&config, 0, sizeof(config));

    /* ssid */
    config.field = WLAN_STA_FIELD_SSID;
    memcpy(config.u.ssid.ssid, ssid, ssid_len);
    config.u.ssid.ssid_len = ssid_len;
    if (wlan_sta_set_config(&config) != 0)
        goto err;

    /* WEP key0 */
    config.field = WLAN_STA_FIELD_WEP_KEY0;
    strlcpy((char *)config.u.wep_key, passwd, sizeof(config.u.wep_key));
    if (wlan_sta_set_config(&config) != 0)
        return -1;

    /* WEP key index */
    config.field = WLAN_STA_FIELD_WEP_KEY_INDEX;
    config.u.wep_tx_keyidx = 0;
    if (wlan_sta_set_config(&config) != 0)
        goto err;

    /* auth_alg: OPEN */
    config.field = WLAN_STA_FIELD_AUTH_ALG;
    config.u.auth_alg = WPA_AUTH_ALG_OPEN | WPA_AUTH_ALG_SHARED;
    if (wlan_sta_set_config(&config) != 0)
        goto err;

    /* key_mgmt: NONE */
    config.field = WLAN_STA_FIELD_KEY_MGMT;
    config.u.key_mgmt = WPA_KEY_MGMT_NONE;
    if (wlan_sta_set_config(&config) != 0)
        goto err;

    if (wlan_sta_enable()!= 0)
        goto err;

	while(sleep_times >= 6) {
		usleep(2000 * 1000);
		WMG_DEBUG("try wep connect\n");
		if (connect_status == 0) {
			wlan_sta_disable();
			WMG_DEBUG("%s, WPA_AUTH_ALG_SHARED\n", __func__);
			config.field = WLAN_STA_FIELD_AUTH_ALG;
			config.u.auth_alg = WPA_AUTH_ALG_SHARED;
			if (wlan_sta_set_config(&config) != 0)
				goto err;
			break;
		}
		if (connect_status == 1) {
			WMG_DEBUG("%s, WPA_AUTH_ALG_OPEN\n", __func__);
			break;
		}
		sleep_times--;
    }

    return 0;

err:
    WMG_ERROR("connect ap failed\n");
    return -1;
}

static wmg_status_t freertos_sta_connect(wifi_sta_cn_para_t *cn_para)
{
	WMG_DEBUG("sta connect\n");
	int sleep_times = SLEEP_TIMES;
#ifdef CONFIG_ARCH_SUN20IW2P1
	wlan_sta_states_t state;
	int ret;
	char *sec_buf;
	uint32_t flag;
	flag = 0; /* TODO: think about WPA3*/
	wlan_sta_state(&state);
	WMG_DEBUG("wifi state: %d\n", state);
	if (state != 0){
		wlan_sta_disable();
	}

	/* judge wifi encryption method */
	WMG_DEBUG("ssid: %s, psk: %s sec: %d\n", cn_para->ssid, cn_para->password, cn_para->sec);
	switch (cn_para->sec) {
		case WIFI_SEC_NONE:
			ret = wlan_sta_set((uint8_t *)cn_para->ssid, strlen(cn_para->ssid),(uint8_t *)cn_para->password);
			sec_buf = "WIFI_SEC_NONE";
			break;
		case WIFI_SEC_WPA_PSK:
			ret = wlan_sta_set((uint8_t *)cn_para->ssid, strlen(cn_para->ssid),(uint8_t *)cn_para->password);
			sec_buf = "WIFI_SEC_WPA_PSK";
			break;
		case WIFI_SEC_WPA2_PSK:
			ret = wlan_sta_set((uint8_t *)cn_para->ssid, strlen(cn_para->ssid),(uint8_t *)cn_para->password);
			sec_buf = "WIFI_SEC_WPA2_PSK";
			break;
		case WIFI_SEC_WPA3_PSK:
			ret = wlan_sta_set((uint8_t *)cn_para->ssid, strlen(cn_para->ssid),(uint8_t *)cn_para->password);
			sec_buf = "WIFI_SEC_WPA3_PSK";
			break;
		case WIFI_SEC_WEP:
			wifi_wep_connect(cn_para->ssid, cn_para->password);
			sec_buf = "WIFI_SEC_WEP";
			break;

		default:
			WMG_ERROR("unknown key mgmt\n");
			return WMG_STATUS_FAIL;
	}

	wlan_sta_enable();

	while(sleep_times >= 6) {
		wlan_sta_state(&state);
		if(state){
			break;
		}
		sleep_times--;
		usleep(1000 * 1000);
	}

	/* wifi connected,try get ip. */
	if(state){
		sta_event_notify_to_sta_dev(WIFI_DHCP_START);
		sleep_times = SLEEP_TIMES;
		while(sleep_times >= 1) {
			if(ip_flag){
#if 1
				/* if file existed, we should not create new one */
				int fd;
				int n_write;
				int n_read;
				char *buf_temp = ":";
				char *read_buf;

				if(!access("/data/wpa_supplicant.conf", R_OK))
				{
					WMG_DEBUG("wpa_supplicant.conf file is existed\n");
					fd = open("/data/wpa_supplicant.conf", O_RDWR | O_TRUNC);
					n_write = 0;
					n_write += write(fd, cn_para->ssid, strlen(cn_para->ssid));
					n_write += write(fd, buf_temp, strlen(buf_temp));
					n_write += write(fd, cn_para->password, strlen(cn_para->password));
					n_write += write(fd, buf_temp, strlen(buf_temp));
					n_write += write(fd, sec_buf, strlen(sec_buf));
					if(n_write != -1)
						WMG_DEBUG("write %d byte to wpa_supplicant.conf\n", n_write);
					close(fd);
				}else{
					WMG_DEBUG("wpa_supplicant.conf file is not existed\n");
					fd = open("/data/wpa_supplicant.conf", O_RDWR | O_CREAT | O_WRONLY | O_TRUNC, 0666);
					n_write = 0;
					n_write += write(fd, cn_para->ssid, strlen(cn_para->ssid));
					n_write += write(fd, buf_temp, strlen(buf_temp));
					n_write += write(fd, cn_para->password, strlen(cn_para->password));
					n_write += write(fd, buf_temp, strlen(buf_temp));
					n_write += write(fd, sec_buf, strlen(sec_buf));
					if(n_write != -1)
						WMG_DEBUG("write %d byte to wpa_supplicant.conf\n", n_write);
					close(fd);
				}

				WMG_DEBUG("ssid: %s password: %s\n", cn_para->ssid, cn_para->password);
				fd = open("/data/wpa_supplicant.conf", O_RDWR);
				read_buf = (char *)malloc(n_write+1);
				memset(read_buf, 0, n_write + 1);
				n_read = read(fd, read_buf, n_write);
				WMG_DEBUG("read %d, context:%s\n", n_read, read_buf);
				close(fd);
#endif
				return WMG_STATUS_SUCCESS;
			}
			sleep_times--;
			usleep(1000 * 1000);
		}
		sta_event_notify_to_sta_dev(WIFI_DHCP_TIMEOUT);
		ip_flag = 0;
		return WMG_STATUS_FAIL;
	}
	sta_event_notify_to_sta_dev(WIFI_CONNECT_TIMEOUT);
	/* disable wlan sta, or driver will always try connect. */
	wlan_sta_disable();
	ip_flag = 0;
	return WMG_STATUS_FAIL;
#endif

#ifdef CONFIG_ARCH_SUN8IW18P1
	WMG_ERROR("ssid:%s, passwd:%s\n", cn_para->ssid, cn_para->password);
	wifi_connect(cn_para->ssid, cn_para->password);
	sta_event_notify_to_sta_dev(WIFI_CONNECTED);
#endif
}

static wmg_status_t freertos_sta_auto_connect()
{
	WMG_DEBUG("sta auto connect\n");
	int sleep_times = SLEEP_TIMES;
	wlan_sta_states_t state;
	wlan_sta_enable();
	while(sleep_times >= 6) {
		wlan_sta_state(&state);
		if(state){
			break;
		}
		sleep_times--;
		usleep(1000 * 1000);
	}

	/* wifi reconnected,try get ip. */
	if(state){
		sta_event_notify_to_sta_dev(WIFI_DHCP_START);
		sleep_times = SLEEP_TIMES;
		while(sleep_times >= 1) {
			if(ip_flag){
				return WMG_STATUS_SUCCESS;
			}
			sleep_times--;
			usleep(1000 * 1000);
		}
		sta_event_notify_to_sta_dev(WIFI_DHCP_TIMEOUT);
		ip_flag = 0;
		return WMG_STATUS_FAIL;
	}

	sta_event_notify_to_sta_dev(WIFI_CONNECT_TIMEOUT);
	ip_flag = 0;
	return WMG_STATUS_FAIL;
}

static wmg_status_t freertos_sta_get_info(wifi_sta_info_t *sta_info)
{
	WMG_DEBUG("sta get info\n");
	int ret;
	struct netif *nif = g_wlan_netif;

	wlan_sta_ap_t *ap = cmd_malloc(sizeof(wlan_sta_ap_t));
	if (ap == NULL) {
		WMG_ERROR("no mem\n");
		return WMG_STATUS_FAIL;
	}
	ret = wlan_sta_ap_info(ap);
	if (ret == 0) {
		//wlan_print_ap(ap);
		sta_info->id = -1;
		sta_info->freq = ap->freq;
		sta_info->rssi = ap->rssi;
		memcpy(sta_info->bssid, ap->bssid, (sizeof(uint8_t) * 6));
		memcpy(sta_info->ssid, ap->ssid.ssid, ap->ssid.ssid_len);

		if (NET_IS_IP4_VALID(nif) && netif_is_link_up(nif)) {
			sta_info->ip_addr[0] = (nif->ip_addr.addr & 0xff);
			sta_info->ip_addr[1] = ((nif->ip_addr.addr >> 8) & 0xff);
			sta_info->ip_addr[2] = ((nif->ip_addr.addr >> 16) & 0xff);
			sta_info->ip_addr[3] = ((nif->ip_addr.addr >> 24) & 0xff);

			sta_info->gw_addr[0] = (nif->gw.addr & 0xff);
			sta_info->gw_addr[1] = ((nif->gw.addr >> 8) & 0xff);
			sta_info->gw_addr[2] = ((nif->gw.addr >> 16) & 0xff);
			sta_info->gw_addr[3] = ((nif->gw.addr >> 24) & 0xff);

		}

		if(ap->wpa_flags & WPA_FLAGS_WPA) {
			sta_info->sec = WIFI_SEC_WPA_PSK;
		} else if ((ap->wpa_flags & (WPA_FLAGS_RSN | WPA_FLAGS_SAE)) == WPA_FLAGS_RSN) {
			sta_info->sec = WIFI_SEC_WPA2_PSK;
		} else if ((ap->wpa_flags & (WPA_FLAGS_RSN | WPA_FLAGS_SAE)) == (WPA_FLAGS_RSN | WPA_FLAGS_SAE)) {
			sta_info->sec = WIFI_SEC_WPA3_PSK;
		} else if (ap->wpa_flags & WPA_FLAGS_WEP) {
			sta_info->sec = WIFI_SEC_WEP;
		} else {
			sta_info->sec = WIFI_SEC_NONE;
		}
	}
	cmd_free(ap);
	return WMG_STATUS_SUCCESS;
}

static wmg_status_t freertos_sta_scan_networks(get_scan_results_para_t *sta_scan_results_para)
{
	WMG_DEBUG("sta scan\n");
	int ret, i;
	/* set scan num max, default 50 */
	int size = 50;
	if ((sta_scan_results_para->arr_size != 0) && (sta_scan_results_para->arr_size > size))
	{
		WMG_DEBUG("sta scan bss_num_max=%d\n", sta_scan_results_para->arr_size);
		size = sta_scan_results_para->arr_size;
	}
	wlan_sta_bss_max_count((uint8_t)size);

	wlan_sta_scan_once();
	usleep(1000*1000);
	wlan_sta_scan_results_t results;
	results.ap = cmd_malloc(size * sizeof(wlan_sta_ap_t));
	if (results.ap == NULL) {
		WMG_ERROR("no mem\n");
		return WMG_STATUS_FAIL;
	}
	results.size = size;
	ret = wlan_sta_scan_result(&results);
	if (ret == 0) {
		*(sta_scan_results_para->bss_num) = results.num;
		for(i = 0; i < results.num; ++i) {
			memcpy(sta_scan_results_para->scan_results[i].bssid, results.ap[i].bssid, (sizeof(uint8_t) * 6));
			memcpy(sta_scan_results_para->scan_results[i].ssid, results.ap[i].ssid.ssid, SSID_MAX_LEN);
			sta_scan_results_para->scan_results[i].ssid[SSID_MAX_LEN] = '\0';
			sta_scan_results_para->scan_results[i].freq = (uint32_t)(results.ap[i].freq);
			sta_scan_results_para->scan_results[i].rssi = results.ap[i].level;
			sta_scan_results_para->scan_results[i].key_mgmt = WIFI_SEC_NONE;
			if(results.ap[i].wpa_flags & WPA_FLAGS_WPA) {
				sta_scan_results_para->scan_results[i].key_mgmt = WIFI_SEC_WPA_PSK;
			}
			if ((results.ap[i].wpa_flags & (WPA_FLAGS_RSN | WPA_FLAGS_SAE)) == WPA_FLAGS_RSN) {
				sta_scan_results_para->scan_results[i].key_mgmt |= WIFI_SEC_WPA2_PSK;
			}
			if ((results.ap[i].wpa_flags & (WPA_FLAGS_RSN | WPA_FLAGS_SAE)) == (WPA_FLAGS_RSN | WPA_FLAGS_SAE)) {
				sta_scan_results_para->scan_results[i].key_mgmt |= WIFI_SEC_WPA3_PSK;
			}
			if (results.ap[i].wpa_flags & WPA_FLAGS_WEP) {
				sta_scan_results_para->scan_results[i].key_mgmt |= WIFI_SEC_WEP;
			}
		}
		cmd_free(results.ap);
		return WMG_STATUS_SUCCESS;
	} else {
		cmd_free(results.ap);
		return WMG_STATUS_FAIL;
	}
}

static wmg_status_t freertos_sta_remove_network(char *ssid)
{
	WMG_DEBUG("sta remove network\n");
	// check /data/wpa_supplicant.conf file is exist.
	if(access("/data/wpa_supplicant.conf", R_OK))
	{
		WMG_DEBUG("wpa_supplicant.conf file is not existed\n");
		return WMG_STATUS_FAIL;
	}

	// check /data/wpa_supplicant.conf file is empty.
	FILE* file1 = fopen("/data/wpa_supplicant.conf", "r");
	int c = fgetc(file1);
	if (c == EOF) {
		WMG_DEBUG("wpa_supplicant.conf file is empty\n");
		fclose(file1);
		return WMG_STATUS_FAIL;
	}
	fclose(file1);

	/*ToDo: check ap is exist in wpa_supplicant.conf */
	int file2;
	if (ssid != NULL) {
		file2 = open("/data/wpa_supplicant.conf", O_RDWR | O_TRUNC);
		WMG_DEBUG("remove network (%s) in /data/wpa_supplicant.conf file\n", ssid);
	} else {
		file2 = open("/data/wpa_supplicant.conf", O_RDWR | O_TRUNC);
		WMG_DEBUG("remove all networks in /data/wpa_supplicant.conf file\n");
	}
	close(file2);
	return WMG_STATUS_SUCCESS;
}

static wmg_status_t freertos_platform_extension(int cmd, void* cmd_para,int *erro_code)
{
	WMG_DEBUG("platform extension\n");
	switch (cmd) {
	case STA_CMD_GET_INFO:
		return freertos_sta_get_info((wifi_sta_info_t *)cmd_para);
	case STA_CMD_CONNECT:
		return freertos_sta_connect((wifi_sta_cn_para_t *)cmd_para);
	case STA_CMD_DISCONNECT:
			return freertos_sta_disconnect();
	case STA_CMD_LIST_NETWORKS:
			return WMG_STATUS_UNSUPPORTED;
	case STA_CMD_REMOVE_NETWORKS:
			return freertos_sta_remove_network((char *)cmd_para);
	case STA_CMD_SET_AUTO_RECONN:
			return freertos_sta_auto_connect();
	case STA_CMD_GET_SCAN_RESULTS:
			return freertos_sta_scan_networks((get_scan_results_para_t *)cmd_para);
	default:
			return WMG_FALSE;
	}
	return WMG_FALSE;
}

static wmg_sta_inf_object_t sta_inf_object = {
	.sta_init_flag = WMG_FALSE,
	.sta_auto_reconn = WMG_FALSE,
	.sta_event_cb = NULL,
	.sta_private_data = NULL,

	.sta_inf_init = freertos_sta_mode_init,
	.sta_inf_deinit = freertos_sta_mode_deinit,
	.sta_inf_enable = freertos_sta_mode_enable,
	.sta_inf_disable = freertos_sta_mode_disable,
	.sta_platform_extension = freertos_platform_extension,
};

wmg_sta_inf_object_t* sta_rtos_inf_object_register(void)
{
	return &sta_inf_object;
}
