#ifndef _BTMG_BLE_H_
#define _BTMG_BLE_H_

#include "bt_manager.h"
#include "btmg_common.h"
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct bt_gatt_subscribe_params subscribe_params;
typedef struct char_node_t {
    struct char_node_t *front;
    struct char_node_t *next;
    uint16_t char_handle;
    uint8_t conn_id;
    subscribe_params *sub_param;
} char_node_t;

typedef struct char_list_t {
    char_node_t *head;
    char_node_t *tail;
    int length;
    bool list_cleared;
    pthread_mutex_t lock;
} char_list_t;

char_list_t *btmg_char_list_new();
btmg_err btmg_char_list_add_node(char_list_t *char_list, uint8_t conn_id, uint16_t handle, subscribe_params *sub_param);
char_node_t *btmg_char_list_find_node(char_list_t *char_list, uint8_t conn_id, uint16_t handle);
bool btmg_char_list_remove_node(char_list_t *char_list, uint8_t conn_id, uint16_t handle);
void btmg_char_list_clear(char_list_t *list);
void btmg_char_list_free(char_list_t *list);

typedef enum {
    BTMG_LE_EVENT_ERROR = 0,
    //ble connection
    BTMG_LE_EVENT_BLE_SCAN_CB,
    BTMG_LE_EVENT_BLE_CONNECTION,
    BTMG_LE_EVENT_BLE_LE_PARAM_UPDATED,
    BTMG_LE_EVENT_BLE_SECURITY_CHANGED,
    //le smp
    BTMG_LE_EVENT_SMP_AUTH_PASSKEY_DISPLAY,
    BTMG_LE_EVENT_SMP_AUTH_PASSKEY_CONFIRM,
    BTMG_LE_EVENT_SMP_AUTH_PASSKEY_ENTRY,
    BTMG_LE_EVENT_SMP_AUTH_PINCODE_ENTRY,
    BTMG_LE_EVENT_SMP_AUTH_PAIRING_OOB_DATA_REQUEST,
    BTMG_LE_EVENT_SMP_AUTH_CANCEL,
    BTMG_LE_EVENT_SMP_AUTH_PAIRING_CONFIRM,
    BTMG_LE_EVENT_SMP_AUTH_PAIRING_FAILED,
    BTMG_LE_EVENT_SMP_AUTH_PAIRING_COMPLETE,
    BTMG_LE_EVENT_SMP_PAIRING_ACCEPT,
    BTMG_LE_EVENT_SMP_BOND_DELETED,
    //gatt client
    BTMG_LE_EVENT_GATTC_EXCHANGE_MTU_CB,
    BTMG_LE_EVENT_GATTC_WRITE_CB,
    BTMG_LE_EVENT_GATTC_READ_CB,
    BTMG_LE_EVENT_GATTC_DISCOVER,
    BTMG_LE_EVENT_GATTC_NOTIFY,
    //gatt server
    BTMG_LE_EVENT_GATTS_CHAR_WRITE,
    BTMG_LE_EVENT_GATTS_CHAR_READ,
    BTMG_LE_EVENT_GATTS_CCC_CFG_CHANGED,
    BTMG_LE_EVENT_GATTS_GET_DB,
    BTMG_LE_EVENT_GATTS_INCIDATE_CB,
    //OTHER
    BTMG_LE_EVENT_MAX,
} btmg_le_event_t;

int btmg_le_stack_event_callback(btmg_le_event_t type, void *event, size_t len);
btmg_err btmg_ble_init();
#if defined(CONFIG_BT_DEINIT)
btmg_err btmg_ble_deinit();
#endif

#ifdef __cplusplus
}
#endif

#endif /* _BTMG_BLE_H_ */
