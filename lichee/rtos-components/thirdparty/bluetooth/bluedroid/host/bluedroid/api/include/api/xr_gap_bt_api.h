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

#ifndef __XR_GAP_BT_API_H__
#define __XR_GAP_BT_API_H__

#include <stdint.h>
#include "xr_err.h"
#include "xr_bt_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/// RSSI threshold
#define XR_BT_GAP_RSSI_HIGH_THRLD  -20             /*!< High RSSI threshold */
#define XR_BT_GAP_RSSI_LOW_THRLD   -45             /*!< Low RSSI threshold */

/// Class of device
typedef struct {
    uint32_t      reserved_2: 2;                    /*!< undefined */
    uint32_t      minor: 6;                         /*!< minor class */
    uint32_t      major: 5;                         /*!< major class */
    uint32_t      service: 11;                      /*!< service class */
    uint32_t      reserved_8: 8;                    /*!< undefined */
} xr_bt_cod_t;

/// class of device settings
typedef enum {
    XR_BT_SET_COD_MAJOR_MINOR     = 0x01,          /*!< overwrite major, minor class */
    XR_BT_SET_COD_SERVICE_CLASS   = 0x02,          /*!< set the bits in the input, the current bit will remain */
    XR_BT_CLR_COD_SERVICE_CLASS   = 0x04,          /*!< clear the bits in the input, others will remain */
    XR_BT_SET_COD_ALL             = 0x08,          /*!< overwrite major, minor, set the bits in service class */
    XR_BT_INIT_COD                = 0x0a,          /*!< overwrite major, minor, and service class */
} xr_bt_cod_mode_t;

#define XR_BT_GAP_AFH_CHANNELS_LEN     10
typedef uint8_t xr_bt_gap_afh_channels[XR_BT_GAP_AFH_CHANNELS_LEN];


/// Discoverability and Connectability mode
typedef enum {
    XR_BT_NON_CONNECTABLE,             /*!< Non-connectable */
    XR_BT_CONNECTABLE,                 /*!< Connectable */
} xr_bt_connection_mode_t;

typedef enum {
    XR_BT_NON_DISCOVERABLE,            /*!< Non-discoverable */
    XR_BT_LIMITED_DISCOVERABLE,        /*!< Limited Discoverable */
    XR_BT_GENERAL_DISCOVERABLE,        /*!< General Discoverable */
} xr_bt_discovery_mode_t;

typedef enum {
    XR_BT_NON_PAIRABLE,                /*!< Non-pairable */
    XR_BT_PAIRABLE,                    /*!< pairable */
} xr_bt_paired_mode_t;

typedef enum {
    XR_BT_ALL_PAIRABLE,                /*!< pair with any devices */
    XR_BT_ONLY_CON_PAIRABLE,           /*!< only pair with connected devices */
} xr_bt_paired_con_mode_t;

/// Bluetooth Device Property type
typedef enum {
    XR_BT_GAP_DEV_PROP_BDNAME = 1,                 /*!< Bluetooth device name, value type is int8_t [] */
    XR_BT_GAP_DEV_PROP_COD,                        /*!< Class of Device, value type is uint32_t */
    XR_BT_GAP_DEV_PROP_RSSI,                       /*!< Received Signal strength Indication, value type is int8_t, ranging from -128 to 127 */
    XR_BT_GAP_DEV_PROP_EIR,                        /*!< Extended Inquiry Response, value type is uint8_t [] */
} xr_bt_gap_dev_prop_type_t;

/// Maximum bytes of Bluetooth device name
#define XR_BT_GAP_MAX_BDNAME_LEN             (248)

/// Maximum size of EIR Significant part
#define XR_BT_GAP_EIR_DATA_LEN               (240)

/// Bluetooth Device Property Descriptor
typedef struct {
    xr_bt_gap_dev_prop_type_t type;                /*!< device property type */
    int len;                                        /*!< device property value length */
    void *val;                                      /*!< device property value */
} xr_bt_gap_dev_prop_t;

/// Extended Inquiry Response data type
#define XR_BT_EIR_TYPE_FLAGS                   0x01      /*!< Flag with information such as BR/EDR and LE support */
#define XR_BT_EIR_TYPE_INCMPL_16BITS_UUID      0x02      /*!< Incomplete list of 16-bit service UUIDs */
#define XR_BT_EIR_TYPE_CMPL_16BITS_UUID        0x03      /*!< Complete list of 16-bit service UUIDs */
#define XR_BT_EIR_TYPE_INCMPL_32BITS_UUID      0x04      /*!< Incomplete list of 32-bit service UUIDs */
#define XR_BT_EIR_TYPE_CMPL_32BITS_UUID        0x05      /*!< Complete list of 32-bit service UUIDs */
#define XR_BT_EIR_TYPE_INCMPL_128BITS_UUID     0x06      /*!< Incomplete list of 128-bit service UUIDs */
#define XR_BT_EIR_TYPE_CMPL_128BITS_UUID       0x07      /*!< Complete list of 128-bit service UUIDs */
#define XR_BT_EIR_TYPE_SHORT_LOCAL_NAME        0x08      /*!< Shortened Local Name */
#define XR_BT_EIR_TYPE_CMPL_LOCAL_NAME         0x09      /*!< Complete Local Name */
#define XR_BT_EIR_TYPE_TX_POWER_LEVEL          0x0a      /*!< Tx power level, value is 1 octet ranging from  -127 to 127, unit is dBm*/
#define XR_BT_EIR_TYPE_URL                     0x24      /*!< Uniform resource identifier */
#define XR_BT_EIR_TYPE_MANU_SPECIFIC           0xff      /*!< Manufacturer specific data */
#define  XR_BT_EIR_TYPE_MAX_NUM                12        /*!< MAX number of EIR type */

typedef uint8_t xr_bt_eir_type_t;



/* XR_BT_EIR_FLAG bit definition */
#define XR_BT_EIR_FLAG_LIMIT_DISC         (0x01 << 0)
#define XR_BT_EIR_FLAG_GEN_DISC           (0x01 << 1)
#define XR_BT_EIR_FLAG_BREDR_NOT_SPT      (0x01 << 2)
#define XR_BT_EIR_FLAG_DMT_CONTROLLER_SPT (0x01 << 3)
#define XR_BT_EIR_FLAG_DMT_HOST_SPT       (0x01 << 4)

#define XR_BT_EIR_MAX_LEN                  240
/// EIR data content, according to "Supplement to the Bluetooth Core Specification"
typedef struct {
    bool                    fec_required;           /*!< FEC is required or not, true by default */
    bool                    include_txpower;        /*!< EIR data include TX power, false by default */
    bool                    include_uuid;           /*!< EIR data include UUID, false by default */
    uint8_t                 flag;                   /*!< EIR flags, see XR_BT_EIR_FLAG for details, EIR will not include flag if it is 0, 0 by default */
    uint16_t                manufacturer_len;       /*!< Manufacturer data length, 0 by default */
    uint8_t                 *p_manufacturer_data;   /*!< Manufacturer data point */
    uint16_t                url_len;                /*!< URL length, 0 by default */
    uint8_t                 *p_url;                 /*!< URL point */
} xr_bt_eir_data_t;

/// Major service class field of Class of Device, mutiple bits can be set
typedef enum {
    XR_BT_COD_SRVC_NONE                     =     0,    /*!< None indicates an invalid value */
    XR_BT_COD_SRVC_LMTD_DISCOVER            =   0x1,    /*!< Limited Discoverable Mode */
    XR_BT_COD_SRVC_POSITIONING              =   0x8,    /*!< Positioning (Location identification) */
    XR_BT_COD_SRVC_NETWORKING               =  0x10,    /*!< Networking, e.g. LAN, Ad hoc */
    XR_BT_COD_SRVC_RENDERING                =  0x20,    /*!< Rendering, e.g. Printing, Speakers */
    XR_BT_COD_SRVC_CAPTURING                =  0x40,    /*!< Capturing, e.g. Scanner, Microphone */
    XR_BT_COD_SRVC_OBJ_TRANSFER             =  0x80,    /*!< Object Transfer, e.g. v-Inbox, v-Folder */
    XR_BT_COD_SRVC_AUDIO                    = 0x100,    /*!< Audio, e.g. Speaker, Microphone, Headset service */
    XR_BT_COD_SRVC_TELEPHONY                = 0x200,    /*!< Telephony, e.g. Cordless telephony, Modem, Headset service */
    XR_BT_COD_SRVC_INFORMATION              = 0x400,    /*!< Information, e.g., WEB-server, WAP-server */
} xr_bt_cod_srvc_t;

typedef enum{
    XR_BT_PIN_TYPE_VARIABLE = 0,                       /*!< Refer to BTM_PIN_TYPE_VARIABLE */
    XR_BT_PIN_TYPE_FIXED    = 1,                       /*!< Refer to BTM_PIN_TYPE_FIXED */
} xr_bt_pin_type_t;

#define XR_BT_PIN_CODE_LEN        16                   /*!< Max pin code length */
typedef uint8_t xr_bt_pin_code_t[XR_BT_PIN_CODE_LEN]; /*!< Pin Code (upto 128 bits) MSB is 0 */

typedef enum {
    XR_BT_SP_IOCAP_MODE = 0,                            /*!< Set IO mode */
    //XR_BT_SP_OOB_DATA, //TODO                         /*!< Set OOB data */
} xr_bt_sp_param_t;

/* relate to BTM_IO_CAP_xxx in stack/btm_api.h */
#define XR_BT_IO_CAP_OUT                      0        /*!< DisplayOnly */         /* relate to BTM_IO_CAP_OUT in stack/btm_api.h */
#define XR_BT_IO_CAP_IO                       1        /*!< DisplayYesNo */        /* relate to BTM_IO_CAP_IO in stack/btm_api.h */
#define XR_BT_IO_CAP_IN                       2        /*!< KeyboardOnly */        /* relate to BTM_IO_CAP_IN in stack/btm_api.h */
#define XR_BT_IO_CAP_NONE                     3        /*!< NoInputNoOutput */     /* relate to BTM_IO_CAP_NONE in stack/btm_api.h */
typedef uint8_t xr_bt_io_cap_t;                        /*!< combination of the io capability */

/* sniff mode */
typedef enum {
    XR_BT_SNIFF_MODE_OFF                    = 0,        /* sniff mode off*/
    XR_BT_SNIFF_MODE_ON                     = 1,        /* sniff mode on*/
}xr_bt_sniff_mode_t;

/* role mode */
typedef enum {
    XR_BT_ROLE_MASTER                       = 0,    /*role slave*/
    XR_BT_ROLE_SLAVE                        = 1,    /*role master*/
}xr_bt_role_t;

/// Bits of major service class field
#define XR_BT_COD_SRVC_BIT_MASK              (0xffe000) /*!< Major service bit mask */
#define XR_BT_COD_SRVC_BIT_OFFSET            (13)       /*!< Major service bit offset */

/// Major device class field of Class of Device
typedef enum {
    XR_BT_COD_MAJOR_DEV_MISC                = 0,    /*!< Miscellaneous */
    XR_BT_COD_MAJOR_DEV_COMPUTER            = 1,    /*!< Computer */
    XR_BT_COD_MAJOR_DEV_PHONE               = 2,    /*!< Phone(cellular, cordless, pay phone, modem */
    XR_BT_COD_MAJOR_DEV_LAN_NAP             = 3,    /*!< LAN, Network Access Point */
    XR_BT_COD_MAJOR_DEV_AV                  = 4,    /*!< Audio/Video(headset, speaker, stereo, video display, VCR */
    XR_BT_COD_MAJOR_DEV_PERIPHERAL          = 5,    /*!< Peripheral(mouse, joystick, keyboard) */
    XR_BT_COD_MAJOR_DEV_IMAGING             = 6,    /*!< Imaging(printer, scanner, camera, display */
    XR_BT_COD_MAJOR_DEV_WEARABLE            = 7,    /*!< Wearable */
    XR_BT_COD_MAJOR_DEV_TOY                 = 8,    /*!< Toy */
    XR_BT_COD_MAJOR_DEV_HEALTH              = 9,    /*!< Health */
    XR_BT_COD_MAJOR_DEV_UNCATEGORIZED       = 31,   /*!< Uncategorized: device not specified */
} xr_bt_cod_major_dev_t;

/// Bits of major device class field
#define XR_BT_COD_MAJOR_DEV_BIT_MASK         (0x1f00) /*!< Major device bit mask */
#define XR_BT_COD_MAJOR_DEV_BIT_OFFSET       (8)      /*!< Major device bit offset */

/// Bits of minor device class field
#define XR_BT_COD_MINOR_DEV_BIT_MASK         (0xfc)   /*!< Minor device bit mask */
#define XR_BT_COD_MINOR_DEV_BIT_OFFSET       (2)      /*!< Minor device bit offset */

/// Bits of format type
#define XR_BT_COD_FORMAT_TYPE_BIT_MASK       (0x03)   /*!< Format type bit mask */
#define XR_BT_COD_FORMAT_TYPE_BIT_OFFSET     (0)      /*!< Format type bit offset */

/// Class of device format type 1
#define XR_BT_COD_FORMAT_TYPE_1              (0x00)

/** Bluetooth Device Discovery state */
typedef enum {
    XR_BT_GAP_DISCOVERY_STOPPED,                   /*!< device discovery stopped */
    XR_BT_GAP_DISCOVERY_STARTED,                   /*!< device discovery started */
} xr_bt_gap_discovery_state_t;

/// BT GAP callback events
typedef enum {
    XR_BT_GAP_DISC_RES_EVT = 0,                    /*!< device discovery result event */
    XR_BT_GAP_DISC_STATE_CHANGED_EVT,              /*!< discovery state changed event */
    XR_BT_GAP_RMT_SRVCS_EVT,                       /*!< get remote services event */
    XR_BT_GAP_RMT_SRVC_REC_EVT,                    /*!< get remote service record event */
    XR_BT_GAP_AUTH_CMPL_EVT,                       /*!< AUTH complete event */
    XR_BT_GAP_PIN_REQ_EVT,                         /*!< Legacy Pairing Pin code request */
    XR_BT_GAP_CFM_REQ_EVT,                         /*!< Simple Pairing User Confirmation request. */
    XR_BT_GAP_KEY_NOTIF_EVT,                       /*!< Simple Pairing Passkey Notification */
    XR_BT_GAP_KEY_REQ_EVT,                         /*!< Simple Pairing Passkey request */
    XR_BT_GAP_READ_RSSI_DELTA_EVT,                 /*!< read rssi event */
    XR_BT_GAP_CONFIG_EIR_DATA_EVT,                 /*!< config EIR data event */
    XR_BT_GAP_SET_AFH_CHANNELS_EVT,                /*!< set AFH channels event */
    XR_BT_GAP_READ_REMOTE_NAME_EVT,                /*!< read Remote Name event */
    XR_BT_GAP_READ_LOCAL_NAME_EVT,                /*!< read Local Name event */
    XR_BT_GAP_REMOVE_BOND_DEV_COMPLETE_EVT,        /*!< remove bond device complete event */
    XR_BT_GAP_POWER_MODE_EVT,                      /*!< power mode event */
    XR_BT_GAP_SET_ROLE_COMPLETE_EVT,               /*!< set new role mode complete event */
    XR_BT_GAP_GET_ROLE_COMPLETE_EVT,               /*!< get role complete event>*/
    XR_BT_GAP_EVT_MAX,
} xr_bt_gap_cb_event_t;

/** Inquiry Mode */
typedef enum {
    XR_BT_INQ_MODE_GENERAL_INQUIRY,                /*!< General inquiry mode */
    XR_BT_INQ_MODE_LIMITED_INQUIRY,                /*!< Limited inquiry mode */
} xr_bt_inq_mode_t;

/** Minimum and Maximum inquiry length*/
#define XR_BT_GAP_MIN_INQ_LEN                (0x01)  /*!< Minimum inquiry duration, unit is 1.28s */
#define XR_BT_GAP_MAX_INQ_LEN                (0x30)  /*!< Maximum inquiry duration, unit is 1.28s */

/// A2DP state callback parameters
typedef union {
    /**
     * @brief XR_BT_GAP_DISC_RES_EVT
     */
    struct disc_res_param {
        xr_bd_addr_t bda;                     /*!< remote bluetooth device address*/
        int num_prop;                          /*!< number of properties got */
        xr_bt_gap_dev_prop_t *prop;           /*!< properties discovered from the new device */
    } disc_res;                                /*!< discovery result parameter struct */

    /**
     * @brief  XR_BT_GAP_DISC_STATE_CHANGED_EVT
     */
    struct disc_state_changed_param {
        xr_bt_gap_discovery_state_t state;    /*!< discovery state */
    } disc_st_chg;                             /*!< discovery state changed parameter struct */

    /**
     * @brief XR_BT_GAP_RMT_SRVCS_EVT
     */
    struct rmt_srvcs_param {
        xr_bd_addr_t bda;                     /*!< remote bluetooth device address*/
        xr_bt_status_t stat;                  /*!< service search status */
        int num_uuids;                         /*!< number of UUID in uuid_list */
        xr_bt_uuid_t *uuid_list;              /*!< list of service UUIDs of remote device */
    } rmt_srvcs;                               /*!< services of remote device parameter struct */

    /**
     * @brief XR_BT_GAP_RMT_SRVC_REC_EVT
     */
    struct rmt_srvc_rec_param {
        xr_bd_addr_t bda;                     /*!< remote bluetooth device address*/
        xr_bt_status_t stat;                  /*!< service search status */
    } rmt_srvc_rec;                            /*!< specific service record from remote device parameter struct */

    /**
     * @brief XR_BT_GAP_READ_RSSI_DELTA_EVT *
     */
    struct read_rssi_delta_param {
        xr_bd_addr_t bda;                     /*!< remote bluetooth device address*/
        xr_bt_status_t stat;                  /*!< read rssi status */
        int8_t rssi_delta;                     /*!< rssi delta value range -128 ~127, The value zero indicates that the RSSI is inside the Golden Receive Power Range, the Golden Receive Power Range is from XR_BT_GAP_RSSI_LOW_THRLD to XR_BT_GAP_RSSI_HIGH_THRLD */
    } read_rssi_delta;                         /*!< read rssi parameter struct */

    /**
     * @brief XR_BT_GAP_CONFIG_EIR_DATA_EVT *
     */
    struct config_eir_data_param {
        xr_bt_status_t stat;                                   /*!< config EIR status:
                                                                    XR_BT_STATUS_SUCCESS: config success
                                                                    XR_BT_STATUS_EIR_TOO_LARGE: the EIR data is more than 240B. The EIR may not contain the whole data.
                                                                    others: failed
                                                                */
        uint8_t eir_type_num;                                   /*!< the number of EIR types in EIR type */
        xr_bt_eir_type_t eir_type[XR_BT_EIR_TYPE_MAX_NUM];    /*!< EIR types in EIR type */
    } config_eir_data;                                          /*!< config EIR data */

    /**
     * @brief XR_BT_GAP_AUTH_CMPL_EVT
     */
    struct auth_cmpl_param {
        xr_bd_addr_t bda;                     /*!< remote bluetooth device address*/
        xr_bt_status_t stat;                  /*!< authentication complete status */
        uint8_t device_name[XR_BT_GAP_MAX_BDNAME_LEN + 1]; /*!< device name */
    } auth_cmpl;                               /*!< authentication complete parameter struct */

    /**
     * @brief XR_BT_GAP_PIN_REQ_EVT
     */
    struct pin_req_param {
        xr_bd_addr_t bda;                     /*!< remote bluetooth device address*/
        bool min_16_digit;                     /*!< TRUE if the pin returned must be at least 16 digits */
    } pin_req;                                 /*!< pin request parameter struct */

    /**
     * @brief XR_BT_GAP_CFM_REQ_EVT
     */
    struct cfm_req_param {
        xr_bd_addr_t bda;                     /*!< remote bluetooth device address*/
        uint32_t num_val;                      /*!< the numeric value for comparison. */
    } cfm_req;                                 /*!< confirm request parameter struct */

    /**
     * @brief XR_BT_GAP_KEY_NOTIF_EVT
     */
    struct key_notif_param {
        xr_bd_addr_t bda;                     /*!< remote bluetooth device address*/
        uint32_t passkey;                      /*!< the numeric value for passkey entry. */
    } key_notif;                               /*!< passkey notif parameter struct */

    /**
     * @brief XR_BT_GAP_KEY_REQ_EVT
     */
    struct key_req_param {
        xr_bd_addr_t bda;                     /*!< remote bluetooth device address*/
    } key_req;                                 /*!< passkey request parameter struct */

    /**
     * @brief XR_BT_GAP_SET_AFH_CHANNELS_EVT
     */
    struct set_afh_channels_param {
        xr_bt_status_t stat;                  /*!< set AFH channel status */
    } set_afh_channels;                        /*!< set AFH channel parameter struct */

    /**
     * @brief XR_BT_GAP_READ_REMOTE_NAME_EVT
     */
    struct read_rmt_name_param {
        xr_bt_status_t stat;                  /*!< read Remote Name status */
        uint8_t rmt_name[XR_BT_GAP_MAX_BDNAME_LEN + 1]; /*!< Remote device name */
    } read_rmt_name;                        /*!< read Remote Name parameter struct */

    /**
     * @brief XR_BT_GAP_REMOVE_BOND_DEV_COMPLETE_EVT
     */
    struct bt_remove_bond_dev_cmpl_evt_param {
        xr_bd_addr_t bda;                          /*!< remote bluetooth device address*/
        xr_bt_status_t status;                     /*!< Indicate the remove bond device operation success status */
    }remove_bond_dev_cmpl;                           /*!< Event parameter of XR_BT_GAP_REMOVE_BOND_DEV_COMPLETE_EVT */

    /**
     * @brief XR_BT_GAP_POWER_MODE_EVT
     */
    struct power_mode_param {
        uint8_t   mode;
    } power_mode;

    /**
     * @brief XR_BT_GAP_SET_ROLE_COMPLETE_EVT
     */
    struct bt_set_role_cmpl_evt_param {
        xr_bd_addr_t bda;                       /*!< set role bluetooth device address*/
        uint8_t new_role;
        xr_bt_status_t status;
    } set_role_cmpl;

    /**
     * @brief XR_BT_GAP_GET_ROLE_COMPLETE_EVT
     */
    struct bt_get_role_cmpl_evt_param {
        xr_bd_addr_t bda;                       /*!< set role bluetooth device address*/
        uint8_t role;
        xr_bt_status_t status;
    } get_role_cmpl;

    /**
     * @brief XR_BT_GAP_READ_LOCAL_NAME_EVT
     */
    struct bt_read_local_name_evt_param {
        uint8_t *name;
        xr_bt_status_t status;
    } read_local_name;

} xr_bt_gap_cb_param_t;

/**
 * @brief           bluetooth GAP callback function type
 * @param           event : Event type
 * @param           param : Pointer to callback parameter
 */
typedef void (* xr_bt_gap_cb_t)(xr_bt_gap_cb_event_t event, xr_bt_gap_cb_param_t *param);

/**
 * @brief           get major service field of COD
 * @param[in]       cod: Class of Device
 * @return          major service bits
 */
static inline uint32_t xr_bt_gap_get_cod_srvc(uint32_t cod)
{
    return (cod & XR_BT_COD_SRVC_BIT_MASK) >> XR_BT_COD_SRVC_BIT_OFFSET;
}

/**
 * @brief           get major device field of COD
 * @param[in]       cod: Class of Device
 * @return          major device bits
 */
static inline uint32_t xr_bt_gap_get_cod_major_dev(uint32_t cod)
{
    return (cod & XR_BT_COD_MAJOR_DEV_BIT_MASK) >> XR_BT_COD_MAJOR_DEV_BIT_OFFSET;
}

/**
 * @brief           get minor service field of COD
 * @param[in]       cod: Class of Device
 * @return          minor service bits
 */
static inline uint32_t xr_bt_gap_get_cod_minor_dev(uint32_t cod)
{
    return (cod & XR_BT_COD_MINOR_DEV_BIT_MASK) >> XR_BT_COD_MINOR_DEV_BIT_OFFSET;
}

/**
 * @brief           get format type of COD
 * @param[in]       cod: Class of Device
 * @return          format type
 */
static inline uint32_t xr_bt_gap_get_cod_format_type(uint32_t cod)
{
    return (cod & XR_BT_COD_FORMAT_TYPE_BIT_MASK);
}

/**
 * @brief           decide the integrity of COD
 * @param[in]       cod: Class of Device
 * @return
 *                  - true if cod is valid
 *                  - false otherise
 */
static inline bool xr_bt_gap_is_valid_cod(uint32_t cod)
{
    if (xr_bt_gap_get_cod_format_type(cod) == XR_BT_COD_FORMAT_TYPE_1 &&
            xr_bt_gap_get_cod_srvc(cod) != XR_BT_COD_SRVC_NONE) {
        return true;
    }

    return false;
}

/**
 * @brief           register callback function. This function should be called after xr_bluedroid_enable() completes successfully
 *
 * @return
 *                  - XR_OK : Succeed
 *                  - XR_FAIL: others
 */
xr_err_t xr_bt_gap_register_callback(xr_bt_gap_cb_t callback);

/**
 * @brief           Set discoverability and connectability mode for legacy bluetooth. This function should
 *                  be called after xr_bluedroid_enable() completes successfully
 *
 * @param[in]       c_mode : one of the enums of xr_bt_connection_mode_t
 * @param[in]       d_mode : one of the enums of xr_bt_discovery_mode_t
 *
 * @return
 *                  - XR_OK : Succeed
 *                  - XR_ERR_INVALID_ARG: if argument invalid
 *                  - XR_ERR_INVALID_STATE: if bluetooth stack is not yet enabled
 *                  - XR_FAIL: others
 */
xr_err_t xr_bt_gap_set_scan_mode(xr_bt_connection_mode_t c_mode, xr_bt_discovery_mode_t d_mode, xr_bt_paired_mode_t pair_mode, xr_bt_paired_con_mode_t conn_paired_only);

/**
 * @brief           Set inquiry scan or page scan window, interval
 *
 * @param[in]       window : scan window
 * @param[in]       interval : scan interval
 * @param[in]       type : inquiry scan or page scan
 *
 * @return
 *                  - XR_OK : Succeed
 *                  - XR_ERR_INVALID_ARG: if argument invalid
 *                  - XR_ERR_INVALID_STATE: if bluetooth stack is not yet enabled
 *                  - XR_FAIL: others
 */
xr_err_t xr_bt_gap_set_ipscan_param(uint32_t window, uint32_t interval, uint8_t type);

/**
 * @brief           This function starts Inquiry and Name Discovery. It should be called after xr_bluedroid_enable() completes successfully.
 *                  When Inquiry is halted and cached results do not contain device name, then Name Discovery will connect to the peer target to get the device name.
 *                  xr_bt_gap_cb_t will be called with XR_BT_GAP_DISC_STATE_CHANGED_EVT when Inquriry is started or Name Discovery is completed.
 *                  xr_bt_gap_cb_t will be called with XR_BT_GAP_DISC_RES_EVT each time the two types of discovery results are got.
 *
 * @param[in]       mode - Inquiry mode
 * @param[in]       inq_len - Inquiry duration in 1.28 sec units, ranging from 0x01 to 0x30. This parameter only specifies the total duration of the Inquiry process,
 *                          - when this time expires, Inquiry will be halted.
 * @param[in]       num_rsps - Number of responses that can be received before the Inquiry is halted, value 0 indicates an unlimited number of responses.
 *
 * @return
 *                  - XR_OK : Succeed
 *                  - XR_ERR_INVALID_STATE: if bluetooth stack is not yet enabled
 *                  - XR_ERR_INVALID_ARG: if invalid parameters are provided
 *                  - XR_FAIL: others
 */
xr_err_t xr_bt_gap_start_discovery(xr_bt_inq_mode_t mode, uint8_t inq_len, uint8_t num_rsps);

/**
 * @brief           Cancel Inquiry and Name Discovery. This function should be called after xr_bluedroid_enable() completes successfully.
 *                  xr_bt_gap_cb_t will be called with XR_BT_GAP_DISC_STATE_CHANGED_EVT if Inquiry or Name Discovery is cancelled by
 *                  calling this function.
 *
 * @return
 *                  - XR_OK : Succeed
 *                  - XR_ERR_INVALID_STATE: if bluetooth stack is not yet enabled
 *                  - XR_FAIL: others
 */
xr_err_t xr_bt_gap_cancel_discovery(void);

/**
 * @brief           Start SDP to get remote services. This function should be called after xr_bluedroid_enable() completes successfully.
 *                  xr_bt_gap_cb_t will be called with XR_BT_GAP_RMT_SRVCS_EVT after service discovery ends
 *
 * @return
 *                  - XR_OK : Succeed
 *                  - XR_ERR_INVALID_STATE: if bluetooth stack is not yet enabled
 *                  - XR_FAIL: others
 */
xr_err_t xr_bt_gap_get_remote_services(xr_bd_addr_t remote_bda);

/**
 * @brief           Start SDP to look up the service matching uuid on the remote device. This function should be called after
 *                  xr_bluedroid_enable() completes successfully
 *
 *                  xr_bt_gap_cb_t will be called with XR_BT_GAP_RMT_SRVC_REC_EVT after service discovery ends
 * @return
 *                  - XR_OK : Succeed
 *                  - XR_ERR_INVALID_STATE: if bluetooth stack is not yet enabled
 *                  - XR_FAIL: others
 */
xr_err_t xr_bt_gap_get_remote_service_record(xr_bd_addr_t remote_bda, xr_bt_uuid_t *uuid);

/**
 * @brief           This function is called to get EIR data for a specific type.
 *
 * @param[in]       eir - pointer of raw eir data to be resolved
 * @param[in]       type   - specific EIR data type
 * @param[out]      length - return the length of EIR data excluding fields of length and data type
 *
 * @return          pointer of starting position of eir data excluding eir data type, NULL if not found
 *
 */
uint8_t *xr_bt_gap_resolve_eir_data(uint8_t *eir, xr_bt_eir_type_t type, uint8_t *length);

/**
 * @brief           This function is called to config EIR data.
 *
 *                  xr_bt_gap_cb_t will be called with XR_BT_GAP_CONFIG_EIR_DATA_EVT after config EIR ends.
 *
 * @param[in]       eir_data - pointer of EIR data content
 * @return
 *                  - XR_OK : Succeed
 *                  - XR_ERR_INVALID_STATE: if bluetooth stack is not yet enabled
 *                  - XR_ERR_INVALID_ARG: if param is invalid
 *                  - XR_FAIL: others
 */
xr_err_t xr_bt_gap_config_eir_data(xr_bt_eir_data_t *eir_data);

/**
 * @brief           This function is called to set class of device.
 *                  xr_bt_gap_cb_t will be called with XR_BT_GAP_SET_COD_EVT after set COD ends
 *                  Some profile have special restrictions on class of device,
 *                  changes may cause these profile do not work
 *
 * @param[in]       cod - class of device
 * @param[in]       mode   - setting mode
 *
 * @return
 *                  - XR_OK : Succeed
 *                  - XR_ERR_INVALID_STATE: if bluetooth stack is not yet enabled
 *                  - XR_ERR_INVALID_ARG: if param is invalid
 *                  - XR_FAIL: others
 */
xr_err_t xr_bt_gap_set_cod(xr_bt_cod_t cod, xr_bt_cod_mode_t mode);

/**
 * @brief           This function is called to get class of device.
 *
 * @param[out]      cod - class of device
 *
 * @return
 *                  - XR_OK : Succeed
 *                  - XR_FAIL: others
 */
xr_err_t xr_bt_gap_get_cod(xr_bt_cod_t *cod);

/**
 * @brief           This function is called to read RSSI delta by address after connected. The RSSI value returned by XR_BT_GAP_READ_RSSI_DELTA_EVT.
 *
 *
 * @param[in]       remote_addr - remote device address, corresponding to a certain connection handle.
 * @return
 *                  - XR_OK : Succeed
 *                  - XR_FAIL: others
 *
 */
xr_err_t xr_bt_gap_read_rssi_delta(xr_bd_addr_t remote_addr);

/**
* @brief           Removes a device from the security database list of
*                  peer device.
*
* @param[in]       bd_addr : BD address of the peer device
*
* @return          - XR_OK : success
*                  - XR_FAIL  : failed
*
*/
xr_err_t xr_bt_gap_remove_bond_device(xr_bd_addr_t bd_addr);

/**
* @brief           Get the device number from the security database list of peer device.
*                  It will return the device bonded number immediately.
*
* @return          - >= 0 : bonded devices number.
*                  - XR_FAIL  : failed
*
*/
int xr_bt_gap_get_bond_device_num(void);

/**
* @brief           Get the device from the security database list of peer device.
*                  It will return the device bonded information immediately.
* @param[inout]    dev_num: Indicate the dev_list array(buffer) size as input.
*                           If dev_num is large enough, it means the actual number as output.
*                           Suggest that dev_num value equal to xr_ble_get_bond_device_num().
*
* @param[out]      dev_list_addr: an array(buffer) of `xr_bd_addr_t` type. Use for storing the bonded devices address.
*                            The dev_list should be allocated by who call this API.
*
* @param[out]      dev_list_name: an array(buffer) of `dev_list_name` type. Use for storing the bonded devices name.
*                            The dev_list should be allocated by who call this API.
* @return
*                  - XR_OK : Succeed
*                  - XR_ERR_INVALID_STATE: if bluetooth stack is not yet enabled
*                  - XR_FAIL: others
*/
xr_err_t xr_bt_gap_get_bond_device_list(int *dev_num, xr_bd_addr_t *dev_list_addr, xr_bd_name_t *dev_list_name);

/**
* @brief            Set pin type and default pin code for legacy pairing.
*
* @param[in]        pin_type:       Use variable or fixed pin.
*                                   If pin_type is XR_BT_PIN_TYPE_VARIABLE, pin_code and pin_code_len
*                                   will be ignored, and XR_BT_GAP_PIN_REQ_EVT will come when control
*                                   requests for pin code.
*                                   Else, will use fixed pin code and not callback to users.
* @param[in]        pin_code_len:   Length of pin_code
* @param[in]        pin_code:       Pin_code
*
* @return           - XR_OK : success
*                   - XR_ERR_INVALID_STATE: if bluetooth stack is not yet enabled
*                   - other  : failed
*/
xr_err_t xr_bt_gap_set_pin(xr_bt_pin_type_t pin_type, uint8_t pin_code_len, xr_bt_pin_code_t pin_code);

/**
* @brief            Reply the pin_code to the peer device for legacy pairing
*                   when XR_BT_GAP_PIN_REQ_EVT is coming.
*
* @param[in]        bd_addr:        BD address of the peer
* @param[in]        accept:         Pin_code reply successful or declined.
* @param[in]        pin_code_len:   Length of pin_code
* @param[in]        pin_code:       Pin_code
*
* @return           - XR_OK : success
*                   - XR_ERR_INVALID_STATE: if bluetooth stack is not yet enabled
*                   - other  : failed
*/
xr_err_t xr_bt_gap_pin_reply(xr_bd_addr_t bd_addr, bool accept, uint8_t pin_code_len, xr_bt_pin_code_t pin_code);

#if (BT_SSP_INCLUDED == TRUE)
/**
* @brief            Set a GAP security parameter value. Overrides the default value.
*
* @param[in]        param_type : the type of the param which is to be set
* @param[in]        value  : the param value
* @param[in]        len : the length of the param value
*
* @return           - XR_OK : success
*                   - XR_ERR_INVALID_STATE: if bluetooth stack is not yet enabled
*                   - other  : failed
*
*/
xr_err_t xr_bt_gap_set_security_param(xr_bt_sp_param_t param_type,
                                        void *value, uint8_t len);

/**
* @brief            Reply the key value to the peer device in the legacy connection stage.
*
* @param[in]        bd_addr : BD address of the peer
* @param[in]        accept : passkey entry successful or declined.
* @param[in]        passkey : passkey value, must be a 6 digit number,
*                                     can be lead by 0.
*
* @return           - XR_OK : success
*                   - XR_ERR_INVALID_STATE: if bluetooth stack is not yet enabled
*                   - other  : failed
*
*/
xr_err_t xr_bt_gap_ssp_passkey_reply(xr_bd_addr_t bd_addr, bool accept, uint32_t passkey);


/**
* @brief            Reply the confirm value to the peer device in the legacy connection stage.
*
* @param[in]        bd_addr : BD address of the peer device
* @param[in]        accept : numbers to compare are the same or different.
*
* @return           - XR_OK : success
*                   - XR_ERR_INVALID_STATE: if bluetooth stack is not yet enabled
*                   - other  : failed
*
*/
xr_err_t xr_bt_gap_ssp_confirm_reply(xr_bd_addr_t bd_addr, bool accept);

#endif /*(BT_SSP_INCLUDED == TRUE)*/

/**
* @brief            Set the AFH channels
*
* @param[in]        channels :  The n th such field (in the range 0 to 78) contains the value for channel n :
*                               0 means channel n is bad.
*                               1 means channel n is unknown.
*                               The most significant bit is reserved and shall be set to 0.
*                               At least 20 channels shall be marked as unknown.
*
* @return           - XR_OK : success
*                   - XR_ERR_INVALID_STATE: if bluetooth stack is not yet enabled
*                   - other  : failed
*
*/
xr_err_t xr_bt_gap_set_afh_channels(xr_bt_gap_afh_channels channels);

/**
* @brief            Read the remote device name
*
* @param[in]        remote_bda: The remote device's address
*
* @return           - XR_OK : success
*                   - XR_ERR_INVALID_STATE: if bluetooth stack is not yet enabled
*                   - other  : failed
*
*/
xr_err_t xr_bt_gap_read_remote_name(xr_bd_addr_t remote_bda);

/**
* @brief            Set sniff mode
*
* @param[in]        remote_bda: The remote device's address
* @param[in]        sniff_mode: sniff mode
* @param[in]        min_period: min period
* @param[in]        max_period: max period
* @param[in]        attempt: Number of Baseband receive slots for sniff attempt
* @param[in]        timeout: Number of Baseband receive slots for sniff timeout
*
* @return           - XR_OK : success
*                   - XR_ERR_INVALID_STATE: if bluetooth stack is not yet enabled
*                   - other  : failed
*
*/
xr_err_t xr_bt_gap_set_sniff_mode(xr_bd_addr_t remote_bda, xr_bt_sniff_mode_t sniff_mode,
           uint32_t min_period, uint32_t max_period, uint32_t attempt,
           uint32_t timeout);

/**
* @brief           Read the remote device power mode
*
* @param[in]       remote_bda: The remote device's address
*
* @return          - XR_OK : success
*                  - XR_ERR_INVALID_STATE: if bluetooth stack is not yet enabled
*                  - other  : failed
*
*/
xr_err_t xr_bt_gap_power_mode(xr_bd_addr_t remote_bda);

/**
* @brief           Setting roles in link
*
* @param[in]       remote_bda: The remote device's address
* @param[in]       role: The roles in link
*
* @return          - XR_OK : success
*                  - XR_ERR_INVALID_STATE: if bluetooth stack is not yet enabled
*                  - other  : failed
*
*/
xr_err_t xr_bt_gap_set_role(xr_bd_addr_t remote_bda, xr_bt_role_t role);

/**
* @brief           Read roles in link
*
* @param[in]       remote_bda: The remote device's address
*
* @return          - XR_OK : success
*                  - XR_ERR_INVALID_STATE: if bluetooth stack is not yet enabled
*                  - other  : failed
*
*/
xr_err_t xr_bt_gap_role(xr_bd_addr_t remote_bda);

/**
* @brief           set host whether auto enter sniff mode or not
*
* @param[in]       mode: TURE means auto enter,otherwise means would not auto enter
*
* @return          - XR_OK : success
*                  - XR_ERR_INVALID_STATE: if bluetooth stack is not yet enabled
*                  - other  : failed
*
*/
xr_err_t xr_bt_gap_set_auto_sniff_mode(uint8_t mode);
#ifdef __cplusplus
}
#endif

#endif /* __XR_GAP_BT_API_H__ */
