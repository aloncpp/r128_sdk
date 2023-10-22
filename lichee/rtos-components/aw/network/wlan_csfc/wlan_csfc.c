/*#####################################################################
# File Describe:wlan_csfc.c
# Author: flyranchaoflyranchao
# Created Time:flyranchao@allwinnertech.com
# Created Time:2022年10月24日 星期一 11时13分47秒
#====================================================================*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <wlan_csfc.h>
#include <wifimg.h>

#ifdef CONFIG_DRIVERS_XRADIO
#include "wlan.h"
#include "net_init.h"
#include "ethernetif.h"
#include "errno.h"

#endif

#include "sysinfo/sysinfo.h"
#include <sunxi_hal_trng.h>

extern void wlan_mac(int argc, char *argv[]);

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

void wlan_csfc()
{
	int fd;
	int n_read;
	char *read_buf;
	char *p = NULL;
	char *q = NULL;
	char *ssid_buf = NULL;
	char *psk_buf = NULL;
	struct stat file_stat;
	int ret;
	uint8_t mac_addr[6];
	int loop_flag = 0;
	wlan_sta_states_t state;
	wifi_sta_cn_para_t cn_para;
	cn_para.sec = 0;

	//wifimg init
	wifimanager_init();
	//set mac addr by random
	get_mac_by_random(mac_addr);
	sysinfo_set_default_mac(mac_addr);

#ifdef CONFIG_DRIVERS_XRADIO
	//default open wlan sta mode
    ret = wifi_on(WIFI_STATION);
    if (ret)
		printf("wlan sta mode init error.\n");
	//juge ap info is exist
	//exist: fast connect when system start.
	//not exist: enter peiwang mode.
	if(!access("/data/wpa_supplicant.conf", R_OK))
	{
		fd = open("/data/wpa_supplicant.conf", O_RDWR);
		ret = fstat(fd, &file_stat);
		if (ret == -1) {
			printf("get file state failed.\n");
		} else{
			printf("get file size: %d\n", file_stat.st_size);
			read_buf = (char *)malloc(file_stat.st_size + 1);
			memset(read_buf, 0, file_stat.st_size +1);
			n_read = read(fd, read_buf, file_stat.st_size);
			printf("======read %d, context:%s ======\n", n_read, read_buf);
			p = read_buf;
			while ((q = strchr(p, ':')) != NULL){
				if (loop_flag == 0){
					*q = '\0';
					ssid_buf = p;
					cn_para.ssid = ssid_buf;
					printf("===ssid_buf:[%s], len: %d\n", ssid_buf, strlen(ssid_buf));
				}
				if (loop_flag == 1){
					*q = '\0';
					psk_buf = p;
					cn_para.password = psk_buf;
					printf("===psk_buf:[%s], len: %d\n", psk_buf, strlen(psk_buf));
				}
				p = q + 1;
				loop_flag += 1;
			}
			if (p != NULL){
				if ( strcmp(p, "WIFI_SEC_NONE") == 0)
					cn_para.sec = WIFI_SEC_NONE;
				if ( strcmp(p, "WIFI_SEC_WPA_PSK") == 0)
					cn_para.sec = WIFI_SEC_WPA_PSK;
				if ( strcmp(p, "WIFI_SEC_WPA2_PSK") == 0)
					cn_para.sec = WIFI_SEC_WPA2_PSK;
				if ( strcmp(p, "WIFI_SEC_WPA3_PSK") == 0)
					cn_para.sec = WIFI_SEC_WPA3_PSK;
				if ( strcmp(p, "WIFI_SEC_WEP") == 0)
					cn_para.sec = WIFI_SEC_WEP;
				printf("===sec_buf:[%s]\n", p);
			}

			close(fd);
		}
		if (ssid_buf != NULL)
			ret = wifi_sta_connect(&cn_para);
		else
			printf("======wpa_supplicant.conf is exist, but is empty.======\n");

	} else
		printf("======wpa_supplicant.conf is not exist, please connect wifi first.======\n");

#endif

}
