// Copyright 2019 Espressif Systems (Shanghai) PTE LTD
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

#ifndef __XR_HF_AG_API_H__
#define __XR_HF_AG_API_H__

#include "xr_err.h"
#include "xr_bt_defs.h"
#include "xr_hf_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* features masks of HF AG  */
#define XR_HF_PEER_FEAT_3WAY       0x01        /* Three-way calling */
#define XR_HF_PEER_FEAT_ECNR       0x02        /* Echo cancellation and/or noise reduction */
#define XR_HF_PEER_FEAT_VREC       0x04        /* Voice recognition */
#define XR_HF_PEER_FEAT_INBAND     0x08        /* In-band ring tone */
#define XR_HF_PEER_FEAT_VTAG       0x10        /* Attach a phone number to a voice tag */
#define XR_HF_PEER_FEAT_REJECT     0x20        /* Ability to reject incoming call */
#define XR_HF_PEER_FEAT_ECS        0x40        /* Enhanced Call Status */
#define XR_HF_PEER_FEAT_ECC        0x80        /* Enhanced Call Control */
#define XR_HF_PEER_FEAT_EXTERR    0x100        /* Extended error codes */
#define XR_HF_PEER_FEAT_CODEC     0x200        /* Codec Negotiation */

/* CHLD feature masks of HF AG */
#define XR_HF_CHLD_FEAT_REL           0x01       /* 0  Release waiting call or held calls */
#define XR_HF_CHLD_FEAT_REL_ACC       0x02       /* 1  Release active calls and accept other waiting or held call */
#define XR_HF_CHLD_FEAT_REL_X         0x04       /* 1x Release specified active call only */
#define XR_HF_CHLD_FEAT_HOLD_ACC      0x08       /* 2  Active calls on hold and accept other waiting or held call */
#define XR_HF_CHLD_FEAT_PRIV_X        0x10       /* 2x Request private mode with specified call(put the rest on hold) */
#define XR_HF_CHLD_FEAT_MERGE         0x20       /* 3  Add held call to multiparty */
#define XR_HF_CHLD_FEAT_MERGE_DETACH  0x40       /* 4  Connect two calls and leave(disconnect from multiparty) */

/// HF callback events
typedef enum
{
    XR_HF_CONNECTION_STATE_EVT = 0,          /*!< Connection state changed event */
    XR_HF_AUDIO_STATE_EVT,                   /*!< Audio connection state change event */
    XR_HF_BVRA_RESPONSE_EVT,                 /*!< Voice recognition state change event */
    XR_HF_VOLUME_CONTROL_EVT,                /*!< Audio volume control command from HF Client, provided by +VGM or +VGS message */

    XR_HF_UNAT_RESPONSE_EVT,                 /*!< Unknown AT cmd Response*/
    XR_HF_IND_UPDATE_EVT,                    /*!< Indicator Update Event*/
    XR_HF_CIND_RESPONSE_EVT,                 /*!< Call And Device Indicator Response*/
    XR_HF_COPS_RESPONSE_EVT,                 /*!< Current operator information */
    XR_HF_CLCC_RESPONSE_EVT,                 /*!< List of current calls notification */
    XR_HF_CNUM_RESPONSE_EVT,                 /*!< Subscriber information response from HF Client */
    XR_HF_VTS_RESPONSE_EVT,                  /*!< Enable or not DTMF */
    XR_HF_NREC_RESPONSE_EVT,                 /*!< Enable or not NREC */

    XR_HF_ATA_RESPONSE_EVT,                  /*!< Answer an Incoming Call */
    XR_HF_CHUP_RESPONSE_EVT,                 /*!< Reject an Incoming Call */
    XR_HF_DIAL_EVT,                          /*!< Origin an outgoing call with specific number or the dial the last number */
    XR_HF_WBS_RESPONSE_EVT,                  /*!< Codec Status */
    XR_HF_BCS_RESPONSE_EVT,                  /*!< Final Codec Choice */
} xr_hf_cb_event_t;

/// HFP AG callback parameters
typedef union
{
    /**
     * @brief  XR_HS_CONNECTION_STATE_EVT
     */
    struct hf_conn_stat_param {
        xr_bd_addr_t remote_bda;                 /*!< remote bluetooth device address */
        xr_hf_connection_state_t state;          /*!< Connection state */
        uint32_t peer_feat;                       /*!< HF supported features */
        uint32_t chld_feat;                       /*!< AG supported features on call hold and multiparty services */
    } conn_stat;                                  /*!< AG callback param of XR_HF_CONNECTION_STATE_EVT */

    /**
     * @brief XR_HF_AUDIO_STATE_EVT
     */
    struct hf_audio_stat_param {
        xr_bd_addr_t remote_addr;                /*!< remote bluetooth device address */
        xr_hf_audio_state_t state;               /*!< audio connection state */
    } audio_stat;                                 /*!< AG callback param of XR_HF_AUDIO_STATE_EVT */

    /**
     * @brief XR_HF_BVRA_RESPONSE_EVT
     */
    struct hf_vra_rep_param {
        xr_bd_addr_t     remote_addr;            /*!< remote bluetooth device address */
        xr_hf_vr_state_t value;                  /*!< voice recognition state */
    } vra_rep;                                    /*!< AG callback param of XR_HF_BVRA_RESPONSE_EVT */

    /**
     * @brief XR_HF_VOLUME_CONTROL_EVT
     */
    struct hf_volume_control_param {
        xr_hf_volume_type_t type;                /*!< volume control target, speaker or microphone */
        int volume;                               /*!< gain, ranges from 0 to 15 */
    } volume_control;                             /*!< AG callback param of XR_HF_VOLUME_CONTROL_EVT */

    /**
     * @brief XR_HF_UNAT_RESPONSE_EVT
     */
    struct hf_unat_rep_param {
        char *unat;                               /*!< unknown AT command string */
    }unat_rep;                                    /*!< AG callback param of XR_HF_UNAT_RESPONSE_EVT */

    /**
     * @brief XR_HF_CIND_RESPONSE_EVT
     */
    struct hf_cind_param {
        xr_hf_call_status_t       call_status;         /*!< call status indicator */
        xr_hf_call_setup_status_t call_setup_status;   /*!< call setup status indicator */
        xr_hf_network_state_t svc;                     /*!< network service availability status */
        int signal_strength;                            /*!< signal strength */
        xr_hf_roaming_status_t roam;                   /*!< roam state */
        int battery_level;                              /*!< battery charge value, ranges from 0 to 5 */
        xr_hf_call_held_status_t  call_held_status;    /*!< bluetooth proprietary call hold status indicator */
    } cind;                                             /*!< AG callback param of XR_HF_CIND_RESPONSE_EVT */

    /**
     * @brief XR_HF_DIAL_EVT
     */
    struct hf_out_call_param {
        xr_bd_addr_t remote_addr;                /*!< remote bluetooth device address */
        char *num_or_loc;                         /*!< location in phone memory */
    } out_call;                                   /*!< AG callback param of XR_HF_DIAL_EVT */

    /**
     * @brief XR_HF_VTS_RESPONSE_EVT
     */
    struct hf_vts_rep_param {
        char *code;                               /*!< MTF code from HF Client */
    }vts_rep;                                     /*!< AG callback param of XR_HF_VTS_RESPONSE_EVT */

    /**
     * @brief XR_HF_NREC_RESPONSE_EVT
     */
    struct hf_nrec_param {
       xr_hf_nrec_t state;                       /*!< NREC enabled or disabled */
    } nrec;                                       /*!< AG callback param of XR_HF_NREC_RESPONSE_EVT */

    /**
     * @brief XR_HF_WBS_RESPONSE_EVT
     */
    struct hf_wbs_rep_param {
        xr_hf_wbs_config_t codec;                /*!< codec mode CVSD or mSBC */
    } wbs_rep;                                    /*!< AG callback param of XR_HF_WBS_RESPONSE_EVT */

    /**
     * @brief XR_HF_BCS_RESPONSE_EVT
     */
    struct hf_bcs_rep_param {
        xr_hf_wbs_config_t mode;                 /*!< codec mode CVSD or mSBC */
    } bcs_rep;                                    /*!< AG callback param of XR_HF_BCS_RESPONSE_EVT */

} xr_hf_cb_param_t;                              /*!< HFP AG callback param compound*/

/**
 * @brief           AG incoming data callback function, the callback is useful in case of
 *                  Voice Over HCI.
 * @param[in]       buf : pointer to incoming data(payload of HCI synchronous data packet), the
 *                  buffer is allocated inside bluetooth protocol stack and will be released after
 *                  invoke of the callback is finished.
 * @param[in]       len : size(in bytes) in buf
 */
typedef void (* xr_hf_incoming_data_cb_t)(const uint8_t *buf, uint32_t len);

/**
 * @brief           AG outgoing data callback function, the callback is useful in case of
 *                  Voice Over HCI. Once audio connection is set up and the application layer has
 *                  prepared data to send, the lower layer will call this function to read data
 *                  and then send. This callback is supposed to be implemented as non-blocking,
 *                  and if data is not enough, return value 0 is supposed.
 *
 * @param[in]       buf : pointer to incoming data(payload of HCI synchronous data packet), the
 *                  buffer is allocated inside bluetooth protocol stack and will be released after
 *                  invoke of the callback is finished.
 * @param[in]       len : size(in bytes) in buf
 * @param[out]      length of data successfully read
 */
typedef uint32_t (* xr_hf_outgoing_data_cb_t) (uint8_t *buf, uint32_t len);

/**
 * @brief           HF AG callback function type
 *
 * @param           event : Event type
 *
 * @param           param : Pointer to callback parameter
 */
typedef void (* xr_hf_cb_t) (xr_hf_cb_event_t event, xr_hf_cb_param_t *param);

/************************************************************************************
**  XR HF API
************************************************************************************/
/**
 * @brief           Register application callback function to HFP AG module. This function should be called
 *                  only after xr_bluedroid_enable() completes successfully, used by HFP AG
 *
 * @param[in]       callback: HFP AG event callback function
 *
 * @return
 *                  - XR_OK: success
 *                  - XR_INVALID_STATE: if bluetooth stack is not yet enabled
 *                  - XR_FAIL: if callback is a NULL function pointer
 *
 */
xr_err_t xr_bt_hf_register_callback(xr_hf_cb_t callback);

/**
 *
 * @brief           Initialize the bluetooth HF AG module. This function should be called
 *                  after xr_bluedroid_enable() completes successfully
 *
 * @param[in]       remote_addr: remote bluetooth device address
 * @return
 *                  - XR_OK: if the initialization request is sent successfully
 *                  - XR_INVALID_STATE: if bluetooth stack is not yet enabled
 *                  - XR_FAIL: others
 *
 */
xr_err_t xr_bt_hf_init(xr_bd_addr_t remote_addr);

/**
 *
 * @brief           De-initialize for HF AG module. This function
 *                  should be called only after xr_bluedroid_enable() completes successfully
 *
 * @param[in]       remote_addr: remote bluetooth device address
 * @return
 *                  - XR_OK: success
 *                  - XR_INVALID_STATE: if bluetooth stack is not yet enabled
 *                  - XR_FAIL: others
 *
 */
xr_err_t xr_bt_hf_deinit(xr_bd_addr_t remote_addr);

/**
 *
 * @brief           Connect to remote bluetooth HFP client device, must after xr_bt_hf_init()
 *
 * @param[in]       remote_bda: remote bluetooth HFP client device address
 *
 * @return
 *                  - XR_OK: connect request is sent to lower layer
 *                  - XR_INVALID_STATE: if bluetooth stack is not yet enabled
 *                  - XR_FAIL: others
 *
 */
xr_err_t xr_bt_hf_connect(xr_bd_addr_t remote_bda);

/**
 *
 * @brief           Disconnect from the remote HFP client
 *
 * @param[in]       remote_bda: remote bluetooth device address
 * @return
 *                  - XR_OK: disconnect request is sent to lower layer
 *                  - XR_INVALID_STATE: if bluetooth stack is not yet enabled
 *                  - XR_FAIL: others
 *
 */
xr_err_t xr_bt_hf_disconnect(xr_bd_addr_t remote_bda);

/**
 *
 * @brief           Create audio connection with remote HFP client. As a precondition to use this API,
 *                  Service Level Connection shall exist between HF client and AG.
 *
 * @param[in]       remote_bda: remote bluetooth device address
 * @return
 *                  - XR_OK: audio connection request is sent to lower layer
 *                  - XR_INVALID_STATE: if bluetooth stack is not yet enabled
 *                  - XR_FAIL: others
 *
 */
xr_err_t xr_bt_hf_connect_audio(xr_bd_addr_t remote_bda);

/**
 *
 * @brief           Release the established audio connection with remote HFP client.
 *
 * @param[in]       remote_bda: remote bluetooth device address
 * @return
 *                  - XR_OK: disconnect request is sent to lower layer
 *                  - XR_INVALID_STATE: if bluetooth stack is not yet enabled
 *                  - XR_FAIL: others
 *
 */
xr_err_t xr_bt_hf_disconnect_audio(xr_bd_addr_t remote_bda);

/**
 *
 * @brief           Response of voice Recognition Command(AT+BVRA) from HFP client. As a precondition to use this API,
 *                  Service Level Connection shall exist with HFP client.
 *
 * @param[in]       remote_bda: the device address of voice recognization initiator
 * @param[in]       value: 0 - voice recognition disabled, 1- voice recognition enabled
 *
 * @return
 *                  - XR_OK: voice recognition response is sent to lower layer
 *                  - XR_INVALID_STATE: if bluetooth stack is not yet enabled
 *                  - XR_FAIL: others
 *
 */
xr_err_t xr_bt_hf_vra(xr_bd_addr_t remote_bda, xr_hf_vr_state_t value);

/**
 *
 * @brief           Volume synchronization with HFP client. As a precondition to use this API,
 *                  Service Level Connection shall exist with HFP client.
 *
 * @param[in]       remote_bda: remote bluetooth device address
 * @param[in]       type: volume control target, speaker or microphone
 * @param[in]       volume: gain of the speaker of microphone, ranges 0 to 15
 *
 * @return
 *                  - XR_OK: volume synchronization request is sent to lower layer
 *                  - XR_INVALID_STATE: if bluetooth stack is not yet enabled
 *                  - XR_FAIL: others
 *
 */
xr_err_t xr_bt_hf_volume_control(xr_bd_addr_t remote_bda, xr_hf_volume_control_target_t type, int volume);

 /**
 *
 * @brief           Handle Unknown AT command from HFP Client.
 *                  As a precondition to use this API, Service Level Connection shall exist between AG and HF Client.
 *
 * @param[in]       remote_addr: remote bluetooth device address
 * @param[in]       unat: User AT command response to HF Client.
 *                        It will response "ERROR" by default if unat is NULL.
 * @return
 *                  - XR_OK: unat respnose is sent to lower layer
 *                  - XR_INVALID_STATE: if bluetooth stack is not yet enabled
 *                  - XR_FAIL: others
 *
 */
xr_err_t xr_hf_unat_response(xr_bd_addr_t remote_addr, char *unat);

 /**
 *
 * @brief           Unsolicited send extend AT error code to HFP Client.
 *                  As a precondition to use this API, Service Level Connection shall exist between AG and HF Client.
 *
 * @param[in]       remote_bda: remote bluetooth device address
 * @param[in]       response_code: AT command response code
 * @param[in]       error_code: CME error code
 * @return
 *                  - XR_OK: AT error code is sent to lower layer
 *                  - XR_INVALID_STATE: if bluetooth stack is not yet enabled
 *                  - XR_FAIL: others
 *
 */
xr_err_t xr_bt_hf_cmee_response(xr_bd_addr_t remote_bda, xr_hf_at_response_code_t response_code, xr_hf_cme_err_t error_code);

 /**
 *
 * @brief           Usolicited send device status notification to HFP Client.
 *                  As a precondition to use this API, Service Level Connection shall exist between AG and HF Client
 *
 * @param[in]       remote_addr: remote bluetooth device address
 * @param[in]       call_state: call state
 * @param[in]       call_setup_state: call setup state
 * @param[in]       ntk_state: network service state
 * @param[in]       signal: signal strength from 0 to 5
 * @return
 *                  - XR_OK: device status notification is sent to lower layer
 *                  - XR_INVALID_STATE: if bluetooth stack is not yet enabled
 *                  - XR_FAIL: others
 *
 */
xr_err_t xr_bt_hf_indchange_notification(xr_bd_addr_t remote_addr, xr_hf_call_status_t call_state,
                                            xr_hf_call_setup_status_t call_setup_state,
                                            xr_hf_network_state_t ntk_state, int signal);

 /**
 *
 * @brief           Response to device individual indicatiors to HFP Client.
 *                  As a precondition to use this API, Service Level Connection shall exist between AG and HF Client.
 *
 * @param[in]       remote_addr: remote bluetooth device address
 * @param[in]       call_state: call state
 * @param[in]       call_setup_state: call setup state
 * @param[in]       ntk_state: network service state
 * @param[in]       signal: signal strength from 0 to 5
 * @param[in]       roam: roam state
 * @param[in]       batt_lev: batery level from 0 to 5
 * @param[in]       call_held_status: call held status
 * @return
 *                  - XR_OK: cind response is sent to lower layer
 *                  - XR_INVALID_STATE: if bluetooth stack is not yet enabled
 *                  - XR_FAIL: others
 *
 */
xr_err_t xr_bt_hf_cind_response(xr_bd_addr_t remote_addr,
                                xr_hf_call_status_t call_state,
                                xr_hf_call_setup_status_t call_setup_state,
                                xr_hf_network_state_t ntk_state, int signal, xr_hf_roaming_status_t roam, int batt_lev,
                                xr_hf_call_held_status_t call_held_status);

/**
 *
 * @brief           Reponse for AT+COPS command from HF Client.
 *                  As a precondition to use this API, Service Level Connection shall exist with HFP Client.
 *
 * @param[in]       remote_addr: remote bluetooth device address
 * @param[in]       name: current operator name
 * @return
 *                  - XR_OK: reponse for AT+COPS command is sent to lower layer
 *                  - XR_INVALID_STATE: if bluetooth stack is not yet enabled
 *                  - XR_FAIL: others
 *
 */
xr_err_t xr_bt_hf_cops_response(xr_bd_addr_t remote_addr, char *name);

/**
 *
 * @brief           Response to AT+CLCC command from HFP Client.
 *                  As a precondition to use this API, Service Level Connection shall exist between AG and HF Client.
 *
 * @param[in]       remote_addr: remote bluetooth device address
 * @param[in]       index: the index of current call
 * @param[in]       dir: call direction (incoming/outgoing)
 * @param[in]       current_call_state: current call state
 * @param[in]       mode: current call mode (voice/data/fax)
 * @param[in]       mpty: single or multi type
 * @param[in]       number: current call number
 * @param[in]       type: international type or unknow
 * @return
 *                  - XR_OK: response to AT+CLCC command is sent to lower layer
 *                  - XR_INVALID_STATE: if bluetooth stack is not yet enabled
 *                  - XR_FAIL: others
 *
 */
xr_err_t xr_bt_hf_clcc_response(xr_bd_addr_t remote_addr, int index, xr_hf_current_call_direction_t dir,
                                 xr_hf_current_call_status_t current_call_state, xr_hf_current_call_mode_t mode,
                                 xr_hf_current_call_mpty_type_t mpty, char *number, xr_hf_call_addr_type_t type);

/**
 *
 * @brief           Response for AT+CNUM command from HF Client.
 *                  As a precondition to use this API, Service Level Connection shall exist with AG.
 *
 * @param[in]       remote_addr: remote bluetooth device address
 * @param[in]       number: registration number
 * @param[in]       type: service type (unknown/voice/fax)
 * @return
 *                  - XR_OK: response for AT+CNUM command is sent to lower layer
 *                  - XR_INVALID_STATE: if bluetooth stack is not yet enabled
 *                  - XR_FAIL: others
 *
 */
xr_err_t xr_bt_hf_cnum_response(xr_bd_addr_t remote_addr, char *number, xr_hf_subscriber_service_type_t type);

/**
 *
 * @brief           Inform HF Client that AG Provided in-band ring tone or not.
 *                  As a precondition to use this API, Service Level Connection shall exist with AG.
 *
 * @param[in]       remote_addr: remote bluetooth device address
 * @param[in]       state: in-band ring tone state
 * @return
 *                  - XR_OK: inform is sent to lower layer
 *                  - XR_INVALID_STATE: if bluetooth stack is not yet enabled
 *                  - XR_FAIL: others
 *
 */
xr_err_t xr_bt_hf_bsir(xr_bd_addr_t remote_addr, xr_hf_in_band_ring_state_t state);

/**
 *
 * @brief           Answer Incoming Call from AG.
 *                  As a precondition to use this API, Service Level Connection shall exist with AG.
 *
 * @param[in]       remote_addr: remote bluetooth device address
 * @param[in]       num_active: the number of active call
 * @param[in]       num_held: the number of held call
 * @param[in]       call_state: call state
 * @param[in]       call_setup_state: call setup state
 * @param[in]       number: number of the incoming call
 * @param[in]       call_addr_type: call address type
 * @return
 *                  - XR_OK: answer incoming call is sent to lower layer
 *                  - XR_INVALID_STATE: if bluetooth stack is not yet enabled
 *                  - XR_FAIL: others
 *
 */
xr_err_t xr_bt_hf_answer_call(xr_bd_addr_t remote_addr, int num_active, int num_held,
                                xr_hf_call_status_t call_state,  xr_hf_call_setup_status_t call_setup_state,
                                char *number, xr_hf_call_addr_type_t call_addr_type);

/**
 *
 * @brief           Reject Incoming Call from AG.
 *                  As a precondition to use this API, Service Level Connection shall exist with AG.
 *
 * @param[in]       remote_addr: remote bluetooth device address
 * @param[in]       num_active: the number of active call
 * @param[in]       num_held: the number of held call
 * @param[in]       call_state: call state
 * @param[in]       call_setup_state: call setup state
 * @param[in]       number: number of the incoming call
 * @param[in]       call_addr_type: call address type
 * @return
 *                  - XR_OK: disconnect request is sent to lower layer
 *                  - XR_INVALID_STATE: if bluetooth stack is not yet enabled
 *                  - XR_FAIL: others
 *
 */
xr_err_t xr_bt_hf_reject_call(xr_bd_addr_t remote_addr, int num_active, int num_held,
                                xr_hf_call_status_t call_state,  xr_hf_call_setup_status_t call_setup_state,
                                char *number, xr_hf_call_addr_type_t call_addr_type);

/**
 *
 * @brief           End ongoing call with AG.
 *                  As a precondition to use this API, Service Level Connection shall exist with AG.
 *
 * @param[in]       remote_addr: remote bluetooth device address
 * @param[in]       num_active: the number of active call
 * @param[in]       num_held: the number of held call
 * @param[in]       call_state: call state
 * @param[in]       call_setup_state: call setup state
 * @param[in]       number: number of the outgoing call
 * @param[in]       call_addr_type: call address type
 * @return
 *                  - XR_OK: disconnect request is sent to lower layer
 *                  - XR_INVALID_STATE: if bluetooth stack is not yet enabled
 *                  - XR_FAIL: others
 *
 */
xr_err_t xr_bt_hf_out_call(xr_bd_addr_t remote_addr, int num_active, int num_held,
                            xr_hf_call_status_t call_state,  xr_hf_call_setup_status_t call_setup_state,
                            char *number, xr_hf_call_addr_type_t call_addr_type);

/**
 *
 * @brief           End an ongoing call.
 *                  As a precondition to use this API, Service Level Connection shall exist with AG.
 *
 * @param[in]       remote_addr: remote bluetooth device address
 * @param[in]       num_active: the number of active call
 * @param[in]       num_held: the number of held call
 * @param[in]       call_state: call state
 * @param[in]       call_setup_state: call setup state
 * @param[in]       number: number of the call
 * @param[in]       call_addr_type: call address type
 * @return
 *                  - XR_OK: end ongoing call is sent to lower layer
 *                  - XR_INVALID_STATE: if bluetooth stack is not yet enabled
 *                  - XR_FAIL: others
 *
 */
xr_err_t xr_bt_hf_end_call(xr_bd_addr_t remote_addr, int num_active, int num_held,
                            xr_hf_call_status_t call_state,  xr_hf_call_setup_status_t call_setup_state,
                            char *number, xr_hf_call_addr_type_t call_addr_type);

/**
 * @brief           Register AG data output function; the callback is only used in
 *                  the case that Voice Over HCI is enabled.
 *
 * @param[in]       recv: HFP client incoming data callback function
 * @param[in]       send: HFP client outgoing data callback function
 *
 * @return
 *                  - XR_OK: success
 *                  - XR_INVALID_STATE: if bluetooth stack is not yet enabled
 *                  - XR_FAIL: if callback is a NULL function pointer
 *
 */
xr_err_t xr_bt_hf_register_data_callback(xr_hf_incoming_data_cb_t recv, xr_hf_outgoing_data_cb_t send);


/**
 * @brief           Trigger the lower-layer to fetch and send audio data. This function is only
 *                  only used in the case that Voice Over HCI is enabled. Precondition is that
 *                  the HFP audio connection is connected. After this function is called, lower
 *                  layer will invoke xr_hf_client_outgoing_data_cb_t to fetch data
 *
 */
void xr_hf_outgoing_data_ready(void);

#ifdef __cplusplus
}
#endif

#endif //__XR_HF_AG_API_H__