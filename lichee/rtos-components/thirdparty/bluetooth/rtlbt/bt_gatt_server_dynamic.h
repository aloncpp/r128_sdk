#ifndef _BT_GATT_SERVER_DYNAMIC_H_
#define _BT_GATT_SERVER_DYNAMIC_H_

#include <profile_server.h>

bool bt_gatt_server_dynamic_add_service(int32_t server_if,
                                        char *service_uuid, uint8_t is_primary, int32_t number);

bool bt_gatt_server_dynamic_add_char(int32_t server_if, int32_t service_handle,
                                     char *uuid, int32_t properties, int32_t permissions);

bool bt_gatt_server_dynamic_add_desc(int32_t server_if,
                                     int32_t service_handle, char *uuid, int32_t permissions);

bool bt_gatt_server_dynamic_start_service(int32_t server_if,
                                          int32_t service_handle, int32_t transport);

bool bt_gatt_server_dynamic_send_rsp(int32_t conn_id, int32_t trans_id,
                                     int32_t status, int32_t handle, char *p_val,
                                     int32_t val_len, int32_t auth_req);

bool bt_gatt_server_dynamic_send_indication(int32_t server_if, int32_t handle,
                                            int32_t conn_id, int32_t fg_confirm,
                                            char *p_val, int32_t val_len);

bool bt_gatt_server_dynamic_enable_adv(uint8_t enable);

void bt_gatt_server_dynamic_init(void);

#endif
