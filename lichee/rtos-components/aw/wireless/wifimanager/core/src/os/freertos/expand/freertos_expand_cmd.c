#include <wifi_log.h>
#include <wifimg.h>
#include <stdlib.h>
#include <string.h>
#include <freertos_common.h>
#include "sysinfo.h"

#ifdef CONFIG_DRIVERS_XRADIO
#include "wlan.h"
#include "wlan_ext_req.h"
#include "net_init.h"
#include "ethernetif.h"
#include "tcpip_adapter.h"
extern struct netif *aw_netif[IF_MAX];

#endif
typedef enum {
    COUNTRY_CODE_CN,  //中国
    COUNTRY_CODE_US,  //美国
    COUNTRY_CODE_JP,  //日本
    COUNTRY_CODE_EU,  //欧洲
    COUNTRY_CODE_KR,  //韩国
    COUNTRY_CODE_CA,  //加拿大
    COUNTRY_CODE_BR,  //巴西
    COUNTRY_CODE_AU,  //澳大利亚
    COUNTRY_CODE_DE,  //德国
    COUNTRY_CODE_NONE //其他
} COUNTRY_CODE_E;

static int wifi_set_countrycode(COUNTRY_CODE_E countrycode);
static int wifi_get_countrycode();

static int max_chan = 0;
static COUNTRY_CODE_E g_country_code = COUNTRY_CODE_NONE;
static int cn_scan_freq_list[] = {2412, 2417, 2422, 2427, 2432, 2437, 2442, 2447, 2452, 2457, 2462, 2467, 2472};      //CN 1-13
static int us_scan_freq_list[] = {2412, 2417, 2422, 2427, 2432, 2437, 2442, 2447, 2452, 2457, 2462};                  //US  1-11
static int jp_scan_freq_list[] = {2412, 2417, 2422, 2427, 2432, 2437, 2442, 2447, 2452, 2457, 2462, 2467, 2472, 2484};//JP  1-14
static int eu_scan_freq_list[] = {2412, 2417, 2422, 2427, 2432, 2437, 2442, 2447, 2452, 2457, 2462, 2467, 2472};      //EU  1-13
static int kr_scan_freq_list[] = {2412, 2417, 2422, 2427, 2432, 2437, 2442, 2447, 2452, 2457, 2462, 2467, 2472};      //KR  1-13
static int ca_scan_freq_list[] = {2412, 2417, 2422, 2427, 2432, 2437, 2442, 2447, 2452, 2457, 2462};                  //JP  1-11
static int br_scan_freq_list[] = {2412, 2417, 2422, 2427, 2432, 2437, 2442, 2447, 2452, 2457, 2462, 2467, 2472};      //BR  1-13
static int au_scan_freq_list[] = {2412, 2417, 2422, 2427, 2432, 2437, 2442, 2447, 2452, 2457, 2462};                  //AU  1-11
static int de_scan_freq_list[] = {2412, 2417, 2422, 2427, 2432, 2437, 2442, 2447, 2452, 2457, 2462, 2467, 2472};      //DE  1-13
static int other_scan_freq_list[] = {2412, 2417, 2422, 2427, 2432, 2437, 2442, 2447, 2452, 2457, 2462, 2467, 2472};   //NONE  1-13
extern struct netif *g_wlan_netif;

static void print_help()
{
    printf("=======================================================================\n");
    printf("wifi -e \"freertos: setcountrycode: n\"\n");
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
    printf("wifi -e \"freertos: getcountrycode\"\n");
    printf("=======================================================================\n");
}

static int wifi_set_eur_enable(int enable)
{
#if 0
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
#endif
}

static int wifi_set_scan_freq(wlan_ext_scan_freq_t *scan_freq)
{
    int ret = 0;
    if (g_wlan_netif == NULL) {
        printf("%s, %d, warning: netif is null\n", __func__, __LINE__);
        return -1;
    }

    ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_SCAN_FREQ, (uint32_t)(intptr_t)scan_freq);
    if (ret == -2) {
        printf("invalid arg\n");
    } else if (ret == -1) {
        printf("exec failed\n");
    }

    return ret;
}

static int wifi_set_countrycode(COUNTRY_CODE_E countrycode)
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

static int wifi_get_countrycode()
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

wmg_status_t wmg_freertos_send_expand_set_countrycode(char *expand_cmd, void *expand_cb)
{
    int ch = 0;
	print_help();
	ch = atoi(expand_cmd);
	printf("ch:%d===\n", ch);
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
	return WMG_STATUS_SUCCESS;
}

wmg_status_t wmg_freertos_send_expand_get_countrycode(char *expand_cmd, void *expand_cb)
{
    wifi_get_countrycode();
	return WMG_STATUS_SUCCESS;
}


wmg_status_t wmg_freertos_send_expand_cmd_gmac(char *expand_cmd, void *expand_cb)
{
	char ifname[10] = {0};
	uint8_t *mac_addr = (uint8_t *)expand_cb;
	struct sysinfo *sysinfo = sysinfo_get();

	strcpy(ifname, expand_cmd);
	if(ifname == NULL) {
		WMG_ERROR("ifname is NULL\n");
		return WMG_STATUS_FAIL;
	}

	if(sysinfo){
		memcpy(mac_addr, sysinfo->mac_addr, IEEE80211_ADDR_LEN);
		return WMG_STATUS_SUCCESS;
	} else {
		WMG_ERROR("sysinfo get faile\n");
		return WMG_STATUS_FAIL;
	}
}

wmg_status_t wmg_freertos_send_expand_cmd_smac(char *expand_cmd, void *expand_cb)
{
	uint8_t mac_addr[6] = {0};
	char ifname[10] = {0};
	struct sysinfo *sysinfo = sysinfo_get();
	char *pch;
	int i = 0;

	pch = strtok(expand_cmd, ":");
	strcpy(ifname, pch);
	pch = strtok(NULL, ":");
	pch++;
	for(i = 0;(pch != NULL) && (i < 6); i++){
		mac_addr[i] = char2uint8(pch);
		pch = strtok(NULL, ":");
	}

	if(i != 6) {
		WMG_ERROR("%s: mac address format is incorrect\n", expand_cmd);
		return WMG_STATUS_FAIL;
	}
	//set mac_addr
	if(sysinfo) {
		memcpy(sysinfo->mac_addr, mac_addr, IEEE80211_ADDR_LEN);
		//wlan_set_mac_addr(NULL, mac_addr, IEEE80211_ADDR_LEN);
	} else {
		WMG_ERROR("sysinfo get faile\n");
		return WMG_STATUS_FAIL;
	}
	//close wifi first
	xr_wlan_off();
	//default open wlan sta mode
	xr_wlan_on(WLAN_MODE_STA);
	//sync mac_addr to ifconfig cmd
	memcpy(aw_netif[MODE_STA]->hwaddr, mac_addr, NETIF_MAX_HWADDR_LEN);
	return WMG_STATUS_SUCCESS;
}

wmg_status_t wmg_freertos_send_expand_cmd(char *expand_cmd, void *expand_cb)
{
	WMG_DEBUG("freertos get exp cmd: %s\n", expand_cmd);
	if(!strncmp(expand_cmd, "gmac:", 5)) {
		return wmg_freertos_send_expand_cmd_gmac((expand_cmd + 6), expand_cb);
	} else if(!strncmp(expand_cmd, "smac:", 5)) {
		return wmg_freertos_send_expand_cmd_smac((expand_cmd + 6), expand_cb);
	}
	if(!strncmp(expand_cmd, "setcountrycode: ", 16)) {
		return wmg_freertos_send_expand_set_countrycode((expand_cmd + 16), expand_cb);
	} else if(!strncmp(expand_cmd, "getcountrycode", 14)) {
		return wmg_freertos_send_expand_get_countrycode((expand_cmd + 14), expand_cb);
	} else {
		WMG_ERROR("unspport freertos expand_cmd: %s\n", expand_cmd);
	}
	return WMG_STATUS_FAIL;
}
