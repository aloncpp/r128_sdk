// Copyright 2017-2019 Espressif Systems (Shanghai) PTE LTD
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

#ifndef _XR_BLE_MESH_HEALTH_MODEL_API_H_
#define _XR_BLE_MESH_HEALTH_MODEL_API_H_

#include "xr_ble_mesh_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @def    XR_BLE_MESH_MODEL_HEALTH_SRV
 *
 *  @brief  Define a new Health Server Model.
 *
 *  @note   The Health Server Model can only be included by a Primary Element.
 *
 *  @param  srv Pointer to the unique struct xr_ble_mesh_health_srv_t.
 *  @param  pub Pointer to the unique struct xr_ble_mesh_model_pub_t.
 *
 *  @return New Health Server Model instance.
 */
#define XR_BLE_MESH_MODEL_HEALTH_SRV(srv, pub)                           \
        XR_BLE_MESH_SIG_MODEL(XR_BLE_MESH_MODEL_ID_HEALTH_SRV,          \
                           NULL, pub, srv)

/** @def    XR_BLE_MESH_MODEL_HEALTH_CLI
 *
 *  @brief  Define a new Health Client Model.
 *
 *  @note   This API needs to be called for each element on which
 *          the application needs to have a Health Client Model.
 *
 *  @param  cli_data Pointer to the unique struct xr_ble_mesh_client_t.
 *
 *  @return New Health Client Model instance.
 */
#define XR_BLE_MESH_MODEL_HEALTH_CLI(cli_data)                           \
        XR_BLE_MESH_SIG_MODEL(XR_BLE_MESH_MODEL_ID_HEALTH_CLI,          \
                           NULL, NULL, cli_data)

/** @def XR_BLE_MESH_HEALTH_PUB_DEFINE
 *
 *  A helper to define a health publication context
 *
 *  @param _name Name given to the publication context variable.
 *  @param _max  Maximum number of faults the element can have.
 *  @param _role Role of the device which contains the model.
 */
#define XR_BLE_MESH_HEALTH_PUB_DEFINE(_name, _max, _role) \
        XR_BLE_MESH_MODEL_PUB_DEFINE(_name, (1 + 3 + (_max)), _role)

/**
 * SIG identifier of Health Fault Test.
 * 0x01 ~ 0xFF: Vendor Specific Test.
 */
#define XR_BLE_MESH_HEALTH_STANDARD_TEST               0x00

/**
 * Fault values of Health Fault Test.
 * 0x33 ~ 0x7F: Reserved for Future Use.
 * 0x80 ~ 0xFF: Vendor Specific Warning/Error.
 */
#define XR_BLE_MESH_NO_FAULT                           0x00
#define XR_BLE_MESH_BATTERY_LOW_WARNING                0x01
#define XR_BLE_MESH_BATTERY_LOW_ERROR                  0x02
#define XR_BLE_MESH_SUPPLY_VOLTAGE_TOO_LOW_WARNING     0x03
#define XR_BLE_MESH_SUPPLY_VOLTAGE_TOO_LOW_ERROR       0x04
#define XR_BLE_MESH_SUPPLY_VOLTAGE_TOO_HIGH_WARNING    0x05
#define XR_BLE_MESH_SUPPLY_VOLTAGE_TOO_HIGH_ERROR      0x06
#define XR_BLE_MESH_POWER_SUPPLY_INTERRUPTED_WARNING   0x07
#define XR_BLE_MESH_POWER_SUPPLY_INTERRUPTED_ERROR     0x08
#define XR_BLE_MESH_NO_LOAD_WARNING                    0x09
#define XR_BLE_MESH_NO_LOAD_ERROR                      0x0A
#define XR_BLE_MESH_OVERLOAD_WARNING                   0x0B
#define XR_BLE_MESH_OVERLOAD_ERROR                     0x0C
#define XR_BLE_MESH_OVERHEAT_WARNING                   0x0D
#define XR_BLE_MESH_OVERHEAT_ERROR                     0x0E
#define XR_BLE_MESH_CONDENSATION_WARNING               0x0F
#define XR_BLE_MESH_CONDENSATION_ERROR                 0x10
#define XR_BLE_MESH_VIBRATION_WARNING                  0x11
#define XR_BLE_MESH_VIBRATION_ERROR                    0x12
#define XR_BLE_MESH_CONFIGURATION_WARNING              0x13
#define XR_BLE_MESH_CONFIGURATION_ERROR                0x14
#define XR_BLE_MESH_ELEMENT_NOT_CALIBRATED_WARNING     0x15
#define XR_BLE_MESH_ELEMENT_NOT_CALIBRATED_ERROR       0x16
#define XR_BLE_MESH_MEMORY_WARNING                     0x17
#define XR_BLE_MESH_MEMORY_ERROR                       0x18
#define XR_BLE_MESH_SELF_TEST_WARNING                  0x19
#define XR_BLE_MESH_SELF_TEST_ERROR                    0x1A
#define XR_BLE_MESH_INPUT_TOO_LOW_WARNING              0x1B
#define XR_BLE_MESH_INPUT_TOO_LOW_ERROR                0x1C
#define XR_BLE_MESH_INPUT_TOO_HIGH_WARNING             0x1D
#define XR_BLE_MESH_INPUT_TOO_HIGH_ERROR               0x1E
#define XR_BLE_MESH_INPUT_NO_CHANGE_WARNING            0x1F
#define XR_BLE_MESH_INPUT_NO_CHANGE_ERROR              0x20
#define XR_BLE_MESH_ACTUATOR_BLOCKED_WARNING           0x21
#define XR_BLE_MESH_ACTUATOR_BLOCKED_ERROR             0x22
#define XR_BLE_MESH_HOUSING_OPENED_WARNING             0x23
#define XR_BLE_MESH_HOUSING_OPENED_ERROR               0x24
#define XR_BLE_MESH_TAMPER_WARNING                     0x25
#define XR_BLE_MESH_TAMPER_ERROR                       0x26
#define XR_BLE_MESH_DEVICE_MOVED_WARNING               0x27
#define XR_BLE_MESH_DEVICE_MOVED_ERROR                 0x28
#define XR_BLE_MESH_DEVICE_DROPPED_WARNING             0x29
#define XR_BLE_MESH_DEVICE_DROPPED_ERROR               0x2A
#define XR_BLE_MESH_OVERFLOW_WARNING                   0x2B
#define XR_BLE_MESH_OVERFLOW_ERROR                     0x2C
#define XR_BLE_MESH_EMPTY_WARNING                      0x2D
#define XR_BLE_MESH_EMPTY_ERROR                        0x2E
#define XR_BLE_MESH_INTERNAL_BUS_WARNING               0x2F
#define XR_BLE_MESH_INTERNAL_BUS_ERROR                 0x30
#define XR_BLE_MESH_MECHANISM_JAMMED_WARNING           0x31
#define XR_BLE_MESH_MECHANISM_JAMMED_ERROR             0x32

/** BLE Mesh Health Server callback */
typedef struct {
    /** Clear health registered faults. Initialized by the stack. */
    xr_ble_mesh_cb_t fault_clear;

    /** Run a specific health test. Initialized by the stack. */
    xr_ble_mesh_cb_t fault_test;

    /** Health attention on callback. Initialized by the stack. */
    xr_ble_mesh_cb_t attention_on;

    /** Health attention off callback. Initialized by the stack. */
    xr_ble_mesh_cb_t attention_off;
} xr_ble_mesh_health_srv_cb_t;

#define XR_BLE_MESH_HEALTH_FAULT_ARRAY_SIZE    32

/** BLE Mesh Health Server test Context */
typedef struct {
    uint8_t  id_count;          /*!< Number of Health self-test ID */
    const uint8_t *test_ids;    /*!< Array of Health self-test IDs */
    uint16_t company_id;        /*!< Company ID used to identify the Health Fault state */
    uint8_t  prev_test_id;      /*!< Current test ID of the health fault test */
    uint8_t  current_faults[XR_BLE_MESH_HEALTH_FAULT_ARRAY_SIZE];      /*!< Array of current faults */
    uint8_t  registered_faults[XR_BLE_MESH_HEALTH_FAULT_ARRAY_SIZE];   /*!< Array of registered faults */
} __attribute__((packed)) xr_ble_mesh_health_test_t;

/** BLE Mesh Health Server Model Context */
typedef struct {
    /** Pointer to Health Server Model */
    xr_ble_mesh_model_t *model;

    /** Health callback struct */
    xr_ble_mesh_health_srv_cb_t health_cb;

    /** Attention Timer state */
    struct k_delayed_work attention_timer;

    /** Attention Timer start flag */
    bool attention_timer_start;

    /** Health Server fault test */
    xr_ble_mesh_health_test_t health_test;
} xr_ble_mesh_health_srv_t;

/** Parameter of Health Fault Get */
typedef struct {
    uint16_t company_id;    /*!< Bluetooth assigned 16-bit Company ID */
} xr_ble_mesh_health_fault_get_t;

/** Parameter of Health Attention Set */
typedef struct {
    uint8_t attention;      /*!< Value of the Attention Timer state */
} xr_ble_mesh_health_attention_set_t;

/** Parameter of Health Period Set */
typedef struct {
    uint8_t fast_period_divisor;    /*!< Divider for the Publish Period */
} xr_ble_mesh_health_period_set_t;

/** Parameter of Health Fault Test */
typedef struct {
    uint16_t company_id;    /*!< Bluetooth assigned 16-bit Company ID */
    uint8_t  test_id;       /*!< ID of a specific test to be performed */
} xr_ble_mesh_health_fault_test_t;

/** Parameter of Health Fault Clear */
typedef struct {
    uint16_t company_id;    /*!< Bluetooth assigned 16-bit Company ID */
} xr_ble_mesh_health_fault_clear_t;

/**
 * @brief For XR_BLE_MESH_MODEL_OP_HEALTH_FAULT_GET
 *            XR_BLE_MESH_MODEL_OP_ATTENTION_GET
 *            XR_BLE_MESH_MODEL_OP_HEALTH_PERIOD_GET
 * the get_state parameter in the xr_ble_mesh_health_client_get_state function should not be set to NULL.
 */
typedef union {
    xr_ble_mesh_health_fault_get_t fault_get;          /*!< For XR_BLE_MESH_MODEL_OP_HEALTH_FAULT_GET. */
} xr_ble_mesh_health_client_get_state_t;

/**
 * @brief For XR_BLE_MESH_MODEL_OP_HEALTH_FAULT_CLEAR
 *            XR_BLE_MESH_MODEL_OP_HEALTH_FAULT_CLEAR_UNACK
 *            XR_BLE_MESH_MODEL_OP_HEALTH_FAULT_TEST
 *            XR_BLE_MESH_MODEL_OP_HEALTH_FAULT_TEST_UNACK
 *            XR_BLE_MESH_MODEL_OP_HEALTH_PERIOD_SET
 *            XR_BLE_MESH_MODEL_OP_HEALTH_PERIOD_SET_UNACK
 *            XR_BLE_MESH_MODEL_OP_ATTENTION_SET
 *            XR_BLE_MESH_MODEL_OP_ATTENTION_SET_UNACK
 * the set_state parameter in the xr_ble_mesh_health_client_set_state function should not be set to NULL.
 */
typedef union {
    xr_ble_mesh_health_attention_set_t attention_set;    /*!< For XR_BLE_MESH_MODEL_OP_ATTENTION_SET or XR_BLE_MESH_MODEL_OP_ATTENTION_SET_UNACK. */
    xr_ble_mesh_health_period_set_t    period_set;       /*!< For XR_BLE_MESH_MODEL_OP_HEALTH_PERIOD_SET or XR_BLE_MESH_MODEL_OP_HEALTH_PERIOD_SET_UNACK. */
    xr_ble_mesh_health_fault_test_t    fault_test;       /*!< For XR_BLE_MESH_MODEL_OP_HEALTH_FAULT_TEST or XR_BLE_MESH_MODEL_OP_HEALTH_FAULT_TEST_UNACK. */
    xr_ble_mesh_health_fault_clear_t   fault_clear;      /*!< For XR_BLE_MESH_MODEL_OP_HEALTH_FAULT_CLEAR or XR_BLE_MESH_MODEL_OP_HEALTH_FAULT_CLEAR_UNACK. */
} xr_ble_mesh_health_client_set_state_t;

/** Parameters of Health Current Status */
typedef struct {
    uint8_t  test_id;                       /*!< ID of a most recently performed test */
    uint16_t company_id;                    /*!< Bluetooth assigned 16-bit Company ID */
    struct net_buf_simple *fault_array;     /*!< FaultArray field contains a sequence of 1-octet fault values */
} xr_ble_mesh_health_current_status_cb_t;

/** Parameters of Health Fault Status */
typedef struct {
    uint8_t  test_id;                       /*!< ID of a most recently performed test */
    uint16_t company_id;                    /*!< Bluetooth assigned 16-bit Company ID */
    struct net_buf_simple *fault_array;     /*!< FaultArray field contains a sequence of 1-octet fault values */
} xr_ble_mesh_health_fault_status_cb_t;

/** Parameter of Health Period Status */
typedef struct {
    uint8_t fast_period_divisor;            /*!< Divider for the Publish Period */
} xr_ble_mesh_health_period_status_cb_t;

/** Parameter of Health Attention Status */
typedef struct {
    uint8_t attention;                      /*!< Value of the Attention Timer state */
} xr_ble_mesh_health_attention_status_cb_t;

/**
 * @brief Health Client Model received message union
 */
typedef union {
    xr_ble_mesh_health_current_status_cb_t   current_status;       /*!< The health current status value */
    xr_ble_mesh_health_fault_status_cb_t     fault_status;         /*!< The health fault status value */
    xr_ble_mesh_health_period_status_cb_t    period_status;        /*!< The health period status value */
    xr_ble_mesh_health_attention_status_cb_t attention_status;     /*!< The health attention status value */
} xr_ble_mesh_health_client_common_cb_param_t;

/** Health Client Model callback parameters */
typedef struct {
    int error_code;                                         /*!< Appropriate error code */
    xr_ble_mesh_client_common_param_t          *params;    /*!< The client common parameters. */
    xr_ble_mesh_health_client_common_cb_param_t status_cb; /*!< The health message status callback values */
} xr_ble_mesh_health_client_cb_param_t;

/** This enum value is the event of Health Client Model */
typedef enum {
    XR_BLE_MESH_HEALTH_CLIENT_GET_STATE_EVT,
    XR_BLE_MESH_HEALTH_CLIENT_SET_STATE_EVT,
    XR_BLE_MESH_HEALTH_CLIENT_PUBLISH_EVT,
    XR_BLE_MESH_HEALTH_CLIENT_TIMEOUT_EVT,
    XR_BLE_MESH_HEALTH_CLIENT_EVT_MAX,
} xr_ble_mesh_health_client_cb_event_t;

/** Parameter of publishing Health Current Status completion event */
typedef struct {
    int error_code;                 /*!< The result of publishing Health Current Status */
    xr_ble_mesh_elem_t *element;   /*!< Pointer to the element which contains the Health Server Model */
} xr_ble_mesh_health_fault_update_comp_cb_t;

/** Parameters of Health Fault Clear event */
typedef struct {
    xr_ble_mesh_model_t *model;    /*!< Pointer to the Health Server Model */
    uint16_t company_id;            /*!< Bluetooth assigned 16-bit Company ID */
} xr_ble_mesh_health_fault_clear_cb_t;

/** Parameters of Health Fault Test event */
typedef struct {
    xr_ble_mesh_model_t *model;    /*!< Pointer to the Health Server Model */
    uint8_t  test_id;               /*!< ID of a specific test to be performed */
    uint16_t company_id;            /*!< Bluetooth assigned 16-bit Company ID */
} xr_ble_mesh_health_fault_test_cb_t;

/** Parameter of Health Attention On event */
typedef struct {
    xr_ble_mesh_model_t *model;    /*!< Pointer to the Health Server Model */
    uint8_t time;                   /*!< Duration of attention timer on (in seconds) */
} xr_ble_mesh_health_attention_on_cb_t;

/** Parameter of Health Attention Off event */
typedef struct {
    xr_ble_mesh_model_t *model;    /*!< Pointer to the Health Server Model */
} xr_ble_mesh_health_attention_off_cb_t;

/**
 * @brief Health Server Model callback parameters union
 */
typedef union {
    xr_ble_mesh_health_fault_update_comp_cb_t  fault_update_comp;  /*!< XR_BLE_MESH_HEALTH_SERVER_FAULT_UPDATE_COMP_EVT */
    xr_ble_mesh_health_fault_clear_cb_t        fault_clear;        /*!< XR_BLE_MESH_HEALTH_SERVER_FAULT_CLEAR_EVT */
    xr_ble_mesh_health_fault_test_cb_t         fault_test;         /*!< XR_BLE_MESH_HEALTH_SERVER_FAULT_TEST_EVT */
    xr_ble_mesh_health_attention_on_cb_t       attention_on;       /*!< XR_BLE_MESH_HEALTH_SERVER_ATTENTION_ON_EVT */
    xr_ble_mesh_health_attention_off_cb_t      attention_off;      /*!< XR_BLE_MESH_HEALTH_SERVER_ATTENTION_OFF_EVT */
} xr_ble_mesh_health_server_cb_param_t;

/** This enum value is the event of Health Server Model */
typedef enum {
    XR_BLE_MESH_HEALTH_SERVER_FAULT_UPDATE_COMP_EVT,
    XR_BLE_MESH_HEALTH_SERVER_FAULT_CLEAR_EVT,
    XR_BLE_MESH_HEALTH_SERVER_FAULT_TEST_EVT,
    XR_BLE_MESH_HEALTH_SERVER_ATTENTION_ON_EVT,
    XR_BLE_MESH_HEALTH_SERVER_ATTENTION_OFF_EVT,
    XR_BLE_MESH_HEALTH_SERVER_EVT_MAX,
} xr_ble_mesh_health_server_cb_event_t;

/**
 *  @brief Bluetooth Mesh Health Client and Server Model function.
 */

/**
 * @brief   Health Client Model callback function type
 * @param   event: Event type
 * @param   param: Pointer to callback parameter
 */
typedef void (* xr_ble_mesh_health_client_cb_t)(xr_ble_mesh_health_client_cb_event_t event,
                                                 xr_ble_mesh_health_client_cb_param_t *param);

/**
 * @brief   Health Server Model callback function type
 * @param   event: Event type
 * @param   param: Pointer to callback parameter
 */
typedef void (* xr_ble_mesh_health_server_cb_t)(xr_ble_mesh_health_server_cb_event_t event,
                                                 xr_ble_mesh_health_server_cb_param_t *param);

/**
 * @brief         Register BLE Mesh Health Model callback, the callback will report Health Client & Server Model events.
 *
 * @param[in]     callback: Pointer to the callback function.
 *
 * @return        XR_OK on success or error code otherwise.
 *
 */
xr_err_t xr_ble_mesh_register_health_client_callback(xr_ble_mesh_health_client_cb_t callback);

/**
 * @brief         Register BLE Mesh Health Server Model callback.
 *
 * @param[in]     callback: Pointer to the callback function.
 *
 * @return        XR_OK on success or error code otherwise.
 *
 */
xr_err_t xr_ble_mesh_register_health_server_callback(xr_ble_mesh_health_server_cb_t callback);

/**
 * @brief         This function is called to get the Health Server states using the Health Client Model get messages.
 *
 * @note          If you want to find the opcodes and corresponding meanings accepted by this API,
 *                please refer to xr_ble_mesh_opcode_health_client_get_t in xr_ble_mesh_defs.h
 *
 * @param[in]     params:    Pointer to BLE Mesh common client parameters.
 * @param[in]     get_state: Pointer to a union, each kind of opcode corresponds to one structure inside.
 *                           Shall not be set to NULL.
 *
 * @return        XR_OK on success or error code otherwise.
 *
 */
xr_err_t xr_ble_mesh_health_client_get_state(xr_ble_mesh_client_common_param_t *params,
                                               xr_ble_mesh_health_client_get_state_t *get_state);

/**
 * @brief         This function is called to set the Health Server states using the Health Client Model set messages.
 *
 * @note          If you want to find the opcodes and corresponding meanings accepted by this API,
 *                please refer to xr_ble_mesh_opcode_health_client_set_t in xr_ble_mesh_defs.h
 *
 * @param[in]     params:    Pointer to BLE Mesh common client parameters.
 * @param[in]     set_state: Pointer to a union, each kind of opcode corresponds to one structure inside.
 *                           Shall not be set to NULL.
 *
 * @return        XR_OK on success or error code otherwise.
 *
 */
xr_err_t xr_ble_mesh_health_client_set_state(xr_ble_mesh_client_common_param_t *params,
                                               xr_ble_mesh_health_client_set_state_t *set_state);

/**
 * @brief         This function is called by the Health Server Model to update the context of its Health Current status.
 *
 * @param[in]     element: The element to which the Health Server Model belongs.
 *
 * @return        XR_OK on success or error code otherwise.
 *
 */
xr_err_t xr_ble_mesh_health_server_fault_update(xr_ble_mesh_elem_t *element);

#ifdef __cplusplus
}
#endif

#endif /* _XR_BLE_MESH_HEALTH_MODEL_API_H_ */
