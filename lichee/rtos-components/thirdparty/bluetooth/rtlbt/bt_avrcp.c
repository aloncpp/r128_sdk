/*
* Copyright (C) 2018 Realtek Semiconductor Corporation. All Rights Reserved.
*/

#include <string.h>

#include "trace_app.h"

#include "bt_avrcp.h"
#include "bt_link.h"
#include "customer_api.h"
#define DEBUG
#include "app_common.h"


uint8_t avrcp_connected_bd_addr[6];
uint8_t avrcp_connection_status;

uint8_t *bt_avrcp_get_dev_addr()
{
    if (!avrcp_connection_status)
    {
        return NULL;
    }
    else
    {
        return avrcp_connected_bd_addr;
    }
}

bool bt_avrcp_connect_req(uint8_t *bd_addr)
{
    T_BT_LINK *p_link;

    if (bd_addr == NULL)
    {
        APP_PRINT_WARN0("bt_avrcp_connect_req: BT address is null");
        return false;
    }

    APP_PRINT_WARN1("bt_avrcp_connect_req: address is %s",
                    TRACE_BDADDR(bd_addr));

    p_link = bt_find_link_by_addr(bd_addr);

    if (p_link == NULL)
    {
        p_link = bt_alloc_link(bd_addr);
    }

    if (p_link != NULL)
    {
        //not in connected state
        if ((p_link->connected_profile & AVRCP_PROFILE_MASK) == 0)
        {
            return avrcp_conn_req(bd_addr);
        }
    }

    return false;
}

bool bt_avrcp_disconnect_req(uint8_t *bd_addr)
{
    T_BT_LINK *p_link;

    if (bd_addr == NULL)
    {
        APP_PRINT_WARN0("bt_avrcp_disconnect_req: BT address is null");
        return false;
    }

    p_link = bt_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        APP_PRINT_WARN1("bt_avrcp_disconnect_req: no existing link for device %s",
                        TRACE_BDADDR(bd_addr));
        return false;
    }


    APP_PRINT_WARN1("bt_avrcp_disconnect_req: acl_link_state = %d", p_link->acl_link_state);
    if (p_link->acl_link_state == LINK_STATE_CONNECTED)
    {
        APP_PRINT_WARN1("bt_avrcp_disconnect_req: connected_profile = %d", p_link->connected_profile);
        if ((p_link->connected_profile & AVRCP_PROFILE_MASK) != 0)
        {
            return avrcp_disconn_req(bd_addr);
        }
    }

    return false;
}

bool bt_avrcp_notify_volume_change_req(uint8_t *bd_addr, uint8_t vol)
{
    T_BT_LINK *p_link;

    if (bd_addr == NULL)
    {
        APP_PRINT_WARN0("bt_avrcp_notify_volume_change_req: BT address is null");
        return false;
    }

    p_link = bt_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        APP_PRINT_WARN1("bt_avrcp_notify_volume_change_req: no existing link for device %s",
                        TRACE_BDADDR(bd_addr));
        return false;
    }

    if (p_link->connected_profile & AVRCP_PROFILE_MASK)
    {
        return avrcp_notify_volume_change(bd_addr, vol);
    }

    return false;
}

void bt_avrcp_process_cmd(uint8_t *bd_addr)
{
    T_BT_LINK *p_link = NULL;

    p_link = bt_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        return;
    }

    if ((p_link->connected_profile & AVRCP_PROFILE_MASK) && (p_link->avrcp_data.cmd_credit == 1))
    {
        T_AVRCP_CMD cmd;

        //if there is pending cmd action, process it
        if (p_link->avrcp_data.cmd_queue.read_index != p_link->avrcp_data.cmd_queue.write_index)
        {
            cmd = p_link->avrcp_data.cmd_queue.command[p_link->avrcp_data.cmd_queue.read_index];
            if (cmd.func != NULL)
            {
                T_AVRCP_CMD_FUNC func = (T_AVRCP_CMD_FUNC)cmd.func;

                if (func(p_link->bd_addr, cmd.para) == true)
                {
                    p_link->avrcp_data.cmd_credit = 0; //Wait command response
                    p_link->avrcp_data.cmd_queue.read_index++;
                    if (p_link->avrcp_data.cmd_queue.read_index == AVRCP_MAX_QUEUED_CMD)
                    {
                        p_link->avrcp_data.cmd_queue.read_index = 0;
                    }
                }
                else
                {
                    APP_PRINT_TRACE1("bt_avrcp_process_cmd: command failed %x", cmd.para);
                }
            }
        }
    }
}

void bt_avrcp_queue_cmd(uint8_t *bd_addr, T_AVRCP_CMD_FUNC func, uint8_t para)
{
    T_BT_LINK *p_link = NULL;

    p_link = bt_find_link_by_addr(bd_addr);

    if (p_link != NULL)
    {
        //Put command to queue if not full
        if (((p_link->avrcp_data.cmd_queue.write_index + 1) % AVRCP_MAX_QUEUED_CMD) !=
            p_link->avrcp_data.cmd_queue.read_index)
        {
            p_link->avrcp_data.cmd_queue.command[p_link->avrcp_data.cmd_queue.write_index].func = func;
            p_link->avrcp_data.cmd_queue.command[p_link->avrcp_data.cmd_queue.write_index].para = para;

            p_link->avrcp_data.cmd_queue.write_index++;
            if (p_link->avrcp_data.cmd_queue.write_index == AVRCP_MAX_QUEUED_CMD)
            {
                p_link->avrcp_data.cmd_queue.write_index = 0;
            }
        }

        bt_avrcp_process_cmd(p_link->bd_addr);
    }
}

T_PASSTHROUGH_OP_ID avrcp_get_op_id_from_customer_cmd_type(CSM_AVRCP_CMD_TYPE cmd_type)
{

    switch ((int)cmd_type)
    {

    case CSM_AVRCP_CMD_TYPE_PLAY:
        return PASSTHROUGH_ID_PLAY;

    case CSM_AVRCP_CMD_TYPE_PAUSE:
        return PASSTHROUGH_ID_PAUSE;

    case CSM_AVRCP_CMD_TYPE_FWD:
        return PASSTHROUGH_ID_FORWARD;

    case CSM_AVRCP_CMD_TYPE_BWD:
        return PASSTHROUGH_ID_BACKWARD;

    case CSM_AVRCP_CMD_TYPE_FFWD:
        return PASSTHROUGH_ID_FAST_FOR;

    case CSM_AVRCP_CMD_TYPE_RWD:
        return PASSTHROUGH_ID_REWIND;

    case CSM_AVRCP_CMD_TYPE_STOP:
        return PASSTHROUGH_ID_STOP;

    case CSM_AVRCP_CMD_TYPE_VOL_UP:
        return PASSTHROUGH_ID_VOL_UP;

    case CSM_AVRCP_CMD_TYPE_VOL_DOWN:
        return PASSTHROUGH_ID_VOL_DOWN;

    default:
        return PASSTHROUGH_ID_MAX;
    }
}

CSM_AVRCP_CMD_TYPE avrcp_get_cmd_type_from_op_id(T_PASSTHROUGH_OP_ID op_id)
{
    switch ((int)op_id)
    {

    case PASSTHROUGH_ID_PLAY:
        return CSM_AVRCP_CMD_TYPE_PLAY;

    case PASSTHROUGH_ID_PAUSE:
        return CSM_AVRCP_CMD_TYPE_PAUSE;

    case PASSTHROUGH_ID_FORWARD:
        return CSM_AVRCP_CMD_TYPE_FWD;

    case PASSTHROUGH_ID_BACKWARD:
        return CSM_AVRCP_CMD_TYPE_BWD;

    case PASSTHROUGH_ID_FAST_FOR:
        return CSM_AVRCP_CMD_TYPE_FFWD;

    case PASSTHROUGH_ID_REWIND:
        return CSM_AVRCP_CMD_TYPE_RWD;

    case PASSTHROUGH_ID_STOP:
        return CSM_AVRCP_CMD_TYPE_STOP;

    case PASSTHROUGH_ID_VOL_UP:
        return CSM_AVRCP_CMD_TYPE_VOL_UP;

    case PASSTHROUGH_ID_VOL_DOWN:
        return CSM_AVRCP_CMD_TYPE_VOL_DOWN;

    default:
        return CSM_AVRCP_CMD_TYPE_MAX;
    }
}



bool bt_avrcp_cmd_passthrough(uint8_t *bd_addr, uint8_t param)
{
    T_AVRCP_KEY key;
    bool pressed;

    key = (T_AVRCP_KEY)(param & 0x7F);
    pressed = (param & AVRCP_PASS_THROUGH_KEY_RELEASE) ? false : true;

    return avrcp_send_pass_through(bd_addr, key, pressed);
}

bool bt_avrcp_cmd_reg_notif(uint8_t *bd_addr, uint8_t event_id)
{
    return avrcp_register_notification(bd_addr, event_id);
}

void avrcp_app_send_cmd_passthrough(uint8_t *bd_addr, T_AVRCP_KEY key, bool pressed)
{
    uint8_t para;

    para = key;
    if (pressed == false)
    {
        para |= AVRCP_PASS_THROUGH_KEY_RELEASE;
    }

    bt_avrcp_queue_cmd(bd_addr, bt_avrcp_cmd_passthrough, para);
}

void bt_avrcp_cback(uint8_t *bd_addr, T_AVRCP_MSG msg_type, void *msg_buf)
{
    T_BT_LINK *p_link;

    p_link = bt_find_link_by_addr(bd_addr);

    APP_PRINT_TRACE1("bt_avrcp_cback: msg_type 0x%02x", msg_type);

    switch (msg_type)
    {
    case AVRCP_MSG_CONN_IND:
        if (p_link != NULL)
        {
            avrcp_conn_cfm(bd_addr, true);
        }
        else
        {
            avrcp_conn_cfm(bd_addr, false);
        }
        break;

    case AVRCP_MSG_CONN_CMPL:
        if (p_link != NULL)
        {
            p_link->connected_profile |= AVRCP_PROFILE_MASK;
            p_link->avrcp_data.cmd_credit = 1;
            p_link->acl_link_state = LINK_STATE_CONNECTED;

            //save device address for passthrough command
            avrcp_connection_status = true;
            memcpy(avrcp_connected_bd_addr, bd_addr, 6);

            //connected callback
            csm_avrcp_state_cb(CSM_AVRCP_CONNECTED);

            avrcp_get_capability(bd_addr, CAPABILITY_ID_EVENTS_SUPPORTED);
        }
        else
        {
            avrcp_disconn_req(bd_addr);
        }
        break;

    case AVRCP_MSG_DISCONN:
        if (p_link != NULL)
        {
            p_link->connected_profile &= ~AVRCP_PROFILE_MASK;

            p_link->avrcp_data.support_player_status_notify = false;
            p_link->avrcp_data.play_status = AVRCP_PLAYSTATUS_STOPPED;
            p_link->avrcp_data.cmd_queue.write_index = 0;
            p_link->avrcp_data.cmd_queue.read_index = 0;

            memset(avrcp_connected_bd_addr, 0, 6);
            avrcp_connection_status = false;

            //p_link->acl_link_state = LINK_STATE_STANDBY;

            csm_avrcp_state_cb(CSM_AVRCP_DISCONNECTED);
        }
        break;

    case AVRCP_MSG_CMD_VOL_UP:
        if (p_link != NULL)
        {
            APP_PRINT_TRACE1("bt_avrcp_cback: rcv vol up for addr %s", TRACE_BDADDR(bd_addr));
            csm_avrcp_volume_chg_cb(CSM_AVRCP_VOLUME_UP);
        }
        break;

    case AVRCP_MSG_CMD_VOL_DOWN:
        if (p_link != NULL)
        {
            APP_PRINT_TRACE1("bt_avrcp_cback: rcv vol down for addr %s", TRACE_BDADDR(bd_addr));
            csm_avrcp_volume_chg_cb(CSM_AVRCP_VOLUME_DOWN);
        }
        break;

    case AVRCP_MSG_CMD_ABS_VOL:
        if (p_link != NULL)
        {
            uint8_t abs_vol = *(uint8_t *)msg_buf;
            APP_PRINT_TRACE2("bt_avrcp_cback: rcv absult volume %d for addr %s", abs_vol,
                             TRACE_BDADDR(bd_addr));
            csm_avrcp_abs_volume_cb(abs_vol);
        }
        break;

    case AVRCP_MSG_RSP_PASSTHROUGH:
        if (p_link != NULL)
        {
            T_RSP_PASSTHROUGH *rsp = (T_RSP_PASSTHROUGH *)msg_buf;

            if (rsp->state == AVRCP_RSP_STATE_SUCCESS)
            {
                //Handle key press response -> Send key release pass through command
                if (rsp->pressed == false) /* released */
                {
                    if (rsp->key == AVRCP_KEY_PAUSE)
                    {
                        p_link->avrcp_data.play_status = AVRCP_PLAYSTATUS_PAUSED;
                    }
                    else if (rsp->key == AVRCP_KEY_PLAY)
                    {
                        p_link->avrcp_data.play_status = AVRCP_PLAYSTATUS_PLAYING;
                    }
                    else if (rsp->key == AVRCP_KEY_STOP)
                    {
                        p_link->avrcp_data.play_status = AVRCP_PLAYSTATUS_STOPPED;
                    }
                    APP_PRINT_INFO1("bt_avrcp_cback: passthrough callback, op_id = %d", rsp->key);
                    CSM_AVRCP_CMD_TYPE cmd_type = avrcp_get_cmd_type_from_op_id((T_PASSTHROUGH_OP_ID)(rsp->key));

                    if (cmd_type != CSM_AVRCP_CMD_TYPE_MAX)
                    {
                        APP_PRINT_INFO1("bt_avrcp_cback: passthrough callback, cmd_type = %d", cmd_type);
                        csm_avrcp_passthr_cb(cmd_type);
                    }
                }
                else /* pushed */
                {
                    //Send AVRCP_KEY_FAST_FORWARD/AVRCP_KEY_REWIND release when user key release
                    if ((rsp->key != AVRCP_KEY_FAST_FORWARD) && (rsp->key != AVRCP_KEY_REWIND))
                    {
                        bt_avrcp_queue_cmd(p_link->bd_addr, bt_avrcp_cmd_passthrough,
                                           (AVRCP_PASS_THROUGH_KEY_RELEASE | rsp->key));
                    }
                }
            }
        }
        break;

    case AVRCP_MSG_RSP_GET_CPBS:
        if (p_link != NULL)
        {
            uint8_t cpbs_count;
            uint8_t *cpbs_buf;
            T_RSP_CPBS *tmp;
            tmp = msg_buf;
            cpbs_count = tmp->capability_count;
            cpbs_buf = tmp->p_buf;
            while (cpbs_count != 0)
            {
                if (*cpbs_buf == EVENT_PLAYBACK_STATUS_CHANGED)
                {
                    bt_avrcp_queue_cmd(p_link->bd_addr, bt_avrcp_cmd_reg_notif, EVENT_PLAYBACK_STATUS_CHANGED);
                }
                else if (*cpbs_buf == EVENT_TRACK_CHANGED)
                {
                    bt_avrcp_queue_cmd(p_link->bd_addr, bt_avrcp_cmd_reg_notif, EVENT_TRACK_CHANGED);
                }
                cpbs_count -= 1;
                cpbs_buf += 1;
            }
        }
        break;

    case AVRCP_MSG_RSP_REG_NOTIFICATION:
        if (p_link != NULL)
        {
            T_RSP_REG_NOTIFICATION *rsp = (T_RSP_REG_NOTIFICATION *)msg_buf;

            if (rsp->state == AVRCP_RSP_STATE_SUCCESS)
            {
                switch (rsp->event_id)
                {
                case EVENT_PLAYBACK_STATUS_CHANGED:
                    {
                        p_link->avrcp_data.play_status = rsp->u.play_status;
                        if (p_link->avrcp_data.play_status == AVRCP_PLAYSTATUS_STOPPED)
                        {
                            p_link->avrcp_data.play_status = AVRCP_PLAYSTATUS_PAUSED;
                        }

                        if (p_link->avrcp_data.support_player_status_notify == false)
                        {
                            p_link->avrcp_data.support_player_status_notify = true;
                        }

                    }
                    break;

                default:
                    break;
                }
            }
        }
        break;

    case AVRCP_MSG_RSP_GET_PLAYSTATUS:
        if (p_link != NULL)
        {
            T_RSP_GET_PLAY_STATUS *rsp = (T_RSP_GET_PLAY_STATUS *)msg_buf;

            if (rsp->state == AVRCP_RSP_STATE_SUCCESS)
            {
                p_link->avrcp_data.play_status = rsp->play_status;
            }
        }
        break;

    case AVRCP_MSG_NOTIF_CHANGED:
        if (p_link != NULL)
        {
            T_NOTIF_CHANGED *rsp = (T_NOTIF_CHANGED *)msg_buf;

            switch (rsp->event_id)
            {
            case EVENT_PLAYBACK_STATUS_CHANGED:
                {
                    bt_avrcp_queue_cmd(p_link->bd_addr, bt_avrcp_cmd_reg_notif, EVENT_PLAYBACK_STATUS_CHANGED);
                    p_link->avrcp_data.play_status = rsp->u.play_status;
                    if (p_link->avrcp_data.play_status == AVRCP_PLAYSTATUS_STOPPED)
                    {
                        p_link->avrcp_data.play_status = AVRCP_PLAYSTATUS_PAUSED;
                    }

                    if ((p_link->avrcp_data.play_status == AVRCP_PLAYSTATUS_PAUSED) ||
                        (p_link->avrcp_data.play_status == AVRCP_PLAYSTATUS_PLAYING))
                    {
                        csm_avrcp_play_state_cb((CSM_AVRCP_PLAY_STATUS)p_link->avrcp_data.play_status);
                    }
                }
                break;

            case EVENT_TRACK_CHANGED:
                bt_avrcp_queue_cmd(p_link->bd_addr, bt_avrcp_cmd_reg_notif, EVENT_TRACK_CHANGED);
                break;

            default:
                break;
            }
        }
        break;

    case AVRCP_MSG_RCV_RSP:
        if (p_link != NULL)
        {
            p_link->avrcp_data.cmd_credit = 1;
            bt_avrcp_process_cmd(p_link->bd_addr);
        }
        break;

    case AVRCP_MSG_ERR:
        if (p_link != NULL)
        {
            T_AVRCP_MSG_ERR err = *(T_AVRCP_MSG_ERR *)msg_buf;

            if (err == AVRCP_WAIT_RSP_TO)
            {
                p_link->avrcp_data.cmd_credit = 1;
                bt_avrcp_process_cmd(p_link->bd_addr);
            }
        }
        break;

    default:
        break;
    }
}

bool bt_avrcp_init(void)
{
    return avrcp_init(bt_avrcp_cback, COMPANY_INVALID, NULL, NULL);
}

