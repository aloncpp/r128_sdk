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

#ifndef __XR_GATTS_API_H__
#define __XR_GATTS_API_H__

#include "xr_bt_defs.h"
#include "xr_gatt_defs.h"
#include "xr_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/// GATT Server callback function events
typedef enum {
    XR_GATTS_REG_EVT                 = 0,       /*!< When register application id, the event comes */
    XR_GATTS_READ_EVT                = 1,       /*!< When gatt client request read operation, the event comes */
    XR_GATTS_WRITE_EVT               = 2,       /*!< When gatt client request write operation, the event comes */
    XR_GATTS_EXEC_WRITE_EVT          = 3,       /*!< When gatt client request execute write, the event comes */
    XR_GATTS_MTU_EVT                 = 4,       /*!< When set mtu complete, the event comes */
    XR_GATTS_CONF_EVT                = 5,       /*!< When receive confirm, the event comes */
    XR_GATTS_UNREG_EVT               = 6,       /*!< When unregister application id, the event comes */
    XR_GATTS_CREATE_EVT              = 7,       /*!< When create service complete, the event comes */
    XR_GATTS_ADD_INCL_SRVC_EVT       = 8,       /*!< When add included service complete, the event comes */
    XR_GATTS_ADD_CHAR_EVT            = 9,       /*!< When add characteristic complete, the event comes */
    XR_GATTS_ADD_CHAR_DESCR_EVT      = 10,      /*!< When add descriptor complete, the event comes */
    XR_GATTS_DELETE_EVT              = 11,      /*!< When delete service complete, the event comes */
    XR_GATTS_START_EVT               = 12,      /*!< When start service complete, the event comes */
    XR_GATTS_STOP_EVT                = 13,      /*!< When stop service complete, the event comes */
    XR_GATTS_CONNECT_EVT             = 14,      /*!< When gatt client connect, the event comes */
    XR_GATTS_DISCONNECT_EVT          = 15,      /*!< When gatt client disconnect, the event comes */
    XR_GATTS_OPEN_EVT                = 16,      /*!< When connect to peer, the event comes */
    XR_GATTS_CANCEL_OPEN_EVT         = 17,      /*!< When disconnect from peer, the event comes */
    XR_GATTS_CLOSE_EVT               = 18,      /*!< When gatt server close, the event comes */
    XR_GATTS_LISTEN_EVT              = 19,      /*!< When gatt listen to be connected the event comes */
    XR_GATTS_CONGEST_EVT             = 20,      /*!< When congest happen, the event comes */
    /* following is extra event */
    XR_GATTS_RESPONSE_EVT            = 21,      /*!< When gatt send response complete, the event comes */
    XR_GATTS_CREAT_ATTR_TAB_EVT      = 22,      /*!< When gatt create table complete, the event comes */
    XR_GATTS_SET_ATTR_VAL_EVT        = 23,      /*!< When gatt set attr value complete, the event comes */
    XR_GATTS_SEND_SERVICE_CHANGE_EVT = 24,      /*!< When gatt send service change indication complete, the event comes */
} xr_gatts_cb_event_t;

/**
 * @brief Gatt server callback parameters union
 */
typedef union {
    /**
     * @brief XR_GATTS_REG_EVT
     */
    struct gatts_reg_evt_param {
        xr_gatt_status_t status;       /*!< Operation status */
        uint16_t app_id;                /*!< Application id which input in register API */
    } reg;                              /*!< Gatt server callback param of XR_GATTS_REG_EVT */

    /**
     * @brief XR_GATTS_READ_EVT
     */
    struct gatts_read_evt_param {
        uint16_t conn_id;               /*!< Connection id */
        uint32_t trans_id;              /*!< Transfer id */
        xr_bd_addr_t bda;              /*!< The bluetooth device address which been read */
        uint16_t handle;                /*!< The attribute handle */
        uint16_t offset;                /*!< Offset of the value, if the value is too long */
        bool is_long;                   /*!< The value is too long or not */
        bool need_rsp;                  /*!< The read operation need to do response */
    } read;                             /*!< Gatt server callback param of XR_GATTS_READ_EVT */


    /**
     * @brief XR_GATTS_WRITE_EVT
     */
    struct gatts_write_evt_param {
        uint16_t conn_id;               /*!< Connection id */
        uint32_t trans_id;              /*!< Transfer id */
        xr_bd_addr_t bda;              /*!< The bluetooth device address which been written */
        uint16_t handle;                /*!< The attribute handle */
        uint16_t offset;                /*!< Offset of the value, if the value is too long */
        bool need_rsp;                  /*!< The write operation need to do response */
        bool is_prep;                   /*!< This write operation is prepare write */
        uint16_t len;                   /*!< The write attribute value length */
        uint8_t *value;                 /*!< The write attribute value */
    } write;                            /*!< Gatt server callback param of XR_GATTS_WRITE_EVT */

    /**
     * @brief XR_GATTS_EXEC_WRITE_EVT
     */
    struct gatts_exec_write_evt_param {
        uint16_t conn_id;               /*!< Connection id */
        uint32_t trans_id;              /*!< Transfer id */
        xr_bd_addr_t bda;              /*!< The bluetooth device address which been written */
#define XR_GATT_PREP_WRITE_CANCEL 0x00 /*!< Prepare write flag to indicate cancel prepare write */
#define XR_GATT_PREP_WRITE_EXEC   0x01 /*!< Prepare write flag to indicate execute prepare write */
        uint8_t exec_write_flag;        /*!< Execute write flag */
    } exec_write;                       /*!< Gatt server callback param of XR_GATTS_EXEC_WRITE_EVT */

    /**
     * @brief XR_GATTS_MTU_EVT
     */
    struct gatts_mtu_evt_param {
        uint16_t conn_id;               /*!< Connection id */
        uint16_t mtu;                   /*!< MTU size */
    } mtu;                              /*!< Gatt server callback param of XR_GATTS_MTU_EVT */

    /**
     * @brief XR_GATTS_CONF_EVT
     */
    struct gatts_conf_evt_param {
        xr_gatt_status_t status;       /*!< Operation status */
        uint16_t conn_id;               /*!< Connection id */
        uint16_t handle;                /*!< attribute handle */
        uint16_t len;                   /*!< The indication or notification value length, len is valid when send notification or indication failed */
        uint8_t *value;                 /*!< The indication or notification value , value is valid when send notification or indication failed */
    } conf;                             /*!< Gatt server callback param of XR_GATTS_CONF_EVT (confirm) */

    /**
     * @brief XR_GATTS_UNREG_EVT
     */

    /**
     * @brief XR_GATTS_CREATE_EVT
     */
    struct gatts_create_evt_param {
        xr_gatt_status_t status;       /*!< Operation status */
        uint16_t service_handle;        /*!< Service attribute handle */
        xr_gatt_srvc_id_t service_id;  /*!< Service id, include service uuid and other information */
    } create;                           /*!< Gatt server callback param of XR_GATTS_CREATE_EVT */

    /**
     * @brief XR_GATTS_ADD_INCL_SRVC_EVT
     */
    struct gatts_add_incl_srvc_evt_param {
        xr_gatt_status_t status;       /*!< Operation status */
        uint16_t attr_handle;           /*!< Included service attribute handle */
        uint16_t service_handle;        /*!< Service attribute handle */
    } add_incl_srvc;                    /*!< Gatt server callback param of XR_GATTS_ADD_INCL_SRVC_EVT */

    /**
     * @brief XR_GATTS_ADD_CHAR_EVT
     */
    struct gatts_add_char_evt_param {
        xr_gatt_status_t status;       /*!< Operation status */
        uint16_t attr_handle;           /*!< Characteristic attribute handle */
        uint16_t service_handle;        /*!< Service attribute handle */
        xr_bt_uuid_t char_uuid;        /*!< Characteristic uuid */
    } add_char;                         /*!< Gatt server callback param of XR_GATTS_ADD_CHAR_EVT */

    /**
     * @brief XR_GATTS_ADD_CHAR_DESCR_EVT
     */
    struct gatts_add_char_descr_evt_param {
        xr_gatt_status_t status;       /*!< Operation status */
        uint16_t attr_handle;           /*!< Descriptor attribute handle */
        uint16_t service_handle;        /*!< Service attribute handle */
        xr_bt_uuid_t descr_uuid;       /*!< Characteristic descriptor uuid */
    } add_char_descr;                   /*!< Gatt server callback param of XR_GATTS_ADD_CHAR_DESCR_EVT */

    /**
     * @brief XR_GATTS_DELETE_EVT
     */
    struct gatts_delete_evt_param {
        xr_gatt_status_t status;       /*!< Operation status */
        uint16_t service_handle;        /*!< Service attribute handle */
    } del;                              /*!< Gatt server callback param of XR_GATTS_DELETE_EVT */

    /**
     * @brief XR_GATTS_START_EVT
     */
    struct gatts_start_evt_param {
        xr_gatt_status_t status;       /*!< Operation status */
        uint16_t service_handle;        /*!< Service attribute handle */
    } start;                            /*!< Gatt server callback param of XR_GATTS_START_EVT */

    /**
     * @brief XR_GATTS_STOP_EVT
     */
    struct gatts_stop_evt_param {
        xr_gatt_status_t status;       /*!< Operation status */
        uint16_t service_handle;        /*!< Service attribute handle */
    } stop;                             /*!< Gatt server callback param of XR_GATTS_STOP_EVT */

    /**
     * @brief XR_GATTS_CONNECT_EVT
     */
    struct gatts_connect_evt_param {
        uint16_t conn_id;               /*!< Connection id */
        xr_bd_addr_t remote_bda;       /*!< Remote bluetooth device address */
        xr_gatt_conn_params_t conn_params; /*!< current Connection parameters */
    } connect;                          /*!< Gatt server callback param of XR_GATTS_CONNECT_EVT */

    /**
     * @brief XR_GATTS_DISCONNECT_EVT
     */
    struct gatts_disconnect_evt_param {
        uint16_t conn_id;               /*!< Connection id */
        xr_bd_addr_t remote_bda;       /*!< Remote bluetooth device address */
        xr_gatt_conn_reason_t reason;  /*!< Indicate the reason of disconnection */
    } disconnect;                       /*!< Gatt server callback param of XR_GATTS_DISCONNECT_EVT */

    /**
     * @brief XR_GATTS_OPEN_EVT
     */
    struct gatts_open_evt_param {
        xr_gatt_status_t status;       /*!< Operation status */
    } open;                             /*!< Gatt server callback param of XR_GATTS_OPEN_EVT */

    /**
     * @brief XR_GATTS_CANCEL_OPEN_EVT
     */
    struct gatts_cancel_open_evt_param {
        xr_gatt_status_t status;       /*!< Operation status */
    } cancel_open;                      /*!< Gatt server callback param of XR_GATTS_CANCEL_OPEN_EVT */

    /**
     * @brief XR_GATTS_CLOSE_EVT
     */
    struct gatts_close_evt_param {
        xr_gatt_status_t status;       /*!< Operation status */
        uint16_t conn_id;               /*!< Connection id */
    } close;                            /*!< Gatt server callback param of XR_GATTS_CLOSE_EVT */

    /**
     * @brief XR_GATTS_LISTEN_EVT
     */
    /**
     * @brief XR_GATTS_CONGEST_EVT
     */
    struct gatts_congest_evt_param {
        uint16_t conn_id;               /*!< Connection id */
        bool congested;                 /*!< Congested or not */
    } congest;                          /*!< Gatt server callback param of XR_GATTS_CONGEST_EVT */

    /**
     * @brief XR_GATTS_RESPONSE_EVT
     */
    struct gatts_rsp_evt_param {
        xr_gatt_status_t status;       /*!< Operation status */
        uint16_t handle;                /*!< Attribute handle which send response */
    } rsp;                              /*!< Gatt server callback param of XR_GATTS_RESPONSE_EVT */

    /**
     * @brief XR_GATTS_CREAT_ATTR_TAB_EVT
     */
    struct gatts_add_attr_tab_evt_param{
        xr_gatt_status_t status;       /*!< Operation status */
        xr_bt_uuid_t svc_uuid;         /*!< Service uuid type */
        uint8_t svc_inst_id;            /*!< Service id */
        uint16_t num_handle;            /*!< The number of the attribute handle to be added to the gatts database */
        uint16_t *handles;              /*!< The number to the handles */
    } add_attr_tab;                     /*!< Gatt server callback param of XR_GATTS_CREAT_ATTR_TAB_EVT */


   /**
    * @brief XR_GATTS_SET_ATTR_VAL_EVT
    */
    struct gatts_set_attr_val_evt_param{
        uint16_t srvc_handle;           /*!< The service handle */
        uint16_t attr_handle;           /*!< The attribute  handle */
        xr_gatt_status_t status;       /*!< Operation status*/
    } set_attr_val;                     /*!< Gatt server callback param of XR_GATTS_SET_ATTR_VAL_EVT */

    /**
    * @brief XR_GATTS_SEND_SERVICE_CHANGE_EVT
    */
    struct gatts_send_service_change_evt_param{
        xr_gatt_status_t status;       /*!< Operation status*/
    } service_change;                    /*!< Gatt server callback param of XR_GATTS_SEND_SERVICE_CHANGE_EVT */

} xr_ble_gatts_cb_param_t;

/**
 * @brief GATT Server callback function type
 * @param event : Event type
 * @param gatts_if : GATT server access interface, normally
 *                   different gatts_if correspond to different profile
 * @param param : Point to callback parameter, currently is union type
 */
typedef void (* xr_gatts_cb_t)(xr_gatts_cb_event_t event, xr_gatt_if_t gatts_if, xr_ble_gatts_cb_param_t *param);

/**
 * @brief           This function is called to register application callbacks
 *                  with BTA GATTS module.
 *
 * @return
 *                  - XR_OK : success
 *                  - other  : failed
 *
 */
xr_err_t xr_ble_gatts_register_callback(xr_gatts_cb_t callback);

/**
 * @brief           This function is called to register application identifier
 *
 * @return
 *                  - XR_OK : success
 *                  - other  : failed
 *
 */
xr_err_t xr_ble_gatts_app_register(uint16_t app_id);



/**
 * @brief           unregister with GATT Server.
 *
 * @param[in]       gatts_if: GATT server access interface
 * @return
 *                  - XR_OK : success
 *                  - other  : failed
 *
 */
xr_err_t xr_ble_gatts_app_unregister(xr_gatt_if_t gatts_if);


/**
 * @brief           Create a service. When service creation is done, a callback
 *                  event XR_GATTS_CREATE_EVT is called to report status
 *                  and service ID to the profile. The service ID obtained in
 *                  the callback function needs to be used when adding included
 *                  service and characteristics/descriptors into the service.
 *
 * @param[in]       gatts_if: GATT server access interface
 * @param[in]       service_id: service ID.
 * @param[in]       num_handle: number of handle requested for this service.
 *
 * @return
 *                  - XR_OK : success
 *                  - other  : failed
 *
 */
xr_err_t xr_ble_gatts_create_service(xr_gatt_if_t gatts_if,
                                       xr_gatt_srvc_id_t *service_id, uint16_t num_handle);


/**
 * @brief               Create a service attribute tab. 
 * @param[in]       gatts_attr_db: the pointer to the service attr tab
 * @param[in]       gatts_if: GATT server access interface
 * @param[in]       max_nb_attr: the number of attribute to be added to the service database.
 * @param[in]       srvc_inst_id: the instance id of the service
 *
 * @return
 *                  - XR_OK : success
 *                  - other  : failed
 *
 */
xr_err_t xr_ble_gatts_create_attr_tab(const xr_gatts_attr_db_t *gatts_attr_db, 
                                            xr_gatt_if_t gatts_if,
                                            uint8_t max_nb_attr,
                                            uint8_t srvc_inst_id);
/**
 * @brief           This function is called to add an included service. This function have to be called between 
 *                  'xr_ble_gatts_create_service' and 'xr_ble_gatts_add_char'.  After included
 *                  service is included, a callback event XR_GATTS_ADD_INCL_SRVC_EVT
 *                  is reported the included service ID.
 *
 * @param[in]       service_handle: service handle to which this included service is to
 *                  be added.
 * @param[in]       included_service_handle: the service ID to be included.
 *
 * @return
 *                  - XR_OK : success
 *                  - other  : failed
 *
 */
xr_err_t xr_ble_gatts_add_included_service(uint16_t service_handle, uint16_t included_service_handle);



/**
 * @brief           This function is called to add a characteristic into a service.
 *
 * @param[in]       service_handle: service handle to which this included service is to
 *                  be added.
 * @param[in]       char_uuid : Characteristic UUID.
 * @param[in]       perm      : Characteristic value declaration attribute permission.
 * @param[in]       property  : Characteristic Properties
 * @param[in]       char_val    : Characteristic value 
 * @param[in]       control : attribute response control byte
 *
 * @return
 *                  - XR_OK : success
 *                  - other  : failed
 *
 */
xr_err_t xr_ble_gatts_add_char(uint16_t service_handle,  xr_bt_uuid_t  *char_uuid,
                                 xr_gatt_perm_t perm, xr_gatt_char_prop_t property, xr_attr_value_t *char_val,
                                 xr_attr_control_t *control);


/**
 * @brief           This function is called to add characteristic descriptor. When
 *                  it's done, a callback event XR_GATTS_ADD_DESCR_EVT is called
 *                  to report the status and an ID number for this descriptor.
 *
 * @param[in]       service_handle: service handle to which this characteristic descriptor is to
 *                              be added.
 * @param[in]       perm: descriptor access permission.
 * @param[in]       descr_uuid: descriptor UUID.
 * @param[in]       char_descr_val  : Characteristic descriptor value 
 * @param[in]       control : attribute response control byte
 * @return
 *                  - XR_OK : success
 *                  - other  : failed
 *
 */
xr_err_t xr_ble_gatts_add_char_descr (uint16_t service_handle,
                                        xr_bt_uuid_t   *descr_uuid,
                                        xr_gatt_perm_t perm, xr_attr_value_t *char_descr_val,
                                        xr_attr_control_t *control);



/**
 * @brief           This function is called to delete a service. When this is done,
 *                  a callback event XR_GATTS_DELETE_EVT is report with the status.
 *
 * @param[in]       service_handle: service_handle to be deleted.
 *
 * @return
 *                  - XR_OK : success
 *                  - other  : failed
 *
 */
xr_err_t xr_ble_gatts_delete_service(uint16_t service_handle);



/**
 * @brief           This function is called to start a service.
 *
 * @param[in]       service_handle: the service handle to be started.
 *
 * @return
 *                  - XR_OK : success
 *                  - other  : failed
 *
 */
xr_err_t xr_ble_gatts_start_service(uint16_t service_handle);



/**
 * @brief           This function is called to stop a service.
 *
 * @param[in]       service_handle - service to be topped.
 *
 * @return
 *                  - XR_OK : success
 *                  - other  : failed
 *
 */
xr_err_t xr_ble_gatts_stop_service(uint16_t service_handle);



/**
 * @brief           Send indicate or notify to GATT client.
 *                  Set param need_confirm as false will send notification, otherwise indication.
 *
 * @param[in]       gatts_if: GATT server access interface
 * @param[in]       conn_id - connection id to indicate.
 * @param[in]       attr_handle - attribute handle to indicate.
 * @param[in]       value_len - indicate value length.
 * @param[in]       value: value to indicate.
 * @param[in]       need_confirm - Whether a confirmation is required.
 *                  false sends a GATT notification, true sends a GATT indication.
 *
 * @return
 *                  - XR_OK : success
 *                  - other  : failed
 *
 */
xr_err_t xr_ble_gatts_send_indicate(xr_gatt_if_t gatts_if, uint16_t conn_id, uint16_t attr_handle,
                                      uint16_t value_len, uint8_t *value, bool need_confirm);


/**
 * @brief           This function is called to send a response to a request.
 *
 * @param[in]       gatts_if: GATT server access interface
 * @param[in]       conn_id - connection identifier.
 * @param[in]       trans_id - transfer id
 * @param[in]       status - response status
 * @param[in]       rsp - response data.
 *
 * @return
 *                  - XR_OK : success
 *                  - other  : failed
 *
 */
xr_err_t xr_ble_gatts_send_response(xr_gatt_if_t gatts_if, uint16_t conn_id, uint32_t trans_id,
                                      xr_gatt_status_t status, xr_gatt_rsp_t *rsp);


/**
 * @brief           This function is called to set the attribute value by the application
 *
 * @param[in]       attr_handle: the attribute handle which to be set
 * @param[in]       length: the value length
 * @param[in]       value: the pointer to the attribute value
 *
 * @return
 *                  - XR_OK : success
 *                  - other  : failed
 *
 */
xr_err_t xr_ble_gatts_set_attr_value(uint16_t attr_handle, uint16_t length, const uint8_t *value);

/**
 * @brief       Retrieve attribute value
 *
 * @param[in]   attr_handle: Attribute handle.
 * @param[out]  length: pointer to the attribute value length
 * @param[out]  value:  Pointer to attribute value payload, the value cannot be modified by user
 *
 * @return
 *                  - XR_GATT_OK : success
 *                  - other  : failed
 *
 */
xr_gatt_status_t xr_ble_gatts_get_attr_value(uint16_t attr_handle, uint16_t *length, const uint8_t **value);


/**
 * @brief           Open a direct open connection or add a background auto connection
 *
 * @param[in]       gatts_if: GATT server access interface
 * @param[in]       remote_bda: remote device bluetooth device address.
 * @param[in]       is_direct: direct connection or background auto connection
 *
 * @return
 *                  - XR_OK : success
 *                  - other  : failed
 *
 */
xr_err_t xr_ble_gatts_open(xr_gatt_if_t gatts_if, xr_bd_addr_t remote_bda, bool is_direct);

/**
 * @brief           Close a connection  a remote device.
 *
 * @param[in]       gatts_if: GATT server access interface
 * @param[in]       conn_id: connection ID to be closed.
 *
 * @return
 *                  - XR_OK : success
 *                  - other  : failed
 *
 */
xr_err_t xr_ble_gatts_close(xr_gatt_if_t gatts_if, uint16_t conn_id);

/**
 * @brief           Send service change indication
 *
 * @param[in]       gatts_if: GATT server access interface
 * @param[in]       remote_bda: remote device bluetooth device address.
 *                  If remote_bda is NULL then it will send service change
 *                  indication to all the connected devices and if not then
 *                  to a specific device
 *
 * @return
 *                  - XR_OK : success
 *                  - other  : failed
 *
 */
xr_err_t xr_ble_gatts_send_service_change_indication(xr_gatt_if_t gatts_if, xr_bd_addr_t remote_bda);

#ifdef __cplusplus
}
#endif

#endif /* __XR_GATTS_API_H__ */
