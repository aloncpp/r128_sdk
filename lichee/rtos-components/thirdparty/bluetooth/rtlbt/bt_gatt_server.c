#include "bt_gatt_server.h"
#include "string.h"
#include "trace_app.h"

#define ALI_GATT_SERVICE_UUID16     0x9E20
#define ALI_GATT_CHAR1_UUID16       0x9E30
#define ALI_GATT_CHAR2_UUID16       0x9E34

#define ALI_GATT_SERVER_ALL_START_HANDLE    0x0020

uint8_t default_value_all = 0xff;

static const T_ATTRIB_APPL alibaba_attr_tbl[] =
{
    //primary service
    {
        (ATTRIB_FLAG_VALUE_INCL | ATTRIB_FLAG_LE),
        {
            LO_WORD(GATT_UUID_PRIMARY_SERVICE),
            HI_WORD(GATT_UUID_PRIMARY_SERVICE),
            LO_WORD(ALI_GATT_SERVICE_UUID16),
            HI_WORD(ALI_GATT_SERVICE_UUID16),
        },
        UUID_16BIT_SIZE,
        NULL,
        GATT_PERM_READ
    },

    //1st char
    //declaration
    {
        ATTRIB_FLAG_VALUE_INCL,
        {
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            GATT_CHAR_PROP_READ,
        },
        1,
        NULL,
        GATT_PERM_READ,
    },
    //value
    {
        ATTRIB_FLAG_VALUE_APPL,
        {
            LO_WORD(ALI_GATT_CHAR1_UUID16),
            HI_WORD(ALI_GATT_CHAR1_UUID16),
        },
        0,
        NULL,
        GATT_PERM_READ
    },

    //2st char
    //declaration
    {
        ATTRIB_FLAG_VALUE_INCL,
        {
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            GATT_CHAR_PROP_NOTIFY
        },
        1,
        NULL,
        GATT_PERM_READ
    },
    //value
    {
        ATTRIB_FLAG_VALUE_APPL,
        {
            LO_WORD(ALI_GATT_CHAR2_UUID16),
            HI_WORD(ALI_GATT_CHAR2_UUID16),
        },
        0,
        NULL,
        GATT_PERM_NOTIF_IND
    },

    //desc
    {
        (ATTRIB_FLAG_VALUE_INCL | ATTRIB_FLAG_VALUE_APPL),
        {
            LO_WORD(GATT_UUID_CHAR_CLIENT_CONFIG),
            HI_WORD(GATT_UUID_CHAR_CLIENT_CONFIG),

            LO_WORD(GATT_CLIENT_CHAR_CONFIG_DEFAULT),
            HI_WORD(GATT_CLIENT_CHAR_CONFIG_DEFAULT),
        },
        2,
        NULL,
        (GATT_PERM_READ | GATT_PERM_WRITE)
    }
};


T_APP_RESULT ali_attr_read_cb(uint8_t conn_id, T_SERVER_ID service_id, uint16_t attrib_index,
                              uint16_t offset, uint16_t *p_length, uint8_t **pp_value)
{
    T_APP_RESULT cause = APP_RESULT_SUCCESS;
    *p_length = 0;

    APP_PRINT_INFO3("ali_attr_read_cb: conn_id = %d, service_id = %d, index = %d",
                    conn_id, service_id, attrib_index);

    switch (attrib_index)
    {
    default:
        *pp_value = &default_value_all;
        *p_length = sizeof(default_value_all);
        break;
    }

    return cause;
}

T_APP_RESULT ali_attr_write_cb(uint8_t conn_id, T_SERVER_ID service_id, uint16_t iAttribIndex,
                               T_WRITE_TYPE write_type, uint16_t wLength, uint8_t *pValue,
                               P_FUN_WRITE_IND_POST_PROC *pWriteIndPostProc)
{
    T_APP_RESULT  wCause = APP_RESULT_SUCCESS;

    APP_PRINT_INFO3("ali_attr_write_cb: conn_id = %d, service_id = %d, index = %d",
                    conn_id, service_id, iAttribIndex);

    return wCause;
}

void ali_service_cccd_update_cb(uint8_t conn_id, T_SERVER_ID serviceId, uint16_t Index,
                                uint16_t wCCCBits)
{
    APP_PRINT_INFO4("ali_service_cccd_update_cb: conn_id = %d, serviceId = %d, index = %d, wCCCBits = %02x",
                    conn_id, serviceId, Index, wCCCBits);

}

const T_FUN_GATT_SERVICE_CBS ali_cbs =
{
    ali_attr_read_cb,  // Read callback function pointer
    ali_attr_write_cb, // Write callback function pointer
    ali_service_cccd_update_cb  // CCCD update callback function pointer
};

T_SERVER_ID ali_add_service(void *p_func)
{
    APP_PRINT_INFO1("ali_add_service: size = %d", sizeof(alibaba_attr_tbl));

    T_SERVER_ID service_id;
    if (false == server_add_service_by_start_handle(
            &service_id, (uint8_t *)alibaba_attr_tbl,
            sizeof(alibaba_attr_tbl), ali_cbs,
            ALI_GATT_SERVER_ALL_START_HANDLE))
    {
        APP_PRINT_ERROR1("ali_add_service: failed to add service_id %d", service_id);
        service_id = 0xff;
    }
    else
    {
        APP_PRINT_INFO1("ali_add_service: add service %d success!", service_id);
    }

    return service_id;
}
