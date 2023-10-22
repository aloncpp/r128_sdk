/* mbed Microcontroller Library
 * Copyright (c) 2013-2016 Realtek Semiconductor Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//#define _NET_STACK_INTF_C_

//#include <autoconf.h>
#include <net_stack_intf.h>

#define CONFIG_PLATFOMR_CUSTOMER_RTOS 1
//#include <lwip_intf.h>
#if (CONFIG_LWIP_LAYER == 1)
#include <lwip/netif.h>
#endif
#if !defined(CONFIG_MBED_ENABLED) && !defined(CONFIG_PLATFOMR_CUSTOMER_RTOS)
#include <lwip_netconf.h>
#include <ethernetif.h>
#endif
#include <osdep_service.h>
#include <wifi/wifi_util.h>
#include <net_stack_intf.h>
//----- ------------------------------------------------------------------
// External Reference
//----- ------------------------------------------------------------------
#if (CONFIG_LWIP_LAYER == 1)
#ifdef CONFIG_PLATFOMR_CUSTOMER_RTOS
extern struct netif *xnetif[];
#else
extern struct netif xnetif[];			//LWIP netif
#endif
//struct netif xnetif[];			//LWIP netif
#endif

/**
 *      rltk_wlan_set_netif_info - set netif hw address and register dev pointer to netif device
 *      @idx_wlan: netif index
 *			    0 for STA only or SoftAP only or STA in STA+SoftAP concurrent mode,
 *			    1 for SoftAP in STA+SoftAP concurrent mode
 *      @dev: register netdev pointer to LWIP. Reserved.
 *      @dev_addr: set netif hw address
 *
 *      Return Value: None
 */
void rltk_wlan_set_netif_info(int idx_wlan, void * dev, unsigned char * dev_addr)
{
#if (CONFIG_LWIP_LAYER == 1)
#if defined(CONFIG_MBED_ENABLED)
	//rtw_memcpy(xnetif[idx_wlan]->hwaddr, dev_addr, 6);
	//set netif hwaddr later
#else
#ifdef CONFIG_PLATFOMR_CUSTOMER_RTOS
	rtw_memcpy(xnetif[idx_wlan]->hwaddr, dev_addr, 6);
	xnetif[idx_wlan]->state = dev;
#else
	rtw_memcpy(xnetif[idx_wlan].hwaddr, dev_addr, 6);
	xnetif[idx_wlan].state = dev;
#endif
#endif
#endif
}

/*
static void hex_dump(char *pref, int width, unsigned char *buf, int len)
{
	int i,n;
    for (i = 0, n = 1; i < len; i++, n++){
        if (n == 1)
            printf("%s", pref);
        printf("%2.2X ", buf[i]);
        if (n == width) {
            printf("\n");
            n = 0;
        }
    }
    if (i && n!=1)
        printf("\n");
}*/


/**
 *      rltk_wlan_send - send IP packets to WLAN. Called by low_level_output().
 *      @idx: netif index
 *      @sg_list: data buffer list
 *      @sg_len: size of each data buffer
 *      @total_len: total data len
 *
 *      Return Value: None
 */
int rltk_wlan_send(int idx, struct eth_drv_sg *sg_list, int sg_len, int total_len)
{
#if (CONFIG_LWIP_LAYER == 1)
	struct eth_drv_sg *last_sg;
	struct sk_buff *skb = NULL;
	int ret = 0;

	if(idx == -1){
		DBG_ERR("netif is DOWN");
		return -1;
	}
	DBG_TRACE("%s is called", __FUNCTION__);

#ifndef CONFIG_AW_PLATFORM
	save_and_cli();
#else
	uint32_t x;
	x = save_and_cli();
#endif
	if(rltk_wlan_check_isup(idx))
		rltk_wlan_tx_inc(idx);
	else {
		//DBG_EyRR("netif is DOWN");
#ifndef CONFIG_AW_PLATFORM
		restore_flags();
#else
		restore_flags(x);
#endif
		return -1;
	}
#ifndef CONFIG_AW_PLATFORM
	restore_flags();
#else
	restore_flags(x);
#endif
	skb = rltk_wlan_alloc_skb(total_len);
	if (skb == NULL) {
		//DBG_ERR("rltk_wlan_alloc_skb() for data len=%d failed!", total_len);
		ret = -1;
		goto exit;
	}

	for (last_sg = &sg_list[sg_len]; sg_list < last_sg; ++sg_list) {
		rtw_memcpy(skb->tail, (void *)(sg_list->buf), sg_list->len);
		skb_put(skb,  sg_list->len);
	}

	/*
	struct sk_buff *skb_test = NULL;
	for(skb_test = skb; skb_test != NULL; skb_test=skb_test->next) {
		hex_dump("rltk_wlan_send: ", 20, skb->data, skb->len);
	}*/
	rltk_wlan_send_skb(idx, skb);

exit:
#ifndef CONFIG_AW_PLATFORM
	save_and_cli();
	rltk_wlan_tx_dec(idx);
	restore_flags();
#else
	x = save_and_cli();
	rltk_wlan_tx_dec(idx);
	restore_flags(x);
#endif
	return ret;
#endif
}

/**
 *      rltk_wlan_recv - indicate packets to LWIP. Called by ethernetif_recv().
 *      @idx: netif index
 *      @sg_list: data buffer list
 *      @sg_len: size of each data buffer
 *
 *      Return Value: None
 */
void rltk_wlan_recv(int idx, struct eth_drv_sg *sg_list, int sg_len)
{
#if (CONFIG_LWIP_LAYER == 1)
	struct eth_drv_sg *last_sg;
	struct sk_buff *skb;

	DBG_TRACE("%s is called", __FUNCTION__);
	if(idx == -1){
		DBG_ERR("skb is NULL");
		return;
	}
	skb = rltk_wlan_get_recv_skb(idx);
	DBG_ASSERT(skb, "No pending rx skb");

	for (last_sg = &sg_list[sg_len]; sg_list < last_sg; ++sg_list) {
		if (sg_list->buf != 0) {
			rtw_memcpy((void *)(sg_list->buf), skb->data, sg_list->len);
			skb_pull(skb, sg_list->len);
		}
	}
#endif
}

int netif_is_valid_IP(int idx, unsigned char *ip_dest)
{
#if defined(CONFIG_MBED_ENABLED)
	return 1;
#else
#if CONFIG_LWIP_LAYER == 1
#ifdef CONFIG_PLATFOMR_CUSTOMER_RTOS
	struct netif * pnetif = xnetif[idx];
#else
	struct netif * pnetif = &xnetif[idx];
#endif
	ip_addr_t addr = { 0 };

#ifdef CONFIG_MEMORY_ACCESS_ALIGNED
	unsigned int temp;
	memcpy(&temp, ip_dest, sizeof(unsigned int));
	u32_t *ip_dest_addr = &temp;
#else
	u32_t *ip_dest_addr  = (u32_t*)ip_dest;
#endif

#if LWIP_VERSION_MAJOR >= 2
	ip_addr_set_ip4_u32(&addr, *ip_dest_addr);
#else
	addr.addr = *ip_dest_addr;
#endif

#if LWIP_VERSION_MAJOR >= 2
	if((ip_addr_get_ip4_u32(netif_ip_addr4(pnetif))) == 0)
		return 1;
#else

	if(pnetif->ip_addr.addr == 0)
		return 1;
#endif

	if(ip_addr_ismulticast(&addr) || ip_addr_isbroadcast(&addr,pnetif)){
		return 1;
	}

	//if(ip_addr_netcmp(&(pnetif->ip_addr), &addr, &(pnetif->netmask))) //addr&netmask
	//	return 1;

	if(ip_addr_cmp(&(pnetif->ip_addr),&addr))
		return 1;

	DBG_TRACE("invalid IP: %d.%d.%d.%d ",ip_dest[0],ip_dest[1],ip_dest[2],ip_dest[3]);
#endif
#ifdef CONFIG_DONT_CARE_TP
	if(pnetif->flags & NETIF_FLAG_IPSWITCH)
		return 1;
	else
#endif
	return 0;
#endif
}

#if !defined(CONFIG_MBED_ENABLED)
int netif_get_idx(struct netif* pnetif)
{
#if CONFIG_LWIP_LAYER == 1
#ifdef CONFIG_PLATFOMR_CUSTOMER_RTOS
	int idx = pnetif - xnetif[0];
#else
	int idx = pnetif - xnetif;
#endif
	switch(idx) {
	case 0:
		return 0;
	case 1:
		return 1;
	default:
		return -1;
	}
#else
	return -1;
#endif
}

unsigned char *netif_get_hwaddr(int idx_wlan)
{
#if (CONFIG_LWIP_LAYER == 1)
#ifdef CONFIG_PLATFOMR_CUSTOMER_RTOS
	return xnetif[idx_wlan]->hwaddr;
#else
	return xnetif[idx_wlan].hwaddr;
#endif
#else
	return NULL;
#endif
}
#endif

#ifdef CONFIG_PLATFOMR_CUSTOMER_RTOS
extern void rtl8723ds_wlan_receive(struct netif *netif,int total_len);
void ethernetif_recv(struct netif *netif,int total_len)
{
	rtl8723ds_wlan_receive(netif,total_len);
}
#endif

void netif_rx(int idx, unsigned int len)
{
#if (CONFIG_LWIP_LAYER == 1)
#if defined(CONFIG_MBED_ENABLED)
	wlan_emac_recv(NULL, len);
#else
#ifdef CONFIG_PLATFOMR_CUSTOMER_RTOS
	ethernetif_recv(xnetif[idx], len);
#else
	ethernetif_recv(&xnetif[idx], len);
#endif
#endif
#endif
#if (CONFIG_INIC_EN == 1)
        inic_netif_rx(idx, len);
#endif
}

void netif_post_sleep_processing(void)
{
#if (CONFIG_LWIP_LAYER == 1)
#if defined(CONFIG_MBED_ENABLED)
#else
#ifdef CONFIG_PLATFOMR_CUSTOMER_RTOS
	// to do
#else
	lwip_POST_SLEEP_PROCESSING();	//For FreeRTOS tickless to enable Lwip ARP timer when leaving IPS - Alex Fang
#endif
#endif
#endif
}

void netif_pre_sleep_processing(void)
{
#if (CONFIG_LWIP_LAYER == 1)
#if defined(CONFIG_MBED_ENABLED)
#else
#ifdef CONFIG_PLATFOMR_CUSTOMER_RTOS
	//todo
#else
	lwip_PRE_SLEEP_PROCESSING();
#endif
#endif
#endif
}

#ifdef CONFIG_WOWLAN
unsigned char *rltk_wlan_get_ip(int idx){
#if (CONFIG_LWIP_LAYER == 1)
#ifdef CONFIG_PLATFOMR_CUSTOMER_RTOS
	return LwIP_GetIP(xnetif[idx]);
#else
	return LwIP_GetIP(&xnetif[idx]);
#endif
#else
	return NULL;
#endif
}
#endif
