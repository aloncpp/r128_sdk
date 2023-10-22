/*
 * Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
 */

#ifndef __WMG_COMMON_H__
#define __WMG_COMMON_H__

#include <os_net_utils.h>
#include <os_net_mutex.h>
#include <os_net_queue.h>
#include <os_net_sem.h>
#include <os_net_thread.h>
#include <api_action.h>
#include <wifimg.h>
#include <linkd.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Number of modes supported by wifimanger
 * station , ap , monitor, p2p(Not yet supported)
 */
#define MODE_NUM             4
#define MODE_STA_NAME        "sta"
#define MODE_AP_NAME         "ap"
#define MODE_MONITOR_NAME    "monitor"
#define MODE_P2P_NAME        "p2p"      //Not yet supported

#define PLATFORM_MAX    3
#define PLATFORM_LINUX  0
#define PLATFORM_RTOS   1
#define PLATFORM_XRLINK 2

#ifdef OS_NET_FREERTOS_OS
#define PLATFORM PLATFORM_RTOS
#endif

#ifdef OS_NET_LINUX_OS
#define PLATFORM PLATFORM_LINUX
#endif

#ifdef OS_NET_XRLINK_OS
#define PLATFORM PLATFORM_XRLINK
#endif

/*
 * mode_opt - mode operation function
 * @mode_enable: mode enable    ;int for erro code
 * @mode_disable: mode disable  ;int for erro code
 * @mode_ctl: mode control      ;int for cmd, void command parameters, int for erro code
 */
typedef struct {
	wmg_status_t (*mode_enable)(int *);
	wmg_status_t (*mode_disable)(int *);
	wmg_status_t (*mode_ctl)(int,void *,void *);
}mode_opt_t;

typedef struct {
	char mode_name[10];
	mode_opt_t *mode_opt;
	wmg_status_t (*init)(void);
	wmg_status_t (*deinit)(void);
	void *private_data;
}mode_object_t;

//mode support list
typedef struct {
	int item_num;
	uint8_t *item_table;
} mode_support_list_t;

/* This structure is used to describe an wifimg object
 * @init: Is wifimg already initialized
 * @enable: Is wifimg already enable
 * @wifi_status: wifi dev status
 * @current_mode: wifi current mode
 * @mutex:
 * @connect_timeout: connect to supplicant timeout
 * */
typedef struct {
	wmg_bool_t init_flag;
	wmg_bool_t enable;
	wifi_dev_status_t wifi_status;
	uint8_t current_mode_bitmap;
	uint8_t support_mode_bitmap;
	os_net_mutex_t mutex_lock;
	mode_object_t* mode_object[MODE_NUM];
	wifi_msg_cb_t wmg_msg_cb;
	mode_support_list_t* mode_support_list;

	wmg_status_t (*init)(void);
	void (*deinit)(void);
	wmg_status_t (*switch_mode)(wifi_mode_t);

	wmg_bool_t (*is_init)(void);

#ifdef SUPPORT_STA_MODE
	wmg_status_t (*sta_connect)(wifi_sta_cn_para_t *);
	wmg_status_t (*sta_disconnect)(void);
	wmg_status_t (*sta_auto_reconnect)(wmg_bool_t);
	wmg_status_t (*sta_get_info)(wifi_sta_info_t *);
	wmg_status_t (*sta_list_networks)(wifi_sta_list_t *);
	wmg_status_t (*sta_remove_networks)(char *);
	wmg_status_t (*sta_get_state)(wifi_wmg_state_t);
#endif

#ifdef SUPPORT_AP_MODE
	wmg_status_t (*ap_enable)(wifi_ap_config_t *);
	wmg_status_t (*ap_disable)(void);
	wmg_status_t (*ap_get_config)(wifi_ap_config_t *);
	wmg_status_t (*ap_get_state)(wifi_wmg_state_t);
#endif

#ifdef SUPPORT_MONITOR_MODE
	wmg_status_t (*monitor_enable)(uint8_t channel);
	wmg_status_t (*monitor_set_channel)(uint8_t channel);
	wmg_status_t (*monitor_disable)(void);
	wmg_status_t (*monitor_get_state)(wifi_wmg_state_t);
#endif

#ifdef SUPPORT_P2P_MODE
	wmg_status_t (*p2p_enable)(wifi_p2p_config_t *p2p_config);
	wmg_status_t (*p2p_disable)(void);
	wmg_status_t (*p2p_find)(wifi_p2p_peers_t *p2p_peers, uint8_t find_second);
	wmg_status_t (*p2p_connect)(uint8_t *p2p_mac_addr);
	wmg_status_t (*p2p_disconnect)(uint8_t *p2p_mac_addr);
	wmg_status_t (*p2p_get_info)(wifi_p2p_info_t *p2p_info);
#endif

	wmg_status_t (*register_msg_cb)(wifi_msg_cb_t msg_cb, void *msg_cb_arg);
	wmg_status_t (*set_scan_param)(wifi_scan_param_t *);
	wmg_status_t (*get_scan_results)(wifi_scan_result_t *, uint32_t *, uint32_t);
	wmg_status_t (*vendor_send_data)(uint8_t *data, uint32_t len);
	wmg_status_t (*vendor_register_rx_cb)(wifi_vend_cb_t cb);
	wmg_status_t (*send_expand_cmd)(char *expand_cmd, void *expand_cb);

	wmg_status_t (*linkd_protocol)(wifi_linkd_mode_t mode, void *params, int second,
			wifi_linkd_result_t *linkd_result);
}wifimg_object_t;

wifimg_object_t* get_wifimg_object(void);

typedef struct {
	wifi_scan_result_t* scan_results;
	uint32_t *bss_num;
	uint32_t arr_size;
} get_scan_results_para_t;

typedef struct {
	wifi_p2p_peers_t *peers;
	uint8_t find_second;
} find_results_para_t;

typedef struct {
	uint8_t *data;
	uint32_t len;
	} vendor_send_data_para_t;

#define WMG_STA_CMD_CONNECT                     0x1
#define WMG_STA_CMD_DISCONNECT                  0x2
#define WMG_STA_CMD_AUTO_RECONNECT              0x3
#define WMG_STA_CMD_GET_INFO                    0x4
#define WMG_STA_CMD_LIST_NETWORKS               0x5
#define WMG_STA_CMD_REMOVE_NETWORKS             0x6
#define WMG_STA_CMD_SCAN_PARAM                  0x7
#define WMG_STA_CMD_SCAN_RESULTS                0x8
#define WMG_STA_CMD_REGISTER_MSG_CB             0x9
#define WMG_STA_CMD_GET_STATE                   0xa
#define WMG_STA_CMD_VENDOR_SEND_DATA            0xb
#define WMG_STA_CMD_VENDOR_REGISTER_RX_CB       0xc

#define WMG_AP_CMD_ENABLE                       0x1
#define WMG_AP_CMD_DISABLE                      0x2
#define WMG_AP_CMD_GET_CONFIG                   0x3
#define WMG_AP_CMD_SCAN_PARAM                   0x4
#define WMG_AP_CMD_SCAN_RESULTS                 0x5
#define WMG_AP_CMD_REGISTER_MSG_CB              0x6
#define WMG_AP_CMD_GET_STATE                    0x7
#define WMG_AP_CMD_VENDOR_SEND_DATA             0x8
#define WMG_AP_CMD_VENDOR_REGISTER_RX_CB        0x9

#define WMG_MONITOR_CMD_ENABLE                  0x1
#define WMG_MONITOR_CMD_SET_CHANNEL             0x2
#define WMG_MONITOR_CMD_DISABLE                 0x3
#define WMG_MONITOR_CMD_REGISTER_MSG_CB         0x4
#define WMG_MONITOR_CMD_GET_STATE               0x5
#define WMG_MONITOR_CMD_VENDOR_SEND_DATA        0x6
#define WMG_MONITOR_CMD_VENDOR_REGISTER_RX_CB   0x7

#define WMG_P2P_CMD_ENABLE              0x1
#define WMG_P2P_CMD_DISABLE             0x2
#define WMG_P2P_CMD_FIND                0x3
#define WMG_P2P_CMD_CONNECT             0x4
#define WMG_P2P_CMD_DISCONNECT          0x5
#define WMG_P2P_CMD_GET_INFO            0x6
#define WMG_P2P_CMD_REGISTER_MSG_CB     0x7
#define WMG_P2P_CMD_GET_STATE           0x8

typedef struct {
	const char* ifname;
	uint8_t *mac_addr;
} common_mac_para_t;

typedef struct {
	wifi_msg_cb_t msg_cb;
	void *msg_cb_arg;
} common_msg_cb_t;

extern act_handle_t wmg_act_handle;

#define WMG_ACT_TABLE_PLA_ID          10
#define WMG_ACT_TABLE_STA_ID          11
#define WMG_ACT_TABLE_AP_ID           12
#define WMG_ACT_TABLE_MONITOR_ID      13
#define WMG_ACT_TABLE_P2P_ID          14

#define WMG_PLA_ACT_WIFI_ON                    0x0
#define WMG_PLA_ACT_WIFI_OFF                   0x1
#define WMG_PLA_ACT_WIFI_REGISTER_MSG_CB       0x2
#define WMG_PLA_ACT_WIFI_SET_MAC               0x3
#define WMG_PLA_ACT_WIFI_GET_MAC               0x4
#define WMG_PLA_ACT_WIFI_SEND_80211_RAW_FRAME  0x5
#define WMG_PLA_ACT_WIFI_SET_COUNTRY_CODE      0x6
#define WMG_PLA_ACT_WIFI_GET_COUNTRY_CODE      0x7
#define WMG_PLA_ACT_WIFI_SET_PS_MODE           0x8
#define WMG_PLA_ACT_WIFI_LINKD_PROTOCOL        0x9
#define WMG_PLA_ACT_SCAN_PARAM                 0xa
#define WMG_PLA_ACT_SCAN_RESULTS               0xb
#define WMG_PLA_ACT_GET_WMG_STATE              0xc
#define WMG_PLA_ACT_VENDOR_SEND_DATA           0xd
#define WMG_PLA_ACT_VENDOR_REGISTER_RX_CB      0xe
#define WMG_PLA_ACT_SEND_EXPAND_CMD            0xf

#define WMG_STA_ACT_CONNECT             0x0
#define WMG_STA_ACT_DISCONNECT          0x1
#define WMG_STA_ACT_AUTO_RECONNECT      0x2
#define WMG_STA_ACT_GET_INFO            0x3
#define WMG_STA_ACT_LIST_NETWORKS       0x4
#define WMG_STA_ACT_REMOVE_NETWORKS     0x5

#define WMG_AP_ACT_ENABLE               0x0
#define WMG_AP_ACT_DISABLE              0x1
#define WMG_AP_ACT_GET_CONFIG           0x2

#define WMG_MONITOR_ACT_ENABLE          0x0
#define WMG_MONITOR_ACT_SET_CHANNEL     0x1
#define WMG_MONITOR_ACT_DISABLE         0x2

#define WMG_P2P_ACT_ENABLE          0x0
#define WMG_P2P_ACT_DISABLE         0x1
#define WMG_P2P_ACT_FIND            0x2
#define WMG_P2P_ACT_CONNECT         0x3
#define WMG_P2P_ACT_DISCONNECT      0x4
#define WMG_P2P_ACT_GET_INFO        0x5

wmg_status_t __wifimanager_init(void);
wmg_status_t __wifimanager_deinit(void);
int __wifi_on(void **call_argv,void **cb_argv);
int __wifi_off(void **call_argv,void **cb_argv);
int __wifi_sta_connect(void **call_argv,void **cb_argv);
int __wifi_sta_disconnect(void **call_argv,void **cb_argv);
int __wifi_sta_auto_reconnect(void **call_argv,void **cb_argv);
int __wifi_sta_get_info(void **call_argv,void **cb_argv);
int __wifi_sta_list_networks(void **call_argv,void **cb_argv);
int __wifi_sta_remove_networks(void **call_argv,void **cb_argv);
int __wifi_ap_enable(void **call_argv,void **cb_argv);
int __wifi_ap_disable(void **call_argv,void **cb_argv);
int __wifi_ap_get_config(void **call_argv,void **cb_argv);
int __wifi_monitor_enable(void **call_argv,void **cb_argv);
int __wifi_monitor_set_channel(void **call_argv,void **cb_argv);
int __wifi_monitor_disable(void **call_argv,void **cb_argv);
int __wifi_p2p_enable(void **call_argv,void **cb_argv);
int __wifi_p2p_disable(void **call_argv,void **cb_argv);
int __wifi_p2p_find(void **call_argv,void **cb_argv);
int __wifi_p2p_connect(void **call_argv,void **cb_argv);
int __wifi_p2p_disconnect(void **call_argv,void **cb_argv);
int __wifi_p2p_get_info(void **call_argv,void **cb_argv);
int __wifi_register_msg_cb(void **call_argv,void **cb_argv);
int __wifi_set_scan_param(void **call_argv,void **cb_argv);
int __wifi_get_scan_results(void **call_argv,void **cb_argv);
int __wifi_set_mac(void **call_argv,void **cb_argv);
int __wifi_get_mac(void **call_argv,void **cb_argv);
int __wifi_send_80211_raw_frame(void **call_argv,void **cb_argv);
int __wifi_set_country_code(void **call_argv,void **cb_argv);
int __wifi_get_country_code(void **call_argv,void **cb_argv);
int __wifi_set_ps_mode(void **call_argv,void **cb_argv);
int __wifi_get_wmg_state(void **call_argv,void **cb_argv);
int __wifi_vendor_send_data(void **call_argv,void **cb_argv);
int __wifi_vendor_register_rx_cb(void **call_argv,void **cb_argv);
int __wifi_send_expand_cmd(void **call_argv,void **cb_argv);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __WMG_COMMON_H__ */
