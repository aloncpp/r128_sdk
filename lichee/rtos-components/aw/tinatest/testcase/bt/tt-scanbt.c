#include <stdint.h>
#include <stdio.h>
#include <tinatest.h>
#include <stdbool.h>
#include "tt-btcommon.h"
#include "bt_manager.h"
#include "btmg_dev_list.h"

int tt_scanbt(int argc, char **argv)
{
    btmg_err ret;
    int sleep_ms = 1000 * 10;
    dev_node_t *dev_node = NULL;
    char tt_name[64];
    btmg_adapter_state_t bt_state;

    tname(tt_name);

    ttbt_printf("=======TINATEST FOR %s=========", tt_name);
    ttbt_printf("It's in onff[turn off bt]for tinatest");
    ttips("Currently bluetooth scan test");

    btmg_adapter_get_state(&bt_state);
    if (bt_state != BTMG_ADAPTER_ON) {
        ttbt_printf("Warn:bt is not turned on, please turn on bt by [tt bt_on]");
        ttbt_printf("=====TINATEST FOR %s FAIL======", tt_name);
        return -1;
    }

    if (scan_devices != NULL) {
        btmg_dev_list_free(scan_devices);
        scan_devices = NULL;
    }

    scan_devices = btmg_dev_list_new();
    if (scan_devices == NULL) {
        ttbt_printf("=====TINATEST FOR %s FAIL======", tt_name);
        return -1;
    }

    ret = btmg_adapter_start_scan();
    if (ret == BT_FAIL) {
        ttbt_printf("=====TINATEST FOR %s FAIL======", tt_name);
        return -1;
    }

    ttbt_printf("Start scanning devices for 10 seconds\n");
    usleep(sleep_ms * 1000);
    ret = btmg_adapter_stop_scan();
    if (ret == BT_FAIL) {
        ttbt_printf("=====TINATEST FOR %s FAIL======", tt_name);
        return -1;
    }

    if (scan_devices == NULL) {
        ttbt_printf("Scan list is empty");
        ttbt_printf("=====TINATEST FOR %s FAIL======", tt_name);
        return -1;
    }

    usleep(1000 * 1000);
    ttbt_printf("==========Scanned Device List==========\n");
    dev_node = scan_devices->head;
    while (dev_node != NULL) {
        ttbt_printf("addr: %s, name: %s", dev_node->dev_addr, dev_node->dev_name);
        dev_node = dev_node->next;
    }

    ttbt_printf("=====TINATEST FOR %s OK======", tt_name);

    return 0;
}

testcase_init(tt_scanbt, bt_scan, bt scan for tinatest);
