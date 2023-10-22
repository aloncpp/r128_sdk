/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <stdint.h>
#include <string.h>

#include "os_msg.h"
#include "os_task.h"
#include "trace_app.h"
#include "os_sched.h"

#include "app_msg.h"
#include "app_flags.h"
#include "cod.h"
#include "gap_legacy.h"
#include "gap_timer.h"

#include "bt_gap.h"
#include "bt_sdp.h"
#include "bt_a2dp.h"
#include "bt_avrcp.h"
#include "bt_hfp.h"
#include "blue_cmd.h"
//#include "osif.h"
#include "blue_cmd_legacy.h"

#include "gap_le.h"
#include "gap_adv.h"
#include "gap_bond_le.h"

#include "gatt.h"
#include "bt_le_peripheral.h"
#include "bt_gatt_server_dynamic.h"

#define MAX_NUMBER_OF_GAP_MESSAGE       0x20    //!< indicate BT stack message queue size
#define MAX_NUMBER_OF_IO_MESSAGE        0x40    //!< indicate io queue size, extra 0x20 for data uart
#define MAX_NUMBER_OF_GAP_TIMER         0x10    //!< indicate gap timer queue size
/** indicate rx event queue size*/
#define MAX_NUMBER_OF_RX_EVENT      \
    (MAX_NUMBER_OF_GAP_MESSAGE + MAX_NUMBER_OF_IO_MESSAGE + MAX_NUMBER_OF_GAP_TIMER)

#define DEFAULT_PAGESCAN_WINDOW             0x12
#define DEFAULT_PAGESCAN_INTERVAL           0x800 //0x800
#define DEFAULT_PAGE_TIMEOUT                0x2000
#define DEFAULT_SUPVISIONTIMEOUT            0x1f40 //0x7D00
#define DEFAULT_INQUIRYSCAN_WINDOW          0x12
#define DEFAULT_INQUIRYSCAN_INTERVAL        0x800 //0x1000

void *audio_io_queue_handle;
void *audio_evt_queue_handle;
static struct bta_struct *audio_bta;

static void app_gap_init(void)
{
    uint32_t class_of_device = SERVICE_CLASS_AUDIO | SERVICE_CLASS_RENDERING | MAJOR_DEVICE_CLASS_AUDIO
                               | MINOR_DEVICE_CLASS_HEADPHONES;
    uint16_t supervision_timeout = DEFAULT_SUPVISIONTIMEOUT;
    uint16_t link_policy = GAP_LINK_POLICY_ROLE_SWITCH | GAP_LINK_POLICY_SNIFF_MODE;

    uint8_t radio_mode = GAP_RADIO_MODE_NONE_DISCOVERABLE;
    bool limited_discoverable = false;

    uint8_t pagescan_type = GAP_PAGE_SCAN_TYPE_INTERLACED;
    uint16_t pagescan_interval = DEFAULT_PAGESCAN_INTERVAL;
    uint16_t pagescan_window = DEFAULT_PAGESCAN_WINDOW;
    uint16_t page_timeout = DEFAULT_PAGE_TIMEOUT;

    uint8_t inquiryscan_type = GAP_INQUIRY_SCAN_TYPE_INTERLACED;
    uint16_t inquiryscan_window = DEFAULT_INQUIRYSCAN_WINDOW;
    uint16_t inquiryscan_interval = DEFAULT_INQUIRYSCAN_INTERVAL;
    uint8_t inquiry_mode = GAP_INQUIRY_MODE_EXTENDED_RESULT;

    uint8_t pair_mode = GAP_PAIRING_MODE_PAIRABLE;
    uint16_t auth_flags = GAP_AUTHEN_BIT_GENERAL_BONDING_FLAG | GAP_AUTHEN_BIT_SC_FLAG;
    uint8_t io_cap = GAP_IO_CAP_NO_INPUT_NO_OUTPUT;
    uint8_t oob_enable = false;
    uint8_t bt_mode = GAP_BT_MODE_21ENABLED;

    legacy_gap_init();

    legacy_set_gap_param(GAP_PARAM_LEGACY_NAME, GAP_DEVICE_NAME_LEN, "rt_bt_ly");

    gap_set_param(GAP_PARAM_BOND_PAIRING_MODE, sizeof(uint8_t), &pair_mode);
    gap_set_param(GAP_PARAM_BOND_AUTHEN_REQUIREMENTS_FLAGS, sizeof(uint16_t), &auth_flags);
    gap_set_param(GAP_PARAM_BOND_IO_CAPABILITIES, sizeof(uint8_t), &io_cap);
    gap_set_param(GAP_PARAM_BOND_OOB_ENABLED, sizeof(uint8_t), &oob_enable);

    legacy_set_gap_param(GAP_PARAM_BT_MODE, sizeof(uint8_t), &bt_mode);
    legacy_set_gap_param(GAP_PARAM_COD, sizeof(uint32_t), &class_of_device);
    legacy_set_gap_param(GAP_PARAM_LINK_POLICY, sizeof(uint16_t), &link_policy);
    legacy_set_gap_param(GAP_PARAM_SUPV_TOUT, sizeof(uint16_t), &supervision_timeout);

    legacy_set_gap_param(GAP_PARAM_RADIO_MODE, sizeof(uint8_t), &radio_mode);
    legacy_set_gap_param(GAP_PARAM_LIMIT_DISCOV, sizeof(bool), &limited_discoverable);

    legacy_set_gap_param(GAP_PARAM_PAGE_SCAN_TYPE, sizeof(uint8_t), &pagescan_type);
    legacy_set_gap_param(GAP_PARAM_PAGE_SCAN_INTERVAL, sizeof(uint16_t), &pagescan_interval);
    legacy_set_gap_param(GAP_PARAM_PAGE_SCAN_WINDOW, sizeof(uint16_t), &pagescan_window);
    legacy_set_gap_param(GAP_PARAM_PAGE_TIMEOUT, sizeof(uint16_t), &page_timeout);

    legacy_set_gap_param(GAP_PARAM_INQUIRY_SCAN_TYPE, sizeof(uint8_t), &inquiryscan_type);
    legacy_set_gap_param(GAP_PARAM_INQUIRY_SCAN_INTERVAL, sizeof(uint16_t), &inquiryscan_interval);
    legacy_set_gap_param(GAP_PARAM_INQUIRY_SCAN_WINDOW, sizeof(uint16_t), &inquiryscan_window);
    legacy_set_gap_param(GAP_PARAM_INQUIRY_MODE, sizeof(uint8_t), &inquiry_mode);
}

/** @brief  GAP - scan response data (max size = 31 bytes) */
static const uint8_t scan_rsp_data[] =
{
    0x0C,                             /* length */
    GAP_ADTYPE_LOCAL_NAME_COMPLETE,            /* type="Appearance" */
    'R', 'T', 'K', '_', 'B', 'T', '_', '4', '.', '1', '\0',
};

#define GATT_UUID_SIMPLE_PROFILE                    0xA00A

/** @brief  GAP - Advertisement data (max size = 31 bytes, best kept short to conserve power) */
static const uint8_t adv_data[] =
{
    /* Flags */
    0x0F,             /* length */
    GAP_ADTYPE_MANUFACTURER_SPECIFIC, /* type="Flags" */
    0x54, 0x4D,
    '_', 'G', 'E', 'N', 'I', 'E',
    ' ', ' ', ' ', ' ', ' ', ' ',
    /* Service */
    0x03,             /* length */
    GAP_ADTYPE_16BIT_COMPLETE,
    LO_WORD(GATT_UUID_SIMPLE_PROFILE),
    HI_WORD(GATT_UUID_SIMPLE_PROFILE),
    /* Local name */
    0x0C,             /* length */
    GAP_ADTYPE_LOCAL_NAME_COMPLETE,
    'R', 'T', 'K', '_', 'B', 'T', '_', '4', '.', '1', '\0',
};

void app_le_gap_init(void)
{
    /* Device name and device appearance */
    uint8_t  device_name[GAP_DEVICE_NAME_LEN] = "rt_bt_ly";
    uint16_t appearance = GAP_GATT_APPEARANCE_UNKNOWN;
    uint8_t  slave_init_mtu_req = false;


    /* Advertising parameters */
    uint8_t  adv_evt_type = GAP_ADTYPE_ADV_IND;
    uint8_t  adv_direct_type = GAP_REMOTE_ADDR_LE_PUBLIC;
    uint8_t  adv_direct_addr[GAP_BD_ADDR_LEN] = {0};
    uint8_t  adv_chann_map = GAP_ADVCHAN_ALL;
    uint8_t  adv_filter_policy = GAP_ADV_FILTER_ANY;
    uint16_t adv_int_min = DEFAULT_ADVERTISING_INTERVAL_MIN;
    uint16_t adv_int_max = DEFAULT_ADVERTISING_INTERVAL_MAX;

    /* GAP Bond Manager parameters */
    uint8_t  auth_pair_mode = GAP_PAIRING_MODE_PAIRABLE;
    uint16_t auth_flags = GAP_AUTHEN_BIT_BONDING_FLAG;
    uint8_t  auth_io_cap = GAP_IO_CAP_NO_INPUT_NO_OUTPUT;
    uint8_t  auth_oob = false;
    uint8_t  auth_use_fix_passkey = false;
    uint32_t auth_fix_passkey = 0;
#if F_BT_ANCS_CLIENT_SUPPORT
    uint8_t  auth_sec_req_enable = true;
#else
    uint8_t  auth_sec_req_enable = false;
#endif
    uint16_t auth_sec_req_flags = GAP_AUTHEN_BIT_BONDING_FLAG;

    /* Set device name and device appearance */
    le_set_gap_param(GAP_PARAM_DEVICE_NAME, GAP_DEVICE_NAME_LEN, device_name);
    le_set_gap_param(GAP_PARAM_APPEARANCE, sizeof(appearance), &appearance);
    le_set_gap_param(GAP_PARAM_SLAVE_INIT_GATT_MTU_REQ, sizeof(slave_init_mtu_req),
                     &slave_init_mtu_req);

    /* Set advertising parameters */
    le_adv_set_param(GAP_PARAM_ADV_EVENT_TYPE, sizeof(adv_evt_type), &adv_evt_type);
    le_adv_set_param(GAP_PARAM_ADV_DIRECT_ADDR_TYPE, sizeof(adv_direct_type), &adv_direct_type);
    le_adv_set_param(GAP_PARAM_ADV_DIRECT_ADDR, sizeof(adv_direct_addr), adv_direct_addr);
    le_adv_set_param(GAP_PARAM_ADV_CHANNEL_MAP, sizeof(adv_chann_map), &adv_chann_map);
    le_adv_set_param(GAP_PARAM_ADV_FILTER_POLICY, sizeof(adv_filter_policy), &adv_filter_policy);
    le_adv_set_param(GAP_PARAM_ADV_INTERVAL_MIN, sizeof(adv_int_min), &adv_int_min);
    le_adv_set_param(GAP_PARAM_ADV_INTERVAL_MAX, sizeof(adv_int_max), &adv_int_max);
    le_adv_set_param(GAP_PARAM_ADV_DATA, sizeof(adv_data), (void *)adv_data);
    le_adv_set_param(GAP_PARAM_SCAN_RSP_DATA, sizeof(scan_rsp_data), (void *)scan_rsp_data);

    /* Setup the GAP Bond Manager */
    gap_set_param(GAP_PARAM_BOND_PAIRING_MODE, sizeof(auth_pair_mode), &auth_pair_mode);
    gap_set_param(GAP_PARAM_BOND_AUTHEN_REQUIREMENTS_FLAGS, sizeof(auth_flags), &auth_flags);
    gap_set_param(GAP_PARAM_BOND_IO_CAPABILITIES, sizeof(auth_io_cap), &auth_io_cap);
    gap_set_param(GAP_PARAM_BOND_OOB_ENABLED, sizeof(auth_oob), &auth_oob);
    le_bond_set_param(GAP_PARAM_BOND_FIXED_PASSKEY, sizeof(auth_fix_passkey), &auth_fix_passkey);
    le_bond_set_param(GAP_PARAM_BOND_FIXED_PASSKEY_ENABLE, sizeof(auth_use_fix_passkey),
                      &auth_use_fix_passkey);
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_ENABLE, sizeof(auth_sec_req_enable), &auth_sec_req_enable);
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_REQUIREMENT, sizeof(auth_sec_req_flags),
                      &auth_sec_req_flags);

    /* register gap message callback */
    //le_register_app_cb(app_gap_callback);
}

//void app_handle_io_msg(T_IO_MSG io_msg)
//{
//
//}

int bcmd_func(uint16_t opcode, int argc, void **argv)
{
    return bta_cmd_legacy_process(opcode, argc, argv);
}

int bta_submit_command_wait(uint16_t opcode, int argc, void **argv)
{
    return __bta_submit_command_wait(audio_bta, opcode, argc, argv);
}

static void app_task(void *pvParameters)
{
    uint8_t event;
    struct bta_struct *bta;
    void *task_handle = NULL;

    os_task_handle_get(&task_handle);
    APP_PRINT_INFO1("__bta_submit_command_wait3: %p", task_handle);
    bta = bta_struct_init(task_handle, bcmd_func, audio_evt_queue_handle);

    audio_bta = bta;

    gap_start_bt_stack(audio_evt_queue_handle, audio_io_queue_handle, MAX_NUMBER_OF_GAP_MESSAGE);

    while (true)
    {
        if (os_msg_recv(audio_evt_queue_handle, &event, 0xFFFFFFFF) == true)
        {

            if (event == EVENT_IO_TO_APP)
            {
                T_IO_MSG io_msg;
                APP_PRINT_INFO1("app_task, event = %d", event);
                if (os_msg_recv(audio_io_queue_handle, &io_msg, 0) == true)
                {
                    APP_PRINT_INFO0("app_handle_io_msg");
                    app_handle_io_msg(io_msg);
                }
            }
            else if (event == EVENT_CUSTOM_OP)
            {
                bta_run_command(audio_bta);
            }
            else
            {
                gap_handle_msg(event);
            }
        }
    }
}

/*
void test_add_gatt_server_dynamic(void)
{
    bool ret = false;

    int32_t server_if = 0x0001;
    char service_uuid[] = "9E20";
    int32_t num_handle = 6;

    char char1_uuid[] = "9E30";
    int32_t char1_prop = 0x04;
    int32_t char1_perm = 0x01;

    char char2_uuid[] = "9E34";
    int32_t char2_prop = 0x10;
    int32_t char2_perm = 0x01;

    char desc2_1_uuid[] = "2902";
    int32_t desc2_1_perm = 0x11;


    ret = bt_gatt_server_dynamic_add_service(server_if, service_uuid, 1, num_handle);
    if (!ret)
    {
        APP_PRINT_ERROR0("test_add_gatt_server_dynamic: failed to add service");
        return;
    }

    ret = bt_gatt_server_dynamic_add_char(server_if, 0x0020, char1_uuid, char1_prop, char1_perm);
    if (!ret)
    {
        APP_PRINT_ERROR0("test_add_gatt_server_dynamic: failed to add 1st char");
        return;
    }

    ret = bt_gatt_server_dynamic_add_char(server_if, 0x0020, char2_uuid, char2_prop, char2_perm);
    if (!ret)
    {
        APP_PRINT_ERROR0("test_add_gatt_server_dynamic: failed to add 2st char");
        return;
    }

    ret = bt_gatt_server_dynamic_add_desc(server_if, 0x0020, desc2_1_uuid, desc2_1_perm);
    if (!ret)
    {
        APP_PRINT_ERROR0("test_add_gatt_server_dynamic: failed to add 1st desc for 2st char");
        return;
    }

    ret = bt_gatt_server_dynamic_start_service(server_if, 0x0020, 1);
    if (!ret)
    {
        APP_PRINT_ERROR0("test_add_gatt_server_dynamic: failed to start service");
        return;
    }

}
*/

int app_init(void)
{
    void *app_task_handle;
    int init_time_count = 0;

    APP_PRINT_INFO2("APP COMPILE TIME: [%s - %s]", TRACE_STRING(__DATE__), TRACE_STRING(__TIME__));

    os_msg_queue_create(&audio_io_queue_handle, MAX_NUMBER_OF_IO_MESSAGE, sizeof(T_IO_MSG));
    os_msg_queue_create(&audio_evt_queue_handle, MAX_NUMBER_OF_RX_EVENT, sizeof(unsigned char));

    gap_init_timer(audio_evt_queue_handle, MAX_NUMBER_OF_GAP_TIMER);

    app_gap_init();

    bt_gap_init();
    bt_sdp_init();
    bt_hfp_init();
    bt_avrcp_init();
    bt_a2dp_init();

    //le init
    le_gap_init(APP_MAX_LINKS);
    app_le_gap_init();

    bt_le_peripheral_init();
    bt_gatt_server_dynamic_init();
    //test_add_gatt_server_dynamic();

    os_task_create(&app_task_handle, "app_task", app_task, NULL, 1024 * 4, 1);

    while (init_time_count < 100)
    {
        os_delay(100);
        if (is_gap_stack_ready() && is_le_stack_ready())
        {
            APP_PRINT_INFO0("app_init: stack ready");
            break;
        }

        init_time_count++;
    }

    APP_PRINT_INFO1("__bta_submit_command_wait3: %p", app_task_handle);

    return 0;
}

