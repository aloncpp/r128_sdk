#include <tinatest.h>
#include "tt-wifitest.h"

//wifi scan test
//cmd:tt wscanwifi
int tt_scanwifitest(int argc, char **argv)
{
	int ret, i;
	aw_wifi_scan_results_t scan_result[MAX_SCAN_RESULTS_NUM];

	printf("\n========TINATEST FOR WIFI SCAN========\n");
	ret = aw_wifi_scan(scan_result, MAX_SCAN_RESULTS_NUM);
	for (i = 0; i < ret; ++i) {
		printf("%2d bssid: %02X:%02X:%02X:%02X:%02X:%02X  ssid:%-24.24s  "
				"ch: %-2d  rssi: %2d  key_mgmt: %2d\n", i,
				scan_result[i].bssid[0], scan_result[i].bssid[1],
				scan_result[i].bssid[2], scan_result[i].bssid[3],
				scan_result[i].bssid[4], scan_result[i].bssid[5],
				scan_result[i].ssid, scan_result[i].channel,
				scan_result[i].rssi, scan_result[i].key);
	}

	if(ret > 0)
		printf("\n=======TINATEST FOR WIFI SCAN OK=======\n");
	else
		printf("\n=======TINATEST FOR WIFI SCAN FAIL=======\n");

    return (ret <= 0 ? ret : 0);
}
testcase_init(tt_scanwifitest, wscanwifi, wifi scan for tinatest);
