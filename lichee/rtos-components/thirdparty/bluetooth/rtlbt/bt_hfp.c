/*
* Copyright (C) 2018 Realtek Semiconductor Corporation. All Rights Reserved.
*/

#include <stdio.h>
#include <string.h>

#include "trace_app.h"

#include "gap_legacy.h"

#include "bt_hfp.h"
#include "bt_link.h"

#define SCO_PKT_TYPES_HV3_EV3_2EV3  (GAP_PKT_TYPE_HV3 | GAP_PKT_TYPE_EV3 | GAP_PKT_TYPE_NO_3EV3 | GAP_PKT_TYPE_NO_2EV5 | GAP_PKT_TYPE_NO_3EV5)
#define SCO_PKT_TYPES_EV3_2EV3      (GAP_PKT_TYPE_EV3 | GAP_PKT_TYPE_NO_3EV3 | GAP_PKT_TYPE_NO_2EV5 | GAP_PKT_TYPE_NO_3EV5)

void bt_hfp_unsniff(T_BT_LINK *p_link)
{
    if ((p_link->acl_link_in_sniffmode_fg == 1) && (p_link->sco_handle == 0))
    {
        legacy_exit_sniff_mode(p_link->bd_addr);
    }
}

void bt_hfp_cback(uint8_t *bd_addr, T_HFP_MSG msg_type, void *msg_buf)
{
    T_BT_LINK *p_link;

    p_link = bt_find_link_by_addr(bd_addr);

    APP_PRINT_TRACE3("bt_hfp_cback: bd_addr %s, link %p, msg_type 0x%02x",
                     TRACE_BDADDR(bd_addr), p_link, msg_type);

    switch (msg_type)
    {
    case HFP_MSG_RFC_CONN_IND:
        if (p_link != NULL)
        {
            hfp_conn_cfm(bd_addr, true);
        }
        else
        {
            hfp_conn_cfm(bd_addr, false);
        }
        break;

    case HFP_MSG_RFC_CONN:
        if ((hfp_get_hf_capability()& HF_CAPABILITY_CALL_WAITING_AND_3WAY) == 0)
        {
            char buf[15];
            sprintf(buf, "AT+CCWA=0");
            app_send_at_cmd(bd_addr, buf, HFP_AT_CMD_APP_LANCHED, HFP_API_ID_3WAY_CALL_CONTROL_1);
        }

        if (p_link != NULL)
        {
            T_HFP_MSG_RFC_CONN *p_msg = (T_HFP_MSG_RFC_CONN *)msg_buf;

            p_link->hfp_data.hsp_avtive_fg = p_msg->hsp;
            p_link->hfp_data.hfp_state = HF_STATE_RFCOMM_CONNECTED;
            p_link->hfp_data.hfp_active_codec_type = CODEC_TYPE_CVSD;
        }
        else
        {
            hfp_disconn_req(bd_addr);
        }
        break;

    case HFP_MSG_CONN:
        if (p_link != NULL)
        {
            if (p_link->hfp_data.hsp_avtive_fg)
            {
                p_link->hfp_data.hfp_state = HS_STATE_CONNECTED;
            }
            else
            {
                p_link->hfp_data.hfp_state = HF_STATE_CONNECTED;
            }

            p_link->connected_profile |= HFHS_PROFILE_MASK;

            if (p_link->hfp_data.hfp_state == HF_STATE_CONNECTED)
            {
                hfp_send_clip(bd_addr);
                hfp_send_nrec_disable(bd_addr);
                hfp_send_clcc(bd_addr);
            }

            hfp_inform_ag_speaker_gain(bd_addr, AT_MAX_VGS_LEVEL);
            hfp_inform_ag_microphone_gain(bd_addr, AT_MAX_VGM_LEVEL);
        }
        break;

    case HFP_MSG_DISCONN:
        if (p_link != NULL)
        {
            if (p_link->sco_handle)
            {
                legacy_send_sco_disconn_req(p_link->sco_handle);
            }

            memset(&p_link->hfp_data, 0, sizeof(T_HFP_LINK_DATA));

            p_link->connected_profile &= ~HFHS_PROFILE_MASK;
        }
        break;

    case HFP_MSG_CMD_ACK_OK:
        if (p_link != NULL)
        {
            T_HFP_MSG_ACK_OK *p_msg = (T_HFP_MSG_ACK_OK *)msg_buf;

            APP_PRINT_TRACE2("bt_hfp_cback: rcv ok for cmd %d from addr %s", p_msg->last_api_id,
                             TRACE_BDADDR(bd_addr));

            switch (p_msg->last_api_id)
            {
            case HFP_API_ID_XEVENT_BATT:
                p_link->hfp_data.bat_report_type = BAT_REPORTING_TYPE_ANDROID;
                break;

            default:
                break;
            }
        }
        break;

    case HFP_MSG_CMD_ACK_ERROR:
    case HFP_MSG_CME_ERROR:
        if (p_link != NULL)
        {
            T_HFP_MSG_CME_ERROR *p_msg = (T_HFP_MSG_CME_ERROR *)msg_buf;

            APP_PRINT_TRACE2("bt_hfp_cback: rcv error for cmd %d from addr %s", p_msg->last_api_id,
                             TRACE_BDADDR(bd_addr));
        }
        break;

    case HFP_MSG_VOICE_RECOGNITION_STATUS:
        if (p_link != NULL)
        {
            T_HFP_MSG_VOICE_RECOGNITION_STATUS *p_msg = (T_HFP_MSG_VOICE_RECOGNITION_STATUS *)msg_buf;

            APP_PRINT_TRACE2("bt_hfp_cback: rcv voice recognition status %d from addr %s",
                             p_msg->status, TRACE_BDADDR(p_msg->bd_addr));
        }
        break;

    case HFP_MSG_RING:
        if (p_link != NULL)
        {
            bt_hfp_unsniff(p_link);

            APP_PRINT_TRACE1("bt_hfp_cback: rcv RING from addr %s", TRACE_BDADDR(bd_addr));
        }
        break;

    case HFP_MSG_CLIP:
        if (p_link != NULL)
        {
            T_HFP_MSG_CLIP *p_msg = (T_HFP_MSG_CLIP *)msg_buf;

            APP_PRINT_TRACE1("bt_hfp_cback: rcv CLIP with num %b",
                             TRACE_BINARY(strlen(p_msg->num_string), (uint8_t *)p_msg->num_string));
        }
        break;

    case HFP_MSG_SET_MICROPHONE_GAIN:
        if (p_link != NULL)
        {
            T_HFP_MSG_SET_MICROPHONE_VOLUME *p_msg = (T_HFP_MSG_SET_MICROPHONE_VOLUME *)msg_buf;

            APP_PRINT_TRACE1("bt_hfp_cback: rcv set mic gain value %d", p_msg->volume);
        }
        break;

    case HFP_MSG_SET_SPEAKER_GAIN:
        if (p_link != NULL)
        {
            T_HFP_MSG_SET_SPEAKER_VOLUME *p_msg = (T_HFP_MSG_SET_SPEAKER_VOLUME *)msg_buf;

            APP_PRINT_TRACE1("bt_hfp_cback: rcv set speaker gain value %d", p_msg->volume);
        }
        break;

    case HFP_MSG_AG_INDICATOR_EVENT:
        if (p_link != NULL)
        {
            T_HFP_MSG_AG_INDICATOR_EVENT *p_msg = (T_HFP_MSG_AG_INDICATOR_EVENT *)msg_buf;

            APP_PRINT_TRACE1("bt_hfp_cback: rcv indicator id %d", p_msg->event_type);
        }
        break;

    case HFP_MSG_SET_CODEC_TYPE:
        if (p_link != NULL)
        {
            T_HFP_MSG_SET_CODEC_TYPE *t = msg_buf;

            p_link->hfp_data.hfp_active_codec_type = t->codec_type;
        }
        break;

    case HFP_MSG_AG_INBAND_RINGTONE_SETTING:
        if (p_link != NULL)
        {
            T_HFP_MSG_AG_INBAND_RINGTONE_SET *p_msg = (T_HFP_MSG_AG_INBAND_RINGTONE_SET *)msg_buf;

            if (p_msg->ag_support)
            {
                p_link->hfp_data.function_enable_flag |= HFP_INBAND_RINGTONE_ENABLE;
            }
            else
            {
                p_link->hfp_data.function_enable_flag &= ~HFP_INBAND_RINGTONE_ENABLE;
            }
        }
        break;

    case HFP_MSG_AG_BIND:
        if (p_link != NULL)
        {
            T_HFP_MSG_BIND_STATUS *p_msg = (T_HFP_MSG_BIND_STATUS *)msg_buf;
            p_link->hfp_data.bat_report_type = BAT_REPORTING_TYPE_BIEV;

            if (p_msg->batt_ind_enable)
            {
                p_link->hfp_data.function_enable_flag |= HFP_BATTERY_BIEV_ENABLE;
            }
            else
            {
                p_link->hfp_data.function_enable_flag &= ~HFP_BATTERY_BIEV_ENABLE;
            }
        }
        break;

    case HFP_MESSAGE_WAIT_ACK_TIMEOUT:
        if (p_link != NULL)
        {
            T_HFP_MSG_ACK_OK *p_msg = (T_HFP_MSG_ACK_OK *)msg_buf;

            if (p_msg->last_api_id == HFP_API_ID_XAPL)
            {
                p_link->hfp_data.bat_report_type = BAT_REPORTING_TYPE_APPLE;
            }
        }
        break;

    default:
        break;
    }
}

bool bt_hfp_init(void)
{
    return hfp_init(bt_hfp_cback, 0x03BF, 0x03);
}

void bt_hfp_handle_sco_conn_ind(uint8_t *bd_addr, uint8_t is_esco)
{
    T_BT_LINK *p_link;

    p_link = bt_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        APP_PRINT_ERROR1("bt_hfp_handle_sco_conn_ind: fail to find link for addr %s",
                         TRACE_BDADDR(bd_addr));
        return;
    }

    if (p_link->hfp_data.hfp_active_codec_type == CODEC_TYPE_MSBC)
    {
        //eSCO parameter set to "T2"
        legacy_send_sco_conn_cfm(p_link->bd_addr, 8000, 8000, 13, 0x0363, 2,
                                 SCO_PKT_TYPES_HV3_EV3_2EV3, GAP_CFM_CAUSE_ACCEPT);
    }
    else
    {
        if (is_esco == 0)
        {
            //SCO link
            legacy_send_sco_conn_cfm(p_link->bd_addr, 8000, 8000, 7, 0x0360, 0,
                                     SCO_PKT_TYPES_HV3_EV3_2EV3, GAP_CFM_CAUSE_ACCEPT);
        }
        else
        {
            //eSCO parameter set to "S2"
            legacy_send_sco_conn_cfm(p_link->bd_addr, 8000, 8000, 12, 0x0360, 1,
                                     SCO_PKT_TYPES_HV3_EV3_2EV3, GAP_CFM_CAUSE_ACCEPT);
        }
    }
}
