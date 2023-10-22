// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "common/bt_target.h"
#include <string.h>
#include "xr_err.h"
#include "xr_avrc_api.h"
#include "xr_bt_main.h"
#include "btc/btc_manage.h"
#include "btc_avrc.h"
#include <stdio.h>

#if BTC_AV_INCLUDED

xr_err_t xr_avrc_ct_register_callback(xr_avrc_ct_cb_t callback)
{
    if (xr_bluedroid_get_status() != XR_BLUEDROID_STATUS_ENABLED) {
        return XR_ERR_INVALID_STATE;
    }

    if (callback == NULL) {
        return XR_ERR_INVALID_ARG;
    }

    btc_profile_cb_set(BTC_PID_AVRC_CT, callback);
    return XR_OK;
}

xr_err_t xr_avrc_ct_init(void)
{
    if (xr_bluedroid_get_status() != XR_BLUEDROID_STATUS_ENABLED) {
        return XR_ERR_INVALID_STATE;
    }

    btc_msg_t msg;

    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_AVRC_CT;
    msg.act = BTC_AVRC_CT_API_INIT_EVT;

    /* Switch to BTC context */
    bt_status_t stat = btc_transfer_context(&msg, NULL, 0, NULL);
    return (stat == BT_STATUS_SUCCESS) ? XR_OK : XR_FAIL;
}

xr_err_t xr_avrc_ct_deinit(void)
{
    if (xr_bluedroid_get_status() != XR_BLUEDROID_STATUS_ENABLED) {
        return XR_ERR_INVALID_STATE;
    }

    btc_msg_t msg;

    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_AVRC_CT;
    msg.act = BTC_AVRC_CT_API_DEINIT_EVT;

    /* Switch to BTC context */
    bt_status_t stat = btc_transfer_context(&msg, NULL, 0, NULL);
    return (stat == BT_STATUS_SUCCESS) ? XR_OK : XR_FAIL;
}

xr_err_t xr_avrc_ct_send_set_player_value_cmd(uint8_t tl, uint8_t attr_id, uint8_t value_id)
{
    if (xr_bluedroid_get_status() != XR_BLUEDROID_STATUS_ENABLED) {
        return XR_ERR_INVALID_STATE;
    }

    if (tl > XR_AVRC_TRANS_LABEL_MAX || attr_id > XR_AVRC_PS_MAX_ATTR - 1) {
        return XR_ERR_INVALID_ARG;
    }

    btc_msg_t msg;
    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_AVRC_CT;
    msg.act = BTC_AVRC_CTRL_API_SND_SET_PLAYER_SETTING_EVT;

    btc_avrc_args_t arg;
    memset(&arg, 0, sizeof(btc_avrc_args_t));

    arg.ps_cmd.tl = tl;
    arg.ps_cmd.attr_id = attr_id;
    arg.ps_cmd.value_id = value_id;

    /* Switch to BTC context */
    bt_status_t stat = btc_transfer_context(&msg, &arg, sizeof(btc_avrc_args_t), NULL);
    return (stat == BT_STATUS_SUCCESS) ? XR_OK : XR_FAIL;
}

xr_err_t xr_avrc_ct_send_get_rn_capabilities_cmd(uint8_t tl)
{
    if (xr_bluedroid_get_status() != XR_BLUEDROID_STATUS_ENABLED) {
        return XR_ERR_INVALID_STATE;
    }

    if (tl > XR_AVRC_TRANS_LABEL_MAX) {
        return XR_ERR_INVALID_ARG;
    }

    btc_msg_t msg;
    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_AVRC_CT;
    msg.act = BTC_AVRC_STATUS_API_SND_GET_RN_CAPS_EVT;

    btc_avrc_args_t arg;
    memset(&arg, 0, sizeof(btc_avrc_args_t));

    arg.get_caps_cmd.tl = tl;

    /* Switch to BTC context */
    bt_status_t stat = btc_transfer_context(&msg, &arg, sizeof(btc_avrc_args_t), NULL);
    return (stat == BT_STATUS_SUCCESS) ? XR_OK : XR_FAIL;
}

xr_err_t xr_avrc_ct_send_register_notification_cmd(uint8_t tl, uint8_t event_id, uint32_t event_parameter)
{
    if (xr_bluedroid_get_status() != XR_BLUEDROID_STATUS_ENABLED) {
        return XR_ERR_INVALID_STATE;
    }

    if (tl > XR_AVRC_TRANS_LABEL_MAX || event_id > XR_AVRC_RN_MAX_EVT - 1) {
        return XR_ERR_INVALID_ARG;
    }

    if (!btc_avrc_ct_rn_evt_supported(event_id)) {
        return XR_ERR_NOT_SUPPORTED;
    }

    btc_msg_t msg;
    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_AVRC_CT;
    msg.act = BTC_AVRC_NOTIFY_API_SND_REG_NOTIFY_EVT;

    btc_avrc_args_t arg;
    memset(&arg, 0, sizeof(btc_avrc_args_t));

    arg.rn_cmd.tl = tl;
    arg.rn_cmd.event_id = event_id;
    arg.rn_cmd.event_parameter = event_parameter;

    /* Switch to BTC context */
    bt_status_t stat = btc_transfer_context(&msg, &arg, sizeof(btc_avrc_args_t), NULL);
    return (stat == BT_STATUS_SUCCESS) ? XR_OK : XR_FAIL;
}

xr_err_t xr_avrc_ct_send_set_absolute_volume_cmd(uint8_t tl, uint8_t volume)
{
    if (xr_bluedroid_get_status() != XR_BLUEDROID_STATUS_ENABLED) {
        return XR_ERR_INVALID_STATE;
    }

    if (tl > XR_AVRC_TRANS_LABEL_MAX) {
        return XR_ERR_INVALID_ARG;
    }

    if (volume > BTC_AVRC_MAX_VOLUME) {
        return XR_ERR_INVALID_ARG;
    }

    if (!btc_avrc_ct_rn_evt_supported(XR_AVRC_RN_VOLUME_CHANGE)) {
        return XR_ERR_NOT_SUPPORTED;
    }

    btc_msg_t msg;
    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_AVRC_CT;
    msg.act = BTC_AVRC_CTRL_API_SND_SET_ABSOLUTE_VOLUME_EVT;

    btc_avrc_args_t arg;
    memset(&arg, 0, sizeof(btc_avrc_args_t));

    arg.set_abs_vol_cmd.tl = tl;
    arg.set_abs_vol_cmd.volume = volume;

    /* Switch to BTC context */
    bt_status_t stat = btc_transfer_context(&msg, &arg, sizeof(btc_avrc_args_t), NULL);
    return (stat == BT_STATUS_SUCCESS) ? XR_OK : XR_FAIL;
}

xr_err_t xr_avrc_ct_send_metadata_cmd(uint8_t tl, uint8_t attr_mask)
{
    if (xr_bluedroid_get_status() != XR_BLUEDROID_STATUS_ENABLED) {
        return XR_ERR_INVALID_STATE;
    }

    if (tl > XR_AVRC_TRANS_LABEL_MAX) {
        return XR_ERR_INVALID_ARG;
    }

    btc_msg_t msg;
    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_AVRC_CT;
    msg.act = BTC_AVRC_STATUS_API_SND_META_EVT;

    btc_avrc_args_t arg;
    memset(&arg, 0, sizeof(btc_avrc_args_t));

    arg.md_cmd.tl = tl;
    arg.md_cmd.attr_mask = attr_mask;

    /* Switch to BTC context */
    bt_status_t stat = btc_transfer_context(&msg, &arg, sizeof(btc_avrc_args_t), NULL);
    return (stat == BT_STATUS_SUCCESS) ? XR_OK : XR_FAIL;
}

xr_err_t xr_avrc_ct_send_passthrough_cmd(uint8_t tl, uint8_t key_code, uint8_t key_state)
{
    if (xr_bluedroid_get_status() != XR_BLUEDROID_STATUS_ENABLED) {
        return XR_ERR_INVALID_STATE;
    }

    if (tl > XR_AVRC_TRANS_LABEL_MAX || key_state > XR_AVRC_PT_CMD_STATE_RELEASED) {
        return XR_ERR_INVALID_ARG;
    }

    btc_msg_t msg;
    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_AVRC_CT;
    msg.act = BTC_AVRC_CTRL_API_SND_PTCMD_EVT;

    btc_avrc_args_t arg;
    memset(&arg, 0, sizeof(btc_avrc_args_t));

    arg.pt_cmd.tl = tl;
    arg.pt_cmd.key_code = key_code;
    arg.pt_cmd.key_state = key_state;

    /* Switch to BTC context */
    bt_status_t stat = btc_transfer_context(&msg, &arg, sizeof(btc_avrc_args_t), NULL);
    return (stat == BT_STATUS_SUCCESS) ? XR_OK : XR_FAIL;
}

/*********************************************************************************************/
/**                  following is the API of AVRCP target role                              **/
/*********************************************************************************************/

xr_err_t xr_avrc_tg_register_callback(xr_avrc_tg_cb_t callback)
{
    if (xr_bluedroid_get_status() != XR_BLUEDROID_STATUS_ENABLED) {
        return XR_ERR_INVALID_STATE;
    }

    if (callback == NULL) {
        return XR_ERR_INVALID_ARG;
    }

    btc_profile_cb_set(BTC_PID_AVRC_TG, callback);
    return XR_OK;
}

xr_err_t xr_avrc_tg_init(void)
{
    if (xr_bluedroid_get_status() != XR_BLUEDROID_STATUS_ENABLED) {
        return XR_ERR_INVALID_STATE;
    }

    btc_msg_t msg;

    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_AVRC_TG;
    msg.act = BTC_AVRC_TG_API_INIT_EVT;

    /* Switch to BTC context */
    bt_status_t stat = btc_transfer_context(&msg, NULL, 0, NULL);
    return (stat == BT_STATUS_SUCCESS) ? XR_OK : XR_FAIL;
}

xr_err_t xr_avrc_tg_deinit(void)
{
    if (xr_bluedroid_get_status() != XR_BLUEDROID_STATUS_ENABLED) {
        return XR_ERR_INVALID_STATE;
    }

    btc_msg_t msg;

    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_AVRC_TG;
    msg.act = BTC_AVRC_TG_API_DEINIT_EVT;

    /* Switch to BTC context */
    bt_status_t stat = btc_transfer_context(&msg, NULL, 0, NULL);
    return (stat == BT_STATUS_SUCCESS) ? XR_OK : XR_FAIL;
}

bool xr_avrc_psth_bit_mask_operation(xr_avrc_bit_mask_op_t op, xr_avrc_psth_bit_mask_t *psth,
                                      xr_avrc_pt_cmd_t cmd)
{
    if (!psth ||
        cmd > XR_AVRC_PT_CMD_VENDOR) {
        return false;
    }

    uint16_t *p = &psth->bits[(uint8_t)cmd >> 4];
    uint16_t mask = (uint16_t)1 << ((uint8_t)cmd & 0x0F);
    switch (op) {
    case XR_AVRC_BIT_MASK_OP_SET:
        *p |= mask;
        break;
    case XR_AVRC_BIT_MASK_OP_CLEAR:
        *p &= ~mask;
        break;
    case XR_AVRC_BIT_MASK_OP_TEST:
        return (*p & mask);
    default:
        return false;
    }

    return true;
}

xr_err_t xr_avrc_tg_get_psth_cmd_filter(xr_avrc_psth_filter_t filter, xr_avrc_psth_bit_mask_t *cmd_set)
{
    if ((xr_bluedroid_get_status() != XR_BLUEDROID_STATUS_ENABLED) ||
        (! btc_avrc_tg_init_p())) {
        return XR_ERR_INVALID_STATE;
    }
    if (filter >= XR_AVRC_PSTH_FILTER_SUPPORT_MAX ||
        cmd_set == NULL) {
        return XR_ERR_INVALID_ARG;
    }

    if (filter == XR_AVRC_PSTH_FILTER_ALLOWED_CMD) {
        const uint16_t *allowed_cmd_set = btc_avrc_tg_get_allowed_command();
        memcpy(cmd_set, allowed_cmd_set, sizeof(xr_avrc_psth_bit_mask_t));
    } else if (filter == XR_AVRC_PSTH_FILTER_SUPPORTED_CMD) {
        const uint16_t *supported_cmd_set = btc_avrc_tg_get_supported_command();
        memcpy(cmd_set, supported_cmd_set, sizeof(xr_avrc_psth_bit_mask_t));
    } else {
    }

    return XR_OK;
}

xr_err_t xr_avrc_tg_set_psth_cmd_filter(xr_avrc_psth_filter_t filter, const xr_avrc_psth_bit_mask_t *cmd_set)
{
    if (xr_bluedroid_get_status() != XR_BLUEDROID_STATUS_ENABLED) {
        return XR_ERR_INVALID_STATE;
    }
    if (filter >= XR_AVRC_PSTH_FILTER_SUPPORT_MAX ||
        cmd_set == NULL) {
        return XR_ERR_INVALID_ARG;
    }
    if (filter == XR_AVRC_PSTH_FILTER_ALLOWED_CMD) {
        return XR_ERR_NOT_SUPPORTED;
    }
    if (filter == XR_AVRC_PSTH_FILTER_SUPPORTED_CMD) {
        bool allowed = btc_avrc_tg_check_supported_command(cmd_set->bits);
        if (!allowed) {
            return XR_ERR_NOT_SUPPORTED;
        }
        btc_msg_t msg;
        msg.sig = BTC_SIG_API_CALL;
        msg.pid = BTC_PID_AVRC_TG;
        msg.act = BTC_AVRC_TG_API_SET_PSTH_SUPPORTED_CMD_EVT;

        btc_avrc_tg_args_t arg;
        memset(&arg, 0, sizeof(btc_avrc_tg_args_t));
        arg.set_psth_cmd = (uint16_t *)cmd_set->bits;

        /* Switch to BTC context */
        bt_status_t stat = btc_transfer_context(&msg, &arg, sizeof(btc_avrc_tg_args_t),
                                                btc_avrc_tg_arg_deep_copy);
        return (stat == BT_STATUS_SUCCESS) ? XR_OK : XR_FAIL;
    } else {
        return XR_FAIL;
    }
}

xr_err_t xr_avrc_tg_get_rn_evt_cap(xr_avrc_rn_evt_cap_t cap, xr_avrc_rn_evt_cap_mask_t *evt_set)
{
    if ((xr_bluedroid_get_status() != XR_BLUEDROID_STATUS_ENABLED) ||
        (! btc_avrc_tg_init_p())) {
        return XR_ERR_INVALID_STATE;
    }
    if (cap >= XR_AVRC_RN_CAP_MAX ||
        evt_set == NULL) {
        return XR_ERR_INVALID_ARG;
    }

    if (cap == XR_AVRC_RN_CAP_ALLOWED_EVT) {
        evt_set->bits = btc_avrc_tg_get_rn_allowed_evt();
    } else if (cap == XR_AVRC_RN_CAP_SUPPORTED_EVT) {
        evt_set->bits = btc_avrc_tg_get_rn_supported_evt();
    } else {
    }

    return XR_OK;
}

xr_err_t xr_avrc_tg_set_rn_evt_cap(const xr_avrc_rn_evt_cap_mask_t *evt_set)
{
    if (xr_bluedroid_get_status() != XR_BLUEDROID_STATUS_ENABLED) {
        return XR_ERR_INVALID_STATE;
    }
    if (evt_set == NULL) {
        return XR_ERR_INVALID_ARG;
    }

    bool allowed = btc_avrc_tg_check_rn_supported_evt(evt_set->bits);
    if (!allowed) {
        return XR_ERR_NOT_SUPPORTED;
    }

    btc_msg_t msg;
    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_AVRC_TG;
    msg.act = BTC_AVRC_TG_API_SET_RN_SUPPORTED_EVT;

    btc_avrc_tg_args_t arg;
    memset(&arg, 0, sizeof(btc_avrc_tg_args_t));

    arg.set_rn_evt = evt_set->bits;

    /* Switch to BTC context */
    bt_status_t stat = btc_transfer_context(&msg, &arg, sizeof(btc_avrc_tg_args_t), NULL);
    return (stat == BT_STATUS_SUCCESS) ? XR_OK : XR_FAIL;
}

bool xr_avrc_rn_evt_bit_mask_operation(xr_avrc_bit_mask_op_t op, xr_avrc_rn_evt_cap_mask_t *events,
                                        xr_avrc_rn_event_ids_t event_id)
{
    if (!events ||
        event_id >= XR_AVRC_RN_MAX_EVT) {
        return false;
    }

    uint16_t *p = &events->bits;
    uint16_t mask = (uint16_t)1 << ((uint8_t)event_id & 0x0F);
    switch (op) {
    case XR_AVRC_BIT_MASK_OP_SET:
        *p |= mask;
        break;
    case XR_AVRC_BIT_MASK_OP_CLEAR:
        *p &= ~mask;
        break;
    case XR_AVRC_BIT_MASK_OP_TEST:
        return (*p & mask);
    default:
        return false;
    }

    return true;
}

xr_err_t xr_avrc_tg_send_rn_rsp(xr_avrc_rn_event_ids_t event_id, xr_avrc_rn_rsp_t rsp,
                                  xr_avrc_rn_param_t *param)
{
    if ((xr_bluedroid_get_status() != XR_BLUEDROID_STATUS_ENABLED) ||
        (! btc_avrc_tg_connected_p())) {
#ifdef XRADIO_SINK_ADJUST_VOLUME
        if (btc_avrc_tg_connected_p_nofea()){
            return XRADIO_ADJ_BYMYSELF;
		}
		else
#endif
            return XR_ERR_INVALID_STATE;
    }

    if ( ! btc_avrc_tg_rn_evt_supported((uint8_t)event_id)) {
        return XR_ERR_NOT_SUPPORTED;
    }

    btc_msg_t msg;
    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_AVRC_TG;
    msg.act = BTC_AVRC_TG_API_SEND_RN_RSP_EVT;

    btc_avrc_tg_args_t arg;
    memset(&arg, 0, sizeof(btc_avrc_tg_args_t));

    arg.rn_rsp.event_id = event_id;
    arg.rn_rsp.rsp = rsp;
    memcpy(&arg.rn_rsp.param, param, sizeof(xr_avrc_rn_param_t));

    /* Switch to BTC context */
    bt_status_t stat = btc_transfer_context(&msg, &arg, sizeof(btc_avrc_tg_args_t), NULL);
    return (stat == BT_STATUS_SUCCESS) ? XR_OK : XR_FAIL;

}

#endif /* #if BTC_AV_INCLUDED */
