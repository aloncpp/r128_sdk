/*
 * Filename:rtl8723ds.h
 * description: rlt8723ds driver adapter.
 * Created: 2019.07.22
 * Author:laumy
 */

#ifndef _RTL8723DS_H
#define _RTL8723DS_H

#define HW_ADDR_LEN 6

#define MAX_ETH_DRV_SG 1540

err_t rtl8723ds_wlan_send(struct netif *netif,struct pbuf *p);
void rtl8723ds_wlan_receive(struct netif *netif,int total_len);
int rlt8723ds_wlan_init_statin(struct netif *net_if);
int rtl8723ds_wlan_init_ap(struct netif *netif);

#endif
