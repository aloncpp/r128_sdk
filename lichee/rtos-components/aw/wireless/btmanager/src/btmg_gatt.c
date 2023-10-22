
#include "bt_manager.h"
#include "btmg_common.h"
#include "btmg_gatt_db.h"
#include <errno.h>
#include "ble/bluetooth/gatt.h"
#include "hci_core.h"
#include "ble/bluetooth/conn.h"

#include <zephyr.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <ble/sys/atomic.h>
#include <ble/sys/byteorder.h>
#include <ble/sys/util.h>
#include <ble/sys/slist.h>
#include <ble/sys/__assert.h>
#include "ble/bluetooth/buf.h"
#include <bluetooth/hci.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/att.h>
#include "conn_internal.h"
#include "btmg_log.h"
#include "btmg_ble.h"

static uint8_t gatt_write_buf[GATT_MAX_ATTR_LEN] = { 0 };

extern char_list_t *subscribe_char_list;
static struct bt_gatt_write_params write_param;
static struct bt_gatt_read_params read_param;
static struct bt_gatt_exchange_params exchange_param;

int btmg_le_stack_event_callback(btmg_le_event_t type, void *event, size_t len);

static struct bt_uuid_128 btmg_uuid_to_bt_uuid(btmg_uuid_t *uuid)
{
    struct bt_uuid_128 bt_uuid_tmp;

    memset(&bt_uuid_tmp, 0, sizeof(struct bt_uuid_128));
    if (uuid->type == BTMG_UUID_16) {
        bt_uuid_tmp.uuid.type = BT_UUID_TYPE_16;
        BT_UUID_16(&bt_uuid_tmp)->val = uuid->value.u16;
    } else if (uuid->type == BTMG_UUID_32) {
        bt_uuid_tmp.uuid.type = BT_UUID_TYPE_32;
        BT_UUID_32(&bt_uuid_tmp)->val = uuid->value.u32;
    } else if (uuid->type == BTMG_UUID_128) {
        bt_uuid_tmp.uuid.type = BT_UUID_TYPE_128;
        memcpy(bt_uuid_tmp.val, uuid->value.u128, sizeof(uuid->value.u128));
    }

    return bt_uuid_tmp;
}

static btmg_uuid_t bt_uuid_to_btmg_uuid(struct bt_uuid *uuid)
{
    btmg_uuid_t btmg_uuid;

    if (uuid->type == BT_UUID_TYPE_16) {
        btmg_uuid.value.u16 = BT_UUID_16(uuid)->val;
        btmg_uuid.type = BTMG_UUID_16;
    }
    if (uuid->type == BT_UUID_TYPE_32) {
        btmg_uuid.value.u32 = BT_UUID_32(uuid)->val;
        btmg_uuid.type = BTMG_UUID_32;
    }
    if (uuid->type == BT_UUID_TYPE_128) {
        memcpy(btmg_uuid.value.u128, BT_UUID_128(uuid)->val, sizeof(btmg_uuid.value.u128));
        btmg_uuid.type = BTMG_UUID_128;
    }

    return btmg_uuid;
}

static struct bt_gatt_attr *handle_to_attr(btmg_gatt_db_t *db, uint16_t handle)
{
    return gatt_server_handle_to_attr((ll_stack_gatt_server_t *)&(db->stack_gatt_server), handle);
}

/** @brief gatt 回调函数
 */
static void gattc_write_cb(struct bt_conn *conn, uint8_t err, struct bt_gatt_write_params *params)
{
    gattc_write_cb_para_t event;

    event.success = (err) ? 0 : 1;
    event.att_ecode = err;
    event.handle = -1;
    event.user_data = "\0";
    if (params) {
        event.handle = params->handle;
        event.user_data = (void *)params->data; // '\0'
    }

    btmg_le_stack_event_callback(BTMG_LE_EVENT_GATTC_WRITE_CB, &event, sizeof(event));
}

static uint8_t gattc_read_cb(struct bt_conn *conn, uint8_t err, struct bt_gatt_read_params *params,
                         const void *data, uint16_t length)
{
    if (length == 0) {
        return BT_GATT_ITER_STOP;
    }

    gattc_read_cb_para_t event;
    event.length = length;
    event.att_ecode = err;
    event.success = (err || !data) ? (0) : (1);
    event.user_data = NULL;
    event.value = data;
    event.handle = params->single.handle;
    btmg_le_stack_event_callback(BTMG_LE_EVENT_GATTC_READ_CB, &event, sizeof(event));

    if (err || !data) {
        (void)memset(params, 0, sizeof(*params));
        return BT_GATT_ITER_STOP;
    }

    return BT_GATT_ITER_CONTINUE;
}

static void gatts_indicate_cb(struct bt_conn *conn, struct bt_gatt_indicate_params *params,
                              uint8_t err)
{
    gatts_indicate_cb_t event;
    event.conn_id = bt_conn_index(conn);
    event.success = (err) ? (0) : (1);

    btmg_le_stack_event_callback(BTMG_LE_EVENT_GATTS_INCIDATE_CB, &event, sizeof(event));
}

static ssize_t gatts_write_att_req_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                                      const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
    gatts_char_write_req_t event;
    memset(&event, 0, sizeof(event));
    event.conn_id = bt_conn_index(conn);
    event.attr_handle = attr->handle;
    event.need_rsp = 0;
    event.trans_id = 0;
    event.value = (char *)buf;
    event.value_len = len;
    event.offset = offset;

    btmg_le_stack_event_callback(BTMG_LE_EVENT_GATTS_CHAR_WRITE, &event, sizeof(event));
    return len;
}

static ssize_t gatts_read_att_req_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                                     void *buf, uint16_t len, uint16_t offset)
{
    gatts_char_read_req_t event;
    memset(&event, 0, sizeof(event));
    event.conn_id = bt_conn_index(conn);
    event.attr_handle = attr->handle;
    event.is_blob_req = 0;
    event.offset = offset;
    event.trans_id = 0;

    btmg_le_stack_event_callback(BTMG_LE_EVENT_GATTS_CHAR_READ, &event, sizeof(event));

    return bt_gatt_attr_read(conn, attr, buf, len, offset, event.out_data, event.out_len);
}

static void ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    gatts_ccc_cfg_t event;

    event.attr_handle = attr->handle -1;
    event.value = value;
    btmg_le_stack_event_callback(BTMG_LE_EVENT_GATTS_CCC_CFG_CHANGED, &event, sizeof(event));
}

static uint8_t discover_func(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                             struct bt_gatt_discover_params *params)
{
    struct bt_gatt_service_val *gatt_service;
    struct bt_gatt_chrc *gatt_chrc;
    struct bt_gatt_include *gatt_include;
    gattc_dis_cb_para_t para;
    char uuid_str[BT_UUID_STR_LEN];
    char server_uuid_str[BT_UUID_STR_LEN];

    if (!attr) {
        BTMG_DEBUG("Discover complete");
        (void)memset(params, 0, sizeof(*params));
        return BT_GATT_ITER_STOP;
    }

    switch (params->type) {
    case BT_GATT_DISCOVER_PRIMARY:
    case BT_GATT_DISCOVER_SECONDARY:
    case BT_GATT_DISCOVER_INCLUDE:
        gatt_service = attr->user_data;
        if (params->type == BT_GATT_DISCOVER_PRIMARY) {
            para.type = BTMG_DIS_PRIMARY_SERVER;
        } else if (params->type == BT_GATT_DISCOVER_SECONDARY) {
            para.type = BTMG_DIS_SECONDARY_SERVER;
        } else if (params->type == BT_GATT_DISCOVER_INCLUDE) {
            para.type = BTMG_DIS_INCLUDE_SERVER;
        }
        para.start_handle = attr->handle;
        para.end_handle = gatt_service->end_handle;
        para.conn_id = bt_conn_index(conn);
        para.uuid = bt_uuid_to_btmg_uuid((struct bt_uuid *)(attr->uuid));
        para.server_uuid = bt_uuid_to_btmg_uuid((struct bt_uuid *)(gatt_service->uuid));
        bt_uuid_to_str(gatt_service->uuid, server_uuid_str, sizeof(server_uuid_str));
        bt_uuid_to_str(attr->uuid, uuid_str, sizeof(uuid_str));
        break;
    case BT_GATT_DISCOVER_CHARACTERISTIC:
        gatt_chrc = attr->user_data;
        para.conn_id = bt_conn_index(conn);
        para.type = BTMG_DIS_CHARACTERISTIC;
        para.char_handle = attr->handle;
        para.value_handle = attr->handle + 1;
        para.properties = gatt_chrc->properties;
        para.uuid = bt_uuid_to_btmg_uuid((struct bt_uuid *)(attr->uuid));
        para.char_uuid = bt_uuid_to_btmg_uuid((struct bt_uuid *)(gatt_chrc->uuid)); //chrc uuid
        bt_uuid_to_str(gatt_chrc->uuid, uuid_str, sizeof(uuid_str));
        break;
    case BT_GATT_DISCOVER_ATTRIBUTE:
        para.type = BTMG_DIS_ATTRIBUTE;
        para.uuid = bt_uuid_to_btmg_uuid((struct bt_uuid *)(attr->uuid));
        para.attr_handle = attr->handle;
        bt_uuid_to_str(attr->uuid, uuid_str, sizeof(uuid_str));
        break;
    default:
        break;

    }
    btmg_le_stack_event_callback(BTMG_LE_EVENT_GATTC_DISCOVER, &para, sizeof(para));

    return BT_GATT_ITER_CONTINUE;
}

static uint8_t attr_foreach_func(const struct bt_gatt_attr *attr, uint16_t handle, void *user_data)
{
    return 0;
}

static uint8_t gattc_notify_cb(struct bt_conn *conn, struct bt_gatt_subscribe_params *params,
                               const void *data, uint16_t length)
{
    if (!data) {
        BTMG_DEBUG("(handle:0x%04x) unsubscribe complete!", params->value_handle);
        params->value_handle = 0U;
        return BT_GATT_ITER_STOP;
    }

    gattc_notify_indicate_cb_para_t event;
    event.value = data;
    event.length = length;
    event.value_handle = params->value_handle;
    btmg_le_stack_event_callback(BTMG_LE_EVENT_GATTC_NOTIFY, &event, sizeof(event));

    return BT_GATT_ITER_CONTINUE;
}

static void gattc_subscribe_status_cb(struct bt_conn *conn, uint8_t err,
                                      struct bt_gatt_write_params *params)
{
    if (params != NULL) {
        BTMG_DEBUG("(handle:0x%04x) subscribe complete!", params->handle);
    }
}

/** @brief 创建database
 *  @param num_attr: num of attr
 *  @return db: database malloc.
 */
btmg_gatt_db_t *btmg_gatt_service_db_create(int num_attr)
{
    btmg_gatt_db_t *db = malloc(sizeof(btmg_gatt_db_t));
    if (db == NULL) {
        return NULL;
    }
    memset(db, 0, sizeof(btmg_gatt_db_t));

    int ret = ll_gatt_attr_create(&(db->stack_gatt_server), num_attr);
    if (ret != 0) {
        free(db);
        return NULL;
    }
    return db;
}

/** @brief 创建database
 *  @param num_attr: num of attr
 *  @return db: database malloc.
 */
btmg_gatt_db_t *btmg_gatt_attr_create(int num_attr)
{
    return btmg_gatt_service_db_create(num_attr);
}

/** @brief 销毁database 需应用手动设置为NULL
 *  @param db: database ptr
 *  @return 0
 */
btmg_err btmg_gatt_attr_destory(btmg_gatt_db_t *db)
{
    db->stack_gatt_server.base.destroy(&(db->stack_gatt_server.base));
    if (db) {
        free(db);
    }
    return BT_OK;
}

/**
 * @brief 添加服务
 * @param db: database ptr
 * @param service_uuid: service uuid
 * @return 0 in case of success or negative value in case of error.
 */
btmg_err btmg_gatt_attr_primary_service(btmg_gatt_db_t *db, btmg_uuid_t service_uuid)
{
    ll_bt_gatt_attr_t param;

    param.type = GATT_ATTR_SERVICE;
    struct bt_uuid_128 tmp_uuid = btmg_uuid_to_bt_uuid(&service_uuid);
    param.attr.user_data = &tmp_uuid;

    db->stack_gatt_server.base.add(&(db->stack_gatt_server.base), &param);

    return BT_OK;
}

/**
 * @brief 添加特征值
 * @param db: database ptr
 * @param uuid: character uuid
 * @param props: character properties
 * @param perm: character perm
 * @return 0 in case of success or negative value in case of error.
 */
btmg_err btmg_gatt_attr_characteristic(btmg_gatt_db_t *db, btmg_uuid_t uuid,
                                       btmg_gatt_properties_t props, btmg_gatt_permission_t perm)
{
    ll_bt_gatt_attr_t param;

    param.type = GATT_ATTR_CHARACTERISTIC;
    param.properties = props;
    struct bt_uuid_128 tmp_uuid = btmg_uuid_to_bt_uuid(&uuid);
    param.attr.uuid = (struct bt_uuid *)(&tmp_uuid);
    param.attr.perm = perm;
    param.attr.read = gatts_read_att_req_cb;
    param.attr.write = gatts_write_att_req_cb;
    param.attr.user_data = NULL;
    db->stack_gatt_server.base.add(&(db->stack_gatt_server.base), &param);

    return BT_OK;
}

/**
 * @brief 添加特征用户配置
 * @param db: database ptr
 * @param perm: character perm
 * @return 0 in case of success or negative value in case of error.
 */
btmg_err btmg_gatt_attr_ccc(btmg_gatt_db_t *db, btmg_gatt_permission_t perm)
{
    ll_bt_gatt_attr_t param;

    param.type = GATT_ATTR_CCC;
    param.attr.perm = perm;
    param.attr.user_data = ccc_cfg_changed;
    db->stack_gatt_server.base.add(&(db->stack_gatt_server.base), &param);

    return BT_OK;
}

/**
 * @brief 注册服务
 * @param db: database ptr
 * @return 0 in case of success or negative value in case of error.
 */
btmg_err btmg_gatt_register_service(btmg_gatt_db_t *db)
{
    return bt_gatt_service_register(&(db->stack_gatt_server.service));
}

/**
 * @brief 取消注册服务
 * @param db: database ptr
 * @return 0 in case of success or negative value in case of error.
 */
btmg_err btmg_gatt_unregister_service(btmg_gatt_db_t *db)
{
    return bt_gatt_service_unregister(&(db->stack_gatt_server.service));
}

static struct db_stats {
    uint16_t svc_count;
    uint16_t attr_count;
    uint16_t chrc_count;
    uint16_t ccc_count;
} stats;

static uint8_t print_attr(const struct bt_gatt_attr *attr, uint16_t handle, void *user_data)
{
    char str[BT_UUID_STR_LEN];

    stats.attr_count++;

    if (!bt_uuid_cmp(attr->uuid, BT_UUID_GATT_PRIMARY) ||
        !bt_uuid_cmp(attr->uuid, BT_UUID_GATT_SECONDARY)) {
        stats.svc_count++;
    }
    if (!bt_uuid_cmp(attr->uuid, BT_UUID_GATT_CHRC)) {
        stats.chrc_count++;
    }
    if (!bt_uuid_cmp(attr->uuid, BT_UUID_GATT_CCC) && attr->write == bt_gatt_attr_write_ccc) {
        stats.ccc_count++;
    }
    bt_uuid_to_str(attr->uuid, str, sizeof(str));
    //printf("attr %p handle 0x%04x uuid %s perm 0x%02x \n", attr, handle, str, attr->perm);

    gatts_get_db_t event;
    event.attr_handle = handle;
    event.uuid = bt_uuid_to_btmg_uuid((struct bt_uuid *)attr->uuid);
    // event.uuid_value = bt_uuid_to_btmg_uuid((struct bt_uuid *)attr->user_data);
    event.perm = attr->perm;
    btmg_le_stack_event_callback(BTMG_LE_EVENT_GATTS_GET_DB, &event, sizeof(event));
    return BT_GATT_ITER_CONTINUE;
}

/**
 *  @brief 遍历服务,按照att条目遍历database，注册后可以获得handle
 *  @param db: database ptr
 *  @param fun: callback fun ptr
 *  @param len: buffer len
 *  @return 0 in case of success or negative value in case of error.
 */
btmg_err btmg_gatt_get_db(void)
{
    size_t total_len;

    memset(&stats, 0, sizeof(stats));
    bt_gatt_foreach_attr(0x0001, 0xffff, print_attr, NULL);

    if (!stats.attr_count) {
        BTMG_WARNG("No attribute found");
        return BT_FAIL;
    }

    total_len = stats.svc_count * sizeof(struct bt_gatt_service);
    total_len += stats.chrc_count * sizeof(struct bt_gatt_chrc);
    total_len += stats.attr_count * sizeof(struct bt_gatt_attr);
    total_len += stats.ccc_count * sizeof(struct _bt_gatt_ccc);

    printf("=================================================\n");
    printf("Total: %u services %u attributes (%u bytes)\n", stats.svc_count, stats.attr_count,
               total_len);

    return BT_OK;
}

typedef struct {
    struct bt_conn *conn;
    uint8_t *data;
    int len;
    int err;
} ble_attr_foreach_do_t;

static uint8_t btmg_gatts_attr_notify(const struct bt_gatt_attr *attr, uint16_t handle,
                                      void *user_data)
{
    ble_attr_foreach_do_t *notify = (ble_attr_foreach_do_t *)user_data;
    notify->err = bt_gatt_notify(notify->conn, attr, notify->data, notify->len);

    return BT_GATT_ITER_STOP;
}

static uint8_t btmg_gatts_attr_indicate(const struct bt_gatt_attr *attr, uint16_t handle,
                                        void *user_data)
{
    ble_attr_foreach_do_t *indi = (ble_attr_foreach_do_t *)user_data;
    static struct bt_gatt_indicate_params param = { 0 };
    param.attr = attr;
    param.func = gatts_indicate_cb;
    param.data = (void *)indi->data;
    param.len = indi->len;

    indi->err = bt_gatt_indicate(indi->conn, &param);

    return BT_GATT_ITER_STOP;
}

/**
 * @param conn_id: connections identity
 * @param char_handle: character handle
 * @param data: buffer
 * @param len: buffer len
 * @return 0 in case of success or negative value in case of error.
 */
btmg_err btmg_gatts_notify(uint8_t conn_id, uint16_t char_handle, uint8_t *data, size_t len)
{
    ble_attr_foreach_do_t notify;
    struct bt_conn *conn;

    if (data == NULL || len == 0) {
        return BT_ERR_INVALID_ARG;
    }

    conn = bt_conn_lookup_index(conn_id);

    if (conn == NULL) {
        BTMG_ERROR("no connection");
        return BT_FAIL;
    }

    // The interface does not copy data, it is managed by btmanager
    memset(gatt_write_buf, '\0', sizeof(gatt_write_buf));
    memcpy(gatt_write_buf, (char *)data, len);
    notify.conn = conn;
    notify.data = gatt_write_buf;
    notify.len = len;
    notify.err = 0;

    bt_gatt_foreach_attr(char_handle, char_handle, btmg_gatts_attr_notify, &notify);

    if (notify.conn) {
        bt_conn_unref(notify.conn);
    }

    return notify.err;
}

/**
 * @param conn_id: connections identity
 * @param handle: character handle
 * @param data: buffer
 * @param len: buffer len
 * @return 0 in case of success or negative value in case of error.
 */
btmg_err btmg_gatts_indicate(uint8_t conn_id, uint16_t char_handle, uint8_t *data, uint16_t len)
{
    ble_attr_foreach_do_t indi;
    struct bt_conn *conn;

    if (data == NULL || len == 0) {
        return BT_ERR_INVALID_ARG;
    }

    conn = bt_conn_lookup_index(conn_id);

    if (conn == NULL) {
        BTMG_ERROR("no connection");
        return BT_FAIL;
    }

    // The interface does not copy data, it is managed by btmanager
    memset(gatt_write_buf, '\0', sizeof(gatt_write_buf));
    memcpy(gatt_write_buf, (char *)data, len);
    indi.conn = conn;
    indi.data = gatt_write_buf;
    indi.len = len;
    indi.err = 0;

    bt_gatt_foreach_attr(char_handle, char_handle, btmg_gatts_attr_indicate, &indi);

    if (indi.conn) {
        bt_conn_unref(indi.conn);
    }

    return indi.err;
}

/**
 * @param conn_id: connections identity
 * @param handle: character handle
 * @param buf: buffer
 * @param len: buffer len
 * @param offset: read offset
 * @return 0 in case of success or negative value in case of error.
 */
// int btmg_gatts_read_rsp(uint8_t conn_id, uint16_t handle, void *buf, size_t len, size_t offset)
// {
// 	return 0;
// }

/**
 * @param conn_id: connections identity
 * @param handle: character handle
 * @param buf: buffer
 * @param len: buffer len
 * @param offset: read offset
 * @return 0 in case of success or negative value in case of error.
 */
// int btmg_gatts_write_rsp(uint8_t conn_id, uint16_t handle, void *buf, size_t len, size_t offset)
// {
// 	return 0;
// }

/**
 * @param conn_id: connections identity
 * @param uuid: att uuid, set to NULL
 * @param start_handle: start handle
 * @param end_handle: end handle
 * @param type: discover type
 * @return 0 in case of success or negative value in case of error.
 */
static btmg_err btmg_gattc_discover_by_type(uint8_t conn_id, btmg_uuid_t *uuid, uint16_t start_handle,
                                       uint16_t end_handle, uint8_t type)
{
    int err = 0;
    static struct bt_gatt_discover_params discover_params;

    struct bt_conn *conn = bt_conn_lookup_index(conn_id);
    if (conn == NULL) {
        BTMG_ERROR("no connection");
        return BT_FAIL;
    }

    discover_params.start_handle = start_handle;
    discover_params.end_handle = end_handle;
    discover_params.type = type;
    discover_params.func = discover_func;

    struct bt_uuid_128 bt_uuid_tmp;
    if (uuid == NULL) {
        discover_params.uuid = NULL;
    } else {
        bt_uuid_tmp = btmg_uuid_to_bt_uuid(uuid);
        discover_params.uuid = (struct bt_uuid *)(&bt_uuid_tmp);
    }
    err = bt_gatt_discover(conn, &discover_params);
    bt_conn_unref(conn);
    if (err) {
        return BT_FAIL;
    }

    return BT_OK;
}

/**
 * @param conn_id: connections identity
 * @param uuid: service uuid
 * @param start_handle: start handle
 * @param end_handle: end handle
 * @return 0 in case of success or negative value in case of error.
 */
btmg_err btmg_gattc_discover_all_services(uint8_t conn_id, btmg_uuid_t *uuid, uint16_t start_handle,
                                     uint16_t end_handle)
{
    return btmg_gattc_discover_by_type(conn_id, uuid, start_handle, end_handle,
                                       BT_GATT_DISCOVER_ATTRIBUTE);
}

/**
 * @param conn_id: connections identity
 * @param uuid: service uuid
 * @param start_handle: start handle
 * @param end_handle: end handle
 * @return 0 in case of success or negative value in case of error.
 */
btmg_err btmg_gattc_discover_primary_services(uint8_t conn_id, btmg_uuid_t *uuid, uint16_t start_handle,
                                         uint16_t end_handle)
{
    return btmg_gattc_discover_by_type(conn_id, uuid, start_handle, end_handle,
                                       BT_GATT_DISCOVER_PRIMARY);
}

/**
 * @param conn_id: connections identity
 * @return 0 in case of success or negative value in case of error.
*/
btmg_err btmg_gattc_discover_characteristic(uint8_t conn_id)
{
    return btmg_gattc_discover_by_type(conn_id, NULL, (uint16_t)0x0001, (uint16_t)0xFFFF,
                                       BT_GATT_DISCOVER_CHARACTERISTIC);
}

/**
 * @param conn_id: connections identity
 * @return 0 in case of success or negative value in case of error.
*/
btmg_err btmg_gattc_discover_descriptor(uint8_t conn_id)
{
    return btmg_gattc_discover_by_type(conn_id, NULL, (uint16_t)0x0001, (uint16_t)0xFFFF,
                                       BT_GATT_DISCOVER_DESCRIPTOR);
}

/**
 * @param conn_id: connections identity
 * @param char_handle: char handle
 * @return 0 in case of success or negative value in case of error.
 */
btmg_err btmg_gattc_read(uint8_t conn_id, uint16_t char_handle)
{
    return btmg_gattc_read_long(conn_id, char_handle, 0);
}

/**
 * @param conn_id: connections identity
 * @param char_handle: char handle
 * @param offset: offset
 * @return 0 in case of success or negative value in case of error.
*/
btmg_err btmg_gattc_read_long(uint8_t conn_id, uint16_t char_handle, int offset)
{
    int err = 0;
    struct bt_conn *conn = bt_conn_lookup_index(conn_id);

    if (conn == NULL) {
        BTMG_ERROR("no connection");
        return BT_FAIL;
    }

    read_param.func = gattc_read_cb;
    read_param.handle_count = 1;
    read_param.single.handle = char_handle;
    read_param.single.offset = offset;

    err = bt_gatt_read(conn, &read_param);
    if (err) {
        BTMG_ERROR("Read failed (err %d)", err);
    }

    bt_conn_unref(conn);
    if (err) {
        return err;
    }

    return BT_OK;
}

/**
 * @param conn_id: connections identity
 * @param char_handle: char handle
 * @param value: value
 * @param len: len
 * @return 0 in case of success or negative value in case of error.
 */
btmg_err btmg_gattc_write(uint8_t conn_id, int char_handle, uint8_t *value, size_t len)
{
    return btmg_gattc_write_long(conn_id, 0, char_handle, value, len, 0);
}

/**
 * @param conn_id: connections identity
 * @param reliable_writes: reliable_writes or not
 * @param char_handle: char handle
 * @param value: value
 * @param len: len
 * @return 0 in case of success or negative value in case of error.
 */
btmg_err btmg_gattc_write_long(uint8_t conn_id, bool reliable_writes, int char_handle,
                               uint8_t *value, uint16_t len, uint16_t offset)
{
    int err = 0;
    struct bt_conn *conn = bt_conn_lookup_index(conn_id);
    if (conn == NULL) {
        BTMG_ERROR("no connection");
        return BT_FAIL;
    }

    memset(gatt_write_buf + offset, '\0', sizeof(gatt_write_buf) - offset);
    memcpy(gatt_write_buf + offset, value, len); //offset
    write_param.func = gattc_write_cb;
    write_param.data = gatt_write_buf;
    write_param.length = len;
    write_param.handle = char_handle;
    write_param.offset = offset;

    err = bt_gatt_write(conn, &write_param);
    if (err) {
        BTMG_ERROR("Write failed (err %d)", err);
    }

    bt_conn_unref(conn);

    if (err) {
        return err;
    }

    return BT_OK;
}

/**
 * @param conn_id: connections identity
 * @param signed_write: signed_write or not
 * @param char_handle: char handle
 * @param value: value
 * @param len: len
 * @return 0 in case of success or negative value in case of error.
*/
btmg_err btmg_gattc_write_without_response(uint8_t conn_id, bool signed_write, uint16_t char_handle,
                                      uint8_t *value, uint16_t len)
{
    int err = 0;
    struct bt_conn *conn = bt_conn_lookup_index(conn_id);

    if (conn == NULL) {
        BTMG_ERROR("no connection");
        return BT_FAIL;
    }
    err = bt_gatt_write_without_response_cb(conn, char_handle, value, len, signed_write, NULL,
                                            NULL);
    bt_conn_unref(conn);
    if (err) {
        return err;
    }

    return BT_OK;
}

/**
 * @param conn_id: connections identity
 * @param signed_write: signed_write or not
 * @param char_handle: char handle
 * @param value: value
 * @param len: len
 * @return 0 in case of success or negative value in case of error.
 */
btmg_err btmg_gattc_write_long_without_response(uint8_t conn_id, bool signed_write, int char_handle,
                                           uint8_t *value, uint16_t len)
{
    return btmg_gattc_write_without_response(conn_id, signed_write, char_handle, value, len);
}

/**
 * @param conn_id: connections identity
 * @param char_handle: char handle
 * @return 0 in case of success or negative value in case of error.
 */
btmg_err btmg_gattc_subscribe(uint8_t conn_id, uint16_t value_handle)
{
    int err;
    char_node_t *char_node = NULL;
    subscribe_params *param_ptr = NULL ;
    struct bt_conn *conn = NULL;

    char_node = btmg_char_list_find_node(subscribe_char_list, conn_id, value_handle);

    if (char_node == NULL) {
        param_ptr = malloc(sizeof(struct bt_gatt_subscribe_params));
         if (param_ptr == NULL) {
            return BT_ERR_NO_MEMORY;
         }
    } else {
        param_ptr = char_node->sub_param;
        if (param_ptr->value_handle) {
            BTMG_ERROR("subscription to 0x%04x already exists", param_ptr->value_handle);
            return BT_FAIL;
        }
    }

    conn = bt_conn_lookup_index(conn_id);
    if (conn == NULL) {
        BTMG_ERROR("no connection");
        free(param_ptr);
        return BT_FAIL;
    }

    memset(param_ptr, 0, sizeof(struct bt_gatt_subscribe_params));

    param_ptr->ccc_handle = value_handle + 1;
    param_ptr->value_handle = value_handle;
    param_ptr->value = BT_GATT_CCC_NOTIFY | BT_GATT_CCC_INDICATE;
    param_ptr->notify = gattc_notify_cb;
    param_ptr->write = gattc_subscribe_status_cb;

#if defined(CONFIG_BT_GATT_AUTO_DISCOVER_CCC)
        param_ptr->ccc_handle == 0;
        static struct bt_gatt_discover_params disc_params;
        param_ptr->disc_params = &disc_params;
        param_ptr->end_handle = 0xFFFF;

#endif /* CONFIG_BT_GATT_AUTO_DISCOVER_CCC */

    err = bt_gatt_subscribe(conn, param_ptr);
    if (err) {
        param_ptr->value_handle = 0U;
        BTMG_ERROR("Subscribe failed (err %d)", err);
        bt_conn_unref(conn);
        return BT_FAIL;
    }
    bt_conn_unref(conn);

    btmg_char_list_add_node(subscribe_char_list, conn_id, value_handle, param_ptr);

    return err;
}

/**
 * @param conn_id: connections identity
 * @param char_handle: char handle
 * @return 0 in case of success or negative value in case of error.
 */
btmg_err btmg_gattc_unsubscribe(uint8_t conn_id, int char_handle)
{
    int err;
    char_node_t *char_node;

    if (subscribe_char_list == NULL) {
        return BT_FAIL;
    }
    struct bt_conn *conn = bt_conn_lookup_index(conn_id);

    if (conn == NULL) {
        BTMG_ERROR("no connection");
        return BT_FAIL;
    }

    char_node = btmg_char_list_find_node(subscribe_char_list, conn_id, char_handle);
    if (char_node == NULL) {
        return BT_FAIL;
    }

    err = bt_gatt_unsubscribe(conn, char_node->sub_param);
    if (err) {
        BTMG_ERROR("unsubscribe fail,err %d", err);
        bt_conn_unref(conn);
        return BT_FAIL;

    }

    bt_conn_unref(conn);

    btmg_char_list_remove_node(subscribe_char_list, conn_id, char_handle);

    return BT_OK;
}

static void mtu_exchange_cb(struct bt_conn *conn, uint8_t err,
                            struct bt_gatt_exchange_params *params)
{
    gatt_exchange_mtu_cb_para_t event;

    memset(&event, 0, sizeof(event));
    event.conn_id = bt_conn_index(conn);
    event.err = err;

    int mtu = bt_gatt_get_mtu(conn);
    BTMG_DEBUG("MTU size: %d, err = %d", mtu, err);

    btmg_le_stack_event_callback(BTMG_LE_EVENT_GATTC_EXCHANGE_MTU_CB, &event, sizeof(event));
}

int btmg_le_gatt_mtu_exchange(uint8_t conn_id)
{

    int ret;
    struct bt_conn *conn = bt_conn_lookup_index(conn_id);

    if (conn == NULL) {
        BTMG_ERROR("no connection");
        return BT_FAIL;
    }

    exchange_param.func = mtu_exchange_cb;
    ret = bt_gatt_exchange_mtu(conn, &exchange_param);

    if (conn) {
        bt_conn_unref(conn);
    }

    return ret;
}

int btmg_le_conn_get_mtu(uint8_t conn_id)
{
    uint16_t mtu;
    struct bt_conn *conn = bt_conn_lookup_index(conn_id);

    if (conn == NULL) {
        BTMG_ERROR("no connection");
        return 0;
    }

    mtu = bt_gatt_get_mtu(conn);
    BTMG_DEBUG("MTU size: %d", mtu);
    bt_conn_unref(conn);

    return mtu;
}
