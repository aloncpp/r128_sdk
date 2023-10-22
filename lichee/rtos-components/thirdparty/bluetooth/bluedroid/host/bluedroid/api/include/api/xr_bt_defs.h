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

#ifndef __XR_BT_DEFS_H__
#define __XR_BT_DEFS_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define XR_BLUEDROID_STATUS_CHECK(status)           \
    if (xr_bluedroid_get_status() != (status)) {    \
        return XR_ERR_INVALID_STATE;                \
    }


/* relate to BT_STATUS_xxx in bt_def.h */
/// Status Return Value
typedef enum {
    XR_BT_STATUS_SUCCESS       = 0,            /* relate to BT_STATUS_SUCCESS in bt_def.h */
    XR_BT_STATUS_FAIL,                         /* relate to BT_STATUS_FAIL in bt_def.h */
    XR_BT_STATUS_NOT_READY,                    /* relate to BT_STATUS_NOT_READY in bt_def.h */
    XR_BT_STATUS_NOMEM,                        /* relate to BT_STATUS_NOMEM in bt_def.h */
    XR_BT_STATUS_BUSY,                         /* relate to BT_STATUS_BUSY in bt_def.h */
    XR_BT_STATUS_DONE          = 5,            /* relate to BT_STATUS_DONE in bt_def.h */
    XR_BT_STATUS_UNSUPPORTED,                  /* relate to BT_STATUS_UNSUPPORTED in bt_def.h */
    XR_BT_STATUS_PARM_INVALID,                 /* relate to BT_STATUS_PARM_INVALID in bt_def.h */
    XR_BT_STATUS_UNHANDLED,                    /* relate to BT_STATUS_UNHANDLED in bt_def.h */
    XR_BT_STATUS_AUTH_FAILURE,                 /* relate to BT_STATUS_AUTH_FAILURE in bt_def.h */
    XR_BT_STATUS_RMT_DEV_DOWN  = 10,           /* relate to BT_STATUS_RMT_DEV_DOWN in bt_def.h */
    XR_BT_STATUS_AUTH_REJECTED,                /* relate to BT_STATUS_AUTH_REJECTED in bt_def.h */
    XR_BT_STATUS_INVALID_STATIC_RAND_ADDR,     /* relate to BT_STATUS_INVALID_STATIC_RAND_ADDR in bt_def.h */
    XR_BT_STATUS_PENDING,                      /* relate to BT_STATUS_PENDING in bt_def.h */
    XR_BT_STATUS_UNACCEPT_CONN_INTERVAL,       /* relate to BT_UNACCEPT_CONN_INTERVAL in bt_def.h */
    XR_BT_STATUS_PARAM_OUT_OF_RANGE,           /* relate to BT_PARAM_OUT_OF_RANGE in bt_def.h */
    XR_BT_STATUS_TIMEOUT,                      /* relate to BT_STATUS_TIMEOUT in bt_def.h */
    XR_BT_STATUS_PEER_LE_DATA_LEN_UNSUPPORTED, /* relate to BTM_PEER_LE_DATA_LEN_UNSUPPORTED in stack/btm_api.h */
    XR_BT_STATUS_CONTROL_LE_DATA_LEN_UNSUPPORTED,/* relate to BTM_CONTROL_LE_DATA_LEN_UNSUPPORTED in stack/btm_api.h */
    XR_BT_STATUS_ERR_ILLEGAL_PARAMETER_FMT,    /* relate to HCI_ERR_ILLEGAL_PARAMETER_FMT in stack/hcidefs.h */
    XR_BT_STATUS_MEMORY_FULL   = 20,           /* relate to BT_STATUS_MEMORY_FULL in bt_def.h */
    XR_BT_STATUS_EIR_TOO_LARGE,                /* relate to BT_STATUS_EIR_TOO_LARGE in bt_def.h */
} xr_bt_status_t;


/*Define the bt octet 16 bit size*/
#define XR_BT_OCTET16_LEN    16
typedef uint8_t xr_bt_octet16_t[XR_BT_OCTET16_LEN];   /* octet array: size 16 */

#define XR_BT_OCTET8_LEN    8
typedef uint8_t xr_bt_octet8_t[XR_BT_OCTET8_LEN];   /* octet array: size 8 */

typedef uint8_t xr_link_key[XR_BT_OCTET16_LEN];      /* Link Key */

/// Default GATT interface id
#define XR_DEFAULT_GATT_IF             0xff

#define XR_BLE_CONN_INT_MIN                0x0006       /*!< relate to BTM_BLE_CONN_INT_MIN in stack/btm_ble_api.h */
#define XR_BLE_CONN_INT_MAX                0x0C80       /*!< relate to BTM_BLE_CONN_INT_MAX in stack/btm_ble_api.h */
#define XR_BLE_CONN_LATENCY_MAX            499          /*!< relate to XR_BLE_CONN_LATENCY_MAX in stack/btm_ble_api.h */
#define XR_BLE_CONN_SUP_TOUT_MIN           0x000A       /*!< relate to BTM_BLE_CONN_SUP_TOUT_MIN in stack/btm_ble_api.h */
#define XR_BLE_CONN_SUP_TOUT_MAX           0x0C80       /*!< relate to XR_BLE_CONN_SUP_TOUT_MAX in stack/btm_ble_api.h */
#define XR_BLE_CONN_PARAM_UNDEF            0xffff       /* use this value when a specific value not to be overwritten */ /* relate to XR_BLE_CONN_PARAM_UNDEF in stack/btm_ble_api.h */
#define XR_BLE_SCAN_PARAM_UNDEF            0xffffffff   /* relate to XR_BLE_SCAN_PARAM_UNDEF in stack/btm_ble_api.h */

/// Check the param is valid or not
#define XR_BLE_IS_VALID_PARAM(x, min, max)  (((x) >= (min) && (x) <= (max)) || ((x) == XR_BLE_CONN_PARAM_UNDEF))

/// UUID type
typedef struct {
#define XR_UUID_LEN_16     2
#define XR_UUID_LEN_32     4
#define XR_UUID_LEN_128    16
    uint16_t len;							/*!< UUID length, 16bit, 32bit or 128bit */
    union {
        uint16_t    uuid16;                 /*!< 16bit UUID */
        uint32_t    uuid32;                 /*!< 32bit UUID */
        uint8_t     uuid128[XR_UUID_LEN_128]; /*!< 128bit UUID */
    } uuid;									/*!< UUID */
} __attribute__((packed)) xr_bt_uuid_t;

/// Bluetooth device type
typedef enum {
    XR_BT_DEVICE_TYPE_BREDR   = 0x01,
    XR_BT_DEVICE_TYPE_BLE     = 0x02,
    XR_BT_DEVICE_TYPE_DUMO    = 0x03,
} xr_bt_dev_type_t;

/// Bluetooth address length
#define XR_BD_ADDR_LEN     6

/// Bluetooth device address
typedef uint8_t xr_bd_addr_t[XR_BD_ADDR_LEN];

/// Bluetooth remote dev_name length
#ifndef BD_NAME_LEN
#ifdef CONFIG_BREDR_DEVICE_NAME_MAX
#define BD_NAME_LEN    CONFIG_BREDR_DEVICE_NAME_MAX
#else
#define BD_NAME_LEN    (248)
#endif
#endif

/// Bluetooth remote dev_name address
typedef uint8_t xr_bd_name_t[BD_NAME_LEN];

/// BLE device address type
typedef enum {
    BLE_ADDR_TYPE_PUBLIC        = 0x00,
    BLE_ADDR_TYPE_RANDOM        = 0x01,
    BLE_ADDR_TYPE_RPA_PUBLIC    = 0x02,
    BLE_ADDR_TYPE_RPA_RANDOM    = 0x03,
} xr_ble_addr_type_t;

/// white list address type
typedef enum {
    BLE_WL_ADDR_TYPE_PUBLIC        = 0x00,
    BLE_WL_ADDR_TYPE_RANDOM        = 0x01,
} xr_ble_wl_addr_type_t;

/// Used to exchange the encryption key in the init key & response key
#define XR_BLE_ENC_KEY_MASK    (1 << 0)            /* relate to BTM_BLE_ENC_KEY_MASK in stack/btm_api.h */
/// Used to exchange the IRK key in the init key & response key
#define XR_BLE_ID_KEY_MASK     (1 << 1)            /* relate to BTM_BLE_ID_KEY_MASK in stack/btm_api.h */
/// Used to exchange the CSRK key in the init key & response key
#define XR_BLE_CSR_KEY_MASK    (1 << 2)            /* relate to BTM_BLE_CSR_KEY_MASK in stack/btm_api.h */
/// Used to exchange the link key(this key just used in the BLE & BR/EDR coexist mode) in the init key & response key
#define XR_BLE_LINK_KEY_MASK   (1 << 3)            /* relate to BTM_BLE_LINK_KEY_MASK in stack/btm_api.h */
typedef uint8_t xr_ble_key_mask_t;            /* the key mask type */

/// Minimum of the application id
#define XR_APP_ID_MIN  0x0000
/// Maximum of the application id
#define XR_APP_ID_MAX  0x7fff

#define XR_BD_ADDR_STR         "%02x:%02x:%02x:%02x:%02x:%02x"
#define XR_BD_ADDR_HEX(addr)   addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]

#ifdef __cplusplus
}
#endif

#endif     /* __XR_BT_DEFS_H__ */
