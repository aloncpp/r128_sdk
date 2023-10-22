/*
 * Filename:rtl8723ds.c
 * description: rlt8723ds driver adapter.
 * Created: 2019.07.22
 * Author:laumy
 */

#include <lwip/def.h>
#include <lwip/mem.h>
#include <lwip/stats.h>
#include <lwip/snmp.h>
#include <lwip/etharp.h>
#include <lwip/pbuf.h>
#include <lwip/netif.h>
#include <arch/sys_arch.h>
#include "rtl8723ds.h"
#include "net_stack_intf.h"
#include "arch/net_debug.h"

#define WIFI_DEBUG(fmt,args...) do { printf("AW-WIFI[%s,%d]"fmt,__func__,__LINE__,##args); }while(0)

//#define ETHERNET_SEND_DUMP 1
//#define ETHERNET_RECEIVE_DUMP 1

#define RX_DATA_LEN 1
#define TX_DATA_LEN 1

err_t rtl8723ds_wlan_send(struct netif *netif,struct pbuf *p)
{
	struct eth_drv_sg sg_list[MAX_ETH_DRV_SG];
	int sg_len = 0;
	struct pbuf *q;

#ifdef ETHERNET_SEND_DUMP
	printf("============================================\n");
	for(q=p;q!=NULL;q=q->next) {
		hex_dump(" ",20,q->payload,q->len);
	}
	printf("============================================\n");
#endif
#ifdef TX_DATA_LEN
    aw_dbg_drv_tx_data_len(p->tot_len);
#endif
	if(!rltk_wlan_running(netif_get_idx(netif)))
		return -1;
	for(q=p; q!=NULL && sg_len <MAX_ETH_DRV_SG; q=q->next) {
		sg_list[sg_len].buf = (unsigned int)q->payload;
		sg_list[sg_len++].len = q->len;
	}
	if(sg_len)
		rltk_wlan_send(netif_get_idx(netif),sg_list,sg_len,p->tot_len);
	return 0;
}

void rtl8723ds_wlan_receive(struct netif *netif,int total_len)
{
	int errcode;
	struct eth_drv_sg sg_list[MAX_ETH_DRV_SG];
	struct pbuf *p,*q;
	int sg_len = 0;
	if(!rltk_wlan_running(netif_get_idx(netif)))
		return;
	if((total_len >MAX_ETH_DRV_SG) || (total_len <0))
		total_len = MAX_ETH_DRV_SG;

	p = pbuf_alloc(PBUF_RAW,total_len,PBUF_RAM);
	if(p == NULL) {
		WIFI_DEBUG("\n\rCannot allocate pbfuf to receive packets,%d.\n",__LINE__);
		return;
	}

	for(q=p;q!=NULL && sg_len <MAX_ETH_DRV_SG;q=q->next) {
		sg_list[sg_len].buf = (unsigned int) q->payload;
		sg_list[sg_len++].len = q->len;
	}

	rltk_wlan_recv(netif_get_idx(netif),sg_list,sg_len);
#ifdef RX_DATA_LEN
	aw_dbg_drv_rx_data_len(p->tot_len);
#endif
#ifdef  ETHERNET_RECEIVE_DUMP
	struct eth_hdr* ethhdr;
	ethhdr = (struct eth_hdr *)p->payload;
	switch(htons(ethhdr->type)) {
		case ETHTYPE_IP:
			printf("================IP============================\n");
			for(q=p;q!=NULL;q=q->next) {
				hex_dump(" ",20,q->payload,q->len);
			}
			printf("================IP============================\n");
			break;
		case ETHTYPE_ARP:
			printf("================ARP============================\n");
			printf("receive:total_len:%d,type:%d\n",p->tot_len,p->type);
			for(q=p;q!=NULL;q=q->next) {
				hex_dump(" ",20,q->payload,q->len);
			}
			printf("================ARP============================\n");
			break;
		default:
			printf("nothing .\n");
			break;
	}
#endif
	errcode = netif->input(p,netif);

	if(errcode != 0) {
		WIFI_DEBUG("netif->input errcode,code = %d.\n",errcode);
		pbuf_free(p);
	}
}

int rlt8723ds_wlan_init_statin(struct netif *netif)
{
	netif->hostname = "lwip";

	NETIF_INIT_SNMP(netif, snmp_ifType_ethernet_csmacd, 100);

//	MIB2_INIT_NETIF(netif, snmp_ifType_ethernet_csmacd, LINK_SPEED_OF_YOUR_NETIF_IN_BPS);

	netif->name[0] = 'w';
	netif->name[1] = 'l';
	netif->output = etharp_output;
	netif->linkoutput = rtl8723ds_wlan_send;

	netif->hwaddr_len = HW_ADDR_LEN;
	netif->mtu = 1500;

	netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;

	return 0;
}

int rtl8723ds_wlan_init_ap(struct netif *netif)
{
	;
}
