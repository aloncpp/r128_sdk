/*
 * Filename: tcpip_adapter.c
 * description: tcp ip stack init
 * Created: 2019.07.22
 * Author:laumy
 */

#include <stdio.h>
#include <stdbool.h>
#include <lwip/tcpip.h>
#include <lwip/netif.h>
#include "wlan_if.h"
#include <stdlib.h>
#include <string.h>

#include "tcpip_adapter.h"
#include "console.h"

#define DEBUG(fmt,args...) do { printf("tcpip_adapter[%s,%d]"fmt,__func__,__LINE__,##args); }while(0)
#define INFO(fmt,args...) do { printf("tcpip_adapter[%s,%d]"fmt,__func__,__LINE__,##args); }while(0)
#define ERROR(fmt,args...) do { printf("tcpip_adapter[%s,%d]"fmt,__func__,__LINE__,##args); }while(0)
#if 0
static struct netif net_if;
#endif
struct netif *aw_netif[IF_MAX];

int8_t netif_ifconfig(void)
{
	int i;
	for(i=0; i<IF_MAX; i++) {
		if(aw_netif[i]) {
			printf("%c%c%u\t%s\n \
\t%s%02"X16_F":%02"X16_F":%02"X16_F":%02"X16_F":%02"X16_F":%02"X16_F"\n \
\t%s%"U16_F".%"U16_F".%"U16_F".%"U16_F"\n \
\t%s%"U16_F".%"U16_F".%"U16_F".%"U16_F"\n \
\t%s%"U16_F".%"U16_F".%"U16_F".%"U16_F"\n \
\t%s %s %s %s%d\n",
				aw_netif[i]->name[0],aw_netif[i]->name[1],aw_netif[i]->num,
				"Link encap:Ethernet",
				"HWaddr ",
				aw_netif[i]->hwaddr[0], aw_netif[i]->hwaddr[1], aw_netif[i]->hwaddr[2],
				aw_netif[i]->hwaddr[3], aw_netif[i]->hwaddr[4], aw_netif[i]->hwaddr[5],
				"inet addr ",
				ip4_addr1_16(netif_ip4_addr(aw_netif[i])),
				ip4_addr2_16(netif_ip4_addr(aw_netif[i])),
				ip4_addr3_16(netif_ip4_addr(aw_netif[i])),
				ip4_addr4_16(netif_ip4_addr(aw_netif[i])),
				"gw addr ",
			ip4_addr1_16(netif_ip4_gw(aw_netif[i])),
			ip4_addr2_16(netif_ip4_gw(aw_netif[i])),
			ip4_addr3_16(netif_ip4_gw(aw_netif[i])),
			ip4_addr4_16(netif_ip4_gw(aw_netif[i])),
				"netmask ",
				ip4_addr1_16(netif_ip4_netmask(aw_netif[i])),
				ip4_addr2_16(netif_ip4_netmask(aw_netif[i])),
				ip4_addr3_16(netif_ip4_netmask(aw_netif[i])),
				ip4_addr4_16(netif_ip4_netmask(aw_netif[i])),
				((aw_netif[i]->flags & NETIF_FLAG_UP) == NETIF_FLAG_UP) ? "UP" : "DOWN",
				((aw_netif[i]->flags & NETIF_FLAG_BROADCAST) == NETIF_FLAG_BROADCAST) ? "BROADCAST" : " ",
				((aw_netif[i]->flags & NETIF_FLAG_LINK_UP) == NETIF_FLAG_LINK_UP) ? "LINK_UP" : "LINK_DOWN",
				" mtu: ",aw_netif[i]->mtu);
		}
	}

	return 0;
}

FINSH_FUNCTION_EXPORT_CMD(netif_ifconfig,ifconfig, Console network config Command);

static int8_t ethernetif_init(if_type_t mode,if_info_t *info,uint8_t *mac)
{
	if(mode > IF_MAX || info == NULL) {
		return -1;
	}
	if(aw_netif[mode] == NULL ) {
#if 1
		aw_netif[mode] = calloc(1,sizeof(*aw_netif[mode])); //if free ?
		if(aw_netif[mode] == NULL) {
			ERROR("interface resource request failed\n");
			return -1;
		}
		aw_netif[mode]->name[0] = 'e';
		aw_netif[mode]->name[1] = 'n';
		aw_netif[mode]->num = mode;
#else
		aw_netif[mode] = &net_if;
#endif
	}
	if(mac != NULL) {
		memcpy(aw_netif[mode]->hwaddr, mac, NETIF_MAX_HWADDR_LEN);
	}

#ifdef CONFIG_DRIVERS_WIRELESS
	if(mode == MODE_STA) {
		netif_add(aw_netif[mode],&info->ip,&info->netmask,
					&info->gw,NULL,wlan_if_init_station,tcpip_input);
	}else if(mode == MODE_AP) {
		netif_add(aw_netif[mode],&info->ip,&info->netmask,
					&info->gw,NULL,wlan_if_init_ap,tcpip_input);
	}else {
		ERROR("net interface mode un supported.\n");
		goto failed;
	}
#else
#warning "should select DRIVERS_WIRELESS!"
	goto failed;
#endif

	netif_set_default(aw_netif[mode]);
	return 0;

failed:
	if(aw_netif[mode] != NULL)
		free(aw_netif[mode]);
	return -1;
}

struct netif* get_netif(if_type_t mode)
{
	if(aw_netif[mode] != NULL) {
		return aw_netif[mode];
	}
	ERROR("net interface not init.\n");
	return NULL;
}

void set_netif(if_type_t mode, struct netif *netif)
{
    if (mode >= IF_MAX) {
        printf("unsupported interface mode: %d\n", mode);
        return;
    }

    aw_netif[mode] = netif;
}

static bool tcpip_enable = false;
int8_t tcpip_stack_init(if_type_t mode,if_info_t *info,uint8_t *mac)
{

	if(tcpip_enable == false) {
		tcpip_init(NULL,NULL);
		tcpip_enable = true;
	}
	return ethernetif_init(mode,info,mac);
}

int get_mac_hwaddr(unsigned char *addr, int len)
{
    struct netif* nif = get_netif(MODE_AP);

    if (!nif) {
        return -1;
    }

#ifndef MIN
#define MIN(x, y)   ((x) < (y) ? (x) : (y))
#endif
    memcpy(addr, nif->hwaddr, MIN(sizeof(nif->hwaddr), len));
    return 1;
}

void cmd_tcpip_init(void)
{
	if_info_t net_info;
	IP4_ADDR(&net_info.ip,0,0,0,0);
	IP4_ADDR(&net_info.netmask,0,0,0,0);
	IP4_ADDR(&net_info.gw,0,0,0,0);
	tcpip_stack_init(MODE_STA,&net_info,NULL);
	printf("======%s->%d tcpip init successful!======\n", __func__, __LINE__);
}

FINSH_FUNCTION_EXPORT_CMD(cmd_tcpip_init,tcpip, Console tcpip init test Command);
