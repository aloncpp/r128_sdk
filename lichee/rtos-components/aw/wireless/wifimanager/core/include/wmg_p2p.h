/*
 * Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
 */

#ifndef __WMG_P2P_H__
#define __WMG_P2P_H__

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

#ifndef EVENT_BUF_SIZE
#define EVENT_BUF_SIZE 1024
#endif

#define P2P_CMD_ENABLE     0
#define P2P_CMD_DISABLE    1
#define P2P_CMD_FIND       2
#define P2P_CMD_CONNECT    3
#define P2P_CMD_DISCONNECT 4
#define P2P_CMD_GET_INFO   5
#define P2P_CMD_GET_STATE  6

#define P2P_ROLE_GO        0
#define P2P_ROLE_GC        1

typedef void(*p2p_event_cb_t)(wifi_p2p_event_t event);

typedef struct {
	wmg_bool_t p2p_init_flag;
	wmg_bool_t p2p_enable_flag;
	event_handle_t *p2p_event_handle;
	p2p_event_cb_t p2p_event_cb;
	int p2p_role;
	wmg_bool_t auto_connect;
	void *p2p_private_data;
	wmg_status_t (*p2p_inf_init)(p2p_event_cb_t,void*);
	wmg_status_t (*p2p_inf_deinit)(void*);
	wmg_status_t (*p2p_inf_enable)(void*);
	wmg_status_t (*p2p_inf_disable)(void*);
	wmg_status_t (*p2p_platform_extension)(int,void*,int*);
} wmg_p2p_inf_object_t;

mode_object_t* wmg_p2p_register_object(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __WMG_P2P_H__ */
