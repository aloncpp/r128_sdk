#ifndef _WLAN_IF_H
#define _WLAN_IF_H

err_t wlan_if_init_ap(struct netif *netif);

err_t wlan_if_init_station(struct netif *netif);

void wlan_if_recv(struct netif *netif,int total_len);

int wlan_if_detect(void);

#endif
