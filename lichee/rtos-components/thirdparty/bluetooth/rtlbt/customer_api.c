/*
 *  Copyright (C) 2019 Realtek Semiconductor Corporation.
 *
 *  Author: Alex Lu <alex_lu@realsil.com.cn>
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "customer_api.h"
#include "blue_cmd.h"
#include "blue_cmd_legacy.h"
#include "app.h"
#define DEBUG
#include "app_common.h"
#include "trace_app.h"

static CSM_A2DP_ROLE csm_a2dp_role;

RTK_GAP_IMPL_CB_FUNC_T rtkGapImplCB;
RTK_A2DP_IMPL_CB_FUNC_T rtkA2dpImplCB;
RTK_AVRCP_IMPL_CB_FUNC_T rtkAvrcpImplCB;
RTK_BT_APP_GATTS_CB_FUNC_T rtkGattServerImplCB;

void GapCallbackRegister(RTK_GAP_IMPL_CB_FUNC_T *func)
{
    pr_info("++");
    rtkGapImplCB.gapPairCB = func->gapPairCB;
    rtkGapImplCB.gapUnpairCB = func->gapUnpairCB;
    rtkGapImplCB.gapGapScanCB = func->gapGapScanCB;
}
void A2dpCallbackRegister(RTK_A2DP_IMPL_CB_FUNC_T *func)
{
    pr_info("++");
    rtkA2dpImplCB.a2dpStateCB = func->a2dpStateCB;
    rtkA2dpImplCB.a2dpStreamCB = func->a2dpStreamCB;
}
void AvrcpCallbackRegister(RTK_AVRCP_IMPL_CB_FUNC_T *func)
{
    pr_info("++");
    rtkAvrcpImplCB.avrcpStateCB = func->avrcpStateCB;
    rtkAvrcpImplCB.avrcpPlayStateCB = func->avrcpPlayStateCB;
    rtkAvrcpImplCB.volumeChangeCB = func->volumeChangeCB;
    rtkAvrcpImplCB.absoluteVolumeCB = func->absoluteVolumeCB;
    rtkAvrcpImplCB.avrcpCmdSrcCB = func->avrcpCmdSrcCB;
}

void GattServerCallbackRegister(RTK_BT_APP_GATTS_CB_FUNC_T *func)
{
    pr_info("++");
    rtkGattServerImplCB.onGattsInitCallback = func->onGattsInitCallback;
    rtkGattServerImplCB.onGattsAddServiceCallback = func->onGattsAddServiceCallback;
    rtkGattServerImplCB.onGattsAddCharCallback = func->onGattsAddCharCallback;
    rtkGattServerImplCB.onGattsAddDescCallback = func->onGattsAddDescCallback;
    rtkGattServerImplCB.onGattsReqWrite = func->onGattsReqWrite;
    rtkGattServerImplCB.onGattsReqRead = func->onGattsReqRead;
    rtkGattServerImplCB.onGattsConnectionEventCallback = func->onGattsConnectionEventCallback;
    rtkGattServerImplCB.onGattsStartServerCallback = func->onGattsStartServerCallback;
    rtkGattServerImplCB.onGattsStopServerCallback = func->onGattsStopServerCallback;
    rtkGattServerImplCB.onGattsDeleteServerCallback = func->onGattsDeleteServerCallback;
}

int CSM_gapSetOnOff(bool fg_on)
{
    void *argv[16];
    int result;

    argv[0] = (void *)(UINT_TO_PTR(fg_on));
    result = bta_submit_command_wait(BCMD_SET_POWER, 1, (void **)argv);
    if (result < 0)
    {
        pr_err("Run %04x command failed, %d",
               BCMD_SET_POWER, result);
        return (int)false;
    }

    return (int)true;
}

void CSM_gapGetLocalDevInfo(CSM_BT_DEV_INFO *local_dev_info)
{
    int result;
    void *argv[1];

    argv[0] = local_dev_info;
    result = bta_submit_command_wait(BCMD_READ_LOCAL_INFO, 1, argv);
    if (result < 0)
    {
        pr_err("Run %04x command failed, %d",
               BCMD_READ_LOCAL_INFO, result);
        return;
    }

    pr_info("name: %s", local_dev_info->name);
    pr_info("address: %s", local_dev_info->bdAddr);
}

void CSM_gapGetRemoteDevInfo(CSM_BT_DEV_INFO *remote_dev_info)
{
    int result;
    void *argv[1];

    argv[0] = remote_dev_info;
    memset(remote_dev_info, 0, sizeof(*remote_dev_info));
    result = bta_submit_command_wait(BCMD_READ_REMOTE_INFO, 1, argv);
    if (result < 0)
    {
        pr_err("Run %04x command failed, %d",
               BCMD_READ_REMOTE_INFO, result);
        return;
    }
    if (remote_dev_info->name[0] && remote_dev_info->bdAddr[0])
    {
        pr_info("name: %s", remote_dev_info->name);
        pr_info("address: %s", remote_dev_info->bdAddr);
    }
    else
    {
        pr_info("No remote device info. no device connected?");
    }
}

void CSM_gapSetScanMode(bool conn_flag, bool disc_flag)
{
    int result;
    void *argv[2];

    argv[0] = (void *)(UINT_TO_PTR(conn_flag));
    argv[1] = (void *)(UINT_TO_PTR(disc_flag));
    result = bta_submit_command_wait(BCMD_SET_SCAN_MODE, 2, argv);
    if (result < 0)
    {
        pr_err("Run %04x command failed, %d",
               BCMD_SET_SCAN_MODE, result);
        return;
    }

}

int CSM_gapSetName(const char *set_name)
{
    int result;
    void *argv[1];

    argv[0] = (void *)set_name;
    result = bta_submit_command_wait(BCMD_SET_LOCAL_NAME, 1, argv);
    if (result < 0)
    {
        pr_err("Run %04x command failed, %d",
               BCMD_SET_LOCAL_NAME, result);
        return -1;

    }
    return 0;
}

void CSM_startInquirySink()
{
    int result;

    result = bta_submit_command_wait(BCMD_START_DISCOVERY, 0, NULL);
    if (result < 0)
        pr_err("Run %04x command failed, %d",
               BCMD_START_DISCOVERY, result);
}

void CSM_stopInquirySink()
{
    int result;

    result = bta_submit_command_wait(BCMD_STOP_DISCOVERY, 0, NULL);
    if (result < 0)
        pr_err("Run %04x command failed, %d",
               BCMD_STOP_DISCOVERY, result);
}

void CSM_gapUnPair(const char *mac)
{
    int result;
    void *argv[1];

    APP_PRINT_INFO1("CSM_gapUnPair: address is %s", TRACE_STRING(mac));

    argv[0] = (char *)mac;
    result = bta_submit_command_wait(BCMD_UNPAIR_DEVICE, 1, argv);
    if (result < 0)
        pr_err("Run %04x command failed, %d",
               BCMD_UNPAIR_DEVICE, result);
}

/*
void CSM_gapInquiryRespCbk(CSM_BT_DEV_INFO *dev_info)
{
    if (dev_info) {
        pr_info("name %s", dev_info->name);
        pr_info("address %s", dev_info->bdAddr);
    }
}
*/

void csm_inquiry_resp_cb(const char *name, uint8_t *address)
{
    char bdAddr[CSM_BDADDR_MAX_LEN];

    APP_PRINT_INFO2("csm_inquiry_resp_cb: name is %s, address is %s",
                    TRACE_STRING(name), TRACE_BDADDR(address));

    mac_bin_to_str(bdAddr, address);

    pr_info("++");
    if (rtkGapImplCB.gapGapScanCB)
    {
        rtkGapImplCB.gapGapScanCB(name, bdAddr);
    }
}
/*
static const char *pair_result_str(int type)
{
    static const char *str[] = { "FAIL", "SUCESS" };

    if (type <= CSM_PAIR_SUCCESS)
        return str[type];

    return "(unknown)";
}

void CSM_gapPairResultCbk(CSM_BT_PAIR_RESULT result, char *remote_mac)
{
    pr_info("pair result %d %s, %s", result, pair_result_str(result),
        remote_mac);
}
*/
void csm_paired_result_cb(uint8_t result, uint8_t *address)
{
    char bdAddr[CSM_BDADDR_MAX_LEN];

    APP_PRINT_INFO2("csm_paired_result_cb: result = %02x, address is %s",
                    result, TRACE_BDADDR(address));

    mac_bin_to_str(bdAddr, address);

    pr_info("++");
    if (rtkGapImplCB.gapPairCB)
    {
        rtkGapImplCB.gapPairCB(result, bdAddr);
    }
}

/*
static const char *unpair_result_str(int type)
{
    static const char *str[] = { "FAIL", "SUCESS" };

    if (type <= CSM_UNPAIR_SUCCESS)
        return str[type];

    return "(unknown)";
}


void CSM_gapUnpairResultCbk(CSM_BT_UNPAIR_RESULT result, char *remote_mac)
{
    pr_info("unpair result %d %s, %s", result, unpair_result_str(result),
        remote_mac);
}
*/

void csm_unpaired_result_cb(int result, uint8_t *addr)
{
    char bdAddr[CSM_BDADDR_MAX_LEN];

    APP_PRINT_INFO2("csm_unpaired_result_cb: result = %02x, address is %s",
                    result, TRACE_BDADDR(addr));

    mac_bin_to_str(bdAddr, addr);

    pr_info("++");
    if (rtkGapImplCB.gapUnpairCB)
    {
        rtkGapImplCB.gapUnpairCB(result, bdAddr);
    }
}

void CSM_a2dpConnect(const char *mac, CSM_A2DP_ROLE role)
{
    void *argv[2];
    int result;
    uint8_t profile;

    APP_PRINT_INFO0("CSM_a2dpConnect");

    if (role == CSM_A2DP_ROLE_SINK)
    {
        profile = LOCAL_A2DP_SINK;
    }
    else
    {
        profile = LOCAL_A2DP_SOURCE;
    }
    argv[0] = (void *)mac;
    argv[1] = (void *)UINT_TO_PTR(profile);

    result = bta_submit_command_wait(BCMD_A2DP_CONNECT, 2, argv);
    if (result < 0)
        pr_err("Run %04x command failed, %d",
               BCMD_A2DP_CONNECT, result);
}

void CSM_a2dpDisconnect(const char *mac, CSM_A2DP_ROLE role)
{
    void *argv[2];
    int result;
    uint8_t profile;

    APP_PRINT_INFO0("CSM_a2dpDisconnect");

    if (role == CSM_A2DP_ROLE_SINK)
    {
        profile = LOCAL_A2DP_SINK;
    }
    else
    {
        profile = LOCAL_A2DP_SOURCE;
    }
    argv[0] = (void *)mac;
    argv[1] = (void *)UINT_TO_PTR(profile);

    result = bta_submit_command_wait(BCMD_A2DP_DISCONNECT, 2, argv);
    if (result < 0)
        pr_err("Run %04x command failed, %d",
               BCMD_A2DP_DISCONNECT, result);
}

void CSM_a2dpStartPlayer()
{
    pr_info("Not implemented because avrcp is supported");
}

void CSM_a2dpStopPlayer()
{
    pr_info("Not implemented because avrcp is supported");
}

int CSM_a2dpSetRole(CSM_A2DP_ROLE role)
{
    csm_a2dp_role = role;

    return 0;
}

CSM_A2DP_ROLE CSM_a2dpGetRole()
{
    return csm_a2dp_role;
}

void CSM_getA2dpPairedList(CSM_A2DP_DEV_INFO_LIST *info, CSM_A2DP_ROLE role)
{
    void *argv[2];
    uint8_t profile;
    int result;
    int i;

    APP_PRINT_INFO0("CSM_getA2dpPairedList");

    if (role == CSM_A2DP_ROLE_SINK)
    {
        profile = LOCAL_A2DP_SINK;
    }
    else
    {
        profile = LOCAL_A2DP_SOURCE;
    }

    argv[0] = info;
    argv[1] = (void *)UINT_TO_PTR(profile);

    result = bta_submit_command_wait(BCMD_A2DP_PAIRED_LIST, 2, argv);
    if (result < 0)
    {
        pr_err("Run %04x command failed, %d",
               BCMD_A2DP_PAIRED_LIST, result);
        return;
    }

    if (info->dev_num == 0)
    {
        return;
    }

    for (i = 0; i < info->dev_num; i++)
    {
        pr_info("name: %s", info->device_list[i].name);
        pr_info("address: %s", info->device_list[i].bdAddr);
    }
}

/*
static const char *a2dp_state_str(int type)
{
    static const char *str[] = {
        "CSM_A2DP_CONNECTED",
        "CSM_A2DP_DISCONNECTED",
        "CSM_A2DP_CONNECTING",
        "CSM_A2DP_LINK_LOSS",
        "CSM_A2DP_CONNECT_TIMEOUT",
    };

    if (type <= CSM_A2DP_CONNECT_TIMEOUT)
        return str[type];

    return "(unknown)";
}

void CSM_a2dpStateChangedCbk(CSM_A2DP_STATE state, char *mac)
{
    pr_info("state %d %s, address %s", state, a2dp_state_str(state), mac);
}
*/

void csm_a2dp_state_cb(CSM_A2DP_STATE state, uint8_t *address)
{
    char bdAddr[CSM_BDADDR_MAX_LEN];

    mac_bin_to_str(bdAddr, address);

    APP_PRINT_INFO2("csm_a2dp_state_cb: address = %s, state = %d", TRACE_STRING(bdAddr), state);
    if (rtkA2dpImplCB.a2dpStateCB)
    {
        rtkA2dpImplCB.a2dpStateCB(state, bdAddr);
    }
}

/*
static const char *stream_state_str(int type)
{
    static const char *str[] = {
        "CSM_A2DP_STREAM_STATE_STOP",
        "CSM_A2DP_STREAM_STATE_PLAYING",
    };

    if (type <= CSM_A2DP_STREAM_STATE_PLAYING)
        return str[type];

    return "(unknown)";
}


void CSM_a2dpStreamStateCbk(CSM_A2DP_STREAM_STATE streamState)
{
    pr_info("state %d, %s", streamState, stream_state_str(streamState));
}
*/

void csm_a2dp_stream_cb(CSM_A2DP_STREAM_STATE state)
{
    APP_PRINT_INFO1("csm_a2dp_stream_cb: state = %d", state);
    if (rtkA2dpImplCB.a2dpStreamCB)
    {
        rtkA2dpImplCB.a2dpStreamCB(state);
    }
}

void csm_avrcp_connect(const char *address)
{
    int result;
    void *argv[1];

    APP_PRINT_INFO0("csm_avrcp_connects");

    argv[0] = (void *)address;
    result = bta_submit_command_wait(BCMD_AVRCP_CONNECT, 1, argv);
    if (result < 0)
        pr_err("Run %04x command failed, %d",
               BCMD_AVRCP_CONNECT, result);
}

void csm_avrcp_disconnect(void)
{
    int result;

    APP_PRINT_INFO0("csm_avrcp_disconnect");

    result = bta_submit_command_wait(BCMD_AVRCP_DISCONNECT, 0, NULL);
    if (result < 0)
        pr_err("Run %04x command failed, %d",
               BCMD_AVRCP_DISCONNECT, result);
}

void CSM_avrcpSendPassthroughCmd(CSM_AVRCP_CMD_TYPE cmd_type)
{
    int result;
    void *argv[1];

    APP_PRINT_INFO1("CSM_avrcpSendPassthroughCmd: cmd_type = %d", (int)cmd_type);

    argv[0] = (void *)INT_TO_PTR(cmd_type);
    result = bta_submit_command_wait(BCMD_AVRCP_SEND_PASSTHR_CMD, 1, argv);
    if (result < 0)
        APP_PRINT_INFO2("Run %04x command failed, %d",
                        BCMD_AVRCP_SEND_PASSTHR_CMD, result);
}

int CSM_avrcpChangeVolume(unsigned char ui1_vol, CSM_A2DP_ROLE role)
{
    int result;
    void *argv[2];

    APP_PRINT_INFO1("CSM_avrcpChangeVolume: %d", (int)ui1_vol);

    argv[0] = (void *)UINT_TO_PTR(ui1_vol);
    argv[1] = (void *)INT_TO_PTR(role);
    result = bta_submit_command_wait(BCMD_AVRCP_CHG_VOL, 2, argv);
    if (result < 0)
        pr_err("Run %04x command failed, %d",
               BCMD_AVRCP_CHG_VOL, result);

    return result;
}

/*
static const char *avrcp_state_str(int type)
{
    static const char *str[] = {
        "CSM_AVRCP_DISCONNECTED",
        "CSM_AVRCP_CONNECTED",
    };

    if (type <= CSM_AVRCP_CONNECTED)
        return str[type];

    return "(unknown)";
}

void CSM_avrcpStateChangedCbk(CSM_AVRCP_STATUS state)
{
    pr_info("avrcp state %d, %s", state, avrcp_state_str(state));
}
*/

void csm_avrcp_state_cb(CSM_AVRCP_STATUS state)
{
    APP_PRINT_INFO1("csm_avrcp_state_cb: state = %d", state);
    if (rtkAvrcpImplCB.avrcpStateCB)
    {
        rtkAvrcpImplCB.avrcpStateCB(state);
    }
}

/*
static const char *avrcp_play_str(int type)
{
    static const char *str[] = {
        "CSM_AVRCP_PLAY_STATUS_STOPPED",
        "CSM_AVRCP_PLAY_STATUS_PLAYING",
        "CSM_AVRCP_PLAY_STATUS_PAUSEED",
    };

    if (type <= CSM_AVRCP_PLAY_STATUS_PAUSEED)
        return str[type];

    return "(unknown)";
}

void CSM_avrcpPlayStateChangeCbk(CSM_AVRCP_PLAY_STATUS state)
{
    pr_info("avrcp play state %d %s", state, avrcp_play_str(state));
}
*/

void csm_avrcp_play_state_cb(CSM_AVRCP_PLAY_STATUS state)
{
    APP_PRINT_INFO1("csm_avrcp_play_state_cb: state = %d", state);
    if (rtkAvrcpImplCB.avrcpPlayStateCB)
    {
        rtkAvrcpImplCB.avrcpPlayStateCB(state);
    }
}

/*
void CSM_avrcpVolumeChangeCbk(CSM_AVRCP_VOLUME_CHANGE direction)
{
    pr_info("avrcp volume change dir %d", direction);
}
*/

void csm_avrcp_volume_chg_cb(CSM_AVRCP_VOLUME_CHANGE dir)
{
    pr_info("++");
    APP_PRINT_INFO1("csm_avrcp_volume_chg_cb: dir = %d", dir);
    if (rtkAvrcpImplCB.volumeChangeCB)
    {
        rtkAvrcpImplCB.volumeChangeCB(dir);
    }
}

/*
void CSM_avrcpAbsoluteVolumeCbk(unsigned int vol)
{
    pr_info("Received set abs vol %u", vol);
}
*/

void csm_avrcp_abs_volume_cb(unsigned int vol)
{
    pr_info("++");
    APP_PRINT_INFO1("csm_avrcp_abs_volume_cb: vol = %d", vol);
    if (rtkAvrcpImplCB.absoluteVolumeCB)
    {
        rtkAvrcpImplCB.absoluteVolumeCB(vol);
    }
}

/*
static const char *avrcp_passthr_str(int type)
{
    static const char *str[] = {
        "CSM_AVRCP_CMD_TYPE_PLAY",
        "CSM_AVRCP_CMD_TYPE_PAUSE",
        "CSM_AVRCP_CMD_TYPE_FWD",
        "CSM_AVRCP_CMD_TYPE_BWD",
        "CSM_AVRCP_CMD_TYPE_FFWD",
        "CSM_AVRCP_CMD_TYPE_RWD",
        "CSM_AVRCP_CMD_TYPE_STOP",
        "CSM_AVRCP_CMD_TYPE_VOL_UP",
        "CSM_AVRCP_CMD_TYPE_VOL_DOWN",
    };

    if (type <= CSM_AVRCP_CMD_TYPE_VOL_DOWN)
        return str[type];

    return "(unknown)";
}

void CSM_avrcpCmdCbkForSrc(CSM_AVRCP_CMD_TYPE avrcp_cmd)
{
    pr_info("Received passthr cmd %d %s", avrcp_cmd,
        avrcp_passthr_str(avrcp_cmd));
}
*/

void csm_avrcp_passthr_cb(CSM_AVRCP_CMD_TYPE cmd)
{
    APP_PRINT_INFO1("csm_avrcp_passthr_cb: cmd = %d", cmd);
    if (rtkAvrcpImplCB.avrcpCmdSrcCB)
    {
        rtkAvrcpImplCB.avrcpCmdSrcCB(cmd);
    }
}

//////////////////////////////////
//LE GATT Server
//////////////////////////////////

// start a thread
INT32 CSM_init()
{
    int result;

    APP_PRINT_INFO0("CSM_init");

    result = bta_submit_command_wait(BCMD_GATTS_INIT, 0, NULL);
    if (result < 0)
    {
        pr_err("Run %04x command failed, %d",
               BCMD_GATTS_INIT, result);
        return -1;
    }

    return 0;
}

// quit the thread
INT32 CSM_deinitGatts()
{
    int result;

    APP_PRINT_INFO0("CSM_deinitGatts");

    result = bta_submit_command_wait(BCMD_GATTS_DEINIT, 0, NULL);
    if (result < 0)
    {
        pr_err("Run %04x command failed, %d",
               BCMD_GATTS_DEINIT, result);
        return -1;
    }

    return 0;
}

// setcb
void CSM_setCallback(RTK_BT_APP_GATTS_CB_FUNC_T *callback)
{
    APP_PRINT_INFO0("CSM_setCallback");
    GattServerCallbackRegister(callback);
}

// getcb
RTK_BT_APP_GATTS_CB_FUNC_T *CSM_getCallback()
{
    APP_PRINT_INFO0("CSM_getCallback");
    return &rtkGattServerImplCB;
}

INT32 CSM_addService(INT32 server_if, CHAR *service_uuid, UINT8 is_primary, INT32 number)
{
    int result;
    void *argv[4];

    //APP_PRINT_INFO1("CSM_addService: server_if = %d, ", (int)ui1_vol);

    argv[0] = (void *)INT_TO_PTR(server_if);
    argv[1] = (void *)service_uuid;
    argv[2] = (void *)UINT_TO_PTR(is_primary);
    argv[3] = (void *)INT_TO_PTR(number);

    result = bta_submit_command_wait(BCMD_GATTS_ADD_SERVICE, 4, argv);
    if (result < 0)
        pr_err("Run %04x command failed, %d",
               BCMD_GATTS_ADD_SERVICE, result);

    return result;
}

INT32 CSM_addChar(INT32 server_if, INT32 service_handle, CHAR *uuid, INT32 properties,
                  INT32 permissions)
{
    int result;
    void *argv[5];

    //APP_PRINT_INFO1("CSM_addService: server_if = %d, ", (int)ui1_vol);

    argv[0] = (void *)INT_TO_PTR(server_if);
    argv[1] = (void *)INT_TO_PTR(service_handle);
    argv[2] = (void *)uuid;
    argv[3] = (void *)INT_TO_PTR(properties);
    argv[4] = (void *)INT_TO_PTR(permissions);

    result = bta_submit_command_wait(BCMD_GATTS_ADD_CHAR, 5, argv);
    if (result < 0)
        pr_err("Run %04x command failed, %d",
               BCMD_GATTS_ADD_CHAR, result);

    return result;
}

INT32 CSM_addDesc(INT32 server_if, INT32 service_handle, CHAR *uuid, INT32 permissions)
{
    int result;
    void *argv[4];

    //APP_PRINT_INFO1("CSM_addService: server_if = %d, ", (int)ui1_vol);

    argv[0] = (void *)INT_TO_PTR(server_if);
    argv[1] = (void *)INT_TO_PTR(service_handle);
    argv[2] = (void *)uuid;
    argv[3] = (void *)INT_TO_PTR(permissions);

    result = bta_submit_command_wait(BCMD_GATTS_ADD_DESC, 4, argv);
    if (result < 0)
        pr_err("Run %04x command failed, %d",
               BCMD_GATTS_ADD_DESC, result);

    return result;
}

INT32 CSM_startService(INT32 server_if, INT32 service_handle, INT32 transport)
{
    int result;
    void *argv[3];

    //APP_PRINT_INFO1("CSM_addService: server_if = %d, ", (int)ui1_vol);

    argv[0] = (void *)INT_TO_PTR(server_if);
    argv[1] = (void *)INT_TO_PTR(service_handle);
    argv[2] = (void *)INT_TO_PTR(transport);

    result = bta_submit_command_wait(BCMD_GATTS_START_SERVICE, 3, argv);
    if (result < 0)
        pr_err("Run %04x command failed, %d",
               BCMD_GATTS_START_SERVICE, result);

    return result;
}

INT32 CSM_stopService(INT32 server_if, INT32 service_handle)
{
    int result;
    void *argv[2];

    //APP_PRINT_INFO1("CSM_addService: server_if = %d, ", (int)ui1_vol);

    argv[0] = (void *)INT_TO_PTR(server_if);
    argv[1] = (void *)INT_TO_PTR(service_handle);


    result = bta_submit_command_wait(BCMD_GATTS_STOP_SERVICE, 2, argv);
    if (result < 0)
        pr_err("Run %04x command failed, %d",
               BCMD_GATTS_STOP_SERVICE, result);

    return result;
}

INT32 CSM_deleteService(INT32 server_if, INT32 service_handle)
{
    int result;
    void *argv[2];

    //APP_PRINT_INFO1("CSM_addService: server_if = %d, ", (int)ui1_vol);

    argv[0] = (void *)INT_TO_PTR(server_if);
    argv[1] = (void *)INT_TO_PTR(service_handle);


    result = bta_submit_command_wait(BCMD_GATTS_DELETE_SERVICE, 2, argv);
    if (result < 0)
        pr_err("Run %04x command failed, %d",
               BCMD_GATTS_DELETE_SERVICE, result);

    return result;
}

INT32 CSM_unregisterService(INT32 server_if)
{
    int result;
    void *argv[1];

    //APP_PRINT_INFO1("CSM_addService: server_if = %d, ", (int)ui1_vol);

    argv[0] = (void *)INT_TO_PTR(server_if);

    result = bta_submit_command_wait(BCMD_GATTS_UNREGISTER_SERVICE, 1, argv);
    if (result < 0)
        pr_err("Run %04x command failed, %d",
               BCMD_GATTS_UNREGISTER_SERVICE, result);

    return result;
}

INT32 CSM_sendResponse(INT32 conn_id, INT32 trans_id, INT32 status, INT32 handle, CHAR *p_value,
                       INT32 value_len, INT32 auth_req)
{
    int result;
    void *argv[7];

    //APP_PRINT_INFO1("CSM_addService: server_if = %d, ", (int)ui1_vol);

    argv[0] = (void *)INT_TO_PTR(conn_id);
    argv[1] = (void *)INT_TO_PTR(trans_id);
    argv[2] = (void *)INT_TO_PTR(status);
    argv[3] = (void *)INT_TO_PTR(handle);
    argv[4] = (void *)p_value;
    argv[5] = (void *)INT_TO_PTR(value_len);
    argv[6] = (void *)INT_TO_PTR(auth_req);

    result = bta_submit_command_wait(BCMD_GATTS_SEND_RSP, 7, argv);
    if (result < 0)
        pr_err("Run %04x command failed, %d",
               BCMD_GATTS_SEND_RSP, result);

    return result;
}

INT32 CSM_sendIndication(INT32 server_if, INT32 handle, INT32 conn_id, INT32 fg_confirm,
                         CHAR *p_value, INT32 value_len)
{
    int result;
    void *argv[6];

    //APP_PRINT_INFO1("CSM_addService: server_if = %d, ", (int)ui1_vol);

    argv[0] = (void *)INT_TO_PTR(server_if);
    argv[1] = (void *)INT_TO_PTR(handle);
    argv[2] = (void *)INT_TO_PTR(conn_id);
    argv[3] = (void *)INT_TO_PTR(fg_confirm);
    argv[4] = (void *)p_value;
    argv[5] = (void *)INT_TO_PTR(value_len);

    result = bta_submit_command_wait(BCMD_GATTS_SEND_INDICATION, 6, argv);
    if (result < 0)
        pr_err("Run %04x command failed, %d",
               BCMD_GATTS_SEND_INDICATION, result);

    return result;
}

void CSM_setServerIf(INT32 serverIf)
{
    int result;
    void *argv[1];

    //APP_PRINT_INFO1("CSM_addService: server_if = %d, ", (int)ui1_vol);

    argv[0] = (void *)INT_TO_PTR(serverIf);

    result = bta_submit_command_wait(BCMD_GATTS_SET_SERVER_IF, 1, argv);
    if (result < 0)
        pr_err("Run %04x command failed, %d",
               BCMD_GATTS_SET_SERVER_IF, result);

    //return result;
}

void CSM_delServerIf(INT32 serverIf)
{
    int result;
    void *argv[1];

    //APP_PRINT_INFO1("CSM_addService: server_if = %d, ", (int)ui1_vol);

    argv[0] = (void *)INT_TO_PTR(serverIf);

    result = bta_submit_command_wait(BCMD_GATTS_DEL_SERVER_IF, 1, argv);
    if (result < 0)
        pr_err("Run %04x command failed, %d",
               BCMD_GATTS_DEL_SERVER_IF, result);

    //return result;
}

INT32 CSM_enableAdv(bool enable)
{
    int result;
    void *argv[1];
    uint8_t en = enable ? 1 : 0;

    argv[0] = (void *)UINT_TO_PTR(en);
    result = bta_submit_command_wait(BCMD_GATTS_ENABLE_ADV, 1, argv);
    if (result < 0)
        pr_err("Run %04x command failed, %d",
               BCMD_GATTS_DEL_SERVER_IF, result);

    return result;
}


void csm_gatts_init_cb(INT32 server_if)
{
    pr_info("++");
    APP_PRINT_INFO1("csm_gatts_init_cb: server_if = %d", server_if);
    if (rtkGattServerImplCB.onGattsInitCallback)
    {
        rtkGattServerImplCB.onGattsInitCallback(server_if);
    }
}

void csm_gatts_add_service_cb(CSM_AG_GATTS_ADD_SRVC_RST_T *bt_gatts_add_srvc)
{
    if (rtkGattServerImplCB.onGattsAddServiceCallback)
    {
        rtkGattServerImplCB.onGattsAddServiceCallback(bt_gatts_add_srvc);
    }
}

void csm_gatts_add_char_cb(CSM_AG_GATTS_ADD_CHAR_RST_T *bt_gatts_add_char)
{
    if (rtkGattServerImplCB.onGattsAddCharCallback)
    {
        rtkGattServerImplCB.onGattsAddCharCallback(bt_gatts_add_char);
    }
}

void csm_gatts_add_desc_cb(CSM_AG_GATTS_ADD_DESCR_RST_T *bt_gatts_add_desc)
{
    if (rtkGattServerImplCB.onGattsAddDescCallback)
    {
        rtkGattServerImplCB.onGattsAddDescCallback(bt_gatts_add_desc);
    }
}

void csm_gatts_reg_write_cb(CSM_AG_GATTS_REQ_WRITE_RST_T *bt_gatts_req_write)
{
    APP_PRINT_INFO8("csm_gatts_reg_write_cb: %d, %d, %s, %d, %d, %d, %d, %d",
                    bt_gatts_req_write->conn_id, bt_gatts_req_write->trans_id,
                    TRACE_STRING(bt_gatts_req_write->btaddr),
                    bt_gatts_req_write->attr_handle,
                    bt_gatts_req_write->offset, bt_gatts_req_write->length,
                    bt_gatts_req_write->need_rsp, bt_gatts_req_write->is_prep);

    if (rtkGattServerImplCB.onGattsReqWrite)
    {
        rtkGattServerImplCB.onGattsReqWrite(bt_gatts_req_write);
    }
}

void csm_gatts_reg_read_cb(CSM_AG_GATTS_REQ_READ_RST_T *bt_gatts_req_read)
{
    APP_PRINT_INFO6("csm_gatts_reg_read_cb: %d-%d--%s--%d-%d-%d",
                    bt_gatts_req_read->conn_id, bt_gatts_req_read->trans_id,
                    TRACE_STRING(bt_gatts_req_read->btaddr),
                    bt_gatts_req_read->attr_handle, bt_gatts_req_read->offset,
                    bt_gatts_req_read->is_long);
    if (rtkGattServerImplCB.onGattsReqRead)
    {
        rtkGattServerImplCB.onGattsReqRead(bt_gatts_req_read);
    }
}

void csm_gatt_conn_evt_cb(CSM_AG_GATTS_EVENT_T bt_gatts_connection_evt)
{
    if (rtkGattServerImplCB.onGattsConnectionEventCallback)
    {
        rtkGattServerImplCB.onGattsConnectionEventCallback(bt_gatts_connection_evt);
    }
}

void csm_gatts_start_server_cb()
{
    //To be done
    if (rtkGattServerImplCB.onGattsStartServerCallback)
    {
        rtkGattServerImplCB.onGattsStartServerCallback();
    }
}

void csm_gatts_stop_server_cb()
{
    //To be done
    if (rtkGattServerImplCB.onGattsStopServerCallback)
    {
        rtkGattServerImplCB.onGattsStopServerCallback();
    }
}

void csm_gatts_del_server_cb()
{
    //To be done
    if (rtkGattServerImplCB.onGattsDeleteServerCallback)
    {
        rtkGattServerImplCB.onGattsDeleteServerCallback();
    }
}

bool mac_str_to_bin(char *str, uint8_t *mac)
{
    int i;
    char *s, *e;

    if ((mac == NULL) || (str == NULL))
    {
        return false;
    }

    s = (char *) str;
    for (i = 5; i >= 0; i--)
    {
        mac[i] = s ? strtoul(s, &e, 16) : 0;
        if (s)
        {
            s = (*e) ? e + 1 : e;
        }
    }
    return true;
}
bool mac_bin_to_str(char *str, uint8_t *mac)
{
    int i;

    if ((mac == NULL) || (str == NULL))
    {
        return false;
    }

    for (i = 5; i >= 0; i--)
    {
        sprintf(str + 3 * (5 - i), "%02X:", mac[i]);
    }
    str[17] = '\0';
    return true;
}

