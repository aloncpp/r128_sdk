/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */


#ifndef _AVRCP_H_
#define _AVRCP_H_

#include <stdint.h>
#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define AVRCP_PANEL             0x09
#define AVRCP_SUB_UNIT_ID       0x00

#define AVRCP_PASS_THROUGH_KEY_PRESS    0x00
#define AVRCP_PASS_THROUGH_KEY_RELEASE  0x80

#define COMPANY_INVALID                 0xffffff
#define COMPANY_BT_SIG                  0x001958

/** Get Element Attributes */
#define MAX_ELEMENT_ATTR_NUM    8
typedef enum t_avrcp_element_attr_type
{
    ELEMENT_ATTR_TITLE             = 0x01,
    ELEMENT_ATTR_ARTIST            = 0x02,
    ELEMENT_ATTR_ALBUM             = 0x03,
    ELEMENT_ATTR_TRACK             = 0x04,
    ELEMENT_ATTR_TOTAL_TRACK       = 0x05,
    ELEMENT_ATTR_GENRE             = 0x06,
    ELEMENT_ATTR_PLAYING_TIME      = 0x07,
    ELEMENT_ATTR_DEFAULT_COVER_ART = 0x08,
} T_AVRCP_ELEMENT_ATTR_TYPE;

/*play status*/
typedef enum t_avrcp_playstatus_type
{
    AVRCP_PLAYSTATUS_STOPPED  = 0x00,
    AVRCP_PLAYSTATUS_PLAYING  = 0x01,
    AVRCP_PLAYSTATUS_PAUSED   = 0x02,
    AVRCP_PLAYSTATUS_FWD_SEEK = 0x03,
    AVRCP_PLAYSTATUS_REV_SEEK = 0x04,
    AVRCP_PLAYSTATUS_FAST_FWD = 0x05,
    AVRCP_PLAYSTATUS_REWIND   = 0x06,
} T_AVRCP_PLAYSTATUS_TYPE;

/*event id */
typedef enum t_event_id
{
    EVENT_PLAYBACK_STATUS_CHANGED = 0x01,       /* (CT) Change in playback status of the current track */
    EVENT_TRACK_CHANGED           = 0x02,       /* (CT) Change of current track */
} T_EVENT_ID;

/*capability id*/
typedef enum t_capability_id
{
    CAPABILITY_ID_COMPANY_ID       = 0x02,
    CAPABILITY_ID_EVENTS_SUPPORTED = 0x03,
} T_CAPABILITY_ID;

typedef enum t_avrcp_msg
{
    AVRCP_MSG_CONN_IND             = 0x00,
    AVRCP_MSG_CONN_CMPL            = 0x01,
    AVRCP_MSG_DISCONN              = 0x02,

    /*volume sync feature as catogory2(amplifier) TG*/
    AVRCP_MSG_CMD_VOL_UP           = 0x03,
    AVRCP_MSG_CMD_VOL_DOWN         = 0x04,
    AVRCP_MSG_CMD_ABS_VOL          = 0x05,
    AVRCP_MSG_CMD_REG_VOL_CHANGE   = 0x06,

    /*catogory1(player) CT*/
    AVRCP_MSG_RSP_UNIT_INFO        = 0x07,
    AVRCP_MSG_RSP_PASSTHROUGH      = 0x08,
    AVRCP_MSG_RSP_GET_CPBS         = 0x09,
    AVRCP_MSG_RSP_GET_PLAYSTATUS   = 0x0a,
    AVRCP_MSG_RSP_GET_ELEMENT_ATTR = 0x0b,
    AVRCP_MSG_RSP_REG_NOTIFICATION = 0x0c,

    AVRCP_MSG_RSP_DUMMY            = 0x0d,

    AVRCP_MSG_NOTIF_CHANGED        = 0x0e,
    AVRCP_MSG_RCV_RSP              = 0x0f,
    AVRCP_MSG_ERR                  = 0xff,
} T_AVRCP_MSG;

typedef enum t_avrcp_key
{
    AVRCP_KEY_PLAY          = 0x44,
    AVRCP_KEY_STOP          = 0x45,
    AVRCP_KEY_PAUSE         = 0x46,
    AVRCP_KEY_FORWARD       = 0x4B,
    AVRCP_KEY_BACKWARD      = 0x4C,
    AVRCP_KEY_FAST_FORWARD  = 0x49,
    AVRCP_KEY_REWIND        = 0x48,
} T_AVRCP_KEY;

typedef struct t_element_attr
{
    uint32_t attribute_id;
    uint16_t character_set_id;
    uint16_t length;
    uint8_t *p_buf;
} T_ELEMENT_ATTR;

typedef enum t_avrcp_rsp_state
{
    AVRCP_RSP_STATE_SUCCESS = 0x00,
    AVRCP_RSP_STATE_FAIL    = 0x01,
//  AVRCP_REMOTE_REJECT?
} T_AVRCP_RSP_STATE;

typedef enum t_avrcp_msg_err
{
    AVRCP_WAIT_RSP_TO = 0x00,
} T_AVRCP_MSG_ERR;

typedef struct t_rsp_unit_info
{
    T_AVRCP_RSP_STATE state;
    uint8_t sub_unit_type;
    uint8_t sub_unit_id;
    uint32_t company_id;
} T_RSP_UNIT_INFO;

typedef struct t_rsp_passthrough
{
    T_AVRCP_RSP_STATE state;
    T_AVRCP_KEY key;
    bool pressed;
} T_RSP_PASSTHROUGH;

typedef struct t_rsp_cpbs
{
    T_AVRCP_RSP_STATE state;
    uint8_t capability_id;
    uint8_t capability_count;
    uint8_t *p_buf;
} T_RSP_CPBS;

typedef struct t_rsp_get_play_status
{
    T_AVRCP_RSP_STATE state;
    uint32_t length_ms;
    uint32_t position_ms;
    uint8_t play_status;
} T_RSP_GET_PLAY_STATUS;

typedef struct t_rsp_get_element_attr
{
    T_AVRCP_RSP_STATE state;
    uint8_t num_of_attr;
    T_ELEMENT_ATTR attr[MAX_ELEMENT_ATTR_NUM];
} T_RSP_GET_ELEMENT_ATTR;

typedef struct t_rsp_reg_notification
{
    T_AVRCP_RSP_STATE state;
    uint8_t event_id;
    union
    {
        uint8_t play_status;
        unsigned long long *p_track_id; //0xFFFFFFFFFFFFFFFF means no track;
    } u;
} T_RSP_REG_NOTIFICATION;

typedef struct t_notification_changed
{
    uint8_t event_id;
    union
    {
        uint8_t play_status;
        unsigned long long *p_track_id;
    } u;
} T_NOTIF_CHANGED;

typedef void (*P_AVRCP_CBACK)(uint8_t *bd_addr, T_AVRCP_MSG msg_type, void *msg_buf);
typedef void (*P_AVRCP_VENDOR_CMD_CALLBACK)(uint8_t *bd_addr, uint32_t company_id, uint8_t *p_pdu,
                                            uint16_t length, uint8_t ctype, uint8_t transact);
typedef void (*P_AVRCP_VENDOR_RSP_CALLBACK)(uint8_t *bd_addr, uint32_t company_id, uint8_t *p_pdu,
                                            uint16_t length, uint8_t response);

bool avrcp_init(P_AVRCP_CBACK cback, uint32_t company_id,
                P_AVRCP_VENDOR_CMD_CALLBACK vendor_cmd_handler, P_AVRCP_VENDOR_RSP_CALLBACK vendor_rsp_handler);

bool avrcp_conn_req(uint8_t *bd_addr);
bool avrcp_disconn_req(uint8_t *bd_addr);
bool avrcp_conn_cfm(uint8_t *bd_addr, bool accept);
bool avrcp_send_unit_info(uint8_t *bd_addr);
bool avrcp_send_pass_through(uint8_t *bd_addr, T_AVRCP_KEY key, bool pressed);
bool avrcp_get_capability(uint8_t *bd_addr, uint8_t capability_id);
bool avrcp_get_play_status(uint8_t *bd_addr);
bool avrcp_get_element_attr(uint8_t *bd_addr, uint8_t attr_num, ...);
bool avrcp_register_notification(uint8_t *bd_addr, uint8_t event_id);
bool avrcp_notify_volume_change(uint8_t *bd_addr, uint8_t volume);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _AVRCP_H_ */
