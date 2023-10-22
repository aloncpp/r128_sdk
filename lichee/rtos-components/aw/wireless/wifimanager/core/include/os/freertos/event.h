/*
 * Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
 */

#ifndef __EVENT_H__
#define __EVENT_H__

#include "wmg_common.h"

#if __cplusplus
extern "C" {
#endif

#define EVENT_BUF_SIZE 2048
#define EVENT_TRY_MAX 5
#define MAX_ASSOC_REJECT_COUNT  5
#define MAX_RETRIES_ON_AUTHENTICATION_FAILURE 2

typedef enum {
    WPAE_CONNECTED = 1,
    WPAE_DISCONNECTED,
    WPAE_STATE_CHANGE,
    WPAE_SCAN_FAILED,
    WPAE_SCAN_RESULTS,
    WPAE_LINK_SPEED,
    WPAE_TERMINATING,
    WPAE_DRIVER_STATE,
    WPAE_EAP_FAILURE,
    WPAE_ASSOC_REJECT,
    WPAE_NETWORK_NOT_FOUND,
    WPAE_PASSWORD_INCORRECT,
    WPAE_UNKNOWN,
} wpas_event_t;

typedef enum {
    HAPD_UNKNOWN,
} ap_event_t;

#define NET_ID_LEN 10

/**
 * event_handle_t - socket pair handle
 *
 * @wpas_evt: wpa_supplicant event
 * @ap_evt: hostpad event
 * @evt_socket: socket pair which can send and recv data to each other
 * @evt_socket_enable: socket pair initialized success or not
 */
typedef struct {
	wpas_event_t wpas_evt;
	ap_event_t ap_evt;
	int evt_socket[2];
	wmg_bool_t evt_socket_enable;
} event_handle_t;

int evt_socket_init(event_handle_t *evt_handle);
void evt_socket_exit(event_handle_t *evt_handle);
int evt_socket_clear(event_handle_t *evt_handle);
int evt_send(event_handle_t *evt_handle, int event);
int evt_read(event_handle_t *evt_handle, int *event);

#if __cplusplus
}
#endif

#endif /* __EVENT_H__ */
