/*
* Copyright (C) 2018 Realtek Semiconductor Corporation. All Rights Reserved.
*/

#ifndef _BT_HFP_H_
#define _BT_HFP_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "hfp.h"

#define HFP_INBAND_RINGTONE_ENABLE      0x01
#define HFP_BATTERY_BIEV_ENABLE         0x02

#define AT_MAX_VGS_LEVEL                0x0F
#define AT_MAX_VGM_LEVEL                0x0F

typedef enum
{
    HF_STATE_STANDBY          = 0x00,
    HF_STATE_CONNECTING       = 0x01,
    HF_STATE_RFCOMM_CONNECTED = 0x02,
    HF_STATE_CONNECTED        = 0x03,
    HS_STATE_CONNECTED        = 0x04
} T_HFP_APP_STATE;

typedef struct
{
    T_HFP_APP_STATE                 hfp_state;
    uint8_t                         hsp_avtive_fg;
    T_HFP_ACTIVE_CODEC_TYPE         hfp_active_codec_type;
    uint8_t                         function_enable_flag;
    uint8_t                         bat_report_type;
} T_HFP_LINK_DATA;

bool bt_hfp_init(void);

void bt_hfp_handle_sco_conn_ind(uint8_t *bd_addr, uint8_t is_esco);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BT_HFP_H_ */

