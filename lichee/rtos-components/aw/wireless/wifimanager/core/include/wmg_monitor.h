/*
 * Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
 */

#ifndef __WMG_MONITOR_H__
#define __WMG_MONITOR_H__

#include <wmg_common.h>
#ifdef OS_NET_FREERTOS_OS
#include <freertos/event.h>
#else
#include <linux/event.h>
#endif
#include <wifimg.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MON_BUF_SIZE
#define MON_BUF_SIZE    4096
#endif

#ifndef MON_IF_SIZE
#define MON_IF_SIZE     20
#endif

#define MONITOR_CMD_ENABLE                 0x0
#define MONITOR_CMD_DISABLE                0x1
#define MONITOR_CMD_SET_CHANNL             0x2
#define MONITOR_CMD_GET_STATE              0x3
#define MONITOR_CMD_VENDOR_SEND_DATA       0x4
#define MONITOR_CMD_VENDOR_REGISTER_RX_CB  0x5

typedef void(*monitor_data_frame_cb_t)(wifi_monitor_data_t *frame);

typedef struct {
	struct nl_sock *nl_sock;
	int nl80211_id;
} nl80211_state_t;

typedef struct {
	wmg_bool_t monitor_init_flag;
	wmg_bool_t monitor_enable;
	os_net_thread_t monitor_pid;
	nl80211_state_t *monitor_state;
	monitor_data_frame_cb_t monitor_data_frame_cb;
	uint8_t monitor_channel;
	void *monitor_private_data;
	char monitor_if[MON_IF_SIZE];
	wmg_status_t (*monitor_inf_init)(monitor_data_frame_cb_t,void*);
	wmg_status_t (*monitor_inf_deinit)(void*);
	wmg_status_t (*monitor_inf_enable)(void*);
	wmg_status_t (*monitor_inf_disable)(void*);
	wmg_status_t (*monitor_platform_extension)(int,void*,int*);
} wmg_monitor_inf_object_t;

mode_object_t* wmg_monitor_register_object(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __WMG_MONITOR_H__ */
