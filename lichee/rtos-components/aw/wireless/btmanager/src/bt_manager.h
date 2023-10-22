#ifndef _BT_MANAGER_H_
#define _BT_MANAGER_H_

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>

#if __cplusplus
extern "C" {
#endif

#define BTMGVERSION "V5.1.9"

typedef int btmg_err;

/* Definitions for error constants. */
#define BT_OK   0
#define BT_FAIL -1
#define BT_ERR_NO_MEMORY     0x101 /*!< Out of memory */
#define BT_ERR_INVALID_ARG   0x102 /*!< Invalid argument */
#define BT_ERR_INVALID_STATE 0x103 /*!< Invalid state */
#define BT_ERR_TIMEOUT       0x104 /*!< Operation timed out */
#define BT_ERR_IN_PROCESS    0x105 /*!< Operation now in progress */
#define BT_ERR_NOT_SUPPORTED 0x106 /*!< Operation or feature not supported */
#define BT_ERR_NOT_ENABLED   0x107 /*!< Local adapter not enabled */
#define BT_ERR_RESOURCE_BUSY 0x108 /*!< Device or resource busy */
#define BT_ERR_NOT_CONNECTED 0x109 /*!< Device not connected */

/**
 * @brief Log level
 *
 */
typedef enum {
    BTMG_LOG_LEVEL_NONE = 0, /*!< No log output */
    BTMG_LOG_LEVEL_ERROR,    /*!< Critical errors, software module can not recover on its own */
    BTMG_LOG_LEVEL_WARNG,    /*!< Error conditions from which recovery measures have been taken */
    BTMG_LOG_LEVEL_INFO,     /*!< Information messages which describe normal flow of events */
    BTMG_LOG_LEVEL_DEBUG,    /*!< Extra information which is not necessary for normal use (values, pointers, sizes, etc). */
} btmg_log_level_t;

/* Definitions for profile. */
#define BTMG_A2DP_SINK   (1 << 0)
#define BTMG_A2DP_SOURCE (1 << 1)
#define BTMG_HFP_HF      (1 << 2)
#define BTMG_SPP_SERVER  (1 << 3)
#define BTMG_SPP_CLIENT  (1 << 4)
#define BTMG_GATT_SERVER (1 << 5)
#define BTMG_GATT_CLIENT (1 << 6)
#define BTMG_HFP_AG      (1 << 7)

/**
 * @brief Enabled profile information
 *
 */
typedef struct {
    bool a2dp_sink_enabled;
    bool a2dp_source_enabled;
    bool hfp_hf_enabled;
    bool hfp_ag_enabled;
    bool gatts_enabled;
    bool gattc_enabled;
    bool sppc_enabled;
    bool spps_enabled;
} btmg_profile_info_t;

typedef struct {
    bool gap_register;
    bool a2dp_sink_register;
    bool a2dp_source_register;
    bool hfp_hf_register;
    bool hfp_ag_register;
    bool gatts_register;
    bool gattc_register;
    bool sppc_register;
    bool spps_register;
} btmg_profile_state_t;

/*******************************************************************************
**
** Function         btmg_core_init
**
** Description      btmanager core resource initialization.
**
** Parameter        void
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_core_init(void);

/*******************************************************************************
**
** Function         btmg_core_deinit
**
** Description      btmanager core resource deinitialization.
**
** Parameter        void
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_core_deinit(void);

/*******************************************************************************
**
** Function         btmg_set_profile
**
** Description      Set the profile to be initialized.
**
** Parameter        profile  -specified profile, such as BTMG_A2DP_SINK.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_set_profile(int profile);

/*******************************************************************************
**
** Function         btmg_set_loglevel
**
** Description      Set btmanager log level.
**
** Parameter        profile  -specified profile, such as BTMG_A2DP_SINK.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_set_loglevel(btmg_log_level_t log_level);

/*******************************************************************************
**
** Function         btmg_get_loglevel
**
** Description      Get btmanager log level.
**
** Parameter        log_level  -Pointer to the obtained log_level.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_get_loglevel(btmg_log_level_t *log_level);

/*******************************************************************************
**
** Function         btmg_set_ex_debug
**
** Description      Set the extended debug mask bit.
**
** Parameter        mask  -extended debug mask.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
**
*******************************************************************************/
btmg_err btmg_set_ex_debug(int mask);

/*******************************************************************************
**
** Function         btmg_get_ex_debug
**
** Description      Get the extended debug mask bit.
**
** Parameter        mask  -Pointer to the obtained extended debug mask.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_get_ex_debug(int *mask);

/*******************************************************************************
**
** Function         btmg_get_error_info
**
** Description      Convert error codes to messages.
**
** Parameter        btmg_err  -error code.
**
** Returns          error message string.
**
*******************************************************************************/
const char *btmg_get_error_info(btmg_err);

/**
 * @brief Adapter state
 *
 */
typedef enum {
    BTMG_ADAPTER_OFF,
    BTMG_ADAPTER_TURNING_ON,
    BTMG_ADAPTER_ON,
    BTMG_ADAPTER_TURNING_OFF,
} btmg_adapter_state_t;

/**
 * @brief Status of bt scan
 *
 */
typedef enum {
    BTMG_SCAN_STARTED,
    BTMG_SCAN_START_FAILED,
    BTMG_SCAN_STOPPED,
} btmg_scan_state_t;

/**
 * @brief Bt adapter connect/discovery mode
 *
 */
typedef enum {
    BTMG_SCAN_MODE_NONE,
    BTMG_SCAN_MODE_CONNECTABLE,
    BTMG_SCAN_MODE_CONNECTABLE_DISCOVERABLE,
} btmg_scan_mode_t;

/**
 * @brief Device bond state
 *
 */
typedef enum {
    BTMG_BOND_STATE_UNBONDED,
    BTMG_BOND_STATE_BONDING,
    BTMG_BOND_STATE_BONDED,
    BTMG_BOND_STATE_BOND_FAILED,
} btmg_bond_state_t;

/**
 * @brief Bt adapter io capability
 *
 */
typedef enum {
    BTMG_IO_CAP_DISPLAYONLY,
    BTMG_IO_CAP_DISPLAYYESNO,
    BTMG_IO_CAP_KEYBOARDONLY,
    BTMG_IO_CAP_NOINPUTNOOUTPUT,
} btmg_io_capability_t;

typedef void (*bt_adapter_state_cb)(btmg_adapter_state_t state);
typedef void (*bt_adapter_scan_status_cb)(btmg_scan_state_t status);
typedef void (*bt_adapter_get_name_cb)(char *name);
typedef void (*bt_adapter_bonded_device_cb)(btmg_bond_state_t state, const char *bd_addr);
/**
 * @brief   Adapter related callback
 *
 */
typedef struct {
    bt_adapter_state_cb state_cb;                 /*adapter state callback*/
    bt_adapter_scan_status_cb scan_status_cb;     /*scan status callback*/
    bt_adapter_get_name_cb get_name_cb;           /*adapter name callback*/
    bt_adapter_bonded_device_cb bonded_device_cb; /*device bonded state callback*/
} btmg_adapter_callback_t;

/*******************************************************************************
**
** Function         btmg_adapter_enable
**
** Description      Used to enable and disable the adapter.
**
** Parameter        enable  -ture or false.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
*******************************************************************************/
btmg_err btmg_adapter_enable(bool enable);

/*******************************************************************************
**
** Function         btmg_adapter_get_state
**
** Description      Get the current adapter state.
**
** Parameter        adapter_state  -Pointer to the obtained adapter state.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_adapter_get_state(btmg_adapter_state_t *adapter_state);

/*******************************************************************************
**
** Function         btmg_get_connnected_dev_list
**
** Description      Get the connected devices.
**
** Parameter        connected_list  -Pointer to the obtained connected list.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_get_connnected_dev_list(void *connected_list);

/*******************************************************************************
**
** Function         btmg_adapter_get_address
**
** Description      Get the address of the adapter.
**
** Parameter        addr  -Pointer to the obtained address.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_adapter_get_address(char *addr);

/*******************************************************************************
**
** Function         btmg_adapter_set_name
**
** Description      Set the name of the adapter.
**
** Parameter        name  -Pointer to the name to set.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_adapter_set_name(const char *name);

/*******************************************************************************
**
** Function         btmg_adapter_get_name
**
** Description      Get the name of the adapter.
**
** Parameter        name  -Pointer to the obtained name.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_adapter_get_name(void);

/*******************************************************************************
**
** Function         btmg_adapter_get_name
**
** Description      Set the scanmode of the adapter.
**
** Parameter        mode  -whether device is discoverable and connectable
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_adapter_set_scanmode(btmg_scan_mode_t mode);

/*******************************************************************************
**
** Function         btmg_adapter_set_io_capability
**
** Description      Set the io capability of the adapter.
**
** Parameter        io_cap  -io capability,see btmg_io_capability_t for details.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
*******************************************************************************/
btmg_err btmg_adapter_set_io_capability(btmg_io_capability_t io_cap);

/*******************************************************************************
**
** Function         btmg_adapter_start_scan
**
** Description      Enable Bluetooth Discovery Scan
**
** Parameter        void.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_adapter_start_scan(void);

/*******************************************************************************
**
** Function         btmg_adapter_stop_scan
**
** Description      Disable Bluetooth Discovery Scan
**
** Parameter        void.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_adapter_stop_scan(void);

/*******************************************************************************
**
** Function         btmg_adapter_is_scanning
**
** Description      Determine if it is currently scanning.
**
** Parameter        is_scanning  -Pointer to the obtained scan state.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_adapter_is_scanning(bool *is_scanning);

/*******************************************************************************
**
** Function         btmg_adapter_pair_device
**
** Description      Initiate a pairing request to the device.
**
** Parameter        addr  -Pointer to the address of the peer device.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_adapter_pair_device(const char *addr);

/*******************************************************************************
**
** Function         btmg_adapter_unpair_device
**
** Description      Unpair the specified device.
**
** Parameter        addr  -Pointer to the address of the peer device.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_adapter_unpair_device(const char *addr);

/**
 * @brief   Paired device information
 *
 */
typedef struct {
    char address[18];
    char name[248];
} btmg_paired_device_t;

/**
 * @brief   remote device information
 *
 */
typedef struct {
    char *address;
    char *name;
    uint32_t cod;
    int16_t rssi;
} btmg_device_t;


typedef enum {
    A2DP_SRC_DEV  = 1 << 1,
    A2DP_SNK_DEV  = 1 << 2,
    SPP_CLIENT_DEV   = 1 << 3,
    SPP_SERVER_DEV   = 1 << 4,
    HFP_HF_DEV       = 1 << 5,
    HFP_AG_DEV       = 1 << 6,
} btmg_device_type;

/*******************************************************************************
**
** Function         btmg_get_paired_device_num
**
** Description      Get the number of paired devices.
**
** Parameter        number  -Pointer to the obtained number.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_get_paired_device_num(int *number);

/*******************************************************************************
**
** Function         btmg_get_paired_devices
**
** Description      Get the paired devices.
**
** Parameter        count  -Number of paired devices.
**                  dev_list  -Pointer to the acquired paired devices.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_get_paired_devices(int count, btmg_paired_device_t *dev_list);

/*******************************************************************************
**
** Function         btmg_set_page_timeout
**
** Description      Set page timeout.
**
** Parameter        slots  -1 slot is equal to 0.625ms.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_set_page_timeout(int slots);

/*******************************************************************************
**
** Function         btmg_set_link_supervision_timeout
**
** Description      Set link supervision timeout.Determines how long the two parties
**                  of the ACL connection agree to be without contact disconnected.
**
** Parameter        addr  - Pointer to the address of the peer device.
**                  slots  -1 slot is equal to 0.625ms.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_set_link_supervision_timeout(const char *addr, int slots);

typedef void (*bt_device_add_cb)(btmg_device_t *device);
//typedef void (*bt_gap_dev_device_remove_cb)(btmg_bt_device_t *device);
typedef void (*bt_device_get_name_cb)(char *name);
typedef void (*bt_device_pindcoe_request_cb)(const char *addr);
typedef void (*bt_device_passkey_request_cb)(const char *addr);
typedef void (*bt_device_passkey_confirm_cb)(const char *addr, uint32_t passkey);
typedef void (*bt_device_pairing_confirm_cb)(const char *addr, uint32_t passkey);
/**
 * @brief   Device related callback
 *
 */
typedef struct {
    bt_device_add_cb device_add_cb;                   /*scan to a new device callback*/
    //bt_device_remove_cb device_remove_cb;
    bt_device_get_name_cb get_name_cb;                /*Get the name of the peer device callback*/
    bt_device_pindcoe_request_cb pindcoe_request_cb;  /*pindcoe request callback*/
    bt_device_passkey_request_cb passkey_request_cb;  /*passkey request callback*/
    bt_device_passkey_confirm_cb passkey_confirm_cb;  /*passkey confirm callback*/
    bt_device_pairing_confirm_cb pairing_confirm_cb;  /*pairing confirm callback*/
} btmg_device_callback_t;

/*******************************************************************************
**
** Function         btmg_device_is_connected
**
** Description      Get the connection status of the peer device.
**
** Parameter        addr  -Pointer to the address of the peer device.
**
** Returns          true:  connected
**                  false: no connected
**
*******************************************************************************/
bool btmg_device_is_connected(const char *addr);

/*******************************************************************************
**
** Function         btmg_device_get_name
**
** Description      Get the name of the peer device.
**
** Parameter        addr  -Pointer to the address of the peer device.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_device_get_name(const char *addr);

/*******************************************************************************
**
** Function         btmg_device_remove
**
** Description      Delete specified device information.
**
** Parameter        addr  -Pointer to the address of the device.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_device_remove(const char *addr);

/*******************************************************************************
**
** Function         btmg_device_pincode_reply
**
** Description      reply pincode.
**
** Parameter        pincode  -Pointer to the string of the pincode.
**                            1~16 character string, can be alphanumeric.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_device_pincode_reply(char *pincode);

/*******************************************************************************
**
** Function         btmg_device_passkey_reply
**
** Description      reply passkey.
**
** Parameter        passkey  -passkey value, 1~999999.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_device_passkey_reply(uint32_t passkey);

/*******************************************************************************
**
** Function         btmg_device_passkey_confirm
**
** Description      confirm passkey.
**
** Parameter        passkey  -passkey valee, must same as the peer device.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_device_passkey_confirm(uint32_t passkey);

/*******************************************************************************
**
** Function         btmg_device_pairing_confirm
**
** Description      confirm pairing.
**
** Parameter        void.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_device_pairing_confirm(void);

/**
 * @brief   A2dp sink connection state
 *
 */
typedef enum {
    BTMG_A2DP_SINK_DISCONNECTED,
    BTMG_A2DP_SINK_CONNECTING,
    BTMG_A2DP_SINK_CONNECTED,
    BTMG_A2DP_SINK_DISCONNECTING,
    BTMG_A2DP_SINK_CONNECT_FAILED,
    BTMG_A2DP_SINK_DISCONNEC_FAILED,
} btmg_a2dp_sink_connection_state_t;

/**
 * @brief   A2dp sink audio state
 *
 */
typedef enum {
    BTMG_A2DP_SINK_AUDIO_SUSPENDED,
    BTMG_A2DP_SINK_AUDIO_STOPPED,
    BTMG_A2DP_SINK_AUDIO_STARTED,
} btmg_a2dp_sink_audio_state_t;

typedef void (*bt_a2dp_sink_connection_state_cb)(const char *bd_addr,
                                                 btmg_a2dp_sink_connection_state_t state);
typedef void (*bt_a2dp_sink_audio_state_cb)(const char *bd_addr,
                                            btmg_a2dp_sink_audio_state_t state);
typedef void (*bt_a2dp_sink_stream_cb)(const char *bd_addr, uint16_t channels, uint16_t sampling,
                                       uint8_t *data, uint32_t len);
/**
 * @brief   A2dp sink profile callback.
 *
 */
typedef struct {
    bt_a2dp_sink_connection_state_cb conn_state_cb; /*used to report the a2dp_sink connection state*/
    bt_a2dp_sink_audio_state_cb audio_state_cb; /*used to report the a2dp_sink audio state, not recommended as mentioned before*/
    bt_a2dp_sink_stream_cb stream_cb;
} btmg_a2dp_sink_callback_t;

/*******************************************************************************
**
** Function         btmg_a2dp_sink_connect
**
** Description      Connect a2dp source device.
**
** Parameter        addr  -Pointer to the address of the device.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_a2dp_sink_connect(const char *addr);

/*******************************************************************************
**
** Function         btmg_a2dp_sink_disconnect
**
** Description      Disconnect from a2dp source device.
**
** Parameter        addr  -Pointer to the address of the device.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_a2dp_sink_disconnect(const char *addr);

/**
 * @brief   A2dp source connection state
 *
 */
typedef enum {
    BTMG_A2DP_SOURCE_DISCONNECTED,
    BTMG_A2DP_SOURCE_CONNECTING,
    BTMG_A2DP_SOURCE_CONNECTED,
    BTMG_A2DP_SOURCE_DISCONNECTING,
    BTMG_A2DP_SOURCE_CONNECT_FAILED,
    BTMG_A2DP_SOURCE_DISCONNEC_FAILED,
} btmg_a2dp_source_connection_state_t;

/**
 * @brief   A2dp source audio state
 *
 */
typedef enum {
    BTMG_A2DP_SOURCE_AUDIO_SUSPENDED,
    BTMG_A2DP_SOURCE_AUDIO_STOPPED,
    BTMG_A2DP_SOURCE_AUDIO_STARTED,
} btmg_a2dp_source_audio_state_t;

typedef void (*bt_a2dp_source_connection_state_cb)(const char *bd_addr,
                                                   btmg_a2dp_source_connection_state_t state);
typedef void (*bt_a2dp_source_audio_state_cb)(const char *bd_addr,
                                              btmg_a2dp_source_audio_state_t state);
typedef int32_t (*bt_a2dp_source_audio_data_cb)(uint8_t *data, int32_t len);

/**
 * @brief   A2dp source profile callback.
 *
 */
typedef struct {
    bt_a2dp_source_connection_state_cb conn_state_cb; /*used to report the a2dp_source connection state*/
    bt_a2dp_source_audio_state_cb audio_state_cb; /*used to report the a2dp_source audio state*/
    bt_a2dp_source_audio_data_cb  audio_data_cb;  /*used to report the a2dp_source audio data*/
} btmg_a2dp_source_callback_t;

/*******************************************************************************
**
** Function         btmg_a2dp_source_connect
**
** Description      Connect a2dp sink device.
**
** Parameter        addr  -Pointer to the address of the device.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_a2dp_source_connect(const char *addr);

/*******************************************************************************
**
** Function         btmg_a2dp_source_disconnect
**
** Description      Disconnect from a2dp sink device.
**
** Parameter        addr  -Pointer to the address of the device.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_a2dp_source_disconnect(const char *addr);

/*******************************************************************************
**
** Function         btmg_a2dp_source_set_audio_param
**
** Description      Set the audio parameters that need to be played to
**                  let btmanager know internally
**
** Parameter        channels  -Number of audio channels, Usually 1 or 2.
**                  sampling  -Audio sample rate, such as 44100/48000 etc.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_a2dp_source_set_audio_param(uint8_t channels, uint32_t sampling);

/*******************************************************************************
**
** Function         btmg_a2dp_source_send_data
**
** Description      Application sends data to btmanager.
**
** Parameter        data  -Pointer to the pcm data of the audio
**                  len  -Audio pcm data length, Recommendation 512.
**
** Returns          Sent data size
**
*******************************************************************************/
int btmg_a2dp_source_send_data(uint8_t *data, int len);

/*******************************************************************************
**
** Function         btmg_a2dp_source_play_start
**
** Description      Start the playback processing thread.
**
** Parameter        void
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_a2dp_source_play_start(void);

/*******************************************************************************
**
** Function         btmg_a2dp_source_play_stop
**
** Description      Stop the playback processing thread.
**
** Parameter        drop : if true stop immediately
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_a2dp_source_play_stop(bool drop);

/*******************************************************************************
**
** Function         btmg_a2dp_source_is_ready
**
** Description      Can judge whether a2dp source is ready.
**
** Parameter        void
**
** Returns          return status
**
*******************************************************************************/
bool btmg_a2dp_source_is_ready(void);

/**
 * @brief  Avrcp passthrough cmd.
 *
 */
typedef enum {
    BTMG_AVRC_PT_CMD_SELECT         =   0x00,    /*!< select */
    BTMG_AVRC_PT_CMD_UP             =   0x01,    /*!< up */
    BTMG_AVRC_PT_CMD_DOWN           =   0x02,    /*!< down */
    BTMG_AVRC_PT_CMD_LEFT           =   0x03,    /*!< left */
    BTMG_AVRC_PT_CMD_RIGHT          =   0x04,    /*!< right */
    BTMG_AVRC_PT_CMD_RIGHT_UP       =   0x05,    /*!< right-up */
    BTMG_AVRC_PT_CMD_RIGHT_DOWN     =   0x06,    /*!< right-down */
    BTMG_AVRC_PT_CMD_LEFT_UP        =   0x07,    /*!< left-up */
    BTMG_AVRC_PT_CMD_LEFT_DOWN      =   0x08,    /*!< left-down */
    BTMG_AVRC_PT_CMD_ROOT_MENU      =   0x09,    /*!< root menu */
    BTMG_AVRC_PT_CMD_SETUP_MENU     =   0x0A,    /*!< setup menu */
    BTMG_AVRC_PT_CMD_CONT_MENU      =   0x0B,    /*!< contents menu */
    BTMG_AVRC_PT_CMD_FAV_MENU       =   0x0C,    /*!< favorite menu */
    BTMG_AVRC_PT_CMD_EXIT           =   0x0D,    /*!< exit */
    BTMG_AVRC_PT_CMD_0              =   0x20,    /*!< 0 */
    BTMG_AVRC_PT_CMD_1              =   0x21,    /*!< 1 */
    BTMG_AVRC_PT_CMD_2              =   0x22,    /*!< 2 */
    BTMG_AVRC_PT_CMD_3              =   0x23,    /*!< 3 */
    BTMG_AVRC_PT_CMD_4              =   0x24,    /*!< 4 */
    BTMG_AVRC_PT_CMD_5              =   0x25,    /*!< 5 */
    BTMG_AVRC_PT_CMD_6              =   0x26,    /*!< 6 */
    BTMG_AVRC_PT_CMD_7              =   0x27,    /*!< 7 */
    BTMG_AVRC_PT_CMD_8              =   0x28,    /*!< 8 */
    BTMG_AVRC_PT_CMD_9              =   0x29,    /*!< 9 */
    BTMG_AVRC_PT_CMD_DOT            =   0x2A,    /*!< dot */
    BTMG_AVRC_PT_CMD_ENTER          =   0x2B,    /*!< enter */
    BTMG_AVRC_PT_CMD_CLEAR          =   0x2C,    /*!< clear */
    BTMG_AVRC_PT_CMD_CHAN_UP        =   0x30,    /*!< channel up */
    BTMG_AVRC_PT_CMD_CHAN_DOWN      =   0x31,    /*!< channel down */
    BTMG_AVRC_PT_CMD_PREV_CHAN      =   0x32,    /*!< previous channel */
    BTMG_AVRC_PT_CMD_SOUND_SEL      =   0x33,    /*!< sound select */
    BTMG_AVRC_PT_CMD_INPUT_SEL      =   0x34,    /*!< input select */
    BTMG_AVRC_PT_CMD_DISP_INFO      =   0x35,    /*!< display information */
    BTMG_AVRC_PT_CMD_HELP           =   0x36,    /*!< help */
    BTMG_AVRC_PT_CMD_PAGE_UP        =   0x37,    /*!< page up */
    BTMG_AVRC_PT_CMD_PAGE_DOWN      =   0x38,    /*!< page down */
    BTMG_AVRC_PT_CMD_POWER          =   0x40,    /*!< power */
    BTMG_AVRC_PT_CMD_VOL_UP         =   0x41,    /*!< volume up */
    BTMG_AVRC_PT_CMD_VOL_DOWN       =   0x42,    /*!< volume down */
    BTMG_AVRC_PT_CMD_MUTE           =   0x43,    /*!< mute */
    BTMG_AVRC_PT_CMD_PLAY           =   0x44,    /*!< play */
    BTMG_AVRC_PT_CMD_STOP           =   0x45,    /*!< stop */
    BTMG_AVRC_PT_CMD_PAUSE          =   0x46,    /*!< pause */
    BTMG_AVRC_PT_CMD_RECORD         =   0x47,    /*!< record */
    BTMG_AVRC_PT_CMD_REWIND         =   0x48,    /*!< rewind */
    BTMG_AVRC_PT_CMD_FAST_FORWARD   =   0x49,    /*!< fast forward */
    BTMG_AVRC_PT_CMD_EJECT          =   0x4A,    /*!< eject */
    BTMG_AVRC_PT_CMD_FORWARD        =   0x4B,    /*!< forward */
    BTMG_AVRC_PT_CMD_BACKWARD       =   0x4C,    /*!< backward */
    BTMG_AVRC_PT_CMD_ANGLE          =   0x50,    /*!< angle */
    BTMG_AVRC_PT_CMD_SUBPICT        =   0x51,    /*!< subpicture */
    BTMG_AVRC_PT_CMD_F1             =   0x71,    /*!< F1 */
    BTMG_AVRC_PT_CMD_F2             =   0x72,    /*!< F2 */
    BTMG_AVRC_PT_CMD_F3             =   0x73,    /*!< F3 */
    BTMG_AVRC_PT_CMD_F4             =   0x74,    /*!< F4 */
    BTMG_AVRC_PT_CMD_F5             =   0x75,    /*!< F5 */
    BTMG_AVRC_PT_CMD_VENDOR         =   0x7E,    /*!< vendor unique */
} btmg_avrc_pt_cmd_t;

/**
 * @brief  The events that can be registered for notifications
 *
 */
typedef enum {
    BTMG_AVRC_RN_PLAY_STATUS_CHANGE = 0x01,  /*!< track status change, eg. from playing to paused */
    BTMG_AVRC_RN_TRACK_CHANGE = 0x02,        /*!< new track is loaded */
    BTMG_AVRC_RN_TRACK_REACHED_END = 0x03,   /*!< current track reached end */
    BTMG_AVRC_RN_TRACK_REACHED_START = 0x04, /*!< current track reached start position */
    BTMG_AVRC_RN_PLAY_POS_CHANGED = 0x05,    /*!< track playing position changed */
    BTMG_AVRC_RN_BATTERY_STATUS_CHANGE = 0x06,    /*!< battery status changed */
    BTMG_AVRC_RN_SYSTEM_STATUS_CHANGE = 0x07,     /*!< system status changed */
    BTMG_AVRC_RN_APP_SETTING_CHANGE = 0x08,       /*!< application settings changed */
    BTMG_AVRC_RN_NOW_PLAYING_CHANGE = 0x09,       /*!< now playing content changed */
    BTMG_AVRC_RN_AVAILABLE_PLAYERS_CHANGE = 0x0a, /*!< available players changed */
    BTMG_AVRC_RN_ADDRESSED_PLAYER_CHANGE = 0x0b,  /*!< the addressed player changed */
    BTMG_AVRC_RN_UIDS_CHANGE = 0x0c,              /*!< UIDs changed */
    BTMG_AVRC_RN_VOLUME_CHANGE = 0x0d,            /*!< volume changed locally on TG */
    BTMG_AVRC_RN_MAX_EVT
} btmg_avrc_rn_event_ids_t;

/*******************************************************************************
**
** Function         btmg_avrc_ct_send_passthrough_cmd
**
** Description      Avrcp ct send passthrough cmd.
**
** Parameter        key_code  -cmd code, see btmg_avrc_pt_cmd_t.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_avrc_ct_send_passthrough_cmd(uint8_t key_code);

/*******************************************************************************
**
** Function         btmg_avrc_set_absolute_volume
**
** Description      Set absolute volume value.
**
** Parameter        volume  -volume value, 0 ~100.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_avrc_set_absolute_volume(uint32_t volume);

/*******************************************************************************
**
** Function         btmg_avrc_get_absolute_volume
**
** Description      Get absolute volume value.
**
** Parameter        volume  -output Parameter:Pointer to the obtained number.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_avrc_get_absolute_volume(uint32_t *volume);

/**
 * @brief  Avrcp play state.
 *
 */
typedef enum {
    BTMG_AVRCP_PLAYSTATE_STOPPED,
    BTMG_AVRCP_PLAYSTATE_PLAYING,
    BTMG_AVRCP_PLAYSTATE_PAUSED,
    BTMG_AVRCP_PLAYSTATE_FWD_SEEK,
    BTMG_AVRCP_PLAYSTATE_REV_SEEK,
    BTMG_AVRCP_PLAYSTATE_FORWARD,
    BTMG_AVRCP_PLAYSTATE_BACKWARD,
    BTMG_AVRCP_PLAYSTATE_ERROR,
} btmg_avrcp_play_state_t;

/**
 * @brief  Music track info.
 *
 */
typedef struct {
    char title[512];
    char artist[256];
    char album[256];
    char track_num[64];
    char num_tracks[64];
    char genre[256];
    char duration[256];
} btmg_track_info_t;

typedef void (*bt_avrcp_ct_play_state_cb)(const char *bd_addr, btmg_avrcp_play_state_t state);
typedef void (*bt_avrcp_ct_track_changed_cb)(const char *bd_addr, btmg_track_info_t *track_info);
typedef void (*bt_avrcp_ct_play_position_cb)(const char *bd_addr, int song_len, int song_pos);
typedef void (*bt_avrcp_tg_play_state_cb)(const char *bd_addr, btmg_avrcp_play_state_t state);
typedef void (*bt_avrcp_audio_volume_cb)(const char *bd_addr, unsigned int volume);

/**
 * @brief  Avrcp callback.
 *
 */
typedef struct {
    bt_avrcp_ct_play_state_cb avrcp_ct_play_state_cb;
    bt_avrcp_ct_track_changed_cb avrcp_ct_track_changed_cb;
    bt_avrcp_ct_play_position_cb avrcp_ct_play_position_cb;
    bt_avrcp_tg_play_state_cb avrcp_tg_play_state_cb;
    bt_avrcp_audio_volume_cb avrcp_audio_volume_cb;
} btmg_avrcp_callback_t;

/**
 * @brief  HFP_HF callback events.
 *
 */
typedef enum {
    BTMG_HFP_HF_CONNECTION_STATE_EVT = 0,      /*!< connection state changed event */
    BTMG_HFP_HF_AUDIO_STATE_EVT,               /*!< audio connection state change event */
    BTMG_HFP_HF_BVRA_EVT,                      /*!< voice recognition state change event */
    BTMG_HFP_HF_CIND_CALL_EVT,                 /*!< call indication */
    BTMG_HFP_HF_CIND_CALL_SETUP_EVT,           /*!< call setup indication */
    BTMG_HFP_HF_CIND_CALL_HELD_EVT,            /*!< call held indication */
    BTMG_HFP_HF_CIND_SERVICE_AVAILABILITY_EVT, /*!< network service availability indication */
    BTMG_HFP_HF_CIND_SIGNAL_STRENGTH_EVT,      /*!< signal strength indication */
    BTMG_HFP_HF_CIND_ROAMING_STATUS_EVT,       /*!< roaming status indication */
    BTMG_HFP_HF_CIND_BATTERY_LEVEL_EVT,        /*!< battery level indication */
    BTMG_HFP_HF_COPS_CURRENT_OPERATOR_EVT,     /*!< current operator information */
    BTMG_HFP_HF_BTRH_EVT,                      /*!< call response and hold event */
    BTMG_HFP_HF_CLIP_EVT,                      /*!< Calling Line Identification notification */
    BTMG_HFP_HF_CCWA_EVT,                      /*!< call waiting notification */
    BTMG_HFP_HF_CLCC_EVT,                      /*!< list of current calls notification */
    BTMG_HFP_HF_VOLUME_CONTROL_EVT, /*!< audio volume control command from AG, provided by +VGM or +VGS message */
    BTMG_HFP_HF_CNUM_EVT,     /*!< subscriber information response from AG */
    BTMG_HFP_HF_BSIR_EVT,     /*!< setting of in-band ring tone */
    BTMG_HFP_HF_BINP_EVT,     /*!< requested number of last voice tag from AG */
    BTMG_HFP_HF_RING_IND_EVT, /*!< ring indication event */
} btmg_hfp_hf_event_t;

/**
 * @brief  Voice recognition state
 *
 */
typedef enum {
    BTMG_HF_VR_STATE_DISABLED = 0, /*!< voice recognition disabled */
    BTMG_HF_VR_STATE_ENABLED,      /*!< voice recognition enabled */
} btmg_hf_vr_state_t;

/**
* @brief BTMG_HF_BVRA_EVT
*
*/
typedef struct {
    btmg_hf_vr_state_t value; /*!< voice recognition state */
} btmg_hf_bvra_t;             /*!< HF callback param of BTMG_HF_BVRA_EVT */

/**
* @brief +CIND call status indicator values
*
*/
typedef enum {
    BTMG_HF_CALL_STATUS_NO_CALLS = 0,         /*!< no call in progress  */
    BTMG_HF_CALL_STATUS_CALL_IN_PROGRESS = 1, /*!< call is present(active or held) */
} btmg_hf_call_status_t;

/**
* @brief BTMG_HFP_HF_CIND_CALL_EVT
*
*/
typedef struct btmg_hf_call_ind_param {
    btmg_hf_call_status_t status; /*!< call status indicator */
} btmg_hf_call_ind_t;

/**
* @brief +CIND call setup status indicator values
*
*/
typedef enum {
    BTMG_HF_CALL_SETUP_STATUS_IDLE = 0,              /*!< no call setup in progress */
    BTMG_HF_CALL_SETUP_STATUS_INCOMING = 1,          /*!< incoming call setup in progress */
    BTMG_HF_CALL_SETUP_STATUS_OUTGOING_DIALING = 2,  /*!< outgoing call setup in dialing state */
    BTMG_HF_CALL_SETUP_STATUS_OUTGOING_ALERTING = 3, /*!< outgoing call setup in alerting state */
} btmg_hf_call_setup_status_t;

/**
* @brief BTMG_HFP_HF_CIND_CALL_SETUP_EVT
*
*/
typedef struct {
    btmg_hf_call_setup_status_t status; /*!< call setup status indicator */
} btmg_hf_call_setup_ind_t;             /*!< HF callback param of BTMG_HFP_HF_CIND_CALL_SETUP_EVT */

/**
* @brief +CIND call held indicator values
*
*/
typedef enum {
    BTMG_HF_CALL_HELD_STATUS_NONE = 0,            /*!< no calls held */
    BTMG_HF_CALL_HELD_STATUS_HELD_AND_ACTIVE = 1, /*!< both active and held call */
    BTMG_HF_CALL_HELD_STATUS_HELD = 2,            /*!< call on hold, no active call*/
} btmg_hf_call_held_status_t;

/**
* @brief BTMG_HFP_HF_CIND_CALL_HELD_EVT
*
*/
typedef struct {
    btmg_hf_call_held_status_t status; /*!< bluetooth proprietary call hold status indicator */
} btmg_hf_call_held_ind_t;             /*!< HF callback param of BTMG_HFP_HF_CIND_CALL_HELD_EVT */

/**
* @brief +CIND network service availability status
*
*/
typedef enum {
    BTMG_HF_NETWORK_STATE_NOT_AVAILABLE = 0,
    BTMG_HF_NETWORK_STATE_AVAILABLE
} btmg_hf_network_state_t;

/**
* @brief BTMG_HFP_HF_CIND_SERVICE_AVAILABILITY_EVT
*
*/
typedef struct {
    btmg_hf_network_state_t status; /*!< service availability status */
} btmg_hf_service_availability_t; /*!< HF callback param of BTMG_HFP_HF_CIND_SERVICE_AVAILABILITY_EVT */

/**
* @brief BTMG_HFP_HF_CIND_SIGNAL_STRENGTH_EVT
*
*/
typedef struct {
    int value;                   /*!< signal strength value, ranges from 0 to 5 */
} btmg_hf_signal_strength_ind_t; /*!< HF callback param of BTMG_HFP_HF_CIND_SIGNAL_STRENGTH_EVT */

/**
* @brief +CIND roaming status indicator values
*
*/
typedef enum {
    BTMG_HF_ROAMING_STATUS_INACTIVE = 0, /*!< roaming is not active */
    BTMG_HF_ROAMING_STATUS_ACTIVE,       /*!< a roaming is active */
} btmg_hf_roaming_status_t;

/**
* @brief BTMG_HFP_HF_CIND_ROAMING_STATUS_EVT
*
*/
typedef struct {
    btmg_hf_roaming_status_t status; /*!< roaming status */
} btmg_hf_network_roaming_t;

/**
* @brief BTMG_HFP_HF_CIND_BATTERY_LEVEL_EVT
*
*/
typedef struct {
    int value;                 /*!< battery charge value, ranges from 0 to 5 */
} btmg_hf_battery_level_ind_t; /*!< HF callback param of BTMG_HFP_HF_CIND_BATTERY_LEVEL_EVT */

/**
* @brief BTMG_HFP_HF_COPS_CURRENT_OPERATOR_EVT
*
*/
typedef struct {
    const char *name;         /*!< name of the network operator */
} btmg_hf_current_operator_t; /*!< HF callback param of BTMG_HFP_HF_COPS_CURRENT_OPERATOR_EVT */

/**
* @brief +BTRH response and hold result code
*
*/
typedef enum {
    BTMG_HF_BTRH_STATUS_HELD = 0, /*!< incoming call is put on held in AG */
    BTMG_HF_BTRH_STATUS_ACCEPTED, /*!< held incoming call is accepted in AG */
    BTMG_HF_BTRH_STATUS_REJECTED, /*!< held incoming call is rejected in AG */
} btmg_hf_btrh_status_t;

/**
* @brief BTMG_HFP_HF_BTRH_EVT
*
*/
typedef struct {
    btmg_hf_btrh_status_t status; /*!< call hold and response status result code */
} btmg_hf_btrh_t;                 /*!< HF callback param of BTMG_HFP_HF_BTRH_EVT */

/**
* @brief BTMG_HFP_HF_CLIP_EVT
*
*/
typedef struct {
    const char *number; /*!< phone number string of call */
} btmg_hf_clip_t;       /*!< HF callback param of BTMG_HFP_HF_CLIP_EVT */

/**
* @brief BTMG_HFP_HF_CCWA_EVT
*
*/
typedef struct btmg_hf_ccwa_param {
    const char *number; /*!< phone number string of waiting call */
} btmg_hf_ccwa_t;       /*!< HF callback param of BTMG_HFP_HF_CCWA_EVT */

/**
* @brief +CLCC direction of the call
*
*/
typedef enum {
    BTMG_HF_CURRENT_CALL_DIRECTION_OUTGOING = 0, /*!< outgoing */
    BTMG_HF_CURRENT_CALL_DIRECTION_INCOMING = 1, /*!< incoming */
} btmg_hf_current_call_direction_t;

/**
* @brief +CLCC status of the call
*
*/
typedef enum {
    BTMG_HF_CURRENT_CALL_STATUS_ACTIVE = 0,            /*!< active */
    BTMG_HF_CURRENT_CALL_STATUS_HELD = 1,              /*!< held */
    BTMG_HF_CURRENT_CALL_STATUS_DIALING = 2,           /*!< dialing (outgoing calls only) */
    BTMG_HF_CURRENT_CALL_STATUS_ALERTING = 3,          /*!< alerting (outgoing calls only) */
    BTMG_HF_CURRENT_CALL_STATUS_INCOMING = 4,          /*!< incoming (incoming calls only) */
    BTMG_HF_CURRENT_CALL_STATUS_WAITING = 5,           /*!< waiting (incoming calls only) */
    BTMG_HF_CURRENT_CALL_STATUS_HELD_BY_RESP_HOLD = 6, /*!< call held by response and hold */
} btmg_hf_current_call_status_t;

/**
* @brief +CLCC multi-party call flag
*
*/
typedef enum {
    BTMG_HF_CURRENT_CALL_MPTY_TYPE_SINGLE = 0, /*!< not a member of a multi-party call */
    BTMG_HF_CURRENT_CALL_MPTY_TYPE_MULTI = 1,  /*!< member of a multi-party call */
} btmg_hf_current_call_mpty_type_t;

/**
* @brief BTMG_HFP_HF_CLCC_EVT
*
*/
typedef struct {
    int idx;                               /*!< numbering(starting with 1) of the call */
    btmg_hf_current_call_direction_t dir;  /*!< direction of the call */
    btmg_hf_current_call_status_t status;  /*!< status of the call */
    btmg_hf_current_call_mpty_type_t mpty; /*!< multi-party flag */
    char *number;                          /*!< phone number(optional) */
} btmg_hf_clcc_t;                          /*!< HF callback param of BTMG_HFP_HF_CLCC_EVT */

/**
* @brief Bluetooth HFP audio volume control target
*
*/
typedef enum {
    BTMG_HF_VOLUME_CONTROL_TARGET_SPK = 0, /*!< speaker */
    BTMG_HF_VOLUME_CONTROL_TARGET_MIC,     /*!< microphone */
} btmg_hf_volume_control_target_t;

/**
* @brief BTMG_HFP_HF_VOLUME_CONTROL_EVT
*
*/
typedef struct {
    btmg_hf_volume_control_target_t type; /*!< volume control target, speaker or microphone */
    int volume;                           /*!< gain, ranges from 0 to 15 */
} btmg_hf_volume_control_t; /*!< HF callback param of BTMG_HFP_HF_VOLUME_CONTROL_EVT */

/**
* @brief +CNUM service type of the phone number
*
*/
typedef enum {
    BTMG_HF_SUBSCRIBER_SERVICE_TYPE_UNKNOWN = 0, /*!< unknown */
    BTMG_HF_SUBSCRIBER_SERVICE_TYPE_VOICE,       /*!< voice service */
    BTMG_HF_SUBSCRIBER_SERVICE_TYPE_FAX,         /*!< fax service */
} btmg_hf_subscriber_service_type_t;

/**
* @brief BTMG_HFP_HF_CNUM_EVT
*
*/
typedef struct {
    const char *number;                     /*!< phone number string */
    btmg_hf_subscriber_service_type_t type; /*!< service type that the phone number relates to */
} btmg_hf_cnum_t;

/**
* @brief in-band ring tone state
*
*/
typedef enum {
    BTMG_HF_IN_BAND_RINGTONE_NOT_PROVIDED = 0,
    BTMG_HF_IN_BAND_RINGTONE_PROVIDED,
} btmg_hf_in_band_ring_state_t;

/**
* @brief BTMG_HFP_HF_BSIR_EVT
*
*/
typedef struct {
    btmg_hf_in_band_ring_state_t state; /*!< setting state of in-band ring tone */
} btmg_hf_bsir_t;                       /*!< HF callback param of BTMG_HFP_HF_BSIR_EVT */

/**
* @brief BTMG_HFP_HF_BINP_EVT
*
*/
typedef struct {
    const char *number; /*!< phone number corresponding to the last voice tag in the HF */
} btmg_hf_binp_t;       /*!< HF callback param of BTMG_HFP_HF_BINP_EVT */

/**
* @brief Bluetooth HFP RFCOMM connection and service level connection status
*
*/
typedef enum {
    BTMG_HFP_HF_DISCONNECTED = 0, /*!< RFCOMM data link channel released */
    BTMG_HFP_HF_CONNECTING,       /*!< connecting remote device on the RFCOMM data link*/
    BTMG_HFP_HF_CONNECTED,        /*!< RFCOMM connection established */
    BTMG_HFP_HF_SLC_CONNECTED,    /*!< service level connection established */
    BTMG_HFP_HF_DISCONNECTING,    /*!< disconnecting with remote device on the RFCOMM dat link*/
} btmg_hfp_hf_connection_state_t;

typedef enum {
    BTMG_HFP_AG_DISCONNECTED = 0, /*!< RFCOMM data link channel released */
    BTMG_HFP_AG_CONNECTING,       /*!< connecting remote device on the RFCOMM data link*/
    BTMG_HFP_AG_CONNECTED,        /*!< RFCOMM connection established */
    BTMG_HFP_AG_SLC_CONNECTED,    /*!< service level connection established */
    BTMG_HFP_AG_DISCONNECTING,    /*!< disconnecting with remote device on the RFCOMM dat link*/
} btmg_hfp_ag_connection_state_t;

/**
* @brief AT+CHLD command values
*
*/
typedef enum {
    BTMG_HF_CHLD_TYPE_REL = 0, /*!< <0>, Terminate all held or set UDUB("busy") to a waiting call */
    BTMG_HF_CHLD_TYPE_REL_ACC, /*!< <1>, Terminate all active calls and accepts a waiting/held call */
    BTMG_HF_CHLD_TYPE_HOLD_ACC, /*!< <2>, Hold all active calls and accepts a waiting/held call */
    BTMG_HF_CHLD_TYPE_MERGE,    /*!< <3>, Add all held calls to a conference */
    BTMG_HF_CHLD_TYPE_MERGE_DETACH, /*!< <4>, connect the two calls and disconnects the subscriber from both calls */
    BTMG_HF_CHLD_TYPE_REL_X,  /*!< <1x>, releases specified calls only */
    BTMG_HF_CHLD_TYPE_PRIV_X, /*!< <2x>, request private consultation mode with specified call */
} btmg_hf_chld_type_t;

/**
* @brief AT+BTRH response and hold action code
*
*/
typedef enum {
    BTMG_HF_BTRH_CMD_HOLD = 0,   /*!< put the incoming call on hold */
    BTMG_HF_BTRH_CMD_ACCEPT = 1, /*!< accept a held incoming call */
    BTMG_HF_BTRH_CMD_REJECT = 2, /*!< reject a held incoming call */
} btmg_hf_btrh_cmd_t;

/*******************************************************************************
**
** Function         btmg_hfp_hf_connect
**
** Description      hfp hf connect api.
**
** Parameter        addr  -Pointer to the address of the device.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_hfp_hf_connect(const char *addr);

/*******************************************************************************
**
** Function         btmg_hfp_hf_disconnect
**
** Description      hfp hf disconnect api.
**
** Parameter        addr  -Pointer to the address of the device.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_hfp_hf_disconnect(const char *addr);

/*******************************************************************************
**
** Function         btmg_hfp_hf_start_voice_recognition
**
** Description      start voice recognition
**
** Parameter        void.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_hfp_hf_start_voice_recognition(void);

/*******************************************************************************
**
** Function         btmg_hfp_hf_stop_voice_recognition
**
** Description      stop voice recognition
**
** Parameter        void.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_hfp_hf_stop_voice_recognition(void);

/*******************************************************************************
**
** Function         btmg_hfp_hf_spk_vol_update
**
** Description      Speaker volume synchronization with AG.
**
** Parameter        volume  -gain of the speaker, ranges 0 to 15
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_hfp_hf_spk_vol_update(int volume);

/*******************************************************************************
**
** Function         btmg_hfp_hf_mic_vol_update
**
** Description      microphone volume synchronization with AG.
**
** Parameter        volume  -gain of the microphone, ranges 0 to 15.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_hfp_hf_mic_vol_update(int volume);

/*******************************************************************************
**
** Function         btmg_hfp_hf_dial
**
** Description      Place a call with a specified number, if number is NULL,
**                  last called number is called.
**
** Parameter        number  -Pointer to the string of phone number.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_hfp_hf_dial(const char *number);

/*******************************************************************************
**
** Function         btmg_hfp_hf_dial_memory
**
** Description      Place a call with number specified by location(speed dial)
**
** Parameter        location  -location of the number in the memory.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_hfp_hf_dial_memory(int location);

/*******************************************************************************
**
** Function         btmg_hfp_hf_send_chld_cmd
**
** Description      Send call hold and multiparty commands, or enhanced call
**                  control commands(Use AT+CHLD)
**
** Parameter        chld  -AT+CHLD call hold and multiparty handling AT command.
**                  idx  -used in Enhanced Call Control Mechanisms, used if chld is
**                        BTMG_HF_CHLD_TYPE_REL_X or BTMG_HF_CHLD_TYPE_PRIV_X
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_hfp_hf_send_chld_cmd(btmg_hf_chld_type_t chld, int idx);

/*******************************************************************************
**
** Function         btmg_hfp_hf_send_btrh_cmd
**
** Description      Send response and hold action command(Send AT+BTRH command).
**
** Parameter        btrh  -response and hold action to send
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_hfp_hf_send_btrh_cmd(btmg_hf_btrh_cmd_t btrh);

/*******************************************************************************
**
** Function         btmg_hfp_hf_answer_call
**
** Description      Answer an incoming call(send ATA command).
**
** Parameter        void.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_hfp_hf_answer_call(void);

/*******************************************************************************
**
** Function         btmg_hfp_hf_reject_call
**
** Description      reject an incoming call(send AT+CHUP command)
**
** Parameter        void.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_hfp_hf_reject_call(void);

/*******************************************************************************
**
** Function         btmg_hfp_hf_query_calls
**
** Description      Query list of current calls in AG(send AT+CLCC command).
**
** Parameter        void.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_hfp_hf_query_calls(void);

/*******************************************************************************
**
** Function         btmg_hfp_hf_query_operator
**
** Description      Query the name of currently selected network operator in AG
**                  (use AT+COPS commands)
**
** Parameter        void.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_hfp_hf_query_operator(void);

/*******************************************************************************
**
** Function         btmg_hfp_hf_query_operator
**
** Description      Get subscriber information number from AG(send AT+CNUM command)
**
** Parameter        void.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_hfp_hf_query_number(void);

/*******************************************************************************
**
** Function         btmg_hfp_hf_send_dtmf
**
** Description      Transmit DTMF codes during an ongoing call(use AT+VTS commands)
**
** Parameter        code  -dtmf code, single ascii character in the set 0-9, #, *, A-D.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_hfp_hf_send_dtmf(char code);

/*******************************************************************************
**
** Function         btmg_hfp_hf_request_last_voice_tag_number
**
** Description      Request a phone number from AG corresponding to last voice tag recorded
**                  (send AT+BINP command).
**
** Parameter        code  -dtmf code, single ascii character in the set 0-9, #, *, A-D.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_hfp_hf_request_last_voice_tag_number(void);

/*******************************************************************************
**
** Function         btmg_hfp_hf_send_nrec
**
** Description      Disable echo cancellation and noise reduction in the AG (use AT+NREC=0 command)
**
** Parameter        void.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_hfp_hf_send_nrec(void);

typedef void (*bt_hfp_hf_connection_state_cb)(const char *bd_addr,
                                              btmg_hfp_hf_connection_state_t state);
typedef void (*bt_hfp_hf_event_cb)(btmg_hfp_hf_event_t event, void *data);

/**
* @brief hfp hf callback
*
*/
typedef struct {
    bt_hfp_hf_connection_state_cb conn_state_cb;
    bt_hfp_hf_event_cb event_cb;
} btmg_hfp_hf_callback_t;

/**
 * @brief  HFP_AG callback events.
 *
 */
typedef enum
{
    BTMG_HFP_AG_CONNECTION_STATE_EVT = 0,          /*!< Connection state changed event */
    BTMG_HFP_AG_AUDIO_STATE_EVT,                   /*!< Audio connection state change event */
    BTMG_HFP_AG_BVRA_RESPONSE_EVT,                 /*!< Voice recognition state change event */
    BTMG_HFP_AG_VOLUME_CONTROL_EVT,                /*!< Audio volume control command from HF Client, provided by +VGM or +VGS message */

    BTMG_HFP_AG_UNAT_RESPONSE_EVT,                 /*!< Unknown AT cmd Response*/
    BTMG_HFP_AG_IND_UPDATE_EVT,                    /*!< Indicator Update Event*/
    BTMG_HFP_AG_CIND_RESPONSE_EVT,                 /*!< Call And Device Indicator Response*/
    BTMG_HFP_AG_COPS_RESPONSE_EVT,                 /*!< Current operator information */
    BTMG_HFP_AG_CLCC_RESPONSE_EVT,                 /*!< List of current calls notification */
    BTMG_HFP_AG_CNUM_RESPONSE_EVT,                 /*!< Subscriber information response from HF Client */
    BTMG_HFP_AG_VTS_RESPONSE_EVT,                  /*!< Enable or not DTMF */
    BTMG_HFP_AG_NREC_RESPONSE_EVT,                 /*!< Enable or not NREC */

    BTMG_HFP_AG_ATA_RESPONSE_EVT,                  /*!< Answer an Incoming Call */
    BTMG_HFP_AG_CHUP_RESPONSE_EVT,                 /*!< Reject an Incoming Call */
    BTMG_HFP_AG_DIAL_EVT,                          /*!< Origin an outgoing call with specific number or the dial the last number */
    BTMG_HFP_AG_WBS_RESPONSE_EVT,                  /*!< Codec Status */
    BTMG_HFP_AG_BCS_RESPONSE_EVT,                  /*!< Final Codec Choice */
} btmg_hfp_ag_event_t;

/// Bluetooth HFP RFCOMM connection and service level connection status
typedef enum {
    BTMG_AG_CONNECTION_STATE_DISCONNECTED = 0,     /*!< RFCOMM data link channel released */
    BTMG_AG_CONNECTION_STATE_CONNECTING,           /*!< connecting remote device on the RFCOMM data link*/
    BTMG_AG_CONNECTION_STATE_CONNECTED,            /*!< RFCOMM connection established */
    BTMG_AG_CONNECTION_STATE_SLC_CONNECTED,        /*!< service level connection established */
    BTMG_AG_CONNECTION_STATE_DISCONNECTING,        /*!< disconnecting with remote device on the RFCOMM data link*/
} btmg_ag_connection_state_t;

/**
 * @brief  BTMG_HFP_AG_CONNECTION_STATE_EVT
 */
typedef struct {
    btmg_ag_connection_state_t state;          /*!< Connection state */
    uint32_t peer_feat;                       /*!< HF supported features */
    uint32_t chld_feat;                       /*!< AG supported features on call hold and multiparty services */
} btmg_ag_conn_stat_t;                                  /*!< AG callback param of BTMG_AG_CONNECTION_STATE_EVT */

/// Bluetooth HFP audio connection status
typedef enum {
    BTMG_AG_AUDIO_STATE_DISCONNECTED = 0,          /*!< audio connection released */
    BTMG_AG_AUDIO_STATE_CONNECTING,                /*!< audio connection has been initiated */
    BTMG_AG_AUDIO_STATE_CONNECTED,                 /*!< audio connection is established */
    BTMG_AG_AUDIO_STATE_CONNECTED_MSBC,            /*!< mSBC audio connection is established */
} btmg_ag_audio_state_t;

/**
 * @brief BTMG_HFP_AG_AUDIO_STATE_EVT
 */
typedef struct {
    btmg_ag_audio_state_t state;               /*!< audio connection state */
} btmg_ag_audio_stat_t;                                 /*!< AG callback param of BTMG_AG_AUDIO_STATE_EVT */

/// voice recognition state
typedef enum {
    BTMG_AG_VR_STATE_DISABLED = 0,           /*!< voice recognition disabled */
    BTMG_AG_VR_STATE_ENABLED,                /*!< voice recognition enabled */
} btmg_ag_vr_state_t;

/**
 * @brief BTMG_HFP_AG_BVRA_RESPONSE_EVT
 */
typedef struct {
    btmg_ag_vr_state_t value;                  /*!< voice recognition state */
} btmg_ag_vra_rep_t;                                    /*!< AG callback param of BTMG_AG_BVRA_RESPONSE_EVT */

typedef enum {
    BTMG_AG_VOLUME_TYPE_SPK = 0,
    BTMG_AG_VOLUME_TYPE_MIC
} btmg_ag_volume_type_t;

/**
 * @brief BTMG_HFP_AG_VOLUME_CONTROL_EVT
 */
typedef struct {
    btmg_ag_volume_type_t type;                /*!< volume control target, speaker or microphone */
    int volume;                               /*!< gain, ranges from 0 to 15 */
} btmg_ag_volume_control_t;                             /*!< AG callback param of BTMG_AG_VOLUME_CONTROL_EVT */

/**
 * @brief BTMG_HFP_AG_UNAT_RESPONSE_EVT
 */
typedef struct {
    char *unat;                               /*!< unknown AT command string */
} btmg_ag_unat_rep_t;                                    /*!< AG callback param of BTMG_AG_UNAT_RESPONSE_EVT */

/// +CIND call status indicator values
typedef enum {
    BTMG_AG_CALL_STATUS_NO_CALLS = 0,                  /*!< no call in progress  */
    BTMG_AG_CALL_STATUS_CALL_IN_PROGRESS = 1,          /*!< call is present(active or held) */
} btmg_ag_call_status_t;

/// +CIND call setup status indicator values
typedef enum {
    BTMG_AG_CALL_SETUP_STATUS_IDLE = 0,                /*!< no call setup in progress */
    BTMG_AG_CALL_SETUP_STATUS_INCOMING = 1,            /*!< incoming call setup in progress */
    BTMG_AG_CALL_SETUP_STATUS_OUTGOING_DIALING = 2,    /*!< outgoing call setup in dialing state */
    BTMG_AG_CALL_SETUP_STATUS_OUTGOING_ALERTING = 3,   /*!< outgoing call setup in alerting state */
} btmg_ag_call_setup_status_t;

/// +CIND network service availability status
typedef enum
{
    BTMG_AG_NETWORK_STATE_NOT_AVAILABLE = 0,
    BTMG_AG_NETWORK_STATE_AVAILABLE
} btmg_ag_network_state_t;

/// +CIND roaming status indicator values
typedef enum {
    BTMG_AG_ROAMING_STATUS_INACTIVE = 0,               /*!< roaming is not active */
    BTMG_AG_ROAMING_STATUS_ACTIVE,                     /*!< a roaming is active */
} btmg_ag_roaming_status_t;

/// +CIND call held indicator values
typedef enum {
    BTMG_AG_CALL_HELD_STATUS_NONE = 0,                 /*!< no calls held */
    BTMG_AG_CALL_HELD_STATUS_HELD_AND_ACTIVE = 1,      /*!< both active and held call */
    BTMG_AG_CALL_HELD_STATUS_HELD = 2,                 /*!< call on hold, no active call*/
} btmg_ag_call_held_status_t;

/**
 * @brief BTMG_HFP_AG_CIND_RESPONSE_EVT
 */
typedef struct {
    btmg_ag_call_status_t       call_status;         /*!< call status indicator */
    btmg_ag_call_setup_status_t call_setup_status;   /*!< call setup status indicator */
    btmg_ag_network_state_t svc;                     /*!< network service availability status */
    int signal_strength;                            /*!< signal strength */
    btmg_ag_roaming_status_t roam;                   /*!< roam state */
    int battery_level;                              /*!< battery charge value, ranges from 0 to 5 */
    btmg_ag_call_held_status_t  call_held_status;    /*!< bluetooth proprietary call hold status indicator */
} btmg_ag_cind_t;                                             /*!< AG callback param of BTMG_AG_CIND_RESPONSE_EVT */

/**
 * @brief BTMG_HFP_AG_DIAL_EVT
 */
typedef struct {
    char *num_or_loc;                         /*!< location in phone memory */
} btmg_ag_out_call_t;                                   /*!< AG callback param of BTMG_AG_DIAL_EVT */

/**
 * @brief BTMG_HFP_AG_VTS_RESPONSE_EVT
 */
typedef struct {
    char *code;                               /*!< MTF code from HF Client */
} btmg_ag_vts_rep_t;                                     /*!< AG callback param of BTMG_AG_VTS_RESPONSE_EVT */

/* +NREC */
typedef enum
{
    BTMG_AG_NREC_STOP = 0,
    BTMG_AG_NREC_START
} btmg_ag_nrec_state_t;

/**
 * @brief BTMG_HFP_AG_NREC_RESPONSE_EVT
 */
typedef struct {
    btmg_ag_nrec_state_t state;                       /*!< NREC enabled or disabled */
} btmg_ag_nrec_t;                                       /*!< AG callback param of BTMG_AG_NREC_RESPONSE_EVT */

/* WBS codec setting */
typedef enum
{
   BTMG_AG_WBS_NONE,
   BTMG_AG_WBS_NO,
   BTMG_AG_WBS_YES
} btmg_ag_wbs_config_t;

/**
 * @brief BTMG_HFP_AG_WBS_RESPONSE_EVT
 */
typedef struct {
    btmg_ag_wbs_config_t codec;                /*!< codec mode CVSD or mSBC */
} btmg_ag_wbs_rep_t;                                    /*!< AG callback param of BTMG_AG_WBS_RESPONSE_EVT */

/**
 * @brief BTMG_HFP_AG_BCS_RESPONSE_EVT
 */
typedef struct {
    btmg_ag_wbs_config_t mode;                 /*!< codec mode CVSD or mSBC */
} btmg_ag_bcs_rep_t;                                    /*!< AG callback param of BTMG_AG_BCS_RESPONSE_EVT */

/*******************************************************************************
**
** Function         btmg_hfp_ag_connect
**
** Description      hfp ag connect api.
**
** Parameter        addr  -Pointer to the address of the device.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_hfp_ag_connect(const char *addr);

/*******************************************************************************
**
** Function         btmg_hfp_ag_disconnect
**
** Description      hfp ag disconnect api.
**
** Parameter        addr  -Pointer to the address of the device.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_hfp_ag_disconnect(const char *addr);

/*******************************************************************************
**
** Function         btmg_hfp_ag_connect_audio
**
** Description      hfp ag audio connect api.
**
** Parameter        void.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_hfp_ag_connect_audio(const char *addr);

/*******************************************************************************
**
** Function         btmg_hfp_ag_disconnect_audio
**
** Description      hfp ag audio disconnect api.
**
** Parameter        void.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_hfp_ag_disconnect_audio(const char *addr);

/*******************************************************************************
**
** Function         btmg_hfp_ag_spk_vol_update
**
** Description      microphone volume synchronization with AG.
**
** Parameter        volume  -gain of the microphone, ranges 0 to 15.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_hfp_ag_spk_vol_update(const char *addr, int volume);

typedef void (*bt_hfp_ag_connection_state_cb)(const char *bd_addr,
                                              btmg_hfp_hf_connection_state_t state);
typedef void (*bt_hfp_ag_event_cb)(btmg_hfp_ag_event_t event, void *data);
typedef void (*bt_hfp_ag_incoming_cb)(const uint8_t *buf, uint32_t sz);
typedef uint32_t (*bt_hfp_ag_outgoing_cb)(uint8_t *buf, uint32_t sz);

/**
* @brief hfp hf callback
*
*/
typedef struct {
    bt_hfp_ag_connection_state_cb conn_state_cb;
    bt_hfp_ag_event_cb event_cb;
    bt_hfp_ag_incoming_cb audio_incoming_cb;
    bt_hfp_ag_outgoing_cb audio_outgoing_cb;
} btmg_hfp_ag_callback_t;

/**
* @brief spp connection state.
*
*/
typedef enum {
    BTMG_SPP_DISCONNECTED,
    BTMG_SPP_CONNECTING,
    BTMG_SPP_CONNECTED,
    BTMG_SPP_DISCONNECTING,
    BTMG_SPP_CONNECT_FAILED,
    BTMG_SPP_DISCONNEC_FAILED,
} btmg_spp_connection_state_t;

typedef void (*bt_sppc_connection_state_cb)(const char *bd_addr, btmg_spp_connection_state_t state);
typedef void (*bt_sppc_recvdata_cb)(const char *bd_addr, char *data, int data_len);
/**
* @brief spp client callback.
*
*/
typedef struct {
    bt_sppc_connection_state_cb conn_state_cb;
    bt_sppc_recvdata_cb recvdata_cb;
} btmg_sppc_callback_t;

/*******************************************************************************
**
** Function         btmg_sppc_connect
**
** Description      Make an spp connection to remote device.
**
** Parameter        dst  -Pointer to the address of the device.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_sppc_connect(const char *dst);

/*******************************************************************************
**
** Function         btmg_sppc_disconnect
**
** Description      Disconnect spp connection.
**
** Parameter        dst  -Pointer to the address of the device.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_sppc_disconnect(const char *dst);

/*******************************************************************************
**
** Function         btmg_sppc_write
**
** Description      This function is used to write data.
**
** Parameter        data  -Pointer to the data to be sent.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_sppc_write(char *data, uint32_t len);

/*SPP SERVER*/
typedef void (*bt_spps_connection_state_cb)(const char *bd_addr, btmg_spp_connection_state_t state);
typedef void (*bt_spps_recvdata_cb)(const char *bd_addr, char *data, int data_len);
/**
* @brief spp server callback.
*
*/
typedef struct {
    bt_spps_connection_state_cb conn_state_cb;
    bt_spps_recvdata_cb recvdata_cb;
} btmg_spps_callback_t;

/*******************************************************************************
**
** Function         btmg_spps_start
**
** Description      create a SPP server and starts listening for an
**                  spp connection request from a remote Bluetooth device.
**
** Parameter        scn  -The specific channel.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_spps_start(int scn);

/*******************************************************************************
**
** Function         btmg_spps_stop
**
** Description      This function stops a specific SPP server.
**
** Parameter        void.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_spps_stop(void);

/*******************************************************************************
**
** Function         btmg_spps_write
**
** Description      This function is used to write data.
**
** Parameter        data  -The data written.
**                  len  -The length of the data written.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_spps_write(char *data, uint32_t len);

/*******************************************************************************
**
** Function         btmg_spps_disconnect
**
** Description      This function closes an SPP connection.
**
** Parameter        dst  -Pointer to the address of the device.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_spps_disconnect(const char *dst);

#ifdef CONFIG_BLEHOST
#include "btmg_gatt_db.h"
/*BLE*/
// typedef enum {
// 	LE_EVENT_GET_NAME = 0,
// 	LE_EVENT_SCAN,
// 	......
// } btmg_le_event;

/**
* @brief Bluetooth UUID types
*
*/
typedef struct {
    enum {
        BTMG_UUID_UN_SPEC = 0,
        BTMG_UUID_16 = 16,     /** UUID type 16-bit. */
        BTMG_UUID_32 = 32,     /** UUID type 32-bit. */
        BTMG_UUID_128 = 128,   /** UUID type 128-bit. */
    } type;
    union {
        uint16_t u16;
        uint32_t u32;
        uint8_t u128[16];
    } value;
} btmg_uuid_t;

/*******************************************************************************
**
** Function         btmg_uuid_to_uuid128
**
** Description      Convert UUID to 128-bit UUID.
**
** Parameter        src  -UUID to convert.
**                  dst  -Converted UUID.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_uuid_to_uuid128(const btmg_uuid_t *src, btmg_uuid_t *dst);

/*******************************************************************************
**
** Function         btmg_uuid_to_string
**
** Description      Convert Bluetooth UUID to string.
**
** Parameter        uuid  -Bluetooth UUID.
**                  str  -pointer where to put converted string.
**                  n  -length of str.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_uuid_to_string(const btmg_uuid_t *uuid, char *str, size_t n);

/*******************************************************************************
**
** Function         btmg_le_set_name
**
** Description      Set the local BLE name.
**
** Parameter        name  - name content.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_le_set_name(const char *name);


/*******************************************************************************
**
** Function         btmg_le_get_name
**
** Description      Get local BLE name.
**
** Parameter        void.
**
** Returns          name string.
**
*******************************************************************************/
const char *btmg_le_get_name(void);


/*******************************************************************************
**
** Function         btmg_le_enable_adv
**
** Description      Turn on/off BLE Advertising
**
** Parameter        enable  - true or flase.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_le_enable_adv(bool enable);

/*******************************************************************************
**
** Function         btmg_le_enable_ext_adv
**
** Description      Turn on/off BLE Extended Advertising
**
** Parameter        enable  - true or flase.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_le_enable_ext_adv(bool enable);

/**
* @brief BLE address type.
*
*/
typedef enum {
    BTMG_LE_PUBLIC_ADDRESS = 0x00,
    BTMG_LE_RANDOM_ADDRESS = 0x01,
    BTMG_LE_PUBLIC_ADDRESS_ID = 0x02,
    BTMG_LE_RANDOM_ADDRESS_ID = 0x03,
} btmg_le_addr_type_t;

/**
* @brief Bluetooth address.
*
*/
typedef struct {
    uint8_t val[6];
} btmg_addr_t;

/**
* @brief BLE address.
*
*/
typedef struct {
    btmg_le_addr_type_t type;
    btmg_addr_t addr;
} btmg_le_addr_t;

/**
* @brief Advertising filter policy.
*
*/
typedef int btmg_le_adv_filter_policy_t;

/**
* @brief BLE advertising type.
*
*/
typedef enum {
    BTMG_LE_ADV_IND = 0x00,             /*connectable and scannable undirected advertising*/
    BTMG_LE_ADV_DIRECT_HIGH_IND = 0x01, /*connectable high duty cycle directed advertising*/
    BTMG_LE_ADV_SCAN_IND = 0x02,        /*scannable undirected advertising*/
    BTMG_LE_ADV_NONCONN_IND = 0x03,     /*non connectable undirected advertising*/
    BTMG_LE_ADV_DIRECT_LOW_IND = 0x04,  /*connectable low duty cycle directed advertising*/
    BTMG_LE_ADV_TYPE_MAX = 0x05,
} btmg_le_adv_type_t;

/**
* @brief Advertising filter policy.
*
*/
typedef enum {
    /* process scan and connection requests from all devices */
    BTMG_LE_PROCESS_ALL_REQ = 0x00,
    /* process connection request from all devices and scan request only from white list */
    BTMG_LE_PROCESS_CONN_REQ = 0x01,
    /* process scan request from all devices and connection request only from white list */
    BTMG_LE_PROCESS_SCAN_REQ = 0x02,
    /* process requests only from white list*/
    BTMG_LE_PROCESS_WHITE_LIST_REQ = 0x03,
    BTMG_LE_FILTER_POLICY_MAX = 0x04,
} btmg_le_advertising_filter_policy_t;

/**
* @brief Remote BLE device address type.
*
*/
typedef enum {
    /* public device address(default) or public indentiy address */
    BTMG_LE_PEER_PUBLIC_ADDRESS = 0x00,
    /* random device address(default) or random indentiy address */
    BTMG_LE_PEER_RANDOM_ADDRESS = 0x01,
} btmg_le_peer_addr_type_t;

/**
* @brief BLE Advertising Parameters.
*
*/
typedef struct {
	/** Minimum Advertising Interval (N * 0.625 milliseconds)
	 * Minimum Advertising Interval shall be less than or equal to the
	 * Maximum Advertising Interval. The Minimum Advertising Interval and
	 * Maximum Advertising Interval should not be the same value (as stated
	 * in Bluetooth Core Spec 5.2, section 7.8.5)
	 * Range: 0x0020 to 0x4000
	 */
    uint16_t interval_min;
	/** Maximum Advertising Interval (N * 0.625 milliseconds)
	 * Minimum Advertising Interval shall be less than or equal to the
	 * Maximum Advertising Interval. The Minimum Advertising Interval and
	 * Maximum Advertising Interval should not be the same value (as stated
	 * in Bluetooth Core Spec 5.2, section 7.8.5)
	 * Range: 0x0020 to 0x4000
	 */
    uint16_t interval_max;
    /* advertising type */
    btmg_le_adv_type_t adv_type;
    /* Owner bluetooth device address type */
    btmg_le_addr_type_t own_addr_type;
    /* Peer device bluetooth device address */
    btmg_le_peer_addr_type_t peer_addr_type;
    /* Peer device BLE address */
    char peer_addr[18];
    struct {
        /* Setting this bit to 1 will turn off advertising on channel 37 */
        uint8_t ch_37_off : 1;
        /* Setting this bit to 1 will turn off advertising on channel 38 */
        uint8_t ch_38_off : 1;
        /* Setting this bit to 1 will turn off advertising on channel 39 */
        uint8_t ch_39_off : 1;
    } ch_mask;
    btmg_le_adv_filter_policy_t filter;
} btmg_le_adv_param_t;

typedef struct {
    uint8_t handle;
    uint16_t props;
    uint8_t prim_min_interval[3];
    uint8_t prim_max_interval[3];
    uint8_t prim_channel_map;
    uint8_t own_addr_type;
    bt_addr_le_t peer_addr;
    uint8_t filter_policy;
    int8_t tx_power;
    uint8_t prim_adv_phy;
    uint8_t sec_adv_max_skip;
    uint8_t sec_adv_phy;
    uint8_t sid;
    uint8_t scan_req_notify_enable;
} btmg_le_ext_adv_param_t;

/*******************************************************************************
**
** Function         btmg_le_set_adv_param
**
** Description      Set Advertising Parameters.
**
** Parameter        adv_param  - Parameters.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_le_set_adv_param(btmg_le_adv_param_t *adv_param);

/*******************************************************************************
**
** Function         btmg_le_set_ext_adv_param
**
** Description      Set Extended Advertising Parameters.
**
** Parameter        ext_adv_param  - Parameters.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_le_set_ext_adv_param(btmg_le_ext_adv_param_t *ext_adv_param);

/**
* @brief  Scan response data
*
*/
typedef struct {
    uint8_t data[31];
    uint8_t data_len;
} btmg_adv_scan_rsp_data_t;

/*******************************************************************************
**
** Function         btmg_le_set_adv_scan_rsp_data
**
** Description      Set scan response data
**
** Parameter        adv_data  - Advertising data.
**                  scan_rsp_data - Scan response data
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_le_set_adv_scan_rsp_data(btmg_adv_scan_rsp_data_t *adv_data,
                                       btmg_adv_scan_rsp_data_t *scan_rsp_data);

/**
* @brief  BLE Scan type.
*
*/
typedef enum {
    /* passive scanning */
    LE_SCAN_TYPE_PASSIVE = 0x00,
    /* active scanning */
    LE_SCAN_TYPE_ACTIVE,
} btmg_le_scan_type_t;

/**
* @brief  Ble scan duplicate type.
*
*/
typedef enum {
    /* the Link Layer should generate advertising reports to the host for each packet received */
    LE_SCAN_DUPLICATE_DISABLE = 0x0,
    /* the Link Layer should filter out duplicate advertising reports to the Host */
    LE_SCAN_DUPLICATE_ENABLE = 0x1,
} btmg_le_scan_filter_duplicate_t;

/**
* @brief  Ble scan filter type.
*
*/
typedef enum {
/* Accept all :
1. advertisement packets except directed advertising packets not addressed to this device (default). */
    LE_SCAN_FILTER_POLICY_ALLOW_ALL = 0,
/* Accept only :
1. advertisement packets from devices where the advertiser’s address is in the White list.
2. Directed advertising packets which are not addressed for this device shall be ignored. */
    LE_SCAN_FILTER_POLICY_ONLY_WLIST,
/* Accept all :
1. undirected advertisement packets, and
2. directed advertising packets where the initiator address is a resolvable private address, and
3. directed advertising packets addressed to this device. */
    LE_SCAN_FILTER_POLICY_UND_RPA_DIR,
/* Accept all :
1. advertisement packets from devices where the advertiser’s address is in the White list, and
2. directed advertising packets where the initiator address is a resolvable private address, and
3. directed advertising packets addressed to this device.*/
    LE_SCAN_FILTER_POLICY_WLIST_RPA_DIR,
} btmg_le_scan_filter_policy_t;

/**
* @brief  Ble scan parameters.
*
*/
typedef struct {
    /* Scan type */
    btmg_le_scan_type_t scan_type;
    /* Range: 0x0004 to 0x4000 Default: 0x0010 (10 ms)
    * Time = N * 0.625 msec
    * Time Range: 2.5 msec to 10.24 seconds*/
    uint16_t scan_interval;
    /* Range: 0x0004 to 0x4000 Default: 0x0010 (10 ms)
    * Time = N * 0.625 msec
    * Time Range: 2.5 msec to 10.24 seconds
    * shall be less than or equal to scan_interval*/
    uint16_t scan_window;
    /* whether the Link Layer should filter out duplicate advertising reports to the Host,
    *or if the Link Layer should generate advertising reports for each packet received */
    btmg_le_scan_filter_duplicate_t filter_duplicate; //是否重复上报
    /* Scan filter policy */
    btmg_le_scan_filter_policy_t filter_policy;
    uint16_t timeout; // Scan timeout between 0x0001 and 0xFFFF in seconds, 0x0000 disables timeout. //
} btmg_le_scan_param_t;

typedef struct {
    uint8_t conn_id;
    btmg_le_addr_t addr;
} gattc_connected_list_para_t;

btmg_err btmg_le_scan_start(btmg_le_scan_param_t *scan_param);
btmg_err btmg_le_scan_stop(void);
btmg_err btmg_le_whitelist_add(btmg_le_addr_t *addr);
btmg_err btmg_le_white_list_remove(btmg_le_addr_t *addr);
btmg_err btmg_le_whitelist_clear(void);
btmg_err btmg_le_set_chan_map(uint8_t chan_map[5]);
void btmg_le_get_connected_num(int *conn_count);
btmg_err btmg_le_get_connected_list(gattc_connected_list_para_t *param);

/** LE Connection Info Structure */
typedef struct {
    /** Source (Local) Identity Address */
    const btmg_le_addr_t *src;
    /** Destination (Remote) Identity Address or remote Resolvable Private
	 *  Address (RPA) before identity has been resolved.
	 */
    const btmg_le_addr_t *dst;
    /** Local device address used during connection setup. */
    const btmg_le_addr_t *local;
    /** Remote device address used during connection setup. */
    const btmg_le_addr_t *remote;
    uint16_t interval; /** Connection interval */
    uint16_t latency;  /** Connection slave latency */
    uint16_t timeout;  /** Connection supervision timeout */
} btmg_le_conn_info_t;

btmg_err btmg_le_conn_get_info(uint8_t conn_id, btmg_le_conn_info_t *info);

typedef struct {
    uint16_t min_conn_interval; // Range: 0x0006 to 0x0C80, Time = N * 1.25 msec, Time Range: 7.5 msec to 4 seconds.
    uint16_t max_conn_interval; // Range: 0x0006 to 0x0C80, Time = N * 1.25 msec, Time Range: 7.5 msec to 4 seconds.
    uint16_t slave_latency; // Range: 0x0000 to 0x01F3
    uint16_t conn_sup_timeout; // Range: 0x000A to 0x0C80, Time = N * 10 msec, Time Range: 100 msec to 32 seconds
} btmg_le_conn_param_t;

btmg_err btmg_le_conn_param_update(uint8_t conn_id, btmg_le_conn_param_t *param);
btmg_err btmg_le_connect(const btmg_le_addr_t *peer, btmg_le_conn_param_t *conn_param);
btmg_err btmg_le_disconnect(uint8_t conn_id, uint8_t reason);
btmg_err btmg_le_unpair(btmg_le_addr_t *addr);
btmg_err btmg_le_connect_auto_start(btmg_le_conn_param_t *conn_param);
btmg_err btmg_le_connect_auto_stop(void);
btmg_err btmg_le_set_auto_connect(btmg_le_addr_t *addr, btmg_le_conn_param_t *param);
int btmg_le_get_security(uint8_t conn_id);
btmg_err btmg_le_set_security(uint8_t conn_id, int level);
btmg_err btmg_le_smp_set_iocap(btmg_io_capability_t io_cap);
btmg_err btmg_le_smp_passkey_entry(uint8_t conn_id, uint32_t passkey);
btmg_err btmg_le_smp_cancel(uint8_t conn_id);
btmg_err btmg_le_smp_passkey_confirm(uint8_t conn_id);
btmg_err btmg_le_smp_pairing_confirm(uint8_t conn_id);

int btmg_le_gatt_mtu_exchange(uint8_t conn_id);
int btmg_le_conn_get_mtu(uint8_t conn_id);

/** @brief 服务类型
 */
typedef enum {
    BTMG_BLE_PRIMARY_SERVICE = 0,
    BTMG_BLE_SECONDARY_SERVICE,
    BTMG_BLE_INCLUDE_SERVICE,
} btmg_gatts_service_type_t;

#ifndef BIT
#define BIT(n) (1UL << (n))
#endif

/** @brief GATT attribute permission bit field values. same as zephyr
 */
enum {
    BTMG_GATT_PERM_NONE = 0,
    BTMG_GATT_PERM_READ = BIT(0),
    BTMG_GATT_PERM_WRITE = BIT(1),
    BTMG_GATT_PERM_READ_ENCRYPT = BIT(2),
    BTMG_GATT_PERM_WRITE_ENCRYPT = BIT(3),
    BTMG_GATT_PERM_READ_AUTHEN = BIT(4),
    BTMG_GATT_PERM_WRITE_AUTHEN = BIT(5),
    BTMG_GATT_PERM_PREPARE_WRITE = BIT(6),
};
typedef uint8_t btmg_gatt_permission_t;

/** @brief Characteristic Properties Bit field values same as zephyr
 */
#define BTMG_GATT_CHRC_BROADCAST          0x01
#define BTMG_GATT_CHRC_READ               0x02
#define BTMG_GATT_CHRC_WRITE_WITHOUT_RESP 0x04
#define BTMG_GATT_CHRC_WRITE              0x08
#define BTMG_GATT_CHRC_NOTIFY             0x10
#define BTMG_GATT_CHRC_INDICATE           0x20
#define BTMG_GATT_CHRC_AUTH               0x40
#define BTMG_GATT_CHRC_EXT_PROP           0x80
typedef uint8_t btmg_gatt_properties_t;

/** @brief 一条特征条目，协议栈需要在此处对齐
 */
// typedef struct {
// 	uint16_t handle;
// 	btmg_uuid_t uuid;
// 	btmg_gatt_permission_t perm;
// 	// read;
// 	// write;
// 	// void *user_data;
// } btmg_gatt_attr_t;

/** @brief databse
 */
typedef struct {
    // uint8_t *entry; // 分配的内存，协议栈不再动态分配内存
    int char_num;                             // 特征的数量
    ll_stack_gatt_server_t stack_gatt_server; // 协议栈的database结构体
} btmg_gatt_db_t;                             // 可以包含1或多个服务

/** @brief 创建database
 *  @praram num_attr: num of attr
 *  @return db: database malloc.
 */
btmg_gatt_db_t *btmg_gatt_service_db_create(int num_attr);

/** @brief 创建database
 *  @praram num_attr: num of attr
 *  @return db: database malloc.
 */
btmg_gatt_db_t *btmg_gatt_attr_create(int num_attr);

/** @brief 销毁database 需应用手动设置为NULL
 *  @param db: database ptr
 *  @return 0
 */
btmg_err btmg_gatt_attr_destory(btmg_gatt_db_t *db);

/** @brief 添加服务
 *  @praram db: database ptr
 *  @praram service_uuid: service uuid
 *  @return 0 in case of success or negative value in case of error.
 */
btmg_err btmg_gatt_attr_primary_service(btmg_gatt_db_t *db, btmg_uuid_t service_uuid);

/** @brief 添加特征值
 *  @praram db: database ptr
 *  @praram uuid: character uuid
 *  @praram props: character properties
 *  @praram perm: character perm
 *  @return 0 in case of success or negative value in case of error.
 */
btmg_err btmg_gatt_attr_characteristic(btmg_gatt_db_t *db, btmg_uuid_t uuid,
                                       btmg_gatt_properties_t props, btmg_gatt_permission_t perm);

/** @brief 添加特征用户配置
 *  @praram db: database ptr
 *  @praram perm: character perm
 *  @return 0 in case of success or negative value in case of error.
 */
btmg_err btmg_gatt_attr_ccc(btmg_gatt_db_t *db, btmg_gatt_permission_t perm);

/** @brief 注册服务
 *  @praram db: database ptr
 *  @return 0 in case of success or negative value in case of error.
 */
btmg_err btmg_gatt_register_service(btmg_gatt_db_t *db);

/** @brief 取消注册服务
 *  @praram db: database ptr
 *  @return 0 in case of success or negative value in case of error.
 */
btmg_err btmg_gatt_unregister_service(btmg_gatt_db_t *db);

/** @brief 遍历服务
 *
 *  按照att条目遍历database，注册后可以获得handle
 *
 *  @praram db: database ptr
 *  @praram fun: callback fun ptr
 *  @praram len: buffer len
 *  @return 0 in case of success or negative value in case of error.
 */
// btmg_err btmg_gatt_foreach_chrc(btmg_gatt_db_t *db, int (*fun)(btmg_gatt_attr_t *chrc));
btmg_err btmg_gatt_get_db(void);

/**
 * @praram conn_id: connections identity
 * @praram handle: character handle
 * @praram buf: buffer
 * @praram len: buffer len
 * @return 0 in case of success or negative value in case of error.
 */
// btmg_err btmg_gatts_notify(uint8_t conn_id, uint16_t handle, void *buf, size_t len, btmg_gatt_db_t *db);
btmg_err btmg_gatts_notify(uint8_t conn_id, uint16_t char_handle, uint8_t *buf, size_t len);

/**
 * @praram conn_id: connections identity
 * @praram handle: character handle
 * @praram buf: buffer
 * @praram len: buffer len
 * @return 0 in case of success or negative value in case of error.
 */
// btmg_err btmg_gatts_indicate(uint8_t conn_id, uint16_t handle, void *buf, size_t len, btmg_gatt_db_t *db);
btmg_err btmg_gatts_indicate(uint8_t conn_id, uint16_t char_handle, uint8_t *data, uint16_t len);

/**
 * @praram conn_id: connections identity
 * @praram handle: character handle
 * @praram buf: buffer
 * @praram len: buffer len
 * @praram offset: read offset
 * @return 0 in case of success or negative value in case of error.
 */
btmg_err btmg_gatts_read_rsp(uint8_t conn_id, uint16_t handle, void *buf, size_t len, size_t offset);

/**
 * @praram conn_id: connections identity
 * @praram handle: character handle
 * @praram buf: buffer
 * @praram len: buffer len
 * @praram offset: read offset
 * @return 0 in case of success or negative value in case of error.
 */
btmg_err btmg_gatts_write_rsp(uint8_t conn_id, uint16_t handle, void *buf, size_t len, size_t offset);

/**
 * @praram conn_id: connections identity
 * @praram uuid: service uuid
 * @praram start_handle: start handle
 * @praram end_handle: end handle
 * @return 0 in case of success or negative value in case of error.
 */
btmg_err btmg_gattc_discover_all_services(uint8_t conn_id, btmg_uuid_t *uuid, uint16_t start_handle,
                                     uint16_t end_handle);

/**
 * @praram conn_id: connections identity
 * @praram uuid: service uuid
 * @praram start_handle: start handle
 * @praram end_handle: end handle
 * @return 0 in case of success or negative value in case of error.
 */
btmg_err btmg_gattc_discover_primary_services(uint8_t conn_id, btmg_uuid_t *uuid, uint16_t start_handle,
                                         uint16_t end_handle);

/**
 * @praram conn_id: connections identity
 * @return 0 in case of success or negative value in case of error.
 */
btmg_err btmg_gattc_discover_characteristic(uint8_t conn_id);

/**
 * @praram conn_id: connections identity
 * @return 0 in case of success or negative value in case of error.
 */
btmg_err btmg_gattc_discover_descriptor(uint8_t conn_id);

/**
 * @praram conn_id: connections identity
 * @praram char_handle: char handle
 * @return 0 in case of success or negative value in case of error.
 */
btmg_err btmg_gattc_read(uint8_t conn_id, uint16_t char_handle);

/**
 * @praram conn_id: connections identity
 * @praram char_handle: char handle
 * @praram offset: offset
 * @return 0 in case of success or negative value in case of error.
 */
btmg_err btmg_gattc_read_long(uint8_t conn_id, uint16_t char_handle, int offset);

/**
 * @praram conn_id: connections identity
 * @praram char_handle: char handle
 * @praram value: value
 * @praram len: len
 * @return 0 in case of success or negative value in case of error.
 */
btmg_err btmg_gattc_write(uint8_t conn_id, int char_handle, uint8_t *value, size_t len);

/**
 * @praram conn_id: connections identity
 * @praram reliable_writes: reliable_writes or not
 * @praram char_handle: char handle
 * @praram value: value
 * @praram len: len
 * @return 0 in case of success or negative value in case of error.
 */
btmg_err btmg_gattc_write_long(uint8_t conn_id, bool reliable_writes, int char_handle, uint8_t *value,
                          uint16_t len, uint16_t offset);

/**
 * @praram conn_id: connections identity
 * @praram signed_write: signed_write or not
 * @praram char_handle: char handle
 * @praram value: value
 * @praram len: len
 * @return 0 in case of success or negative value in case of error.
 */
btmg_err btmg_gattc_write_without_response(uint8_t conn_id, bool signed_write, uint16_t char_handle,
                                      uint8_t *value, uint16_t len);

/**
 * @praram conn_id: connections identity
 * @praram signed_write: signed_write or not
 * @praram char_handle: char handle
 * @praram value: value
 * @praram len: len
 * @return 0 in case of success or negative value in case of error.
 */
btmg_err btmg_gattc_write_long_without_response(uint8_t conn_id, bool signed_write, int char_handle,
                                           uint8_t *value, uint16_t len);

/**
 * @praram conn_id: connections identity
 * @praram char_handle: char handle
 * @return 0 in case of success or negative value in case of error.
 */
btmg_err btmg_gattc_subscribe(uint8_t conn_id, uint16_t value_handle);

/**
 * @praram conn_id: connections identity
 * @praram char_handle: char handle
 * @return 0 in case of success or negative value in case of error.
 */
btmg_err btmg_gattc_unsubscribe(uint8_t conn_id, int char_handle);

#define GATT_MAX_ATTR_LEN 600

typedef enum {
    LE_CONNECTED = 0,
    LE_CONNECT_FAIL,
    LE_DISCONNECTED,
    LE_DISCONNECT_FAIL,
} le_connection_state_t;

typedef struct {
    uint8_t conn_id;
    le_connection_state_t status;
    btmg_le_addr_t addr;
    uint8_t role; // 0 gattclient 1 gattserver
    int reason;   // gattc_disconnect_reason_t
    // uint8_t att_ecode; //
    // void *user_data; //
} le_connection_para_t;

typedef struct {
    uint8_t conn_id;
    unsigned int trans_id;
    int attr_handle;
    int offset;
    bool is_blob_req;
    uint8_t out_data[GATT_MAX_ATTR_LEN];
    int out_len;
} gatts_char_read_req_t;

typedef struct {
    uint8_t conn_id;
    unsigned int trans_id;
    int attr_handle;
    int offset;
    char *value;
    int value_len;
    bool need_rsp;
} gatts_char_write_req_t;

typedef struct {
    uint8_t conn_id;
    int attr_handle;
    uint8_t value;
} gatts_ccc_cfg_t;

typedef struct {
    int attr_handle;
    btmg_uuid_t uuid;
    btmg_uuid_t uuid_value;
    btmg_gatt_permission_t perm;
} gatts_get_db_t;

typedef struct {
    uint8_t conn_id;
    bool success;
} gatts_indicate_cb_t;

typedef void (*bt_gatts_connection_cb)(le_connection_para_t *data);
typedef void (*bt_gatts_char_read_req_cb)(gatts_char_read_req_t *data);
typedef void (*bt_gatts_char_write_req_cb)(gatts_char_write_req_t *data);
typedef void (*bt_gatts_ccc_cfg_cb)(gatts_ccc_cfg_t *data);
typedef void (*bt_gatts_get_db_cb)(gatts_get_db_t *att);
typedef void (*bt_gatts_indicate_cb)(gatts_indicate_cb_t *data);

typedef struct {
    /*gatt connection event callback*/
    bt_gatts_connection_cb conn_cb;       //ok
    /*gatt characteristic request callback*/
    bt_gatts_char_read_req_cb char_read_req_cb;
    bt_gatts_char_write_req_cb char_write_req_cb;
    bt_gatts_ccc_cfg_cb ccc_cfg_cb;
    bt_gatts_get_db_cb get_db_cb;
    bt_gatts_indicate_cb indicate_cb;
} btmg_gatts_cb_t;

typedef struct {
    btmg_le_addr_t addr; //addr
    int adv_type;     // 0 adv 1 scan_rsp
    int rssi;            // db
    uint8_t data[31];    // adv data
    uint8_t name[31];    // name in adv
    int data_len;        //
    int can_connect;     //
    //add other
} le_scan_cb_para_t;

typedef struct {
    uint8_t conn_id;
    uint8_t interval;
    uint8_t latency;
    uint8_t timeout;
} gatt_le_param_update_cb_para_t;

typedef struct {
    uint8_t conn_id;
    uint8_t level;
    int err;
} gatt_security_changed_cb_para_t;

typedef struct {
    uint8_t conn_id;
    // int mtu;
    int err;
} gatt_exchange_mtu_cb_para_t;

typedef struct {
    bool success;
   /** Attribute handle */
	uint16_t handle;
   /** error code */
    uint8_t att_ecode;
    void *user_data;
} gattc_write_cb_para_t;

typedef struct {
    bool success;
   /** Attribute handle */
	uint16_t handle;
    bool reliable_error;
    /** error code */
    uint8_t att_ecode;
    void *user_data;
} gattc_write_long_cb_para_t;

typedef struct {
    bool success;
    /** Attribute handle */
	uint16_t handle;
    /** error code */
    uint8_t att_ecode;
    const uint8_t *value;
    uint16_t length;
    void *user_data;
} gattc_read_cb_para_t;

typedef enum {
    BTMG_BLE_STATUS_CODE_SUCCESS = 0,
    BTMG_BLE_AUTHENTICATION_FAILURE = 0x05,
    BTMG_BLE_CONNECTION_TIMEOUT = 0x08,
    BTMG_BLE_REMOTE_USER_TERMINATED = 0x13,
    BTMG_BLE_LOCAL_HOST_TERMINATED = 0x16,
    BTMG_BLE_LMP_RESPONSE_TIMEOUT = 0x22,
    BTMG_BLE_FAILED_TO_BE_ESTABLISHED = 0x3E,
    BTMG_BLE_UNKNOWN_OTHER_ERROR = 0xFF,
} gattc_disconnect_reason_t;

typedef struct {
    uint16_t start_handle;
    uint16_t end_handle;
    void *user_data;
} gattc_service_changed_cb_para_t;

typedef enum {
    BTMG_DIS_PRIMARY_SERVER,
    BTMG_DIS_SECONDARY_SERVER,
    BTMG_DIS_INCLUDE_SERVER,
    BTMG_DIS_CHARACTERISTIC,
    BTMG_DIS_ATTRIBUTE,
} btmg_discover_type;

typedef struct {
    btmg_discover_type type;
    uint8_t conn_id;
    uint16_t start_handle; //server start_handle
    uint16_t end_handle;   //server end_handle
    uint16_t value_handle; //character value_handle
    uint16_t char_handle;  //char desc handle
    uint16_t attr_handle;  //attr handle
    uint8_t properties;
    uint16_t ext_prop;
    btmg_uuid_t uuid;        //desc uuid
    btmg_uuid_t server_uuid; //server uuid
    btmg_uuid_t char_uuid;   //char uuid
    // void *attr;
} gattc_dis_cb_para_t;

typedef struct {
    uint16_t value_handle;
    const uint8_t *value;
    uint16_t length;
    void *user_data;
} gattc_notify_indicate_cb_para_t;

typedef void (*bt_gattc_scan_cb)(le_scan_cb_para_t *data);
typedef void (*bt_gattc_connection_cb)(le_connection_para_t *data);
typedef void (*bt_gattc_le_param_update_cb)(gatt_le_param_update_cb_para_t *data);
typedef void (*bt_gattc_security_changed_cb)(gatt_security_changed_cb_para_t *data);
typedef void (*bt_gattc_exchange_mtu_cb)(gatt_exchange_mtu_cb_para_t *data);
typedef void (*bt_gattc_write_cb)(gattc_write_cb_para_t *data);
typedef void (*bt_gattc_read_cb)(gattc_read_cb_para_t *data);
typedef void (*bt_gattc_dis_att_cb)(gattc_dis_cb_para_t *data);
typedef void (*bt_gattc_notify_indicate_cb)(gattc_notify_indicate_cb_para_t *data);

typedef void (*bt_gattc_write_long_cb)(gattc_write_long_cb_para_t *data);
typedef void (*bt_gattc_service_changed_cb)(gattc_service_changed_cb_para_t *data);
// typedef void (*bt_gattc_connected_list_cb)(gattc_connected_list_cb_para_t *data);
typedef struct {
    bt_gattc_scan_cb le_scan_cb;
    bt_gattc_connection_cb conn_cb;                   //ok
    bt_gattc_le_param_update_cb le_param_update_cb;   //ok
    bt_gattc_security_changed_cb security_changed_cb; //ok
    bt_gattc_exchange_mtu_cb exchange_mtu_cb;         //ok
    bt_gattc_write_cb write_cb;                       //ok
    bt_gattc_read_cb read_cb;                         //ok
    bt_gattc_dis_att_cb dis_att_cb;
    bt_gattc_notify_indicate_cb notify_indicate_cb; //ok

    bt_gattc_write_long_cb write_long_cb;           //not support
    bt_gattc_service_changed_cb service_changed_cb; //not support
   // bt_gattc_connected_list_cb connected_list_cb; //todo
} btmg_gattc_cb_t;

typedef struct {
    uint8_t conn_id;
    btmg_le_addr_t addr;
    char *passkey;
} le_smp_passkey_display_para_t;

typedef struct {
    uint8_t conn_id;
    btmg_le_addr_t addr;
    char *passkey;
} le_smp_passkey_confirm_para_t;

typedef struct {
    uint8_t conn_id;
    btmg_le_addr_t addr;
} le_smp_passkey_enter_para_t;

typedef struct {
    uint8_t conn_id;
    btmg_le_addr_t addr;
} le_smp_pairing_confirm_para_t;

typedef struct {
    uint8_t conn_id;
    btmg_le_addr_t addr;
    int bonded;
    int err;
} le_smp_pairing_complete_para_t;

typedef struct {
    uint8_t conn_id;
    btmg_le_addr_t addr;
} le_smp_cancel_para_t;

typedef void (*bt_le_smp_passkey_display_cb)(le_smp_passkey_display_para_t *data);
typedef void (*bt_le_smp_passkey_confirm_cb)(le_smp_passkey_confirm_para_t *data);
typedef void (*bt_le_smp_passkey_enter_cb)(le_smp_passkey_enter_para_t *data);
typedef void (*bt_le_smp_pairing_confirm_cb)(le_smp_pairing_confirm_para_t *data);
typedef void (*bt_le_smp_pairing_complete_cb)(le_smp_pairing_complete_para_t *data);
typedef void (*bt_le_smp_pairing_failed_cb)(le_smp_pairing_complete_para_t *data);
typedef void (*bt_le_smp_cancel_cb)(le_smp_cancel_para_t *data);

typedef struct {
    bt_le_smp_passkey_display_cb le_smp_passkey_display_cb;
    bt_le_smp_passkey_confirm_cb le_smp_passkey_confirm_cb;
    bt_le_smp_passkey_enter_cb le_smp_passkey_enter_cb;
    bt_le_smp_pairing_confirm_cb le_smp_pairing_confirm_cb;
    bt_le_smp_pairing_complete_cb le_smp_pairing_complete_cb;
    bt_le_smp_pairing_failed_cb le_smp_pairing_failed_cb;
    bt_le_smp_cancel_cb le_smp_cancel_cb;
} btmg_le_smp_cb_t;

#endif /* CONFIG_BLEHOST */

typedef struct {
    btmg_adapter_callback_t btmg_adapter_cb;
    btmg_device_callback_t btmg_device_cb;
    btmg_a2dp_sink_callback_t btmg_a2dp_sink_cb;
    btmg_a2dp_source_callback_t btmg_a2dp_source_cb;
    btmg_hfp_hf_callback_t btmg_hfp_hf_cb;
    btmg_hfp_ag_callback_t btmg_hfp_ag_cb;
    btmg_avrcp_callback_t btmg_avrcp_cb;
    btmg_spps_callback_t btmg_spps_cb;
    btmg_sppc_callback_t btmg_sppc_cb;
#ifdef CONFIG_BLEHOST
    btmg_gatts_cb_t btmg_gatts_cb;
    btmg_gattc_cb_t btmg_gattc_cb;
    btmg_le_smp_cb_t btmg_le_smp_cb;
#endif /* CONFIG_BLEHOST */
} btmg_callback_t;

/*******************************************************************************
**
** Function         btmg_register_callback
**
** Description      register callback
**
** Parameter        cb  - callback.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_register_callback(btmg_callback_t *cb);

/*******************************************************************************
**
** Function         btmg_unregister_callback
**
** Description      unregister function
**
** Parameter        void
**
** Returns          void
**
*******************************************************************************/
void btmg_unregister_callback(void);

/*******************************************************************************
**
** Function         btmg_audiosystem_register_cb
**
** Description      Provide a registration callback to audiosystem
**
** Parameter        cb  - callback.
**
** Returns          BT_OK: successful.
**                  BT_FAIL: fail.
**                  OTHER: reference error code.
**
*******************************************************************************/
btmg_err btmg_audiosystem_register_cb(btmg_callback_t *cb);

/*******************************************************************************
**
** Function         btmg_audiosystem_unregister_cb
**
** Description      unregister function
**
** Parameter        void
**
** Returns          void
**
*******************************************************************************/
void btmg_audiosystem_unregister_cb(void);

#if __cplusplus
}; // extern "C"
#endif

#endif
