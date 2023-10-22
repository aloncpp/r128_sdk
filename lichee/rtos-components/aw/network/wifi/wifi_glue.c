/*
 * Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
 * Email  : liumingyuan@allwinnertech.com
 * Create : 2019-09-12
 *
 */

#include <autoconf_rtos.h>
#include <lwip/dhcp.h>

#include <osdep_service.h>
#include <wifi/wifi_conf.h>
#include <wifi/wifi_util.h>
#include <string.h>
#include <FreeRTOS.h>
#include <stdlib.h>
#include "tcpip_adapter.h"
#include "AGWifiLog.h"
#include "wifi_glue.h"
#include "wifi/wifi_util.h"

#ifndef WLAN0_NAME
  #define WLAN0_NAME		"wlan0"
#endif
#ifndef WLAN1_NAME
  #define WLAN1_NAME		"wlan1"
#endif
/* Give default value if not defined */
#ifndef NET_IF_NUM
#ifdef CONFIG_CONCURRENT_MODE
#define NET_IF_NUM 2
#else
#define NET_IF_NUM 1
#endif
#endif

static wifi_msg_cb_t wifi_msg_cb = NULL;

void glue_set_wifi_msg_cb(wifi_msg_cb_t cb)
{
	wifi_msg_cb = cb;
}

void glue_wifi_on(void)
{
	int ret;
	GLWifiMsgData_t MsgData;

	MsgData.id = GLWIFI_MSG_ID_WIFI_STATUS;

	if((ret = wifi_on(RTW_MODE_STA)) < 0) {
		MsgData.data.wlanStatus = GL_WLAN0_STATUS_DOWN;
		printf("\n\rERROR: Wifi on failed!\n");
	}
	MsgData.data.wlanStatus = GL_WLAN0_STATUS_UP;
	wifi_msg_cb(&MsgData);
}

int glue_wifi_connect(const char *ssid, const char *password, const char *bssid, int time_ms)
{
	int ret = RTW_ERROR;
	int key_id = 0;
	rtw_security_t security_type = RTW_SECURITY_WPA2_AES_PSK;
	int pwd_len = 0;
	int ssid_len = 0;

	GLWifiMsgData_t MsgData = {
		.id = GLWIFI_MSG_ID_WIFI_STATUS_FROM_IMPL,
	};

	if(NULL == ssid) {
		WFLOGE("ssid is NULL.");
		return RTW_BADARG;
	}

	ssid_len = strlen(ssid);

	if(NULL == password) {
		WFLOGE("SECURITY OPEN.");
		security_type = RTW_SECURITY_OPEN;
		pwd_len = 0;
	}else{
		pwd_len = strlen(password);
	}
	{
		key_id = 0;
		security_type = RTW_SECURITY_WPA2_AES_PSK;
		//TODO: WEP
	}

	int i = 0;
	while(i < 5) {
		ret = wifi_connect(ssid,
						security_type,
						password,
						ssid_len,
						pwd_len,
						key_id,
						NULL);
		if(ret != RTW_SUCCESS) {
			i++;
			continue;
		}else {
			break;
		}
	}
	if(ret == RTW_SUCCESS) {
		struct netif *net_if = NULL;

		MsgData.data.wlanStatus = GLWIFI_CONNECTED;
		wifi_msg_cb(&MsgData);

		net_if = get_netif(MODE_STA);
		if(net_if == NULL) {
			printf("get net interface failed\n");
		}else {
			dhcp_start(net_if);
			MsgData.data.wlanStatus = GLDHCP_SUCCESS;
		}
	}else{
		MsgData.data.wlanStatus = GLWIFI_DISCONNECTED;
	}

	wifi_msg_cb(&MsgData);

	MsgData.id = GLWIFI_MSG_ID_NETWORK_STATUS;
	GLWifiStaChange_t sta_chg;

	memset(&sta_chg,0,sizeof(GLWifiStaChange_t));

	strncpy(sta_chg.ssid,ssid,ssid_len);
	strncpy(sta_chg.password,password,pwd_len);

	if(ret == RTW_SUCCESS)
		strncpy(sta_chg.status,"connect",7);
	else
		strncpy(sta_chg.status,"disconnect",10);

	//TODO: quantity

	MsgData.data.networkStatusChange = (void*)&sta_chg;
	wifi_msg_cb(&MsgData);
    return ret;
}
int glue_wifi_disconnect(void)
{
	int timeout = 20;
	char essid[33];

	if(wext_get_ssid(WLAN0_NAME, (unsigned char *) essid) < 0) {
		WFLOGE("WIFI disconnected");
		return 0;
	}

	if(wifi_disconnect() < 0) {
		WFLOGE("ERROR: Operation failed!");
		return -1;
	}

	while(1) {
		if(wext_get_ssid(WLAN0_NAME, (unsigned char *) essid) < 0) {
			WFLOGE("WIFI disconnected");
			break;
		}

		if(timeout == 0) {
			WFLOGE("ERROR: Deassoc timeout!");
			return -1;
		}
		vTaskDelay(1 * configTICK_RATE_HZ);
		timeout --;
	}
	return 0;
}

void glue_wifi_info(int argc, char **argv)
{
	int i = 0;
#if CONFIG_LWIP_LAYER
#if !defined(CONFIG_PLATFOMR_CUSTOMER_RTOS)
	u8 *mac = LwIP_GetMAC(&xnetif[0]);
	u8 *ip = LwIP_GetIP(&xnetif[0]);
	u8 *gw = LwIP_GetGW(&xnetif[0]);
#endif
#endif
	u8 *ifname[2] = {WLAN0_NAME,WLAN1_NAME};
#ifdef CONFIG_MEM_MONITOR
	extern int min_free_heap_size;
#endif

	rtw_wifi_setting_t setting;
	for(i=0;i<NET_IF_NUM;i++){
		if(rltk_wlan_running(i)){
#if CONFIG_LWIP_LAYER
#if defined(CONFIG_PLATFOMR_CUSTOMER_RTOS)
			//TODO
#else
			mac = LwIP_GetMAC(&xnetif[i]);
			ip = LwIP_GetIP(&xnetif[i]);
			gw = LwIP_GetGW(&xnetif[i]);
#endif
#endif
			printf("\n\r\nWIFI %s Status: Running",  ifname[i]);
			printf("\n\r==============================");

			rltk_wlan_statistic(i);

			wifi_get_setting((const char*)ifname[i],&setting);
			wifi_show_setting((const char*)ifname[i],&setting);
#if CONFIG_LWIP_LAYER
#if defined(CONFIG_PLATFOMR_CUSTOMER_RTOS)
			//TODO
#else
			printf("\n\rInterface (%s)", ifname[i]);
			printf("\n\r==============================");
			printf("\n\r\tMAC => %02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]) ;
			printf("\n\r\tIP  => %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
			printf("\n\r\tGW  => %d.%d.%d.%d\n\r", gw[0], gw[1], gw[2], gw[3]);
#endif
#endif
			if(setting.mode == RTW_MODE_AP || i == 1)
			{
				int client_number;
				struct {
					int    count;
					rtw_mac_t mac_list[AP_STA_NUM];
				} client_info;

				client_info.count = AP_STA_NUM;
				wifi_get_associated_client_list(&client_info, sizeof(client_info));

				printf("\n\rAssociated Client List:");
				printf("\n\r==============================");

				if(client_info.count == 0)
					printf("\n\rClient Num: 0\n\r");
				else
				{
					printf("\n\rClient Num: %d", client_info.count);
					for( client_number=0; client_number < client_info.count; client_number++ )
					{
						printf("\n\rClient [%d]:", client_number);
						printf("\n\r\tMAC => "MAC_FMT"",
										MAC_ARG(client_info.mac_list[client_number].octet));
					}
					printf("\n\r");
				}
			}

			{
				int error = wifi_get_last_error();
				printf("\n\rLast Link Error");
				printf("\n\r==============================");
				switch(error)
				{
					case RTW_NO_ERROR:
						printf("\n\r\tNo Error");
						break;
					case RTW_NONE_NETWORK:
						printf("\n\r\tTarget AP Not Found");
						break;
					case RTW_CONNECT_FAIL:
						printf("\n\r\tAssociation Failed");
						break;
					case RTW_WRONG_PASSWORD:
						printf("\n\r\tWrong Password");
						break;
					case RTW_DHCP_FAIL:
						printf("\n\r\tDHCP Failed");
						break;
					default:
						printf("\n\r\tUnknown Error(%d)", error);
				}
				printf("\n\r");
			}
		}
	}
}

int glue_get_wifi_config(GLWifiConfig_t *config)
{
	int ret = 0;
	struct netif *pnetif = NULL;

	if(wext_get_ssid(WLAN0_NAME,config->ssid) < 0)
		ret = -1;
	if(wext_get_passphrase(WLAN0_NAME,config->pwd) < 0)
		ret = -1;
	if(wext_get_bssid(WLAN0_NAME, config->bssid) < 0)
		ret = -1;
	if(wext_get_channel(WLAN0_NAME,&config->ch) < 0)
		ret = -1;
	pnetif = get_netif(MODE_STA);
	if(pnetif != NULL) {
		strncpy(config->addr,pnetif->hwaddr,WC_BSSID_LEN);
		config->ip_addr[0] = ip4_addr1_16(netif_ip4_addr(pnetif));
		config->ip_addr[1] = ip4_addr2_16(netif_ip4_addr(pnetif));
		config->ip_addr[2] = ip4_addr3_16(netif_ip4_addr(pnetif));
		config->ip_addr[3] = ip4_addr4_16(netif_ip4_addr(pnetif));
	}else
		ret = -1;
	WFLOGD("\n \
ssid:%s \n \
pwd:%s \n \
bssid:%2x:%2x:%2x:%2x:%2x:%2x \n \
ch:%d \n \
addr:%2x:%2x:%2x:%2x:%2x:%2x \n \
ip_addr:%3d.%3d.%3d.%3d\n",
	config->ssid,config->pwd,
	config->bssid[0],config->bssid[1],config->bssid[1],
	config->bssid[3],config->bssid[4],config->bssid[5],
	config->ch,
	config->addr[0],config->addr[1],config->addr[2],
	config->addr[3],config->addr[4],config->addr[5],
	config->ip_addr[0],config->ip_addr[1],config->ip_addr[2],
	config->ip_addr[3]);
	return ret;
}

const char *mac_to_str(const char *mac)
{
	static char str[18];
	str[17]='\0';

	sprintf(str,"%02X:%02X:%02X:%02X:%02X:%02X",
			mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
	return str;
}

const char *ip_to_str(uint8_t *ip_addr)
{
	static char ip[16];
	sprintf(ip,"%"U16_F".%"U16_F".%"U16_F".%"U16_F"%c",
			ip_addr[0],ip_addr[1],ip_addr[2],ip_addr[3],'\0');
	return ip;
}

int glue_get_rssi(int *rssi)
{
	return wext_get_rssi(WLAN0_NAME,rssi);
}

int glue_get_mac(char *mac)
{
	return wifi_get_mac_address(mac);
}

int glue_set_mac(char *mac)
{
	return wifi_set_mac_address(mac);
}

int glue_get_ipaddr(char *ipaddr)
{
	struct netif *pnetif = NULL;
	uint8_t ip_addr[WC_IP_ADDR_LEN];
	pnetif = get_netif(MODE_STA);
	if(pnetif == NULL) {
		return -1;
	}
	ip_addr[0] = ip4_addr1_16(netif_ip4_addr(pnetif));
	ip_addr[1] = ip4_addr2_16(netif_ip4_addr(pnetif));
	ip_addr[2] = ip4_addr3_16(netif_ip4_addr(pnetif));
	ip_addr[3] = ip4_addr4_16(netif_ip4_addr(pnetif));
	sprintf(ipaddr,"%"U16_F".%"U16_F".%"U16_F".%"U16_F"%c",
			ip_addr[0],ip_addr[1],ip_addr[2],ip_addr[3],'\0');
}
