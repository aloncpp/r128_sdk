#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#if defined(OS_NET_LINUX_OS) || defined(OS_NET_XRLINK_OS)
#include <sys/socket.h>
#include <sys/un.h>
#endif
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <wifimg.h>
#include <wifi_log.h>
#include <fcntl.h>
#include <sys/times.h>
#include "wifi_daemon.h"

#ifdef DEFAULT_DEBUG_LV_ERROR
#define DEFAULT_DEBUG_LEVEL 1
#elif DEFAULT_DEBUG_LV_WARN
#define DEFAULT_DEBUG_LEVEL 2
#elif DEFAULT_DEBUG_LV_INFO
#define DEFAULT_DEBUG_LEVEL 3
#elif DEFAULT_DEBUG_LV_DEBUG
#define DEFAULT_DEBUG_LEVEL 4
#elif DEFAULT_DEBUG_LV_DUMP
#define DEFAULT_DEBUG_LEVEL 5
#elif DEFAULT_DEBUG_LV_EXCE
#define DEFAULT_DEBUG_LEVEL 6
#else
#define DEFAULT_DEBUG_LEVEL 3
#endif

#define DATA_BUF_SIZE    128

#define LIST_NETWORK_DEFAUT_NUM   5

#define MONITOR_DEFAULT_CHANNEL   6

#define BAND_NOME     0
#define BAND_2_4G     1
#define BAND_5G       2

static deamon_object_t deamon_object;

#if defined(OS_NET_LINUX_OS) || defined(OS_NET_XRLINK_OS)
#define RES_LEN          60
#define PROC_NAME "wifi_daemon"
#define PID_FILE_PATH "/var/run/"

#define UNIX_DOMAIN "/tmp/UNIX_WIFI.domain"
#define READ_FROM_CLIENT 0X01
#define WRITE_TO_CLIENT  0x02

//Calculate system time
int sc_clk_tck;
struct tms time_tms;
#define USE_TIME(a,b) ((((b - a) / (double)sc_clk_tck)) * 1000)
#define GET_TIME times(&time_tms)

#else

#define RES_LEN          50
static int rtos_wifimanager_init = 0;
#define USE_TIME(a,b) (b - a)
#define GET_TIME 0
#endif

static uint8_t char2uint8(char* trs)
{
	uint8_t ret = 0;
	uint8_t tmp_ret[2] = {0};
	int i = 0;
	for(; i < 2; i++) {
		switch (*(trs + i)) {
			case '0' :
				tmp_ret[i] = 0x0;
				break;
			case '1' :
				tmp_ret[i] = 0x1;
				break;
			case '2' :
				tmp_ret[i] = 0x2;
				break;
			case '3' :
				tmp_ret[i] = 0x3;
				break;
			case '4' :
				tmp_ret[i] = 0x4;
				break;
			case '5' :
				tmp_ret[i] = 0x5;
				break;
			case '6' :
				tmp_ret[i] = 0x6;
				break;
			case '7' :
				tmp_ret[i] = 0x7;
				break;
			case '8' :
				tmp_ret[i] = 0x8;
				break;
			case '9' :
				tmp_ret[i] = 0x9;
				break;
			case 'a' :
			case 'A' :
				tmp_ret[i] = 0xa;
				break;
			case 'b' :
			case 'B' :
				tmp_ret[i] = 0xb;
				break;
			case 'c' :
			case 'C' :
				tmp_ret[i] = 0xc;
				break;
			case 'd' :
			case 'D' :
				tmp_ret[i] = 0xd;
				break;
			case 'e' :
			case 'E' :
				tmp_ret[i] = 0xe;
				break;
			case 'f' :
			case 'F' :
				tmp_ret[i] = 0xf;
		break;
		}
	}
	ret = ((tmp_ret[0] << 4) | tmp_ret[1]);
	return ret;
}

static void uint8tochar(char *mac_addr_char, uint8_t *mac_addr_uint8)
{
	sprintf(mac_addr_char,"%02x:%02x:%02x:%02x:%02x:%02x",
						(unsigned char)mac_addr_uint8[0],
						(unsigned char)mac_addr_uint8[1],
						(unsigned char)mac_addr_uint8[2],
						(unsigned char)mac_addr_uint8[3],
						(unsigned char)mac_addr_uint8[4],
						(unsigned char)mac_addr_uint8[5]);
	mac_addr_char[17] = '\0';
}

static int isdigitstr(char *str)
{
	return (strspn(str, "0123456789")==strlen(str));
}

#ifndef UNREGISTER_CB
static char sta_msg_cb_char[32] = "sta msg cb arg";

#ifdef SUPPORT_AP_MODE
static char ap_msg_cb_char[32] = "ap msg cb arg";
#endif
#ifdef SUPPORT_MONITOR_MODE
static char monitor_msg_cb_char[32] = "mon msg cb arg";
#endif
#ifdef SUPPORT_P2P_MODE
static char p2p_msg_cb_char[32] = "p2p msg cb arg";
#endif

//Callback functions cannot do high-load and blocking actions
void wifi_msg_cb(wifi_msg_data_t *msg)
{
	char msg_cb_arg[32] = "NULL";
	char *msg_cb_arg_p;
	//msg cb test, need to pass static parameters
	if(msg->private_data){
		msg_cb_arg_p = (char *)msg->private_data;
	} else {
		msg_cb_arg_p = msg_cb_arg;
	}
	WMG_DEBUG("** wmg cb: ");
	switch(msg->id) {
		case WIFI_MSG_ID_DEV_STATUS:
			WMG_DEBUG_NONE("dev ");
			switch(msg->data.d_status) {
				case WLAN_STATUS_DOWN:
					WMG_DEBUG_NONE("down ");
					break;
				case WLAN_STATUS_UP:
					WMG_DEBUG_NONE("up ");
					break;
				default:
					WMG_DEBUG_NONE("unknow ");
					break;
			}
			WMG_DEBUG_NONE("arg:%s\n", msg_cb_arg_p);
			break;
#ifdef SUPPORT_STA_MODE
		case WIFI_MSG_ID_STA_CN_EVENT:
			WMG_DEBUG_NONE("sta event ");
			switch(msg->data.event) {
				case WIFI_DISCONNECTED:
					WMG_DEBUG_NONE("disconnect ");
					break;
				case WIFI_SCAN_STARTED:
					WMG_DEBUG_NONE("scan started ");
					break;
				case WIFI_SCAN_FAILED:
					WMG_DEBUG_NONE("scan failed ");
					break;
				case WIFI_SCAN_RESULTS:
					WMG_DEBUG_NONE("scan results ");
					break;
				case WIFI_NETWORK_NOT_FOUND:
					WMG_DEBUG_NONE("network not found ");
					break;
				case WIFI_PASSWORD_INCORRECT:
					WMG_DEBUG_NONE("password incorrect ");
					break;
				case WIFI_AUTHENTIACATION:
					WMG_DEBUG_NONE("authentiacation ");
					break;
				case WIFI_AUTH_REJECT:
					WMG_DEBUG_NONE("auth reject ");
					break;
				case WIFI_ASSOCIATING:
					WMG_DEBUG_NONE("associating ");
					break;
				case WIFI_ASSOC_REJECT:
					WMG_DEBUG_NONE("assoc reject ");
					break;
				case WIFI_ASSOCIATED:
					WMG_DEBUG_NONE("associated ");
					break;
				case WIFI_4WAY_HANDSHAKE:
					WMG_DEBUG_NONE("4 way handshake ");
					break;
				case WIFI_GROUNP_HANDSHAKE:
					WMG_DEBUG_NONE("grounp handshake ");
					break;
				case WIFI_GROUNP_HANDSHAKE_DONE:
					WMG_DEBUG_NONE("handshake done ");
					break;
				case WIFI_CONNECTED:
					WMG_DEBUG_NONE("connected ");
					break;
				case WIFI_CONNECT_TIMEOUT:
					WMG_DEBUG_NONE("connect timeout ");
					break;
				case WIFI_DEAUTH:
					WMG_DEBUG_NONE("deauth ");
					break;
				case WIFI_DHCP_START:
					WMG_DEBUG_NONE("dhcp start ");
					break;
				case WIFI_DHCP_TIMEOUT:
					WMG_DEBUG_NONE("dhcp timeout ");
					break;
				case WIFI_DHCP_SUCCESS:
					WMG_DEBUG_NONE("dhcp success ");
					break;
				case WIFI_TERMINATING:
					WMG_DEBUG_NONE("terminating ");
					break;
				case WIFI_UNKNOWN:
				default:
					WMG_DEBUG_NONE("unknow ");
					break;
			}
			WMG_DEBUG_NONE("arg:%s\n", msg_cb_arg_p);
			break;
		case WIFI_MSG_ID_STA_STATE_CHANGE:
			WMG_DEBUG_NONE("sta state ");
			switch(msg->data.state) {
				case WIFI_STA_IDLE:
					WMG_DEBUG_NONE("idle ");
					break;
				case WIFI_STA_CONNECTING:
					WMG_DEBUG_NONE("connecting ");
					break;
				case WIFI_STA_CONNECTED:
					WMG_DEBUG_NONE("connected ");
					break;
				case WIFI_STA_OBTAINING_IP:
					WMG_DEBUG_NONE("obtaining ip ");
					break;
				case WIFI_STA_NET_CONNECTED:
					WMG_DEBUG_NONE("net connected ");
					break;
				case WIFI_STA_DHCP_TIMEOUT:
					WMG_DEBUG_NONE("dhcp timeout ");
					break;
				case WIFI_STA_DISCONNECTING:
					WMG_DEBUG_NONE("disconnecting ");
					break;
				case WIFI_STA_DISCONNECTED:
					WMG_DEBUG_NONE("disconnected ");
					break;
				default:
					WMG_DEBUG_NONE("unknow ");
					break;
			}
			WMG_DEBUG_NONE("arg:%s\n", msg_cb_arg_p);
			break;
#endif
#ifdef SUPPORT_AP_MODE
		case WIFI_MSG_ID_AP_CN_EVENT:
			WMG_DEBUG_NONE("ap ");
			switch(msg->data.ap_event) {
				case WIFI_AP_ENABLED:
					WMG_DEBUG_NONE("enable ");
					break;
				case WIFI_AP_DISABLED:
					WMG_DEBUG_NONE("disable ");
					break;
				case WIFI_AP_STA_DISCONNECTED:
					WMG_DEBUG_NONE("sta disconnected ");
					break;
				case WIFI_AP_STA_CONNECTED:
					WMG_DEBUG_NONE("sta connected ");
					break;
				case WIFI_AP_UNKNOWN:
				default:
					WMG_DEBUG_NONE("event unknow ");
					break;
			}
			WMG_DEBUG_NONE("arg:%s\n", msg_cb_arg_p);
			break;
#endif
#ifdef SUPPORT_MONITOR_MODE
		case WIFI_MSG_ID_MONITOR:
			WMG_DEBUG_NONE("monitor ");
			switch(msg->data.mon_state) {
				case WIFI_MONITOR_DISABLE:
					WMG_DEBUG_NONE("disable arg:%s\n", msg_cb_arg_p);
					break;
				case WIFI_MONITOR_ENABLE:
					WMG_DEBUG_NONE("enable arg:%s\n", msg_cb_arg_p);
					break;
				default:
					WMG_DEBUG_NONE("frame arg:%s\ndata channel %d\ndata len %d\n",
							msg_cb_arg_p, msg->data.frame->channel, msg->data.frame->len);
					break;
			}
			break;
#endif
#ifdef SUPPORT_P2P_MODE
		case WIFI_MSG_ID_P2P_CN_EVENT:
			WMG_DEBUG_NONE("p2p event ");
			switch(msg->data.event) {
				case WIFI_P2P_DEV_FOUND:
					WMG_DEBUG_NONE("dev found ");
					break;
				case WIFI_P2P_DEV_LOST:
					WMG_DEBUG_NONE("dev lost ");
					break;
				case WIFI_P2P_PBC_REQ:
					WMG_DEBUG_NONE("pbc req ");
					break;
				case WIFI_P2P_GO_NEG_RQ:
					WMG_DEBUG_NONE("go neg rq ");
					break;
				case WIFI_P2P_GO_NEG_SUCCESS:
					WMG_DEBUG_NONE("go neg success ");
					break;
				case WIFI_P2P_GO_NEG_FAILURE:
					WMG_DEBUG_NONE("go neg failure ");
					break;
				case WIFI_P2P_GROUP_FOR_SUCCESS:
					WMG_DEBUG_NONE("group for success ");
					break;
				case WIFI_P2P_GROUP_FOR_FAILURE:
					WMG_DEBUG_NONE("group for failure ");
					break;
				case WIFI_P2P_GROUP_STARTED:
					WMG_DEBUG_NONE("group started ");
					break;
				case WIFI_P2P_GROUP_REMOVED:
					WMG_DEBUG_NONE("group remove ");
					break;
				case WIFI_P2P_CROSS_CONNECT_ENABLE:
					WMG_DEBUG_NONE("cross connect enable ");
					break;
				case WIFI_P2P_CROSS_CONNECT_DISABLE:
					WMG_DEBUG_NONE("cross connect disable ");
					break;
				case WIFI_P2P_SCAN_RESULTS:
					WMG_DEBUG_NONE("scan results ");
					break;
				case WIFI_P2P_GROUP_DHCP_DNS_FAILURE:
					WMG_DEBUG_NONE("group dhcp dns failure ");
					break;
				case WIFI_P2P_GROUP_DHCP_SUCCESS:
					WMG_DEBUG_NONE("group dhcp success ");
					break;
				case WIFI_P2P_GROUP_DHCP_FAILURE:
					WMG_DEBUG_NONE("group dhcp failure ");
					break;
				case WIFI_P2P_GROUP_DNS_SUCCESS:
					WMG_DEBUG_NONE("group dns success ");
					break;
				case WIFI_P2P_GROUP_DNS_FAILURE:
					WMG_DEBUG_NONE("group dns failure ");
					break;
				case WIFI_P2P_UNKNOWN:
				default:
					WMG_DEBUG_NONE("unknow ");
					break;
			}
			WMG_DEBUG_NONE("arg:%s\n", msg_cb_arg_p);
			break;
		case WIFI_MSG_ID_P2P_STATE_CHANGE:
			WMG_DEBUG_NONE("p2p state ");
			switch(msg->data.state) {
				case WIFI_P2P_ENABLE:
					WMG_DEBUG_NONE("enable ");
					break;
				case WIFI_P2P_DISABLE:
					WMG_DEBUG_NONE("disable ");
					break;
				case WIFI_P2P_CONNECTD_GC:
					WMG_DEBUG_NONE("connected gc");
					break;
				case WIFI_P2P_CONNECTD_GO:
					WMG_DEBUG_NONE("connected go");
					break;
				case WIFI_P2P_DISCONNECTD:
					WMG_DEBUG_NONE("disconnected ");
					break;
				default:
					WMG_DEBUG_NONE("unknow ");
					break;
			}
			WMG_DEBUG_NONE("arg:%s\n", msg_cb_arg_p);
			break;
#endif
		default:
			break;
	}
}
#endif //UNREGISTER_CB

static int cmd_handle_o(char *p)
{
	int ret = -1;
	int ret_cb = -1;
	char *ssid_p = NULL;
	char *ssid_tmp_p = NULL;
	int ssid_len = 0;
	int ssid_total_len = 0;
	char *psk_p = NULL;
	char *channel_p = NULL;
	char *tmp_p2p_p = NULL;
	char *delim = " ";

	char ssid_buf[SSID_MAX_LEN + 1] = "allwinner-ap";
	char psk_buf[PSK_MAX_LEN + 1] = "Aa123456";
	wifi_ap_config_t ap_config;
	ap_config.ssid = ssid_buf;
	ap_config.psk = psk_buf;
	ap_config.sec = WIFI_SEC_WPA2_PSK;
	ap_config.channel = 6;
	int monitor_channel = MONITOR_DEFAULT_CHANNEL;

	char p2p_dev_buf[P2P_DEV_NAME_MAX_LEN + 1] = "allwinner-p2p-dev";
	wifi_p2p_config_t p2p_config;
	p2p_config.dev_name = p2p_dev_buf;
	p2p_config.listen_time = 0;
	p2p_config.p2p_go_intent = -1;
	p2p_config.auto_connect = WMG_FALSE;

	WMG_DEBUG("cmd handle o: %s\n",p);
	if(!strncmp(p, "sta", 3)) {
		WMG_INFO("open sta mode\n");
		ret = wifi_on(WIFI_STATION);
		if (ret == 0) {
			deamon_object.deamon_current_mode = WIFI_STATION;
			WMG_INFO("wifi on sta success\n");
#ifndef UNREGISTER_CB
			WMG_DEBUG("wifi register msg cb\n");
			//msg cb arg, need to pass static parameters
			ret_cb = wifi_register_msg_cb(wifi_msg_cb, (void *)sta_msg_cb_char);
			if(ret_cb) {
				WMG_ERROR("register msg cb failed\n");
			}
#endif
			return 0;
		} else {
			WMG_ERROR("wifi on sta failed\n");
			return -1;
		}
#ifdef SUPPORT_AP_MODE
	} else if(!strncmp(p, "ap", 2)) {
		WMG_INFO("open ap mode\n");
		if(strlen(p) > 2) {
			memset(ssid_buf, '\0', (SSID_MAX_LEN + 1));
			memset(psk_buf, '\0', (PSK_MAX_LEN + 1));
			p = p + 2;
			ssid_p = strtok(p, delim);
			ssid_tmp_p = ssid_p;
			ssid_len = strlen(ssid_p);
			ssid_total_len += ssid_len;
			while((ssid_total_len <= SSID_MAX_LEN) && (ssid_p != NULL)) {
				if(ssid_p[ssid_len - 1] == '\\') {
					ssid_p = strtok(NULL, delim);
					ssid_tmp_p[ssid_len - 1] = ' ';
					strcat(ssid_buf, ssid_tmp_p);
					ssid_tmp_p = ssid_p;
					ssid_len = strlen(ssid_p);
					ssid_total_len += ssid_len;
				} else {
					strcat(ssid_buf, ssid_p);
					break;
				}
			}
			if(ssid_total_len > SSID_MAX_LEN) {
				WMG_ERROR("Input ssid is longer than %d\n",SSID_MAX_LEN);
				return -1;
			}
			psk_p = strtok(NULL, delim);
			if((psk_p == NULL) || (strlen(psk_p) <= 3)) {
				strcpy(psk_buf, "NULL");
				WMG_INFO("open ap(%s) without psk\n",ssid_buf);
				ap_config.sec = WIFI_SEC_NONE;
				//When the length of the input is less than 3, the input may be a channel
				if((psk_p != NULL) && (strlen(psk_p) <= 3)) {
					ap_config.channel = atoi(psk_p);
					if(ap_config.channel > 13) {
						WMG_ERROR("channel cannot be greater than 13\n");
						return -1;
					}
				}
			} else {
				if((strlen(psk_p) > PSK_MAX_LEN) || (strlen(psk_p) < 8)) {
					WMG_ERROR("The length of psk(%d) needs to be between 8 and %d\n", strlen(psk_p), PSK_MAX_LEN);
					return -1;
				}
				strcpy(psk_buf, psk_p);
				WMG_INFO("open ap(%s) with psk(%s)\n",ssid_buf, psk_buf);
				channel_p = strtok(NULL, delim);
				if(channel_p != NULL) {
					ap_config.channel = atoi(channel_p);
				}
			}
		} else {
			WMG_INFO("open default ap\n");
		}
		ret = wifi_on(WIFI_AP);
		if (ret == 0) {
			deamon_object.deamon_current_mode = WIFI_AP;
			WMG_INFO("wifi on ap success\n");
#ifndef UNREGISTER_CB
			WMG_DEBUG("wifi register msg cb\n");
			//msg cb arg, need to pass static parameters
			ret_cb = wifi_register_msg_cb(wifi_msg_cb, (void *)ap_msg_cb_char);
			if(ret_cb) {
				WMG_ERROR("register msg cb failed\n");
			}
#endif
		} else {
			WMG_ERROR("wifi on ap failed\n");
			return -1;
		}
		ret = wifi_ap_enable(&ap_config);
		if (ret == 0) {
			WMG_INFO("ap enable success, ssid=%s, psk=%s, sec=%d, channel=%d\n", ap_config.ssid, ap_config.psk, ap_config.sec, ap_config.channel);
		} else {
			WMG_ERROR("ap enable failed\n");
			return -1;
		}
		return 0;
#endif
#ifdef SUPPORT_MONITOR_MODE
	} else if(!strncmp(p, "monitor", 7)) {
		WMG_INFO("open monitor mode\n");
		if(strlen(p) > 7) {
			p = p + 8;
			if(strlen(p) <= 3) {
				if(isdigitstr(p)){
					monitor_channel = atoi(p);
					if((monitor_channel > 13) || (monitor_channel <= 0)) {
						WMG_ERROR("channel(%d) is out of range 1 ~ 13\n", monitor_channel);
						return -1;
					}
				} else {
					WMG_ERROR("Unsupported channel format, channel(%s) is out of range 1 ~ 13\n", p);
					return -1;
				}
			} else {
				WMG_ERROR("Unsupported channel format, channel(%s) is out of range 1 ~ 13\n", p);
				return -1;
			}
		}
		ret = wifi_on(WIFI_MONITOR);
		if (ret == 0) {
			deamon_object.deamon_current_mode = WIFI_MONITOR;
			WMG_INFO("wifi on mon success\n");
#ifndef UNREGISTER_CB
			WMG_DEBUG("wifi register msg cb\n");
			//msg cb arg, need to pass static parameters
			ret_cb = wifi_register_msg_cb(wifi_msg_cb, (void *)monitor_msg_cb_char);
			if(ret_cb) {
				WMG_ERROR("register msg cb failed\n");
			}
#endif
		} else {
			WMG_ERROR("wifi on mon failed\n");
			return -1;
		}
		ret = wifi_monitor_enable((uint8_t)monitor_channel);
		if (ret == 0) {
			WMG_INFO("mon enable success, channel=%d\n", monitor_channel);
		} else {
			WMG_ERROR("mon enable failed\n");
			return -1;
		}
		return 0;
#endif
#ifdef SUPPORT_P2P_MODE
	} else if(!strncmp(p, "p2p", 3)) {
		WMG_INFO("open p2p mode\n");
		if(strlen(p) > 3){
			p = p + 4;
			tmp_p2p_p = strtok(p, delim);
			while(tmp_p2p_p != NULL) {
				switch (*tmp_p2p_p) {
					case 'N':
						strncpy(p2p_config.dev_name, (tmp_p2p_p + 2), P2P_DEV_NAME_MAX_LEN);
						p2p_config.dev_name[P2P_DEV_NAME_MAX_LEN] = '\0';
						break;
					case 'I':
						if(isdigitstr(tmp_p2p_p + 2)){
							p2p_config.p2p_go_intent = atoi(tmp_p2p_p + 2);
							if((p2p_config.p2p_go_intent > 15) || (p2p_config.p2p_go_intent < 0)) {
								WMG_ERROR("p2p_go_intent(%d) is out of range 0 ~ 15\n", p2p_config.p2p_go_intent);
								return -1;
							}
						} else {
							WMG_ERROR("Unsupported p2p_go_intent format, p2p_go_intent(%s) is out of range 0 ~ 15\n", p);
							return -1;
						}
						break;
					case 'T':
						if(isdigitstr(tmp_p2p_p + 2)){
							p2p_config.listen_time = atoi(tmp_p2p_p + 2);
							p2p_config.auto_connect = WMG_TRUE;
						}
						break;
				}
				tmp_p2p_p = strtok(NULL, delim);
			}
		}
		ret = wifi_on(WIFI_P2P);
		if (ret == 0) {
			deamon_object.deamon_current_mode = WIFI_P2P;
			WMG_INFO("wifi on p2p success\n");
			WMG_DEBUG("wifi register msg cb\n");
#ifndef UNREGISTER_CB
			//msg cb arg, need to pass static parameters
			ret_cb = wifi_register_msg_cb(wifi_msg_cb, (void *)p2p_msg_cb_char);
			if(ret_cb) {
				WMG_ERROR("register msg cb faile\n");
			}
#endif
		} else {
			WMG_ERROR("wifi on p2p failed\n");
			return -1;
		}
		WMG_DEBUG("p2p config: dev_name: %s go_intent: %d listen_time: %d auto_connect: %s\n",
				p2p_config.dev_name, p2p_config.p2p_go_intent, p2p_config.listen_time,
				(p2p_config.auto_connect == WMG_TRUE)?"enable":"disable");
		ret = wifi_p2p_enable(&p2p_config);
		WMG_INFO("p2p enable %s\n", (ret == 0)?"success":"faile");
		return ret;
#endif
	} else {
		WMG_ERROR("Unsupport wifi mode %s\n", p);
		return -1;
	}
}

static int cmd_handle_f(char *p)
{
	WMG_DEBUG("cmd handle f: %s\n",p);
	WMG_INFO("wmg off now\n");
	wifi_off();
#ifdef OS_NET_FREERTOS_OS
	wifimanager_deinit();
	rtos_wifimanager_init = 0;
#endif
	return 0;
}

static uint32_t freq_to_channel(uint32_t freq)
{
	int band;
	uint32_t channel = 0;
	if((freq >= 5180) && (freq <= 5825)){
		band = BAND_5G;
	} else if((freq >= 2407) && (freq <= 2484)){
		band = BAND_2_4G;
	} else {
		band = BAND_NOME;
	}
	switch (band) {
	case BAND_2_4G:
		if(freq == 2484) {
			channel = 14;
		} else if(freq == 2407) {
			channel = 0;
		} else if((freq <= 2472) && (freq > 2407)){
			if(((freq - 2407) % 5) == 0) {
				channel = ((freq - 2407) / 5);
			} else {
				channel = 1000;
			}
		} else {
			channel = 1000;
		}
		break;
	case BAND_5G:
		if(((freq - 5180) % 5) == 0) {
			channel = ((freq - 5180) / 5) + 36;
		} else {
			channel = 1000;
		}
		break;
	case BAND_NOME:
	default:
		channel = 1000;
		break;
	}
	return channel;
}

static char * key_mgmt_to_char(char *key_mgmt_buf, wifi_sec key_mgmt)
{
	if(key_mgmt & WIFI_SEC_WEP) {
		strcat(key_mgmt_buf, "[WEP]");
	}
	if(key_mgmt & WIFI_SEC_WPA_PSK) {
		strcat(key_mgmt_buf, "[WPA]");
	}
	if(key_mgmt & WIFI_SEC_WPA2_PSK) {
		strcat(key_mgmt_buf, "[WPA2]");
	}
	if(key_mgmt & WIFI_SEC_WPA3_PSK) {
		strcat(key_mgmt_buf, "[WPA3]");
	}
	if(key_mgmt == WIFI_SEC_NONE) {
		strcat(key_mgmt_buf, "[NONE]");
	}
	return key_mgmt_buf;
}

static void printf_mode_state(wifi_wmg_state_t wmg_state)
{
#ifdef SUPPORT_STA_MODE
	if(wmg_state.current_mode & WIFI_STATION) {
		WMG_INFO("* sta state: ");
		switch (wmg_state.sta_state) {
			case WIFI_STA_IDLE:
				WMG_INFO_NONE("IDLE\n");
				break;
				case WIFI_STA_CONNECTING:
				WMG_INFO_NONE("CONNECTING\n");
				break;
			case WIFI_STA_CONNECTED:
				WMG_INFO_NONE("CONNECTED\n");
				break;
			case WIFI_STA_OBTAINING_IP:
				WMG_INFO_NONE("OBTAINING_IP\n");
				break;
			case WIFI_STA_NET_CONNECTED:
				WMG_INFO_NONE("NET_CONNECTED\n");
				break;
			case WIFI_STA_DHCP_TIMEOUT:
				WMG_INFO_NONE("DHCP_TIMEOUT\n");
				break;
			case WIFI_STA_DISCONNECTING:
				WMG_INFO_NONE("DISCONNECTING\n");
				break;
			case WIFI_STA_DISCONNECTED:
				WMG_INFO_NONE("DISCONNECTED\n");
				break;
			default:
				WMG_INFO_NONE("UNKNOW\n");
				break;
		}
	}
#endif
#ifdef SUPPORT_AP_MODE
	if(wmg_state.current_mode & WIFI_AP) {
		WMG_INFO("* ap state: ");
		switch (wmg_state.ap_state) {
			case WIFI_AP_DISABLE:
				WMG_INFO_NONE("DISABLE\n");
				break;
			case WIFI_AP_ENABLE:
				WMG_INFO_NONE("ENABLE\n");
				break;
			default:
				WMG_INFO_NONE("UNKNOW\n");
				break;
		}
	}
#endif
#ifdef SUPPORT_MONITOR_MODE
	if(wmg_state.current_mode & WIFI_MONITOR) {
		WMG_INFO("* mon state: ");
		switch (wmg_state.monitor_state) {
			case WIFI_MONITOR_DISABLE:
				WMG_INFO_NONE("DISABLE\n");
				break;
			case WIFI_MONITOR_ENABLE:
				WMG_INFO_NONE("ENABLE\n");
				break;
			default:
				WMG_INFO_NONE("UNKNOW\n");
				break;
		}
	}
#endif
#ifdef SUPPORT_P2P_MODE
	if(wmg_state.current_mode & WIFI_P2P) {
		WMG_INFO("* p2p state: ");
		switch (wmg_state.p2p_state) {
			case WIFI_P2P_ENABLE:
				WMG_INFO_NONE("ENABLE\n");
				break;
			case WIFI_P2P_DISABLE:
				WMG_INFO_NONE("DISABLE\n");
				break;
			case WIFI_P2P_CONNECTD_GC:
				WMG_INFO_NONE("CONNECTD_GC\n");
				break;
			case WIFI_P2P_CONNECTD_GO:
				WMG_INFO_NONE("CONNECTD_GO\n");
				break;
			case WIFI_P2P_DISCONNECTD:
				WMG_INFO_NONE("DISCONNECTD\n");
				break;
			default:
				WMG_INFO_NONE("UNKNOW\n");
				break;
		}
	}
#endif
}

static int cmd_handle_i(char *p)
{
	wmg_status_t ret = WMG_STATUS_FAIL;
	WMG_DEBUG("cmd handle i: %s\n",p);

	wifi_wmg_state_t wmg_state;
	ret = wifi_get_wmg_state(&wmg_state);
	if (ret == WMG_STATUS_SUCCESS) {
		WMG_INFO("-------------------------------------\n");
		WMG_INFO("* support mode:(%s | %s | %s | %s)\n",
			(wmg_state.support_mode & WIFI_STATION)?"sta":"---",
			(wmg_state.support_mode & WIFI_AP)?"ap":"--",
			(wmg_state.support_mode & WIFI_MONITOR)?"mon":"---",
			(wmg_state.support_mode & WIFI_P2P)?"p2p":"---");
		WMG_INFO("* current mode:(%s & %s & %s & %s)\n",
			(wmg_state.current_mode & WIFI_STATION)?"sta":"---",
			(wmg_state.current_mode & WIFI_AP)?"ap":"--",
			(wmg_state.current_mode & WIFI_MONITOR)?"mon":"---",
			(wmg_state.current_mode & WIFI_P2P)?"p2p":"---");
		printf_mode_state(wmg_state);
		WMG_INFO("-------------------------------------\n");
	} else {
		WMG_ERROR("get wifimanger state faile\n");
	}
	return ret;
}

static int cmd_handle_s(char *p)
{
	WMG_DEBUG("cmd handle s: %s\n",p);

	if(!strncmp(p, "p2p", 3)) {
		wifi_p2p_peers_t p2p_peers;
		wifi_p2p_find(&p2p_peers, 10);
		return 0;
	} else {
#ifdef OS_NET_LINUX_OS
		clock_t begin, end;
#else
		int begin = 0;
		int end = 0;
#endif
		wmg_status_t ret = WMG_STATUS_FAIL;
		char scan_res_char[20] = {0};
		int get_scan_results_num = 0;
		char mac_addr_char[18] = {0};
		int scan_max_num = RES_LEN;
		wifi_scan_result_t *scan_res = NULL;

		int i, bss_num = 0;
		if(strlen(p) > 0) {
			if(isdigitstr(p)){
				scan_max_num = atoi(p);
			} else {
				WMG_WARNG("input scan max num is not legal to use the default value %d\n", RES_LEN);
			}
		}

		scan_res = (wifi_scan_result_t *)malloc(sizeof(wifi_scan_result_t) * scan_max_num);
		if(scan_res == NULL) {
			WMG_ERROR("malloc scan_res fail\n");
			return WMG_STATUS_FAIL;
		}

		begin = GET_TIME;
		ret = wifi_get_scan_results(scan_res, &bss_num, scan_max_num);
		end = GET_TIME;
		if (ret == WMG_STATUS_SUCCESS) {
			get_scan_results_num = (scan_max_num > bss_num ? bss_num : scan_max_num);
			for (i = 0; i < get_scan_results_num; i++) {
				memset(scan_res_char, 0, 20);
				memset(mac_addr_char, 0, 18);
				uint8tochar(mac_addr_char, scan_res[i].bssid);
				WMG_INFO("[%02d]: bssid=%s  channel=%-3d  freq=%d  rssi=%d  sec=%-12s  ssid=%s\n",
						i, mac_addr_char, freq_to_channel(scan_res[i].freq),scan_res[i].freq,
						scan_res[i].rssi, key_mgmt_to_char(scan_res_char, scan_res[i].key_mgmt), scan_res[i].ssid);
			}
			WMG_INFO("==Wi-Fi scan successful, total %d ap(scan_max_num: %d) time %4f ms==\n", bss_num, scan_max_num, USE_TIME(begin, end));
			ret = WMG_STATUS_SUCCESS;
		} else {
			WMG_ERROR("==Wi-Fi scan failed, time %4f ms==\n", USE_TIME(begin, end));
			ret = WMG_STATUS_FAIL;
		}
		if(scan_res != NULL) {
			free(scan_res);
			scan_res = NULL;
		}
		return ret;
	}
}

static int wifi_try_connect_directly(wifi_sta_cn_para_t *cn_para)
{
	int ret = 0;
	WMG_INFO("==Wi-Fi connect ssid: %s, psk: %s, sec: %d==\n", cn_para->ssid, cn_para->password, cn_para->sec);
	ret = wifi_sta_connect(cn_para);
	if (ret == 0) {
		WMG_INFO("==Wi-Fi connect use sec(%d)==\n"
			"==Wi-Fi connect directly successful==\n", cn_para->sec);
		return 0;
	} else {
		WMG_INFO("==Wi-Fi connect(wpa2) failed ,try wep...==\n");
		cn_para->sec = WIFI_SEC_WEP;
		ret = wifi_sta_connect(cn_para);
		if (ret == 0) {
			WMG_INFO("==Wi-Fi connect use sec(%d)==\n"
				"==Wi-Fi connect directly successful==\n", cn_para->sec);
			return 0;
		} else {
			WMG_INFO("==Wi-Fi connect (wpa2/wep) failed ,try wp3...==\n");
			cn_para->sec = WIFI_SEC_WPA3_PSK;
			ret = wifi_sta_connect(cn_para);
			if (ret == 0) {
				WMG_INFO("==Wi-Fi connect use sec(%d)==\n"
					"==Wi-Fi connect directly successful==\n", cn_para->sec);
				return 0;
			}
		}
	}
	WMG_ERROR("==Wi-Fi connect directly failed==\n");
	return -1;
}

#ifdef SUPPORT_STA_MODE
static int cmd_handle_c(char *p)
{
	WMG_DEBUG("cmd handle c: %s\n",p);

#ifdef OS_NET_LINUX_OS
	clock_t begin, end;
#else
	int begin, end;
#endif
	char *ssid_p = NULL;
	char *ssid_tmp_p = NULL;
	int ssid_len = 0;
	int ssid_total_len = 0;
	char *psk_p = NULL;
	char *sec_p = NULL;
	char *delim = " ";
	char ssid_buf[SSID_MAX_LEN + 1] = {0};
	char psk_buf[PSK_MAX_LEN + 1] = {0};
	int ret = -1;
	int i, bss_num = 0, get_scan_results_flag = 0, get_scan_results_num = 0;
	wifi_scan_result_t scan_res[RES_LEN] = {0};
	char scan_res_char[20] = {0};

	wifi_sta_cn_para_t cn_para;
	ssid_p = strtok(p, delim);
	ssid_tmp_p = ssid_p;
	ssid_len = strlen(ssid_p);
	ssid_total_len += ssid_len;
	while((ssid_total_len <= SSID_MAX_LEN) && (ssid_p != NULL)) {
		if(ssid_p[ssid_len - 1] == '\\') {
			ssid_p = strtok(NULL, delim);
			ssid_tmp_p[ssid_len - 1] = ' ';
			strcat(ssid_buf, ssid_tmp_p);
			ssid_tmp_p = ssid_p;
			ssid_len = strlen(ssid_p);
			ssid_total_len += ssid_len;
		} else {
			strcat(ssid_buf, ssid_p);
			break;
		}
	}
	if(ssid_total_len > SSID_MAX_LEN) {
		WMG_ERROR("Input ssid is longer than %d\n",SSID_MAX_LEN);
		return -1;
	}

	psk_p = strtok(NULL, delim);
	begin = GET_TIME;
	if(psk_p == NULL) {
		cn_para.sec = WIFI_SEC_NONE;
		WMG_INFO("connect to sta(%s) without pask\n",ssid_p);
	} else {
		if((strlen(psk_p) > PSK_MAX_LEN) || (strlen(psk_p) < 8)) {
			WMG_ERROR("The length of psk(%d) needs to be between 8 and %d\n", strlen(psk_p), PSK_MAX_LEN);
			return -1;
		}
		strcpy(psk_buf, psk_p);
		sec_p = strtok(NULL, delim);
		if(sec_p != NULL) {
			if(!strncmp(sec_p, "wep", 3)) {
				cn_para.sec = WIFI_SEC_WEP;
			} else if(!strncmp(sec_p, "wpa", 4)) {
				cn_para.sec = WIFI_SEC_WPA_PSK;
			} else if(!strncmp(sec_p, "wpa2", 4)) {
				cn_para.sec = WIFI_SEC_WPA2_PSK;
			} else if(!strncmp(sec_p, "wpa3", 4)) {
				cn_para.sec = WIFI_SEC_WPA3_PSK;
			}
		} else if(!wifi_get_scan_results(scan_res, &bss_num, RES_LEN)) {
			get_scan_results_num = (RES_LEN > bss_num ? bss_num : RES_LEN);
			for (i = 0; i < get_scan_results_num; i++) {
				if (!strcmp(scan_res[i].ssid, ssid_buf)) {
					get_scan_results_flag = 1;
					cn_para.sec = WIFI_SEC_NONE;
					if((scan_res[i].key_mgmt) & WIFI_SEC_WPA_PSK) {
						cn_para.sec = WIFI_SEC_WPA_PSK;
					}
					if((scan_res[i].key_mgmt) & WIFI_SEC_WPA2_PSK) {
						cn_para.sec = WIFI_SEC_WPA2_PSK ;
					}
					if((scan_res[i].key_mgmt) & WIFI_SEC_WPA3_PSK) {
						cn_para.sec = WIFI_SEC_WPA3_PSK ;
					}
					if((scan_res[i].key_mgmt) & WIFI_SEC_WEP) {
						cn_para.sec = WIFI_SEC_WEP ;
					}
					break;
				}
			}
			if(get_scan_results_flag) {
				WMG_DEBUG("==Wi-Fi scan ssid:%s(sec%s)==\n", ssid_buf, key_mgmt_to_char(scan_res_char, scan_res[i].key_mgmt));
			} else {
				WMG_ERROR("==Wi-Fi can't scan ssid:%s, try connect directly.==\n", ssid_buf);
				cn_para.ssid = ssid_buf;
				cn_para.password = psk_buf;
				cn_para.sec = WIFI_SEC_WPA2_PSK;
				ret = wifi_try_connect_directly(&cn_para);
				goto results;
			}
		} else {
			WMG_ERROR("==Wi-Fi scan failed, when connect to ap==\n");
			return -1;
		}
	}

	/* juge wifi is connected and needs to be swithed.*/
	wifi_sta_info_t wifi_sta_info;
	wifi_wmg_state_t wmg_state;
	memset(&wifi_sta_info, 0, sizeof(wifi_sta_info_t));
	wifi_get_wmg_state(&wmg_state);
	if (wmg_state.current_mode & WIFI_STATION) {
		if(wmg_state.sta_state == WIFI_STA_NET_CONNECTED) {
			wifi_sta_get_info(&wifi_sta_info);
			if(!strcmp(wifi_sta_info.ssid, ssid_buf)) {
				WMG_INFO("==wifi already connect to %s,and not need to change==\n",wifi_sta_info.ssid);
				return 0;
			}
			WMG_INFO("==wifi already connect to %s,but now need to change to connect%s.==\n", wifi_sta_info.ssid, ssid_buf);
		}
	}

	cn_para.ssid = ssid_buf;
	cn_para.password = psk_buf;
	WMG_INFO("connect to sta(%s) with pask(%s)\n", ssid_buf, psk_buf);
	ret = wifi_sta_connect(&cn_para);
	end = GET_TIME;
results:
	if (ret == 0) {
		memset(scan_res_char, 0, 20);
		WMG_INFO("==Wi-Fi connect use sec(%s)==\n", key_mgmt_to_char(scan_res_char, cn_para.sec));
		WMG_INFO("==Wi-Fi connect successful,time %4f ms==\n", cn_para.sec, USE_TIME(begin, end));
		return 0;
	} else {
		WMG_ERROR("==Wi-Fi connect failed,time %4f ms==\n", USE_TIME(begin, end));
		return -1;
	}
}
#endif

#ifdef SUPPORT_P2P_MODE
static int cmd_handle_C(char *p)
{
	WMG_DEBUG("cmd handle C: %s\n",p);

#ifdef OS_NET_LINUX_OS
	clock_t begin, end;
#else
	int begin, end;
#endif

	int ret = -1;
	uint8_t p2p_mac[6] = {0};
	char *pch;
	int i;

	pch = strtok(p, ":");
	for(i = 0;(pch != NULL) && (i < 6); i++){
		p2p_mac[i] = char2uint8(pch);
		pch = strtok(NULL, ":");
	}

	begin = GET_TIME;
	ret = wifi_p2p_connect(&p2p_mac);
	end = GET_TIME;
	WMG_INFO("==Wi-Fi p2p connect %s,time %4f ms==\n", (ret == 0)?"success":"failed", USE_TIME(begin, end));
	return ret;
}
#endif

static int cmd_handle_d(char *p)
{
	int ret = -1;
	WMG_DEBUG("cmd handle d: %s\n",p);

	if(*p == '\0') {
		ret = wifi_sta_disconnect();
		WMG_INFO("wifi sta disconect %s\n", (ret == WMG_STATUS_SUCCESS)?"success":"faile");
#ifdef SUPPORT_AP_MODE
	} else if(!strncmp(p, "ap", 2)) {
		ret = wifi_ap_disable();
		WMG_INFO("wifi ap disable %s\n", (ret == WMG_STATUS_SUCCESS)?"success":"faile");
#endif
#ifdef SUPPORT_MONITOR_MODE
	} else if(!strncmp(p, "monitor", 7)) {
		ret = wifi_monitor_disable();
		WMG_INFO("wifi mon disable %s\n", (ret == WMG_STATUS_SUCCESS)?"success":"faile");
#endif
#ifdef SUPPORT_P2P_MODE
	} else if(!strncmp(p, "p2p", 3)) {
		ret = wifi_p2p_disconnect(NULL);
		if ((ret == WMG_STATUS_SUCCESS) || (ret == WMG_STATUS_UNSUPPORTED)) {
			WMG_INFO("wifi p2p disconnect success!\n");
		} else {
			WMG_ERROR("wifi p2p disconect failed, please check if p2p has connect\n");
		}
		ret = wifi_p2p_disable();
		if (ret != WMG_STATUS_SUCCESS) {
			WMG_ERROR("wifi p2p disable failed, please check if p2p has enable\n");
		} else {
			WMG_INFO("wifi p2p disable success!\n");
		}
#endif
	}
	return ret;
}

static int cmd_handle_a(char *p)
{
	int ret = -1;
	WMG_DEBUG("cmd handle a: %s\n",p);

	if(!strncmp(p, "enable", 6)) {
		ret = wifi_sta_auto_reconnect(true);
		WMG_INFO("wifi set auto reconnect enable %s\n", (ret == WMG_STATUS_SUCCESS)?"success":"faile");
	} else if(!strncmp(p, "disable", 7)) {
		ret = wifi_sta_auto_reconnect(false);
		WMG_INFO("wifi set auto reconnect disable %s\n", (ret == WMG_STATUS_SUCCESS)?"success":"faile");
	} else {
		WMG_ERROR("sta auto reconnect don't support cmd\n");
	}
	return ret;
}

static int cmd_handle_l(char *p)
{
	wmg_status_t ret;
	wifi_sta_info_t wifi_sta_info;

	wifi_sta_list_t sta_list_networks;
	wifi_sta_list_nod_t sta_list_nod[LIST_NETWORK_DEFAUT_NUM] = {0};
	int entry = 0, i = 0;

	wifi_ap_config_t ap_config;
	char ssid_buf[SSID_MAX_LEN + 1] = {0};
	char psk_buf[PSK_MAX_LEN + 1] = {0};
	uint8_t device_list[STA_MAX_NUM][6] = {0};
	char mac_addr_char[18] = {0};
	char key_mgmt_char[20] = {0};

	WMG_DEBUG("cmd handle l: %s\n",p);
	memset(&wifi_sta_info, 0, sizeof(wifi_sta_info_t));
	if(!strncmp(p, "all", 3)) {
		WMG_INFO("list all saved ap config info\n");
		sta_list_networks.list_nod = sta_list_nod;
		sta_list_networks.list_num = LIST_NETWORK_DEFAUT_NUM;
		ret = wifi_sta_list_networks(&sta_list_networks);
		if(ret == WMG_STATUS_SUCCESS) {
			entry = sta_list_networks.list_num >= sta_list_networks.sys_list_num ? sta_list_networks.sys_list_num : sta_list_networks.list_num;
			if(entry != 0){
				WMG_INFO("network id / ssid / bssid / flags\n");
				for(i = 0; entry > 0; entry--){
					WMG_INFO("%d\t%s\t%s\t%s\n",sta_list_networks.list_nod[i].id,
												sta_list_networks.list_nod[i].ssid,
												sta_list_networks.list_nod[i].bssid,
												sta_list_networks.list_nod[i].flags);
					i++;
				}
			} else {
				WMG_WARNG("System has no entry save\n");
			}
			goto results;
		} else {
			WMG_ERROR("get list info faile\n");
		}
	} else if(!strncmp(p, "p2p", 3)) {
		wifi_p2p_info_t p2p_info;
		ret = wifi_p2p_get_info(&p2p_info);
		if(ret == WMG_STATUS_SUCCESS) {
			memset(mac_addr_char, 0, 18);
			uint8tochar(mac_addr_char, p2p_info.bssid);
			WMG_INFO("p2p bssid: %s\np2p mode: %d\np2p freq: %d(channel %d)\np2p ssid: %s\n",
					mac_addr_char, p2p_info.mode, p2p_info.freq, freq_to_channel(p2p_info.freq), p2p_info.ssid);
			goto results;
		}
		goto results;
	} else if(!strncmp(p, "ap", 2)) {
		ap_config.ssid = ssid_buf;
		ap_config.psk = psk_buf;
		for (i = 0; i < STA_MAX_NUM; i++) {
			ap_config.dev_list[i] = device_list[i];
		};
		ret = wifi_ap_get_config(&ap_config);
		if (ret == 0) {
			WMG_INFO("get ap config success:\nssid=%s\npsk=%s\nkey_mgmt=%d\nsec=%d\nchannel=%d\n",
					ap_config.ssid, ap_config.psk, ap_config.key_mgmt, ap_config.sec, ap_config.channel);
			WMG_INFO("ap ip_addr: %d.%d.%d.%d\n", ap_config.ip_addr[0],
												ap_config.ip_addr[1],
												ap_config.ip_addr[2],
												ap_config.ip_addr[3]);
			WMG_INFO("ap gw_addr: %d.%d.%d.%d\n", ap_config.gw_addr[0],
												ap_config.gw_addr[1],
												ap_config.gw_addr[2],
												ap_config.gw_addr[3]);
			WMG_INFO("sta_num=%d\n", ap_config.sta_num);
			for (i = 0; i < ap_config.sta_num; i++) {
				memset(mac_addr_char, 0, 18);
				uint8tochar(mac_addr_char, ap_config.dev_list[i]);
				WMG_INFO("device%d: %s\n", i + 1, mac_addr_char);
			}
			goto results;
		} else {
			goto results;
		}
	} else {
		WMG_INFO("list connected ap\n");
		ret = wifi_sta_get_info(&wifi_sta_info);
		if(ret == WMG_STATUS_SUCCESS) {
			WMG_INFO("sta id: %d\n", wifi_sta_info.id);
			WMG_INFO("sta freq: %d\n", wifi_sta_info.freq);
			WMG_INFO("sta rssi: %d\n", wifi_sta_info.rssi);
			memset(mac_addr_char, 0, 18);
			uint8tochar(mac_addr_char, wifi_sta_info.bssid);
			WMG_INFO("sta bssid: %s\n", mac_addr_char);
			WMG_INFO("sta ssid: %s\n", wifi_sta_info.ssid);
			memset(mac_addr_char, 0, 18);
			uint8tochar(mac_addr_char, wifi_sta_info.mac_addr);
			WMG_INFO("sta mac_addr: %s\n", mac_addr_char);
			WMG_INFO("sta ip_addr: %d.%d.%d.%d\n",wifi_sta_info.ip_addr[0],
												wifi_sta_info.ip_addr[1],
												wifi_sta_info.ip_addr[2],
												wifi_sta_info.ip_addr[3]);
			WMG_INFO("sta sec: %s\n", key_mgmt_to_char(key_mgmt_char, wifi_sta_info.sec));
			goto results;
		}
	}
results:
	if(ret)
	{
		WMG_ERROR("get ap config failed\n");
		return WMG_STATUS_FAIL;
	}else
	{
		WMG_ERROR("get ap config successful\n");
		return WMG_STATUS_SUCCESS;
	}
}

static int cmd_handle_r(char *p)
{
	wmg_status_t ret = WMG_STATUS_FAIL;
	WMG_DEBUG("cmd handle r: %s\n",p);
	if(!strncmp(p, "all", 3)) {
		ret = wifi_sta_remove_networks(NULL);
		WMG_INFO("remove all networks %s\n", (ret == WMG_STATUS_SUCCESS)?"success":"fail");
	} else {
		if(strlen(p) > SSID_MAX_LEN){
			WMG_ERROR("remove network(%s) longer than %d\n", p, SSID_MAX_LEN);
		} else {
			ret = wifi_sta_remove_networks(p);
			WMG_INFO("remove networks %s (%s)\n", (ret == WMG_STATUS_SUCCESS)?"success":"fail", p);
		}
	}
	return ret;
}

static int cmd_handle_g(char *p)
{
	wmg_status_t ret = WMG_STATUS_FAIL;
	WMG_DEBUG("cmd handle g: %s\n",p);
	uint8_t mac_addr[6];
	char mac_addr_char[18] = {0};
	ret = wifi_get_mac(NULL, mac_addr);
	if (ret == WMG_STATUS_SUCCESS) {
		uint8tochar(mac_addr_char, mac_addr);
		WMG_INFO("get macaddr: %s\n", mac_addr_char);
	}
	return ret;
}

static int cmd_handle_m(char *p)
{
	wmg_status_t ret = WMG_STATUS_FAIL;
	WMG_DEBUG("cmd handle m: %s\n",p);
	uint8_t mac_addr[6] = {0};
	char mac_addr_char[18] = {0};
	char *pch;
	int i;

	pch = strtok(p, ":");
	for(i = 0;(pch != NULL) && (i < 6); i++){
		mac_addr[i] = char2uint8(pch);
		pch = strtok(NULL, ":");
	}

	if(i != 6) {
		WMG_DEBUG("%s: mac address format is incorrect\n",p);
		return -1;
	}

	ret = wifi_set_mac(NULL, mac_addr);
	uint8tochar(mac_addr_char, mac_addr);
	WMG_INFO("set macaddr: %s %s\n", mac_addr_char, (ret == WMG_STATUS_SUCCESS)?"success":"faile");
	return ret;
}

#ifdef OS_NET_XRLINK_OS
const char XR_RAW_TEST_HANDLE[] = "raw_data : ";

void cmd_v_echo_printf(uint8_t *data, uint32_t len)
{
	WMG_INFO("recv: %d\n",len);
}
#define XRLINK_VENDOR_MAX_LEN 1500
static int cmd_handle_v(char *p)
{
	int length;

	char *data = NULL;

	int i = 0;

	wifi_vendor_register_rx_cb(cmd_v_echo_printf);

	length = atoi(p);

	WMG_INFO("send vendor data length:%d start\n",length);

	data = (char*)malloc(length);
	if (!data) {
		WMG_ERROR("vendor data malloc failed\n");
		return 0;
	}

	if (length >= XRLINK_VENDOR_MAX_LEN) {
		for (i = 0; i < length / XRLINK_VENDOR_MAX_LEN; i++) {

			wifi_vendor_send_data(data + i * XRLINK_VENDOR_MAX_LEN,
					XRLINK_VENDOR_MAX_LEN);

			WMG_DEBUG("send offset:%d,len:%d\n", i * XRLINK_VENDOR_MAX_LEN,
					XRLINK_VENDOR_MAX_LEN);
		}
	}

	if (length % XRLINK_VENDOR_MAX_LEN) {

		WMG_DEBUG("send offset:%d,len:%d\n", i * XRLINK_VENDOR_MAX_LEN,
				length % XRLINK_VENDOR_MAX_LEN);

		wifi_vendor_send_data(data + i * XRLINK_VENDOR_MAX_LEN,
				length % XRLINK_VENDOR_MAX_LEN);

	}

	WMG_INFO("send vendor data length:%d end\n",length);
	free(data);

	return 0;
}
#endif

#ifdef SUPPORT_EXPAND
static int cmd_handle_e(char *p)
{
	char send_expand_cmd[DATA_BUF_SIZE] = {0};
	memcpy(send_expand_cmd, p, DATA_BUF_SIZE);
	return wifi_send_expand_cmd(p, NULL);
}
#endif

#ifdef SUPPORT_LINKD
static int cmd_handle_p(char *p)
{
	int ret = -1;
	char ssid_buf[SSID_MAX_LEN + 1] = {0};
	char psk_buf[PSK_MAX_LEN + 1] = {0};
	wifi_linkd_result_t linkd_result;
	wmg_status_t erro_code;

	linkd_result.ssid = ssid_buf;
	linkd_result.psk = psk_buf;

	WMG_DEBUG("cmd handle p: %s\n",p);
	if(!strncmp(p, "ble", 3)) {
		WMG_INFO("ble config net\n");
		erro_code = wifi_linkd_protocol(WMG_LINKD_MODE_BLE, NULL, 0, &linkd_result);
		if(erro_code != WMG_STATUS_SUCCESS){
			WMG_ERROR("ble config net get result failed\n");
			return WMG_STATUS_FAIL;
		}
	} else if(!strncmp(p, "softap", 6)) {
		WMG_INFO("softap config net\n");
		wifi_ap_config_t ap_config;
		char ssid_buf[SSID_MAX_LEN + 1] = "Aw-config-net-Test";
		char psk_buf[PSK_MAX_LEN + 1] = "Aa123456";
		ap_config.ssid = ssid_buf;
		ap_config.psk = psk_buf;
		ap_config.sec = WIFI_SEC_WPA2_PSK;
		ap_config.channel = 6;

		erro_code = wifi_linkd_protocol(WMG_LINKD_MODE_SOFTAP, &ap_config, 0, &linkd_result);
		if(erro_code != WMG_STATUS_SUCCESS){
			if(erro_code == WMG_STATUS_TIMEOUT) {
				WMG_ERROR("softap config net get result failed(linkd time out)\n");
			} else {
				WMG_ERROR("softap config net get result failed\n");
			}
			return WMG_STATUS_FAIL;
		}
	} else if(!strncmp(p, "xconfig", 7)) {
		WMG_INFO("xconfig config net\n");
		erro_code = wifi_linkd_protocol(WMG_LINKD_MODE_XCONFIG, NULL, 0, &linkd_result);
		if(erro_code != WMG_STATUS_SUCCESS){
			WMG_ERROR("xconfig config net get result failed\n");
			return WMG_STATUS_FAIL;
		}
	} else if(!strncmp(p, "soundwave", 9)) {
		WMG_INFO("soundwave config net\n");
		erro_code = wifi_linkd_protocol(WMG_LINKD_MODE_SOUNDWAVE, NULL, 0, &linkd_result);
		if(erro_code != WMG_STATUS_SUCCESS){
			WMG_ERROR("soundwave config net get result failed\n");
			return WMG_STATUS_FAIL;
		}
	} else {
		WMG_ERROR("Don't support %s config net\n", p);
		return WMG_STATUS_FAIL;
	}

	WMG_INFO("Get ssid(%s) and psk(%s) success, now connect to ap\n", linkd_result.ssid, linkd_result.psk);
	ret = wifi_on(WIFI_STATION);
	WMG_INFO("wifi on sta %s\n", (ret == WMG_STATUS_SUCCESS)?"success":"faile");
	if (ret != WMG_STATUS_SUCCESS) {
		return ret;
	}
	wifi_sta_cn_para_t cn_para;
	cn_para.ssid = linkd_result.ssid;
	cn_para.password = linkd_result.psk;
	cn_para.fast_connect = 0;
	cn_para.sec = WIFI_SEC_WPA2_PSK;
	ret = wifi_sta_connect(&cn_para);
	WMG_INFO("Wi-Fi connect %s\n", (ret == WMG_STATUS_SUCCESS)?"successful":"failed");
	return ret;
}
#endif

static int cmd_handle_D(char *p)
{
	int level = -1;

	WMG_DEBUG("cmd handle D: %s\n",p);
	WMG_INFO("set debug lv: ");
	if(!strncmp(p, "error", 5)) {
		WMG_INFO("error\n");
		level = 1;
	} else if(!strncmp(p, "warn", 4)) {
		WMG_INFO("warn\n");
		level = 2;
	} else if(!strncmp(p, "info", 4)) {
		WMG_INFO("info\n");
		level = 3;
	} else if(!strncmp(p, "debug", 5)) {
		WMG_INFO("debug\n");
		level = 4;
	} else if(!strncmp(p, "dump", 4)) {
		WMG_INFO("dump\n");
		level = 5;
	} else if(!strncmp(p, "exce", 4)) {
		WMG_INFO("exce\n");
		level = 6;
	} else if(!strncmp(p, "open", 4)) {
		WMG_INFO("open path file func line printf info\n");
		wmg_set_log_para(WMG_LOG_SETTING_TIME | WMG_LOG_SETTING_FILE | WMG_LOG_SETTING_FUNC | WMG_LOG_SETTING_LINE);
		return 0;
	} else if(!strncmp(p, "close", 5)) {
		WMG_INFO("close path file func line printf info\n");
		wmg_set_log_para(0);
		return 0;
	} else {
		WMG_ERROR("unknow %s\n", p);
		return -1;
	}

	wmg_set_debug_level(level);
	return 0;
}

void *cmd_handle(void *arg)
{
#ifdef OS_NET_LINUX_OS
	pthread_detach(pthread_self());
#endif
	char *cmd_data = (char *)arg;
	WMG_DEBUG("get cmd: %s\n", cmd_data);
	char *p = cmd_data;
	char ch = *p;
	p = p+2;
	switch (ch) {
		case 'o':
			cmd_handle_o(p);
		break;
		case 'f':
			cmd_handle_f(p);
		break;
		case 's':
			cmd_handle_s(p);
		break;
#ifdef SUPPORT_STA_MODE
		case 'c':
			cmd_handle_c(p);
		break;
#endif
#ifdef SUPPORT_P2P_MODE
		case 'C':
			cmd_handle_C(p);
		break;
#endif
		case 'd':
			cmd_handle_d(p);
		break;
		case 'a':
			cmd_handle_a(p);
		break;
		case 'l':
			cmd_handle_l(p);
		break;
		case 'r':
			cmd_handle_r(p);
		break;
		case 'g':
			cmd_handle_g(p);
		break;
		case 'm':
			cmd_handle_m(p);
		break;
		case 'i':
			cmd_handle_i(p);
		break;
#ifdef OS_NET_XRLINK_OS
		case 'v':
			cmd_handle_v(p);
		break;
#endif
#ifdef SUPPORT_EXPAND
		case 'e':
			cmd_handle_e(p);
		break;
#endif
#ifdef SUPPORT_LINKD
		case 'p':
			cmd_handle_p(p);
		break;
#endif
		case 'D':
			cmd_handle_D(p);
		break;
		default:
			WMG_ERROR("Un't support cmd: %c\n",ch);
		break;
	}
#ifdef OS_NET_LINUX_OS
	free(arg);
	pthread_exit(0);
#endif
}

static deamon_object_t deamon_object = {
	.deamon_current_mode = WIFI_MODE_UNKNOWN,
};

#ifdef OS_NET_FREERTOS_OS
int rtos_main(void *snd_cmd)
{
	if(!rtos_wifimanager_init) {
		wifimanager_init();
		rtos_wifimanager_init = 1;
	}

	cmd_handle(snd_cmd);

	return 0;
}
#else
static int lockFile(int fd)
{
	struct flock fl;
	fl.l_type   = F_WRLCK;
	fl.l_start  = 0;
	fl.l_whence = SEEK_SET;
	fl.l_len    = 0;
	fl.l_pid = -1;

	return(fcntl(fd, F_SETLK, &fl));
}

static int isRunning(const char *procname)
{
	char buf[16] = {0};
	char filename[128] = {0};
	sprintf(filename, "%s%s.pid",PID_FILE_PATH,procname);
	int fd = open(filename, O_CREAT|O_RDWR, 0600);
	if (fd < 0) {
		WMG_ERROR("open lockfile %s failed!\n", filename);
		return 1;
	}
	if (-1 == lockFile(fd))
	{
		WMG_WARNG("%s is already running\n", procname);
		close(fd);
		return 1;
	} else {
		ftruncate(fd, 0);
		sprintf(buf, "%ld", (long)getpid());
		write(fd, buf, strlen(buf) + 1);
		return 0;
	}
}

int main(void)
{
	pthread_t tid;
	pid_t pc;
	socklen_t clt_addr_len;
	int listen_fd;
	int com_fd;
	int ret;
	char data_buf[DATA_BUF_SIZE];
	char *pthread_arg = NULL;
	int len;
	struct sockaddr_un clt_addr;
	struct sockaddr_un srv_addr;
	int debug_level;

	pc = fork();
	if(pc<0)
	{
		printf("error fork\n");
		exit(-1);
	} else if(pc>0) {
		exit(0);
	}

	//if(isRunning(PROC_NAME))
	//{
	//	exit(-1);
	//}

	wifimanager_init();
	//use to get time
	sc_clk_tck = sysconf(_SC_CLK_TCK);

	listen_fd = socket(PF_UNIX, SOCK_STREAM, 0);
	if(listen_fd < 0) {
		perror("cannot create communication socket");
		return 1;
	}

	//set server addr_param
	srv_addr.sun_family = AF_UNIX;
	strcpy(srv_addr.sun_path,UNIX_DOMAIN);
	unlink(UNIX_DOMAIN);

	//bind sockfd & addr
	ret = bind(listen_fd, (struct sockaddr*)&srv_addr, sizeof(srv_addr));
	if(ret == -1) {
		perror("cannot bind server socket");
		close(listen_fd);
		unlink(UNIX_DOMAIN);
		return 1;
	}

	//listen sockfd
	ret = listen(listen_fd,1);
	if(ret == -1) {
		perror("cannot listen the client connect request");
		close(listen_fd);
		unlink(UNIX_DOMAIN);
		return 1;
	}

	while(1) {
		//have connect request use accept
		len = sizeof(clt_addr);
		com_fd = accept(listen_fd,(struct sockaddr*)&clt_addr,&len);
		if(com_fd < 0) {
			perror("cannot accept client connect request");
			close(listen_fd);
			unlink(UNIX_DOMAIN);
			return 1;
		}

		memset(data_buf, 0, DATA_BUF_SIZE);
		read(com_fd, data_buf, sizeof(data_buf));

		pthread_arg = (char *)malloc(DATA_BUF_SIZE);
		if(pthread_arg == NULL) {
			WMG_ERROR("malloc pthread_arg failed\n");
			return -1;
		}

		memset(pthread_arg, 0, DATA_BUF_SIZE);
		memcpy(pthread_arg, data_buf, DATA_BUF_SIZE);

		ret = pthread_create(&tid, NULL, cmd_handle, (void *)pthread_arg);
		if (ret) {
			WMG_ERROR("failed to create handle thread: %s\n", strerror(ret));
			return -1;
		}

		close(com_fd);
	}
	wifimanager_deinit();

	close(listen_fd);
	unlink(UNIX_DOMAIN);
	return 0;
}

#endif //OS_NET_FREERTOS_OS
