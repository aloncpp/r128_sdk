/*
* Copyright (C) 2018 Realtek Semiconductor Corporation. All Rights Reserved.
*/

#include <string.h>

#include "trace_app.h"

#include "bt_a2dp.h"
#include "bt_link.h"
#include "gap_storage_legacy.h"
#include "bt_gap.h"

bool bt_a2dp_connect_req(uint8_t *bd_addr, uint16_t avdtp_ver)
{
    T_BT_LINK *p_link;

    if (bd_addr == NULL)
    {
        APP_PRINT_ERROR1("bt_a2dp_connect_req: can't alloc link for %s",
                         TRACE_BDADDR(bd_addr));
        return false;
    }

    p_link = bt_find_link_by_addr(bd_addr);

    if (p_link == NULL)
    {
        p_link = bt_alloc_link(bd_addr);
    }

    if (p_link != NULL)
    {
        if ((p_link->connected_profile & A2DP_PROFILE_MASK) == 0)
        {
            if (a2dp_conn_signal_req(bd_addr, avdtp_ver) == true)
            {
                csm_a2dp_state_cb(CSM_A2DP_CONNECTING, bd_addr);
                return true;
            }
        }
    }

    return false;
}

bool bt_a2dp_disconnect_req(uint8_t *bd_addr)
{
    T_BT_LINK *p_link = NULL;

    if (bd_addr == NULL)
    {
        APP_PRINT_ERROR0("bt_a2dp_disconnect_req: address can't be NULL!");
        return false;
    }

    p_link = bt_find_link_by_addr(bd_addr);

    if (p_link != NULL)
    {
        APP_PRINT_INFO2("bt_a2dp_disconnect_req: acl = %d, profile = %d",
                        p_link->acl_link_state, p_link->connected_profile);
        if (p_link->acl_link_state == LINK_STATE_CONNECTED)
        {
            if (p_link->connected_profile & A2DP_PROFILE_MASK)
            {
                return a2dp_disconn_req(bd_addr);
            }
        }
    }
    return false;
}

static bool bt_a2dp_send_cmd(uint8_t *bd_addr, uint8_t cmd)
{
    T_BT_LINK *p_link = NULL;

    if (bd_addr == NULL)
    {
        APP_PRINT_ERROR0("bt_a2dp_send_msg: address can't be NULL!");
        return false;
    }

    if ((cmd < 0x01) || (cmd > 0x0d))
    {
        APP_PRINT_ERROR1("bt_a2dp_send_msg: unsupported cmd %02x", cmd);
        return false;
    }

    p_link = bt_find_link_by_addr(bd_addr);

    if (p_link != NULL)
    {
        if (p_link->connected_profile & A2DP_PROFILE_MASK)
        {
            return a2dp_send_cmd(bd_addr, cmd);
        }
    }
    return false;
}

bool bt_a2dp_stream_start_request(uint8_t *bd_addr)
{
    APP_PRINT_INFO0("bt_a2dp_stream_start_request");
    return bt_a2dp_send_cmd(bd_addr, 0x07);
}

bool bt_a2dp_stream_suspend_request(uint8_t *bd_addr)
{
    APP_PRINT_INFO0("bt_a2dp_stream_suspend_request");
    return bt_a2dp_send_cmd(bd_addr, 0x09);
}

bool bt_a2dp_get_paired_dev_list(CSM_A2DP_DEV_INFO_LIST *info)
{
    uint8_t paired_num;
    uint8_t a2dp_num = 0;
    uint8_t priority;
    //uint8_t index;
    uint8_t bond_flag;

    uint8_t remote_bd[6];

    char bdAddr[CSM_BDADDR_MAX_LEN];
    char name[CSM_NAME_MAX_LEN];

    CSM_BT_DEV_INFO *dev_list;

    APP_PRINT_INFO0("bt_a2dp_get_paired_dev_list");

    if (info == NULL)
    {
        APP_PRINT_ERROR0("bt_a2dp_get_paired_dev_list: info is NULL");
        return false;
    }

    dev_list = info->device_list;

    paired_num = legacy_get_bond_num();

    if (paired_num > 0)
    {
        for (priority = 1;
             (priority <= paired_num) && (a2dp_num < CSM_MAX_BT_DEV_NUM);
             priority++)
        {
            //get address based on priority
            if (legacy_get_addr_by_priority(priority, remote_bd))
            {
                bond_flag = legacy_get_bond_flag(remote_bd);

                if (bond_flag & BOND_FLAG_A2DP)     //a2dp device
                {
                    mac_bin_to_str(bdAddr, remote_bd);
                    APP_PRINT_INFO3("bt_a2dp_get_paired_dev_list: priority = %d, address = %s, %s",
                                    priority, TRACE_BDADDR(remote_bd), TRACE_STRING(bdAddr));
                    memcpy(dev_list->bdAddr, bdAddr, CSM_BDADDR_MAX_LEN);
                    APP_PRINT_INFO1("bt_a2dp_get_paired_dev_list: address = %s,", TRACE_STRING(dev_list->bdAddr));

                    //get name from bt_gap
                    if (bt_gap_read_remote_name(remote_bd, name, CSM_NAME_MAX_LEN))
                    {
                        memset(dev_list->name, 0, CSM_NAME_MAX_LEN);
                        strncpy(dev_list->name, name, strlen(name));
                    }
                    else
                    {
                        dev_list->name[0] = '\0';
                    }


                    a2dp_num++;
                    dev_list++;
                    //device name: to be done
                }
            }
        }
    }

    //info->head_idx = 0;
    info->dev_num = a2dp_num;

    return true;
}

static void a2dp_set_bond_flag(uint8_t *bd_addr, bool on_off)
{
    uint32_t bond_flag;
    //bool ret = false;

    bond_flag = legacy_get_bond_flag(bd_addr);

    if (on_off)
    {
        bond_flag |= BOND_FLAG_A2DP;
    }
    else
    {
        bond_flag &= ~BOND_FLAG_A2DP;
    }

    legacy_set_bond_flag(bd_addr, bond_flag);
}

void bt_a2dp_cback(uint8_t *bd_addr, T_A2DP_MSG msg_type, void *msg_buf)
{
    T_BT_LINK *p_link = NULL;

    p_link = bt_find_link_by_addr(bd_addr);

    if (msg_type != A2DP_MSG_STREAM_IND)
    {
        APP_PRINT_TRACE2("bt_a2dp_cback: bd_addr %s, msg 0x%02x",
                         TRACE_BDADDR(bd_addr), msg_type);
    }

    switch (msg_type)
    {
    case A2DP_MSG_CONN_IND:
        if (p_link != NULL)
        {
            a2dp_signal_conn_cfm(bd_addr, true);
        }
        else
        {
            a2dp_signal_conn_cfm(bd_addr, false);
        }
        break;

    case A2DP_MSG_CONN_CMPL:
        if (p_link != NULL)
        {
            p_link->connected_profile |= A2DP_PROFILE_MASK;
            //p_link->acl_link_state = LINK_STATE_CONNECTED;
            a2dp_set_bond_flag(bd_addr, true);
            csm_a2dp_state_cb(CSM_A2DP_CONNECTED, bd_addr);
            APP_PRINT_INFO2("bt_a2dp_cback: A2DP_MSG_CONN_CMPL, acl = %d, profile = %d",
                            p_link->acl_link_state, p_link->connected_profile);
        }
        else
        {
            a2dp_disconn_req(bd_addr);
        }
        break;

    case A2DP_MSG_DISCONN:
        if (p_link != NULL)
        {
            p_link->connected_profile &= ~A2DP_PROFILE_MASK;
            //p_link->acl_link_state = LINK_STATE_STANDBY;
            memset(&(p_link->a2dp_data), 0, sizeof(T_A2DP_LINK_DATA));
            csm_a2dp_state_cb(CSM_A2DP_DISCONNECTED, bd_addr);
        }
        break;

    case A2DP_MSG_SET_CFG:
    case A2DP_MSG_RE_CFG:
        if (p_link != NULL)
        {
            p_link->a2dp_data.configured_codec_type = ((T_A2DP_CFG *)msg_buf)->codec_type;

            if (((T_A2DP_CFG *)msg_buf)->cp_flag)
            {
                p_link->a2dp_data.a2dp_content_protect = 1;
            }
            else
            {
                p_link->a2dp_data.a2dp_content_protect = 0;
            }

            if (((T_A2DP_CFG *)msg_buf)->delay_report_flag)
            {
                p_link->a2dp_data.a2dp_delay_report = 1;
            }
            else
            {
                p_link->a2dp_data.a2dp_delay_report = 0;
            }

            p_link->a2dp_data.sample_frequency = ((T_A2DP_CFG *)msg_buf)->sample_frequency;
            p_link->a2dp_data.channel_mode = ((T_A2DP_CFG *)msg_buf)->channel_mode;
            p_link->a2dp_data.sbc_encode_hdr = ((T_A2DP_CFG *)msg_buf)->sbc_encode_hdr;
			aw_decode_set_config(p_link->a2dp_data.sample_frequency,
	                p_link->a2dp_data.channel_mode,
	                p_link->a2dp_data.sbc_encode_hdr);

        }
        break;

    case A2DP_MSG_OPEN:
        if (p_link != NULL)
        {
            p_link->a2dp_data.streaming_fg = false;
        }
        break;

    case A2DP_MSG_START:
        if (p_link != NULL)
        {
            p_link->a2dp_data.streaming_fg = true;
            csm_a2dp_stream_cb(CSM_A2DP_STREAM_STATE_PLAYING);
        }
        break;

    case A2DP_MSG_ABORT:
    case A2DP_MSG_STOP:
    case A2DP_MSG_SUSPEND:
        if (p_link != NULL)
        {
            p_link->a2dp_data.streaming_fg = false;
            csm_a2dp_stream_cb(CSM_A2DP_STREAM_STATE_STOP);
        }
        break;

    case A2DP_MSG_STREAM_IND:
        if (p_link != NULL)
        {
            uint8_t *pkt_ptr;
            uint16_t pkt_len;

            pkt_ptr = ((T_A2DP_STREAM_IND *)msg_buf)->pkt_ptr;
            pkt_len = ((T_A2DP_STREAM_IND *)msg_buf)->pkt_len;

            APP_PRINT_TRACE2("bt_a2dp_cback: rcv media data len %d, buf addr %p", pkt_len, pkt_ptr);
			aw_a2dp_stream_push(pkt_ptr,pkt_len);
        }
        break;

    default:
        break;
    }
}

void bt_a2dp_init(void)
{
    a2dp_init(bt_a2dp_cback);

    //a2dp_set_param(A2DP_PARAM_CODEC_TYPE, 0x03);    //sbc,aac
    a2dp_set_param(A2DP_PARAM_CODEC_TYPE, 0x01);    //sbc
    a2dp_set_param(A2DP_PARAM_LATENCY, 180);
    a2dp_set_param(A2DP_PARAM_ROLE, 0);     //sink
}

