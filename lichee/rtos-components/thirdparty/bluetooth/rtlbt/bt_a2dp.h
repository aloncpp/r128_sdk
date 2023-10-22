/*
* Copyright (C) 2018 Realtek Semiconductor Corporation. All Rights Reserved.
*/

#ifndef _BT_A2DP_H_
#define _BT_A2DP_H_

#include <stdint.h>
#include <stdbool.h>
#include "customer_api.h"

#include "a2dp.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct
{
    uint8_t         configured_codec_type;
    uint8_t         sample_frequency;
    uint8_t         channel_mode;
    bool            streaming_fg;
    uint8_t         sbc_encode_hdr;
    uint8_t         a2dp_delay_report;
    uint8_t         a2dp_content_protect;
} T_A2DP_LINK_DATA;

void bt_a2dp_init(void);
bool bt_a2dp_connect_req(uint8_t *bd_addr, uint16_t avdtp_ver);
bool bt_a2dp_disconnect_req(uint8_t *bd_addr);
bool bt_a2dp_get_paired_dev_list(CSM_A2DP_DEV_INFO_LIST *info);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BT_A2DP_H_ */

