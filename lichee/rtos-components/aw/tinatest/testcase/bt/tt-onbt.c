#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <tinatest.h>
#include "tt-btcommon.h"
#include "bt_manager.h"
#include "btmg_dev_list.h"

btmg_callback_t tt_btcbs;
dev_list_t *scan_devices = NULL;

int tt_onbt(int argc, char **argv)
{
    btmg_err ret;
    btmg_adapter_state_t bt_state;
    char tt_name[64];
    tname(tt_name);

    ttbt_printf("=======TINATEST FOR %s=========", tt_name);
    ttbt_printf("It's in onbt[turn on bt]for tinatest");
    ttips("Currently turn on bluetooth test");

    btmg_adapter_get_state(&bt_state);

    if (bt_state == BTMG_ADAPTER_ON) {
        ttbt_printf("Warn:bt is on, please turn off bt by [tt bt_off]");
        ttbt_printf("=====TINATEST FOR %s FAIL======", tt_name);
        return -1;
    }

    tt_btcbs.btmg_adapter_cb.state_cb = tt_bt_adapter_status_cb;
    tt_btcbs.btmg_adapter_cb.scan_status_cb = tt_bt_scan_status_cb;
    tt_btcbs.btmg_device_cb.device_add_cb = tt_bt_scan_dev_add_cb;

    btmg_set_loglevel(BTMG_LOG_LEVEL_WARNG);
    btmg_core_init();
    btmg_register_callback(&tt_btcbs);
    btmg_set_profile(BTMG_A2DP_SOURCE | BTMG_GATT_CLIENT);
    ret = btmg_adapter_enable(true);

    tt_ble_advertise_on();
    ttbt_printf("BLE advertise on!");

    if (ret == BT_FAIL) {
        ttbt_printf("=====TINATEST FOR %s FAIL======", tt_name);
        return -1;
    }

    ttbt_printf("=====TINATEST FOR %s OK======", tt_name);

    return 0;
}

testcase_init(tt_onbt, bt_on, bt turn on for tinatest);
