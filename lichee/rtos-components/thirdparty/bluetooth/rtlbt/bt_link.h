/*
* Copyright (C) 2018 Realtek Semiconductor Corporation. All Rights Reserved.
*/

#ifndef _BT_LINK_H_
#define _BT_LINK_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "bt_a2dp.h"
#include "bt_avrcp.h"
#include "bt_hfp.h"

#define MAX_LINK_NUM        2

#define A2DP_PROFILE_MASK               0x01    //!< bitmask for a2dp profile
#define AVRCP_PROFILE_MASK              0x02    //!< bitmask for avrcp profile
#define HFHS_PROFILE_MASK               0x04    //!< bitmask for hf or hs profile

typedef enum
{
    LINK_STATE_STANDBY,
    LINK_STATE_ALLOCATED,
    LINK_STATE_CONNECTED
} T_LINK_STATE;

typedef struct
{
    T_HFP_LINK_DATA     hfp_data;
    T_AVRCP_LINK_DATA   avrcp_data;
    T_A2DP_LINK_DATA    a2dp_data;
    uint16_t            hci_conn_handle;
    uint16_t            sco_handle;
    uint8_t             bd_addr[6];
    uint8_t             connected_profile;
    uint8_t             acl_link_role_matser_fg;
    uint8_t             acl_link_in_sniffmode_fg;
    T_LINK_STATE        acl_link_state;
} T_BT_LINK;

typedef struct
{
    T_BT_LINK   bt_link[MAX_LINK_NUM];
} T_BT_DATA;

extern T_BT_DATA bt_data;

T_BT_LINK *bt_find_link_by_addr(uint8_t *bd_addr);

T_BT_LINK *bt_alloc_link(uint8_t *bd_addr);

void bt_free_link(T_BT_LINK *p_link);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BT_LINK_H_ */

