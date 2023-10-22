/*
 * Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
 */

#ifndef __LINKD_H__
#define __LINKD_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <wifimg.h>
#include <pthread.h>

#define WMG_LINKD_PROTO_MAX     (4)
#define DEFAULT_SECOND          180

typedef void (*proto_result_cb)(wifi_linkd_result_t *linkd_result);
typedef void *(*proto_main_loop)(void *arg);
typedef struct {
	proto_result_cb result_cb;
	void *private;
} proto_main_loop_para_t;

typedef enum {
	WMG_LINKD_IDEL,
	WMG_LINKD_RUNNING,
	WMG_LINKD_OFF,
} wmg_linkd_state_t;

typedef enum {
	WMG_LINKD_RESULT_SUCCESS,
	WMG_LINKD_RESULT_FAIL,
	WMG_LINKD_RESULT_INVALIN,
} wmg_linkd_result_state_t;

typedef struct {
	wmg_linkd_state_t linkd_state;
	wifi_linkd_mode_t linkd_mode_state;
	proto_main_loop *proto_main_loop_list;
	proto_main_loop_para_t main_loop_para;
	char ssid_result[SSID_MAX_LEN];
	char psk_result[PSK_MAX_LEN];
	wmg_linkd_result_state_t result_state;
	pthread_t thread;
	wmg_status_t (*linkd_init)(void);
	wmg_status_t (*linkd_protocol_enable)(wifi_linkd_mode_t mode, void *proto_function_param);
	wmg_status_t (*linkd_protocol_get_results)(wifi_linkd_result_t *linkd_result, int second);
	void (*linkd_deinit)(void);
} wmg_linkd_object_t;

wmg_status_t wmg_linkd_protocol(wifi_linkd_mode_t mode, void *params, int second, wifi_linkd_result_t *linkd_result);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __LINKD_H__ */
