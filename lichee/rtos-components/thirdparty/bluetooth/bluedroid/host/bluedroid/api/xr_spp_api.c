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

#include "xr_bt_main.h"
#include "btc/btc_manage.h"

#include "btc_spp.h"
#include "xr_spp_api.h"
#include "common/bt_target.h"

#if (defined BTC_SPP_INCLUDED && BTC_SPP_INCLUDED == TRUE)

static const uint8_t UUID_SPP[16] = {0x00, 0x00, 0x11, 0x01, 0x00, 0x00, 0x10, 0x00,
                                    0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB
                                    };
static tSDP_UUID sdp_uuid;
xr_err_t xr_spp_register_callback(xr_spp_cb_t *callback)
{
    XR_BLUEDROID_STATUS_CHECK(XR_BLUEDROID_STATUS_ENABLED);

    if (callback == NULL) {
        return XR_FAIL;
    }

    btc_profile_cb_set(BTC_PID_SPP, callback);
    return XR_OK;
}


xr_err_t xr_spp_init(xr_spp_mode_t mode)
{
    btc_msg_t msg;
    btc_spp_args_t arg;
    XR_BLUEDROID_STATUS_CHECK(XR_BLUEDROID_STATUS_ENABLED);

    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_SPP;
    msg.act = BTC_SPP_ACT_INIT;

    arg.init.mode = mode;
    return (btc_transfer_context(&msg, &arg, sizeof(btc_spp_args_t), NULL) == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
}

xr_err_t xr_spp_deinit(void)
{
    btc_msg_t msg;
    btc_spp_args_t arg;
    XR_BLUEDROID_STATUS_CHECK(XR_BLUEDROID_STATUS_ENABLED);

    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_SPP;
    msg.act = BTC_SPP_ACT_UNINIT;

    return (btc_transfer_context(&msg, &arg, sizeof(btc_spp_args_t), NULL) == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
}


xr_err_t xr_spp_start_discovery(xr_bd_addr_t bd_addr)
{
    sdp_uuid.len = 16;
    memcpy(sdp_uuid.uu.uuid128, UUID_SPP, sizeof(sdp_uuid.uu.uuid128));

    btc_msg_t msg;
    btc_spp_args_t arg;
    XR_BLUEDROID_STATUS_CHECK(XR_BLUEDROID_STATUS_ENABLED);

    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_SPP;
    msg.act = BTC_SPP_ACT_START_DISCOVERY;

    memcpy(arg.start_discovery.bd_addr, bd_addr, XR_BD_ADDR_LEN);
    arg.start_discovery.num_uuid = 1;
    arg.start_discovery.p_uuid_list = &sdp_uuid;

    return (btc_transfer_context(&msg, &arg, sizeof(btc_spp_args_t), btc_spp_arg_deep_copy) == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
}

xr_err_t xr_spp_connect(xr_spp_sec_t sec_mask,
                          xr_spp_role_t role, uint8_t remote_scn, xr_bd_addr_t peer_bd_addr)
{
    btc_msg_t msg;
    btc_spp_args_t arg;
    XR_BLUEDROID_STATUS_CHECK(XR_BLUEDROID_STATUS_ENABLED);

    if (sec_mask != XR_SPP_SEC_NONE && sec_mask != XR_SPP_SEC_AUTHORIZE && sec_mask != XR_SPP_SEC_AUTHENTICATE) {
        LOG_WARN("Suggest to use XR_SPP_SEC_NONE, XR_SPP_SEC_AUTHORIZE or XR_SPP_SEC_AUTHENTICATE only\n");
    }

    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_SPP;
    msg.act = BTC_SPP_ACT_CONNECT;

    arg.connect.sec_mask = sec_mask;
    arg.connect.role = role;
    arg.connect.remote_scn = remote_scn;
    memcpy(arg.connect.peer_bd_addr, peer_bd_addr, XR_BD_ADDR_LEN);

    return (btc_transfer_context(&msg, &arg, sizeof(btc_spp_args_t), NULL) == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
}

xr_err_t xr_spp_disconnect(uint32_t handle)
{
    btc_msg_t msg;
    btc_spp_args_t arg;
    XR_BLUEDROID_STATUS_CHECK(XR_BLUEDROID_STATUS_ENABLED);

    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_SPP;
    msg.act = BTC_SPP_ACT_DISCONNECT;

    arg.disconnect.handle = handle;

    return (btc_transfer_context(&msg, &arg, sizeof(btc_spp_args_t), NULL) == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
}

xr_err_t xr_spp_start_srv(xr_spp_sec_t sec_mask,
                            xr_spp_role_t role, uint8_t local_scn, const char *name)
{
    btc_msg_t msg;
    btc_spp_args_t arg;
    XR_BLUEDROID_STATUS_CHECK(XR_BLUEDROID_STATUS_ENABLED);

    if (name == NULL || strlen(name) > XR_SPP_SERVER_NAME_MAX) {
        LOG_ERROR("Invalid server name!\n");
        return XR_ERR_INVALID_ARG;
    }

    if (sec_mask != XR_SPP_SEC_NONE && sec_mask != XR_SPP_SEC_AUTHORIZE && sec_mask != XR_SPP_SEC_AUTHENTICATE) {
        LOG_WARN("Suggest to use XR_SPP_SEC_NONE, XR_SPP_SEC_AUTHORIZE or XR_SPP_SEC_AUTHENTICATE only\n");
    }

    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_SPP;
    msg.act = BTC_SPP_ACT_START_SRV;

    arg.start_srv.sec_mask = sec_mask;
    arg.start_srv.role = role;
    arg.start_srv.local_scn = local_scn;
    arg.start_srv.max_session = XR_SPP_MAX_SESSION;
    strcpy(arg.start_srv.name, name);

    return (btc_transfer_context(&msg, &arg, sizeof(btc_spp_args_t), NULL) == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
}

xr_err_t xr_spp_stop_srv(void)
{
    btc_msg_t msg;
    btc_spp_args_t arg;
    XR_BLUEDROID_STATUS_CHECK(XR_BLUEDROID_STATUS_ENABLED);

    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_SPP;
    msg.act = BTC_SPP_ACT_STOP_SRV;
    arg.stop_srv.scn = BTC_SPP_INVALID_SCN;

    return (btc_transfer_context(&msg, &arg, sizeof(btc_spp_args_t), NULL) == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
}

xr_err_t xr_spp_stop_srv_scn(uint8_t scn)
{
    btc_msg_t msg;
    btc_spp_args_t arg;
    XR_BLUEDROID_STATUS_CHECK(XR_BLUEDROID_STATUS_ENABLED);

    if ((scn == 0) || (scn >= PORT_MAX_RFC_PORTS)) {
        LOG_ERROR("Invalid SCN!\n");
        return XR_ERR_INVALID_ARG;
    }

    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_SPP;
    msg.act = BTC_SPP_ACT_STOP_SRV;
    arg.stop_srv.scn = scn;

    return (btc_transfer_context(&msg, &arg, sizeof(btc_spp_args_t), NULL) == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
}


xr_err_t xr_spp_write(uint32_t handle, int len, uint8_t *p_data)
{
    btc_msg_t msg;
    btc_spp_args_t arg;
    XR_BLUEDROID_STATUS_CHECK(XR_BLUEDROID_STATUS_ENABLED);

    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_SPP;
    msg.act = BTC_SPP_ACT_WRITE;

    arg.write.handle = handle;
    arg.write.len = len;
    arg.write.p_data = p_data;

    return (btc_transfer_context(&msg, &arg, sizeof(btc_spp_args_t), btc_spp_arg_deep_copy) == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
}

xr_err_t xr_spp_vfs_register(void)
{
    XR_BLUEDROID_STATUS_CHECK(XR_BLUEDROID_STATUS_ENABLED);

    return btc_spp_vfs_register();
}

#endif ///defined BTC_SPP_INCLUDED && BTC_SPP_INCLUDED == TRUE
