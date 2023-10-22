#include "cmd_util.h"
#include "ctype.h"
#include "btcli_common.h"
#include <bt_manager.h>

#define MAX_LE_CLIENT_CONN 7
#define MAX_ATT_HANDLE 48

typedef struct {
    int handle;
    char *data;
    int data_len;
} HandleData;

typedef struct {
    HandleData handle_data[MAX_ATT_HANDLE];
} Connection;

static Connection conns[MAX_LE_CLIENT_CONN];

void btcli_gatts_attributedata_create(int conn_id, int handle)
{
    if (conn_id >= 0 && conn_id < MAX_LE_CLIENT_CONN) {
        Connection *connection = &(conns[conn_id]);

        HandleData *handle_data = NULL;
        for (int i = 0; i < MAX_ATT_HANDLE; i++) {
            if (connection->handle_data[i].handle <= 0) {
                handle_data = &(connection->handle_data[i]);
                handle_data->handle = handle;
                break;
            }
        }

        if (handle_data != NULL && handle_data->data == NULL) {
            handle_data->data = (char *)malloc(sizeof(char) * GATT_MAX_ATTR_LEN);
            memset(handle_data->data, '\0', sizeof(char) * GATT_MAX_ATTR_LEN);
            handle_data->data_len = 0;
        }
    }
}

char *btcli_gatts_attributedata_read(int conn_id, int handle)
{
    if (conn_id >= 0 && conn_id < MAX_LE_CLIENT_CONN) {
        Connection *connection = &(conns[conn_id]);
        HandleData *handle_data = NULL;
        for (int i = 0; i < MAX_ATT_HANDLE; i++) {
            if (connection->handle_data[i].handle == handle) {
                handle_data = &(connection->handle_data[i]);
                break;
            }
        }

        if (handle_data != NULL && handle_data->data != NULL) {
            return handle_data->data;
        }
    }
    return NULL;
}

int btcli_gatts_attributedata_write(int conn_id, int handle, char *data, int len, int offset)
{
    if (conn_id >= 0 && conn_id < MAX_LE_CLIENT_CONN) {
        Connection *connection = &(conns[conn_id]);

        HandleData *handle_data = NULL;
        for (int i = 0; i < MAX_ATT_HANDLE; i++) {
            if (connection->handle_data[i].handle == handle) {
                handle_data = &(connection->handle_data[i]);
                break;
            }
        }

        if (handle_data != NULL && handle_data->data != NULL) {
            if (offset == 0) {
                memset(handle_data->data, '\0', sizeof(char) * GATT_MAX_ATTR_LEN);
            }
            if (offset + len <= GATT_MAX_ATTR_LEN) {
                memcpy(handle_data->data + offset, data, len);
                handle_data->data_len = offset + len;
                return len;
            }
        }
    }
    return 0;
}

void btcli_gatts_attributedata_memset(int conn_id, int handle)
{
    if (conn_id >= 0 && conn_id < MAX_LE_CLIENT_CONN) {
        Connection *connection = &(conns[conn_id]);

        HandleData *handle_data = NULL;
        for (int i = 0; i < MAX_ATT_HANDLE; i++) {
            if (connection->handle_data[i].handle == handle) {
                handle_data = &(connection->handle_data[i]);
                break;
            }
        }

        if (handle_data != NULL && handle_data->data != NULL) {
            memset(handle_data->data, '\0', sizeof(char) * GATT_MAX_ATTR_LEN);
            handle_data->data_len = 0;
        }
    }
}

int btcli_gatts_attributedata_copy(int conn_id, int handle, char *copydata, int copysize)
{
    if (conn_id >= 0 && conn_id < MAX_LE_CLIENT_CONN) {
        Connection *connection = &(conns[conn_id]);

        HandleData *handle_data = NULL;
        for (int i = 0; i < MAX_ATT_HANDLE; i++) {
            if (connection->handle_data[i].handle == handle) {
                handle_data = &(connection->handle_data[i]);
                break;
            }
        }

        if (handle_data != NULL && handle_data->data != NULL) {
            int srcLength = handle_data->data_len;
            if (copysize >= srcLength) {
                memcpy(copydata, handle_data->data, srcLength);
                return srcLength;
            }
        }
    }

    return 0;
}

void btcli_gatts_attributedata_delete(int conn_id)
{
    if (conn_id >= 0 && conn_id < MAX_LE_CLIENT_CONN) {
        Connection *connection = &(conns[conn_id]);

        for (int i = 0; i < MAX_ATT_HANDLE; i++) {
            HandleData *handle_data = &(connection->handle_data[i]);
            if (handle_data->data != NULL) {
                free(handle_data->data);
                handle_data->data = NULL;
            }
            handle_data->handle = -1;
            handle_data->data_len = 0;
        }
    }
}

void btcli_gatts_all_attributedata_delete()
{
    for (int i = 0; i < MAX_LE_CLIENT_CONN; i++) {
        Connection *connection = &(conns[i]);
        for (int j = 0; j < MAX_ATT_HANDLE; j++) {
            HandleData *handle_data = &(connection->handle_data[j]);
            if (handle_data->data != NULL) {
                free(handle_data->data);
                handle_data->data = NULL;
            }
            handle_data->handle = -1;
            handle_data->data_len = 0;
        }
        memset(connection, 0x00, sizeof(Connection));
    }
}

static enum cmd_status btcli_gatt_help(char *cmd);

static void btcli_print_chrc_props(uint8_t properties)
{
    printf("Properties: ");
    if (properties & BT_GATT_CHRC_BROADCAST) {
        printf("[bcast] ");
    }
    if (properties & BT_GATT_CHRC_READ) {
        printf("[read] ");
    }
    if (properties & BT_GATT_CHRC_WRITE) {
        printf("[write] ");
    }
    if (properties & BT_GATT_CHRC_WRITE_WITHOUT_RESP) {
        printf("[write without rsp] ");
    }
    if (properties & BT_GATT_CHRC_NOTIFY) {
        printf("[notify] ");
    }
    if (properties & BT_GATT_CHRC_INDICATE) {
        printf("[indicate] ");
    }
    if (properties & BT_GATT_CHRC_AUTH) {
        printf("[auth] ");
    }
    if (properties & BT_GATT_CHRC_EXT_PROP) {
        printf("[ext prop] ");
    }
    printf("\n");
}

static void btcli_uuid_to_str(btmg_uuid_t *uuid, char *str, size_t len)
{
    uint32_t tmp1, tmp5;
    uint16_t tmp0, tmp2, tmp3, tmp4;

    switch (uuid->type) {
    case BTMG_UUID_16:
        snprintk(str, len, "%04x", uuid->value.u16);
        break;
    case BTMG_UUID_32:
        snprintk(str, len, "%08x", uuid->value.u32);
        break;
    case BTMG_UUID_128:
        memcpy(&tmp0, &uuid->value.u128[0], sizeof(tmp0));
        memcpy(&tmp1, &uuid->value.u128[2], sizeof(tmp1));
        memcpy(&tmp2, &uuid->value.u128[6], sizeof(tmp2));
        memcpy(&tmp3, &uuid->value.u128[8], sizeof(tmp3));
        memcpy(&tmp4, &uuid->value.u128[10], sizeof(tmp4));
        memcpy(&tmp5, &uuid->value.u128[12], sizeof(tmp5));

        snprintk(str, len, "%08x-%04x-%04x-%04x-%08x%04x", tmp5, tmp4, tmp3, tmp2, tmp1, tmp0);
        break;
    default:
        memset(str, 0, len);
        return;
    }
}

void btcli_gattc_dis_att_cb(gattc_dis_cb_para_t *data)
{
    char uuid_str1[37];
    char uuid_str2[37];

    switch (data->type) {
    case BTMG_DIS_PRIMARY_SERVER:
    case BTMG_DIS_SECONDARY_SERVER:
    case BTMG_DIS_INCLUDE_SERVER:
        btcli_uuid_to_str(&(data->uuid), uuid_str1, sizeof(uuid_str1));
        btcli_uuid_to_str(&(data->server_uuid), uuid_str2, sizeof(uuid_str2));
        printf("start_handle [0x%04x]------------end_handle [0x%04x]\n",
                                    data->start_handle, data->end_handle);
        printf("-------|--------------------------------------------\n");
        printf("0x%04x | Service declaration[%s] | %s\n", data->start_handle, uuid_str1,
            data->type == BTMG_DIS_PRIMARY_SERVER ? "PRIMARY_SERVER" : "SECONDARY_SERVER");
        printf("0x%04x | Service uuid[%s]        | %s\n", data->start_handle+1, uuid_str2,
                !strcmp(uuid_str2, "1800") ?"Generic Access" :
                (!strcmp(uuid_str2, "1801")? "Generic Attribute": "Unknow Service"));
        printf("----------------------------------------------------\n");
        break;
    case BTMG_DIS_CHARACTERISTIC:
        btcli_uuid_to_str(&(data->uuid), uuid_str1, sizeof(uuid_str1));
        btcli_uuid_to_str(&(data->char_uuid), uuid_str2, sizeof(uuid_str2));
        printf("-------|--------------------------------------------\n");
        printf("0x%04x | Characteristic declaration[%s]\n", data->char_handle, uuid_str1);
        printf("0x%04x | Characteristic [%s]\n", data->value_handle, uuid_str2);
        if (data->properties & BT_GATT_CHRC_NOTIFY | data->properties & BT_GATT_CHRC_INDICATE) {
            printf("0x%04x | CCCD[2902]\n", data->value_handle+1);
        }
        btcli_print_chrc_props(data->properties);
        break;
    case BTMG_DIS_ATTRIBUTE:
        btcli_uuid_to_str(&(data->uuid), uuid_str1, sizeof(uuid_str1));
        printf("-------|--------------------------------------------\n");
        printf("0x%04x | Attribute uuid [%s]\n", data->attr_handle, uuid_str1);
        break;
    default:
        break;
    }
}

void btcli_gattc_notify_indicate_cb(gattc_notify_indicate_cb_para_t *data)
{
    char recv_data[data->length + 1];
    memcpy(recv_data, data->value, data->length);
    recv_data[data->length] = '\0';

    CMD_DBG("subscribe recv [handle=0x%04X][len=%d][data:%s]\n", data->value_handle,
              data->length, recv_data);
}

void btcli_gattc_read_cb(gattc_read_cb_para_t *data)
{
    int i;

    if (!data->success) {
        CMD_DBG("gattc read failed:(0x%02x),handle[0x%04x]\n", data->att_ecode, data->handle);
        return;
    }

    printf("\ngattc read handle[0x%04x],", data->handle);

    if (data->length == 0) {
        CMD_DBG(": 0 bytes\n");
        return;
    }

    printf(" [len=%d],value:[", data->length);

    for (i = 0; i < data->length; i++)
        printf("%c", data->value[i]);

    printf("]\n");
}

void btcli_gattc_write_cb(gattc_write_cb_para_t *data)
{
    if (data->success) {
        CMD_DBG("gattc write OK,handle[0x%04x]\n", data->handle);
    } else {
        CMD_DBG("gattc write failed:(0x%02x), handle[0x%04x]\n", data->att_ecode, data->handle);
    }
}

void btcli_gatts_get_db_cb(gatts_get_db_t *data)
{
    char str[37];
    char str2[37];

    btcli_uuid_to_str(&(data->uuid), str, sizeof(str));
    btcli_uuid_to_str(&(data->uuid_value), str2, sizeof(str2));
    printf("[handle=0x%04X],[uuid=%s][uuid2=%s][perm=%d]\n", data->attr_handle, str, str2,
              data->perm);
}

void btcli_gatts_char_read_req_cb(gatts_char_read_req_t *data)
{
    btcli_gatts_attributedata_create(data->conn_id,
                          data->attr_handle); //查找conn_id对于的handle，若data指针为空则创建空值
    char *outdata = btcli_gatts_attributedata_read(data->conn_id, data->attr_handle);
    data->out_len = strlen(outdata);
    memcpy(data->out_data, outdata, data->out_len);

    if (data->offset == 0)
        CMD_DBG("receive char read: [conn_id=%d][handle=0x%04X],[data:%s]\n", data->conn_id,
                data->attr_handle, data->out_data);
}

void btcli_gatts_char_write_req_cb(gatts_char_write_req_t *data)
{
    char recv_data[data->value_len + 1];
    memcpy(recv_data, data->value, data->value_len);
    recv_data[data->value_len] = '\0';

    btcli_gatts_attributedata_create(data->conn_id, data->attr_handle);
    if (data->offset == 0) {
        btcli_gatts_attributedata_memset(data->conn_id, data->attr_handle);
    }
    btcli_gatts_attributedata_write(data->conn_id, data->attr_handle, (char *)recv_data,
                                        data->value_len, data->offset);

    CMD_DBG("receive char write: [conn_id=%d][handle=0x%04X][len=%d][value=%s]\n", data->conn_id,
            data->attr_handle, data->value_len, recv_data);
}

void btcli_gatts_ccc_cfg_cb(gatts_ccc_cfg_t *data)
{
    if (data->value == 0x0) {
        CMD_DBG("Notified and Indicated disabled, CCCD handle:%#x\n", data->attr_handle);
    } else if (data->value == 0x1) {
        CMD_DBG("Notified enabled, Indicated disabled, CCCD handle:%#x\n", data->attr_handle);
    } else if (data->value == 0x2) {
        CMD_DBG("Indicated enabled, Notified disabled, CCCD handle:%#x\n", data->attr_handle);
    } else if (data->value == 0x3) {
        CMD_DBG("Notified and Indicated enabled, CCCD handle:%#x\n", data->attr_handle);
    }
}

void btcli_gatts_indicate_cb(gatts_indicate_cb_t *data)
{
    if (data->success) {
        CMD_DBG("conn_id[%d], Indicated data success\n", data->conn_id);
    } else {
        CMD_ERR("conn_id[%d], Indicated data failed\n", data->conn_id);
    }
}

static btmg_gatt_db_t *db;
int btcli_gatt_register_test_service(void)
{
    if (db != NULL) {
        CMD_ERR("gatt already registered\n");
        return 0;
    }

    btmg_uuid_t uuid;
    btmg_gatt_properties_t prop;
    btmg_gatt_permission_t perm = BTMG_GATT_PERM_READ | BTMG_GATT_PERM_WRITE;

    /* service1 start, uuid=0xABCD */
    db = btmg_gatt_attr_create(12); //CHAR+2 other+1
    uuid.type = BTMG_UUID_16;
    uuid.value.u16 = 0xABCD;
    btmg_gatt_attr_primary_service(db, uuid); // +1

    uuid.value.u16 = 0xfff2;
    prop = BTMG_GATT_CHRC_READ | BTMG_GATT_CHRC_WRITE;
    btmg_gatt_attr_characteristic(db, uuid, prop, perm); // +2

    uuid.value.u16 = 0xfff3;
    prop = BTMG_GATT_CHRC_READ | BTMG_GATT_CHRC_WRITE | BTMG_GATT_CHRC_NOTIFY | BTMG_GATT_CHRC_INDICATE;
    btmg_gatt_attr_characteristic(db, uuid, prop, perm); // +2
    btmg_gatt_attr_ccc(db, perm);                        // +1
    /* service1 end*/
    /* service2 start, uuid=0x7788 */
    uuid.type = BTMG_UUID_16;
    uuid.value.u16 = 0x7788;
    btmg_gatt_attr_primary_service(db, uuid); // +1

    uuid.value.u16 = 0x1122;
    prop = BTMG_GATT_CHRC_READ;
    btmg_gatt_attr_characteristic(db, uuid, prop, perm); // +2

    uuid.value.u16 = 0x3344;
    prop = BTMG_GATT_CHRC_READ | BTMG_GATT_CHRC_WRITE | BTMG_GATT_CHRC_NOTIFY;
    btmg_gatt_attr_characteristic(db, uuid, prop, perm); // +2
    btmg_gatt_attr_ccc(db, perm);                        // +1
    /* service2 end*/

    btmg_gatt_register_service(db);

    return CMD_STATUS_OK;
}

int btcli_gatt_unregister_test_service(void)
{
    if (db == NULL) {
        CMD_ERR("gatt already unregistered\n");
        return -1;
    }

    btmg_gatt_unregister_service(db);
    btcli_gatts_all_attributedata_delete();
    btmg_gatt_attr_destory(db);
    db  = NULL;

    return 0;
}

enum cmd_status btcli_gattc_discover(char *cmd)
{
    int argc;
    char *argv[2];
    uint8_t conn_id = 0;
    btmg_uuid_t *uuid = NULL;
    int dis_tpye = 0;
    uint16_t start_handle = 0x0001;
    uint16_t end_handle = 0xFFFF;

    argc = cmd_parse_argv(cmd, argv, 2);
    if (argc < 1) {
        CMD_ERR("invalid param number %d\n", argc);
        return CMD_STATUS_INVALID_ARG;
    }

    if (!cmd_strcmp(argv[0], "service")) {
            dis_tpye = 1;
    } else if (!cmd_strcmp(argv[0], "char")) {
            dis_tpye = 2;
    } else if (!cmd_strcmp(argv[0], "desc")) {
            dis_tpye = 3;
    }

    if (argc == 2) {
        conn_id = strtoul(argv[1], NULL, 16);
    }

    if (dis_tpye == 0) {
        btmg_gattc_discover_all_services(conn_id, uuid, start_handle, end_handle);
    } else if (dis_tpye == 1) {
        btmg_gattc_discover_primary_services(conn_id, uuid, start_handle, end_handle);
    } else if (dis_tpye == 2) {
        btmg_gattc_discover_characteristic(conn_id);
    } else if (dis_tpye == 3) {
        btmg_gattc_discover_descriptor(conn_id);
    }

    return CMD_STATUS_OK;
}

enum cmd_status btcli_gattc_write(char *cmd)
{
    int argc;
    char *argv[4];
    int err;
    uint16_t handle, conn_id = 0;
    size_t len = 0;
    uint8_t gatt_write_buf[GATT_MAX_ATTR_LEN] = { 0 };

    argc = cmd_parse_argv(cmd, argv, 4);
    if (argc < 1) {
        CMD_ERR("invalid param number %d\n", argc);
        return CMD_STATUS_INVALID_ARG;
    }

    handle = strtoul(argv[0], NULL, 16);

    if (argc >= 3 && !cmd_strcmp(argv[2], "string")) {
        len = MIN(strlen(argv[1]), sizeof(gatt_write_buf));
        strncpy((char *)gatt_write_buf, argv[1], len);
        if (argc == 4) {
            conn_id = strtoul(argv[3], NULL, 16);
        }
    } else {
        len = hex2bin(argv[1], strlen(argv[1]), gatt_write_buf, sizeof(gatt_write_buf));
        if (len == 0) {
            CMD_ERR("No data set\n");
            return CMD_STATUS_INVALID_ARG;
        }
        if (argc == 3) {
            conn_id = strtoul(argv[2], NULL, 16);
        }
    }

    err = btmg_gattc_write(conn_id, handle, gatt_write_buf, len);

    if (err) {
        CMD_ERR("Write failed (err %d)\n", err);
    }

    return CMD_STATUS_OK;
}

enum cmd_status btcli_gattc_read(char *cmd)
{
    int argc;
    char *argv[2];
    int err;
    uint16_t handle, conn_id = 0;

    argc = cmd_parse_argv(cmd, argv, 2);
    if (argc < 1) {
        CMD_ERR("invalid param number %d\n", argc);
        return CMD_STATUS_INVALID_ARG;
    }

    handle = strtoul(argv[0], NULL, 16);

    if (argc == 2) {
        conn_id = strtoul(argv[1], NULL, 16);
    }

    err = btmg_gattc_read(conn_id, handle);

    if (err) {
        CMD_ERR("Read failed (err %d)\n", err);
    }

    return CMD_STATUS_OK;
}

enum cmd_status btcli_gattc_subscribe(char *cmd)
{
    int argc;
    char *argv[2];
    int err;
    uint16_t handle, conn_id = 0;
    bool is_indicate = false;

    argc = cmd_parse_argv(cmd, argv, 2);
    if (argc < 1) {
        CMD_ERR("invalid param number %d\n", argc);
        return CMD_STATUS_INVALID_ARG;
    }

    handle = strtoul(argv[0], NULL, 16);
    if (argc > 1) {
        conn_id = strtoul(argv[1], NULL, 16);
    }

    btmg_gattc_subscribe(conn_id, handle);

    return CMD_STATUS_OK;
}

enum cmd_status btcli_gattc_unsubscribe(char *cmd)
{
    int argc;
    char *argv[2];
    int err;
    uint16_t handle, conn_id = 0;

    argc = cmd_parse_argv(cmd, argv, 2);
    if (argc < 1) {
        CMD_ERR("invalid param number %d\n", argc);
        return CMD_STATUS_INVALID_ARG;
    }

    handle = strtoul(argv[0], NULL, 16);
    if (argc > 1) {
        conn_id = strtoul(argv[1], NULL, 16);
    }

    err = btmg_gattc_unsubscribe(conn_id, handle);

    return CMD_STATUS_OK;
}

enum cmd_status btcli_gatts_show_db(char *cmd)
{
    btmg_gatt_get_db();

    return CMD_STATUS_OK;
}

enum cmd_status btcli_gatts_notify(char *cmd)
{
    int argc;
    char *argv[4];
    int err;
    uint16_t handle, offset, conn_id = 0;
    size_t len = 0;
    uint8_t gatt_write_buf[GATT_MAX_ATTR_LEN] = { 0 };

    argc = cmd_parse_argv(cmd, argv, 4);
    if (argc < 1) {
        CMD_ERR("invalid param number %d\n", argc);
        return CMD_STATUS_INVALID_ARG;
    }

    handle = strtoul(argv[0], NULL, 16);

    if (argc >= 3 && !cmd_strcmp(argv[2], "string")) {
        len = MIN(strlen(argv[1]), sizeof(gatt_write_buf));
        strncpy((char *)gatt_write_buf, argv[1], len);
        if (argc == 4) {
            conn_id = strtoul(argv[3], NULL, 16);
        }
    } else {
        len = hex2bin(argv[1], strlen(argv[1]), gatt_write_buf, sizeof(gatt_write_buf));
        if (len == 0) {
            CMD_ERR("No data set\n");
            return CMD_STATUS_INVALID_ARG;
        }
        if (argc == 3) {
            conn_id = strtoul(argv[2], NULL, 16);
        }
    }

    btcli_gatts_attributedata_create(conn_id, handle); //查找conn_id对于的handle，若data指针为空则创建
    btcli_gatts_attributedata_memset(conn_id, handle);
    btcli_gatts_attributedata_write(conn_id, handle, (char *)gatt_write_buf, len, 0);

    btmg_gatts_notify(conn_id, handle, gatt_write_buf, len);

    return CMD_STATUS_OK;
}

enum cmd_status btcli_gatts_indicate(char *cmd)
{
    int argc;
    char *argv[4];
    int err;
    uint16_t handle, offset, conn_id = 0;
    size_t len = 0;
    uint8_t gatt_write_buf[GATT_MAX_ATTR_LEN] = { 0 };

    argc = cmd_parse_argv(cmd, argv, 4);
    if (argc < 1) {
        CMD_ERR("invalid param number %d\n", argc);
        return CMD_STATUS_INVALID_ARG;
    }

    handle = strtoul(argv[0], NULL, 16);

    if (argc >= 3 && !cmd_strcmp(argv[2], "string")) {
        len = MIN(strlen(argv[1]), sizeof(gatt_write_buf));
        strncpy((char *)gatt_write_buf, argv[1], len);
        if (argc == 4) {
            conn_id = strtoul(argv[3], NULL, 16);
        }
    } else {
        len = hex2bin(argv[1], strlen(argv[1]), gatt_write_buf, sizeof(gatt_write_buf));
        if (len == 0) {
            CMD_ERR("No data set\n");
            return CMD_STATUS_INVALID_ARG;
        }
        if (argc == 3) {
            conn_id = strtoul(argv[2], NULL, 16);
        }
    }

    btcli_gatts_attributedata_create(conn_id, handle);
    btcli_gatts_attributedata_memset(conn_id, handle);
    btcli_gatts_attributedata_write(conn_id, handle, (char *)gatt_write_buf, len, 0);

    btmg_gatts_indicate(conn_id, handle, gatt_write_buf, len);

    return CMD_STATUS_OK;
}

enum cmd_status btcli_gatt_exchange_mtu(char *cmd)
{
    int argc;
    char *argv[1];
    int conn_id = 0;

    argc = cmd_parse_argv(cmd, argv, 1);
    if (argc == 1) {
        conn_id = atoi(argv[0]);
    }

    btmg_le_gatt_mtu_exchange(conn_id);
    CMD_DBG("conn %d mtu_exchange \n", conn_id);

    return CMD_STATUS_OK;
}

enum cmd_status btcli_gatt_get_mtu(char *cmd)
{
    int argc;
    char *argv[1];
    int mtu = 0;
    int conn_id = 0;

    argc = cmd_parse_argv(cmd, argv, 1);

    if (argc == 1) {
        conn_id = atoi(argv[0]);
    }

    mtu = btmg_le_conn_get_mtu(conn_id);
    CMD_DBG("Get conn:%d,MTU=%d\n", conn_id, mtu);

    return CMD_STATUS_OK;
}

static const struct cmd_data gatt_cmds[] = {
    { "discover",       btcli_gattc_discover,     CMD_DESC("[type: service, char, desc] [conn_id]")},
    { "write",          btcli_gattc_write,        CMD_DESC("<handle> <data> [string] [conn_id]")},
    { "read",           btcli_gattc_read,         CMD_DESC("<handle> [conn_id]")},
    { "subscribe",      btcli_gattc_subscribe,    CMD_DESC("<char_handle> [conn_id]")},
    { "unsubscribe",    btcli_gattc_unsubscribe,  CMD_DESC("<char_handle> [conn_id]")},
    { "show",           btcli_gatts_show_db,      CMD_DESC("[none]")},
    { "notify",         btcli_gatts_notify,       CMD_DESC("<handle> <data> [string] [conn_id]")},
    { "indicate",       btcli_gatts_indicate,     CMD_DESC("<handle> <data> [string] [conn_id]")},
    { "exchange_mtu",   btcli_gatt_exchange_mtu,  CMD_DESC("[conn_id]")},
    { "get_mtu",        btcli_gatt_get_mtu,       CMD_DESC("[conn_id]")},
    { "help",           btcli_gatt_help,          CMD_DESC(CMD_HELP_DESC)},
};

/* btcli gatt help */
static enum cmd_status btcli_gatt_help(char *cmd)
{
	return cmd_help_exec(gatt_cmds, cmd_nitems(gatt_cmds), 10);
}

enum cmd_status btcli_gatt(char *cmd)
{
    return cmd_exec(cmd, gatt_cmds, cmd_nitems(gatt_cmds));
}
