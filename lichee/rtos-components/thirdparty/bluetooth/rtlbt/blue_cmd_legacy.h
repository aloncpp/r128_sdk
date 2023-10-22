/*
 *  Copyright (C) 2019 Realtek Semiconductor Corporation.
 *
 *  Author: Alex Lu <alex_lu@realsil.com.cn>
 */

#ifndef __BLUE_CMD_LEGACY_H__
#define __BLUE_CMD_LEGACY_H__

#include <stdint.h>

#if __cplusplus
extern "C" {
#endif

    enum bta_cmdop
    {
        BCMD_TEST = 0,

        //GAP LEGACY
        BCMD_SET_POWER,
        BCMD_READ_LOCAL_INFO,
        BCMD_READ_REMOTE_INFO,
        BCMD_SET_SCAN_MODE,
        BCMD_SET_LOCAL_NAME,
        BCMD_START_DISCOVERY,
        BCMD_STOP_DISCOVERY,
        BCMD_UNPAIR_DEVICE,

        //A2DP
        BCMD_A2DP_CONNECT,
        BCMD_A2DP_DISCONNECT,
        BCMD_A2DP_PAIRED_LIST,

        //AVRCP
        BCMD_AVRCP_CONNECT,
        BCMD_AVRCP_DISCONNECT,
        BCMD_AVRCP_SEND_PASSTHR_CMD,
        BCMD_AVRCP_CHG_VOL,

        //GATT SERVER
        BCMD_GATTS_INIT,
        BCMD_GATTS_DEINIT,
        BCMD_GATTS_ADD_SERVICE,
        BCMD_GATTS_ADD_CHAR,
        BCMD_GATTS_ADD_DESC,
        BCMD_GATTS_START_SERVICE,
        BCMD_GATTS_STOP_SERVICE,
        BCMD_GATTS_DELETE_SERVICE,
        BCMD_GATTS_UNREGISTER_SERVICE,
        BCMD_GATTS_SEND_RSP,
        BCMD_GATTS_SEND_INDICATION,
        BCMD_GATTS_SET_SERVER_IF,
        BCMD_GATTS_DEL_SERVER_IF,
        BCMD_GATTS_ENABLE_ADV,
    };

#define LOCAL_A2DP_SINK    0x00
#define LOCAL_A2DP_SOURCE  0x01

    /*
    void bta_register_legacy_handler(void);
    void csm_test(void);
    */

    int bta_cmd_legacy_process(uint16_t opcode, int argc, void **argv);

#if __cplusplus
};
#endif

#endif
