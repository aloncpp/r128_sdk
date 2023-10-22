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

#ifndef __XR_BT_DEVICE_H__
#define __XR_BT_DEVICE_H__

#include <stdint.h>
#include <stdbool.h>
#include "xr_err.h"
#include "xr_bt_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 *
 * @brief      Get bluetooth device address.  Must use after "xr_bluedroid_enable".
 *
 * @return     bluetooth device address (six bytes), or NULL if bluetooth stack is not enabled
 */
const uint8_t *xr_bt_dev_get_address(void);


/**
 * @brief           Set bluetooth device name. This function should be called after xr_bluedroid_enable()
 *                  completes successfully.
 *                  A BR/EDR/LE device type shall have a single Bluetooth device name which shall be
 *                  identical irrespective of the physical channel used to perform the name discovery procedure.
 *
 * @param[in]       name : device name to be set
 *
 * @return
 *                  - XR_OK : Succeed
 *                  - XR_ERR_INVALID_ARG : if name is NULL pointer or empty, or string length out of limit
 *                  - XR_ERR_INVALID_STATE : if bluetooth stack is not yet enabled
 *                  - XR_FAIL : others
 */
xr_err_t xr_bt_dev_set_device_name(const char *name);

/**
 * @brief           Get bluetooth device name. This function should be called after xr_bluedroid_enable()
 *                  completes successfully.
 *
 * @return
 *                  - XR_OK : Succeed
 *                  - XR_ERR_INVALID_ARG : if name is NULL pointer or empty, or string length out of limit
 *                  - XR_ERR_INVALID_STATE : if bluetooth stack is not yet enabled
 *                  - XR_FAIL : others
 */
xr_err_t xr_bt_dev_get_device_name(void);
#ifdef __cplusplus
}
#endif


#endif /* __XR_BT_DEVICE_H__ */
