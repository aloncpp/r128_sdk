// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include <string.h>

#include "btc/btc_storage.h"
#include "btc/btc_util.h"
#include "osi/osi.h"
#include "common/bt_trace.h"
#if 0 //origin
#include "xr_system.h"
#else //zipmodi
#endif
#include "bta/bta_api.h"
#include "device/bdaddr.h"
#include "btc/btc_config.h"

/*******************************************************************************
**
** Function         btc_storage_add_bonded_device
**
** Description      BTIF storage API - Adds the newly bonded device to NVRAM
**                  along with the link-key, Key type and Pin key length
**
** Returns          BT_STATUS_SUCCESS if the store was successful,
**                  BT_STATUS_FAIL otherwise
**
*******************************************************************************/

bt_status_t btc_storage_add_bonded_device(bt_bdaddr_t *remote_bd_addr,
        LINK_KEY link_key,
        uint8_t key_type,
        uint8_t pin_length,
        BOOLEAN sc_support,
        BD_NAME *bd_name)
{
    bdstr_t bdstr;

    bdaddr_to_string(remote_bd_addr, bdstr, sizeof(bdstr));
    BTC_TRACE_DEBUG("add to storage: Remote device:%s\n", bdstr);

    btc_config_lock();
    int ret = btc_config_set_int(bdstr, BTC_STORAGE_LINK_KEY_TYPE_STR, (int)key_type);
    ret &= btc_config_set_int(bdstr, BTC_STORAGE_PIN_LENGTH_STR, (int)pin_length);
    ret &= btc_config_set_bin(bdstr, BTC_STORAGE_LINK_KEY_STR, link_key, sizeof(LINK_KEY));
    ret &= btc_config_set_bin(bdstr, BTC_STORAGE_SC_SUPPORT, (uint8_t *)&sc_support, sizeof(sc_support));
    ret &= btc_config_set_str(bdstr, BTC_STORAGE_DEV_NAME, (uint8_t *)bd_name);
    /* write bonded info immediately */
    btc_config_flush();
    btc_config_unlock();

    BTC_TRACE_DEBUG("Storage add rslt %d\n", ret);
    return ret ? BT_STATUS_SUCCESS : BT_STATUS_FAIL;
}

/*******************************************************************************
**
** Function         btc_in_fetch_bonded_devices
**
** Description      Internal helper function to fetch the bonded devices
**                  from NVRAM
**
** Returns          BT_STATUS_SUCCESS if successful, BT_STATUS_FAIL otherwise
**
*******************************************************************************/
static bt_status_t btc_in_fetch_bonded_devices(int add)
{
    BOOLEAN bt_linkkey_file_found = FALSE;
    UINT8 sc_support = 0;
    btc_config_lock();
    for (const btc_config_section_iter_t *iter = btc_config_section_begin(); iter != btc_config_section_end(); iter = btc_config_section_next(iter)) {
        const char *name = btc_config_section_name(iter);
        if (!string_is_bdaddr(name)) {
            continue;
        }

        BTC_TRACE_DEBUG("Remote device:%s\n", name);
        LINK_KEY link_key;
        size_t size = sizeof(link_key);
        if (btc_config_get_bin(name, BTC_STORAGE_LINK_KEY_STR, link_key, &size)) {
            int linkkey_type;
            if (btc_config_get_int(name, BTC_STORAGE_LINK_KEY_TYPE_STR, &linkkey_type)) {
                bt_bdaddr_t bd_addr;
                string_to_bdaddr(name, &bd_addr);
                if (add) {
                    DEV_CLASS dev_class = {0, 0, 0};
                    int cod;
                    int pin_length = 0;
                    if (btc_config_get_int(name, BTC_STORAGE_DEV_CLASS_STR, &cod)) {
                        uint2devclass((UINT32)cod, dev_class);
                    }
                    btc_config_get_int(name, BTC_STORAGE_PIN_LENGTH_STR, &pin_length);
                    size = sizeof(sc_support);
                    btc_config_get_bin(name, BTC_STORAGE_SC_SUPPORT, &sc_support, &size);
                    BD_NAME bd_name;
                    int i_size = sizeof(bd_name);
                    btc_config_get_str(name, BTC_STORAGE_DEV_NAME, bd_name, &i_size);
#if (SMP_INCLUDED == TRUE)
                    BTA_DmAddDevice(bd_addr.address, dev_class, link_key, 0, 0,
                                    (UINT8)linkkey_type, 0, pin_length, (UINT8)sc_support, bd_name);
#endif  ///SMP_INCLUDED == TRUE
                }
                bt_linkkey_file_found = TRUE;
            } else {
                BTC_TRACE_ERROR("bounded device:%s, LinkKeyType or PinLength is invalid\n", name);
            }
        }
        if (!bt_linkkey_file_found) {
            BTC_TRACE_DEBUG("Remote device:%s, no link key\n", name);
        }
    }
    btc_config_unlock();

    return BT_STATUS_SUCCESS;
}


/*******************************************************************************
**
** Function         btc_storage_load_bonded_devices
**
** Description      BTC storage API - Loads all the bonded devices from NVRAM
**                  and adds to the BTA.
**                  Additionally, this API also invokes the adaper_properties_cb
**                  and remote_device_properties_cb for each of the bonded devices.
**
** Returns          BT_STATUS_SUCCESS if successful, BT_STATUS_FAIL otherwise
**
*******************************************************************************/
bt_status_t btc_storage_load_bonded_devices(void)
{
    bt_status_t status;
    status = btc_in_fetch_bonded_devices(1);
    BTC_TRACE_DEBUG("Storage load rslt %d\n", status);
    return status;
}

/*******************************************************************************
**
** Function         btc_storage_remove_bonded_device
**
** Description      BTC storage API - Deletes the bonded device from NVRAM
**
** Returns          BT_STATUS_SUCCESS if the deletion was successful,
**                  BT_STATUS_FAIL otherwise
**
*******************************************************************************/
bt_status_t btc_storage_remove_bonded_device(bt_bdaddr_t *remote_bd_addr)
{
    bdstr_t bdstr;
    bdaddr_to_string(remote_bd_addr, bdstr, sizeof(bdstr));
    int ret = 1;
    BTC_TRACE_DEBUG("Add to storage: Remote device:%s\n", bdstr);

    btc_config_lock();
    if (btc_config_exist(bdstr, BTC_STORAGE_LINK_KEY_TYPE_STR)) {
        ret &= btc_config_remove(bdstr, BTC_STORAGE_LINK_KEY_TYPE_STR);
    }
    if (btc_config_exist(bdstr, BTC_STORAGE_PIN_LENGTH_STR)) {
        ret &= btc_config_remove(bdstr, BTC_STORAGE_PIN_LENGTH_STR);
    }
    if (btc_config_exist(bdstr, BTC_STORAGE_LINK_KEY_STR)) {
        ret &= btc_config_remove(bdstr, BTC_STORAGE_LINK_KEY_STR);
    }
    if (btc_config_exist(bdstr, BTC_STORAGE_SC_SUPPORT)) {
        ret &= btc_config_remove(bdstr, BTC_STORAGE_SC_SUPPORT);
    }
    if (btc_config_exist(bdstr, BTC_STORAGE_DEV_NAME)) {
        ret &= btc_config_remove(bdstr, BTC_STORAGE_DEV_NAME);
    }
    /* write bonded info immediately */
    btc_config_flush();
    btc_config_unlock();

    return ret ? BT_STATUS_SUCCESS : BT_STATUS_FAIL;
}

/*******************************************************************************
**
** Function         btc_storage_get_num_bt_bond_devices
**
** Description      BTC storage API - get the num of the bonded device from NVRAM
**
** Returns          the num of the bonded device
**
*******************************************************************************/
int btc_storage_get_num_bt_bond_devices(void)
{
    int num_dev = 0;

    btc_config_lock();
    for (const btc_config_section_iter_t *iter = btc_config_section_begin(); iter != btc_config_section_end();
            iter = btc_config_section_next(iter)) {
        const char *name = btc_config_section_name(iter);
        if (string_is_bdaddr(name) &&
            btc_config_exist(name, BTC_STORAGE_LINK_KEY_TYPE_STR) &&
            btc_config_exist(name, BTC_STORAGE_PIN_LENGTH_STR) &&
            btc_config_exist(name, BTC_STORAGE_SC_SUPPORT) &&
            btc_config_exist(name, BTC_STORAGE_DEV_NAME) &&
            btc_config_exist(name, BTC_STORAGE_LINK_KEY_STR)) {
            num_dev++;
        }
    }
    btc_config_unlock();
    return num_dev;
}

/*******************************************************************************
**
** Function         btc_storage_get_bonded_bt_devices_list
**
** Description      BTC storage API - get the list of the bonded device from NVRAM
**
** Returns          BT_STATUS_SUCCESS if get the list successful,
**                  BT_STATUS_FAIL otherwise
**
*******************************************************************************/
bt_status_t btc_storage_get_bonded_bt_devices_list(bt_bdaddr_t *bond_dev_addr, bt_bdname_t *bond_dev_name, int *dev_num)
{
    bt_bdaddr_t bd_addr;
    bt_bdname_t bd_name;
    int in_dev_num = *dev_num; /* buffer size */
    int out_dev_num = 0; /* bond_dev size */

    btc_config_lock();
    for (const btc_config_section_iter_t *iter = btc_config_section_begin(); iter != btc_config_section_end();
            iter = btc_config_section_next(iter)) {

        if (in_dev_num <= 0) {
            break;
        }

        const char *name = btc_config_section_name(iter);

        if (string_is_bdaddr(name) &&
            btc_config_exist(name, BTC_STORAGE_LINK_KEY_TYPE_STR) &&
            btc_config_exist(name, BTC_STORAGE_PIN_LENGTH_STR) &&
            btc_config_exist(name, BTC_STORAGE_SC_SUPPORT) &&
            btc_config_exist(name, BTC_STORAGE_DEV_NAME) &&
            btc_config_exist(name, BTC_STORAGE_LINK_KEY_STR)) {
            string_to_bdaddr(name, &bd_addr);
            memcpy(bond_dev_addr, &bd_addr, sizeof(bt_bdaddr_t));
            int i_size = sizeof(bd_name);
            btc_config_get_str(name, BTC_STORAGE_DEV_NAME, bd_name.name, &i_size);
            memcpy(bond_dev_name, &bd_name, sizeof(bt_bdname_t));
            in_dev_num--;
            out_dev_num++;
            bond_dev_addr++;
            bond_dev_name++;
        }
    }
    *dev_num = out_dev_num; /* out_dev_num <= in_dev_num */
    btc_config_unlock();

    return BT_STATUS_SUCCESS;
}
