/*
 * Filename:bt_rf.h
 * description: bluetooth rfkill manager.
 * Created: 2019.07.22
 * Author:laumy
 */
#ifndef _BT_RF_H
#define _BT_RF_H

#include <stdbool.h>

int bt_rf_probe(void);

int bt_rf_remove(void);

int bt_set_power(bool on_off);

int bt_set_rfkill_state(bool on_off);

int bt_rfkill_state_reset(void);

#endif


