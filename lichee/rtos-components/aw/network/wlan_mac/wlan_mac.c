/*#####################################################################
# File Describe:wlan_mac/wlan_mac.c
# Author: flyranchaoflyranchao
# Created Time:flyranchao@allwinnertech.com
# Created Time:2022年11月14日 星期一 13时28分22秒
#====================================================================*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "sysinfo.h"

#include <wifimg.h>
#include <console.h>

#ifdef CONFIG_DRIVERS_XRADIO
#include "wlan.h"
#include "net_init.h"
#include "ethernetif.h"
#include "tcpip_adapter.h"
#endif

#include <sunxi_hal_trng.h>
#include <sunxi_hal_efuse.h>

static uint8_t char2uint8(char* trs)
{
	uint8_t ret = 0;
	uint8_t tmp_ret[2] = {0};
	int i = 0;
	for(; i < 2; i++) {
		switch (*(trs + i)) {
			case '0' :
				tmp_ret[i] = 0x0;
				break;
			case '1' :
				tmp_ret[i] = 0x1;
				break;
			case '2' :
				tmp_ret[i] = 0x2;
				break;
			case '3' :
				tmp_ret[i] = 0x3;
				break;
			case '4' :
				tmp_ret[i] = 0x4;
				break;
			case '5' :
				tmp_ret[i] = 0x5;
				break;
			case '6' :
				tmp_ret[i] = 0x6;
				break;
			case '7' :
				tmp_ret[i] = 0x7;
				break;
			case '8' :
				tmp_ret[i] = 0x8;
				break;
			case '9' :
				tmp_ret[i] = 0x9;
				break;
			case 'a' :
				tmp_ret[i] = 0xa;
				break;
			case 'b' :
				tmp_ret[i] = 0xb;
				break;
			case 'c' :
				tmp_ret[i] = 0xc;
				break;
			case 'd' :
				tmp_ret[i] = 0xd;
				break;
			case 'e' :
				tmp_ret[i] = 0xe;
				break;
			case 'f' :
				tmp_ret[i] = 0xf;
		break;
	}
	}
	ret = ((tmp_ret[0] << 4) | tmp_ret[1]);
	return ret;
}

static void uint8tochar(char *mac_addr_char, uint8_t *mac_addr_uint8)
{
	sprintf(mac_addr_char,"%02x:%02x:%02x:%02x:%02x:%02x",
						(unsigned char)mac_addr_uint8[0],
						(unsigned char)mac_addr_uint8[1],
						(unsigned char)mac_addr_uint8[2],
						(unsigned char)mac_addr_uint8[3],
						(unsigned char)mac_addr_uint8[4],
						(unsigned char)mac_addr_uint8[5]);
	mac_addr_char[17] = '\0';
}

static void print_help()
{
    printf("=======================================================================\n");
    printf("*************************  get mac addr way  **************************\n");
    printf("=======================================================================\n");
    printf("wifi_get_mac \n");
    printf("\t1: get mac_addr use default data, fe:dc:ba:65:43:21\n");
    printf("\t2: get mac_addr use random data\n");
    printf("\t3: get mac_addr use parameter data, wifi_get_mac  3 44:29:04:b4:78:d6\n");
    printf("\t4: get mac_addr use file data\n");
    printf("\t5: get mac_addr use efuse data\n");
    printf("\t6: get mac_addr use chipid data\n");
    printf("=======================================================================\n");
}

extern struct netif *aw_netif[IF_MAX];

static void get_mac_by_random(uint8_t sysinfo_mac_addr[6])
{
    uint32_t rand[4];
	int i;
	HAL_TRNG_Extract(0, rand);
    for (i = 0; i < 6; ++i) {
        sysinfo_mac_addr[i] = *(uint8_t *)rand + i;
    }
    sysinfo_mac_addr[0] &= 0xFC;
}

static void get_mac_by_chipid(uint8_t sysinfo_mac_addr[6])
{
    int i;
    uint8_t chipid[16];

	hal_efuse_read_ext(0, 96, chipid);
    for (i = 0; i < 2; ++i) {
        sysinfo_mac_addr[i] = chipid[i] ^ chipid[i + 6] ^ chipid[i + 12];
    }
    for (i = 2; i < 6; ++i) {
        sysinfo_mac_addr[i] = chipid[i] ^ chipid[i + 6] ^ chipid[i + 10];
    }
    sysinfo_mac_addr[0] &= 0xFC;
}

static void get_mac_by_efuse(uint8_t sysinfo_mac_addr[6])
{
	hal_efuse_read_ext(128, 96, sysinfo_mac_addr);
}

static int set_mac_addr(uint8_t *mac)
{
	struct sysinfo *sysinfo = sysinfo_get();
	if(sysinfo)
		memcpy(sysinfo->mac_addr, mac, IEEE80211_ADDR_LEN);
	//wlan_set_mac_addr(NULL, mac_addr, IEEE80211_ADDR_LEN);
}

static int rtos_wifimanagre_init = 0;
/*****************************************
 * get mac_addr
 *****************************************
 * 1.use default data: fe:dc:ba:65:43:21
 * 2.use random data.
 * 3.use parameter data.
 * 4.use file data.
 * 5.use efuse data.
 * 6.use chipid data.
 ****************************************/
void wlan_mac(int argc, char *argv[])
{
	int fd;
	int ret;
	int i;
    char *read_buf;
	uint8_t mac_addr[6];
	char mac_addr_char[24] = {0};
	char *pch;

	//wifimg init
	if(!rtos_wifimanagre_init) {
		wifimanager_init();
		rtos_wifimanagre_init = 1;
	}

	//wlan driver init
#ifdef CONFIG_DRIVERS_XRADIO
	int ch = 0;
	if (argv[1] != NULL)
		ch = atoi(argv[1]);
	else{
		print_help();
		return;
	}

	switch(ch) {
		case 1:
			printf("===%s:%d get mac_addr use default value.\n", __func__, __LINE__);
			mac_addr[0] = 0xFE;
			mac_addr[1] = 0xDC;
			mac_addr[2] = 0xBA;
			mac_addr[3] = 0x65;
			mac_addr[4] = 0x43;
			mac_addr[5] = 0x21;
			goto set_mac;
		case 2:
			printf("===%s:%d get mac_addr use random data.===\n", __func__, __LINE__);
			get_mac_by_random(mac_addr);
			goto set_mac;
		case 3:
			printf("===%s:%d get mac_addr use parameter data.===\n", __func__, __LINE__);
			if (argv[2] != NULL)
			{
				pch = strtok(argv[2], ":");
				for(i = 0;(pch != NULL) && (i < 6); i++){
					mac_addr[i] = char2uint8(pch);
					pch= strtok(NULL, ":");
				}
			}
			goto set_mac;
		case 4:
			printf("===%s:%d get mac_addr use file data===\n", __func__, __LINE__);
		    if(!access("/data/xr_wifi.conf", R_OK))
		    {
		        printf("======%s:%d /data/xr_wifi.conf file is existed, get mac_addr. ======\n", __func__, __LINE__);
		        fd = open("/data/xr_wifi.conf", O_RDWR);
		        read_buf = (char *)malloc(18);
		        memset(read_buf, 0, 18);
		        ret = read(fd, read_buf, 18);
		        printf("======%s:%d read %d, file mac_addr:%s ======\n", __func__, __LINE__, ret, read_buf);
				pch = strtok(read_buf, ":");
				for(i = 0;(pch != NULL) && (i < 6); i++){
					mac_addr[i] = char2uint8(pch);
					pch = strtok(NULL, ":");
				}
		        close(fd);
				goto set_mac;
		    }else{
		        printf("======%s:%d /data/xr_wifi.conf is not existed! ======\n", __func__, __LINE__);
		    }
			goto set_mac;
		case 5:
			printf("===%s:%d get mac_addr use efuse data.===\n", __func__, __LINE__);
			get_mac_by_efuse(mac_addr);
			goto set_mac;
		case 6:
			printf("===%s:%d get mac_addr use chipid data.===\n", __func__, __LINE__);
			get_mac_by_chipid(mac_addr);
			goto set_mac;
		default:
			print_help();
			return;
	}

	return;

set_mac:
	//set mac_addr
	set_mac_addr(mac_addr);
	//close wifi first
	xr_wlan_off();
	//default open wlan sta mode
	ret = xr_wlan_on(WLAN_MODE_STA);
	if (ret)
		printf("%s:%d wlan system init error.\n", __func__, __LINE__);
	//sync mac_addr to ifconfig cmd
	uint8tochar(mac_addr_char, mac_addr);
	memcpy(aw_netif[MODE_STA]->hwaddr, mac_addr, NETIF_MAX_HWADDR_LEN);
	printf("%s:%d set mac_addr: %s sucess\n", __func__, __LINE__, mac_addr_char);
#endif

}
FINSH_FUNCTION_EXPORT_CMD(wlan_mac, wifi_get_mac, get wlan_mac test);
