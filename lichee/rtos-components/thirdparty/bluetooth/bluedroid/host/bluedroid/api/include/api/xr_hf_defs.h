// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
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

#ifndef __XR_HF_DEFS_H__
#define __XR_HF_DEFS_H__

#include "xr_bt_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

#define XR_BT_HF_NUMBER_LEN           (32)
#define XR_BT_HF_OPERATOR_NAME_LEN    (16)

#ifndef BTC_HSAG_SERVICE_NAME
#define BTC_HSAG_SERVICE_NAME ("Headset Gateway")
#endif

#ifndef BTC_HFAG_SERVICE_NAME
#define BTC_HFAG_SERVICE_NAME ("Handsfree Gateway")
#endif

#ifndef BTC_HF_SERVICES
#define BTC_HF_SERVICES    (BTA_HSP_SERVICE_MASK | BTA_HFP_SERVICE_MASK )
#endif

#ifndef BTC_HF_SERVICE_NAMES
#define BTC_HF_SERVICE_NAMES {BTC_HSAG_SERVICE_NAME , BTC_HFAG_SERVICE_NAME}
#endif

#ifndef BTC_HF_SECURITY
#define BTC_HF_SECURITY    (BTA_SEC_AUTHENTICATE | BTA_SEC_ENCRYPT)
#endif

#define BTC_HF_CALL_END_TIMEOUT       6

#define BTC_HF_INVALID_IDX       -1

/// in-band ring tone state
typedef enum {
    XR_HF_IN_BAND_RINGTONE_NOT_PROVIDED = 0,
    XR_HF_IN_BAND_RINGTONE_PROVIDED,
} xr_hf_in_band_ring_state_t;

/// voice recognition state
typedef enum {
    XR_HF_VR_STATE_DISABLED = 0,           /*!< voice recognition disabled */
    XR_HF_VR_STATE_ENABLED,                /*!< voice recognition enabled */
} xr_hf_vr_state_t;

/// Bluetooth HFP audio volume control target
typedef enum {
    XR_HF_VOLUME_CONTROL_TARGET_SPK = 0,             /*!< speaker */
    XR_HF_VOLUME_CONTROL_TARGET_MIC,                 /*!< microphone */
} xr_hf_volume_control_target_t;

/// Bluetooth HFP audio connection status
typedef enum {
    XR_HF_AUDIO_STATE_DISCONNECTED = 0,          /*!< audio connection released */
    XR_HF_AUDIO_STATE_CONNECTING,                /*!< audio connection has been initiated */
    XR_HF_AUDIO_STATE_CONNECTED,                 /*!< audio connection is established */
    XR_HF_AUDIO_STATE_CONNECTED_MSBC,            /*!< mSBC audio connection is established */
} xr_hf_audio_state_t;

typedef enum {
    XR_HF_VOLUME_TYPE_SPK = 0,
    XR_HF_VOLUME_TYPE_MIC
} xr_hf_volume_type_t;

/// +CIND network service availability status
typedef enum
{
    XR_HF_NETWORK_STATE_NOT_AVAILABLE = 0,
    XR_HF_NETWORK_STATE_AVAILABLE
} xr_hf_network_state_t;

/** +CIEV Service type */
typedef enum
{
    XR_HF_SERVICE_TYPE_HOME = 0,
    XR_HF_SERVICE_TYPE_ROAMING
} xr_hf_service_type_t;

/// +CIND call status indicator values
typedef enum {
    XR_HF_CALL_STATUS_NO_CALLS = 0,                  /*!< no call in progress  */
    XR_HF_CALL_STATUS_CALL_IN_PROGRESS = 1,          /*!< call is present(active or held) */
} xr_hf_call_status_t;

/// +CIND call setup status indicator values
typedef enum {
    XR_HF_CALL_SETUP_STATUS_IDLE = 0,                /*!< no call setup in progress */
    XR_HF_CALL_SETUP_STATUS_INCOMING = 1,            /*!< incoming call setup in progress */
    XR_HF_CALL_SETUP_STATUS_OUTGOING_DIALING = 2,    /*!< outgoing call setup in dialing state */
    XR_HF_CALL_SETUP_STATUS_OUTGOING_ALERTING = 3,   /*!< outgoing call setup in alerting state */
} xr_hf_call_setup_status_t;

/// +CIND roaming status indicator values
typedef enum {
    XR_HF_ROAMING_STATUS_INACTIVE = 0,               /*!< roaming is not active */
    XR_HF_ROAMING_STATUS_ACTIVE,                     /*!< a roaming is active */
} xr_hf_roaming_status_t;

/// +CIND call held indicator values
typedef enum {
    XR_HF_CALL_HELD_STATUS_NONE = 0,                 /*!< no calls held */
    XR_HF_CALL_HELD_STATUS_HELD_AND_ACTIVE = 1,      /*!< both active and held call */
    XR_HF_CALL_HELD_STATUS_HELD = 2,                 /*!< call on hold, no active call*/
} xr_hf_call_held_status_t;

/// +CLCC status of the call
typedef enum {
    XR_HF_CURRENT_CALL_STATUS_ACTIVE = 0,            /*!< active */
    XR_HF_CURRENT_CALL_STATUS_HELD = 1,              /*!< held */
    XR_HF_CURRENT_CALL_STATUS_DIALING = 2,           /*!< dialing (outgoing calls only) */
    XR_HF_CURRENT_CALL_STATUS_ALERTING = 3,          /*!< alerting (outgoing calls only) */
    XR_HF_CURRENT_CALL_STATUS_INCOMING = 4,          /*!< incoming (incoming calls only) */
    XR_HF_CURRENT_CALL_STATUS_WAITING = 5,           /*!< waiting (incoming calls only) */
    XR_HF_CURRENT_CALL_STATUS_HELD_BY_RESP_HOLD = 6, /*!< call held by response and hold */
} xr_hf_current_call_status_t;

/// +CLCC direction of the call
typedef enum {
    XR_HF_CURRENT_CALL_DIRECTION_OUTGOING = 0,       /*!< outgoing */
    XR_HF_CURRENT_CALL_DIRECTION_INCOMING = 1,       /*!< incoming */
} xr_hf_current_call_direction_t;

/// +CLCC multi-party call flag
typedef enum {
    XR_HF_CURRENT_CALL_MPTY_TYPE_SINGLE = 0,         /*!< not a member of a multi-party call */
    XR_HF_CURRENT_CALL_MPTY_TYPE_MULTI = 1,          /*!< member of a multi-party call */
} xr_hf_current_call_mpty_type_t;

/// +CLCC call mode
typedef enum {
    XR_HF_CURRENT_CALL_MODE_VOICE = 0,
    XR_HF_CURRENT_CALL_MODE_DATA = 1,
    XR_HF_CURRENT_CALL_MODE_FAX = 2,
} xr_hf_current_call_mode_t;

/// +CLCC address type
typedef enum {
    XR_HF_CALL_ADDR_TYPE_UNKNOWN = 0x81,            /*!< unkown address type */
    XR_HF_CALL_ADDR_TYPE_INTERNATIONAL = 0x91,      /*!< international address */
} xr_hf_call_addr_type_t;

/// +CNUM service type of the phone number
typedef enum {
    XR_HF_SUBSCRIBER_SERVICE_TYPE_UNKNOWN = 0,      /*!< unknown */
    XR_HF_SUBSCRIBER_SERVICE_TYPE_VOICE,            /*!< voice service */
    XR_HF_SUBSCRIBER_SERVICE_TYPE_FAX,              /*!< fax service */
} xr_hf_subscriber_service_type_t;

/// +BTRH response and hold result code
typedef enum {
    XR_HF_BTRH_STATUS_HELD = 0,       /*!< incoming call is put on held in AG */
    XR_HF_BTRH_STATUS_ACCEPTED,       /*!< held incoming call is accepted in AG */
    XR_HF_BTRH_STATUS_REJECTED,       /*!< held incoming call is rejected in AG */
} xr_hf_btrh_status_t;

/// AT+BTRH response and hold action code
typedef enum {
    XR_HF_BTRH_CMD_HOLD = 0,          /*!< put the incoming call on hold */
    XR_HF_BTRH_CMD_ACCEPT = 1,        /*!< accept a held incoming call */
    XR_HF_BTRH_CMD_REJECT = 2,        /*!< reject a held incoming call */
} xr_hf_btrh_cmd_t;

/* +NREC */
typedef enum
{
    XR_HF_NREC_STOP = 0,
    XR_HF_NREC_START
} xr_hf_nrec_t;

///+CCWA resposne status
typedef enum {
    XR_HF_CALL_WAITING_INACTIVE,
    XR_HF_CALL_WAITING_ACTIVE,
} xr_hf_call_waiting_status_t;

/* WBS codec setting */
typedef enum
{
   XR_HF_WBS_NONE,
   XR_HF_WBS_NO,
   XR_HF_WBS_YES
}xr_hf_wbs_config_t;

/// Bluetooth HFP RFCOMM connection and service level connection status
typedef enum {
    XR_HF_CONNECTION_STATE_DISCONNECTED = 0,     /*!< RFCOMM data link channel released */
    XR_HF_CONNECTION_STATE_CONNECTING,           /*!< connecting remote device on the RFCOMM data link*/
    XR_HF_CONNECTION_STATE_CONNECTED,            /*!< RFCOMM connection established */
    XR_HF_CONNECTION_STATE_SLC_CONNECTED,        /*!< service level connection established */
    XR_HF_CONNECTION_STATE_DISCONNECTING,        /*!< disconnecting with remote device on the RFCOMM data link*/
} xr_hf_connection_state_t;

/// AT+CHLD command values
typedef enum {
    XR_HF_CHLD_TYPE_REL = 0,               /*!< <0>, Terminate all held or set UDUB("busy") to a waiting call */
    XR_HF_CHLD_TYPE_REL_ACC,               /*!< <1>, Terminate all active calls and accepts a waiting/held call */
    XR_HF_CHLD_TYPE_HOLD_ACC,              /*!< <2>, Hold all active calls and accepts a waiting/held call */
    XR_HF_CHLD_TYPE_MERGE,                 /*!< <3>, Add all held calls to a conference */
    XR_HF_CHLD_TYPE_MERGE_DETACH,          /*!< <4>, connect the two calls and disconnects the subscriber from both calls */
    XR_HF_CHLD_TYPE_REL_X,                 /*!< <1x>, releases specified calls only */
    XR_HF_CHLD_TYPE_PRIV_X,                /*!< <2x>, request private consultation mode with specified call */
} xr_hf_chld_type_t;

/* AT response code - OK/Error */
typedef enum {
    XR_HF_AT_RESPONSE_CODE_OK = 0,         /*!< acknowledges execution of a command line */
    XR_HF_AT_RESPONSE_CODE_ERR,            /*!< command not accepted */
    XR_HF_AT_RESPONSE_CODE_NO_CARRIER,     /*!< connection terminated */
    XR_HF_AT_RESPONSE_CODE_BUSY,           /*!< busy signal detected */
    XR_HF_AT_RESPONSE_CODE_NO_ANSWER,      /*!< connection completion timeout */
    XR_HF_AT_RESPONSE_CODE_DELAYED,        /*!< delayed */
    XR_HF_AT_RESPONSE_CODE_BLACKLISTED,    /*!< blacklisted */
    XR_HF_AT_RESPONSE_CODE_CME,            /*!< CME error */
} xr_hf_at_response_code_t;

/* AT response code - OK/Error */
typedef enum {
    XR_HF_AT_RESPONSE_ERROR = 0,
    XR_HF_AT_RESPONSE_OK
} xr_hf_at_response_t;

/// Extended Audio Gateway Error Result Code Response
typedef enum {
    XR_HF_CME_AG_FAILURE = 0,                    /*!< ag failure */
    XR_HF_CME_NO_CONNECTION_TO_PHONE = 1,        /*!< no connection to phone */
    XR_HF_CME_OPERATION_NOT_ALLOWED = 3,         /*!< operation not allowed */
    XR_HF_CME_OPERATION_NOT_SUPPORTED = 4,       /*!< operation not supported */
    XR_HF_CME_PH_SIM_PIN_REQUIRED = 5,           /*!< PH-SIM PIN Required */
    XR_HF_CME_SIM_NOT_INSERTED = 10,             /*!< SIM not inserted */
    XR_HF_CME_SIM_PIN_REQUIRED = 11,             /*!< SIM PIN required */
    XR_HF_CME_SIM_PUK_REQUIRED = 12,             /*!< SIM PUK required */
    XR_HF_CME_SIM_FAILURE = 13,                  /*!< SIM failure */
    XR_HF_CME_SIM_BUSY = 14,                     /*!< SIM busy */
    XR_HF_CME_INCORRECT_PASSWORD = 16,           /*!< incorrect password */
    XR_HF_CME_SIM_PIN2_REQUIRED = 17,            /*!< SIM PIN2 required */
    XR_HF_CME_SIM_PUK2_REQUIRED = 18,            /*!< SIM PUK2 required */
    XR_HF_CME_MEMEORY_FULL = 20,                 /*!< memory full */
    XR_HF_CME_INVALID_INDEX = 21,                /*!< invalid index */
    XR_HF_CME_MEMEORY_FAILURE = 23,              /*!< memory failure */
    XR_HF_CME_TEXT_STRING_TOO_LONG = 24,         /*!< text string too long */
    XR_HF_CME_INVALID_CHARACTERS_IN_TEXT_STRING = 25,  /*!< invalid characters in text string */
    XR_HF_CME_DIAL_STRING_TOO_LONG = 26,         /*!< dial string too long*/
    XR_HF_CME_INVALID_CHARACTERS_IN_DIAL_STRING = 27,  /*!< invalid characters in dial string */
    XR_HF_CME_NO_NETWORK_SERVICE = 30,           /*!< no network service */
    XR_HF_CME_NETWORK_TIMEOUT = 31,              /*!< network timeout */
    XR_HF_CME_NETWORK_NOT_ALLOWED = 32,          /*!< network not allowed --emergency calls only */
} xr_hf_cme_err_t;

/** Callback for connection state change.
 *  state will have one of the values from BtHfConnectionState
 */
typedef void (* xr_hf_connection_state_callback)(xr_hf_connection_state_t state, xr_bd_addr_t *bd_addr);

/** Callback for audio connection state change.
 *  state will have one of the values from BtHfAudioState
 */
typedef void (* xr_hf_audio_state_callback)(xr_hf_audio_state_t state, xr_bd_addr_t *bd_addr);

/** Callback for VR connection state change.
 *  state will have one of the values from BtHfVRState
 */
typedef void (* xr_hf_vr_cmd_callback)(xr_hf_vr_state_t state, xr_bd_addr_t *bd_addr);

/** Callback for answer incoming call (ATA)
 */
typedef void (* xr_hf_answer_call_cmd_callback)(xr_bd_addr_t *bd_addr);

/** Callback for disconnect call (AT+CHUP)
 */
typedef void (* xr_hf_hangup_call_cmd_callback)(xr_bd_addr_t *bd_addr);

/** Callback for volume control
 *  type will denote Speaker/Mic gain (BtHfVolumeControl).
 */
typedef void (* xr_hf_volume_cmd_callback)(xr_hf_volume_control_target_t type, int volume, xr_bd_addr_t *bd_addr);

/** Callback for dialing an outgoing call
 *  If number is NULL, redial
 */
typedef void (* xr_hf_dial_call_cmd_callback)(char *number, xr_bd_addr_t *bd_addr);

/** Callback for sending DTMF tones
 *  tone contains the dtmf character to be sent
 */
typedef void (* xr_hf_dtmf_cmd_callback)(char tone, xr_bd_addr_t *bd_addr);

/** Callback for enabling/disabling noise reduction/echo cancellation
 *  value will be 1 to enable, 0 to disable
 */
typedef void (* xr_hf_nrec_cmd_callback)(xr_hf_nrec_t nrec, xr_bd_addr_t *bd_addr);

/** Callback for AT+BCS and event from BAC
 *  WBS enable, WBS disable
 */
typedef void (* xr_hf_wbs_callback)(xr_hf_wbs_config_t wbs, xr_bd_addr_t *bd_addr);

/** Callback for call hold handling (AT+CHLD)
 *  value will contain the call hold command (0, 1, 2, 3)
 */
typedef void (* xr_hf_chld_cmd_callback)(xr_hf_chld_type_t chld, xr_bd_addr_t *bd_addr);

/** Callback for CNUM (subscriber number)
 */
typedef void (* xr_hf_cnum_cmd_callback)(xr_bd_addr_t *bd_addr);

/** Callback for indicators (CIND)
 */
typedef void (* xr_hf_cind_cmd_callback)(xr_bd_addr_t *bd_addr);

/** Callback for operator selection (COPS)
 */
typedef void (* xr_hf_cops_cmd_callback)(xr_bd_addr_t *bd_addr);

/** Callback for call list (AT+CLCC)
 */
typedef void (* xr_hf_clcc_cmd_callback) (xr_bd_addr_t *bd_addr);

/** Callback for unknown AT command recd from AG
 *  at_string will contain the unparsed AT string
 */
typedef void (* xr_hf_unknown_at_cmd_callback)(char *at_string, xr_bd_addr_t *bd_addr);

/** Callback for keypressed (HSP) event.
 */
typedef void (* xr_hf_key_pressed_cmd_callback)(xr_bd_addr_t *bd_addr);

#ifdef __cplusplus
}
#endif

#endif /* __XR_HF_DEFS_H__ */
