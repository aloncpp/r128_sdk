/*
* Copyright (C) 2018 Realtek Semiconductor Corporation. All Rights Reserved.
*/

#ifndef _BT_GAP_H_
#define _BT_GAP_H_

#include "customer_api.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void bt_gap_init(void);

bool bt_gap_read_local_address(uint8_t *addr);
bool bt_gap_read_local_name(char *name, int length);
bool bt_gap_read_remote_address(uint8_t *addr);
bool bt_gap_read_remote_name(uint8_t *addr, char *name, int length);

bool bt_gap_read_remote_info(CSM_BT_DEV_INFO *info);
bool bt_gap_set_scan_mode(bool conn_flag, bool disc_flag);
bool bt_gap_set_local_name(char *name);
bool bt_gap_start_discovery(void);
bool bt_gap_stop_discovery(void);
bool bt_gap_unpair_device(uint8_t *addr);

bool is_gap_stack_ready(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BT_GAP_H_ */

