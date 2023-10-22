/*
 * Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
 */

#ifndef __SCAN_H__
#define __SCAN_H__

#include "wmg_common.h"

#if __cplusplus
extern "C" {
#endif

#define SCAN_BUF_LEN      4096
#define SCAN_TRY_MAX	  5
#define KEY_NONE_INDEX    0
#define KEY_WPA_PSK_INDEX 1
#define KEY_WEP_INDEX     2
#define KEY_UNKOWN		  3

struct net_scan {
	/* store scan results */
	char results[SCAN_BUF_LEN];
	unsigned int results_len;
	unsigned int try_scan_count;
	bool enable;
};

void remove_slash_from_scan_results(char *recv_results);
int parse_scan_results(char *recv_results, wifi_scan_result_t *scan_results,
		uint32_t *bss_num, uint32_t arr_size);
int direct_get_scan_results_inner(char *results, int *len);
int get_key_mgmt(const char *ssid, int key_mgmt_info[]);
int isScanEnable();


#if __cplusplus
}
#endif

#endif /* __SCAN_H__ */
