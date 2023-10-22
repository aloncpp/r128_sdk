/*
* Copyright (C) 2018 Realtek Semiconductor Corporation. All Rights Reserved.
*/

#ifndef _BT_AVRCP_H_
#define _BT_AVRCP_H_

#include <stdint.h>
#include <stdbool.h>

#include "avrcp.h"
#include "customer_api.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define AVRCP_MAX_QUEUED_CMD    10 //Should be larger than local supported CT events number

typedef bool (*T_AVRCP_CMD_FUNC)(uint8_t *bd_addr, uint8_t param);

typedef struct
{
    void       *func;
    uint8_t     para;
} T_AVRCP_CMD;

typedef struct
{
    T_AVRCP_CMD     command[AVRCP_MAX_QUEUED_CMD + 1]; //Queue depth, can hold n-1 items.
    uint8_t         write_index;
    uint8_t         read_index;
} T_AVRCP_QUEUE;

typedef struct
{
    T_AVRCP_QUEUE      cmd_queue;
    bool               support_player_status_notify;
    uint8_t            play_status;
    uint8_t            cmd_credit;
} T_AVRCP_LINK_DATA;

typedef enum t_passthrough_op_id
{

    PASSTHROUGH_ID_VOL_UP   = 0x41,     /* volume up */
    PASSTHROUGH_ID_VOL_DOWN = 0x42,     /* volume down */

    PASSTHROUGH_ID_PLAY     = 0x44,     /* play */
    PASSTHROUGH_ID_STOP     = 0x45,     /* stop */
    PASSTHROUGH_ID_PAUSE    = 0x46,     /* pause */
    PASSTHROUGH_ID_RECORD   = 0x47,     /* record */
    PASSTHROUGH_ID_REWIND   = 0x48,     /* rewind */
    PASSTHROUGH_ID_FAST_FOR = 0x49,     /* fast forward */

    PASSTHROUGH_ID_FORWARD  = 0x4B,     /* forward */
    PASSTHROUGH_ID_BACKWARD = 0x4C,     /* backward */

    PASSTHROUGH_ID_MAX      = 0x7f,     /* reserved */

} T_PASSTHROUGH_OP_ID;

bool bt_avrcp_init(void);
uint8_t *bt_avrcp_get_dev_addr(void);
bool bt_avrcp_connect_req(uint8_t *bd_addr);
bool bt_avrcp_disconnect_req(uint8_t *bd_addr);
bool bt_avrcp_notify_volume_change_req(uint8_t *bd_addr, uint8_t vol);
T_PASSTHROUGH_OP_ID avrcp_get_op_id_from_customer_cmd_type(CSM_AVRCP_CMD_TYPE cmd_type);
CSM_AVRCP_CMD_TYPE avrcp_get_cmd_type_from_op_id(T_PASSTHROUGH_OP_ID op_id);
void avrcp_app_send_cmd_passthrough(uint8_t *bd_addr, T_AVRCP_KEY key, bool pressed);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BT_AVRCP_H_ */

