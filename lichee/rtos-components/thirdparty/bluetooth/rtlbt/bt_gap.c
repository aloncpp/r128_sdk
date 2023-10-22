/*
* Copyright (C) 2018 Realtek Semiconductor Corporation. All Rights Reserved.
*/

#include <string.h>

#include "trace_app.h"

#include "gap_legacy.h"
#include "gap_bond_legacy.h"
#include "gap_storage_legacy.h"

#include "bt_link.h"

#include "gap.h"
#include "blue_cmd_legacy.h"
#include "bt_gap.h"
#include "trace.h"
#include "ftl_if.h"
//#include "app_flags.h"

bool classic_stack_ready = false;

#define FLASH_OFFSET_LOCAL_NAME    1408
char bt_gap_local_name[GAP_DEVICE_NAME_LEN] = {'a', 'u', 'd', 'i', 'o', '_', 't', 'h', 'o', 'm', 'a', 's', '\0'};
char bt_gap_local_addr[6];

//most recent connected device info
#define FLASH_OFFSET_REMOTE_NAME    1024
char bt_gap_remote_name[8][GAP_DEVICE_NAME_LEN] = {0};

bool unpair_flag = false;

static void bt_gap_flash_save_remote_dev_name(uint8_t index)
{
    //to be done
    uint16_t offset;

    offset = FLASH_OFFSET_REMOTE_NAME + index * GAP_DEVICE_NAME_LEN;

    ftl_save_to_storage(bt_gap_remote_name[index], offset, GAP_DEVICE_NAME_LEN);
    APP_PRINT_INFO2("bt_gap_flash_save_dev_name: %d-%s", index,
                    TRACE_STRING(bt_gap_remote_name[index]));
}

//load paired device names
//all 8 devices loaded, even if invalid
static void bt_gap_flash_load_remote_dev_name()
{
    int i;
    ftl_load_from_storage(bt_gap_remote_name,
                          FLASH_OFFSET_REMOTE_NAME, 8 * GAP_DEVICE_NAME_LEN);

    for (i = 0; i < 8; i++)
    {
        bt_gap_remote_name[i][GAP_DEVICE_NAME_LEN - 1] = '\0';
    }
}

static void bt_gap_flash_save_local_dev_name()
{
    ftl_save_to_storage(bt_gap_local_name,
                        FLASH_OFFSET_LOCAL_NAME, GAP_DEVICE_NAME_LEN);
}

static void bt_gap_flash_load_local_dev_name()
{
    char load_local_name[GAP_DEVICE_NAME_LEN] = {0};
    if (!ftl_load_from_storage(load_local_name,
                               FLASH_OFFSET_LOCAL_NAME, GAP_DEVICE_NAME_LEN))
    {
        load_local_name[GAP_DEVICE_NAME_LEN - 1] = '\0';
        memcpy(bt_gap_local_name,
               load_local_name, GAP_DEVICE_NAME_LEN);

        legacy_set_dev_name((uint8_t *)bt_gap_local_name,
                            strlen(bt_gap_local_name));
    }
}


bool bt_gap_read_local_address(uint8_t *addr)
{
    if (addr == NULL)
    {
        APP_PRINT_ERROR0("bt_gap_read_local_address, address is NULL");
        return false;
    }

    memcpy(addr, bt_gap_local_addr, 6);

    return true;
}

bool bt_gap_read_local_name(char *name, int length)
{
    int actual_length = 0;

    if (name == NULL)
    {
        APP_PRINT_ERROR0("bt_gap_read_local_name, name is NULL");
        return false;
    }

    memset(name, 0, length);

    actual_length = strlen((char *)bt_gap_local_name);

    if (actual_length >= length)
    {
        actual_length = length - 1;
    }

    strncpy(name, (char *)bt_gap_local_name, actual_length);
    name[actual_length] = '\0';

    return true;
}

//get most recent paired and connected device address
bool bt_gap_read_remote_address(uint8_t *bd_addr)
{
    T_BT_LINK *p_link = NULL;

    if (bd_addr == NULL)
    {
        APP_PRINT_ERROR0("bt_gap_read_remote_address, address is NULL");
        return false;
    }

    //get the device address with the highest priority
    if (!legacy_get_addr_by_priority(1, bd_addr))
    {
        APP_PRINT_ERROR0("bt_gap_read_remote_address: no paired device now!");
        return false;
    }

    p_link = bt_find_link_by_addr(bd_addr);

    if (p_link == NULL)
    {
        APP_PRINT_ERROR0("no connected device now!");
        return false;
    }

    return true;
}

bool bt_gap_read_remote_name(uint8_t *addr, char *name, int length)
{
    int actual_length = 0;
    uint8_t index = 0xff;

    if (addr == NULL)
    {
        APP_PRINT_ERROR0("bt_gap_read_remote_name: address is NULL");
        return false;
    }

    if (name == NULL)
    {
        APP_PRINT_ERROR0("bt_gap_read_remote_name, name is NULL");
        return false;
    }

    memset(name, 0, length);

    if (!legacy_get_paired_idx(addr, &index))
    {
        APP_PRINT_ERROR1("bt_gap_read_remote_name: % is not a paired device", TRACE_BDADDR(addr));
        return false;
    }

    actual_length = strlen((char *)(bt_gap_remote_name[index]));

    if (actual_length >= length)
    {
        actual_length = length - 1;
    }

    strncpy(name, (char *)(bt_gap_remote_name[index]), actual_length);
    //name[actual_length] = '\0';

    return true;
}

bool bt_gap_set_scan_mode(bool conn_flag, bool disc_flag)
{
    uint8_t radio_mode;
    T_GAP_CAUSE ret = GAP_CAUSE_ERROR_UNKNOWN;

    if (conn_flag && disc_flag)
    {
        radio_mode = GAP_RADIO_MODE_VISIABLE_CONNECTABLE;
    }
    else if (conn_flag)
    {
        radio_mode = GAP_RADIO_MODE_CONNECTABLE;
    }
    else if (disc_flag)
    {
        radio_mode = GAP_RADIO_MODE_VISIABLE;
    }
    else
    {
        radio_mode = GAP_RADIO_MODE_NONE_DISCOVERABLE;
    }

    ret = legacy_set_radio_mode(radio_mode, false, 255);

    if (ret != GAP_CAUSE_SUCCESS)
    {
        APP_PRINT_ERROR1("bt_gap_set_scan_mode failed: %d", ret);
        return false;
    }

    return true;
}

bool bt_gap_set_local_name(char *name)
{
    T_GAP_CAUSE ret = GAP_CAUSE_ERROR_UNKNOWN;
    int actual_length = 0;

    if (name == NULL)
    {
        APP_PRINT_ERROR0("bt_gap_set_local_name: name is NULL!");
        return false;
    }

    actual_length = strlen(name);

    if (actual_length >= GAP_DEVICE_NAME_LEN)
    {
        actual_length = GAP_DEVICE_NAME_LEN - 1;
    }

    ret = legacy_set_dev_name((uint8_t *)name, actual_length);

    if (ret != GAP_CAUSE_SUCCESS)
    {
        APP_PRINT_ERROR1("bt_gap_set_local_name failed: %d", ret);
        return false;
    }

    strncpy((char *)bt_gap_local_name, name, actual_length);
    bt_gap_local_name[actual_length] = '\0';

    bt_gap_flash_save_local_dev_name();

    return true;
}

bool bt_gap_start_discovery(void)
{
    T_GAP_CAUSE ret;

    uint8_t timeouts = 10;

    ret = legacy_start_inquiry(false, timeouts);

    if (ret != GAP_CAUSE_SUCCESS)
    {
        APP_PRINT_ERROR1("bt_gap_start_discovery failed: %d", ret);
        return false;
    }

    return true;
}

bool bt_gap_stop_discovery(void)
{
    T_GAP_CAUSE ret;

    ret = legacy_stop_inquiry();

    if (ret != GAP_CAUSE_SUCCESS)
    {
        APP_PRINT_ERROR1("bt_gap_stop_discovery failed: %d", ret);
        return false;
    }

    return true;
}

bool bt_gap_unpair_device(uint8_t *addr)
{
    bool ret = false;

    if (addr == NULL)
    {
        APP_PRINT_ERROR0("bt_gap_unpair_device: address is NULL");
        return false;
    }

    ret = legacy_delete_bond(addr);

    if (ret == false)
    {
        APP_PRINT_ERROR1("bt_gap_unpair_device failed, address is %s", TRACE_BDADDR(addr));
        return false;
    }

    legacy_send_acl_disconn_req(addr);

    unpair_flag = true;

    return true;
}


static bool bt_gap_save_remote_device_name(uint8_t *bd_addr, char *name)
{
    uint8_t index = 0xff;

    if ((bd_addr == NULL) || (name == NULL))
    {
        APP_PRINT_ERROR0("bt_gap_save_remote_device_name: address or name is NULL!");
        return false;
    }

    if (!legacy_get_paired_idx(bd_addr, &index))
    {
        APP_PRINT_ERROR1("bt_gap_save_remote_device_name: failed to get index for device %s!",
                         TRACE_BDADDR(bd_addr));
        return false;
    }

    APP_PRINT_INFO2("bt_gap_save_remote_device_name: address is %s, name is %s",
                    TRACE_BDADDR(bd_addr), TRACE_STRING(name));

    memset(bt_gap_remote_name[index], 0, GAP_DEVICE_NAME_LEN);
    strncpy((char *)(bt_gap_remote_name[index]), name, strlen(name));

    bt_gap_flash_save_remote_dev_name(index);

    return true;
}

void bt_handle_acl_status(T_GAP_ACL_STATUS_INFO *p_info)
{
    T_BT_LINK *p_link = NULL;

    p_link = bt_find_link_by_addr(p_info->bd_addr);

    APP_PRINT_TRACE1("bt_handle_acl_status: status 0x%x", p_info->status);

    switch (p_info->status)
    {
    case GAP_ACL_CONN_ACTIVE:
        if (p_link)
        {
            p_link->acl_link_in_sniffmode_fg = 0;
        }
        break;

    case GAP_ACL_CONN_SNIFF:
        if (p_link)
        {
            p_link->acl_link_in_sniffmode_fg = 1;
        }
        break;

    case GAP_ACL_CONN_SUCCESS:
        if (p_link == NULL)
        {
            p_link = bt_alloc_link(p_info->bd_addr);
            if (p_link == NULL)
            {
                APP_PRINT_ERROR1("bt_handle_acl_status: fail to alloc for addr %s", TRACE_BDADDR(p_info->bd_addr));
                return;
            }
        }
        else
        {
            APP_PRINT_INFO1("bt_handle_acl_status: link already exists for addr %s",
                            TRACE_BDADDR(p_info->bd_addr));
            //return;
        }

//===============TODO=================
        APP_PRINT_ERROR1("bt_handle_acl_status: pairok to alloc for addr %s",
                         TRACE_BDADDR(p_info->bd_addr));
        //READ REMOTE NAME  legacy_get_remote_name(p_info->bd_addr);
        //save remote address name
        //char bdaddr_str[20];

        p_link->acl_link_state = LINK_STATE_CONNECTED;
        p_link->hci_conn_handle = p_info->p.conn_success.handle;

        break;

    case GAP_ACL_CONN_FAIL:
        APP_PRINT_INFO2("bt_handle_acl_status: acl conn fail for addr %s, reason 0x%x",
                        TRACE_BDADDR(p_info->bd_addr), p_info->p.conn_fail.cause);
        break;

    case GAP_ACL_AUTHEN_SUCCESS:
        if (p_link)
        {
            APP_PRINT_INFO1("bt_handle_acl_status: authen success addr %s",
                            TRACE_BDADDR(p_info->bd_addr));
            legacy_get_remote_name(p_info->bd_addr);
            csm_paired_result_cb(0, p_info->bd_addr);
        }
        break;

    case GAP_ACL_AUTHEN_FAIL:
        {
            APP_PRINT_INFO2("bt_handle_acl_status: authen fail addr %s, cause 0x%x",
                            TRACE_BDADDR(p_info->bd_addr), p_info->p.authen.cause);

            csm_paired_result_cb(1, p_info->bd_addr);

            if (p_info->p.authen.cause == (HCI_ERR | HCI_ERR_AUTHEN_FAIL))
            {
                legacy_delete_bond(p_info->bd_addr);
            }
        }
        break;

    case GAP_ACL_CONN_DISCONN:
        if (p_link)
        {
            APP_PRINT_INFO2("bt_handle_acl_status: acl disconnd addr %s, status 0x%x",
                            TRACE_BDADDR(p_info->bd_addr), p_info->p.conn_disconn.cause);
            if (unpair_flag)
            {
                if (p_info->p.conn_disconn.cause == (HCI_ERR | HCI_ERR_LOCAL_HOST_TERMINATE))
                {
                    csm_unpaired_result_cb(0, p_info->bd_addr);
                    unpair_flag = false;
                }
            }
            bt_free_link(p_link);
        }
        break;

    case GAP_ACL_ROLE_MASTER:
        if (p_link)
        {
            APP_PRINT_TRACE0("bt_handle_acl_status: link role master");
            p_link->acl_link_role_matser_fg = 1;
        }
        break;

    case GAP_ACL_ROLE_SLAVE:
        if (p_link)
        {
            APP_PRINT_TRACE0("bt_handle_acl_status: link role slave");
            p_link->acl_link_role_matser_fg = 0;
        }
        break;

    default:
        break;
    }
}


void bt_app_handle_inquiry_result(void *data)
{
    T_GAP_INQUIRY_RESULT_INFO *pinfo = (T_GAP_INQUIRY_RESULT_INFO *)data;

    csm_inquiry_resp_cb((char *)pinfo->name, pinfo->bd_addr);
    APP_PRINT_INFO3("\r\nbt_app_handle_inquiry_result:%s:%s, %x",
                    TRACE_STRING(pinfo->name), TRACE_BDADDR(pinfo->bd_addr),
                    strlen((char *)(pinfo->name)));
}

void bt_handle_remote_name(T_GAP_REMOTE_NAME_INFO *p_rsp)
{
    if (!p_rsp->cause)
    {
        APP_PRINT_TRACE3("Get remote name response: address is %s, name is %s, len is %d",
                         TRACE_BDADDR(p_rsp->bd_addr), TRACE_STRING((char *)p_rsp->name),
                         strlen((char *)p_rsp->name));

        bt_gap_save_remote_device_name(p_rsp->bd_addr, (char *)p_rsp->name);
    }
    else
    {
        //get fail
    }
}


void bt_legacy_cb(void *p_buf, T_LEGACY_MSG legacy_msg)
{
    if ((legacy_msg != L2C_DATA_RSP) && (legacy_msg != SCO_DATA_IND))
    {
        APP_PRINT_TRACE1("bt_legacy_cb: msg 0x%x", legacy_msg);
    }

    switch (legacy_msg)
    {
    case PROTO_REG_CMPL:
        gap_get_param(GAP_PARAM_BD_ADDR, bt_gap_local_addr);
        classic_stack_ready = true;

        legacy_set_page_with_scan(1);
        legacy_cfg_accept_role(1);
        legacy_set_radio_mode(GAP_RADIO_MODE_VISIABLE_CONNECTABLE, false, 0);

        APP_PRINT_INFO0("before load device name");
        bt_gap_flash_load_remote_dev_name();
        bt_gap_flash_load_local_dev_name();
        APP_PRINT_INFO0("after load devices name");
        break;

    case USER_CONFIRM_REQ:
        {
            T_GAP_USER_CFM_REQ_IND *p_ind = (T_GAP_USER_CFM_REQ_IND *)p_buf;
            legacy_bond_user_cfm(p_ind->bd_addr, GAP_CFM_CAUSE_ACCEPT);
        }
        break;

    case SCO_CONN_IND:
        {
            T_GAP_SCO_CONN_IND *p_ind;
            T_BT_LINK *p_link;

            p_ind = (T_GAP_SCO_CONN_IND *)p_buf;
            p_link = bt_find_link_by_addr(p_ind->bd_addr);

            if (p_link)
            {
                if (p_link->acl_link_in_sniffmode_fg)
                {
                    legacy_exit_sniff_mode(p_link->bd_addr);
                }

                legacy_cfg_acl_link_policy(p_link->bd_addr, GAP_LINK_POLICY_DISABLE_ALL);
            }

            bt_hfp_handle_sco_conn_ind(p_ind->bd_addr, p_ind->is_esco);
        }
        break;

    case SCO_CONN_CMPL:
        {
            T_GAP_SCO_CONN_CMPL_INFO *p_info;
            T_BT_LINK *p_link;

            p_info = (T_GAP_SCO_CONN_CMPL_INFO *)p_buf;
            p_link = bt_find_link_by_addr(p_info->bd_addr);

            if (p_info->cause)
            {
                APP_PRINT_WARN1("bt_legacy_cb: sco conn fail cause 0x%x", p_info->cause);
                return;
            }

            if (p_link == NULL)
            {
                APP_PRINT_ERROR1("bt_legacy_cb: fail to find link for addr %s", TRACE_BDADDR(p_info->bd_addr));
                legacy_send_sco_disconn_req(p_info->handle);
                return;
            }

            p_link->sco_handle = p_info->handle;
        }
        break;

    case SCO_DISCONN_IND:
        {
            T_GAP_SCO_DISCONN_IND *p_ind = (T_GAP_SCO_DISCONN_IND *)p_buf;
            uint8_t i;
            T_BT_LINK *p_link;

            for (i = 0; i < MAX_LINK_NUM; i++)
            {
                if (bt_data.bt_link[i].sco_handle == p_ind->handle)
                {
                    break;
                }
            }

            if (i < MAX_LINK_NUM)
            {
                p_link = &bt_data.bt_link[i];
                p_link->sco_handle = 0;

                legacy_cfg_acl_link_policy(p_link->bd_addr,
                                           GAP_LINK_POLICY_DISABLE_ALL | GAP_LINK_POLICY_ROLE_SWITCH | GAP_LINK_POLICY_SNIFF_MODE);
            }
        }
        break;

    case ACL_STATUS_INFO:
        bt_handle_acl_status((T_GAP_ACL_STATUS_INFO *)p_buf);
        break;
    case REMOTE_NAME_INFO:
        //==================TODO===============
        //SAVE THE REMOTE NAME
        bt_handle_remote_name((T_GAP_REMOTE_NAME_INFO *)p_buf);
        break;

    case INQUIRY_RESULT:
        bt_app_handle_inquiry_result(p_buf);
        break;

    default:
        break;
    }
}

bool is_gap_stack_ready(void)
{
    return classic_stack_ready;
}

void bt_gap_init(void)
{
    legacy_register_cb(bt_legacy_cb);
}
