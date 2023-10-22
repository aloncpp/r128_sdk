#include <string.h>

#include "xr_gap_bt_api.h"
#include "xr_bt_defs.h"
#include "btmg_log.h"
#include "btmg_common.h"
#include "bt_manager.h"

/* discovery state */
static int disc_state = XR_BT_GAP_DISCOVERY_STOPPED;

char bda_str[18] = { 0 };
static xr_bd_addr_t bd_addr = { 0, 0, 0, 0, 0, 0 };

static void bt_gap_cb(xr_bt_gap_cb_event_t event, xr_bt_gap_cb_param_t *param)
{
    switch (event) {
    case XR_BT_GAP_DISC_STATE_CHANGED_EVT: {
        if (btmg_cb_p[CB_MAIN] && btmg_cb_p[CB_MAIN]->btmg_adapter_cb.scan_status_cb) {
            if (param->disc_st_chg.state == XR_BT_GAP_DISCOVERY_STARTED) {
                disc_state = XR_BT_GAP_DISCOVERY_STARTED;
                btmg_cb_p[CB_MAIN]->btmg_adapter_cb.scan_status_cb(BTMG_SCAN_STARTED);
            } else if (param->disc_st_chg.state == XR_BT_GAP_DISCOVERY_STOPPED) {
                if (disc_state == XR_BT_GAP_DISCOVERY_STARTED) {
                    disc_state = XR_BT_GAP_DISCOVERY_STOPPED;
                    btmg_cb_p[CB_MAIN]->btmg_adapter_cb.scan_status_cb(BTMG_SCAN_STOPPED);
                }
            }
        }
        break;
    }
    case XR_BT_GAP_DISC_RES_EVT: {
        btmg_device_t device;
        char *dev_name = "NULL";
        int device_pos = 0;
        uint32_t cod = 0;
        xr_bt_gap_dev_prop_t *p;
        int8_t rssi = 0;

        bda2str(param->disc_res.bda, bda_str);
        for (int i = 0; i < param->disc_res.num_prop; i++) {
            p = param->disc_res.prop + i;
            switch (p->type) {
            case XR_BT_GAP_DEV_PROP_COD:
                cod = *(uint32_t *)(p->val);
                break;
            case XR_BT_GAP_DEV_PROP_RSSI:
                rssi = *(int8_t *)(p->val);
                break;
            case XR_BT_GAP_DEV_PROP_BDNAME:
                dev_name = (char *)(p->val);
                break;
            default:
                break;
            }
        }
        device.address = bda_str;
        device.name = dev_name;
        device.cod = cod;
        device.rssi = rssi;

        if (btmg_cb_p[CB_MAIN] && btmg_cb_p[CB_MAIN]->btmg_device_cb.device_add_cb) {
            btmg_cb_p[CB_MAIN]->btmg_device_cb.device_add_cb(&device);
        }
        break;
    }
#if 0
    case XR_BT_GAP_RMT_SRVCS_EVT: {
        int uuid_num = param->rmt_srvcs.num_uuids;
        xr_bt_uuid_t *uuid_list;

        if (uuid_num == 0) {
            BTMG_INFO("no uuid found for device[%02x:%02x:%02x:%02x:%02x:%02x]\n ",
                      param->rmt_srvcs.bda[0], param->rmt_srvcs.bda[1], param->rmt_srvcs.bda[2],
                      param->rmt_srvcs.bda[3], param->rmt_srvcs.bda[4], param->rmt_srvcs.bda[5]);
        } else {
            uuid_list = param->rmt_srvcs.uuid_list;

            BTMG_INFO("uuid list for device[%02x:%02x:%02x:%02x:%02x:%02x]\n",
                      param->rmt_srvcs.bda[0], param->rmt_srvcs.bda[1], param->rmt_srvcs.bda[2],
                      param->rmt_srvcs.bda[3], param->rmt_srvcs.bda[4], param->rmt_srvcs.bda[5]);

            for (int i = 0; i < uuid_num; i++, uuid_list++) {
                switch (uuid_list->len) {
                case 2:
                    BTMG_INFO("uuid16: 0x%04x\n", uuid_list->uuid.uuid16);
                    break;
                case 4:
                    BTMG_INFO("uuid32: 0x%08x\n", uuid_list->uuid.uuid32);
                    break;
                case 16:
                    /* TO-DO: translate uuid128 and print it*/
                    break;
                }
            }
        }
        break;
    }
#endif
    case XR_BT_GAP_PIN_REQ_EVT: {
        bool pin_digit = param->pin_req.min_16_digit;
        if (pin_digit) {
            BTMG_INFO("need to enter 16 digital pin code");
        } else {
            BTMG_INFO("need to enter 4 digital pin code");
        }
        if (btmg_cb_p[CB_MAIN] && btmg_cb_p[CB_MAIN]->btmg_device_cb.pindcoe_request_cb) {
            btmg_cb_p[CB_MAIN]->btmg_device_cb.pindcoe_request_cb(bda_str);
        }
        break;
    }
#if CONFIG_BT_SSP_ENABLE || CONFIG_BT_SSP_ENABLED
    case XR_BT_GAP_AUTH_CMPL_EVT: {
        bda2str(param->auth_cmpl.bda, bda_str);
        if (param->auth_cmpl.stat == XR_BT_STATUS_SUCCESS) {
            BTMG_DEBUG("authentication success");
            if (strlen((const char *)param->auth_cmpl.device_name) != 0) {
                BTMG_DEBUG("device_name[%s] \n", param->auth_cmpl.device_name);
            }
            if (btmg_cb_p[CB_MAIN] && btmg_cb_p[CB_MAIN]->btmg_adapter_cb.bonded_device_cb) {
                btmg_cb_p[CB_MAIN]->btmg_adapter_cb.bonded_device_cb(BTMG_BOND_STATE_BONDED, bda_str);
            }
        } else {
            BTMG_DEBUG("authentication failed, status:%d", param->auth_cmpl.stat);
            if (btmg_cb_p[CB_MAIN] && btmg_cb_p[CB_MAIN]->btmg_adapter_cb.bonded_device_cb) {
                btmg_cb_p[CB_MAIN]->btmg_adapter_cb.bonded_device_cb(BTMG_BOND_STATE_BOND_FAILED,
                                                            bda_str);
            }
        }
        break;
    }
    case XR_BT_GAP_CFM_REQ_EVT:
        //BTMG_DEBUG("XR_BT_GAP_CFM_REQ_EVT Please compare the numeric value: %d\n", param->cfm_req.num_val);
        memcpy(bd_addr, param->key_req.bda, sizeof(xr_bd_addr_t));
        bda2str(param->key_req.bda, bda_str);
        if (btmg_cb_p[CB_MAIN] && btmg_cb_p[CB_MAIN]->btmg_device_cb.pairing_confirm_cb) {
            btmg_cb_p[CB_MAIN]->btmg_device_cb.pairing_confirm_cb(bda_str, param->cfm_req.num_val);
        }
        break;
    case XR_BT_GAP_KEY_NOTIF_EVT:
        //BTMG_DEBUG("XR_BT_GAP_KEY_NOTIF_EVT, peer device send passkey:%d\n", param->key_notif.passkey);
        memcpy(bd_addr, param->key_req.bda, sizeof(xr_bd_addr_t));
        bda2str(param->key_req.bda, bda_str);
        if (btmg_cb_p[CB_MAIN] && btmg_cb_p[CB_MAIN]->btmg_device_cb.passkey_confirm_cb) {
            btmg_cb_p[CB_MAIN]->btmg_device_cb.passkey_confirm_cb(bda_str, param->cfm_req.num_val);
        }
        break;
    case XR_BT_GAP_KEY_REQ_EVT:
        //BTMG_DEBUG("XR_BT_GAP_KEY_REQ_EVT Please enter passkey!\n");
        memcpy(bd_addr, param->key_req.bda, sizeof(xr_bd_addr_t));
        bda2str(param->key_req.bda, bda_str);
        if (btmg_cb_p[CB_MAIN] && btmg_cb_p[CB_MAIN]->btmg_device_cb.passkey_request_cb) {
            btmg_cb_p[CB_MAIN]->btmg_device_cb.passkey_request_cb(bda_str);
        }
        break;
#endif ///CONFIG_BT_SSP_ENABLE || CONFIG_BT_SSP_ENABLED
    case XR_BT_GAP_READ_REMOTE_NAME_EVT:
        if (param->read_rmt_name.stat == XR_BT_STATUS_SUCCESS) {
            if (btmg_cb_p[CB_MAIN] && btmg_cb_p[CB_MAIN]->btmg_device_cb.get_name_cb) {
                btmg_cb_p[CB_MAIN]->btmg_device_cb.get_name_cb(param->read_rmt_name.rmt_name);
            }
        } else {
            BTMG_ERROR("read remote name failed, status:%d", param->read_rmt_name.stat);
        }
        break;
    case XR_BT_GAP_READ_LOCAL_NAME_EVT:
        if (btmg_cb_p[CB_MAIN] && btmg_cb_p[CB_MAIN]->btmg_adapter_cb.get_name_cb) {
            btmg_cb_p[CB_MAIN]->btmg_adapter_cb.get_name_cb(param->read_local_name.name);
        }
        break;
    case XR_BT_GAP_REMOVE_BOND_DEV_COMPLETE_EVT: {
        bda2str(param->remove_bond_dev_cmpl.bda, bda_str);
        if (btmg_cb_p[CB_MAIN] && btmg_cb_p[CB_MAIN]->btmg_adapter_cb.bonded_device_cb) {
            if (param->remove_bond_dev_cmpl.status == XR_BT_STATUS_SUCCESS) {
                btmg_cb_p[CB_MAIN]->btmg_adapter_cb.bonded_device_cb(BTMG_BOND_STATE_UNBONDED, bda_str);
            }
        }
        break;
    }
    default:
        break;
    }

    return;
}

btmg_err bt_gap_init(void)
{
    xr_err_t ret;

    if ((ret = xr_bt_gap_register_callback(bt_gap_cb)) != XR_OK) {
        BTMG_ERROR("return failed: %d", ret);
        return BT_FAIL;
    }

    return BT_OK;
}

btmg_err bt_gap_deinit(void)
{
    BTMG_DEBUG("enter");

    return BT_OK;
}

/* discovery state */
static int scan_state = BTMG_SCAN_STARTED;

btmg_err bt_gap_scan_start(void)
{
    xr_err_t ret;
    int scan_num = 0;
    int scan_time = 0x30; //0x30*1.28s = 61.44s
    int scan_mode = XR_BT_INQ_MODE_GENERAL_INQUIRY;

    /* note: the max device num rsp is 5, which is defined in bt_target.h */
    if ((ret = xr_bt_gap_start_discovery(scan_mode, scan_time, scan_num)) != XR_OK) {
        BTMG_ERROR("start discovery return failed: %d", ret);
        return BT_FAIL;
    }

    return BT_OK;
}

btmg_err bt_gap_scan_stop(void)
{
    xr_err_t ret;

    if ((ret = xr_bt_gap_cancel_discovery()) != XR_OK) {
        BTMG_ERROR("stop discovery return failed: %d", ret);
        return BT_FAIL;
    }

    return BT_OK;
}

btmg_err bt_gap_set_scan_mode(btmg_scan_mode_t mode)
{
    if (BTMG_SCAN_MODE_NONE == mode) {
        BTMG_DEBUG("Scan mode:Discoverable(no),Connectable(no)");
        if (xr_bt_gap_set_scan_mode(XR_BT_NON_CONNECTABLE, XR_BT_NON_DISCOVERABLE,
                                    XR_BT_NON_PAIRABLE, XR_BT_ONLY_CON_PAIRABLE) != XR_OK)
            goto fail;
    } else if (BTMG_SCAN_MODE_CONNECTABLE == mode) {
        BTMG_DEBUG("Scan mode:Discoverable(no),Connectable(yes)");
        if (xr_bt_gap_set_scan_mode(XR_BT_CONNECTABLE, XR_BT_NON_DISCOVERABLE, XR_BT_PAIRABLE,
                                    XR_BT_ONLY_CON_PAIRABLE) != XR_OK)
            goto fail;
    } else if (BTMG_SCAN_MODE_CONNECTABLE_DISCOVERABLE == mode) {
        BTMG_DEBUG("Scan mode:Discoverable(yes),Connectable(yes)");
        if (xr_bt_gap_set_scan_mode(XR_BT_CONNECTABLE, XR_BT_GENERAL_DISCOVERABLE, XR_BT_PAIRABLE,
                                    XR_BT_ALL_PAIRABLE) != XR_OK)
            goto fail;
    }

    return BT_OK;

fail:
    BTMG_ERROR("set scan mode fail");

    return BT_FAIL;
}

btmg_err bt_gap_get_device_rssi(const char *addr)
{
    xr_bd_addr_t remote_bda = {0};

    str2bda(addr, remote_bda);
    return xr_bt_gap_read_rssi_delta(remote_bda);
}

btmg_err bt_gap_set_io_capability(btmg_io_capability_t io_cap)
{
    xr_err_t ret;
    xr_bt_io_cap_t iocap;

    if (BTMG_IO_CAP_DISPLAYONLY == io_cap) {
        iocap = XR_BT_IO_CAP_OUT;
    } else if (BTMG_IO_CAP_DISPLAYYESNO == io_cap) {
        iocap = XR_BT_IO_CAP_IO;
    } else if (BTMG_IO_CAP_KEYBOARDONLY == io_cap) {
        iocap = XR_BT_IO_CAP_IN;
    } else if (BTMG_IO_CAP_NOINPUTNOOUTPUT == io_cap) {
        iocap = XR_BT_IO_CAP_NONE;
    } else {
        return BT_FAIL;
    }

    xr_bt_sp_param_t param_type = XR_BT_SP_IOCAP_MODE;
    if ((ret = xr_bt_gap_set_security_param(param_type, &iocap, sizeof(uint8_t))) != XR_OK) {
        BTMG_ERROR("set io capability fail");
        return BT_FAIL;
    }

    return BT_OK;
}

btmg_err bt_gap_pincode_reply(char *pincode)
{
    int i = 0;
    bool digit = false;

    xr_bt_pin_type_t pin_type = XR_BT_PIN_TYPE_VARIABLE;
    xr_bt_pin_code_t pin_code;

    //digit is 1,pin is 16 digit. gap_req.digitis 0,pin is 4.
    for (i = 4 + 12 * digit - 1; i >= 0; i--) {
        if (*(pincode + i) < '0' || *(pincode + i) > '9') {
            BTMG_ERROR("invalid pin code");
            return BT_ERR_INVALID_ARG;
        }

        pin_code[i] = *(pincode + i);
    }

    return xr_bt_gap_pin_reply(bd_addr, true, 4 + 12 * digit, pin_code);
}

btmg_err bt_gap_ssp_passkey_reply(uint32_t passkey)
{
    if (passkey > 999999) {
        BTMG_ERROR("Passkey should be between 0-999999");
        return BT_ERR_INVALID_ARG;
    }

    return xr_bt_gap_ssp_passkey_reply(bd_addr, true, passkey);
}

btmg_err bt_gap_ssp_passkey_confirm(uint32_t passkey)
{
    return xr_bt_gap_ssp_passkey_reply(bd_addr, true, passkey);
}

btmg_err bt_gap_pairing_confirm(void)
{
    return xr_bt_gap_ssp_confirm_reply(bd_addr, true);
}

btmg_err bt_gap_get_device_name(const char *addr)
{
    xr_bd_addr_t remote_bda = {0};

    str2bda(addr, remote_bda);

    return xr_bt_gap_read_remote_name(remote_bda);
}

btmg_err bt_gap_remove_bond_device(const char *addr)
{
    xr_bd_addr_t bda = {0};

    str2bda(addr, bda);

    return xr_bt_gap_remove_bond_device(bda);
}

int bt_gap_get_bond_device_num(void)
{
    return xr_bt_gap_get_bond_device_num();
}

btmg_err bt_gap_get_bond_device_list(int device_num, btmg_paired_device_t *paired_list)
{
    xr_err_t ret;
    char bda_str[18];
    xr_bd_addr_t *devices_addr;
    xr_bd_name_t *devices_name;

    if (device_num <= 0) {
        BTMG_ERROR("device_num should be greater than 0!");
        return BT_ERR_INVALID_ARG;
    }

    devices_addr = malloc(device_num * sizeof(xr_bd_addr_t));
    if (devices_addr == NULL) {
        return BT_ERR_NO_MEMORY;
    }

    devices_name = malloc(device_num * sizeof(xr_bd_name_t));
    if (devices_name == NULL) {
        free(devices_addr);
        return BT_ERR_NO_MEMORY;
    }

    ret = xr_bt_gap_get_bond_device_list(&device_num, devices_addr, devices_name);
    if (ret != XR_OK) {
        BTMG_ERROR("return failed: %d", ret);
        free(devices_name);
        free(devices_addr);
        return BT_FAIL;
    }

    for (int i = 0; i < device_num; i++) {
        bda2str(devices_addr[i], bda_str);
        memcpy((paired_list + i)->address, bda_str, 18);
        memcpy((paired_list + i)->name, devices_name[i], 248);
    }
    free(devices_name);
    free(devices_addr);

    return BT_OK;
}
