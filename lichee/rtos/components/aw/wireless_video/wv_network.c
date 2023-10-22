#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "wv_log.h"
#include <wifimg.h>
#include <hal_time.h>

static int netlink_config(char *mode)
{
	int ret = -1;
	char ssid_buf[SSID_MAX_LEN + 1] = {0};
	char psk_buf[PSK_MAX_LEN + 1] = {0};
	wifi_linkd_result_t linkd_result;
	wmg_status_t erro_code;

	linkd_result.ssid = ssid_buf;
	linkd_result.psk = psk_buf;

	LOG_D("netlink mode: %s", mode);
	if (!strncmp(mode, "ble", 3)) {
		erro_code = wifi_linkd_protocol(WMG_LINKD_MODE_BLE, NULL, 0, &linkd_result);
		if (erro_code != WMG_STATUS_SUCCESS){
			ret = -1;
			LOG_E("ble config net get result failed");
			goto end;
		}
	} else if (!strncmp(mode, "softap", 6)) {
		wifi_ap_config_t ap_config;
		char ssid_buf[SSID_MAX_LEN + 1] = "Aw-config-net-Test";
		char psk_buf[PSK_MAX_LEN + 1] = "aw123456";
		ap_config.ssid = ssid_buf;
		ap_config.psk = psk_buf;
		ap_config.sec = WIFI_SEC_WPA2_PSK;
		ap_config.channel = 6;

		erro_code = wifi_linkd_protocol(WMG_LINKD_MODE_SOFTAP, &ap_config, 0, &linkd_result);
		if (erro_code != WMG_STATUS_SUCCESS){
			if (erro_code == WMG_STATUS_TIMEOUT) {
				LOG_E("softap config net get result failed(linkd time out)");
			} else {
				LOG_E("softap config net get result failed");
			}
			ret = -1;
			goto end;
		}
	} else if (!strncmp(mode, "xconfig", 7)) {
		erro_code = wifi_linkd_protocol(WMG_LINKD_MODE_XCONFIG, NULL, 0, &linkd_result);
		if (erro_code != WMG_STATUS_SUCCESS){
			ret = -1;
			LOG_E("xconfig config net get result failed");
			goto end;
		}
	} else if(!strncmp(mode, "soundwave", 9)) {
		erro_code = wifi_linkd_protocol(WMG_LINKD_MODE_SOUNDWAVE, NULL, 0, &linkd_result);
		if (erro_code != WMG_STATUS_SUCCESS){
			ret = -1;
			LOG_E("soundwave config net get result failed");
			goto end;
		}
	} else {
		LOG_E("Don't support [%s] netlink mode", mode);
		goto end;
	}

	LOG_I("Get ssid and psk success, now connect to ap:%s with psk:%s", linkd_result.ssid, linkd_result.psk);

	wifi_sta_cn_para_t cn_para;
	cn_para.ssid = linkd_result.ssid;
	cn_para.password = linkd_result.psk;
	cn_para.fast_connect = 0;
	cn_para.sec = WIFI_SEC_WPA2_PSK;
	ret = wifi_sta_connect(&cn_para);
	if (ret == 0) {
		LOG_I("wifi connect ap success!");
	} else {
		LOG_E("wifi connect ap failed!");
		goto end;
	}

	return 0;

end:
	return -1;
}

static int network_check(void)
{
	int fd, ret, n_read, try;
	char *read_buf;
	char *p;
	char *q = NULL;
	char *ssid_buf;
	char *psk_buf;
	int sec_num;
	struct stat file_stat;
	int loop_flag = 0;
	wifi_sta_cn_para_t cn_para;
	cn_para.sec = 0;

	if (!access("/data/wpa_supplicant.conf", R_OK)) {
		fd = open("/data/wpa_supplicant.conf", O_RDWR);
		ret = fstat(fd, &file_stat);
		if (ret == -1) {
			LOG_E("get file state failed");
			return -1;
		} else {
			read_buf = (char *)malloc(file_stat.st_size + 1);
			memset(read_buf, 0, file_stat.st_size +1);
			n_read = read(fd, read_buf, file_stat.st_size);
			p = read_buf;
			while ((q = strchr(p, ':')) != NULL) {
				if (loop_flag == 0){
					*q = '\0';
					ssid_buf = p;
					cn_para.ssid = ssid_buf;
					LOG_D("===ssid_buf:[%s], len: %d", ssid_buf, strlen(ssid_buf));
				}
				if (loop_flag == 1){
					*q = '\0';
					psk_buf = p;
					cn_para.password = psk_buf;
					LOG_D("===psk_buf:[%s], len: %d", psk_buf, strlen(psk_buf));
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
				LOG_D("===sec_buf:[%s]", p);
			}

			close(fd);
		}
    	for (try = 0; try < 5; try++) {
        	LOG_D("set adapter power,try:%d", try + 1);
        	if (wifi_sta_connect(&cn_para) == 0) {
				LOG_I("wifi connect ap success!");
            	break;
        	} else {
				LOG_E("wifi connect ap failed!");
			}
    	}
	} else {
		LOG_W("Device has no network");
		return -1;
	}

	return 0;
}

int network_init(char *mode)
{
	int ret;

	LOG_I("=======wifi enter sta mode=======");
	wifimanager_init();
	ret = wifi_on(WIFI_STATION);
	if (ret == 0) {
		LOG_I("wifi on sta mode success");
	} else {
		LOG_I("wifi on sta mode fail");
		goto fail2;
	}

	hal_msleep(1 * 1000);

	if (network_check() < 0) {
		if (netlink_config(mode) == 0) {
			LOG_I("netlink config success!");
		} else {
			LOG_I("netlink config fail, demo end!");
			goto fail1;
		}
	}

	return 0;

fail1:
	wifi_off();
fail2:
	wifimanager_deinit();
	return -1;
}
