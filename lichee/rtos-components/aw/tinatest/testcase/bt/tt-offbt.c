#include <stdint.h>
#include <stdio.h>
#include <tinatest.h>
#include "tt-btcommon.h"
#include "bt_manager.h"

int tt_offbt(int argc, char **argv)
{
    btmg_err ret;
    char tt_name[64];
    btmg_adapter_state_t bt_state;
    tname(tt_name);

    ttbt_printf("=======TINATEST FOR %s=========", tt_name);
    ttbt_printf("It's in onff[turn off bt]for tinatest\n");
    ttips("Currently turn off bluetooth test");

    btmg_adapter_get_state(&bt_state);
    if (bt_state == BTMG_ADAPTER_OFF) {
        ttbt_printf("Warn:bt is off, please turn on bt by [tt bt_on]");
        ttbt_printf("=====TINATEST FOR %s FAIL======", tt_name);
        return -1;
    }

    btmg_le_enable_adv(false);
    btmg_adapter_enable(false);
    ret = btmg_core_deinit();
    if (ret == BT_FAIL) {
        ttbt_printf("=====TINATEST FOR %s FAIL======", tt_name);
        return -1;
    }

    btmg_unregister_callback();

    if (scan_devices != NULL) {
        btmg_dev_list_free(scan_devices);
        scan_devices = NULL;
    }

    ttbt_printf("======TINATEST FOR %s OK=======", tt_name);

    return 0;
}

testcase_init(tt_offbt, bt_off, bt turn off for tinatest);
