#include <string.h>

#include "kernel/os/os_timer.h"

#include "xr_bt_device.h"
#include "xr_bt_main.h"
#include "bt_manager.h"
#include "btmg_log.h"
#include "btmg_gap.h"
#include "bt_ctrl.h"
#include "btmg_common.h"
#include "btmg_a2dp_sink.h"
#include "btmg_a2dp_source.h"
#include "btmg_spp_client.h"
#include "btmg_spp_server.h"
#include "btmg_hfp_hf.h"
#include "btmg_ble.h"
#include "ble/bluetooth/buf.h"
#include "conn_internal.h"
#include "bluetooth/conn.h"
#include "settings/settings.h"
#include "ble/sys/byteorder.h"

#ifdef CONFIG_BLEHOST

#define DEVICE_NAME     CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

static uint8_t selected_id = BT_ID_DEFAULT;
char_list_t *subscribe_char_list = NULL;
static uint8_t connected_num = 0;
static uint8_t count = 0;

#define NAME_LEN 30
#define KEY_STR_LEN 33

int btmg_le_stack_event_callback(btmg_le_event_t type, void *event, size_t len)
{
    if (!btmg_cb_p) {
        return -1;
    }

    switch (type) {
    case BTMG_LE_EVENT_BLE_SCAN_CB: {
        if (btmg_cb_p[CB_MAIN]->btmg_gattc_cb.le_scan_cb) {
            btmg_cb_p[CB_MAIN]->btmg_gattc_cb.le_scan_cb(event);
        }
        break;
    }
    case BTMG_LE_EVENT_BLE_CONNECTION: {
        if (((le_connection_para_t *)event)->role == 0) {
            if (btmg_cb_p[CB_MAIN]->btmg_gattc_cb.conn_cb) {
                btmg_cb_p[CB_MAIN]->btmg_gattc_cb.conn_cb(event);
            }
        }
        if (((le_connection_para_t *)event)->role == 1) {
            if (btmg_cb_p[CB_MAIN]->btmg_gatts_cb.conn_cb) {
                btmg_cb_p[CB_MAIN]->btmg_gatts_cb.conn_cb(event);
            }
        }
        break;
    }
    case BTMG_LE_EVENT_BLE_LE_PARAM_UPDATED: {
        if (btmg_cb_p[CB_MAIN]->btmg_gattc_cb.le_param_update_cb) {
            btmg_cb_p[CB_MAIN]->btmg_gattc_cb.le_param_update_cb(event);
        }
        break;
    }
    case BTMG_LE_EVENT_BLE_SECURITY_CHANGED: {
        if (btmg_cb_p[CB_MAIN]->btmg_gattc_cb.security_changed_cb) {
            btmg_cb_p[CB_MAIN]->btmg_gattc_cb.security_changed_cb(event);
        }
        break;
    }
    //le smp
    case BTMG_LE_EVENT_SMP_AUTH_PASSKEY_DISPLAY: {
        if (btmg_cb_p[CB_MAIN]->btmg_le_smp_cb.le_smp_passkey_display_cb) {
            btmg_cb_p[CB_MAIN]->btmg_le_smp_cb.le_smp_passkey_display_cb(event);
        }
        break;
    }
    case BTMG_LE_EVENT_SMP_AUTH_PASSKEY_CONFIRM: {
        if (btmg_cb_p[CB_MAIN]->btmg_le_smp_cb.le_smp_passkey_confirm_cb) {
            btmg_cb_p[CB_MAIN]->btmg_le_smp_cb.le_smp_passkey_confirm_cb(event);
        }
        break;
    }
    case BTMG_LE_EVENT_SMP_AUTH_PASSKEY_ENTRY: {
        if (btmg_cb_p[CB_MAIN]->btmg_le_smp_cb.le_smp_passkey_enter_cb) {
            btmg_cb_p[CB_MAIN]->btmg_le_smp_cb.le_smp_passkey_enter_cb(event);
        }
        break;
    }
    case BTMG_LE_EVENT_SMP_AUTH_PINCODE_ENTRY: {
        break;
    }
    case BTMG_LE_EVENT_SMP_AUTH_PAIRING_OOB_DATA_REQUEST: {
        break;
    }
    case BTMG_LE_EVENT_SMP_AUTH_CANCEL: {
        if (btmg_cb_p[CB_MAIN]->btmg_le_smp_cb.le_smp_cancel_cb) {
            btmg_cb_p[CB_MAIN]->btmg_le_smp_cb.le_smp_cancel_cb(event);
        }
        break;
    }
    case BTMG_LE_EVENT_SMP_AUTH_PAIRING_CONFIRM: {
        if (btmg_cb_p[CB_MAIN]->btmg_le_smp_cb.le_smp_pairing_confirm_cb) {
            btmg_cb_p[CB_MAIN]->btmg_le_smp_cb.le_smp_pairing_confirm_cb(event);
        }
        break;
    }
    case BTMG_LE_EVENT_SMP_AUTH_PAIRING_FAILED: {
        if (btmg_cb_p[CB_MAIN]->btmg_le_smp_cb.le_smp_pairing_complete_cb) {
            btmg_cb_p[CB_MAIN]->btmg_le_smp_cb.le_smp_pairing_failed_cb(event);
        }
        break;
    }
    case BTMG_LE_EVENT_SMP_AUTH_PAIRING_COMPLETE: {
        if (btmg_cb_p[CB_MAIN]->btmg_le_smp_cb.le_smp_pairing_complete_cb) {
            btmg_cb_p[CB_MAIN]->btmg_le_smp_cb.le_smp_pairing_complete_cb(event);
        }
        break;
    }
    case BTMG_LE_EVENT_SMP_PAIRING_ACCEPT: {
        break;
    }
    case BTMG_LE_EVENT_SMP_BOND_DELETED: {
        break;
    }
    //gatt client
    case BTMG_LE_EVENT_GATTC_EXCHANGE_MTU_CB: {
        if (btmg_cb_p[CB_MAIN]->btmg_gattc_cb.exchange_mtu_cb) {
            btmg_cb_p[CB_MAIN]->btmg_gattc_cb.exchange_mtu_cb(event);
        }
        break;
    }
    case BTMG_LE_EVENT_GATTC_WRITE_CB: {
        if (btmg_cb_p[CB_MAIN]->btmg_gattc_cb.write_cb) {
            btmg_cb_p[CB_MAIN]->btmg_gattc_cb.write_cb(event);
        }
        break;
    }
    case BTMG_LE_EVENT_GATTC_READ_CB: {
        if (btmg_cb_p[CB_MAIN]->btmg_gattc_cb.read_cb) {
            btmg_cb_p[CB_MAIN]->btmg_gattc_cb.read_cb(event);
        }
        break;
    }
    case BTMG_LE_EVENT_GATTC_DISCOVER: {
        if (btmg_cb_p[CB_MAIN]->btmg_gattc_cb.dis_att_cb) {
            btmg_cb_p[CB_MAIN]->btmg_gattc_cb.dis_att_cb(event);
        }
        break;
    }
    case BTMG_LE_EVENT_GATTC_NOTIFY: {
        if (btmg_cb_p[CB_MAIN]->btmg_gattc_cb.notify_indicate_cb) {
            btmg_cb_p[CB_MAIN]->btmg_gattc_cb.notify_indicate_cb(event);
        }
        break;
    }
    //gatt server
    case BTMG_LE_EVENT_GATTS_CHAR_WRITE: {
        if (btmg_cb_p[CB_MAIN]->btmg_gatts_cb.char_write_req_cb) {
            btmg_cb_p[CB_MAIN]->btmg_gatts_cb.char_write_req_cb(event);
        }
        break;
    }
    case BTMG_LE_EVENT_GATTS_CHAR_READ: {
        if (btmg_cb_p[CB_MAIN]->btmg_gatts_cb.char_read_req_cb) {
            btmg_cb_p[CB_MAIN]->btmg_gatts_cb.char_read_req_cb(event);
        }
        break;
    }
    case BTMG_LE_EVENT_GATTS_CCC_CFG_CHANGED: {
        if (btmg_cb_p[CB_MAIN]->btmg_gatts_cb.ccc_cfg_cb) {
            btmg_cb_p[CB_MAIN]->btmg_gatts_cb.ccc_cfg_cb(event);
        }
        break;
    }
    case BTMG_LE_EVENT_GATTS_GET_DB: {
        if (btmg_cb_p[CB_MAIN]->btmg_gatts_cb.get_db_cb) {
            btmg_cb_p[CB_MAIN]->btmg_gatts_cb.get_db_cb(event);
        }
        break;
    }
    case BTMG_LE_EVENT_GATTS_INCIDATE_CB: {
        if (btmg_cb_p[CB_MAIN]->btmg_gatts_cb.indicate_cb) {
            btmg_cb_p[CB_MAIN]->btmg_gatts_cb.indicate_cb(event);
        }
        break;
    }
    //OTHER
    case BTMG_LE_EVENT_ERROR:
    case BTMG_LE_EVENT_MAX:
    default:
        BTMG_ERROR("unsupported event %d", type);
        break;
    }
    return 0;
}

#if defined(CONFIG_BT_OBSERVER)
static bool le_scan_data_cb(struct bt_data *data, void *user_data)
{
    char *name = user_data;

    switch (data->type) {
    case BT_DATA_NAME_SHORTENED:
    case BT_DATA_NAME_COMPLETE:
        memcpy(name, data->data, MIN(data->data_len, NAME_LEN - 1));
        return false;
    default:
        return true;
    }
}

static void le_scan_recv(const struct bt_le_scan_recv_info *info, struct net_buf_simple *buf)
{
    char name[NAME_LEN];

    (void)memset(name, 0, sizeof(name));

    bt_data_parse(buf, le_scan_data_cb, name);
    if (buf->len > 31) {
        return;
    }
    le_scan_cb_para_t event;
    {
        event.addr.type = info->addr->type;
        event.addr.addr.val[0] = info->addr->a.val[5];
        event.addr.addr.val[1] = info->addr->a.val[4];
        event.addr.addr.val[2] = info->addr->a.val[3];
        event.addr.addr.val[3] = info->addr->a.val[2];
        event.addr.addr.val[4] = info->addr->a.val[1];
        event.addr.addr.val[5] = info->addr->a.val[0];
    }
    event.adv_type = info->adv_type;
    event.can_connect = info->adv_props & BT_GAP_ADV_PROP_SCANNABLE;
    memcpy(event.data, buf->data, buf->len);
    memcpy(event.name, name, strlen(name));
    event.data_len = buf->len;
    event.rssi = info->rssi;

    btmg_le_stack_event_callback(BTMG_LE_EVENT_BLE_SCAN_CB, &event, sizeof(event));
}

static void le_scan_timeout(void)
{
    BTMG_WARNG("Scan timeout");
}

#endif /* CONFIG_BT_OBSERVER */

#if defined(CONFIG_BT_CONN)

static void bt_conn_get_addr_str(struct bt_conn *conn, char *addr, size_t len)
{
    struct bt_conn_info info;

    if (bt_conn_get_info(conn, &info) < 0) {
        addr[0] = '\0';
        return;
    }

    bt_addr_le_to_str(info.le.dst, addr, len);
}

static int bt_addr_to_btmg_bdaddr(bt_addr_le_t *bt_addr, btmg_le_addr_t *btmg_addr)
{
    if (bt_addr == NULL || btmg_addr == NULL) {
        return -1;
    }
    btmg_addr->type = bt_addr->type;
    btmg_addr->addr.val[0] = bt_addr->a.val[5];
    btmg_addr->addr.val[1] = bt_addr->a.val[4];
    btmg_addr->addr.val[2] = bt_addr->a.val[3];
    btmg_addr->addr.val[3] = bt_addr->a.val[2];
    btmg_addr->addr.val[4] = bt_addr->a.val[1];
    btmg_addr->addr.val[5] = bt_addr->a.val[0];

    return 0;
}

static int btmg_bdaddr_to_bt_addr(btmg_le_addr_t *btmg_addr, bt_addr_le_t *bt_addr)
{
    if (bt_addr == NULL || btmg_addr == NULL) {
        return -1;
    }
    bt_addr->type = btmg_addr->type;
    bt_addr->a.val[5] = btmg_addr->addr.val[0];
    bt_addr->a.val[4] = btmg_addr->addr.val[1];
    bt_addr->a.val[3] = btmg_addr->addr.val[2];
    bt_addr->a.val[2] = btmg_addr->addr.val[3];
    bt_addr->a.val[1] = btmg_addr->addr.val[4];
    bt_addr->a.val[0] = btmg_addr->addr.val[5];

    return 0;
}

static int btmg_conn_param_to_bt_conn_param(btmg_le_conn_param_t *in_conn_param,
                                            struct bt_le_conn_param *conn_param)
{
    if (conn_param == NULL) {
        return -1;
    }
    if (in_conn_param == NULL) {
        conn_param->interval_min = BT_GAP_INIT_CONN_INT_MIN;
        conn_param->interval_max = BT_GAP_INIT_CONN_INT_MAX;
        conn_param->latency = 0;
        conn_param->timeout = 400;
    } else {
        conn_param->latency = (uint16_t)(in_conn_param->slave_latency);
        conn_param->timeout = (uint16_t)(in_conn_param->conn_sup_timeout);
        conn_param->interval_min = (uint16_t)(in_conn_param->min_conn_interval);
        conn_param->interval_max = (uint16_t)(in_conn_param->max_conn_interval);
    }
    return 0;
}

static void bt_conn_addr_to_btmg_bdaddr(struct bt_conn *conn, btmg_le_addr_t *btmg_addr)
{
    struct bt_conn_info info;

    if (bt_conn_get_info(conn, &info) < 0) {
        memset(btmg_addr->addr.val, 0, 6);
        return;
    }

    switch (info.type) {
    case BT_CONN_TYPE_LE:
        btmg_addr->type = info.le.dst->type;
        btmg_addr->addr.val[0] = info.le.dst->a.val[5];
        btmg_addr->addr.val[1] = info.le.dst->a.val[4];
        btmg_addr->addr.val[2] = info.le.dst->a.val[3];
        btmg_addr->addr.val[3] = info.le.dst->a.val[2];
        btmg_addr->addr.val[4] = info.le.dst->a.val[1];
        btmg_addr->addr.val[5] = info.le.dst->a.val[0];
        break;
    }
}

static void le_connected(struct bt_conn *conn, uint8_t err)
{
    char addr[BT_ADDR_LE_STR_LEN];
    le_connection_para_t event;

    bt_conn_get_addr_str(conn, addr, sizeof(addr));
    if (err) {
        BTMG_ERROR("Failed to connect to %s (0x%02x)", addr, err);
        goto done;
    }

done:
    event.conn_id = (err ? -1 : bt_conn_index(conn));
    event.status = err ? LE_CONNECT_FAIL : LE_CONNECTED;
    event.role = conn->role;
    BTMG_DEBUG("conn->role: %d", conn->role);
    bt_conn_addr_to_btmg_bdaddr(conn, &event.addr);
    btmg_le_stack_event_callback(BTMG_LE_EVENT_BLE_CONNECTION, &event, sizeof(event));
}

static void le_disconnected(struct bt_conn *conn, uint8_t reason)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_conn_get_addr_str(conn, addr, sizeof(addr));
    uint8_t conn_id = bt_conn_index(conn);
    uint8_t role = conn->role;

    le_connection_para_t event;
    event.conn_id = conn_id;
    event.status = LE_DISCONNECTED;
    event.role = role;
    BTMG_DEBUG("conn->role: %d", conn->role);
    event.reason = reason;
    bt_conn_addr_to_btmg_bdaddr(conn, &event.addr);

    btmg_le_stack_event_callback(BTMG_LE_EVENT_BLE_CONNECTION, &event, sizeof(event));
}

static bool le_param_request(struct bt_conn *conn, struct bt_le_conn_param *param)
{
    BTMG_DEBUG("LE conn param req: interval:(0x%04x, 0x%04x), latency:%d, timeout:%d",
               param->interval_min, param->interval_max, param->latency, param->timeout);

    return true;
}

static void le_param_updated(struct bt_conn *conn, uint16_t interval, uint16_t latency,
                             uint16_t timeout)
{
    BTMG_DEBUG("LE conn param updated: interval 0x%04x, latency %d, timeout %d",
               interval, latency, timeout);
    gatt_le_param_update_cb_para_t event;
    event.conn_id = bt_conn_index(conn);
    event.interval = interval;
    event.latency = latency;
    event.timeout = timeout;
    btmg_le_stack_event_callback(BTMG_LE_EVENT_BLE_LE_PARAM_UPDATED, &event, sizeof(event));
}

#if defined(CONFIG_BT_SMP)
static void identity_resolved(struct bt_conn *conn, const bt_addr_le_t *rpa,
                              const bt_addr_le_t *identity)
{
    char addr_identity[BT_ADDR_LE_STR_LEN];
    char addr_rpa[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(identity, addr_identity, sizeof(addr_identity));
    bt_addr_le_to_str(rpa, addr_rpa, sizeof(addr_rpa));

    BTMG_DEBUG("Identity resolved %s -> %s", addr_rpa, addr_identity);
}
#endif

#if defined(CONFIG_BT_SMP)
static const char *security_err_str(enum bt_security_err err)
{
    switch (err) {
    case BT_SECURITY_ERR_SUCCESS:
        return "Success";
    case BT_SECURITY_ERR_AUTH_FAIL:
        return "Authentication failure";
    case BT_SECURITY_ERR_PIN_OR_KEY_MISSING:
        return "PIN or key missing";
    case BT_SECURITY_ERR_OOB_NOT_AVAILABLE:
        return "OOB not available";
    case BT_SECURITY_ERR_AUTH_REQUIREMENT:
        return "Authentication requirements";
    case BT_SECURITY_ERR_PAIR_NOT_SUPPORTED:
        return "Pairing not supported";
    case BT_SECURITY_ERR_PAIR_NOT_ALLOWED:
        return "Pairing not allowed";
    case BT_SECURITY_ERR_INVALID_PARAM:
        return "Invalid parameters";
    case BT_SECURITY_ERR_UNSPECIFIED:
        return "Unspecified";
    default:
        return "Unknown";
    }
}

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_conn_get_addr_str(conn, addr, sizeof(addr));

    if (!err) {
        BTMG_DEBUG("Security changed: %s level %u", addr, level);
    } else {
        BTMG_DEBUG("Security failed: %s level %u "
                   "reason: %s (%d)",
                   addr, level, security_err_str(err), err);
    }
    gatt_security_changed_cb_para_t event;
    event.conn_id = bt_conn_index(conn);
    event.level = level;
    event.err = err;
    btmg_le_stack_event_callback(BTMG_LE_EVENT_BLE_SECURITY_CHANGED, &event, sizeof(event));
}
#endif

static struct bt_conn_cb conn_callbacks = {
    .connected = le_connected,
    .disconnected = le_disconnected,
    .le_param_req = le_param_request,
    .le_param_updated = le_param_updated,
#if defined(CONFIG_BT_SMP)
    .identity_resolved = identity_resolved,
#endif
#if defined(CONFIG_BT_SMP)
    .security_changed = security_changed,
#endif
};
#endif /* CONFIG_BT_CONN */

#if defined(CONFIG_BT_OBSERVER)
static struct bt_le_scan_cb scan_callbacks = {
    .recv = le_scan_recv,
    .timeout = le_scan_timeout,
};
#endif /* defined(CONFIG_BT_OBSERVER) */

static void bt_ready(int err)
{
    if (err) {
        BTMG_ERROR("Bluetooth init failed (err %d)", err);
        return;
    }

    BTMG_DEBUG("Bluetooth initialized");

#ifdef CONFIG_SETTINGS
    if (IS_ENABLED(CONFIG_SETTINGS)) {
        settings_load();
        BTMG_DEBUG("Settings Loaded");
    }
#endif

    if (IS_ENABLED(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY)) {
        bt_set_oob_data_flag(true);
    }

#if defined(CONFIG_BT_OBSERVER)
    bt_le_scan_cb_register(&scan_callbacks);
#endif

#if defined(CONFIG_BT_CONN)

    bt_conn_cb_register(&conn_callbacks);
#endif /* CONFIG_BT_CONN */
}

btmg_err btmg_ble_init()
{
    int err = 0;
    bool no_ready_cb = false;

    if (no_ready_cb) {
        err = bt_enable(bt_ready);
    } else {
        err = bt_enable(NULL);
        bt_ready(err);
    }

    if (err) {
        BTMG_ERROR("Bluetooth init failed (err %d)", err);
        bt_zephyr_adapter_unregister();
        bt_ctrl_disable();
        return BT_FAIL;
    }

    if (bt_pro_info->gattc_enabled) {
        subscribe_char_list = btmg_char_list_new();
        if (subscribe_char_list == NULL){
            BTMG_ERROR("char list new fail");
            bt_disable();
            bt_zephyr_adapter_unregister();
            bt_ctrl_disable();
            return BT_ERR_NO_MEMORY;
        }
    }

    return BT_OK;
}

#if defined(CONFIG_BT_DEINIT)
btmg_err btmg_ble_deinit()
{
    int err;

#if defined(CONFIG_BT_CONN)
    bt_conn_cb_unregister(&conn_callbacks);
#endif /* CONFIG_BT_CONN */

#if defined(CONFIG_BT_OBSERVER)
    bt_le_scan_cb_unregister(&scan_callbacks);
#endif /* CONFIG_BT_OBSERVER */

    if (IS_ENABLED(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY)) {
        bt_set_oob_data_flag(false);
    }

    btmg_char_list_free(subscribe_char_list);
    err = bt_disable();
    if (err) {
        BTMG_ERROR("ble deinit failed (err %d)", err);
        return BT_FAIL;
    } else {
        BTMG_DEBUG("ble deinitialized");
    }

    return BT_OK;
}
#endif

/**
 *  @brief 开启扫描
 *
 *  @param scan_param: scan param ptr
 *  @return 0 in case of success or other value in case of error.
 */
btmg_err btmg_le_scan_start(btmg_le_scan_param_t *scan_param)
{
    int err;

    struct bt_le_scan_param param = {
        .type = BT_LE_SCAN_TYPE_ACTIVE,
        .options = BT_LE_SCAN_OPT_FILTER_DUPLICATE,
        .interval = BT_GAP_ADV_SLOW_INT_MIN,
        .window = BT_GAP_ADV_SLOW_INT_MIN,
        .timeout = 0,
    };

    if (scan_param->filter_duplicate == LE_SCAN_DUPLICATE_DISABLE) {
        param.options &= ~BT_LE_SCAN_OPT_FILTER_DUPLICATE;
    } else {
        param.options |= BT_LE_SCAN_OPT_FILTER_DUPLICATE;
    }
    if (scan_param->filter_policy == LE_SCAN_FILTER_POLICY_ALLOW_ALL) {
    } else if (scan_param->filter_policy == LE_SCAN_FILTER_POLICY_ONLY_WLIST) {
        param.options |= BT_LE_SCAN_OPT_FILTER_WHITELIST;
    } else if (scan_param->filter_policy == LE_SCAN_FILTER_POLICY_UND_RPA_DIR) {
    } else if (scan_param->filter_policy == LE_SCAN_FILTER_POLICY_WLIST_RPA_DIR) {
        param.options |= BT_LE_SCAN_OPT_FILTER_WHITELIST;
    } //todo
    param.timeout = scan_param->timeout;
    param.interval = scan_param->scan_interval;
    if (scan_param->scan_interval == LE_SCAN_TYPE_PASSIVE) {
        param.type = BT_LE_SCAN_TYPE_PASSIVE;
    }
    param.window = scan_param->scan_window;

    err = bt_le_scan_start(&param, NULL);
    if (err) {
        BTMG_ERROR("LE scan start failed,(err %d)", err);
        return BT_FAIL;
    }

    return BT_OK;
}

/**
 *  @brief 关闭扫描
 *
 *  @param void:
 *  @return 0 in case of success or other value in case of error.
 */
btmg_err btmg_le_scan_stop(void)
{
    int err;

    err = bt_le_scan_stop();
    if (err) {
        BTMG_ERROR("LE scan stop failed,(err %d)",err);
        return BT_FAIL;
    }

    return BT_OK;
}

/*============================GAPADV=============================================*/

#define VEND_BLE_ADV_DATA_ARRAY_SIZE (5)
#define VEND_BLE_ADV_DATA_LEN        (31)

static struct bt_le_adv_param ad_param;
static struct bt_data ad_data[VEND_BLE_ADV_DATA_ARRAY_SIZE];
static struct bt_data sd_data[VEND_BLE_ADV_DATA_ARRAY_SIZE];
static int sig_ad_num;
static int sig_sd_num;
static char ad_data_buf[VEND_BLE_ADV_DATA_LEN];
static char sd_data_buf[VEND_BLE_ADV_DATA_LEN];

/**
 *  @brief 启动广播
 *  @param enable
 *  @return 0 in case of success or other value in case of error.
 */
btmg_err btmg_le_enable_adv(bool enable)
{
    int err;

    if (enable) {
        err = bt_le_adv_start(&ad_param, ad_data, sig_ad_num, sd_data, sig_sd_num);
        if (err < 0) {
            BTMG_ERROR("Failed to start advertising (err %d)", err);
            return BT_FAIL;
        }
    } else {
        err = bt_le_adv_stop();
        if (err < 0) {
            BTMG_ERROR("Failed to stop advertising (err %d)", err);
            return BT_FAIL;
        }
    }
    return BT_OK;
}

/**
 *  @brief 启动扩展广播
 *  @param enable
 *  @return 0 in case of success or other value in case of error.
 */
btmg_err btmg_le_enable_ext_adv(bool enable)
{
    return btmg_le_enable_adv(enable);
}

/**
 *  @brief 设置广播参数
 *  @param adv_param: 广播参数
 *  @return 0 in case of success or other value in case of error.
 */
btmg_err btmg_le_set_adv_param(btmg_le_adv_param_t *adv_param)
{
    if (adv_param == NULL) {
        ad_param.id = selected_id;
        ad_param.interval_min = 0x00a0;
        ad_param.interval_max = 0x00f0;
        ad_param.options = BT_LE_ADV_OPT_CONNECTABLE;
        ad_param.options |= BT_LE_ADV_OPT_USE_NAME;
    } else {
        ad_param.id = selected_id;
        ad_param.interval_min = adv_param->interval_min;
        ad_param.interval_max = adv_param->interval_max;

        if (adv_param->adv_type == BTMG_LE_ADV_IND) {
            ad_param.options |= BT_LE_ADV_OPT_CONNECTABLE;
            ad_param.options |= BT_LE_ADV_OPT_USE_NAME;
        } else if (adv_param->adv_type == BTMG_LE_ADV_DIRECT_HIGH_IND) {
            ad_param.options |= BT_LE_ADV_OPT_CONNECTABLE;
        } else if (adv_param->adv_type == BTMG_LE_ADV_SCAN_IND) {
            ad_param.options |= BT_LE_ADV_OPT_SCANNABLE;
            ad_param.options |= BT_LE_ADV_OPT_USE_NAME;
        } else if (adv_param->adv_type == BTMG_LE_ADV_DIRECT_LOW_IND) {
            ad_param.options |= BT_LE_ADV_OPT_CONNECTABLE;
            ad_param.options |= BT_LE_ADV_OPT_DIR_MODE_LOW_DUTY;
        }

        if (adv_param->filter == BTMG_LE_PROCESS_CONN_REQ) {
           ad_param.options |= BT_LE_ADV_OPT_FILTER_CONN;
        } else if (adv_param->filter == BTMG_LE_PROCESS_SCAN_REQ) {
           ad_param.options |= BT_LE_ADV_OPT_FILTER_SCAN_REQ;
        }
    }

    return BT_OK;
}

/**
 *  @brief 设置广播参数
 *  @param ext_adv_param: 广播参数
 *  @return 0 in case of success or other value in case of error.
 */
btmg_err btmg_le_set_ext_adv_param(btmg_le_ext_adv_param_t *ext_adv_param)
{
    if (ext_adv_param == NULL) {
        btmg_le_ext_adv_param_t ext_adv_default_param;
        ext_adv_param->prim_min_interval[0] = 0x00, ext_adv_param->prim_min_interval[1] = 0x00,
        ext_adv_param->prim_min_interval[2] = 0x30;
        ext_adv_param->prim_max_interval[0] = 0x00, ext_adv_param->prim_max_interval[1] = 0x00,
        ext_adv_param->prim_max_interval[2] = 0x30;
        ad_param.id = selected_id;
        ad_param.interval_min = sys_get_be24(ext_adv_param->prim_min_interval);
        ad_param.interval_max = sys_get_be24(ext_adv_param->prim_max_interval);
        ad_param.options |= BT_LE_ADV_OPT_CONNECTABLE;
        ad_param.options |= BT_LE_ADV_OPT_USE_NAME;
    } else {
        ad_param.id = selected_id;
        ad_param.interval_min = sys_get_be24(ext_adv_param->prim_min_interval);
        ad_param.interval_max = sys_get_be24(ext_adv_param->prim_max_interval);
        ad_param.options |= BT_LE_ADV_OPT_CONNECTABLE;
        ad_param.options |= BT_LE_ADV_OPT_USE_NAME;
    }

    return BT_OK;
}

/**
 *  @brief 设置广播和扫描响应数据
 *
 *  @param adv_data: 广播数据
 *  @param scan_rsp_data: 扫描响应数据
 *  @return 0 in case of success or other value in case of error.
 */
btmg_err btmg_le_set_adv_scan_rsp_data(btmg_adv_scan_rsp_data_t *adv_data,
                                       btmg_adv_scan_rsp_data_t *scan_rsp_data)
{
    int ret = 0;
    int i = 0;
    int j = 0;

    if (adv_data != NULL && adv_data->data != NULL && adv_data->data_len > 0) {
        memcpy(ad_data_buf, adv_data->data, adv_data->data_len);
        for (i = 0, j = 0, sig_ad_num = 0; i < VEND_BLE_ADV_DATA_ARRAY_SIZE; i++) {
            ad_data[i].data_len = ad_data_buf[j] - 1;
            ad_data[i].type = ad_data_buf[j + 1];
            ad_data[i].data = &ad_data_buf[j + 2];
            j = j + ad_data_buf[j] + 1;
            if (ad_data[i].type == BT_DATA_NAME_COMPLETE ||
                ad_data[i].type == BT_DATA_NAME_SHORTENED) {
                    ad_param.options &= ~BT_LE_ADV_OPT_USE_NAME;
            }
            sig_ad_num++;
            if (j >= adv_data->data_len)
                break;
        }
    }


    if (scan_rsp_data != NULL && scan_rsp_data->data != NULL && scan_rsp_data->data_len > 0) {
        memcpy(sd_data_buf, scan_rsp_data->data, scan_rsp_data->data_len);
        for (i = 0, j = 0, sig_sd_num = 0; i < VEND_BLE_ADV_DATA_ARRAY_SIZE; i++) {
            sd_data[i].data_len = sd_data_buf[j] - 1;
            sd_data[i].type = sd_data_buf[j + 1];
            sd_data[i].data = &sd_data_buf[j + 2];
            j = j + sd_data_buf[j] + 1;
            sig_sd_num++;
            if (j >= scan_rsp_data->data_len)
                break;
        }
    }
    return BT_OK;
}

/**
 * @brief
 *
 *  @param void:
 *  @return 0 in case of success or other value in case of error.
 */
btmg_err btmg_le_set_name(const char *name)
{
    int err;

    if (name == NULL) {
        return BT_ERR_INVALID_ARG;
    }
    err = bt_set_name(name);
    if (err) {
        BTMG_ERROR("set name failed");
    } else {
        BTMG_DEBUG("set name done");
    }
     return BT_OK;
}

/**
 *  @brief 获取设备名称
 *  @param void:
 *  @return char *
 */
const char *btmg_le_get_name(void)
{
    return bt_get_name();
}

static void le_connection_count(struct bt_conn *conn, void *user_data)
{
    int *conn_count = user_data;

    (*conn_count)++;

    connected_num = *conn_count;
    count = 0;
}

void btmg_le_get_connected_num(int *conn_count)
{
    bt_conn_foreach(BT_CONN_TYPE_LE, le_connection_count, conn_count);
}

static void le_connection_info(struct bt_conn *conn, void *user_data)
{
    gattc_connected_list_para_t *param = user_data;

    bt_conn_addr_to_btmg_bdaddr(conn, &((param + count)->addr));
    (param + count)->conn_id = bt_conn_index(conn);
    count ++;
}

btmg_err btmg_le_get_connected_list(gattc_connected_list_para_t *param)
{
    bt_conn_foreach(BT_CONN_TYPE_LE, le_connection_info, param);

    return BT_OK;
}

btmg_err btmg_le_conn_param_update(uint8_t conn_id, btmg_le_conn_param_t *param)
{
    int err;
    struct bt_conn *conn;
    struct bt_le_conn_param conn_param = { 0 };

    btmg_conn_param_to_bt_conn_param(param, &conn_param);
    conn = bt_conn_lookup_index(conn_id);
    if (conn == NULL) {
        BTMG_ERROR("not dev connected");
        return BT_FAIL;
    }
    err = bt_conn_le_param_update(conn, &conn_param);
    bt_conn_unref(conn);
    if (err) {
        BTMG_ERROR("conn update failed (err %d).", err);
        return err;
    } else {
        BTMG_DEBUG("conn update initiated.");
    }

    return BT_OK;
}

btmg_err btmg_le_connect(const btmg_le_addr_t *peer, btmg_le_conn_param_t *in_conn_param)
{
    int err;
    bt_addr_le_t addr;
    struct bt_conn *conn;
    uint32_t options = 0;
    struct bt_conn_le_create_param *create_params =
            BT_CONN_LE_CREATE_PARAM(options, BT_GAP_SCAN_FAST_INTERVAL, BT_GAP_SCAN_FAST_INTERVAL);

    if (peer == NULL) {
        return BT_ERR_INVALID_ARG;
    }

    addr.type = peer->type;
    memcpy(addr.a.val, peer->addr.val, 6);

    struct bt_le_conn_param conn_param = { 0 };
    btmg_conn_param_to_bt_conn_param(in_conn_param, &conn_param);
    err = bt_conn_le_create(&addr, create_params, &conn_param, &conn);

    if (err) {
        BTMG_ERROR("Connection failed (%d)", err);
        return BT_FAIL;
    } else {
        // unref connection obj in advance as app user
        bt_conn_unref(conn);
    }

    return BT_OK;
}

btmg_err btmg_le_disconnect(uint8_t conn_id, uint8_t reason)
{
    int err;
    struct bt_conn *conn = bt_conn_lookup_index(conn_id);

    if (conn == NULL) {
        BTMG_ERROR("no connection");
        return BT_FAIL;
    }

    err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
    if (err) {
        bt_conn_unref(conn);
        return err;
    }
    bt_conn_unref(conn);

    return BT_OK;
}

btmg_err btmg_le_unpair(btmg_le_addr_t *addr)
{
    bt_addr_le_t peer_addr;

    if (addr == NULL) {
        return bt_unpair(0, NULL);
    }
    btmg_bdaddr_to_bt_addr(addr, &peer_addr);

    return bt_unpair(0, &peer_addr);
}

#if defined(CONFIG_BT_WHITELIST)
btmg_err btmg_le_whitelist_add(btmg_le_addr_t *addr)
{
    int err;
    bt_addr_le_t peer_addr;

    btmg_bdaddr_to_bt_addr(addr, &peer_addr);
    err = bt_le_whitelist_add((bt_addr_le_t *)&peer_addr);
    if (err) {
        BTMG_ERROR("Clearing whitelist failed (err %d)", err);
        return err;
    }

    return BT_OK;
}

btmg_err btmg_le_white_list_remove(btmg_le_addr_t *addr)
{
    int err;
    bt_addr_le_t peer_addr;

    btmg_bdaddr_to_bt_addr(addr, &peer_addr);
    err = bt_le_whitelist_rem((bt_addr_le_t *)&peer_addr);
    if (err) {
        BTMG_ERROR("Remove whitelist failed (err %d)", err);
        return err;
    }

    return BT_OK;
}

btmg_err btmg_le_whitelist_clear(void)
{
    int err;

    err = bt_le_whitelist_clear();
    if (err) {
        BTMG_ERROR("Clearing whitelist failed (err %d)", err);
        return err;
    }

    return BT_OK;
}

btmg_err btmg_le_connect_auto_start(btmg_le_conn_param_t *in_conn_param)
{
    uint32_t options = 0;
    int err;
    struct bt_conn_le_create_param *create_params =
            BT_CONN_LE_CREATE_PARAM(options, BT_GAP_SCAN_FAST_INTERVAL, BT_GAP_SCAN_FAST_INTERVAL);
    struct bt_le_conn_param conn_param = { 0 };

    btmg_conn_param_to_bt_conn_param(in_conn_param, &conn_param);
    err = bt_conn_le_create_auto(create_params, &conn_param);

    return err;
}

btmg_err btmg_le_connect_auto_stop(void)
{
    return bt_conn_create_auto_stop();
}

#else /* defined(CONFIG_BT_WHITELIST) */
btmg_err btmg_le_set_auto_connect(btmg_le_addr_t *addr, btmg_le_conn_param_t *in_conn_param)
{
    struct bt_le_conn_param conn_param = { 0 };
    bt_addr_le_t peer_addr;

    btmg_bdaddr_to_bt_addr(addr, &peer_addr);
    if (in_conn_param == NULL) {
        bt_le_set_auto_conn(&peer_addr, NULL);
        return BT_OK;
    }
    btmg_conn_param_to_bt_conn_param(in_conn_param, &conn_param);
    bt_le_set_auto_conn(&peer_addr, &conn_param);
    return BT_OK;
}
#endif

#if defined(CONFIG_BT_SMP)

int btmg_le_get_security(uint8_t conn_id)
{
    int ret = -1;

    struct bt_conn *conn = bt_conn_lookup_index(conn_id);

    if (conn) {
        ret = bt_conn_get_security(conn);
        bt_conn_unref(conn);
    }

    return ret;
}

btmg_err btmg_le_set_security(uint8_t conn_id, int level)
{
    int ret = BT_FAIL;

    struct bt_conn *conn = bt_conn_lookup_index(conn_id);

    if (conn) {
        ret = bt_conn_set_security(conn, level);
        bt_conn_unref(conn);
    }

    return ret;
}

btmg_err btmg_le_smp_passkey_entry(uint8_t conn_id, uint32_t passkey)
{
    int ret = BT_FAIL;

    if (conn_id < 0) {
        return BT_ERR_INVALID_ARG;
    }

    struct bt_conn *conn = bt_conn_lookup_index(conn_id);

    if (conn) {
        ret = bt_conn_auth_passkey_entry(conn, passkey);
        bt_conn_unref(conn);
    }

    return ret;
}

btmg_err btmg_le_smp_cancel(uint8_t conn_id)
{
    int ret = BT_FAIL;

    if (conn_id < 0) {
        return BT_ERR_INVALID_ARG;
    }

    struct bt_conn *conn = bt_conn_lookup_index(conn_id);

    if (conn) {
        ret = bt_conn_auth_cancel(conn);
        bt_conn_unref(conn);
    }

    return ret;
}

btmg_err btmg_le_smp_passkey_confirm(uint8_t conn_id)
{
    int ret = BT_FAIL;

    if (conn_id < 0) {
        return BT_ERR_INVALID_ARG;
    }

    struct bt_conn *conn = bt_conn_lookup_index(conn_id);

    if (conn) {
        ret = bt_conn_auth_passkey_confirm(conn);
        bt_conn_unref(conn);
    }

    return ret;
}

btmg_err btmg_le_smp_pairing_confirm(uint8_t conn_id)
{
    int ret = BT_FAIL;

    if (conn_id < 0) {
        return BT_ERR_INVALID_ARG;
    }

    struct bt_conn *conn = bt_conn_lookup_index(conn_id);

    if (conn) {
        ret = bt_conn_auth_pairing_confirm(conn);
        bt_conn_unref(conn);
    }

    return ret;
}

/**
 *  auth callback
 *  !BTMG_LE_EVENT_SMP_AUTH_PINCODE_ENTRY,
 *  !BTMG_LE_EVENT_SMP_AUTH_PAIRING_OOB_DATA_REQUEST,
 *  !BTMG_LE_EVENT_SMP_PAIRING_ACCEPT,
 *  !BTMG_LE_EVENT_SMP_BOND_DELETED,
 */
static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
    char passkey_str[7] = { 0 };
    le_smp_passkey_display_para_t event;

    bt_conn_addr_to_btmg_bdaddr(conn, &event.addr);
    snprintf(passkey_str, 7, "%06u", passkey);

    event.conn_id = bt_conn_index(conn);
    event.passkey = passkey_str;
    btmg_le_stack_event_callback(BTMG_LE_EVENT_SMP_AUTH_PASSKEY_DISPLAY, &event, sizeof(event));
}

static void auth_passkey_confirm(struct bt_conn *conn, unsigned int passkey)
{
    char passkey_str[7] = { 0 };
    le_smp_passkey_confirm_para_t event;

    bt_conn_addr_to_btmg_bdaddr(conn, &event.addr);
    snprintf(passkey_str, 7, "%06u", passkey);

    event.conn_id = bt_conn_index(conn);
    event.passkey = passkey_str;
    btmg_le_stack_event_callback(BTMG_LE_EVENT_SMP_AUTH_PASSKEY_CONFIRM, &event, sizeof(event));
}

static void auth_passkey_entry(struct bt_conn *conn)
{
    le_smp_passkey_enter_para_t event;

    bt_conn_addr_to_btmg_bdaddr(conn, &event.addr);

    event.conn_id = bt_conn_index(conn);
    btmg_le_stack_event_callback(BTMG_LE_EVENT_SMP_AUTH_PASSKEY_ENTRY, &event, sizeof(event));
}

#if defined(CONFIG_BT_SMP_APP_PAIRING_ACCEPT)
enum bt_security_err auth_pairing_accept(struct bt_conn *conn,
                                         const struct bt_conn_pairing_feat *const feat)
{
    BTMG_DEBUG("%s, Remote pairing features: "
               "IO: 0x%02x, OOB: %d, AUTH: 0x%02x, Key: %d, "
               "Init Kdist: 0x%02x, Resp Kdist: 0x%02x",
               __func__, feat->io_capability, feat->oob_data_flag, feat->auth_req,
               feat->max_enc_key_size, feat->init_key_dist, feat->resp_key_dist);

    return BT_SECURITY_ERR_SUCCESS;
}
#endif /* CONFIG_BT_SMP_APP_PAIRING_ACCEPT */

static void bond_deleted(uint8_t id, const bt_addr_le_t *peer)
{
    char addr[BT_ADDR_STR_LEN];

    bt_addr_le_to_str(peer, addr, sizeof(addr));
    BTMG_DEBUG("%s, Bond deleted for %s, id %u", __func__, addr, id);
}

static void auth_cancel(struct bt_conn *conn)
{
    le_smp_cancel_para_t event;

    bt_conn_addr_to_btmg_bdaddr(conn, &event.addr);

    event.conn_id = bt_conn_index(conn);
    btmg_le_stack_event_callback(BTMG_LE_EVENT_SMP_AUTH_CANCEL, &event, sizeof(event));
}

static void auth_pairing_confirm(struct bt_conn *conn)
{
    le_smp_pairing_confirm_para_t event;

    bt_conn_addr_to_btmg_bdaddr(conn, &event.addr);

    event.conn_id = bt_conn_index(conn);
    btmg_le_stack_event_callback(BTMG_LE_EVENT_SMP_AUTH_PAIRING_CONFIRM, &event, sizeof(event));
}

static void auth_pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
    le_smp_pairing_complete_para_t event;

    bt_conn_addr_to_btmg_bdaddr(conn, &event.addr);

    event.conn_id = bt_conn_index(conn);
    event.err = reason;
    btmg_le_stack_event_callback(BTMG_LE_EVENT_SMP_AUTH_PAIRING_FAILED, &event, sizeof(event));
}

static void auth_pairing_complete(struct bt_conn *conn, bool bonded)
{
    le_smp_pairing_complete_para_t event;

    bt_conn_addr_to_btmg_bdaddr(conn, &event.addr);

    event.conn_id = bt_conn_index(conn);
    event.bonded = bonded;
    event.err = 0;
    btmg_le_stack_event_callback(BTMG_LE_EVENT_SMP_AUTH_PAIRING_COMPLETE, &event, sizeof(event));
}

static struct bt_conn_auth_cb auth_callbacks = {
    .cancel = auth_cancel,
    .pairing_failed = auth_pairing_failed,
    .pairing_complete = auth_pairing_complete,
};

btmg_err btmg_le_smp_set_iocap(btmg_io_capability_t io_cap)
{
    int ret = BT_FAIL;

    if (io_cap == BTMG_IO_CAP_DISPLAYONLY) {
        auth_callbacks.passkey_display = auth_passkey_display;
        auth_callbacks.cancel = auth_cancel;
        auth_callbacks.pairing_confirm = auth_pairing_confirm;
        auth_callbacks.pairing_failed = auth_pairing_failed;
        auth_callbacks.pairing_complete = auth_pairing_complete;
    } else if (io_cap == BTMG_IO_CAP_DISPLAYYESNO) {
        auth_callbacks.passkey_display = auth_passkey_display;
        auth_callbacks.passkey_confirm = auth_passkey_confirm;
        auth_callbacks.cancel = auth_cancel;
        auth_callbacks.pairing_confirm = auth_pairing_confirm;
        auth_callbacks.pairing_failed = auth_pairing_failed;
        auth_callbacks.pairing_complete = auth_pairing_complete;
#if defined(CONFIG_BT_SMP_APP_PAIRING_ACCEPT)
        auth_callbacks.pairing_accept = auth_pairing_accept;
#endif
        auth_callbacks.bond_deleted = bond_deleted;
    } else if (io_cap == BTMG_IO_CAP_KEYBOARDONLY) {
        auth_callbacks.passkey_entry = auth_passkey_entry;
        auth_callbacks.passkey_confirm = auth_passkey_confirm;
        auth_callbacks.cancel = auth_cancel;
        auth_callbacks.pairing_confirm = auth_pairing_confirm;
        auth_callbacks.pairing_failed = auth_pairing_failed;
        auth_callbacks.pairing_complete = auth_pairing_complete;
    } else if (io_cap == BTMG_IO_CAP_NOINPUTNOOUTPUT) {
        auth_callbacks.cancel = NULL;
        ret = bt_conn_auth_cb_register(NULL);
    } else {
        auth_callbacks.cancel = NULL;
        ret = bt_conn_auth_cb_register(NULL);
        return BT_FAIL;
    }
    if (auth_callbacks.cancel != NULL) {
        ret = bt_conn_auth_cb_register(&auth_callbacks);
        if (ret) {
            return ret;
        }
    }
    return ret;
}
#endif //if defined(CONFIG_BT_SMP)

#if defined(CONFIG_BT_CENTRAL)
char_list_t *btmg_char_list_new()
{
    char_list_t *list = NULL;

    list = (char_list_t *)calloc(1, sizeof(char_list_t));
    if (list != NULL) {
        list->list_cleared = false;
        if (0 != pthread_mutex_init(&list->lock, NULL)) {
            BTMG_DEBUG("init mutex for dev list failed");
            free(list);
            return NULL;
        }
        return list;
    }
}

btmg_err btmg_char_list_add_node(char_list_t *char_list, uint8_t conn_id, uint16_t handle, subscribe_params *sub_param)
{
    char_node_t *char_node = NULL;
    char_node_t *char_tail = NULL;

    if (!char_list) {
        BTMG_ERROR("dev_list is null");
        return BT_ERR_INVALID_ARG;
    }

    if (sub_param == NULL) {
        BTMG_ERROR("sub_param is null, add param failed!");
        return BT_ERR_INVALID_ARG;
    }

    char_node = (char_node_t *)calloc(1, sizeof(char_node_t));
    if (char_node == NULL) {
        BTMG_ERROR("calloc for char_node failed");
        return BT_ERR_NO_MEMORY;
    }

    char_node->char_handle = handle;
    char_node->sub_param = sub_param;
    char_node->conn_id = conn_id;

    char_node->next = NULL;
    pthread_mutex_lock(&char_list->lock);

    if (char_list->list_cleared) {
        BTMG_WARNG("dev_list is cleared, nothing could be done");
        pthread_mutex_unlock(&char_list->lock);
        free(char_node);
        return BT_FAIL;
    }

    if (char_list->head == NULL) {
        char_list->head = char_node;
        char_list->tail = char_node;
        char_node->front = NULL;
    } else {
        char_tail = char_list->tail;
        char_node->front = char_tail;
        char_tail->next = char_node;
        char_list->tail = char_node;
    }
    pthread_mutex_unlock(&char_list->lock);

    return BT_OK;
}

char_node_t *btmg_char_list_find_node(char_list_t *char_list, uint8_t conn_id, uint16_t handle)
{
    char_node_t *char_node = NULL;

    if (char_list == NULL) {
        BTMG_ERROR("dev_list is null");
        return NULL;
    }

    pthread_mutex_lock(&char_list->lock);
    if (char_list->list_cleared) {
        BTMG_INFO("char_list is cleared, nothing could be done");
        pthread_mutex_unlock(&char_list->lock);
        return NULL;
    }

    char_node = char_list->head;
    if (char_node == NULL) {
        pthread_mutex_unlock(&char_list->lock);
        return NULL;
    }

    while (char_node != NULL) {
        if ((char_node->conn_id == conn_id)&&(char_node->char_handle == handle)) {
            pthread_mutex_unlock(&char_list->lock);
            return char_node;
        } else
            char_node = char_node->next;
    }
    pthread_mutex_unlock(&char_list->lock);

    return NULL;
}

static char_node_t *btmg_char_list_find_node_inner(char_list_t *char_list, uint8_t conn_id, uint16_t handle)
{
    char_node_t *char_node = NULL;

    if (!char_list) {
        BTMG_ERROR("char_list is null");
        return NULL;
    }

    char_node = char_list->head;
    if (char_node == NULL) {
        BTMG_ERROR("char_node is null");
        return NULL;
    }

    while (char_node != NULL) {
        if ((char_node->conn_id == conn_id)&&(char_node->char_handle == handle))
            return char_node;
        else
            char_node = char_node->next;
    }

    return NULL;
}

bool btmg_char_list_remove_node(char_list_t *char_list, uint8_t conn_id, uint16_t handle)
{
    char_node_t *char_node = NULL;

    if (!char_list) {
        BTMG_ERROR("char_list is null");
        return false;
    }

    pthread_mutex_lock(&char_list->lock);

    if (char_list->list_cleared) {
        BTMG_ERROR("char_list is cleared, nothing could be done");
        pthread_mutex_unlock(&char_list->lock);
        return false;
    }

    char_node = btmg_char_list_find_node_inner(char_list, conn_id, handle);

    if (char_node == NULL) {
        BTMG_ERROR("char_node is not in list");
        pthread_mutex_unlock(&char_list->lock);
        return true;
    }

    if (char_node == char_list->head) {
        if (char_node->next == NULL) { //char_node is the only node in char_list
            free(char_node);
            char_list->head = NULL;
            char_list->tail = NULL;
        } else {
            char_list->head = char_node->next;
            char_node->next->front = NULL;
            free(char_node);
        }
    } else {
        char_node->front->next = char_node->next;
        if (char_node->next) { //char_node is not the tail of char_list
            char_node->next->front = char_node->front;
        } else {
            char_list->tail = char_node->front;
        }
        free(char_node);
    }
    pthread_mutex_unlock(&char_list->lock);

    return true;
}

void btmg_char_list_clear(char_list_t *list)
{
    char_node_t *char_node = NULL;
    char_node_t *char_node_free = NULL;

    pthread_mutex_lock(&list->lock);
    list->list_cleared = true;
    char_node = list->head;

    while (char_node != NULL) {
        char_node_free = char_node;
        char_node = char_node->next;
        free(char_node_free);
    }
    pthread_mutex_unlock(&list->lock);
}

void btmg_char_list_free(char_list_t *list)
{
    if (list != NULL) {
        btmg_char_list_clear(list);
        pthread_mutex_destroy(&list->lock);
        free(list);
    }
}
#endif //#if defined(CONFIG_BT_CENTRAL)
#endif //#if defined(CONFIG_BLEHOST)
