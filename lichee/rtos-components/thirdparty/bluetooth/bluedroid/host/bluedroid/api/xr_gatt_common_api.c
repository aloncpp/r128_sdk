// Copyright 2015-2017 Espressif Systems (Shanghai) PTE LTD
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
#include "xr_gatt_common_api.h"
#include "xr_bt_main.h"
#include "xr_gatt_defs.h"
#include "btc_gatt_common.h"

/**
 * @brief           This function is called to set local MTU,
 *                  the function is called before BLE connection.
 *
 * @param[in]       mtu: the size of MTU.
 *
 * @return
 *                  - XR_OK: success
 *                  - other: failed
 *
 */
xr_err_t xr_ble_gatt_set_local_mtu (uint16_t mtu)
{
    btc_msg_t msg;
    btc_ble_gatt_com_args_t arg;

    XR_BLUEDROID_STATUS_CHECK(XR_BLUEDROID_STATUS_ENABLED);

    if ((mtu < XR_GATT_DEF_BLE_MTU_SIZE) || (mtu > XR_GATT_MAX_MTU_SIZE)) {
        return XR_ERR_INVALID_SIZE;
    }

    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_GATT_COMMON;
    msg.act = BTC_GATT_ACT_SET_LOCAL_MTU;
    arg.set_mtu.mtu = mtu;

    return (btc_transfer_context(&msg, &arg, sizeof(btc_ble_gatt_com_args_t), NULL) == BT_STATUS_SUCCESS ? XR_OK : XR_FAIL);
}

#if (BLE_INCLUDED == TRUE)
extern UINT16 L2CA_GetFreePktBufferNum_LE(void);

/**
 * @brief           This function is called to get currently sendable packets number on controller,
 *                  the function is called only in BLE running core and single connection now.
 *
 * @return
 *                  sendable packets number on controller
 *
 */
 
uint16_t xr_ble_get_sendable_packets_num (void)
{
    return L2CA_GetFreePktBufferNum_LE();
}

/**
 * @brief           This function is used to query the number of available buffers for the current connection.
 *                  When you need to query the current available buffer number, it is recommended to use this API.
 * @param[in]       conn_id: current connection id.
 * 
 * @return
 *                  Number of available buffers for the current connection
 *
 */
 
extern UINT16 L2CA_GetCurFreePktBufferNum_LE(UINT16 conn_id);
uint16_t xr_ble_get_cur_sendable_packets_num (uint16_t connid)
{
    return L2CA_GetCurFreePktBufferNum_LE(connid);
}
#endif