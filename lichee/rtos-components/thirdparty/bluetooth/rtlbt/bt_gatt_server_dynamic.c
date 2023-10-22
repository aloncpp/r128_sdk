#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "bt_gatt_server_dynamic.h"
#include "bt_le_peripheral.h"
#include "customer_api.h"
#include "trace_app.h"
#include "os_mem.h"
#include "app_flags.h"
#include "customer_api.h"
#include "gap_conn_le.h"
#include "gap_adv.h"
#include "app_flags.h"

typedef struct
{
    int32_t         server_if;     //from app
    int32_t         service_id;     //from stack
    int32_t         srvc_handle;    //maybe chosen here
    int32_t         total_number;
    int32_t         used_count;
    T_ATTRIB_APPL   *attr_tbl;
} BT_GATT_SERVER_TABLE_ALI_T;

#define UNKNOWN_SERVICE_ID  0xFFFFFFFF
#define BT_GATT_SERVER_DYNAMIC_START_HANDLE 0x0020

typedef struct
{
    int32_t     conn_id;
    uint32_t    trans_id;
    int32_t     attr_handle;
    uint32_t    flag;
    bool        in_use;

} BT_GATT_TRANS_INFO;

#define TRANS_FLAG_IDLE     0x00000000
#define TRANS_FLAG_READ     0x00000001
#define TRANS_FLAG_WRITE    0x00000002

static BT_GATT_SERVER_TABLE_ALI_T bt_gatt_srvc_tbl[BT_GATT_SERVER_MAX_NUM];
static uint8_t cur_service_index = 0;
static uint16_t cur_available_start_handle = BT_GATT_SERVER_DYNAMIC_START_HANDLE;


BT_GATT_TRANS_INFO trans_info_tbl[APP_MAX_LINKS];



static void bt_gatt_server_dynamic_srvc_add_cb(int32_t server_if, char *uuid,
                                               int32_t is_primary, int32_t srvc_handle);

static void bt_gatt_server_dynamic_char_add_cb(int32_t server_if, char *uuid,
                                               int32_t srvc_handle, int32_t char_handle);

static void bt_gatt_server_dynamic_desc_add_cb(int32_t server_if, char *uuid,
                                               int32_t srvc_handle, int32_t desc_handle);

static void bt_gatt_server_conn_state_cb(uint8_t conn_id,
                                         T_GAP_CONN_STATE new_state, uint16_t disc_cause);

//big endian
static uint8_t str2uuid(char *str, uint8_t *uuid)
{
    //only 16bit for now
    uint16_t uuid16;
    char *endptr = NULL;

    uuid16 = strtol(str, &endptr, 16);

    if (endptr && (*endptr == '\0' || *endptr == '-'))
    {
        uuid[0] = LO_WORD(uuid16);
        uuid[1] = HI_WORD(uuid16);
        return 2;
    }
    return 0;
}

/*
static bool uuid2str(uint8_t* uuid, char* str, int n)
{
    if ((str == NULL) || (uuid == NULL))
    {
        APP_PRINT_ERROR0("uuid2str: error in params");
        return false;
    }

    //only 16bit for now
    snprintf(str, n, "%02x%02x", uuid[1], uuid[0]);

    return true;
}
*/


static bool get_srvc_tbl_index_by_server_if(int32_t server_if, uint8_t *index)
{
    int i;
    for (i = 0; i < BT_GATT_SERVER_MAX_NUM; i++)
    {
        if ((bt_gatt_srvc_tbl[i].server_if == server_if) &&
            (bt_gatt_srvc_tbl[i].total_number > 0))
        {
            APP_PRINT_INFO1("get_srvc_tbl_index_by_server_if: %d", i);
            *index = i;
            return true;
        }
    }

    APP_PRINT_ERROR0("get_srvc_tbl_index_by_server_if: failed");

    return false;
}

static bool get_srvc_tbl_index_by_service_id(int32_t service_id, uint8_t *index)
{
    int i;
    for (i = 0; i < BT_GATT_SERVER_MAX_NUM; i++)
    {
        if (bt_gatt_srvc_tbl[i].service_id == service_id)
        {
            APP_PRINT_INFO1("get_srvc_tbl_index_by_service_id: %d", i);
            *index = i;
            return true;
        }
    }

    APP_PRINT_ERROR0("get_srvc_tbl_index_by_service_id: failed");

    return false;
}

static bool get_srvc_tbl_index_by_attr_handle(int32_t attr_handle, uint8_t *index)
{
    int i;
    int32_t start_handle, end_handle;

    for (i = 0; i < BT_GATT_SERVER_MAX_NUM; i++)
    {
        if (bt_gatt_srvc_tbl[i].total_number != 0)
        {
            start_handle = bt_gatt_srvc_tbl[i].srvc_handle;
            end_handle = start_handle + bt_gatt_srvc_tbl[i].total_number - 1;

            if ((attr_handle >= start_handle) && (attr_handle <= end_handle))
            {
                APP_PRINT_INFO1("get_srvc_tbl_index_by_attr_handle: %d", i);
                return true;
            }
        }

    }

    APP_PRINT_ERROR0("get_srvc_tbl_index_by_attr_handle: failed");

    return false;
}

static bool get_available_trans_tbl_entry(uint8_t *index)
{
    int i;

    for (i = 0; i < APP_MAX_LINKS; i++)
    {
        if (trans_info_tbl[i].in_use == false)
        {
            APP_PRINT_INFO1("get_available_trans_tbl_entry: %d", i);
            trans_info_tbl[i].in_use = true;
            *index = i;
            return true;
        }
    }

    APP_PRINT_ERROR0("get_available_trans_tbl_entry failed");
    return false;
}

static bool release_trans_tbl_entry(uint8_t index)
{
    if (index >= APP_MAX_LINKS)
    {
        APP_PRINT_ERROR1("release_trans_tbl_entry: invalide index %d", index);
        return false;
    }

    trans_info_tbl[index].conn_id = 0;
    trans_info_tbl[index].trans_id = 0;
    trans_info_tbl[index].attr_handle = 0;
    trans_info_tbl[index].flag = TRANS_FLAG_IDLE;
    trans_info_tbl[index].in_use = false;

    return true;
}

static bool get_trans_tbl_index_by_conn_id(int32_t conn_id, uint8_t *index)
{
    int i;
    for (i = 0; i < APP_MAX_LINKS; i++)
    {
        if ((trans_info_tbl[i].conn_id == conn_id) && (true == trans_info_tbl[i].in_use))
        {
            APP_PRINT_INFO1("get_trans_info_index_by_conn_id: %d", i);
            *index = i;
            return true;
        }
    }

    APP_PRINT_ERROR0("get_trans_info_index_by_conn_id: failed");

    return false;
}

static uint16_t get_char_val_perm_from_char_prop(uint8_t prop)
{
    uint16_t perm = GATT_PERM_NONE;

    if (prop & GATT_CHAR_PROP_READ)
    {
        perm |= GATT_PERM_READ;
    }

    if (prop & (GATT_CHAR_PROP_WRITE_NO_RSP |
                GATT_CHAR_PROP_WRITE | GATT_CHAR_PROP_WRITE_AUTHEN_SIGNED))
    {
        perm |= GATT_PERM_WRITE;
    }

    if (prop & (GATT_CHAR_PROP_NOTIFY | GATT_CHAR_PROP_INDICATE))
    {
        perm |= GATT_PERM_NOTIF_IND;
    }

    return perm;
}

static bool set_attr(T_ATTRIB_APPL *attr, uint16_t flags, uint8_t *type_val,
                     uint8_t type_val_len, uint16_t val_size,
                     char *val_context, uint32_t perm)
{
    int i;

    if ((attr == NULL) || (type_val == NULL) || (type_val_len > 16) ||
        ((val_context != NULL) && (val_size < 1)))
    {
        APP_PRINT_ERROR0("set_attr: input param error");
        return false;
    }

    attr->flags = flags;

    for (i = 0; i < type_val_len; i++)
    {
        attr->type_value[i] = type_val[i];
    }

    attr->value_len = val_size;

    if (val_context != NULL)
    {
        attr->p_value_context = os_mem_zalloc(RAM_TYPE_DATA_ON, val_size);
        if (attr->p_value_context == NULL)
        {
            APP_PRINT_ERROR0("set_attr: failed to alloc for p_value_context");
            return false;
        }

        memcpy(attr->p_value_context, val_context, val_size);
    }

    attr->permissions = perm;

    return true;
}

static bool set_service_attr(T_ATTRIB_APPL *attr, uint16_t type,
                             uint8_t *val, uint16_t val_size, uint32_t perm)
{
    uint8_t type_val[4] = {LO_WORD(type), HI_WORD(type)};

    if (val_size == UUID_16BIT_SIZE)
    {
        type_val[2] = val[0];
        type_val[3] = val[1];

        return set_attr(attr, ATTRIB_FLAG_VALUE_INCL | ATTRIB_FLAG_LE,
                        type_val, 4, UUID_16BIT_SIZE, NULL, perm);
    }
    else if (val_size == UUID_128BIT_SIZE)
    {
        return set_attr(attr, ATTRIB_FLAG_VOID | ATTRIB_FLAG_LE,
                        type_val, 2, UUID_128BIT_SIZE, (void *)val, perm);
    }
    else
    {
        APP_PRINT_ERROR0("set_service_attr: length other than 16 or 128 bit is not supported now");
        return false;
    }
}

static bool set_char_attr(T_ATTRIB_APPL *attr, uint8_t prop, uint32_t perm)
{
    uint8_t type_val[3] = {LO_WORD(GATT_UUID_CHARACTERISTIC),
                           HI_WORD(GATT_UUID_CHARACTERISTIC), prop
                          };

    return set_attr(attr, ATTRIB_FLAG_VALUE_INCL, type_val, 3, 1, NULL, perm);
}

//char value in app layer
static bool set_char_val_attr(T_ATTRIB_APPL *attr, uint8_t *uuid,
                              uint8_t uuid_len, uint32_t perm)
{
    uint16_t flags = ATTRIB_FLAG_VALUE_APPL;

    if (uuid_len == UUID_128BIT_SIZE)
    {
        flags |= ATTRIB_FLAG_UUID_128BIT;
    }

    return set_attr(attr, flags, uuid, uuid_len, 0, NULL, perm);
}

static bool set_cccd_attr(T_ATTRIB_APPL *attr)
{
    uint16_t flags = ATTRIB_FLAG_VALUE_INCL | ATTRIB_FLAG_CCCD_APPL;

    uint8_t type_val[4] =
    {
        LO_WORD(GATT_UUID_CHAR_CLIENT_CONFIG),
        HI_WORD(GATT_UUID_CHAR_CLIENT_CONFIG),
        LO_WORD(GATT_CLIENT_CHAR_CONFIG_DEFAULT),
        HI_WORD(GATT_CLIENT_CHAR_CONFIG_DEFAULT)
    };

    uint32_t perm = GATT_PERM_READ | GATT_PERM_WRITE;

    return set_attr(attr, flags, type_val, 4, 2, NULL, perm);
}

bool bt_gatt_server_dynamic_add_service(int32_t server_if,
                                        char *service_uuid, uint8_t is_primary, int32_t number)
{
    BT_GATT_SERVER_TABLE_ALI_T *srvc_entry;
    uint8_t uuid128[16] = {0};
    uint8_t uuid_len = 0;

    if (cur_service_index == BT_GATT_SERVER_MAX_NUM)
    {
        APP_PRINT_ERROR0("bt_gatt_server_dynamic_add_service: no more service entrance!");
        return false;
    }

    if ((service_uuid == NULL) || (number < 1))
    {
        APP_PRINT_INFO0("bt_gatt_server_dynamic_add_service: input params error");
        return false;
    }

    uuid_len = str2uuid(service_uuid, uuid128);
    if ((uuid_len != 2) && (uuid_len != 16))
    {
        APP_PRINT_INFO0("bt_gatt_server_dynamic_add_service: error in uuid length!");
        return false;
    }

    APP_PRINT_INFO2("bt_gatt_server_dynamic_add_service: %d-%d",
                    cur_service_index, server_if);

    srvc_entry = bt_gatt_srvc_tbl + cur_service_index;
    memset(srvc_entry, 0, sizeof(BT_GATT_SERVER_TABLE_ALI_T));
    srvc_entry->service_id = UNKNOWN_SERVICE_ID;
    srvc_entry->srvc_handle = cur_available_start_handle;
    srvc_entry->total_number = number;
    cur_available_start_handle += number;


    srvc_entry->attr_tbl = os_mem_zalloc(RAM_TYPE_DATA_ON, number * sizeof(T_ATTRIB_APPL));
    if (srvc_entry->attr_tbl == NULL)
    {
        APP_PRINT_ERROR0("bt_gatt_server_dynamic_add_service: failed to alloc attribute table!");
        return false;
    }

    srvc_entry->server_if = server_if;


    if (set_service_attr(srvc_entry->attr_tbl,
                         GATT_UUID_PRIMARY_SERVICE, uuid128, uuid_len, GATT_PERM_READ))
    {
        srvc_entry->used_count++;
        cur_service_index++;
        bt_gatt_server_dynamic_srvc_add_cb(server_if, service_uuid,
                                           is_primary, srvc_entry->srvc_handle);
        return true;
    }
    else
    {
        APP_PRINT_ERROR0("bt_gatt_server_dynamic_add_service: failed to set service attribute");
        return false;
    }
}

bool bt_gatt_server_dynamic_add_char(int32_t server_if, int32_t service_handle,
                                     char *uuid, int32_t properties, int32_t permissions)
{
    uint8_t index;
    BT_GATT_SERVER_TABLE_ALI_T *srvc_entry;
    T_ATTRIB_APPL *attr_entry;
    uint8_t uuid128[16] = {0};
    uint8_t uuid_len = 0;
    uint16_t val_perm;

    if (!get_srvc_tbl_index_by_server_if(server_if, &index))
    {
        return false;
    }

    srvc_entry = bt_gatt_srvc_tbl + index;

    if (srvc_entry->used_count == srvc_entry->total_number)
    {
        APP_PRINT_ERROR0("bt_gatt_server_dynamic_add_char: no more handle");
        return false;
    }

    attr_entry = &(srvc_entry->attr_tbl[srvc_entry->used_count]);

    uuid_len = str2uuid(uuid, uuid128);

    //set char declaration attribute
    APP_PRINT_INFO2("set_char_attr: %d-%d", properties, permissions);
    set_char_attr(attr_entry, (uint8_t)properties, permissions);
    srvc_entry->used_count++;
    attr_entry++;

    //set char value attribute
    val_perm = get_char_val_perm_from_char_prop((uint8_t)properties);

    APP_PRINT_INFO1("set_char_val_attr: %d", val_perm);
    set_char_val_attr(attr_entry, uuid128, uuid_len, val_perm);

    bt_gatt_server_dynamic_char_add_cb(server_if, uuid, srvc_entry->srvc_handle,
                                       srvc_entry->srvc_handle + srvc_entry->used_count);

    srvc_entry->used_count++;

    return true;
}

bool bt_gatt_server_dynamic_add_desc(int32_t server_if,
                                     int32_t service_handle, char *uuid, int32_t permissions)
{
    uint8_t index;
    BT_GATT_SERVER_TABLE_ALI_T *srvc_entry;
    T_ATTRIB_APPL *attr_entry;
    uint8_t uuid128[16] = {0};
    uint8_t uuid_len = 0;

    if (!get_srvc_tbl_index_by_server_if(server_if, &index))
    {
        return false;
    }

    srvc_entry = bt_gatt_srvc_tbl + index;
    if (srvc_entry->used_count == srvc_entry->total_number)
    {
        APP_PRINT_ERROR0("bt_gatt_server_dynamic_add_desc: no more handle");
        return false;
    }

    attr_entry = &(srvc_entry->attr_tbl[srvc_entry->used_count]);

    uuid_len = str2uuid(uuid, uuid128);

    if ((uuid_len == UUID_16BIT_SIZE) &&
        (uuid128[0] == LO_WORD(GATT_UUID_CHAR_CLIENT_CONFIG)) &&
        (uuid128[1] == HI_WORD(GATT_UUID_CHAR_CLIENT_CONFIG)))
    {
        if (set_cccd_attr(attr_entry))
        {
            bt_gatt_server_dynamic_desc_add_cb(server_if, uuid, srvc_entry->srvc_handle,
                                               srvc_entry->srvc_handle + srvc_entry->used_count);
            srvc_entry->used_count++;
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        APP_PRINT_ERROR1("bt_gatt_server_dynamic_add_desc: unsupported descriptor - %s",
                         TRACE_STRING(uuid));
        return false;
    }
}

T_APP_RESULT bt_gatt_server_dynamic_write_cb(uint8_t conn_id, T_SERVER_ID service_id,
                                             uint16_t iAttribIndex, T_WRITE_TYPE write_type,
                                             uint16_t wLength, uint8_t *pValue)
{
    //T_APP_RESULT  wCause = APP_RESULT_SUCCESS;
    CSM_AG_GATTS_REQ_WRITE_RST_T write_req;

    uint8_t srvc_index;
    BT_GATT_SERVER_TABLE_ALI_T *srvc_entry;

    uint8_t trans_index;
    BT_GATT_TRANS_INFO *trans_entry;

    uint8_t bd_addr[6];
    uint8_t bd_type;
    char address[CSM_BDADDR_MAX_LEN];

    if ((!get_trans_tbl_index_by_conn_id(conn_id, &trans_index)) ||
        !get_srvc_tbl_index_by_service_id((int32_t)service_id, &srvc_index))
    {
        APP_PRINT_ERROR0("bt_gatt_server_dynamic_write_cb: service or transaction not found");
        return APP_RESULT_ATTR_NOT_FOUND;
    }

    srvc_entry = bt_gatt_srvc_tbl + srvc_index;
    trans_entry = trans_info_tbl + trans_index;
    memset(&write_req, 0, sizeof(write_req));

    write_req.conn_id = conn_id;
    if ((write_type == WRITE_REQUEST) || (write_type == WRITE_LONG))
    {
        write_req.trans_id = trans_entry->trans_id;
        //trans_entry->flag = TRANS_FLAG_WRITE;
        trans_entry->trans_id += 1;
        if (trans_entry->trans_id == 0)
        {
            trans_entry->trans_id = 1;
        }
    }
    else
    {
        write_req.trans_id = 0;
    }

    le_get_conn_addr(conn_id, bd_addr, &bd_type);
    mac_bin_to_str(address, bd_addr);
    memcpy(write_req.btaddr, address, strlen(address));

    write_req.attr_handle = srvc_entry->srvc_handle + iAttribIndex;
    write_req.offset = 0;
    write_req.length = wLength;
    write_req.need_rsp = 0;
    write_req.is_prep = 0;

    memcpy(write_req.value, pValue, wLength);

    csm_gatts_reg_write_cb(&write_req);

    return APP_RESULT_SUCCESS;
}

T_APP_RESULT bt_gatt_server_dynamic_read_cb(uint8_t conn_id, T_SERVER_ID service_id,
                                            uint16_t attrib_index, uint16_t offset)
{
    CSM_AG_GATTS_REQ_READ_RST_T read_req;

    uint8_t srvc_index;
    BT_GATT_SERVER_TABLE_ALI_T *srvc_entry;

    uint8_t trans_index;
    BT_GATT_TRANS_INFO *trans_entry;

    uint8_t bd_addr[6];
    uint8_t bd_type;
    char address[CSM_BDADDR_MAX_LEN];

    if ((!get_trans_tbl_index_by_conn_id(conn_id, &trans_index)) ||
        !get_srvc_tbl_index_by_service_id((int32_t)service_id, &srvc_index))
    {
        APP_PRINT_ERROR0("bt_gatt_server_dynamic_read_cb: service or transaction not found");
        return APP_RESULT_ATTR_NOT_FOUND;
    }

    srvc_entry = bt_gatt_srvc_tbl + srvc_index;
    trans_entry = trans_info_tbl + trans_index;
    memset(&read_req, 0, sizeof(read_req));

    read_req.conn_id = conn_id;

    read_req.trans_id = trans_entry->trans_id;

    le_get_conn_addr(conn_id, bd_addr, &bd_type);
    mac_bin_to_str(address, bd_addr);
    memcpy(read_req.btaddr, address, strlen(address));

    read_req.attr_handle = srvc_entry->srvc_handle + attrib_index;
    read_req.offset = offset;
    read_req.is_long = 0;


    trans_entry->flag = TRANS_FLAG_READ;
    trans_entry->attr_handle = read_req.attr_handle;

    csm_gatts_reg_read_cb(&read_req);

    return APP_RESULT_PENDING;
}

T_APP_RESULT dyn_attr_read_cb(uint8_t conn_id, T_SERVER_ID service_id, uint16_t attrib_index,
                              uint16_t offset, uint16_t *p_length, uint8_t **pp_value)
{
    //T_APP_RESULT cause = APP_RESULT_SUCCESS;
    *p_length = 0;

    APP_PRINT_INFO4("dyn_attr_read_cb: conn_id = %d, service_id = %d, index = %d, offset = %d",
                    conn_id, service_id, attrib_index, offset);


    return bt_gatt_server_dynamic_read_cb(conn_id, service_id,
                                          attrib_index, offset);
}

T_APP_RESULT dyn_attr_write_cb(uint8_t conn_id, T_SERVER_ID service_id, uint16_t iAttribIndex,
                               T_WRITE_TYPE write_type, uint16_t wLength, uint8_t *pValue,
                               P_FUN_WRITE_IND_POST_PROC *pWriteIndPostProc)
{
    //T_APP_RESULT  wCause = APP_RESULT_SUCCESS;

    APP_PRINT_INFO5("dyn_attr_write_cb: conn_id = %d, service_id = %d, index = %d, type = %d, length = %d",
                    conn_id, service_id, iAttribIndex, write_type, wLength);

    return bt_gatt_server_dynamic_write_cb(conn_id, service_id, iAttribIndex,
                                           write_type, wLength, pValue);
}

void dyn_service_cccd_update_cb(uint8_t conn_id, T_SERVER_ID serviceId, uint16_t Index,
                                uint16_t wCCCBits)
{
    APP_PRINT_INFO4("dyn_service_cccd_update_cb: conn_id = %d, serviceId = %d, index = %d, wCCCBits = %02x",
                    conn_id, serviceId, Index, wCCCBits);

}

const T_FUN_GATT_SERVICE_CBS dyn_cbs =
{
    dyn_attr_read_cb,  // Read callback function pointer
    dyn_attr_write_cb, // Write callback function pointer
    dyn_service_cccd_update_cb  // CCCD update callback function pointer
};

bool bt_gatt_server_dynamic_start_service(int32_t server_if,
                                          int32_t service_handle, int32_t transport)
{
    uint8_t index;
    uint8_t server_id = 0;
    BT_GATT_SERVER_TABLE_ALI_T *srvc_entry;

    if (!get_srvc_tbl_index_by_server_if(server_if, &index))
    {
        return false;
    }

    srvc_entry = bt_gatt_srvc_tbl + index;

    if (srvc_entry->service_id != UNKNOWN_SERVICE_ID)
    {
        APP_PRINT_ERROR1("bt_gatt_server_dynamic_start_service: server_if %d already registered",
                         server_if);
        return false;
    }

    APP_PRINT_INFO3("bt_gatt_server_dynamic_start_service, %p, size = %d, handle = %d",
                    srvc_entry->attr_tbl,
                    srvc_entry->total_number * sizeof(T_ATTRIB_APPL),
                    srvc_entry->srvc_handle);


    server_add_service_by_start_handle(
        &server_id,
        (uint8_t *)(srvc_entry->attr_tbl),
        srvc_entry->total_number * sizeof(T_ATTRIB_APPL),
        dyn_cbs,
        srvc_entry->srvc_handle);


    APP_PRINT_INFO1("bt_gatt_server_dynamic_start_service: %d", server_id);
    srvc_entry->service_id = server_id;

    csm_gatts_start_server_cb();

    return true;
}



bool bt_gatt_server_dynamic_send_rsp(int32_t conn_id, int32_t trans_id,
                                     int32_t status, int32_t handle, char *p_val,
                                     int32_t val_len, int32_t auth_req)
{
    bool ret = false;

    uint8_t conn_index;
    BT_GATT_TRANS_INFO *trans_entry;

    uint8_t service_index;
    BT_GATT_SERVER_TABLE_ALI_T *srvc_entry;

    if (!get_trans_tbl_index_by_conn_id(conn_id, &conn_index))
    {
        return false;
    }

    trans_entry = trans_info_tbl + conn_index;

    if ((trans_entry->flag == TRANS_FLAG_IDLE) ||
        (trans_entry->trans_id != trans_id) ||
        (trans_entry->attr_handle != handle))
    {
        APP_PRINT_ERROR5("bt_gatt_server_dynamic_send_rsp: error trans state, %d, %x-%x, %x-%x",
                         trans_entry->flag,
                         trans_entry->trans_id, trans_entry->attr_handle,
                         trans_id, handle);
    }

    if (!get_srvc_tbl_index_by_attr_handle(handle, &service_index))
    {
        return false;
    }

    srvc_entry = bt_gatt_srvc_tbl + service_index;

    if (trans_entry->flag == TRANS_FLAG_READ)
    {
        if ((p_val == NULL) || (val_len < 1) || (status != 0))
        {
            APP_PRINT_ERROR0("bt_gatt_server_dynamic_send_rsp: error in value or length or status");
            return false;
        }


        ret = server_attr_read_confirm(conn_id, srvc_entry->service_id,
                                       handle - srvc_entry->srvc_handle,
                                       (uint8_t *)p_val, val_len, APP_RESULT_SUCCESS);
    }
    else if (trans_entry->flag == TRANS_FLAG_WRITE)
    {
        ret = server_attr_write_confirm(conn_id, srvc_entry->service_id,
                                        handle - srvc_entry->srvc_handle,
                                        APP_RESULT_SUCCESS);
    }

    if (ret)
    {
        trans_entry->flag = TRANS_FLAG_IDLE;
        trans_entry->trans_id++;
        trans_entry->attr_handle = 0x0000;
    }
    else
    {
        APP_PRINT_ERROR0("bt_gatt_server_dynamic_send_rsp failed");
    }

    return ret;
}

bool bt_gatt_server_dynamic_send_indication(int32_t server_if, int32_t handle,
                                            int32_t conn_id, int32_t fg_confirm,
                                            char *p_val, int32_t val_len)
{
    uint8_t service_index;
    T_GATT_PDU_TYPE send_type = GATT_PDU_TYPE_ANY;
    BT_GATT_SERVER_TABLE_ALI_T *srvc_entry;

    if ((p_val == NULL) || (val_len < 1))
    {
        APP_PRINT_ERROR0("bt_gatt_server_dynamic_send_indication: error in value or length");
        return false;
    }

    if (!get_srvc_tbl_index_by_server_if(server_if, &service_index))
    {
        return false;
    }

    srvc_entry = bt_gatt_srvc_tbl + service_index;

    if ((handle <= srvc_entry->srvc_handle) ||
        (handle >= srvc_entry->srvc_handle + srvc_entry->used_count))
    {
        APP_PRINT_ERROR2("bt_gatt_server_dynamic_send_indication: wrong handle %04x for server_if %d",
                         handle, server_if);
        return false;
    }

    if (fg_confirm == 0)
    {
        send_type = GATT_PDU_TYPE_NOTIFICATION;
    }
    else if (fg_confirm == 1)
    {
        send_type = GATT_PDU_TYPE_INDICATION;
    }

    APP_PRINT_INFO7("bt_gatt_server_dynamic_send_indication: %d-%d-%d-%d-%d-%d-%d",
                    conn_id, srvc_entry->service_id,
                    handle, service_index, srvc_entry->srvc_handle,   //index starts from 0
                    val_len, send_type);

    return server_send_data(conn_id, srvc_entry->service_id,
                            handle - srvc_entry->srvc_handle,   //index starts from 0
                            (uint8_t *)p_val, val_len, send_type);
}

static void bt_gatt_server_dynamic_srvc_add_cb(int32_t server_if, char *uuid,
                                               int32_t is_primary, int32_t srvc_handle)
{
    APP_PRINT_INFO4("bt_gatt_server_dynamic_srvc_add_cb: %d, %s, %d, %d",
                    server_if, TRACE_STRING(uuid), is_primary, srvc_handle);

    CSM_AG_GATTS_ADD_SRVC_RST_T srvc_rcd;

    memset(&srvc_rcd, 0, sizeof(srvc_rcd));

    srvc_rcd.server_if = server_if;
    srvc_rcd.srvc_handle = srvc_handle;
    srvc_rcd.srvc_id.is_primary = is_primary;

    memcpy(srvc_rcd.srvc_id.id.uuid, uuid, strlen(uuid));
    srvc_rcd.srvc_id.id.inst_id = 0;

    csm_gatts_add_service_cb(&srvc_rcd);
}

static void bt_gatt_server_dynamic_char_add_cb(int32_t server_if, char *uuid,
                                               int32_t srvc_handle, int32_t char_handle)
{
    APP_PRINT_INFO4("bt_gatt_server_dynamic_char_add_cb: %d, %s, %d, %d",
                    server_if, TRACE_STRING(uuid), srvc_handle, char_handle);

    CSM_AG_GATTS_ADD_CHAR_RST_T char_rcd;

    memset(&char_rcd, 0, sizeof(char_rcd));

    char_rcd.server_if = server_if;
    char_rcd.srvc_handle = srvc_handle;
    char_rcd.char_handle = char_handle;

    memcpy(char_rcd.uuid, uuid, strlen(uuid));

    csm_gatts_add_char_cb(&char_rcd);

}

static void bt_gatt_server_dynamic_desc_add_cb(int32_t server_if, char *uuid,
                                               int32_t srvc_handle, int32_t desc_handle)
{
    APP_PRINT_INFO4("bt_gatt_server_dynamic_desc_add_cb: %d, %s, %d, %d",
                    server_if, TRACE_STRING(uuid), srvc_handle, desc_handle);

    CSM_AG_GATTS_ADD_DESCR_RST_T desc_rcd;

    memset(&desc_rcd, 0, sizeof(desc_rcd));

    desc_rcd.server_if = server_if;
    desc_rcd.srvc_handle = srvc_handle;
    desc_rcd.descr_handle = desc_handle;

    memcpy(desc_rcd.uuid, uuid, strlen(uuid));

    csm_gatts_add_desc_cb(&desc_rcd);
}

static void bt_gatt_server_conn_state_cb(uint8_t conn_id,
                                         T_GAP_CONN_STATE new_state, uint16_t disc_cause)
{
    APP_PRINT_INFO3("bt_gatt_server_conn_state_cb: conn_id = %d, new_state = %d, disc_cuase = %d",
                    conn_id, new_state, disc_cause);

    CSM_AG_GATTS_EVENT_T conn_evt = CSM_AG_GATTS_EVENT_MAX;
    uint8_t trans_index;
    BT_GATT_TRANS_INFO *trans_entry;

    if (new_state == GAP_CONN_STATE_CONNECTED)      //new connection
    {
        if (!get_available_trans_tbl_entry(&trans_index))
        {
            return;
        }

        trans_entry = trans_info_tbl + trans_index;
        trans_entry->conn_id = conn_id;
        trans_entry->trans_id = 1;
        trans_entry->attr_handle = 0;
        trans_entry->flag = TRANS_FLAG_IDLE;

        conn_evt = CSM_AG_GATTS_CONNECT;
    }
    else if (new_state == GAP_CONN_STATE_DISCONNECTED)
    {
        if (!get_trans_tbl_index_by_conn_id(conn_id, &trans_index))
        {
            return;
        }

        release_trans_tbl_entry(trans_index);

        conn_evt = CSM_AG_GATTS_DISCONNECT;
    }

    csm_gatt_conn_evt_cb(conn_evt);
}

bool bt_gatt_server_dynamic_enable_adv(uint8_t enable)
{
    T_GAP_CAUSE ret = GAP_CAUSE_ERROR_UNKNOWN;

    if (enable == 1)
    {
        APP_PRINT_INFO0("enable advertising");
        ret = le_adv_start();
    }
    else if (enable == 0)
    {
        APP_PRINT_INFO0("disable advertising");
        ret = le_adv_stop();
    }

    if (ret == GAP_CAUSE_SUCCESS)
    {
        return true;
    }

    return false;
}

static void bt_gatt_server_dynamic_reg_cb(T_SERVER_ID service_id,
                                          T_SERVER_RESULT result, uint16_t cause)
{
    APP_PRINT_INFO3("bt_gatt_server_dynamic_reg_cb: %d-%d-%d",
                    service_id, result, cause);
}

T_APP_RESULT bt_gatt_server_dynamic_profile_callback(T_SERVER_ID service_id, void *p_data)
{
    T_APP_RESULT app_result = APP_RESULT_SUCCESS;
    T_SERVER_APP_CB_DATA *p_param;

    if (p_data == NULL)
    {
        APP_PRINT_INFO0("bt_gatt_server_dynamic_profile_callback: NULL data!");
        return APP_RESULT_INVALID_VALUE_SIZE;
    }

    p_param = (T_SERVER_APP_CB_DATA *)p_data;
    APP_PRINT_INFO2("bt_gatt_server_dynamic_profile_callback: %d-%d",
                    service_id, p_param->eventId);

    switch (p_param->eventId)
    {
    case PROFILE_EVT_SRV_REG_AFTER_INIT_COMPLETE:
        bt_gatt_server_dynamic_reg_cb(
            p_param->event_data.server_reg_after_init_result.service_id,
            p_param->event_data.server_reg_after_init_result.result,
            p_param->event_data.server_reg_after_init_result.cause);
        break;

    case PROFILE_EVT_SEND_DATA_COMPLETE:
        APP_PRINT_INFO5("PROFILE_EVT_SEND_DATA_COMPLETE: %d-0x%x-%d-0x%x-%d",
                        p_param->event_data.send_data_result.conn_id,
                        p_param->event_data.send_data_result.cause,
                        p_param->event_data.send_data_result.service_id,
                        p_param->event_data.send_data_result.attrib_idx,
                        p_param->event_data.send_data_result.credits);
        if (p_param->event_data.send_data_result.cause == GAP_SUCCESS)
        {
            APP_PRINT_INFO0("PROFILE_EVT_SEND_DATA_COMPLETE success");
        }
        else
        {
            APP_PRINT_ERROR0("PROFILE_EVT_SEND_DATA_COMPLETE failed");
        }
        break;

    default:
        break;

    }

    return app_result;
}

void bt_gatt_server_dynamic_init(void)
{
    server_register_app_cb(bt_gatt_server_dynamic_profile_callback);
    bt_gatt_service_set_conn_state_cb_func(bt_gatt_server_conn_state_cb);
}
