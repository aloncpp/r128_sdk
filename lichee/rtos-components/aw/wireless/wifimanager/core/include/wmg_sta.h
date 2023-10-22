/*
 * Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
 */

#ifndef __WMG_STATION_H__
#define __WMG_STATION_H__

#include <wmg_common.h>
#ifdef OS_NET_FREERTOS_OS
#include <freertos/event.h>
#else
#include <linux/event.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef EVENT_BUF_SIZE
#define EVENT_BUF_SIZE 1024
#endif

/* These commands can be selectively implemented by different systems
 * but not all of them need to be implemented.
 */
#define STA_CMD_CONNECT                        0x0
#define STA_CMD_DISCONNECT                     0x1
#define STA_CMD_GET_INFO                       0x2
#define STA_CMD_LIST_NETWORKS                  0x3
#define STA_CMD_REMOVE_NETWORKS                0x4
#define STA_CMD_SET_AUTO_RECONN                0x5
#define STA_CMD_GET_SCAN_RESULTS               0x6
#define STA_CMD_GET_STATE                      0x7
#define STA_CMD_VENDOR_SEND_DATA               0x8
#define STA_CMD_VENDOR_REGISTER_RX_CB          0x9

typedef void(*sta_event_cb_t)(wifi_sta_event_t event);

typedef struct {
	wmg_bool_t sta_init_flag;
	wmg_bool_t sta_enable_flag;
	event_handle_t *sta_event_handle;
	sta_event_cb_t sta_event_cb;
	wmg_bool_t sta_auto_reconn;
	void *sta_private_data;
	wmg_status_t (*sta_inf_init)(sta_event_cb_t,void*);
	wmg_status_t (*sta_inf_deinit)(void*);
	wmg_status_t (*sta_inf_enable)(void*);
	wmg_status_t (*sta_inf_disable)(void*);
	wmg_status_t (*sta_platform_extension)(int,void*,int*);
} wmg_sta_inf_object_t;

mode_object_t* wmg_sta_register_object(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __WMG_STATION_H__ */
