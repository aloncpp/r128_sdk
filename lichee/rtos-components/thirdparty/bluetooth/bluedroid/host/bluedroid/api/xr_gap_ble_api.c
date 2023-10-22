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

#include <string.h>
#include "xr_bt_device.h"
#include "xr_bt_main.h"
#include "xr_gap_ble_api.h"
#include "bta/bta_api.h"
#include "common/bt_trace.h"
#include "btc/btc_manage.h"
#include "btc_gap_ble.h"
#include "btc/btc_ble_storage.h"


xr_err_t xr_ble_gap_register_callback(xr_gap_ble_cb_t callback)
{
    XR_BLUEDROID_STATUS_CHECK(XR_BLUEDROID_STATUS_ENABLED);

    return (btc_profile_cb_set(BTC_PID_GAP_BLE, callback) == 0 ? XR_OK : XR_FAIL);
}


xr_err_t xr_ble_gap_config_adv_data(xr_ble_adv_data_t *adv_data)
{
    btc_msg_t msg;
    btc_ble_gap_args_t arg;

    XR_BLUEDROID_STATUS_CHECK(XR_BLUEDROID_STATUS_ENABLED);

    if (adv_data == NULL) {
        return XR_ERR_INVALID_ARG;
    }

    if (adv_data->service_uuid_len & 0xf) { //not 16*n
        return XR_ERR_INVALID_ARG;
    }

    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_GAP_BLE;
    msg.act = BTC_GAP_BLE_ACT_CFG_ADV_DATA;
    memcpy(&arg.cfg_adv_data.adv_data, adv_data, sizeof(xr_ble_adv_data_t));

    return (btc_transfer_context(&msg, &arg, sizeof(btc_ble_gap_args_t), btc_gap_ble_arg_deep_copy) == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);

}


xr_err_t xr_ble_gap_set_scan_params(xr_ble_scan_params_t *scan_params)
{
    btc_msg_t msg;
    btc_ble_gap_args_t arg;

    XR_BLUEDROID_STATUS_CHECK(XR_BLUEDROID_STATUS_ENABLED);

    if (scan_params == NULL) {
        return XR_ERR_INVALID_ARG;
    }

    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_GAP_BLE;
    msg.act = BTC_GAP_BLE_ACT_SET_SCAN_PARAM;
    memcpy(&arg.set_scan_param.scan_params, scan_params, sizeof(xr_ble_scan_params_t));

    return (btc_transfer_context(&msg, &arg, sizeof(btc_ble_gap_args_t), NULL) == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
}

xr_err_t xr_ble_gap_start_scanning(uint32_t duration)
{
    btc_msg_t msg;
    btc_ble_gap_args_t arg;

    XR_BLUEDROID_STATUS_CHECK(XR_BLUEDROID_STATUS_ENABLED);

    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_GAP_BLE;
    msg.act = BTC_GAP_BLE_ACT_START_SCAN;
    arg.start_scan.duration = duration;

    return (btc_transfer_context(&msg, &arg, sizeof(btc_ble_gap_args_t), NULL) == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
}


xr_err_t xr_ble_gap_stop_scanning(void)
{
    btc_msg_t msg;

    XR_BLUEDROID_STATUS_CHECK(XR_BLUEDROID_STATUS_ENABLED);

    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_GAP_BLE;
    msg.act = BTC_GAP_BLE_ACT_STOP_SCAN;
    return (btc_transfer_context(&msg, NULL, 0, NULL) == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
}

xr_err_t xr_ble_gap_start_advertising(xr_ble_adv_params_t *adv_params)
{
    btc_msg_t msg;
    btc_ble_gap_args_t arg;

    XR_BLUEDROID_STATUS_CHECK(XR_BLUEDROID_STATUS_ENABLED);

    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_GAP_BLE;
    msg.act = BTC_GAP_BLE_ACT_START_ADV;
    memcpy(&arg.start_adv.adv_params, adv_params, sizeof(xr_ble_adv_params_t));

    return (btc_transfer_context(&msg, &arg, sizeof(btc_ble_gap_args_t), NULL) == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
}

xr_err_t xr_ble_gap_stop_advertising(void)
{
    btc_msg_t msg;

    XR_BLUEDROID_STATUS_CHECK(XR_BLUEDROID_STATUS_ENABLED);

    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_GAP_BLE;
    msg.act = BTC_GAP_BLE_ACT_STOP_ADV;

    return (btc_transfer_context(&msg, NULL, 0, NULL) == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
}


xr_err_t xr_ble_gap_update_conn_params(xr_ble_conn_update_params_t *params)
{
    btc_msg_t msg;
    btc_ble_gap_args_t arg;

    XR_BLUEDROID_STATUS_CHECK(XR_BLUEDROID_STATUS_ENABLED);
    if(!params) {
        LOG_ERROR("%s,params is NULL", __func__);
        return XR_FAIL;
    }

    if (XR_BLE_IS_VALID_PARAM(params->min_int, XR_BLE_CONN_INT_MIN, XR_BLE_CONN_INT_MAX) &&
        XR_BLE_IS_VALID_PARAM(params->max_int, XR_BLE_CONN_INT_MIN, XR_BLE_CONN_INT_MAX) &&
        XR_BLE_IS_VALID_PARAM(params->timeout, XR_BLE_CONN_SUP_TOUT_MIN, XR_BLE_CONN_SUP_TOUT_MAX) &&
        (params->latency <= XR_BLE_CONN_LATENCY_MAX || params->latency == XR_BLE_CONN_PARAM_UNDEF) &&
        ((params->timeout * 10) >= ((1 + params->latency) * ((params->max_int * 5) >> 1))) && params->min_int <= params->max_int) {

        msg.sig = BTC_SIG_API_CALL;
        msg.pid = BTC_PID_GAP_BLE;
        msg.act = BTC_GAP_BLE_ACT_UPDATE_CONN_PARAM;
        memcpy(&arg.conn_update_params.conn_params, params, sizeof(xr_ble_conn_update_params_t));

        return (btc_transfer_context(&msg, &arg, sizeof(btc_ble_gap_args_t), NULL) == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
    } else {
        LOG_ERROR("%s,invalid connection params:min_int = %d, max_int = %d, latency = %d, timeout = %d",\
                            __func__, params->min_int, params->max_int, params->latency, params->timeout);
        return XR_FAIL;
    }
}

xr_err_t xr_ble_gap_set_pkt_data_len(xr_bd_addr_t remote_device, uint16_t tx_data_length)
{
    btc_msg_t msg;
    btc_ble_gap_args_t arg;

    XR_BLUEDROID_STATUS_CHECK(XR_BLUEDROID_STATUS_ENABLED);

    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_GAP_BLE;
    msg.act = BTC_GAP_BLE_ACT_SET_PKT_DATA_LEN;
    arg.set_pkt_data_len.tx_data_length = tx_data_length;
    memcpy(arg.set_pkt_data_len.remote_device, remote_device, XR_BD_ADDR_LEN);

    return (btc_transfer_context(&msg, &arg, sizeof(btc_ble_gap_args_t), NULL) == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
}


xr_err_t xr_ble_gap_set_rand_addr(xr_bd_addr_t rand_addr)
{
    btc_msg_t msg;
    btc_ble_gap_args_t arg;

    XR_BLUEDROID_STATUS_CHECK(XR_BLUEDROID_STATUS_ENABLED);

    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_GAP_BLE;
    msg.act = BTC_GAP_BLE_ACT_SET_RAND_ADDRESS;
    memcpy(arg.set_rand_addr.rand_addr, rand_addr, XR_BD_ADDR_LEN);

    return (btc_transfer_context(&msg, &arg, sizeof(btc_ble_gap_args_t), NULL) == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
}

xr_err_t xr_ble_gap_clear_rand_addr(void)
{
    btc_msg_t msg;

    XR_BLUEDROID_STATUS_CHECK(XR_BLUEDROID_STATUS_ENABLED);

    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_GAP_BLE;
    msg.act = BTC_GAP_BLE_ACT_CLEAR_RAND_ADDRESS;

    return (btc_transfer_context(&msg, NULL, 0, NULL) == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
}

xr_err_t xr_ble_gap_config_local_privacy (bool privacy_enable)
{
    btc_msg_t msg;
    btc_ble_gap_args_t arg;

    XR_BLUEDROID_STATUS_CHECK(XR_BLUEDROID_STATUS_ENABLED);

    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_GAP_BLE;
    msg.act = BTC_GAP_BLE_ACT_CONFIG_LOCAL_PRIVACY;
    arg.cfg_local_privacy.privacy_enable = privacy_enable;

    return (btc_transfer_context(&msg, &arg, sizeof(btc_ble_gap_args_t), NULL) == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
}

xr_err_t xr_ble_gap_config_local_icon (uint16_t icon)
{
    xr_err_t ret;
    btc_msg_t msg;
    btc_ble_gap_args_t arg;

    XR_BLUEDROID_STATUS_CHECK(XR_BLUEDROID_STATUS_ENABLED);

    switch (icon) {
    case XR_BLE_APPEARANCE_GENERIC_PHONE:
    case XR_BLE_APPEARANCE_GENERIC_COMPUTER:
    case XR_BLE_APPEARANCE_GENERIC_REMOTE:
    case XR_BLE_APPEARANCE_GENERIC_THERMOMETER:
    case XR_BLE_APPEARANCE_THERMOMETER_EAR:
    case XR_BLE_APPEARANCE_GENERIC_HEART_RATE:
    case XR_BLE_APPEARANCE_HEART_RATE_BELT:
    case XR_BLE_APPEARANCE_GENERIC_BLOOD_PRESSURE:
    case XR_BLE_APPEARANCE_BLOOD_PRESSURE_ARM:
    case XR_BLE_APPEARANCE_BLOOD_PRESSURE_WRIST:
    case XR_BLE_APPEARANCE_GENERIC_PULSE_OXIMETER:
    case XR_BLE_APPEARANCE_PULSE_OXIMETER_FINGERTIP:
    case XR_BLE_APPEARANCE_PULSE_OXIMETER_WRIST:
    case XR_BLE_APPEARANCE_GENERIC_GLUCOSE:
    case XR_BLE_APPEARANCE_GENERIC_WEIGHT:
    case XR_BLE_APPEARANCE_GENERIC_WALKING:
    case XR_BLE_APPEARANCE_WALKING_IN_SHOE:
    case XR_BLE_APPEARANCE_WALKING_ON_SHOE:
    case XR_BLE_APPEARANCE_WALKING_ON_HIP:
    case XR_BLE_APPEARANCE_GENERIC_WATCH:
    case XR_BLE_APPEARANCE_SPORTS_WATCH:
    case XR_BLE_APPEARANCE_GENERIC_EYEGLASSES:
    case XR_BLE_APPEARANCE_GENERIC_DISPLAY:
    case XR_BLE_APPEARANCE_GENERIC_MEDIA_PLAYER:
    case XR_BLE_APPEARANCE_GENERIC_BARCODE_SCANNER:
    case XR_BLE_APPEARANCE_HID_BARCODE_SCANNER:
    case XR_BLE_APPEARANCE_GENERIC_HID:
    case XR_BLE_APPEARANCE_HID_KEYBOARD:
    case XR_BLE_APPEARANCE_HID_MOUSE:
    case XR_BLE_APPEARANCE_HID_JOYSTICK:
    case XR_BLE_APPEARANCE_HID_GAMEPAD:
    case XR_BLE_APPEARANCE_HID_DIGITIZER_TABLET:
    case XR_BLE_APPEARANCE_HID_CARD_READER:
    case XR_BLE_APPEARANCE_HID_DIGITAL_PEN:
    case XR_BLE_APPEARANCE_UNKNOWN:
    case XR_BLE_APPEARANCE_GENERIC_CLOCK:
    case XR_BLE_APPEARANCE_GENERIC_TAG:
    case XR_BLE_APPEARANCE_GENERIC_KEYRING:
    case XR_BLE_APPEARANCE_GENERIC_CYCLING:
    case XR_BLE_APPEARANCE_CYCLING_COMPUTER:
    case XR_BLE_APPEARANCE_CYCLING_SPEED:
    case XR_BLE_APPEARANCE_CYCLING_CADENCE:
    case XR_BLE_APPEARANCE_CYCLING_POWER:
    case XR_BLE_APPEARANCE_CYCLING_SPEED_CADENCE:
    case XR_BLE_APPEARANCE_GENERIC_PERSONAL_MOBILITY_DEVICE:
    case XR_BLE_APPEARANCE_POWERED_WHEELCHAIR:
    case XR_BLE_APPEARANCE_MOBILITY_SCOOTER:
    case XR_BLE_APPEARANCE_GENERIC_CONTINUOUS_GLUCOSE_MONITOR:
    case XR_BLE_APPEARANCE_GENERIC_INSULIN_PUMP:
    case XR_BLE_APPEARANCE_INSULIN_PUMP_DURABLE_PUMP:
    case XR_BLE_APPEARANCE_INSULIN_PUMP_PATCH_PUMP:
    case XR_BLE_APPEARANCE_INSULIN_PEN:
    case XR_BLE_APPEARANCE_GENERIC_MEDICATION_DELIVERY:
    case XR_BLE_APPEARANCE_GENERIC_OUTDOOR_SPORTS:
    case XR_BLE_APPEARANCE_OUTDOOR_SPORTS_LOCATION:
    case XR_BLE_APPEARANCE_OUTDOOR_SPORTS_LOCATION_AND_NAV:
    case XR_BLE_APPEARANCE_OUTDOOR_SPORTS_LOCATION_POD:
    case XR_BLE_APPEARANCE_OUTDOOR_SPORTS_LOCATION_POD_AND_NAV:
        msg.sig = BTC_SIG_API_CALL;
        msg.pid = BTC_PID_GAP_BLE;
        msg.act = BTC_GAP_BLE_ACT_CONFIG_LOCAL_ICON;
        arg.cfg_local_icon.icon = icon;
        ret = (btc_transfer_context(&msg, &arg, sizeof(btc_ble_gap_args_t), NULL) == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
        break;
    default:
        ret = XR_ERR_INVALID_ARG;
        break;
    }
    return ret;
}

xr_err_t xr_ble_gap_update_whitelist(bool add_remove, xr_bd_addr_t remote_bda, xr_ble_wl_addr_type_t wl_addr_type)
{
    btc_msg_t msg;
    btc_ble_gap_args_t arg;

    if (xr_bluedroid_get_status() != XR_BLUEDROID_STATUS_ENABLED) {
        return XR_ERR_INVALID_STATE;
    }
    if (!remote_bda){
        return XR_ERR_INVALID_SIZE;
    }
    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_GAP_BLE;
    msg.act = BTC_GAP_BLE_ACT_UPDATE_WHITE_LIST;
    arg.update_white_list.add_remove = add_remove;
    arg.update_white_list.wl_addr_type = wl_addr_type;
    memcpy(arg.update_white_list.remote_bda, remote_bda, sizeof(xr_bd_addr_t));

    return (btc_transfer_context(&msg, &arg, sizeof(btc_ble_gap_args_t), NULL)
                == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
}

xr_err_t xr_ble_gap_get_whitelist_size(uint16_t *length)
{
    if (length == NULL) {
        return XR_FAIL;
    }
    btc_get_whitelist_size(length);

    return XR_OK;
}

xr_err_t xr_ble_gap_set_prefer_conn_params(xr_bd_addr_t bd_addr,
                                                                 uint16_t min_conn_int, uint16_t max_conn_int,
                                                                 uint16_t slave_latency, uint16_t supervision_tout)
{
    btc_msg_t msg;
    btc_ble_gap_args_t arg;

    if (xr_bluedroid_get_status() != XR_BLUEDROID_STATUS_ENABLED) {
        return XR_ERR_INVALID_STATE;
    }

    if (XR_BLE_IS_VALID_PARAM(min_conn_int, XR_BLE_CONN_INT_MIN, XR_BLE_CONN_INT_MAX) &&
        XR_BLE_IS_VALID_PARAM(max_conn_int, XR_BLE_CONN_INT_MIN, XR_BLE_CONN_INT_MAX) &&
        XR_BLE_IS_VALID_PARAM(supervision_tout, XR_BLE_CONN_SUP_TOUT_MIN, XR_BLE_CONN_SUP_TOUT_MAX) &&
        (slave_latency <= XR_BLE_CONN_LATENCY_MAX || slave_latency == XR_BLE_CONN_PARAM_UNDEF) &&
        ((supervision_tout * 10) >= ((1 + slave_latency) * ((max_conn_int * 5) >> 1))) && min_conn_int <= max_conn_int) {

        msg.sig = BTC_SIG_API_CALL;
        msg.pid = BTC_PID_GAP_BLE;
        msg.act = BTC_GAP_BLE_ACT_SET_CONN_PARAMS;
        arg.set_conn_params.min_conn_int = min_conn_int;
        arg.set_conn_params.max_conn_int = max_conn_int;
        arg.set_conn_params.slave_latency = slave_latency;
        arg.set_conn_params.supervision_tout = supervision_tout;
        memcpy(arg.set_conn_params.bd_addr, bd_addr, sizeof(xr_bd_addr_t));

        return (btc_transfer_context(&msg, &arg, sizeof(btc_ble_gap_args_t), NULL)
                    == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
    } else {
        LOG_ERROR("%s,invalid connection params:min_int = %d, max_int = %d, latency = %d, timeout = %d",\
                            __func__, min_conn_int, max_conn_int, slave_latency, supervision_tout);
        return XR_FAIL;
    }
}

xr_err_t xr_ble_gap_set_device_name(const char *name)
{
    XR_BLUEDROID_STATUS_CHECK(XR_BLUEDROID_STATUS_ENABLED);

    return xr_bt_dev_set_device_name(name);
}

xr_err_t xr_ble_gap_get_local_used_addr(xr_bd_addr_t local_used_addr, uint8_t * addr_type)
{
    if(xr_bluedroid_get_status() != (XR_BLUEDROID_STATUS_ENABLED)) {
        LOG_ERROR("%s, bluedroid status error", __func__);
        return XR_FAIL;
    }
    if(!BTM_BleGetCurrentAddress(local_used_addr, addr_type)) {
        return XR_FAIL;
    }
    return XR_OK;
}

uint8_t *xr_ble_resolve_adv_data( uint8_t *adv_data, uint8_t type, uint8_t *length)
{
    if (((type < XR_BLE_AD_TYPE_FLAG) || (type > XR_BLE_AD_TYPE_128SERVICE_DATA)) &&
            (type != XR_BLE_AD_MANUFACTURER_SPECIFIC_TYPE)) {
        LOG_ERROR("the eir type not define, type = %x\n", type);
        return NULL;
    }

    if (adv_data == NULL) {
        LOG_ERROR("Invalid p_eir data.\n");
        return NULL;
    }

    return (BTM_CheckAdvData( adv_data, type, length));
}

xr_err_t xr_ble_gap_config_adv_data_raw(uint8_t *raw_data, uint32_t raw_data_len)
{
    btc_msg_t msg;
    btc_ble_gap_args_t arg;

    XR_BLUEDROID_STATUS_CHECK(XR_BLUEDROID_STATUS_ENABLED);

    if (raw_data == NULL
            || (raw_data_len <= 0 || raw_data_len > XR_BLE_ADV_DATA_LEN_MAX)) {
        return XR_ERR_INVALID_ARG;
    }

    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_GAP_BLE;
    msg.act = BTC_GAP_BLE_ACT_CFG_ADV_DATA_RAW;
    arg.cfg_adv_data_raw.raw_adv = raw_data;
    arg.cfg_adv_data_raw.raw_adv_len = raw_data_len;

    return (btc_transfer_context(&msg, &arg, sizeof(btc_ble_gap_args_t), btc_gap_ble_arg_deep_copy) == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);

}

xr_err_t xr_ble_gap_read_rssi(xr_bd_addr_t remote_addr)
{
    btc_msg_t msg;
    btc_ble_gap_args_t arg;

    if (xr_bluedroid_get_status() != XR_BLUEDROID_STATUS_ENABLED) {
        return XR_ERR_INVALID_STATE;
    }

    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_GAP_BLE;
    msg.act = BTC_GAP_BLE_ACT_READ_RSSI;
    memcpy(arg.read_rssi.remote_addr, remote_addr, sizeof(xr_bd_addr_t));

    return (btc_transfer_context(&msg, &arg, sizeof(btc_ble_gap_args_t), NULL) == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
}

xr_err_t xr_ble_gap_config_scan_rsp_data_raw(uint8_t *raw_data, uint32_t raw_data_len)
{
    btc_msg_t msg;
    btc_ble_gap_args_t arg;

    XR_BLUEDROID_STATUS_CHECK(XR_BLUEDROID_STATUS_ENABLED);

    if (raw_data == NULL
            || (raw_data_len <= 0 || raw_data_len > XR_BLE_SCAN_RSP_DATA_LEN_MAX)) {
        return XR_ERR_INVALID_ARG;
    }

    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_GAP_BLE;
    msg.act = BTC_GAP_BLE_ACT_CFG_SCAN_RSP_DATA_RAW;
    arg.cfg_scan_rsp_data_raw.raw_scan_rsp = raw_data;
    arg.cfg_scan_rsp_data_raw.raw_scan_rsp_len = raw_data_len;

    return (btc_transfer_context(&msg, &arg, sizeof(btc_ble_gap_args_t), btc_gap_ble_arg_deep_copy) == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);

}

xr_err_t xr_ble_gap_add_duplicate_scan_exceptional_device(xr_ble_duplicate_exceptional_info_type_t type, xr_duplicate_info_t device_info)
{
    btc_msg_t msg;
    btc_ble_gap_args_t arg;

    if (xr_bluedroid_get_status() != XR_BLUEDROID_STATUS_ENABLED) {
        return XR_ERR_INVALID_STATE;
    }
    if (!device_info && type <= XR_BLE_DUPLICATE_SCAN_EXCEPTIONAL_INFO_MESH_LINK_ID) {
        return XR_ERR_INVALID_SIZE;
    }
    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_GAP_BLE;
    msg.act = BTC_GAP_BLE_UPDATE_DUPLICATE_SCAN_EXCEPTIONAL_LIST;
    arg.update_duplicate_exceptional_list.subcode = XR_BLE_DUPLICATE_EXCEPTIONAL_LIST_ADD;
    arg.update_duplicate_exceptional_list.info_type = type;
    if (device_info) {
        memcpy(arg.update_duplicate_exceptional_list.device_info, device_info, sizeof(xr_bd_addr_t));
    }

    return (btc_transfer_context(&msg, &arg, sizeof(btc_ble_gap_args_t), NULL)
                == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
}

xr_err_t xr_ble_gap_remove_duplicate_scan_exceptional_device(xr_ble_duplicate_exceptional_info_type_t type, xr_duplicate_info_t device_info)
{
    btc_msg_t msg;
    btc_ble_gap_args_t arg;

    if (xr_bluedroid_get_status() != XR_BLUEDROID_STATUS_ENABLED) {
        return XR_ERR_INVALID_STATE;
    }
    if (!device_info && type <= XR_BLE_DUPLICATE_SCAN_EXCEPTIONAL_INFO_MESH_LINK_ID) {
        return XR_ERR_INVALID_SIZE;
    }
    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_GAP_BLE;
    msg.act = BTC_GAP_BLE_UPDATE_DUPLICATE_SCAN_EXCEPTIONAL_LIST;
    arg.update_duplicate_exceptional_list.subcode = XR_BLE_DUPLICATE_EXCEPTIONAL_LIST_REMOVE;
    arg.update_duplicate_exceptional_list.info_type = type;
    if (device_info) {
        memcpy(arg.update_duplicate_exceptional_list.device_info, device_info, sizeof(xr_bd_addr_t));
    }

    return (btc_transfer_context(&msg, &arg, sizeof(btc_ble_gap_args_t), NULL)
                == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
}

xr_err_t xr_ble_gap_clean_duplicate_scan_exceptional_list(xr_duplicate_scan_exceptional_list_type_t list_type)
{
    btc_msg_t msg;
    btc_ble_gap_args_t arg;

    if (xr_bluedroid_get_status() != XR_BLUEDROID_STATUS_ENABLED) {
        return XR_ERR_INVALID_STATE;
    }

    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_GAP_BLE;
    msg.act = BTC_GAP_BLE_UPDATE_DUPLICATE_SCAN_EXCEPTIONAL_LIST;
    arg.update_duplicate_exceptional_list.subcode = XR_BLE_DUPLICATE_EXCEPTIONAL_LIST_CLEAN;
    arg.update_duplicate_exceptional_list.info_type = list_type;

    return (btc_transfer_context(&msg, &arg, sizeof(btc_ble_gap_args_t), NULL)
                == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
}

#if (SMP_INCLUDED == TRUE)
xr_err_t xr_ble_gap_set_security_param(xr_ble_sm_param_t param_type,
        void *value, uint8_t len)
{
    if(param_type >= XR_BLE_SM_MAX_PARAM) {
        return XR_ERR_INVALID_ARG;
    }
    if((param_type != XR_BLE_SM_CLEAR_STATIC_PASSKEY) && ( value == NULL || len < sizeof(uint8_t) || len > sizeof(uint32_t))) {
        return XR_ERR_INVALID_ARG;
    }
    if((param_type == XR_BLE_SM_SET_STATIC_PASSKEY)) {
        uint32_t passkey = 0;
        for(uint8_t i = 0; i < len; i++)
        {
            passkey += (((uint8_t *)value)[i]<<(8*i));
        }
        if(passkey > 999999) {
            return XR_ERR_INVALID_ARG;
        }
    }

    btc_msg_t msg;
    btc_ble_gap_args_t arg;

    XR_BLUEDROID_STATUS_CHECK(XR_BLUEDROID_STATUS_ENABLED);

    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_GAP_BLE;
    msg.act = BTC_GAP_BLE_SET_SECURITY_PARAM_EVT;
    arg.set_security_param.param_type = param_type;
    arg.set_security_param.len = len;
    arg.set_security_param.value = value;

    return (btc_transfer_context(&msg, &arg, sizeof(btc_ble_gap_args_t), btc_gap_ble_arg_deep_copy)
            == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
}

xr_err_t xr_ble_set_encryption(xr_bd_addr_t bd_addr, xr_ble_sec_act_t sec_act)
{
    btc_msg_t msg;
    btc_ble_gap_args_t arg;

    XR_BLUEDROID_STATUS_CHECK(XR_BLUEDROID_STATUS_ENABLED);

    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_GAP_BLE;
    msg.act = BTC_GAP_BLE_SET_ENCRYPTION_EVT;
    arg.set_encryption.sec_act = sec_act;
    memcpy(arg.set_encryption.bd_addr, bd_addr, XR_BD_ADDR_LEN);

    return (btc_transfer_context(&msg, &arg, sizeof(btc_ble_gap_args_t), NULL)
            == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
}

xr_err_t xr_ble_gap_security_rsp(xr_bd_addr_t bd_addr, bool accept)
{
    btc_msg_t msg;
    btc_ble_gap_args_t arg;

    XR_BLUEDROID_STATUS_CHECK(XR_BLUEDROID_STATUS_ENABLED);

    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_GAP_BLE;
    msg.act = BTC_GAP_BLE_SECURITY_RSP_EVT;
    arg.sec_rsp.accept = accept;
    memcpy(arg.sec_rsp.bd_addr, bd_addr, XR_BD_ADDR_LEN);

    return (btc_transfer_context(&msg, &arg, sizeof(btc_ble_gap_args_t), NULL)
            == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);

}

xr_err_t xr_ble_passkey_reply(xr_bd_addr_t bd_addr, bool accept, uint32_t passkey)
{
    btc_msg_t msg;
    btc_ble_gap_args_t arg;

    XR_BLUEDROID_STATUS_CHECK(XR_BLUEDROID_STATUS_ENABLED);

    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_GAP_BLE;
    msg.act = BTC_GAP_BLE_PASSKEY_REPLY_EVT;
    arg.enc_passkey_replay.accept = accept;
    arg.enc_passkey_replay.passkey = passkey;
    memcpy(arg.enc_passkey_replay.bd_addr, bd_addr, XR_BD_ADDR_LEN);

    return (btc_transfer_context(&msg, &arg, sizeof(btc_ble_gap_args_t), NULL)
            == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
}

xr_err_t xr_ble_confirm_reply(xr_bd_addr_t bd_addr, bool accept)
{
    btc_msg_t msg;
    btc_ble_gap_args_t arg;

    XR_BLUEDROID_STATUS_CHECK(XR_BLUEDROID_STATUS_ENABLED);

    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_GAP_BLE;
    msg.act = BTC_GAP_BLE_CONFIRM_REPLY_EVT;
    arg.enc_comfirm_replay.accept = accept;
    memcpy(arg.enc_comfirm_replay.bd_addr, bd_addr, XR_BD_ADDR_LEN);

    return (btc_transfer_context(&msg, &arg, sizeof(btc_ble_gap_args_t), NULL)
            == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
}

xr_err_t xr_ble_remove_bond_device(xr_bd_addr_t bd_addr)
{
    btc_msg_t msg;
    btc_ble_gap_args_t arg;
    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_GAP_BLE;
    msg.act = BTC_GAP_BLE_REMOVE_BOND_DEV_EVT;
    memcpy(arg.remove_bond_device.bd_addr, bd_addr, XR_BD_ADDR_LEN);

    return (btc_transfer_context(&msg, &arg, sizeof(btc_ble_gap_args_t), NULL)
            == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
}

int xr_ble_get_bond_device_num(void)
{
    if (xr_bluedroid_get_status() != XR_BLUEDROID_STATUS_ENABLED) {
        return XR_FAIL;
    }

    return btc_storage_get_num_ble_bond_devices();
}

xr_err_t xr_ble_get_bond_device_list(int *dev_num, xr_ble_bond_dev_t *dev_list)
{
    int ret;
    int dev_num_total;

    if (dev_num == NULL || dev_list == NULL) {
        return XR_ERR_INVALID_ARG;
    }

    XR_BLUEDROID_STATUS_CHECK(XR_BLUEDROID_STATUS_ENABLED);

    dev_num_total = btc_storage_get_num_ble_bond_devices();
    if (*dev_num > dev_num_total) {
        *dev_num = dev_num_total;
    }

    ret = btc_storage_get_bonded_ble_devices_list(dev_list, *dev_num);

    return (ret == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
}

xr_err_t xr_ble_oob_req_reply(xr_bd_addr_t bd_addr, uint8_t *TK, uint8_t len)
{
    if(len != XR_BT_OCTET16_LEN) {
        return XR_ERR_INVALID_ARG;
    }

    btc_msg_t msg;
    btc_ble_gap_args_t arg;

    XR_BLUEDROID_STATUS_CHECK(XR_BLUEDROID_STATUS_ENABLED);

    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_GAP_BLE;
    msg.act = BTC_GAP_BLE_OOB_REQ_REPLY_EVT;
    memcpy(arg.oob_req_reply.bd_addr, bd_addr, XR_BD_ADDR_LEN);
    arg.oob_req_reply.len = len;
    arg.oob_req_reply.p_value = TK;

    return (btc_transfer_context(&msg, &arg, sizeof(btc_ble_gap_args_t), btc_gap_ble_arg_deep_copy)
            == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
}

#endif /* #if (SMP_INCLUDED == TRUE) */

xr_err_t xr_ble_gap_disconnect(xr_bd_addr_t remote_device)
{
    btc_msg_t msg;
    btc_ble_gap_args_t arg;

    XR_BLUEDROID_STATUS_CHECK(XR_BLUEDROID_STATUS_ENABLED);

    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_GAP_BLE;
    msg.act = BTC_GAP_BLE_DISCONNECT_EVT;
    memcpy(arg.disconnect.remote_device, remote_device, XR_BD_ADDR_LEN);

    return (btc_transfer_context(&msg, &arg, sizeof(btc_ble_gap_args_t), NULL) == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
}

xr_err_t xr_ble_get_current_conn_params(xr_bd_addr_t bd_addr, xr_gap_conn_params_t *conn_params)
{
    if(!bd_addr || !conn_params) {
        return XR_ERR_INVALID_ARG;
    }
    if(BTM_GetCurrentConnParams(bd_addr, &conn_params->interval, &conn_params->latency, &conn_params->timeout)) {
        return XR_OK;
    }
    return XR_ERR_NOT_FOUND;
}

xr_err_t xr_gap_ble_set_channels(xr_gap_ble_channels channels)
{
    btc_msg_t msg;
    btc_ble_gap_args_t arg;

    if (xr_bluedroid_get_status() != XR_BLUEDROID_STATUS_ENABLED) {
        return XR_ERR_INVALID_STATE;
    }

    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_GAP_BLE;
    msg.act = BTC_GAP_BLE_SET_AFH_CHANNELS;

    memcpy(&arg.set_channels.channels, channels, XR_GAP_BLE_CHANNELS_LEN);
    arg.set_channels.channels[XR_GAP_BLE_CHANNELS_LEN -1] &= 0x1F;
    return (btc_transfer_context(&msg, &arg, sizeof(btc_ble_gap_args_t), NULL) == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
}

xr_err_t xr_gap_ble_set_authorization(xr_bd_addr_t bd_addr, bool authorize)
{
    if (!bd_addr) {
        return XR_ERR_INVALID_ARG;
    }
    if (BTM_Ble_Authorization(bd_addr, authorize)) {
        return XR_OK;
    }
    return XR_FAIL;
}
