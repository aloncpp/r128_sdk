/*#####################################################################
# File Describe:wlan_country_code/wlan_country_code.c
# Author: flyranchaoflyranchao
# Created Time:flyranchao@allwinnertech.com
# Created Time:2023年05月31日 星期一 14时32分22秒
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
#include "wlan_country_code.h"

#include <wifimg.h>
#include <console.h>

#ifdef CONFIG_DRIVERS_XRADIO
#include "wlan.h"
#include "wlan_ext_req.h"
#include "net_init.h"
#include "ethernetif.h"
#include "tcpip_adapter.h"
#endif

static int max_chan = 0;
static COUNTRY_CODE_E g_country_code = COUNTRY_CODE_NONE;
int cn_scan_freq_list[] = {2412, 2417, 2422, 2427, 2432, 2437, 2442, 2447, 2452, 2457, 2462, 2467, 2472};      //CN 1-13
int us_scan_freq_list[] = {2412, 2417, 2422, 2427, 2432, 2437, 2442, 2447, 2452, 2457, 2462};                  //US  1-11
int jp_scan_freq_list[] = {2412, 2417, 2422, 2427, 2432, 2437, 2442, 2447, 2452, 2457, 2462, 2467, 2472, 2484};//JP  1-14
int eu_scan_freq_list[] = {2412, 2417, 2422, 2427, 2432, 2437, 2442, 2447, 2452, 2457, 2462, 2467, 2472};      //EU  1-13
int kr_scan_freq_list[] = {2412, 2417, 2422, 2427, 2432, 2437, 2442, 2447, 2452, 2457, 2462, 2467, 2472};      //KR  1-13
int ca_scan_freq_list[] = {2412, 2417, 2422, 2427, 2432, 2437, 2442, 2447, 2452, 2457, 2462};                  //JP  1-11
int br_scan_freq_list[] = {2412, 2417, 2422, 2427, 2432, 2437, 2442, 2447, 2452, 2457, 2462, 2467, 2472};      //BR  1-13
int au_scan_freq_list[] = {2412, 2417, 2422, 2427, 2432, 2437, 2442, 2447, 2452, 2457, 2462};                  //AU  1-11
int de_scan_freq_list[] = {2412, 2417, 2422, 2427, 2432, 2437, 2442, 2447, 2452, 2457, 2462, 2467, 2472};      //DE  1-13
int other_scan_freq_list[] = {2412, 2417, 2422, 2427, 2432, 2437, 2442, 2447, 2452, 2457, 2462, 2467, 2472};   //NONE  1-13
extern struct netif *g_wlan_netif;

static void print_help()
{
    printf("=======================================================================\n");
    printf("*************************  set/get country code  **********************\n");
    printf("=======================================================================\n");
    printf("wifi_set_countrycode parameter\n");
    printf("                     0: COUNTRY_CODE_CN(中国     1-13)\n");
    printf("                     1: COUNTRY_CODE_US(美国     1-11)\n");
    printf("                     2: COUNTRY_CODE_JP(日本     1-14)\n");
    printf("                     3: COUNTRY_CODE_EU(欧洲     1-13)\n");
    printf("                     4: COUNTRY_CODE_KR(韩国     1-13)\n");
    printf("                     5: COUNTRY_CODE_CA(加拿大   1-11)\n");
    printf("                     6: COUNTRY_CODE_BR(巴西     1-13)\n");
    printf("                     7: COUNTRY_CODE_AU(澳大利亚 1-11)\n");
    printf("                     8: COUNTRY_CODE_DE(德国     1-13)\n");
    printf("                     x: COUNTRY_CODE_NONE(其他区 1-13)\n");
    printf("wifi_get_countrycode \n");
    printf("=======================================================================\n");
}

int wifi_set_eur_enable(int enable)
{
    int ret = 0;
    int eur_enable;
    eur_enable = enable;
    if (g_wlan_netif == NULL) {
        printf("%s, %d, warning: netif is null\n", __func__, __LINE__);
        return -1;
    }

    ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_EUR_CE, eur_enable);
    if (ret == -2) {
        printf("invalid arg\n");
    } else if (ret == -1) {
        printf("exec failed\n");
    }
    return ret;
}

int wifi_set_scan_freq(wlan_ext_scan_freq_t *scan_freq)
{
    int ret = 0;
    if (g_wlan_netif == NULL) {
        printf("%s, %d, warning: netif is null\n", __func__, __LINE__);
        return -1;
    }

    ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_SCAN_FREQ, (uint32_t)scan_freq);
    if (ret == -2) {
        printf("invalid arg\n");
    } else if (ret == -1) {
        printf("exec failed\n");
    }

    return ret;
}

int wifi_set_countrycode(COUNTRY_CODE_E countrycode)
{
    int ret = 0;
    wlan_ext_scan_freq_t scan_freq;
    memset(&scan_freq,0,sizeof(wlan_ext_scan_freq_t));

    switch(countrycode) {
        case COUNTRY_CODE_CN:
			printf("===set COUNTRY_CODE_CN 中国===\n");
            scan_freq.freq_num = 13;
            scan_freq.freq_list = &cn_scan_freq_list[0];
            max_chan = sizeof(cn_scan_freq_list)/sizeof(cn_scan_freq_list[0]);
            wifi_set_eur_enable(1);
			g_country_code = COUNTRY_CODE_CN;
			break;
        case COUNTRY_CODE_US:
			printf("===set COUNTRY_CODE_US 美国===\n");
            scan_freq.freq_num = 11;
            scan_freq.freq_list = &us_scan_freq_list[0];
            max_chan = sizeof(us_scan_freq_list)/sizeof(us_scan_freq_list[0]);
			g_country_code = COUNTRY_CODE_US;
			break;
        case COUNTRY_CODE_JP:
			printf("===set COUNTRY_CODE_JP 日本===\n");
            scan_freq.freq_num = 14;
            scan_freq.freq_list = &jp_scan_freq_list[0];
            max_chan = sizeof(jp_scan_freq_list)/sizeof(jp_scan_freq_list[0]);
			g_country_code = COUNTRY_CODE_JP;
			break;
        case COUNTRY_CODE_EU:
			printf("===set COUNTRY_CODE_EU 欧洲===\n");
            scan_freq.freq_num = 13;
            scan_freq.freq_list = &eu_scan_freq_list[0];
            max_chan = sizeof(eu_scan_freq_list)/sizeof(eu_scan_freq_list[0]);
            wifi_set_eur_enable(1);
			g_country_code = COUNTRY_CODE_EU;
			break;
        case COUNTRY_CODE_KR:
			printf("===set COUNTRY_CODE_KR 韩国===\n");
            scan_freq.freq_num = 13;
            scan_freq.freq_list = &kr_scan_freq_list[0];
            max_chan = sizeof(kr_scan_freq_list)/sizeof(kr_scan_freq_list[0]);
            wifi_set_eur_enable(1);
			g_country_code = COUNTRY_CODE_KR;
			break;
        case COUNTRY_CODE_CA:
			printf("===set COUNTRY_CODE_CA 加拿大===\n");
            scan_freq.freq_num = 11;
            scan_freq.freq_list = &ca_scan_freq_list[0];
            max_chan = sizeof(ca_scan_freq_list)/sizeof(ca_scan_freq_list[0]);
            wifi_set_eur_enable(1);
			g_country_code = COUNTRY_CODE_CA;
			break;
        case COUNTRY_CODE_BR:
			printf("===set COUNTRY_CODE_BR 巴西===\n");
            scan_freq.freq_num = 13;
            scan_freq.freq_list = &br_scan_freq_list[0];
            max_chan = sizeof(br_scan_freq_list)/sizeof(br_scan_freq_list[0]);
            wifi_set_eur_enable(1);
			g_country_code = COUNTRY_CODE_BR;
			break;
        case COUNTRY_CODE_AU:
			printf("===set COUNTRY_CODE_AU 澳大利亚===\n");
            scan_freq.freq_num = 11;
            scan_freq.freq_list = &au_scan_freq_list[0];
            max_chan = sizeof(au_scan_freq_list)/sizeof(au_scan_freq_list[0]);
            wifi_set_eur_enable(1);
			g_country_code = COUNTRY_CODE_AU;
			break;
        case COUNTRY_CODE_DE:
			printf("===set COUNTRY_CODE_DE 德国===\n");
            scan_freq.freq_num = 13;
            scan_freq.freq_list = &de_scan_freq_list[0];
            max_chan = sizeof(de_scan_freq_list)/sizeof(de_scan_freq_list[0]);
            wifi_set_eur_enable(1);
			g_country_code = COUNTRY_CODE_DE;
			break;
        default:
			printf("===set COUNTRY_CODE_NONE 默认===\n");
            scan_freq.freq_num = 13;
            scan_freq.freq_list = &other_scan_freq_list[0];
            max_chan = sizeof(other_scan_freq_list)/sizeof(other_scan_freq_list[0]);
			break;
    }

    ret = wifi_set_scan_freq(&scan_freq);

    return ret;
}

int wifi_get_countrycode()
{
    switch(g_country_code) {
        case COUNTRY_CODE_CN:
			printf("===current COUNTRY_CODE_CN 中国===\n");
			break;
        case COUNTRY_CODE_US:
			printf("===current COUNTRY_CODE_US 美国===\n");
			break;
        case COUNTRY_CODE_JP:
			printf("===current COUNTRY_CODE_JP 日本===\n");
			break;
        case COUNTRY_CODE_EU:
			printf("===current COUNTRY_CODE_EU 欧洲===\n");
			break;
        case COUNTRY_CODE_KR:
			printf("===current COUNTRY_CODE_KR 韩国===\n");
			break;
        case COUNTRY_CODE_CA:
			printf("===current COUNTRY_CODE_CA 加拿大===\n");
			break;
        case COUNTRY_CODE_BR:
			printf("===current COUNTRY_CODE_BR 巴西===\n");
			break;
        case COUNTRY_CODE_AU:
			printf("===current COUNTRY_CODE_AU 澳大利亚===\n");
			break;
        case COUNTRY_CODE_DE:
			printf("===current COUNTRY_CODE_DE 德国===\n");
			break;
        default:
			printf("===current COUNTRY_CODE_NONE 默认===\n");
			break;
    }
}

void wlan_set_countrycode(int argc, char *argv[])
{
	int ch = 0;
	if (argv[1] != NULL)
		ch = atoi(argv[1]);
	else{
		print_help();
		return;
	}
	switch(ch) {
		case 0:
			wifi_set_countrycode(COUNTRY_CODE_CN);
			break;
		case 1:
			wifi_set_countrycode(COUNTRY_CODE_US);
			break;
		case 2:
			wifi_set_countrycode(COUNTRY_CODE_JP);
			break;
		case 3:
			wifi_set_countrycode(COUNTRY_CODE_EU);
			break;
		case 4:
			wifi_set_countrycode(COUNTRY_CODE_KR);
			break;
		case 5:
			wifi_set_countrycode(COUNTRY_CODE_CA);
			break;
		case 6:
			wifi_set_countrycode(COUNTRY_CODE_BR);
			break;
		case 7:
			wifi_set_countrycode(COUNTRY_CODE_AU);
			break;
		case 8:
			wifi_set_countrycode(COUNTRY_CODE_DE);
			break;
		default:
			wifi_set_countrycode(COUNTRY_CODE_NONE);
			break;
	}
}

void wlan_get_countrycode(int argc, char *argv[])
{
	wifi_get_countrycode();
}

FINSH_FUNCTION_EXPORT_CMD(wlan_set_countrycode, wifi_set_countrycode, set country code test);
FINSH_FUNCTION_EXPORT_CMD(wlan_get_countrycode, wifi_get_countrycode, get country code test);
