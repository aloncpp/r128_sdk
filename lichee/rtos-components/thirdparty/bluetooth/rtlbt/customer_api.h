/*
 *  Copyright (C) 2019 Realtek Semiconductor Corporation.
 *
 *  Author: Alex Lu <alex_lu@realsil.com.cn>
 */

#ifndef __CUSTOMER_H__
#define __CUSTOMER_H__

#include <stdint.h>
#include <stdbool.h>
#if __cplusplus
extern "C" {
#endif
#define CSM_BT_OK (0)
#define CSM_BT_FAIL (-1) /* abnormal return must < 0 */
#define CSM_BDADDR_MAX_LEN (18)
#define CSM_NAME_MAX_LEN (96)

    typedef struct _CSM_BT_DEV_INFO
    {
        char bdAddr[CSM_BDADDR_MAX_LEN];/* Bluetooth Address */
        char name[CSM_NAME_MAX_LEN];/* Name of device */
    } CSM_BT_DEV_INFO;

    typedef enum
    {
        CSM_UNPAIR_FAIL,
        CSM_UNPAIR_SUCCESS
    } CSM_BT_UNPAIR_RESULT;

    typedef enum
    {
        CSM_PAIR_FAIL,
        CSM_PAIR_SUCCESS,
        CSM_PAIR_TIMEOUT
    } CSM_BT_PAIR_RESULT;

#define CSM_MAX_BT_DEV_NUM 10
    typedef enum
    {
        CSM_A2DP_CONNECTED,
        CSM_A2DP_DISCONNECTED,
        CSM_A2DP_CONNECTING,
        CSM_A2DP_LINK_LOSS,
        CSM_A2DP_CONNECT_TIMEOUT,
        CSM_A2DP_EVENT_MAX
    } CSM_A2DP_STATE;

    typedef enum
    {
        CSM_A2DP_ROLE_SINK,
        CSM_A2DP_ROLE_SRC,
        CSM_A2DP_ROLE_MAX
    } CSM_A2DP_ROLE;

    typedef enum
    {
        CSM_A2DP_STREAM_STATE_STOP,
        CSM_A2DP_STREAM_STATE_PLAYING,
        CSM_A2DP_STREAM_STATE_MAX
    } CSM_A2DP_STREAM_STATE;

    typedef struct
    {
        unsigned int head_idx;
        unsigned int dev_num;
        CSM_BT_DEV_INFO device_list[CSM_MAX_BT_DEV_NUM];
    } CSM_A2DP_DEV_INFO_LIST;

    typedef enum
    {
        CSM_AVRCP_DISCONNECTED,
        CSM_AVRCP_CONNECTED
    } CSM_AVRCP_STATUS;

    typedef enum
    {
        CSM_AVRCP_PLAY_STATUS_STOPPED,
        CSM_AVRCP_PLAY_STATUS_PLAYING,
        CSM_AVRCP_PLAY_STATUS_PAUSEED,
        CSM_AVRCP_PLAY_STATUS_MAX
    } CSM_AVRCP_PLAY_STATUS;

    typedef enum
    {
        CSM_AVRCP_VOLUME_UP,
        CSM_AVRCP_VOLUME_DOWN
    } CSM_AVRCP_VOLUME_CHANGE;

    typedef enum
    {
        CSM_AVRCP_CMD_TYPE_PLAY = 0,
        CSM_AVRCP_CMD_TYPE_PAUSE,
        CSM_AVRCP_CMD_TYPE_FWD,
        CSM_AVRCP_CMD_TYPE_BWD,
        CSM_AVRCP_CMD_TYPE_FFWD,
        CSM_AVRCP_CMD_TYPE_RWD,
        CSM_AVRCP_CMD_TYPE_STOP,
        CSM_AVRCP_CMD_TYPE_VOL_UP,
        CSM_AVRCP_CMD_TYPE_VOL_DOWN,
        CSM_AVRCP_CMD_TYPE_MAX
    } CSM_AVRCP_CMD_TYPE;

//public api for mac str & char format switch
    bool mac_str_to_bin(char *str, uint8_t *mac);
    bool mac_bin_to_str(char *str, uint8_t *mac);


    /* GAP */
    /*
    void CSM_gapInquiryRespCbk(CSM_BT_DEV_INFO *dev_info);
    void CSM_gapPairResultCbk(CSM_BT_PAIR_RESULT result, char *remote_mac);
    void CSM_gapUnpairResultCbk(CSM_BT_UNPAIR_RESULT result, char *remote_mac);
    */
    void csm_inquiry_resp_cb(const char *name, uint8_t *address);
    void csm_paired_result_cb(uint8_t result, uint8_t *address);
    void csm_unpaired_result_cb(int result, uint8_t *addr);

    int CSM_gapSetOnOff(bool fg_on);
    void CSM_gapGetLocalDevInfo(CSM_BT_DEV_INFO *local_dev_info);
    void CSM_gapGetRemoteDevInfo(CSM_BT_DEV_INFO *remote_dev_info);
    void CSM_gapSetScanMode(bool conn_flag, bool disc_flag);
    int CSM_gapSetName(const char *set_name);
    void CSM_startInquirySink(void);
    void CSM_stopInquirySink(void);
    void CSM_gapUnPair(const char *mac);

    /* A2DP */
    /*
    void CSM_a2dpStateChangedCbk(CSM_A2DP_STATE state, char *mac);
    void CSM_a2dpStreamStateCbk(CSM_A2DP_STREAM_STATE streamState);
    */
    void csm_a2dp_state_cb(CSM_A2DP_STATE state, uint8_t *address);
    void csm_a2dp_stream_cb(CSM_A2DP_STREAM_STATE state);

    void CSM_a2dpStartPlayer(void);
    void CSM_a2dpStopPlayer(void);
    void CSM_a2dpConnect(const char *mac, CSM_A2DP_ROLE role);
    void CSM_a2dpDisconnect(const char *mac, CSM_A2DP_ROLE role);
    int CSM_a2dpSetRole(CSM_A2DP_ROLE role);
    CSM_A2DP_ROLE CSM_a2dpGetRole(void);
    void CSM_getA2dpPairedList(CSM_A2DP_DEV_INFO_LIST *info, CSM_A2DP_ROLE role);

    /* AVRCP */
    /*
    void CSM_avrcpStateChangedCbk(CSM_AVRCP_STATUS state);
    void CSM_avrcpPlayStateChangeCbk(CSM_AVRCP_PLAY_STATUS state);
    void CSM_avrcpVolumeChangeCbk(CSM_AVRCP_VOLUME_CHANGE direction);
    void CSM_avrcpAbsoluteVolumeCbk(unsigned int vol);
    void CSM_avrcpCmdCbkForSrc(CSM_AVRCP_CMD_TYPE avrcp_cmd);
    */
    void csm_avrcp_state_cb(CSM_AVRCP_STATUS state);
    void csm_avrcp_play_state_cb(CSM_AVRCP_PLAY_STATUS state);
    void csm_avrcp_volume_chg_cb(CSM_AVRCP_VOLUME_CHANGE dir);
    void csm_avrcp_abs_volume_cb(unsigned int vol);
    void csm_avrcp_passthr_cb(CSM_AVRCP_CMD_TYPE cmd);

    void csm_avrcp_connect(const char *address);
    void csm_avrcp_disconnect(void);
    void CSM_avrcpSendPassthroughCmd(CSM_AVRCP_CMD_TYPE cmd_type);
    int CSM_avrcpChangeVolume(unsigned char ui1_vol, CSM_A2DP_ROLE role);

    typedef void(* rtkGapPairCB)(uint8_t result, char *address);
    typedef void(* rtkGapUnpairCB)(int result, char *addr);
    typedef void(* rtkGapScanCB)(const char *name, const char *address);

    typedef void(* rtkA2dpStateCB)(uint8_t state, char *address);
    typedef void(* rtkA2dpStreamCB)(uint8_t state);

    typedef void(* rtkAvrcpStateCB)(uint8_t state);
    typedef void(* rtkAvrcpPlayStateCB)(uint8_t state);
    typedef void(* rtkVolumeChangeCB)(uint8_t direction);
    typedef void(* rtkAbsoluteVolumeCB)(unsigned int vol);
    typedef void(* rtkAvrcpCmdSrcCB)(uint8_t avrcp_cmd);

    typedef struct
    {
        rtkGapPairCB gapPairCB;
        rtkGapUnpairCB gapUnpairCB;
        rtkGapScanCB gapGapScanCB;
    } RTK_GAP_IMPL_CB_FUNC_T;

    typedef struct
    {
        rtkA2dpStateCB a2dpStateCB;
        rtkA2dpStreamCB a2dpStreamCB;
    } RTK_A2DP_IMPL_CB_FUNC_T;

    typedef struct
    {
        rtkAvrcpStateCB avrcpStateCB;
        rtkAvrcpPlayStateCB avrcpPlayStateCB;
        rtkVolumeChangeCB volumeChangeCB;
        rtkAbsoluteVolumeCB absoluteVolumeCB;
        rtkAvrcpCmdSrcCB avrcpCmdSrcCB;
    } RTK_AVRCP_IMPL_CB_FUNC_T;

    void GapCallbackRegister(RTK_GAP_IMPL_CB_FUNC_T *func);
    void A2dpCallbackRegister(RTK_A2DP_IMPL_CB_FUNC_T *func);
    void AvrcpCallbackRegister(RTK_AVRCP_IMPL_CB_FUNC_T *func);


//////////////////////////////////
//LE GATT Server
//////////////////////////////////
#define AG_GATT_MAX_ATTR_LEN 600
#define AG_GATT_MAX_UUID_LEN 37
#define AG_MAX_BDADDR_LEN  18

#define UINT8     uint8_t
#define UINT16     uint16_t
#define UINT32     uint32_t
#define INT8     int8_t
#define INT16     int16_t
#define INT32     int32_t
#define CHAR     char

    typedef enum
    {
        CSM_AG_GATTS_REGISTER_SERVER = 0,
        CSM_AG_GATTS_CONNECT,
        CSM_AG_GATTS_DISCONNECT,
        CSM_AG_GATTS_GET_RSSI_DONE,

        CSM_AG_GATTS_EVENT_MAX
    } CSM_AG_GATTS_EVENT_T;

    /** GATT ID adding instance id tracking to the UUID */
    typedef struct
    {
        CHAR  uuid[AG_GATT_MAX_UUID_LEN];
        UINT8 inst_id;
    } CSM_AG_GATT_ID_T;

    /** GATT Service ID also identifies the service type (primary/secondary) */
    typedef struct
    {
        CSM_AG_GATT_ID_T    id;
        UINT8 is_primary;
    } CSM_AG_GATTS_SRVC_ID_T;
    typedef struct
    {
        INT32 server_if;
        CSM_AG_GATTS_SRVC_ID_T srvc_id;
        INT32 srvc_handle;
    } CSM_AG_GATTS_ADD_SRVC_RST_T;

    typedef struct
    {
        INT32 server_if;
        CHAR uuid[AG_GATT_MAX_UUID_LEN];
        INT32 srvc_handle;
        INT32 char_handle;
    } CSM_AG_GATTS_ADD_CHAR_RST_T ;

    typedef struct
    {
        INT32 server_if;
        CHAR uuid[AG_GATT_MAX_UUID_LEN];
        INT32 srvc_handle;
        INT32 descr_handle;
    } CSM_AG_GATTS_ADD_DESCR_RST_T;

    typedef struct
    {
        INT32 conn_id;
        INT32 trans_id;
        CHAR btaddr[AG_MAX_BDADDR_LEN];
        INT32 attr_handle;
        INT32 offset;
        INT32 length;
        UINT8 need_rsp;
        UINT8 is_prep;
        UINT8 value[AG_GATT_MAX_ATTR_LEN];
    } CSM_AG_GATTS_REQ_WRITE_RST_T;

    typedef struct
    {
        INT32 conn_id;
        INT32 trans_id;
        CHAR btaddr[AG_MAX_BDADDR_LEN];
        INT32 attr_handle;
        INT32 offset;
        UINT8 is_long;
    } CSM_AG_GATTS_REQ_READ_RST_T;

    typedef void (*fp_onGattsInitCallback)(INT32 serverIf);
    typedef void (*fp_onGattsAddServiceCallback)(CSM_AG_GATTS_ADD_SRVC_RST_T *bt_gatts_add_srvc);
    typedef void (*fp_onGattsAddCharCallback)(CSM_AG_GATTS_ADD_CHAR_RST_T *bt_gatts_add_char);
    typedef void (*fp_onGattsAddDescCallback)(CSM_AG_GATTS_ADD_DESCR_RST_T *bt_gatts_add_desc);
    typedef void (*fp_onGattsReqWrite)(CSM_AG_GATTS_REQ_WRITE_RST_T *bt_gatts_req_write);
    typedef void (*fp_onGattsReqRead)(CSM_AG_GATTS_REQ_READ_RST_T *bt_gatts_req_read);
    typedef void (*fp_onGattsConnectionEventCallback)(CSM_AG_GATTS_EVENT_T bt_gatts_connection_evt);
    typedef void (*fp_onGattsStartServerCallback)();
    typedef void (*fp_onGattsStopServerCallback)();
    typedef void (*fp_onGattsDeleteServerCallback)();

    typedef struct
    {
        fp_onGattsInitCallback onGattsInitCallback;
        fp_onGattsAddServiceCallback onGattsAddServiceCallback;
        fp_onGattsAddCharCallback onGattsAddCharCallback;
        fp_onGattsAddDescCallback onGattsAddDescCallback;
        fp_onGattsReqWrite onGattsReqWrite;
        fp_onGattsReqRead onGattsReqRead;
        fp_onGattsConnectionEventCallback onGattsConnectionEventCallback;
        fp_onGattsStartServerCallback onGattsStartServerCallback;
        fp_onGattsStopServerCallback onGattsStopServerCallback;
        fp_onGattsDeleteServerCallback onGattsDeleteServerCallback;
    } RTK_BT_APP_GATTS_CB_FUNC_T;

// start a thread
    INT32 CSM_init(void) ;
// quit the thread
    INT32 CSM_deinitGatts(void) ;
// setcb
    void CSM_setCallback(RTK_BT_APP_GATTS_CB_FUNC_T *callback);
// getcb
    RTK_BT_APP_GATTS_CB_FUNC_T *CSM_getCallback(void);
    INT32 CSM_addService(INT32 server_if, CHAR *service_uuid, UINT8 is_primary, INT32 number);
    INT32 CSM_addChar(INT32 server_if, INT32 service_handle, CHAR *uuid, INT32 properties,
                      INT32 permissions);
    INT32 CSM_addDesc(INT32 server_if, INT32 service_handle, CHAR *uuid, INT32 permissions);
    INT32 CSM_startService(INT32 server_if, INT32 service_handle, INT32 transport);
    INT32 CSM_stopService(INT32 server_if, INT32 service_handle);
    INT32 CSM_deleteService(INT32 server_if, INT32 service_handle);
    INT32 CSM_unregisterService(INT32 server_if);
    INT32 CSM_sendResponse(INT32 conn_id, INT32 trans_id, INT32 status, INT32 handle, CHAR *p_value,
                           INT32 value_len, INT32 auth_req);
    INT32 CSM_sendIndication(INT32 server_if, INT32 handle, INT32 conn_id, INT32 fg_confirm,
                             CHAR *p_value, INT32 value_len);
    void CSM_setServerIf(INT32 serverIf);
    void CSM_delServerIf(INT32 serverIf);
    INT32 CSM_enableAdv(bool enable);

//callbacks
    void csm_gatts_init_cb(INT32 server_if);
    void csm_gatts_add_service_cb(CSM_AG_GATTS_ADD_SRVC_RST_T *bt_gatts_add_srvc);
    void csm_gatts_add_char_cb(CSM_AG_GATTS_ADD_CHAR_RST_T *bt_gatts_add_char);
    void csm_gatts_add_desc_cb(CSM_AG_GATTS_ADD_DESCR_RST_T *bt_gatts_add_desc);
    void csm_gatts_reg_write_cb(CSM_AG_GATTS_REQ_WRITE_RST_T *bt_gatts_req_write);
    void csm_gatts_reg_read_cb(CSM_AG_GATTS_REQ_READ_RST_T *bt_gatts_req_read);
    void csm_gatt_conn_evt_cb(CSM_AG_GATTS_EVENT_T bt_gatts_connection_evt);
    void csm_gatts_start_server_cb(void);
    void csm_gatts_stop_server_cb(void);
    void csm_gatts_del_server_cb(void);


#if __cplusplus
};
#endif
#endif
