/*
 * Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
 * Email  : liumingyuan@allwinnertech.com
 * Create : 2019-09-12
 *
 */

#ifndef __WIFI_GLUE_H__
#define __WIFI_GLUE_H__

#if __cplusplus
extern "C" {
#endif
typedef enum {
    GL_WLAN0_STATUS_DOWN,
    GL_WLAN0_STATUS_UP,
    GL_WLAN0_STATUS_MAX
} GLWifiStatus_t;

typedef enum {
    GLWIFI_MSG_ID_MIN = 0,
    GLWIFI_MSG_ID_WIFI_STATUS = GLWIFI_MSG_ID_MIN,
    GLWIFI_MSG_ID_WIFI_STATUS_FROM_IMPL,
    GLWIFI_MSG_ID_WIFI_TRACE_FROM_IMPL,
    GLWIFI_MSG_ID_NETWORK_STATUS,
    GLWIFI_MSG_ID_MAX
} GLWifiMsgId_t;

typedef enum {
	GLWIFI_DISCONNECTED = 1, //连线断开
    GLWIFI_SCAN_STARTED, // 开始扫描
    GLWIFI_SCAN_FAILED, // 扫描失败
    GLWIFI_NETWORK_NOT_FOUND, // 扫描不到AP
    GLWIFI_ASSOCIATING, // 与AP连线进行中
    GLWIFI_AUTH_REJECT, // Authentication rejected by AP
    GLWIFI_AUTH_TIMEOUT, // Authentication timeout with AP
    GLWIFI_ASSOC_REJECT, // Association rejected by AP
    GLWIFI_HANDSHAKE_FAILED, // 4次握手失败
    GLWIFI_CONNECTED, // 与AP连线成功
    GLWIFI_CONN_TIMEOUT, // 与AP连线超时
    GLDHCP_START_FAILED, // DHCP启动失败
    GLDHCP_TIMEOUT, // DHCP超时
    GLDHCP_SUCCESS // DHCP成功
} GLWifiMsgCnSta_t;

typedef struct {
#define ALI_STUB_METHOD_MAX_LENGTH      (32)
#define ALI_STUB_STATUS_MAX_LENGTH      (32)
#define ALI_STUB_SSID_MAX_LENGTH        (32)
#define ALI_STUB_PASSWORD_MAX_LENGTH    (64)

    char method[ALI_STUB_METHOD_MAX_LENGTH+1];
    int  quantity;
    char status[ALI_STUB_STATUS_MAX_LENGTH+1];
    char ssid[ALI_STUB_SSID_MAX_LENGTH+1];
    char password[ALI_STUB_PASSWORD_MAX_LENGTH+1];
}GLWifiStaChange_t;

typedef struct {
    int id;
    union {
        int   wlanStatus;
        void *networkStatusChange;
        void *wifiTrace;
    } data;
} GLWifiMsgData_t;

typedef struct {
#define WC_SSID_LEN       (32)
#define WC_PWD_LEN        (64)
#define WC_IP_ADDR_LEN    (4)
#define WC_BSSID_LEN      (6)
#define WC_ADDRESS_LEN    (6)
#define WC_WPA_STATEA_LEN (14)
	char ssid[WC_SSID_LEN];
	char pwd[WC_PWD_LEN];
	char bssid[WC_BSSID_LEN];
	uint8_t ch;
	char addr[WC_ADDRESS_LEN];
	uint8_t ip_addr[WC_IP_ADDR_LEN];
	char wpa_state[WC_WPA_STATEA_LEN];
}GLWifiConfig_t;

typedef void (*wifi_msg_cb_t)(GLWifiMsgData_t *pMsgData);

void glue_wifi_on(void);

int glue_wifi_connect(const char *ssid,const char *password,const char *bssid, int time_ms);

int glue_wifi_disconnect(void);

void glue_wifi_info(int argc, char **argv);

void glue_set_wifi_msg_cb(wifi_msg_cb_t cb);

int glue_get_wifi_config(GLWifiConfig_t *config);

const char *mac_to_str(const char *mac);

const char *ip_to_str(uint8_t *ip_addr);

int glue_get_rssi(int *rssi);

int glue_get_mac(char *mac);

int glue_get_ipaddr(char *ipaddr);

int glue_set_mac(char *mac); //string type : 11:22:33:44:55:66
#if __cplusplus
};  // extern "C"
#endif
#endif
