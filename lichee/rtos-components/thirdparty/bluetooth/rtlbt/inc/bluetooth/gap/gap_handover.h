/*
 * Copyright (c) 2017, Realtek Semiconductor Corporation. All rights reserved.
 */

#ifndef __GAP_HANDOVER_H__
#define __GAP_HANDOVER_H__

#include <gap.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup    GAP_HANDOVER       GAP Handover
 *
 * \brief   Provide Bluetooth stack handover related functions.
 *
 */

#define GAP_ACL_FLOW_STOP       0
#define GAP_ACL_FLOW_GO         1

#define GAP_ACL_ARQN_NACK       0
#define GAP_ACL_ARQN_ACK        1
typedef enum
{
    GAP_HANDOVER_TYPE_ACL,
    GAP_HANDOVER_TYPE_SM,
    GAP_HANDOVER_TYPE_L2C,
    GAP_HANDOVER_TYPE_SCO,
    GAP_HANDOVER_TYPE_BUD
} T_GAP_HANDOVER_TYPE;

typedef enum
{
    GAP_SHADOW_SNIFF_OP_NO_SNIFFING,
    GAP_SHADOW_SNIFF_OP_SELF_SNIFFING,
    GAP_SHADOW_SNIFF_OP_PEER_SNIFFING
} T_GAP_SHADOW_SNIFF_OP;

typedef struct
{
    uint8_t     bd_addr[6];
    uint16_t    handle;
    uint8_t     role;
    uint8_t     mode;
    uint16_t    link_policy;
    uint16_t    superv_tout;
    uint8_t     encrypt_state;
    uint8_t     bd_type;
    uint8_t     conn_type;
} T_GAP_HANDOVER_ACL_INFO;

typedef struct
{
    uint32_t    mode;
    uint8_t     state;
    uint8_t     sec_state;
    uint8_t     remote_authen;
    uint8_t     remote_io;
    uint8_t     bd_addr[6];
} T_GAP_HANDOVER_SM_INFO;

typedef struct
{
    uint16_t    local_cid;
    uint16_t    remote_cid;

    uint16_t    local_mtu;
    uint16_t    remote_mtu;

    uint16_t    local_mps;
    uint16_t    remote_mps;

    uint16_t    psm;
    uint8_t     role;
    uint8_t     mode;

    uint8_t     bd_addr[6];
} T_GAP_HANDOVER_L2C_INFO;

typedef struct
{
    uint16_t        handle;
    uint8_t         bd_addr[6];
    uint8_t         type;
} T_GAP_HANDOVER_SCO_INFO;

typedef struct
{
    uint8_t     pre_bd_addr[6];
    uint8_t     curr_bd_addr[6];
} T_GAP_HANDOVER_BUD_INFO;

typedef struct
{
    uint16_t        cause;
} T_GAP_SET_ACL_ACTIVE_STATE_RSP;

typedef struct
{
    uint16_t        cause;
} T_GAP_SET_FLOW_RSP;

typedef struct
{
    uint16_t        cause;
} T_GAP_SHADOW_LINK_RSP;

typedef struct
{
    uint16_t        cause;
} T_GAP_HANDOVER_CMPL_INFO;

typedef struct
{
    uint16_t        cause;
    uint16_t        handle;
    uint8_t         bd_addr[6];
    uint8_t         link_type;
    uint8_t         encrypt_enabled;
} T_GAP_HANDOVER_CONN_CMPL_INFO;

typedef struct
{
    uint16_t        cause;
} T_GAP_SHADOW_PRE_SYNC_INFO_RSP;

typedef struct
{
    uint16_t        cause;
} T_GAP_SETUP_AUDIO_RECOVERY_LINK_RSP;

typedef struct
{
    uint8_t         bd_addr[6];
    uint8_t         audio_type;
} T_GAP_AUDIO_RECOVERY_LINK_REQ_IND;

typedef struct
{
    uint16_t        cause;
    uint16_t        recovery_handle;
    uint16_t        ctrl_handle;
    uint16_t        audio_handle;
    uint8_t         audio_type;
} T_GAP_AUDIO_RECOVERY_LINK_CONN_CMPL_INFO;

typedef struct
{
    uint16_t        cause;
    uint16_t        recovery_handle;
    uint16_t        reason;
} T_GAP_AUDIO_RECOVERY_LINK_DISCONN_CMPL_INFO;

typedef enum
{
    GET_ACL_INFO_RSP,
    GET_SCO_INFO_RSP,
    GET_SM_INFO_RSP,
    GET_L2C_INFO_RSP,
    SET_ACL_INFO_RSP,
    SET_SCO_INFO_RSP,
    SET_SM_INFO_RSP,
    SET_L2C_INFO_RSP,
    SET_BD_ADDR_RSP,
    SET_ACL_ACTIVE_STATE_RSP,
    SET_BUD_INFO_RSP,
    DEL_ACL_INFO_RSP,
    DEL_SCO_INFO_RSP,
    DEL_SM_INFO_RSP,
    DEL_L2C_INFO_RSP,
    SET_FLOW_RSP,
    SHADOW_LINK_RSP,
    ACL_RX_EMPTY_INFO,
    HANDOVER_CONN_CMPL_INFO,
    HANDOVER_CMPL_INFO,
    SHADOW_PRE_SYNC_INFO_RSP,
    SETUP_AUDIO_RECOVERY_LINK_RSP,
    AUDIO_RECOVERY_LINK_REQ_IND,
    AUDIO_RECOVERY_LINK_CONN_CMPL_INFO,
    AUDIO_RECOVERY_LINK_DISCONN_CMPL_INFO
} T_HANDOVER_MSG_TYPE;

typedef void (* P_HANDOVER_CB)(void *p_buf, T_HANDOVER_MSG_TYPE msg);

void legacy_reg_handover_cb(P_HANDOVER_CB p_func);

T_GAP_CAUSE legacy_get_handover_acl_info(uint8_t *bd_addr);

T_GAP_CAUSE legacy_get_handover_sco_info(uint8_t *bd_addr);

T_GAP_CAUSE legacy_get_handover_sm_info(uint8_t *bd_addr);

T_GAP_CAUSE legacy_get_handover_l2c_info(uint16_t cid);

T_GAP_CAUSE legacy_set_handover_bud_info(T_GAP_HANDOVER_BUD_INFO *p_info);

T_GAP_CAUSE legacy_set_handover_acl_info(T_GAP_HANDOVER_ACL_INFO *p_info);

T_GAP_CAUSE legacy_set_handover_sco_info(T_GAP_HANDOVER_SCO_INFO *p_info);

T_GAP_CAUSE legacy_set_handover_sm_info(T_GAP_HANDOVER_SM_INFO *p_info);

T_GAP_CAUSE legacy_set_handover_l2c_info(T_GAP_HANDOVER_L2C_INFO *p_info);

T_GAP_CAUSE legacy_del_handover_acl_info(uint8_t *bd_addr);

T_GAP_CAUSE legacy_del_handover_sco_info(uint8_t *bd_addr);

T_GAP_CAUSE legacy_del_handover_sm_info(uint8_t *bd_addr);

T_GAP_CAUSE legacy_del_handover_l2c_info(uint16_t cid);

T_GAP_CAUSE legacy_set_acl_flow(uint16_t acl_handle, uint8_t flow);

T_GAP_CAUSE legacy_shadow_link(uint16_t tgt_handle, uint16_t ctrl_handle,
                               T_GAP_SHADOW_SNIFF_OP sniff_op);

T_GAP_CAUSE legacy_get_remote_feature(uint16_t handle);

T_GAP_CAUSE legacy_shadow_pre_sync_info(uint16_t tgt_handle, uint16_t ctrl_handle,
                                        uint8_t sync_type);

T_GAP_CAUSE legacy_setup_audio_recovery_link(uint16_t ctrl_handle, uint8_t audio_type,
                                             uint16_t audio_handle, uint16_t media_cid, uint16_t interval, uint16_t flush_tout);

T_GAP_CAUSE legacy_audio_recovery_link_req_reply(uint16_t ctrl_handle, uint8_t audio_type,
                                                 uint16_t audio_handle, uint8_t audio_in_order);

T_GAP_CAUSE legacy_remove_audio_recovery_link(uint16_t recovery_handle, uint8_t reason);

T_GAP_CAUSE legacy_reset_audio_recovery_link(uint16_t recovery_handle);

T_GAP_CAUSE legacy_set_bd_addr(uint8_t *bd_addr);

T_GAP_CAUSE legacy_set_acl_arqn(uint16_t acl_handle, uint8_t arqn);
#ifdef __cplusplus
}
#endif    /*  __cplusplus */
#endif    /*  __GAP_HANDOVER_H__*/
