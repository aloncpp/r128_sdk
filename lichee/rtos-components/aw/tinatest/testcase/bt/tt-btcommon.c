#include <stdio.h>
#include <tinatest.h>
#include "tt-btcommon.h"
#include "bt_manager.h"
#include "btmg_dev_list.h"

static int tt_ble_set_adv_data(void)
{
    int index = 0;
    uint16_t uuid = 0x1234;
    btmg_adv_scan_rsp_data_t adv_data;

    adv_data.data[index] = 0x02; /* flag len */
    adv_data.data[index + 1] = 0x01; /* type for flag */
    adv_data.data[index + 2] = 0x1A; //0x05

    /* AD Type: Complete local name */
    /* reply in broadcast scan reply packet*/
    char ble_name[] = "aw-tinatest-ble";
    index += adv_data.data[index] + 1;
    adv_data.data[index] = strlen(ble_name) + 1; /* name len */
    adv_data.data[index + 1] = 0x09; /* type for local name */
    int name_len;
    name_len = strlen(ble_name);
    strcpy(&(adv_data.data[index + 2]), ble_name);

    index += adv_data.data[index] + 1;
    adv_data.data[index] = 0x03; /* uuid len */
    adv_data.data[index + 1] = 0x03; /* type for complete list of 16-bit uuid */
    adv_data.data[index + 2] = (char)(uuid & 0xFF);
    adv_data.data[index + 3] = (char)((uuid >> 8) & 0xFF);
    index += adv_data.data[index] + 1;

    adv_data.data_len = index;

    return btmg_le_set_adv_scan_rsp_data(&adv_data, NULL);
}

int tt_ble_advertise_on(void)
{
    btmg_le_adv_param_t adv_param;

    adv_param.interval_min = 0x0020;
    adv_param.interval_max = 0x01E0;
    adv_param.adv_type = BTMG_LE_ADV_IND;

    btmg_le_set_adv_param(&adv_param);
    tt_ble_set_adv_data();

    return btmg_le_enable_adv(true);
}

void tt_bt_adapter_status_cb(btmg_adapter_state_t status)
{
    char bt_addr[18] = { 0 };
    char bt_name_buf[64] = { 0 };
    char bt_name[64] = { 0 };

    if (status == BTMG_ADAPTER_OFF) {
        ttips("BT is off");
    } else if (status == BTMG_ADAPTER_ON) {
        ttips("BT is on");
        btmg_adapter_get_address(bt_addr);
        if (bt_addr[0] != '\0') {
            snprintf(bt_name_buf, 12, "aw-btcli-%s-", (char *)(bt_addr + 12));
            sprintf(bt_name, "%s-%s", bt_name_buf, (char *)(bt_addr + 15));
            btmg_adapter_set_name(bt_name);
        } else {
            btmg_adapter_set_name("aw-btcli");
        }
        btmg_adapter_set_io_capability(BTMG_IO_CAP_NOINPUTNOOUTPUT);
        btmg_adapter_set_scanmode(BTMG_SCAN_MODE_CONNECTABLE_DISCOVERABLE);
        ttbt_printf("BT Adapter name: %s ", bt_name);
    } else if (status == BTMG_ADAPTER_TURNING_ON) {
        ttbt_printf("BT is turnning on");
    } else if (status == BTMG_ADAPTER_TURNING_OFF) {
        ttbt_printf("BT is turnning off");
    }
}

void tt_bt_scan_status_cb(btmg_scan_state_t status)
{
    if (status == BTMG_SCAN_STARTED) {
        ttips("start bt scan\n");
    } else if (status == BTMG_SCAN_STOPPED) {
        ttips("stop bt scan\n");
    }
}

void tt_bt_scan_dev_add_cb(btmg_device_t *device)
{
    dev_node_t *dev_node = NULL;

    ttbt_printf("name:[%s]\t\taddress:[%s]\t\tclass:[%d]\t\trssi:[%d]", device->name, device->address,
            device->cod, device->rssi);

    dev_node = btmg_dev_list_find_device(scan_devices, device->address);
    if (dev_node != NULL) {
        return;
    }

    btmg_dev_list_add_device(scan_devices, device->name, device->address, 0);
}
