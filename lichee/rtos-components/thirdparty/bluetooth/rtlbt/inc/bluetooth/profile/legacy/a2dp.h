/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */


#ifndef _A2DP_H_
#define _A2DP_H_

#include <stdint.h>
#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum t_a2dp_role
{
    A2DP_ROLE_SNK = 0x00,
    A2DP_ROLE_SRC = 0x01,
} T_A2DP_ROLE;

/** A2DP source/sink state */
typedef enum t_a2dp_state
{
    A2DP_STATE_DISCONNECTED  = 0x00,
    A2DP_STATE_ALLOCATED     = 0x01,
    A2DP_STATE_CONNECTING    = 0x02,
    A2DP_STATE_CONNECTED     = 0x03,
    A2DP_STATE_DISCONNECTING = 0x04,
    A2DP_STATE_STREAMING     = 0x05,
} T_A2DP_STATE;

typedef enum t_a2dp_avdtp_param_type
{
    A2DP_PARAM_LATENCY    = 0x00,
    A2DP_PARAM_CODEC_TYPE = 0x01,
    A2DP_START_SIG_TIMER  = 0x02,
    A2DP_PARAM_ROLE       = 0x03,

    AVDTP_DISCOVER_RETRY_TIMER = 0x10,
    AVDTP_DISCOVER_END_TIMER   = 0x11,
    AVDTP_SIG_TIMER            = 0x12,
} T_A2DP_AVDTP_PARAM_TYPE;

typedef enum t_a2dp_msg
{
    A2DP_MSG_CONN_IND    = 0x00,
    A2DP_MSG_CONN_CMPL   = 0x01,
    A2DP_MSG_DISCONN     = 0x02,
    A2DP_MSG_SET_CFG     = 0x03,
    A2DP_MSG_RE_CFG      = 0x04,
    A2DP_MSG_OPEN        = 0x05,
    A2DP_MSG_START       = 0x06,
    A2DP_MSG_SUSPEND     = 0x07,
    A2DP_MSG_STOP        = 0x08,
    A2DP_MSG_ABORT       = 0x09,
    A2DP_MSG_STREAM_IND  = 0x0a,
    A2DP_MSG_STREAM_CONN = 0x0b,
} T_A2DP_MSG;

typedef struct t_a2dp_cfg
{
    uint8_t     codec_type;
    uint8_t     cp_flag;
    uint8_t     delay_report_flag;
    uint8_t     sample_frequency;
    uint8_t     channel_mode;
    uint8_t     vendor_codec_info[12];
    uint8_t     sbc_encode_hdr;
} T_A2DP_CFG;

typedef struct t_a2dp_stream_ind
{
    uint8_t    *pkt_ptr;
    uint16_t    pkt_len;
} T_A2DP_STREAM_IND;

typedef void(*P_A2DP_CBACK)(uint8_t *bd_addr, T_A2DP_MSG msg_type, void *msg_buf);

bool a2dp_init(P_A2DP_CBACK cback);
bool a2dp_set_param(T_A2DP_AVDTP_PARAM_TYPE type, uint16_t value);
bool a2dp_send_cmd(uint8_t *bd_addr, uint8_t signal_id);
bool a2dp_send_stream(uint8_t *bd_addr, uint8_t *p_data, uint16_t len, uint8_t frame_no);
bool a2dp_conn_signal_req(uint8_t *bd_addr, uint16_t avdtp_ver);
bool a2dp_conn_stream_req(uint8_t *bd_addr);
bool a2dp_signal_conn_cfm(uint8_t *bd_addr, bool accept);
bool a2dp_disconn_req(uint8_t *bd_addr);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _A2DP_H_ */
