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

#ifndef __XR_GATTC_API_H__
#define __XR_GATTC_API_H__

#include "xr_bt_defs.h"
#include "xr_gatt_defs.h"
#include "xr_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/// GATT Client callback function events
typedef enum {
    XR_GATTC_REG_EVT                 = 0,        /*!< When GATT client is registered, the event comes */
    XR_GATTC_UNREG_EVT               = 1,        /*!< When GATT client is unregistered, the event comes */
    XR_GATTC_OPEN_EVT                = 2,        /*!< When GATT virtual connection is set up, the event comes */
    XR_GATTC_READ_CHAR_EVT           = 3,        /*!< When GATT characteristic is read, the event comes */
    XR_GATTC_WRITE_CHAR_EVT          = 4,        /*!< When GATT characteristic write operation completes, the event comes */
    XR_GATTC_CLOSE_EVT               = 5,        /*!< When GATT virtual connection is closed, the event comes */
    XR_GATTC_SEARCH_CMPL_EVT         = 6,        /*!< When GATT service discovery is completed, the event comes */
    XR_GATTC_SEARCH_RES_EVT          = 7,        /*!< When GATT service discovery result is got, the event comes */
    XR_GATTC_READ_DESCR_EVT          = 8,        /*!< When GATT characteristic descriptor read completes, the event comes */
    XR_GATTC_WRITE_DESCR_EVT         = 9,        /*!< When GATT characteristic descriptor write completes, the event comes */
    XR_GATTC_NOTIFY_EVT              = 10,       /*!< When GATT notification or indication arrives, the event comes */
    XR_GATTC_PREP_WRITE_EVT          = 11,       /*!< When GATT prepare-write operation completes, the event comes */
    XR_GATTC_EXEC_EVT                = 12,       /*!< When write execution completes, the event comes */
    XR_GATTC_ACL_EVT                 = 13,       /*!< When ACL connection is up, the event comes */
    XR_GATTC_CANCEL_OPEN_EVT         = 14,       /*!< When GATT client ongoing connection is cancelled, the event comes */
    XR_GATTC_SRVC_CHG_EVT            = 15,       /*!< When "service changed" occurs, the event comes */
    XR_GATTC_ENC_CMPL_CB_EVT         = 17,       /*!< When encryption procedure completes, the event comes */
    XR_GATTC_CFG_MTU_EVT             = 18,       /*!< When configuration of MTU completes, the event comes */
    XR_GATTC_ADV_DATA_EVT            = 19,       /*!< When advertising of data, the event comes */
    XR_GATTC_MULT_ADV_ENB_EVT        = 20,       /*!< When multi-advertising is enabled, the event comes */
    XR_GATTC_MULT_ADV_UPD_EVT        = 21,       /*!< When multi-advertising parameters are updated, the event comes */
    XR_GATTC_MULT_ADV_DATA_EVT       = 22,       /*!< When multi-advertising data arrives, the event comes */
    XR_GATTC_MULT_ADV_DIS_EVT        = 23,       /*!< When multi-advertising is disabled, the event comes */
    XR_GATTC_CONGEST_EVT             = 24,       /*!< When GATT connection congestion comes, the event comes */
    XR_GATTC_BTH_SCAN_ENB_EVT        = 25,       /*!< When batch scan is enabled, the event comes */
    XR_GATTC_BTH_SCAN_CFG_EVT        = 26,       /*!< When batch scan storage is configured, the event comes */
    XR_GATTC_BTH_SCAN_RD_EVT         = 27,       /*!< When Batch scan read event is reported, the event comes */
    XR_GATTC_BTH_SCAN_THR_EVT        = 28,       /*!< When Batch scan threshold is set, the event comes */
    XR_GATTC_BTH_SCAN_PARAM_EVT      = 29,       /*!< When Batch scan parameters are set, the event comes */
    XR_GATTC_BTH_SCAN_DIS_EVT        = 30,       /*!< When Batch scan is disabled, the event comes */
    XR_GATTC_SCAN_FLT_CFG_EVT        = 31,       /*!< When Scan filter configuration completes, the event comes */
    XR_GATTC_SCAN_FLT_PARAM_EVT      = 32,       /*!< When Scan filter parameters are set, the event comes */
    XR_GATTC_SCAN_FLT_STATUS_EVT     = 33,       /*!< When Scan filter status is reported, the event comes */
    XR_GATTC_ADV_VSC_EVT             = 34,       /*!< When advertising vendor spec content event is reported, the event comes */
    XR_GATTC_REG_FOR_NOTIFY_EVT      = 38,       /*!< When register for notification of a service completes, the event comes */
    XR_GATTC_UNREG_FOR_NOTIFY_EVT    = 39,       /*!< When unregister for notification of a service completes, the event comes */
    XR_GATTC_CONNECT_EVT             = 40,       /*!< When the ble physical connection is set up, the event comes */
    XR_GATTC_DISCONNECT_EVT          = 41,       /*!< When the ble physical connection disconnected, the event comes */
    XR_GATTC_READ_MULTIPLE_EVT       = 42,       /*!< When the ble characteristic or descriptor multiple complete, the event comes */
    XR_GATTC_QUEUE_FULL_EVT          = 43,       /*!< When the gattc command queue full, the event comes */
    XR_GATTC_SET_ASSOC_EVT           = 44,       /*!< When the ble gattc set the associated address complete, the event comes */
    XR_GATTC_GET_ADDR_LIST_EVT       = 45,       /*!< When the ble get gattc address list in cache finish, the event comes */
    XR_GATTC_DIS_SRVC_CMPL_EVT       = 46,       /*!< When the ble discover service complete, the event comes */
} xr_gattc_cb_event_t;


/**
 * @brief Gatt client callback parameters union
 */
typedef union {
    /**
     * @brief XR_GATTC_REG_EVT
     */
    struct gattc_reg_evt_param {
        xr_gatt_status_t status;       /*!< Operation status */
        uint16_t app_id;                /*!< Application id which input in register API */
    } reg;                              /*!< Gatt client callback param of XR_GATTC_REG_EVT */

    /**
     * @brief XR_GATTC_OPEN_EVT
     */
    struct gattc_open_evt_param {
        xr_gatt_status_t status;       /*!< Operation status */
        uint16_t conn_id;               /*!< Connection id */
        xr_bd_addr_t remote_bda;       /*!< Remote bluetooth device address */
        uint16_t mtu;                   /*!< MTU size */
    } open;                             /*!< Gatt client callback param of XR_GATTC_OPEN_EVT */

    /**
     * @brief XR_GATTC_CLOSE_EVT
     */
    struct gattc_close_evt_param {
        xr_gatt_status_t status;       /*!< Operation status */
        uint16_t conn_id;               /*!< Connection id */
        xr_bd_addr_t remote_bda;       /*!< Remote bluetooth device address */
        xr_gatt_conn_reason_t reason;  /*!< The reason of gatt connection close */
    } close;                            /*!< Gatt client callback param of XR_GATTC_CLOSE_EVT */

    /**
     * @brief XR_GATTC_CFG_MTU_EVT
     */
    struct gattc_cfg_mtu_evt_param {
        xr_gatt_status_t status;       /*!< Operation status */
        uint16_t conn_id;               /*!< Connection id */
        uint16_t mtu;                   /*!< MTU size */
    } cfg_mtu;                          /*!< Gatt client callback param of XR_GATTC_CFG_MTU_EVT */

    /**
     * @brief XR_GATTC_SEARCH_CMPL_EVT
     */
    struct gattc_search_cmpl_evt_param {
        xr_gatt_status_t status;                     /*!< Operation status */
        uint16_t conn_id;                             /*!< Connection id */
        xr_service_source_t searched_service_source; /*!< The source of the service information */
    } search_cmpl;                                    /*!< Gatt client callback param of XR_GATTC_SEARCH_CMPL_EVT */

    /**
     * @brief XR_GATTC_SEARCH_RES_EVT
     */
    struct gattc_search_res_evt_param {
        uint16_t conn_id;               /*!< Connection id */
        uint16_t start_handle;          /*!< Service start handle */
        uint16_t end_handle;            /*!< Service end handle */
        xr_gatt_id_t srvc_id;          /*!< Service id, include service uuid and other information */
        bool      is_primary;           /*!< True if this is the primary service */
    } search_res;                       /*!< Gatt client callback param of XR_GATTC_SEARCH_RES_EVT */

    /**
     * @brief XR_GATTC_READ_CHAR_EVT,  XR_GATTC_READ_DESCR_EVT
     */
    struct gattc_read_char_evt_param {

        xr_gatt_status_t status;       /*!< Operation status */
        uint16_t conn_id;               /*!< Connection id */
        uint16_t handle;                /*!< Characteristic handle */
        uint8_t *value;                 /*!< Characteristic value */
        uint16_t value_len;             /*!< Characteristic value length */
    } read;                             /*!< Gatt client callback param of XR_GATTC_READ_CHAR_EVT */

    /**
     * @brief XR_GATTC_WRITE_CHAR_EVT, XR_GATTC_PREP_WRITE_EVT, XR_GATTC_WRITE_DESCR_EVT
     */
    struct gattc_write_evt_param {
        xr_gatt_status_t status;       /*!< Operation status */
        uint16_t conn_id;               /*!< Connection id */
        uint16_t handle;                /*!< The Characteristic or descriptor handle */
        uint16_t offset;                /*!< The prepare write offset, this value is valid only when prepare write */
    } write;                            /*!< Gatt client callback param of XR_GATTC_WRITE_DESCR_EVT */

    /**
     * @brief XR_GATTC_EXEC_EVT
     */
    struct gattc_exec_cmpl_evt_param {
        xr_gatt_status_t status;       /*!< Operation status */
        uint16_t conn_id;               /*!< Connection id */
    } exec_cmpl;                        /*!< Gatt client callback param of XR_GATTC_EXEC_EVT */

    /**
     * @brief XR_GATTC_NOTIFY_EVT
     */
    struct gattc_notify_evt_param {
        uint16_t conn_id;               /*!< Connection id */
        xr_bd_addr_t remote_bda;       /*!< Remote bluetooth device address */
        uint16_t handle;                /*!< The Characteristic or descriptor handle */
        uint16_t value_len;             /*!< Notify attribute value */
        uint8_t *value;                 /*!< Notify attribute value */
        bool is_notify;                 /*!< True means notify, false means indicate */
    } notify;                           /*!< Gatt client callback param of XR_GATTC_NOTIFY_EVT */

    /**
     * @brief XR_GATTC_SRVC_CHG_EVT
	 */
    struct gattc_srvc_chg_evt_param {
        xr_bd_addr_t remote_bda;       /*!< Remote bluetooth device address */
    } srvc_chg;                         /*!< Gatt client callback param of XR_GATTC_SRVC_CHG_EVT */

    /**
     * @brief XR_GATTC_CONGEST_EVT
     */
    struct gattc_congest_evt_param {
        uint16_t conn_id;               /*!< Connection id */
        bool congested;                 /*!< Congested or not */
    } congest;                          /*!< Gatt client callback param of XR_GATTC_CONGEST_EVT */
    /**
     * @brief XR_GATTC_REG_FOR_NOTIFY_EVT
     */
    struct gattc_reg_for_notify_evt_param {
        xr_gatt_status_t status;       /*!< Operation status */
        uint16_t handle;                /*!< The characteristic or descriptor handle */
    } reg_for_notify;                   /*!< Gatt client callback param of XR_GATTC_REG_FOR_NOTIFY_EVT */

    /**
     * @brief XR_GATTC_UNREG_FOR_NOTIFY_EVT
     */
    struct gattc_unreg_for_notify_evt_param {
        xr_gatt_status_t status;       /*!< Operation status */
        uint16_t handle;                /*!< The characteristic or descriptor handle */
    } unreg_for_notify;                 /*!< Gatt client callback param of XR_GATTC_UNREG_FOR_NOTIFY_EVT */

    /**
     * @brief XR_GATTC_CONNECT_EVT
     */
    struct gattc_connect_evt_param {
        uint16_t conn_id;               /*!< Connection id */
        xr_bd_addr_t remote_bda;       /*!< Remote bluetooth device address */
        xr_gatt_conn_params_t conn_params; /*!< current connection parameters */
    } connect;                          /*!< Gatt client callback param of XR_GATTC_CONNECT_EVT */

    /**
     * @brief XR_GATTC_DISCONNECT_EVT
     */
    struct gattc_disconnect_evt_param {
        xr_gatt_conn_reason_t reason;  /*!< disconnection reason */
        uint16_t conn_id;               /*!< Connection id */
        xr_bd_addr_t remote_bda;       /*!< Remote bluetooth device address */
    } disconnect;                       /*!< Gatt client callback param of XR_GATTC_DISCONNECT_EVT */
    /**
     * @brief XR_GATTC_SET_ASSOC_EVT
     */
    struct gattc_set_assoc_addr_cmp_evt_param {
        xr_gatt_status_t status;      /*!< Operation status */
    } set_assoc_cmp;                     /*!< Gatt client callback param of XR_GATTC_SET_ASSOC_EVT */
    /**
     * @brief XR_GATTC_GET_ADDR_LIST_EVT
     */
    struct gattc_get_addr_list_evt_param {
        xr_gatt_status_t status;      /*!< Operation status */
        uint8_t num_addr;              /*!< The number of address in the gattc cache address list */
        xr_bd_addr_t *addr_list;      /*!< The pointer to the address list which has been get from the gattc cache */
    } get_addr_list;                   /*!< Gatt client callback param of XR_GATTC_GET_ADDR_LIST_EVT */

    /**
     * @brief XR_GATTC_QUEUE_FULL_EVT
     */
    struct gattc_queue_full_evt_param {
        xr_gatt_status_t status;      /*!< Operation status */
        uint16_t conn_id;              /*!< Connection id */
        bool     is_full;              /*!< The gattc command queue is full or not */
    } queue_full;                      /*!< Gatt client callback param of XR_GATTC_QUEUE_FULL_EVT */

    /**
     * @brief XR_GATTC_DIS_SRVC_CMPL_EVT
     */
    struct gattc_dis_srvc_cmpl_evt_param {
        xr_gatt_status_t status;      /*!< Operation status */
        uint16_t conn_id;              /*!< Connection id */
    } dis_srvc_cmpl;                   /*!< Gatt client callback param of XR_GATTC_DIS_SRVC_CMPL_EVT */

} xr_ble_gattc_cb_param_t;             /*!< GATT client callback parameter union type */

/**
 * @brief GATT Client callback function type
 * @param event : Event type
 * @param gatts_if : GATT client access interface, normally
 *                   different gattc_if correspond to different profile
 * @param param : Point to callback parameter, currently is union type
 */
typedef void (* xr_gattc_cb_t)(xr_gattc_cb_event_t event, xr_gatt_if_t gattc_if, xr_ble_gattc_cb_param_t *param);

/**
 * @brief           This function is called to register application callbacks
 *                  with GATTC module.
 *
 * @param[in]       callback : pointer to the application callback function.
 *
 * @return
 *                  - XR_OK: success
 *                  - other: failed
 *
 */
xr_err_t xr_ble_gattc_register_callback(xr_gattc_cb_t callback);


/**
 * @brief           This function is called to register application callbacks
 *                  with GATTC module.
 *
 * @param[in]       app_id : Application Identify (UUID), for different application
 *
 * @return
 *                  - XR_OK: success
 *                  - other: failed
 *
 */
xr_err_t xr_ble_gattc_app_register(uint16_t app_id);


/**
 * @brief           This function is called to unregister an application
 *                  from GATTC module.
 *
 * @param[in]       gattc_if: Gatt client access interface.
 *
 * @return
 *                  - XR_OK: success
 *                  - other: failed
 *
 */
xr_err_t xr_ble_gattc_app_unregister(xr_gatt_if_t gattc_if);


/**
 * @brief           Open a direct connection or add a background auto connection
 *
 * @param[in]       gattc_if: Gatt client access interface.
 * @param[in]       remote_bda: remote device bluetooth device address.
 * @param[in]       remote_addr_type: remote device bluetooth device the address type.
 * @param[in]       is_direct: direct connection or background auto connection
 *
 * @return
 *                  - XR_OK: success
 *                  - other: failed
 *
 */
xr_err_t xr_ble_gattc_open(xr_gatt_if_t gattc_if, xr_bd_addr_t remote_bda, xr_ble_addr_type_t remote_addr_type, bool is_direct);


/**
 * @brief           Close the virtual connection to the GATT server. gattc may have multiple virtual GATT server connections when multiple app_id registered,
 *                  this API only close one virtual GATT server connection. if there exist other virtual GATT server connections,
 *                  it does not disconnect the physical connection.
 *                  if you want to disconnect the physical connection directly, you can use xr_ble_gap_disconnect(xr_bd_addr_t remote_device).
 *
 * @param[in]       gattc_if: Gatt client access interface.
 * @param[in]       conn_id: connection ID to be closed.
 *
 * @return
 *                  - XR_OK: success
 *                  - other: failed
 *
 */
xr_err_t xr_ble_gattc_close (xr_gatt_if_t gattc_if, uint16_t conn_id);


/**
 * @brief           Configure the MTU size in the GATT channel. This can be done
 *                  only once per connection. Before using, use xr_ble_gatt_set_local_mtu()
 *                  to configure the local MTU size.
 *
 *
 * @param[in]       gattc_if: Gatt client access interface.
 * @param[in]       conn_id: connection ID.
 *
 * @return
 *                  - XR_OK: success
 *                  - other: failed
 *
 */
xr_err_t xr_ble_gattc_send_mtu_req (xr_gatt_if_t gattc_if, uint16_t conn_id);


/**
 * @brief           This function is called to get service from local cache. 
 *                  This function report service search result by a callback 
 *                  event, and followed by a service search complete event.
 *
 * @param[in]       gattc_if: Gatt client access interface.
 * @param[in]       conn_id: connection ID.
 * @param[in]       filter_uuid: a UUID of the service application is interested in.
 *                  If Null, discover for all services.
 *
 * @return
 *                  - XR_OK: success
 *                  - other: failed
 *
 */
xr_err_t xr_ble_gattc_search_service(xr_gatt_if_t gattc_if, uint16_t conn_id, xr_bt_uuid_t *filter_uuid);

/**
 * @brief           Find all the service with the given service uuid in the gattc cache, if the svc_uuid is NULL, find all the service.
 *                  Note: It just get service from local cache, won't get from remote devices. If want to get it from remote device, need
 *                  to used the xr_ble_gattc_search_service.
 *
 * @param[in]       gattc_if: Gatt client access interface.
 * @param[in]       conn_id: connection ID which identify the server.
 * @param[in]       svc_uuid: the pointer to the service uuid.
 * @param[out]      result: The pointer to the service which has been found in the gattc cache.
 * @param[inout]   count: input the number of service want to find,
 *                         it will output the number of service has been found in the gattc cache with the given service uuid.
 * @param[in]       offset: Offset of the service position to get.
 *
 * @return
 *                  - XR_OK: success
 *                  - other: failed
 *
 */
xr_gatt_status_t xr_ble_gattc_get_service(xr_gatt_if_t gattc_if, uint16_t conn_id, xr_bt_uuid_t *svc_uuid,
                                            xr_gattc_service_elem_t *result, uint16_t *count, uint16_t offset);

/**
 * @brief           Find all the characteristic with the given service in the gattc cache
 *                  Note: It just get characteristic from local cache, won't get from remote devices.
 *
 * @param[in]       gattc_if: Gatt client access interface.
 * @param[in]       conn_id: connection ID which identify the server.
 * @param[in]       start_handle: the attribute start handle.
 * @param[in]       end_handle: the attribute end handle
 * @param[out]      result: The pointer to the characteristic in the service.
 * @param[inout]   count: input the number of characteristic want to find,
 *                         it will output the number of characteristic has been found in the gattc cache with the given service.
 * @param[in]       offset: Offset of the characteristic position to get.
 *
 * @return
 *                  - XR_OK: success
 *                  - other: failed
 *
 */
xr_gatt_status_t xr_ble_gattc_get_all_char(xr_gatt_if_t gattc_if,
                                             uint16_t conn_id,
                                             uint16_t start_handle,
                                             uint16_t end_handle,
                                             xr_gattc_char_elem_t *result,
                                             uint16_t *count, uint16_t offset);

/**
 * @brief           Find all the descriptor with the given characteristic in the gattc cache
 *                  Note: It just get descriptor from local cache, won't get from remote devices.
 *
 * @param[in]       gattc_if: Gatt client access interface.
 * @param[in]       conn_id: connection ID which identify the server.
 * @param[in]       char_handle: the given characteristic handle
 * @param[out]      result: The pointer to the descriptor in the characteristic.
 * @param[inout]    count: input the number of descriptor want to find,
 *                         it will output the number of descriptor has been found in the gattc cache with the given characteristic.
 * @param[in]       offset: Offset of the descriptor position to get.
 *
 * @return
 *                  - XR_OK: success
 *                  - other: failed
 *
 */
xr_gatt_status_t xr_ble_gattc_get_all_descr(xr_gatt_if_t gattc_if,
                                              uint16_t conn_id,
                                              uint16_t char_handle,
                                              xr_gattc_descr_elem_t *result,
                                              uint16_t *count, uint16_t offset);


/**
 * @brief           Find the characteristic with the given characteristic uuid in the gattc cache
 *                  Note: It just get characteristic from local cache, won't get from remote devices.
 *
 * @param[in]       gattc_if: Gatt client access interface.
 * @param[in]       conn_id: connection ID which identify the server.
 * @param[in]       start_handle: the attribute start handle
 * @param[in]       end_handle: the attribute end handle
 * @param[in]       char_uuid: the characteristic uuid
 * @param[out]      result: The pointer to the characteristic in the service.
 * @param[inout]   count: input the number of characteristic want to find,
 *                         it will output the number of characteristic has been found in the gattc cache with the given service.
 *
 * @return
 *                  - XR_OK: success
 *                  - other: failed
 *
 */
xr_gatt_status_t xr_ble_gattc_get_char_by_uuid(xr_gatt_if_t gattc_if,
                                                 uint16_t conn_id,
                                                 uint16_t start_handle,
                                                 uint16_t end_handle,
                                                 xr_bt_uuid_t char_uuid,
                                                 xr_gattc_char_elem_t *result,
                                                 uint16_t *count);

/**
 * @brief           Find the descriptor with the given characteristic uuid in the gattc cache
 *                  Note: It just get descriptor from local cache, won't get from remote devices.
 *
 * @param[in]       gattc_if: Gatt client access interface.
 * @param[in]       conn_id: connection ID which identify the server.
 * @param[in]       start_handle: the attribute start handle
 * @param[in]       end_handle: the attribute end handle
 * @param[in]       char_uuid: the characteristic uuid.
 * @param[in]       descr_uuid: the descriptor uuid.
 * @param[out]      result: The pointer to the descriptor in the given characteristic.
 * @param[inout]   count: input the number of descriptor want to find,
 *                         it will output the number of descriptor has been found in the gattc cache with the given characteristic.
 *
 * @return
 *                  - XR_OK: success
 *                  - other: failed
 *
 */
xr_gatt_status_t xr_ble_gattc_get_descr_by_uuid(xr_gatt_if_t gattc_if,
                                                  uint16_t conn_id,
                                                  uint16_t start_handle,
                                                  uint16_t end_handle,
                                                  xr_bt_uuid_t char_uuid,
                                                  xr_bt_uuid_t descr_uuid,
                                                  xr_gattc_descr_elem_t *result,
                                                  uint16_t *count);

/**
 * @brief           Find the descriptor with the given characteristic handle in the gattc cache
 *                  Note: It just get descriptor from local cache, won't get from remote devices.
 *
 * @param[in]       gattc_if: Gatt client access interface.
 * @param[in]       conn_id: connection ID which identify the server.
 * @param[in]       char_handle: the characteristic handle.
 * @param[in]       descr_uuid: the descriptor uuid.
 * @param[out]      result: The pointer to the descriptor in the given characteristic.
 * @param[inout]   count: input the number of descriptor want to find,
 *                         it will output the number of descriptor has been found in the gattc cache with the given characteristic.
 *
 * @return
 *                  - XR_OK: success
 *                  - other: failed
 *
 */
xr_gatt_status_t xr_ble_gattc_get_descr_by_char_handle(xr_gatt_if_t gattc_if,
                                                         uint16_t conn_id,
                                                         uint16_t char_handle,
                                                         xr_bt_uuid_t descr_uuid,
                                                         xr_gattc_descr_elem_t *result,
                                                         uint16_t *count);

/**
 * @brief           Find the include service with the given service handle in the gattc cache
 *                  Note: It just get include service from local cache, won't get from remote devices.
 *
 * @param[in]       gattc_if: Gatt client access interface.
 * @param[in]       conn_id: connection ID which identify the server.
 * @param[in]       start_handle: the attribute start handle
 * @param[in]       end_handle: the attribute end handle
 * @param[in]       incl_uuid: the include service uuid
 * @param[out]      result: The pointer to the include service in the given service.
 * @param[inout]   count: input the number of include service want to find,
 *                         it will output the number of include service has been found in the gattc cache with the given service.
 *
 * @return
 *                  - XR_OK: success
 *                  - other: failed
 *
 */
xr_gatt_status_t xr_ble_gattc_get_include_service(xr_gatt_if_t gattc_if,
                                                    uint16_t conn_id,
                                                    uint16_t start_handle,
                                                    uint16_t end_handle,
                                                    xr_bt_uuid_t *incl_uuid,
                                                    xr_gattc_incl_svc_elem_t *result,
                                                    uint16_t *count);


/**
 * @brief           Find the attribute count with the given service or characteristic in the gattc cache
 *
 * @param[in]       gattc_if: Gatt client access interface.
 * @param[in]       conn_id: connection ID which identify the server.
 * @param[in]       type: the attribute type.
 * @param[in]       start_handle: the attribute start handle, if the type is XR_GATT_DB_DESCRIPTOR, this parameter should be ignore
 * @param[in]       end_handle: the attribute end handle, if the type is XR_GATT_DB_DESCRIPTOR, this parameter should be ignore
 * @param[in]       char_handle: the characteristic handle, this parameter valid when the type is XR_GATT_DB_DESCRIPTOR. If the type
 *                               isn't XR_GATT_DB_DESCRIPTOR, this parameter should be ignore.
 * @param[out]      count: output the number of attribute has been found in the gattc cache with the given attribute type.
 *
 * @return
 *                  - XR_OK: success
 *                  - other: failed
 *
 */
xr_gatt_status_t xr_ble_gattc_get_attr_count(xr_gatt_if_t gattc_if,
                                               uint16_t conn_id,
                                               xr_gatt_db_attr_type_t type,
                                               uint16_t start_handle,
                                               uint16_t end_handle,
                                               uint16_t char_handle,
                                               uint16_t *count);

/**
 * @brief           This function is called to get the GATT database.
 *                  Note: It just get attribute data base from local cache, won't get from remote devices.
 *
 * @param[in]       gattc_if: Gatt client access interface.
 * @param[in]       start_handle: the attribute start handle
 * @param[in]       end_handle: the attribute end handle
 * @param[in]       conn_id: connection ID which identify the server.
 * @param[in]       db: output parameter which will contain the GATT database copy.
 *                      Caller is responsible for freeing it.
 * @param[in]       count: number of elements in database.
 *
 * @return
 *                  - XR_OK: success
 *                  - other: failed
 *
 */
xr_gatt_status_t xr_ble_gattc_get_db(xr_gatt_if_t gattc_if, uint16_t conn_id, uint16_t start_handle, uint16_t end_handle,
                                        xr_gattc_db_elem_t *db, uint16_t *count);

/**
 * @brief           This function is called to read a service's characteristics of
 *                  the given characteristic handle
 *
 * @param[in]       gattc_if: Gatt client access interface.
 * @param[in]       conn_id : connection ID.
 * @param[in]       handle : characteritic handle to read.
 * @param[in]       auth_req : authenticate request type
 *
 * @return
 *                  - XR_OK: success
 *                  - other: failed
 *
 */
xr_err_t xr_ble_gattc_read_char (xr_gatt_if_t gattc_if,
                                   uint16_t conn_id,
                                   uint16_t handle,
                                   xr_gatt_auth_req_t auth_req);

/**
 * @brief           This function is called to read a service's characteristics of
 *                  the given characteristic UUID
 *
 * @param[in]       gattc_if: Gatt client access interface.
 * @param[in]       conn_id : connection ID.
 * @param[in]       start_handle : the attribute start handle.
 * @param[in]       end_handle : the attribute end handle
 * @param[in]       uuid : The UUID of attribute which will be read.
 * @param[in]       auth_req : authenticate request type
 *
 * @return
 *                  - XR_OK: success
 *                  - other: failed
 *
 */
xr_err_t xr_ble_gattc_read_by_type (xr_gatt_if_t gattc_if,
                                      uint16_t conn_id,
                                      uint16_t start_handle,
                                      uint16_t end_handle,
                                      xr_bt_uuid_t *uuid,
                                      xr_gatt_auth_req_t auth_req);

/**
 * @brief           This function is called to read multiple characteristic or
 *                  characteristic descriptors.
 *
 * @param[in]       gattc_if: Gatt client access interface.
 * @param[in]       conn_id : connection ID.
 * @param[in]       read_multi : pointer to the read multiple parameter.
 * @param[in]       auth_req : authenticate request type
 *
 * @return
 *                  - XR_OK: success
 *                  - other: failed
 *
 */
xr_err_t xr_ble_gattc_read_multiple(xr_gatt_if_t gattc_if,
                                      uint16_t conn_id, xr_gattc_multi_t *read_multi,
                                      xr_gatt_auth_req_t auth_req);


/**
 * @brief           This function is called to read a characteristics descriptor.
 *
 * @param[in]       gattc_if: Gatt client access interface.
 * @param[in]       conn_id : connection ID.
 * @param[in]       handle : descriptor handle to read.
 * @param[in]       auth_req : authenticate request type
 *
 * @return
 *                  - XR_OK: success
 *                  - other: failed
 *
 */
xr_err_t xr_ble_gattc_read_char_descr (xr_gatt_if_t gattc_if,
                                         uint16_t conn_id,
                                         uint16_t handle,
                                         xr_gatt_auth_req_t auth_req);


/**
 * @brief           This function is called to write characteristic value.
 *
 * @param[in]       gattc_if: Gatt client access interface.
 * @param[in]       conn_id : connection ID.
 * @param[in]       handle : characteristic handle to write.
 * @param[in]       value_len: length of the value to be written.
 * @param[in]       value : the value to be written.
 * @param[in]       write_type : the type of attribute write operation.
 * @param[in]       auth_req : authentication request.
 *
 * @return
 *                  - XR_OK: success
 *                  - other: failed
 *
 */
xr_err_t xr_ble_gattc_write_char( xr_gatt_if_t gattc_if,
                                    uint16_t conn_id,
                                    uint16_t handle,
                                    uint16_t value_len,
                                    uint8_t *value,
                                    xr_gatt_write_type_t write_type,
                                    xr_gatt_auth_req_t auth_req);


/**
 * @brief           This function is called to write characteristic descriptor value.
 *
 * @param[in]       gattc_if: Gatt client access interface.
 * @param[in]       conn_id : connection ID
 * @param[in]       handle : descriptor hadle to write.
 * @param[in]       value_len: length of the value to be written.
 * @param[in]       value : the value to be written.
 * @param[in]       write_type : the type of attribute write operation.
 * @param[in]       auth_req : authentication request.
 *
 * @return
 *                  - XR_OK: success
 *                  - other: failed
 *
 */
xr_err_t xr_ble_gattc_write_char_descr (xr_gatt_if_t gattc_if,
                                         uint16_t conn_id,
                                         uint16_t handle,
                                         uint16_t value_len,
                                         uint8_t *value,
                                         xr_gatt_write_type_t write_type,
                                         xr_gatt_auth_req_t auth_req);


/**
 * @brief           This function is called to prepare write a characteristic value.
 *
 * @param[in]       gattc_if: Gatt client access interface.
 * @param[in]       conn_id : connection ID.
 * @param[in]       handle : characteristic handle to prepare write.
 * @param[in]       offset : offset of the write value.
 * @param[in]       value_len: length of the value to be written.
 * @param[in]       value : the value to be written.
 * @param[in]       auth_req : authentication request.
 *
 * @return
 *                  - XR_OK: success
 *                  - other: failed
 *
 */
xr_err_t xr_ble_gattc_prepare_write(xr_gatt_if_t gattc_if,
                                      uint16_t conn_id,
                                      uint16_t handle,
                                      uint16_t offset,
                                      uint16_t value_len,
                                      uint8_t *value,
                                      xr_gatt_auth_req_t auth_req);


/**
 * @brief           This function is called to prepare write a characteristic descriptor value.
 *
 * @param[in]       gattc_if: Gatt client access interface.
 * @param[in]       conn_id : connection ID.
 * @param[in]       handle : characteristic descriptor handle to prepare write.
 * @param[in]       offset : offset of the write value.
 * @param[in]       value_len: length of the value to be written.
 * @param[in]       value : the value to be written.
 * @param[in]       auth_req : authentication request.
 *
 * @return
 *                  - XR_OK: success
 *                  - other: failed
 *
 */
xr_err_t xr_ble_gattc_prepare_write_char_descr(xr_gatt_if_t gattc_if,
                                                 uint16_t conn_id,
                                                 uint16_t handle,
                                                 uint16_t offset,
                                                 uint16_t value_len,
                                                 uint8_t *value,
                                                 xr_gatt_auth_req_t auth_req);


/**
 * @brief           This function is called to execute write a prepare write sequence.
 *
 * @param[in]       gattc_if: Gatt client access interface.
 * @param[in]       conn_id : connection ID.
 * @param[in]       is_execute : execute or cancel.
 *
 * @return
 *                  - XR_OK: success
 *                  - other: failed
 *
 */
xr_err_t xr_ble_gattc_execute_write (xr_gatt_if_t gattc_if, uint16_t conn_id, bool is_execute);


/**
 * @brief           This function is called to register for notification of a service.
 *
 * @param[in]       gattc_if: Gatt client access interface.
 * @param[in]       server_bda : target GATT server.
 * @param[in]       handle : GATT characteristic handle.
 *
 * @return
 *                  - XR_OK: registration succeeds
 *                  - other: failed
 *
 */
xr_err_t xr_ble_gattc_register_for_notify (xr_gatt_if_t gattc_if,
                                             xr_bd_addr_t server_bda,
                                             uint16_t handle);


/**
 * @brief           This function is called to de-register for notification of a service.
 *
 * @param[in]       gattc_if: Gatt client access interface.
 * @param[in]       server_bda : target GATT server.
 * @param[in]       handle : GATT characteristic handle.
 *
 * @return
 *                  - XR_OK: unregister succeeds
 *                  - other: failed
 *
 */
xr_err_t xr_ble_gattc_unregister_for_notify (xr_gatt_if_t gattc_if,
                                               xr_bd_addr_t server_bda,
                                               uint16_t handle);


/**
* @brief           Refresh the server cache store in the gattc stack of the remote device. If
*                  the device is connected, this API will restart the discovery of service information of the remote device
*
* @param[in]       remote_bda: remote device BD address.
*
* @return
*                  - XR_OK: success
*                  - other: failed
*
*/
xr_err_t xr_ble_gattc_cache_refresh(xr_bd_addr_t remote_bda);

/**
* @brief           Add or delete the associated address with the source address.
*                  Note: The role of this API is mainly when the client side has stored a server-side database, 
*                        when it needs to connect another device, but the device's attribute database is the same 
*                        as the server database stored on the client-side, calling this API can use the database 
*                        that the device has stored used as the peer server database to reduce the attribute 
*                        database search and discovery process and speed up the connection time.
*                        The associated address mains that device want to used the database has stored in the local cache.
*                        The source address mains that device want to share the database to the associated address device.
*
* @param[in]       gattc_if: Gatt client access interface.
* @param[in]       src_addr: the source address which provide the attribute table.
* @param[in]       assoc_addr: the associated device address which went to share the attribute table with the source address.
* @param[in]       is_assoc: true add the associated device address, false remove the associated device address.
* @return
*                  - XR_OK: success
*                  - other: failed
*
*/
xr_err_t xr_ble_gattc_cache_assoc(xr_gatt_if_t gattc_if, xr_bd_addr_t src_addr, 
                                      xr_bd_addr_t assoc_addr, bool is_assoc);
/**
* @brief           Get the address list which has store the attribute table in the gattc cache. There will
*                  callback XR_GATTC_GET_ADDR_LIST_EVT event when get address list complete.
*
* @param[in]       gattc_if: Gatt client access interface.
* @return
*                  - XR_OK: success
*                  - other: failed
*
*/
xr_err_t xr_ble_gattc_cache_get_addr_list(xr_gatt_if_t gattc_if);

/**
* @brief           Clean the service cache of this device in the gattc stack, 
*
* @param[in]       remote_bda: remote device BD address.
*
* @return
*                  - XR_OK: success
*                  - other: failed
*
*/
xr_err_t xr_ble_gattc_cache_clean(xr_bd_addr_t remote_bda);

#ifdef __cplusplus
}
#endif

#endif /* __XR_GATTC_API_H__ */
