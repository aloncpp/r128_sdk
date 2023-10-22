/*
 * Filename:wlan_rf.h
 * description: wlan rfkill manager.
 * Created: 2019.07.22
 * Author:laumy
 */
#ifndef _WLAN_RF_H
#define _WLAN_RF_H

#include <stdbool.h>

int wlan_rf_probe(void);

int wlan_rf_remove(void);

int wlan_get_bus_index(void);

int wlan_set_power(bool on_off);


#endif


