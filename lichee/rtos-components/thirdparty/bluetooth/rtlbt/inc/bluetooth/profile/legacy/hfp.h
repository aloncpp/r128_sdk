/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */


#ifndef _HFP_H_
#define _HFP_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum t_hfp_bat_reporting_type
{
    BAT_REPORTING_TYPE_NONE    = 0x00,
    BAT_REPORTING_TYPE_BIEV    = 0x01,
    BAT_REPORTING_TYPE_APPLE   = 0x02,    //supports AT+IPHONEACCEV cmd (iDevices and some Android phone eg meizu)
    BAT_REPORTING_TYPE_ANDROID = 0x03,
} T_HFP_BAT_REPORTING_TYPE;

typedef enum t_hfp_ag_indicator_type
{
    AG_INDICATOR_TYPE_SERVICE   = 0x00,
    AG_INDICATOR_TYPE_CALL      = 0x01,
    AG_INDICATOR_TYPE_CALLSETUP = 0x02,
    AG_INDICATOR_TYPE_CALLHELD  = 0x03,
    AG_INDICATOR_TYPE_SIGNAL    = 0x04,
    AG_INDICATOR_TYPE_ROAM      = 0x05,
    AG_INDICATOR_TYPE_BATTCHG   = 0x06,
    AG_INDICATOR_TYPE_SMS_RCV   = 0x07,
    AG_INDICATOR_TYPE_VOICEMAIL = 0x08,
    AG_INDICATOR_TYPE_SMSFULL   = 0x09,

    AG_INDICATOR_TYPE_UNKOWN    = 0xff,
} T_HFP_AG_INDICATOR_TYPE;

/* stores whether this AT cmd is app lanched or profile lanched, determine whether send ok/err/cme err to app*/
#define HFP_AT_CMD_APP_LANCHED  1 << 0
/*stores whether it's a vendor nake AT cmd sent from app, determine whether send nake +XXXX rsp to app*/
#define HFP_AT_CMD_APP_NAKE_AT  1 << 1

/* bitfield containing the capabilities of the mobile phone (AG)*/
typedef uint32_t T_HFP_AG_CPBS;
#define AG_CAPABILITY_3WAY                              (1 << 0)
#define AG_CAPABILITY_EC_NR                             (1 << 1)
#define AG_CAPABILITY_VOICE_RECOGNITION                 (1 << 2)
#define AG_CAPABILITY_IN_BAND_RINGING                   (1 << 3)
#define AG_CAPABILITY_VOICE_TAG                         (1 << 4)
#define AG_CAPABILITY_REJECT_CALLS                      (1 << 5)
#define AG_CAPABILITY_ENHANCED_CALL_STATUS              (1 << 6)
#define AG_CAPABILITY_ENHANCED_CALL_CONTROL             (1 << 7)
#define AG_CAPABILITY_EXTENED_ERROR_RESULT              (1 << 8)
#define AG_CAPABILITY_CODEC_NEGOTIATION                 (1 << 9)
#define AG_CAPABILITY_HF_INDICATORS                     (1 << 10)
#define AG_CAPABILITY_ESCO_S4_T2_SUPPORTED              (1 << 11)

#define AG_CAPABILITY_DEFAULT              (AG_CAPABILITY_3WAY|AG_CAPABILITY_IN_BAND_RINGING)


/* bitfield containing the capabilities of the handsfree unit (HF)*/
#define HF_CAPABILITY_EC_NR                            (1 << 0)
#define HF_CAPABILITY_CALL_WAITING_AND_3WAY            (1 << 1)
#define HF_CAPABILITY_CLI                              (1 << 2)
#define HF_CAPABILITY_VOICE_RECOGNITION                (1 << 3)
#define HF_CAPABILITY_REMOTE_VOLUME_CONTROL            (1 << 4)
#define HF_CAPABILITY_ENHANCED_CALL_STATUS             (1 << 5)
#define HF_CAPABILITY_ENHANCED_CALL_CONTROL            (1 << 6)
#define HF_CAPABILITY_CODEC_NEGOTIATION                (1 << 7)
#define HF_CAPABILITY_HF_INDICATORS                    (1 << 8)
#define HF_CAPABILITY_ESOC_S4_T2_SUPPORTED             (1 << 9)

typedef enum t_hfp_call_status
{
    HFP_CALL_STATUS_IDLE   = 0x00,
    HFP_CALL_STATUS_ACTIVE = 0x01,
} T_HFP_CALL_STATUS;

/* call setup status of the phone as indicated */
typedef enum t_hfp_call_setup_status
{
    HFP_CALL_SETUP_STATUS_IDLE          = 0x00,
    HFP_CALL_SETUP_STATUS_INCOMING_CALL = 0x01,
    HFP_CALL_SETUP_STATUS_OUTGOING_CALL = 0x02,
    HFP_CALL_SETUP_STATUS_ALERTING      = 0x03,
} T_HFP_CALL_SETUP_STATUS;

/* call held status of the phone as indicated */
typedef enum t_hfp_call_held_status
{
    HFP_CALL_HELD_STATUS_IDLE                 = 0x00,
    HFP_CALL_HELD_STATUS_HOLD_AND_ACTIVE_CALL = 0x01,
    HFP_CALL_HELD_STATUS_HOLD_NO_ACTIVE_CALL  = 0x02,
} T_HFP_CALL_HELD_STATUS;

/* service (registration) status of the phone as indicated */
typedef enum t_hfp_service_status
{
    HFP_SERVICE_STATUS_NOT_REG = 0x00,
    HFP_SERVICE_STATUS_REG     = 0x01,
} T_HFP_SERVICE_STATUS;

/*voice recognition status of the phone as indicated */
typedef enum t_hfp_voice_recognition
{
    HFP_VOICE_RECOGNITION_IDLE   = 0x00,
    HFP_VOICE_RECOGNITION_ACTIVE = 0x01,
} T_HFP_VOICE_RECOGNITION;

typedef enum t_hfp_active_codec_type
{
    CODEC_TYPE_CVSD = 0x01,
    CODEC_TYPE_MSBC = 0x02,
} T_HFP_ACTIVE_CODEC_TYPE;

typedef enum t_hfp_api_id
{
    HFP_API_ID_NAKE_AT_CMD                    = 0x00,
    HFP_API_ID_DIAL_WITH_NUMBER               = 0x01,
    HFP_API_ID_DIAL_LAST_NUMBER               = 0x02,
    HFP_API_ID_SEND_DTMF                      = 0x03,
    HFP_API_ID_ACCEPT_CALL                    = 0x04,
    HFP_API_ID_REJECT_HANGUP_CALL             = 0x05,
    HFP_API_ID_SET_VOICE_RECOGNITION_ACTIVE   = 0x06,
    HFP_API_ID_SET_VOICE_RECOGNITION_INACTIVE = 0x07,
    HFP_API_ID_3WAY_CALL_CONTROL_0            = 0x08,
    HFP_API_ID_3WAY_CALL_CONTROL_1            = 0x09,
    HFP_API_ID_3WAY_CALL_CONTROL_2            = 0x0a,
    HFP_API_ID_3WAY_CALL_CONTROL_3            = 0x0b,
    HFP_API_ID_3WAY_CALL_CONTROL_AB           = 0x0c,
    HFP_API_ID_INFORM_MICROPHONE_GAIN         = 0x0d,
    HFP_API_ID_INFORM_SPEAKER_GAIN            = 0x0e,
    HFP_API_ID_HSP_BUTTON_PRESS               = 0x0f,
    HFP_API_ID_SEND_CLCC                      = 0x10,
    HFP_API_ID_ESTABLISH_VOICE                = 0x11,
    HFP_API_ID_XAPL                           = 0x12,
    HFP_API_ID_IPHONEACCEV                    = 0x13,
    HFP_API_ID_XEVENT_BATT                    = 0x14,
    HFP_API_ID_CLIP                           = 0x15,
    HFP_API_ID_NREC                           = 0x16,
    HFP_API_ID_VENDOR                         = 0x17,
} T_HFP_API_ID;


typedef enum t_hfp_msg
{
    HFP_MSG_RFC_CONN_IND               = 0x00,
    HFP_MSG_RFC_CONN                   = 0x01,
    HFP_MSG_CONN                       = 0x02,
    HFP_MSG_DISCONN                    = 0x03,

    HFP_MSG_CMD_ACK_OK                 = 0x04,
    HFP_MSG_CMD_ACK_ERROR              = 0x05,
    HFP_MSG_CME_ERROR                  = 0x06,
    HFP_MESSAGE_WAIT_ACK_TIMEOUT       = 0x07,

    HFP_MSG_AG_INDICATOR_EVENT         = 0x08,
    HFP_MSG_RING                       = 0x09,
    HFP_MSG_CLIP                       = 0x0a,
    HFP_MSG_CCWA                       = 0x0b,
    HFP_MSG_CLCC                       = 0x0c,
    HFP_MSG_VOICE_RECOGNITION_STATUS   = 0x0d,
    HFP_MSG_AG_INBAND_RINGTONE_SETTING = 0x0e,
    HFP_MSG_AG_BIND                    = 0x0f,

    HFP_MSG_SET_MICROPHONE_GAIN        = 0x10,
    HFP_MSG_SET_SPEAKER_GAIN           = 0x11,
    HFP_MSG_SET_CODEC_TYPE             = 0x12,
    HFP_MSG_UNKOWN_AT_CMD              = 0x13,
} T_HFP_MSG;

typedef struct t_hfp_msg_rfc_conn
{
    uint8_t             bd_addr[6];
    bool                hsp;
} T_HFP_MSG_RFC_CONN;
typedef struct t_hfp_msg_conn
{
    uint8_t             bd_addr[6];
} T_HFP_MSG_CONN;
typedef T_HFP_MSG_CONN T_HFP_MSG_DISCONN;

typedef struct t_hfp_msg_ack_ok
{
    uint8_t             bd_addr[6];
    uint8_t             last_api_id;
} T_HFP_MSG_ACK_OK;

typedef struct t_hfp_msg_cme_error
{
    uint8_t             bd_addr[6];
    uint8_t             last_api_id;
    uint8_t             error_number;
} T_HFP_MSG_CME_ERROR;

typedef T_HFP_MSG_CME_ERROR T_HFP_MSG_ACK_ERROR;
typedef T_HFP_MSG_ACK_OK T_HFP_MSG_WAIT_RSP_TOUT;

typedef struct t_hfp_msg_ag_indicator_event
{
    uint8_t             bd_addr[6];
    T_HFP_AG_INDICATOR_TYPE event_type;
    uint8_t             state;
} T_HFP_MSG_AG_INDICATOR_EVENT;
typedef struct t_hfp_msg_ring
{
    uint8_t             bd_addr[6];
} T_HFP_MSG_RING;
typedef struct t_hfp_msg_clip
{
    uint8_t             bd_addr[6];
    char                *num_string;
    uint8_t             type;
} T_HFP_MSG_CLIP;
typedef T_HFP_MSG_CLIP T_HFP_MSG_CCWA;

typedef enum t_clcc_status
{
    CLCC_STATUS_ACTIVE                    = 0x00,
    CLCC_STATUS_HELD                      = 0x01,
    CLCC_STATUS_DIALING                   = 0x02,
    CLCC_STATUS_ALERTING                  = 0x03,
    CLCC_STATUS_INCOMING                  = 0x04,
    CLCC_STATUS_WAITING                   = 0x05,
    CLCC_STATUS_CALL_HELD_BY_RSP_AND_HOLD = 0x06,
} T_CLCC_STATUS;

typedef enum t_clcc_mode
{
    CLCC_MODE_VOICE = 0x00,
    CLCC_MODE_DATA  = 0x01,
    CLCC_MODE_FAX   = 0x02,
} T_CLCC_MODE;

typedef enum t_hfp_call_hold_action
{
    RELEASE_HELD_OR_WAITING_CALL                    = 0x00,
    RELEASE_ACTIVE_CALL_ACCEPT_HELD_OR_WAITING_CALL = 0x01,
    HOLD_ACTIVE_CALL_ACCEPT_HELD_OR_WAITING_CALL    = 0x02,
    JOIN_TWO_CALLS                                  = 0x03,
} T_HFP_CALL_HOLD_ACTION;


typedef struct t_hfp_msg_clcc
{
    uint8_t             bd_addr[6];
    uint8_t             call_idx;
    uint8_t             dir_incoming;
    uint8_t             status;//T_CLCC_STATUS         status;
    uint8_t             mode;//T_CLCC_MODE           mode;
    uint8_t             is_multi_party;
    //char                *phone_number;
    //uint8_t             type;
} T_HFP_MSG_CLCC;

typedef struct t_hfp_msg_voice_recognition_status
{
    uint8_t             bd_addr[6];
    uint8_t             status;
} T_HFP_MSG_VOICE_RECOGNITION_STATUS;
typedef struct t_hfp_msg_ag_inband_ringtone_set
{
    uint8_t             bd_addr[6];
    bool                ag_support;
} T_HFP_MSG_AG_INBAND_RINGTONE_SET;

typedef struct t_hfp_msg_bind_status
{
    uint8_t             bd_addr[6];
    bool                batt_ind_enable;
} T_HFP_MSG_BIND_STATUS;

typedef struct t_hfp_msg_set_microphone_volume
{
    uint8_t             bd_addr[6];
    uint8_t             volume;
} T_HFP_MSG_SET_MICROPHONE_VOLUME;
typedef T_HFP_MSG_SET_MICROPHONE_VOLUME T_HFP_MSG_SET_SPEAKER_VOLUME;

typedef struct t_hfp_msg_unkown_at_cmd
{
    uint8_t             bd_addr[6];
    char                *at_cmd;
} T_HFP_MSG_UNKOWN_AT_CMD;

typedef struct t_hfp_msg_set_codec_type
{
    uint8_t             bd_addr[6];
    T_HFP_ACTIVE_CODEC_TYPE codec_type;
} T_HFP_MSG_SET_CODEC_TYPE;

typedef void (*P_HFP_CALLBACK)(uint8_t *bd_addr, T_HFP_MSG msg_type, void *msg_buf);

bool hfp_init(P_HFP_CALLBACK cback, uint16_t hfp_brsf_cpbs, uint16_t voice_codec);

bool hfp_conn_req(uint8_t *bd_addr, uint8_t remote_dlci, uint8_t hfp_flag);
bool hfp_disconn_req(uint8_t *bd_addr); //bd_addr == null means default link

bool hfp_dial_with_number(uint8_t *bd_addr, const char *number);
bool hfp_dial_last_number(uint8_t *bd_addr);
bool hfp_send_dtmf(uint8_t *bd_addr, char c); //Dual Tone Multi-Frequency code
bool hfp_accept_phone_call(uint8_t *bd_addr);
bool hfp_reject_phone_call(uint8_t *bd_addr);
bool hfp_set_voice_recognition(uint8_t *bd_addr, bool enable);
bool hfp_call_hold_action(uint8_t *bd_addr, T_HFP_CALL_HOLD_ACTION control);
bool hfp_3way_call_control_with_index(uint8_t *bd_addr, uint8_t control, uint8_t index);

bool hfp_inform_ag_microphone_gain(uint8_t *bd_addr, uint8_t level);
bool hfp_inform_ag_speaker_gain(uint8_t *bd_addr, uint8_t level);
bool hfp_send_clcc(uint8_t *bd_addr);
bool hfp_send_clip(uint8_t *bd_addr);
bool hfp_send_nrec_disable(uint8_t *bd_addr);

bool hfp_hsp_button_press(uint8_t *bd_addr);

bool app_send_at_cmd(uint8_t *bd_addr, const char *at_cmd, uint8_t cmd_flag,
                     uint8_t api_id);

bool hfp_get_ag_capability(uint8_t *bd_addr, T_HFP_AG_CPBS *ret);
bool hfp_ask_ag_establish_voice_chann(uint8_t *bd_addr);

uint16_t  hfp_get_hf_capability(void);
bool hfp_conn_cfm(uint8_t *bd_addr, bool accept);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _HFP_H_ */
