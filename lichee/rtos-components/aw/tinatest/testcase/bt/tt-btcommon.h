#ifndef _TT_BTCOMMON_H_
#define _TT_BTCOMMON_H_

#include "bt_manager.h"
#include "btmg_dev_list.h"

extern btmg_callback_t tt_btcbs;
extern dev_list_t *scan_devices;

#define ttbt_printf(fmt, arg...) printf("[tt_bt]" fmt "\e[0m\n", ##arg)

int tt_ble_advertise_on(void);
void tt_bt_adapter_status_cb(btmg_adapter_state_t status);
void tt_bt_scan_status_cb(btmg_scan_state_t status);
void tt_bt_scan_dev_add_cb(btmg_device_t *device);

#endif
