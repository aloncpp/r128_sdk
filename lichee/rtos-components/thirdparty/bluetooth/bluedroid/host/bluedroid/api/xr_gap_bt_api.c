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
#include "xr_bt_main.h"
#include "xr_gap_bt_api.h"
#include "common/bt_trace.h"
#include "bta/bta_api.h"
#include "btc/btc_manage.h"
#include "btc_gap_bt.h"
#include "btc/btc_storage.h"

#if (BTC_GAP_BT_INCLUDED == TRUE)

xr_err_t xr_bt_gap_register_callback(xr_bt_gap_cb_t callback)
{
    if (xr_bluedroid_get_status() != XR_BLUEDROID_STATUS_ENABLED) {
        return XR_ERR_INVALID_STATE;
    }

    if (callback == NULL) {
        return XR_FAIL;
    }

    btc_profile_cb_set(BTC_PID_GAP_BT, callback);
    return XR_OK;
}

xr_err_t xr_bt_gap_set_scan_mode(xr_bt_connection_mode_t c_mode, xr_bt_discovery_mode_t d_mode, xr_bt_paired_mode_t pair_mode, xr_bt_paired_con_mode_t conn_paired_only)
{
    btc_msg_t msg;
    btc_gap_bt_args_t arg;

    if (xr_bluedroid_get_status() != XR_BLUEDROID_STATUS_ENABLED) {
        return XR_ERR_INVALID_STATE;
    }

    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_GAP_BT;
    msg.act = BTC_GAP_BT_ACT_SET_SCAN_MODE;
    arg.set_scan_mode.c_mode = c_mode;
    arg.set_scan_mode.d_mode = d_mode;
    if (pair_mode)
        arg.set_scan_mode.pair_mode = 0x01;
    else
        arg.set_scan_mode.pair_mode = 0x00;
    if (conn_paired_only)
        arg.set_scan_mode.conn_paired_only = 0x01;
    else
        arg.set_scan_mode.conn_paired_only = 0x00;

    return (btc_transfer_context(&msg, &arg, sizeof(btc_gap_bt_args_t), NULL) == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
}

xr_err_t xr_bt_gap_set_ipscan_param(uint32_t window, uint32_t interval, uint8_t type)
{
    btc_msg_t msg;
    btc_gap_bt_args_t arg;

    if (xr_bluedroid_get_status() != XR_BLUEDROID_STATUS_ENABLED) {
        return XR_ERR_INVALID_STATE;
    }

    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_GAP_BT;
    msg.act = BTC_GAP_BT_ACT_SET_IPSCAN_PARAM;
    arg.set_ipscan_param.window = window;
    arg.set_ipscan_param.interval = interval;
    arg.set_ipscan_param.type = type;

    return (btc_transfer_context(&msg, &arg, sizeof(btc_gap_bt_args_t), NULL) == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
}
xr_err_t xr_bt_gap_start_discovery(xr_bt_inq_mode_t mode, uint8_t inq_len, uint8_t num_rsps)
{
    btc_msg_t msg;
    btc_gap_bt_args_t arg;

    if (xr_bluedroid_get_status() != XR_BLUEDROID_STATUS_ENABLED) {
        return XR_ERR_INVALID_STATE;
    }

    if (mode != XR_BT_INQ_MODE_GENERAL_INQUIRY &&
            mode != XR_BT_INQ_MODE_LIMITED_INQUIRY) {
        return XR_ERR_INVALID_ARG;
    }

    if (inq_len < XR_BT_GAP_MIN_INQ_LEN ||
            inq_len > XR_BT_GAP_MAX_INQ_LEN) {
        return XR_ERR_INVALID_ARG;
    }

    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_GAP_BT;
    msg.act = BTC_GAP_BT_ACT_START_DISCOVERY;

    arg.start_disc.mode = mode;
    arg.start_disc.inq_len = inq_len;
    arg.start_disc.num_rsps = num_rsps;

    return (btc_transfer_context(&msg, &arg, sizeof(btc_gap_bt_args_t), NULL) == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
}

xr_err_t xr_bt_gap_cancel_discovery(void)
{
    btc_msg_t msg;

    if (xr_bluedroid_get_status() != XR_BLUEDROID_STATUS_ENABLED) {
        return XR_ERR_INVALID_STATE;
    }

    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_GAP_BT;
    msg.act = BTC_GAP_BT_ACT_CANCEL_DISCOVERY;

    return (btc_transfer_context(&msg, NULL, 0, NULL) == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
}

xr_err_t xr_bt_gap_get_remote_services(xr_bd_addr_t remote_bda)
{
    btc_msg_t msg;
    btc_gap_bt_args_t arg;

    if (xr_bluedroid_get_status() != XR_BLUEDROID_STATUS_ENABLED) {
        return XR_ERR_INVALID_STATE;
    }

    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_GAP_BT;
    msg.act = BTC_GAP_BT_ACT_GET_REMOTE_SERVICES;

    memcpy(&arg.bda, remote_bda, sizeof(bt_bdaddr_t));
    return (btc_transfer_context(&msg, &arg, sizeof(btc_gap_bt_args_t), NULL) == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
}

xr_err_t xr_bt_gap_get_remote_service_record(xr_bd_addr_t remote_bda, xr_bt_uuid_t *uuid)
{
    btc_msg_t msg;
    btc_gap_bt_args_t arg;

    if (xr_bluedroid_get_status() != XR_BLUEDROID_STATUS_ENABLED) {
        return XR_ERR_INVALID_STATE;
    }

    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_GAP_BT;
    msg.act = BTC_GAP_BT_ACT_GET_REMOTE_SERVICE_RECORD;

    memcpy(&arg.get_rmt_srv_rcd.bda, remote_bda, sizeof(bt_bdaddr_t));
    memcpy(&arg.get_rmt_srv_rcd.uuid, uuid, sizeof(xr_bt_uuid_t));
    return (btc_transfer_context(&msg, &arg, sizeof(btc_gap_bt_args_t), NULL) == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
}

uint8_t *xr_bt_gap_resolve_eir_data(uint8_t *eir, xr_bt_eir_type_t type, uint8_t *length)
{
    if (!eir) {
        return NULL;
    }

    return BTM_CheckEirData(eir, type, length);
}

xr_err_t xr_bt_gap_config_eir_data(xr_bt_eir_data_t *eir_data)
{
    btc_msg_t msg;
    btc_gap_bt_args_t arg;

    if (xr_bluedroid_get_status() != XR_BLUEDROID_STATUS_ENABLED) {
        return XR_ERR_INVALID_STATE;
    }

    if (eir_data == NULL) {
        return XR_ERR_INVALID_ARG;
    }

    if (eir_data->manufacturer_len > XR_BT_EIR_MAX_LEN
        || eir_data->url_len > XR_BT_EIR_MAX_LEN) {
        return XR_ERR_INVALID_ARG;
    }

    if ((eir_data->manufacturer_len > 0 && eir_data->p_manufacturer_data == NULL)
        || (eir_data->url_len > 0 && eir_data->p_url == NULL)) {
        return XR_ERR_INVALID_ARG;
    }

    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_GAP_BT;
    msg.act = BTC_GAP_BT_ACT_CONFIG_EIR;

    memcpy(&arg.config_eir, eir_data, sizeof(xr_bt_eir_data_t));

    return (btc_transfer_context(&msg, &arg, sizeof(btc_gap_bt_args_t), btc_gap_bt_arg_deep_copy) == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
}

xr_err_t xr_bt_gap_set_cod(xr_bt_cod_t cod, xr_bt_cod_mode_t mode)
{
    btc_msg_t msg;
    btc_gap_bt_args_t arg;

    if (xr_bluedroid_get_status() != XR_BLUEDROID_STATUS_ENABLED) {
        return XR_ERR_INVALID_STATE;
    }

    switch (mode) {
    case XR_BT_SET_COD_MAJOR_MINOR:
    case XR_BT_SET_COD_SERVICE_CLASS:
    case XR_BT_CLR_COD_SERVICE_CLASS:
    case XR_BT_SET_COD_ALL:
    case XR_BT_INIT_COD:
        break;
    default:
        return XR_ERR_INVALID_ARG;
        break;
    }

    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_GAP_BT;
    msg.act = BTC_GAP_BT_ACT_SET_COD;

    arg.set_cod.mode = mode;
    memcpy(&arg.set_cod.cod, &cod, sizeof(xr_bt_cod_t));
    return (btc_transfer_context(&msg, &arg, sizeof(btc_gap_bt_args_t), NULL) == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
}


xr_err_t xr_bt_gap_get_cod(xr_bt_cod_t *cod)
{
    return btc_gap_bt_get_cod(cod);
}


xr_err_t xr_bt_gap_read_rssi_delta(xr_bd_addr_t remote_addr)
{
    btc_msg_t msg;
    btc_gap_bt_args_t arg;
    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_GAP_BT;
    msg.act = BTC_GAP_BT_ACT_READ_RSSI_DELTA;
    memcpy(arg.read_rssi_delta.bda.address, remote_addr, sizeof(xr_bd_addr_t));

    return (btc_transfer_context(&msg, &arg, sizeof(btc_gap_bt_args_t), NULL) == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
}

xr_err_t xr_bt_gap_remove_bond_device(xr_bd_addr_t bd_addr)
{
    btc_msg_t msg;
    btc_gap_bt_args_t arg;

    if (xr_bluedroid_get_status() != XR_BLUEDROID_STATUS_ENABLED) {
        return XR_ERR_INVALID_STATE;
    }

    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_GAP_BT;
    msg.act = BTC_GAP_BT_ACT_REMOVE_BOND_DEVICE;

    memcpy(arg.rm_bond_device.bda.address, bd_addr, sizeof(xr_bd_addr_t));
    return (btc_transfer_context(&msg, &arg, sizeof(btc_gap_bt_args_t), NULL) == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
}

int xr_bt_gap_get_bond_device_num(void)
{
    if (xr_bluedroid_get_status() != XR_BLUEDROID_STATUS_ENABLED) {
        return XR_ERR_INVALID_STATE;
    }
    return btc_storage_get_num_bt_bond_devices();
}

xr_err_t xr_bt_gap_get_bond_device_list(int *dev_num, xr_bd_addr_t *dev_list_addr, xr_bd_name_t *dev_list_name)
{
    int ret;

    if (dev_num == NULL || dev_list_addr == NULL || dev_list_name == NULL) {
        return XR_ERR_INVALID_ARG;
    }

    if (xr_bluedroid_get_status() != XR_BLUEDROID_STATUS_ENABLED) {
        return XR_ERR_INVALID_STATE;
    }

    ret = btc_storage_get_bonded_bt_devices_list((bt_bdaddr_t *)dev_list_addr, (bt_bdname_t *)dev_list_name, dev_num);

    return (ret == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
}

xr_err_t xr_bt_gap_set_pin(xr_bt_pin_type_t pin_type, uint8_t pin_code_len, xr_bt_pin_code_t pin_code)
{
    btc_msg_t msg;
    btc_gap_bt_args_t arg;

    if (xr_bluedroid_get_status() != XR_BLUEDROID_STATUS_ENABLED) {
        return XR_ERR_INVALID_STATE;
    }

    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_GAP_BT;
    msg.act = BTC_GAP_BT_ACT_SET_PIN_TYPE;
    arg.set_pin_type.pin_type = pin_type;
    if (pin_type == XR_BT_PIN_TYPE_FIXED){
        arg.set_pin_type.pin_code_len = pin_code_len;
        memcpy(arg.set_pin_type.pin_code, pin_code, pin_code_len);
    } else {
        arg.set_pin_type.pin_code_len = 0;
        memset(arg.set_pin_type.pin_code, 0, XR_BT_PIN_CODE_LEN);
    }

    return (btc_transfer_context(&msg, &arg, sizeof(btc_gap_bt_args_t), btc_gap_bt_arg_deep_copy)
            == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
}

xr_err_t xr_bt_gap_pin_reply(xr_bd_addr_t bd_addr, bool accept, uint8_t pin_code_len, xr_bt_pin_code_t pin_code)
{
    btc_msg_t msg;
    btc_gap_bt_args_t arg;

    if (xr_bluedroid_get_status() != XR_BLUEDROID_STATUS_ENABLED) {
        return XR_ERR_INVALID_STATE;
    }

    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_GAP_BT;
    msg.act = BTC_GAP_BT_ACT_PIN_REPLY;
    arg.pin_reply.accept = accept;
    arg.pin_reply.pin_code_len = pin_code_len;
    memcpy(arg.pin_reply.bda.address, bd_addr, sizeof(xr_bd_addr_t));
    memcpy(arg.pin_reply.pin_code, pin_code, pin_code_len);

    return (btc_transfer_context(&msg, &arg, sizeof(btc_gap_bt_args_t), btc_gap_bt_arg_deep_copy)
            == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
}

#if (BT_SSP_INCLUDED == TRUE)
xr_err_t xr_bt_gap_set_security_param(xr_bt_sp_param_t param_type,
        void *value, uint8_t len)
{
    btc_msg_t msg;
    btc_gap_bt_args_t arg;

    if (xr_bluedroid_get_status() != XR_BLUEDROID_STATUS_ENABLED) {
        return XR_ERR_INVALID_STATE;
    }

    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_GAP_BT;
    msg.act = BTC_GAP_BT_ACT_SET_SECURITY_PARAM;
    arg.set_security_param.param_type = param_type;
    arg.set_security_param.len = len;
    arg.set_security_param.value = value;

    return (btc_transfer_context(&msg, &arg, sizeof(btc_gap_bt_args_t), btc_gap_bt_arg_deep_copy)
            == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
}

xr_err_t xr_bt_gap_ssp_passkey_reply(xr_bd_addr_t bd_addr, bool accept, uint32_t passkey)
{
    btc_msg_t msg;
    btc_gap_bt_args_t arg;

    if (xr_bluedroid_get_status() != XR_BLUEDROID_STATUS_ENABLED) {
        return XR_ERR_INVALID_STATE;
    }

    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_GAP_BT;
    msg.act = BTC_GAP_BT_ACT_PASSKEY_REPLY;
    arg.passkey_reply.accept = accept;
    arg.passkey_reply.passkey = passkey;
    memcpy(arg.passkey_reply.bda.address, bd_addr, sizeof(xr_bd_addr_t));
    return (btc_transfer_context(&msg, &arg, sizeof(btc_gap_bt_args_t), btc_gap_bt_arg_deep_copy)
            == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
}

xr_err_t xr_bt_gap_ssp_confirm_reply(xr_bd_addr_t bd_addr, bool accept)
{
    btc_msg_t msg;
    btc_gap_bt_args_t arg;

    if (xr_bluedroid_get_status() != XR_BLUEDROID_STATUS_ENABLED) {
        return XR_ERR_INVALID_STATE;
    }

    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_GAP_BT;
    msg.act = BTC_GAP_BT_ACT_CONFIRM_REPLY;
    arg.confirm_reply.accept = accept;
    memcpy(arg.confirm_reply.bda.address, bd_addr, sizeof(xr_bd_addr_t));
    return (btc_transfer_context(&msg, &arg, sizeof(btc_gap_bt_args_t), btc_gap_bt_arg_deep_copy)
            == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
}

#endif /*(BT_SSP_INCLUDED == TRUE)*/

xr_err_t xr_bt_gap_set_afh_channels(xr_bt_gap_afh_channels channels)
{
    btc_msg_t msg;
    btc_gap_bt_args_t arg;

    if (xr_bluedroid_get_status() != XR_BLUEDROID_STATUS_ENABLED) {
        return XR_ERR_INVALID_STATE;
    }

    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_GAP_BT;
    msg.act = BTC_GAP_BT_ACT_SET_AFH_CHANNELS;

    memcpy(&arg.set_afh_channels.channels, channels, XR_BT_GAP_AFH_CHANNELS_LEN);
    arg.set_afh_channels.channels[XR_BT_GAP_AFH_CHANNELS_LEN -1] &= 0x7F;
    return (btc_transfer_context(&msg, &arg, sizeof(btc_gap_bt_args_t), NULL) == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
}

xr_err_t xr_bt_gap_read_remote_name(xr_bd_addr_t remote_bda)
{
    btc_msg_t msg;
    btc_gap_bt_args_t arg;

    if (xr_bluedroid_get_status() != XR_BLUEDROID_STATUS_ENABLED) {
        return XR_ERR_INVALID_STATE;
    }

    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_GAP_BT;
    msg.act = BTC_GAP_BT_ACT_READ_REMOTE_NAME;

    memcpy(&arg.rmt_name_bda, remote_bda, sizeof(bt_bdaddr_t));
    return (btc_transfer_context(&msg, &arg, sizeof(btc_gap_bt_args_t), NULL) == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
}

xr_err_t xr_bt_gap_set_sniff_mode(xr_bd_addr_t remote_bda, xr_bt_sniff_mode_t sniff_mode,
           uint32_t min_period, uint32_t max_period, uint32_t attempt,
           uint32_t timeout)
{
   btc_msg_t msg;
   btc_gap_bt_args_t arg;

   if (xr_bluedroid_get_status() != XR_BLUEDROID_STATUS_ENABLED) {
       return XR_ERR_INVALID_STATE;
   }

   msg.sig = BTC_SIG_API_CALL;
   msg.pid = BTC_PID_GAP_BT;
   msg.act = BTC_GAP_BT_ACT_SET_SNIFF_MODE;

   memcpy(&arg.config_sniff.bda, remote_bda, sizeof(bt_bdaddr_t));
   arg.config_sniff.sniff_mode = sniff_mode;
   arg.config_sniff.min_period = min_period;
   arg.config_sniff.max_period = max_period;
   arg.config_sniff.attempts = attempt;
   arg.config_sniff.timeout = timeout;
   return (btc_transfer_context(&msg, &arg, sizeof(btc_gap_bt_args_t), NULL) == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
}

xr_err_t xr_bt_gap_power_mode(xr_bd_addr_t remote_bda)
{
    btc_msg_t msg;
    btc_gap_bt_args_t arg;

    if (xr_bluedroid_get_status() != XR_BLUEDROID_STATUS_ENABLED) {
        return XR_ERR_INVALID_STATE;
    }

    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_GAP_BT;
    msg.act = BTC_GAP_BT_ACT_POWER_MODE;

    memcpy(&arg.config_power_mode.bda, remote_bda, sizeof(bt_bdaddr_t));

    return (btc_transfer_context(&msg, &arg, sizeof(btc_gap_bt_args_t), NULL) == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
}

xr_err_t xr_bt_gap_set_role(xr_bd_addr_t remote_bda, xr_bt_role_t role)
{
    btc_msg_t msg;
    btc_gap_bt_args_t arg;

    if (xr_bluedroid_get_status() != XR_BLUEDROID_STATUS_ENABLED) {
        return XR_ERR_INVALID_STATE;
    }

    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_GAP_BT;
    msg.act = BTC_GAP_BT_ACT_SET_ROLE;

    memcpy(&arg.config_role.bda, remote_bda, sizeof(bt_bdaddr_t));
    arg.config_role.role = role;
    return (btc_transfer_context(&msg, &arg, sizeof(btc_gap_bt_args_t), NULL) == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
}

xr_err_t xr_bt_gap_role(xr_bd_addr_t remote_bda)
{
    btc_msg_t msg;
    btc_gap_bt_args_t arg;

    if (xr_bluedroid_get_status() != XR_BLUEDROID_STATUS_ENABLED) {
        return XR_ERR_INVALID_STATE;
    }

    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_GAP_BT;
    msg.act = BTC_GAP_BT_ACT_READ_ROLE;

    memcpy(&arg.read_role.bda, remote_bda, sizeof(bt_bdaddr_t));
    return (btc_transfer_context(&msg, &arg, sizeof(btc_gap_bt_args_t), NULL) == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
}

xr_err_t xr_bt_gap_set_auto_sniff_mode(uint8_t mode)
{
    btc_msg_t msg;
    btc_gap_bt_args_t arg;

    if (xr_bluedroid_get_status() != XR_BLUEDROID_STATUS_ENABLED) {
        return XR_ERR_INVALID_STATE;
    }

    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_GAP_BT;
    msg.act = BTC_GAP_BT_ACT_SET_AUTO_SNIFF_MODE;

    arg.auto_sniff_mode.mode = mode;

    return (btc_transfer_context(&msg, &arg, sizeof(btc_gap_bt_args_t), NULL) == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
}
#endif /* #if BTC_GAP_BT_INCLUDED == TRUE */
