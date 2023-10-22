/*
* Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*    1. Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*    2. Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the
*       distribution.
*    3. Neither the name of XRADIO TECHNOLOGY CO., LTD. nor the names of
*       its contributors may be used to endorse or promote products derived
*       from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/****************************************************************************
*
* This demo showcases BLE GATT server. It can send adv data, be connected by client.
* Run the gatt_client demo, the client demo will automatically connect to the gatt_server demo.
* Client demo will enable gatt_server's notify after connection. The two devices will then exchange
* data.
*
****************************************************************************/
#include "cmd_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xr_log.h"
#include "xr_bt.h"

#include "xr_gap_ble_api.h"
#include "xr_gatts_api.h"
#include "xr_bt_defs.h"
#include "xr_bt_main.h"
#include "xr_gatt_common_api.h"
#include "xr_gattc_api.h"
#include "xr_gatt_defs.h"

#include "timers.h"

#define ADV_IND         0x00
#define ADV_DIRECT_IND  0x01
#define ADV_SCAN_IND    0x02
#define ADV_NONCONN_IND 0x03
#define ADV_SCAN_RSP    0x04

#define ADV_TAG "ADV_DEMO"
#define PACK_LOG 0
#define TXDEV_XIAOMI 1


#define isxdigit(c) ( \
           ('0' <= (c) && (c) <= '9') \
        || ('a' <= (c) && (c) <= 'f') \
        || ('A' <= (c) && (c) <= 'F') \
        )

#define isdigit(c) ('0' <= (c) && (c) <= '9')
#define TOLOWER(x) ((x) | 0x20)

struct adv_test_conf_t {
	char remote_name_scan[18];
	char remote_name_adv[18];
	char remote_bdaddr[18];
	char remote_bdaddr_str[6];
	uint16_t tx_intl;
	uint32_t tx_num;
	bool external;
	char state;// 0: origin 1: set name only 2: set bda only 3: set name and bda both
} adv_test_cfg = {
	{'a', 'b', 'c'},
	{'c', 'b', 'a'},
	{'2', '2', '2', '2', '7', '5', '6', '9', '1', '2', '3', '8'},
	{0x22, 0x22, 0x75, 0x69, 0x12, 0x38},
	0x20,
	1000,
	0,
	0
};

#define COMMAND_DEBUG 1
#if (COMMAND_DEBUG == 1)
#define COM_D printf
#else
#define COM_D
#endif

#define PARM_NUM(num) (num - '0' + 1)
static const char *ble_trx_help =
	"Usage:\n"
	"\t bt adv parm parm_num [option][parameters]\n"
	"Options:\n"
	"\t[-sname1] stand for target scanning device name. default: abc\n"
	"\t[-aname2] stand for adv name. default: cba\n"
	"\t[-b88:C3:97:7A:78:A3] stand for target scanning device bdaddr. default: 88:C3:97:7A:78:A3\n"
	"\t[-n1000] stand for tx packet num. range: 0~10000. default: 1000\n"
	"\t[-e1] stand for opening external feature. default: 0\n"
	"\t[-i20] stand for interval. range 20~1000ms\n"
	"\texample:bt adv parm 6 -sxradio_for_scan -axradio_for_adv -b88:C3:97:7A:78:A3 -n1000 -e1 -i20\n"
	"\n"
	"\t bt adv start [tx on_off] [rx on_off]\n"
	"Options:\n"
	"\t[tx on_off] stand for tx on_off. 1 is on, 0 is off\n"
	"\t[rx on_off] stand for tx on_off. 1 is on, 0 is off\n"
	"\texample bt adv start 1 1\n";


static xr_ble_adv_params_t adv_ble_params = {
	.adv_int_min        = 0x20,
	.adv_int_max        = 0x20,
	.adv_type           = ADV_NONCONN_IND,
	.own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
	.channel_map        = ADV_CHNL_ALL,
	.adv_filter_policy = ADV_FILTER_ALLOW_SCAN_WLST_CON_WLST,
};

static xr_ble_scan_params_t scan_ble_params = {
	.scan_type              = BLE_SCAN_TYPE_ACTIVE,
	.own_addr_type          = BLE_ADDR_TYPE_PUBLIC,
	.scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
	.scan_interval          = 0x50,
	.scan_window            = 0x30,
	.scan_duplicate         = BLE_SCAN_DUPLICATE_DISABLE
};

static uint8_t adv_service_uuid128[32] = {
	/* LSB <--------------------------------------------------------------------------------> MSB */
	//first uuid, 16bit, [12],[13] is the value
	0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xEE, 0x00, 0x00, 0x00,
	//second uuid, 32bit, [12], [13], [14], [15] is the value
	0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
};

uint8_t volatile cnt_adv_data[2] = {0x00, 0x00};

uint16_t pkt_cnt = 0;

bool set_data_flag = false;

static xr_ble_adv_data_t ble_adv_data = {
	.set_scan_rsp = false,
	.include_name = true,
	.include_txpower = false,
	.min_interval = 0x0006, //slave connection min interval, Time = min_interval * 1.25 msec
	.max_interval = 0x0010, //slave connection max interval, Time = max_interval * 1.25 msec
	.appearance = 0x00,
	.manufacturer_len = 2,
	.p_manufacturer_data =  cnt_adv_data,
	.service_data_len = 0,
	.p_service_data = NULL,
	.service_uuid_len = sizeof(adv_service_uuid128),
	.p_service_uuid = adv_service_uuid128,
	.flag = (XR_BLE_ADV_FLAG_GEN_DISC | XR_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

// scan response data
static xr_ble_adv_data_t scan_rsp_data = {
	.set_scan_rsp = true,
	.include_name = true,
	.include_txpower = true,
	.appearance = 0x00,
	.manufacturer_len = 0,
	.p_manufacturer_data =  NULL,
	.service_data_len = 0,
	.p_service_data = NULL,
	.service_uuid_len = sizeof(adv_service_uuid128),
	.p_service_uuid = adv_service_uuid128,
	.flag = (XR_BLE_ADV_FLAG_GEN_DISC | XR_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

void str_to_bda(char *str, char *bda)
{
	int value, result, i;
	COM_D("bda is :");
	for (i = 0; i < 6; i++) {
		value = result = 0;
		while (isxdigit(*str) &&
				   (value = isdigit(*str) ? *str-'0' : TOLOWER(*str)-'a'+10) < 16) {
			result = result*16 + value;
			str++;
		}
		bda[i] = result;
		COM_D("%x ", bda[i]);
		str++;
	}
	COM_D("\n");
}

uint32_t str_to_int(char *str)
{
	uint32_t i, num = 0, times = 1;
	for(i = 0; str[i] != '\0'; i++)
		times = times * 10;
	i = 0;
	do {
		times = times / 10;
		num = num + (str[i] - '0') * times;
		i++;
	} while (times != 1);
	return num;
}

void scan_process(uint8_t *data)
{
	static uint16_t recv_count = 0;
	static uint16_t repeat_count = 0;
	static uint16_t last_seqn = 0;
	static uint16_t near_seqn = 0;
	static uint16_t near_recv_count = 0;
	uint16_t seqn_delt = 0;
	uint16_t revc_delt = 0;
	recv_count++;

	uint16_t send_count = (data[1] << 8) | data[0];
	if (send_count == last_seqn) {
		repeat_count++;
	}
	last_seqn = send_count;
	if (recv_count % 50 == 0) {
		if (send_count <= near_seqn)
			seqn_delt = 1;
		else
			seqn_delt = send_count - near_seqn;

		if (recv_count <= near_recv_count)
			revc_delt = 1;
		else
			revc_delt = recv_count - near_recv_count;

		if (adv_test_cfg.external == true)
			XR_LOGI(ADV_TAG, "tx:%d,rx:%d,ratio:%d,repeat ratio:%d,recent ratio:%d,per tx:%d,per rx:%d, per ratio:%d\n", send_count, recv_count, recv_count*100/send_count,   \
				repeat_count*100/send_count, revc_delt*100/seqn_delt, near_seqn, 50, 5000/near_seqn);
		else
			XR_LOGI(ADV_TAG, "tx:%d,rx:%d,ratio:%d\n", send_count, recv_count, recv_count*100/send_count);

		near_seqn = send_count;
		near_recv_count = recv_count;
	}
}

static void gap_event_handler(xr_gap_ble_cb_event_t event, xr_ble_gap_cb_param_t *param)
{
	uint8_t *adv_name = NULL;
	uint8_t adv_name_len = 0;
#ifdef PACK_LOG
	int i = 0;
#endif
	switch (event) {
	case XR_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
		if (param->adv_data_cmpl.status != XR_BT_STATUS_SUCCESS) {
			XR_LOGE(ADV_TAG, "Advertising start failed\n");
		}
		break;
	case XR_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
		xr_ble_gap_set_device_name(adv_test_cfg.remote_name_adv);
		xr_ble_gap_start_advertising(&adv_ble_params);
		break;
	case XR_GAP_BLE_ADV_START_COMPLETE_EVT:
		if (param->adv_start_cmpl.status != XR_BT_STATUS_SUCCESS) {
			XR_LOGE(ADV_TAG, "Advertising start failed\n");
		}
		break;
	case XR_GAP_BLE_ADV_STOP_COMPLETE_EVT:
		if (param->adv_stop_cmpl.status != XR_BT_STATUS_SUCCESS) {
			XR_LOGE(ADV_TAG, "Advertising stop failed\n");
		}
	break;
	case XR_GAP_BLE_SCAN_RESULT_EVT: {
		xr_ble_gap_cb_param_t *scan_result = (xr_ble_gap_cb_param_t *)param;
		switch (scan_result->scan_rst.search_evt) {
			case XR_GAP_SEARCH_INQ_RES_EVT:
				adv_name = xr_ble_resolve_adv_data(scan_result->scan_rst.ble_adv,
				                                   XR_BLE_AD_TYPE_NAME_CMPL, &adv_name_len);
				if (adv_test_cfg.state == 1) {
					if ((strlen(adv_test_cfg.remote_name_scan) == adv_name_len
						&& strncmp((char *)adv_name, adv_test_cfg.remote_name_scan, adv_name_len) == 0)) {
#ifdef PACK_LOG
						for (i = 0; i < scan_result->scan_rst.adv_data_len - 1; i++) {
							printf("%x ", scan_result->scan_rst.ble_adv[i]);
						}
						printf("\n");
#endif
#if (TXDEV_XIAOMI == 1)
						scan_process(&(scan_result->scan_rst.ble_adv[3+2]));
#else
						scan_process(&(scan_result->scan_rst.ble_adv[3+1+adv_name_len+1+2]));//by ourself
#endif
					}

				} else if(adv_test_cfg.state == 2) {
					printf("result is %x %x %x %x %x %x", scan_result->scan_rst.bda[0], scan_result->scan_rst.bda[1], scan_result->scan_rst.bda[2], scan_result->scan_rst.bda[3], scan_result->scan_rst.bda[4], scan_result->scan_rst.bda[5]);
					if (strncmp((char *)scan_result->scan_rst.bda, (char *)adv_test_cfg.remote_bdaddr, 6) == 0) {
#ifdef PACK_LOG
						for (i = 0; i < scan_result->scan_rst.adv_data_len - 1; i++) {
							printf("%x ", scan_result->scan_rst.ble_adv[i]);
						}
						printf("\n");
#endif
#if (TXDEV_XIAOMI == 1)
						scan_process(&(scan_result->scan_rst.ble_adv[3+2]));
#else
						scan_process(&(scan_result->scan_rst.ble_adv[3+1+adv_name_len+1+2]));//by ourself
#endif
					}
				} else {
					if ((strlen(adv_test_cfg.remote_name_scan) == adv_name_len
						&& strncmp((char *)adv_name, adv_test_cfg.remote_name_scan, adv_name_len) == 0)
						&& strncmp((char *)(scan_result->scan_rst.bda), (char *)(adv_test_cfg.remote_bdaddr), 6) == 0) {
#ifdef PACK_LOG
						for (i = 0; i < scan_result->scan_rst.adv_data_len - 1; i++) {
							printf("%x ", scan_result->scan_rst.ble_adv[i]);
						}
						printf("\n");
#endif
#if (TXDEV_XIAOMI == 1)
						scan_process(&(scan_result->scan_rst.ble_adv[3+2]));
#else
						scan_process(&(scan_result->scan_rst.ble_adv[3+1+adv_name_len+1+2]));//by ourself
#endif
					}
				}
				break;
		}
	}
	break;
	default:
		break;
	}
}


static void prvAutoReloadTimerCallback( TimerHandle_t xTimer )
{
	static int num = 0;
#ifdef PACK_LOG
	if (num % 5 == 0) {
		printf("num is %d adv_test_cfg.tx_num is %d [0] is %d [1] is %d\n", num, adv_test_cfg.tx_num, ble_adv_data.p_manufacturer_data[0], ble_adv_data.p_manufacturer_data[1]);
	}
#endif
	//xr_ble_gap_stop_advertising();
	if (adv_test_cfg.tx_num > num) {
		if(ble_adv_data.p_manufacturer_data[0] != 0xff){
				ble_adv_data.p_manufacturer_data[0]++;
		} else {
			ble_adv_data.p_manufacturer_data[0] = 0;
			ble_adv_data.p_manufacturer_data[1]++;
		}
		xr_ble_gap_config_adv_data(&ble_adv_data);
		num++;
	} else if(adv_test_cfg.tx_num == num) {
		xr_ble_gap_stop_advertising();
		XR_LOGI(ADV_TAG, "send %d pkt finish\n", num);
		num++;
	}
	else{
		;
	}

	//usleep(10);
	//xr_ble_gap_start_advertising(&adv_ble_params);
}

static void adv_rx_start()
{
	XR_LOGI(ADV_TAG, "adv_rx_start\n\n");
	xr_ble_gap_set_scan_params(&scan_ble_params);
	xr_ble_gap_start_scanning(0xffffffff);
}

static void adv_tx_start()
{
	TimerHandle_t xAutoReloadTimer;
	BaseType_t xTimer2Started;
	xAutoReloadTimer = xTimerCreate( "AutoReload",
									 (TickType_t         )adv_test_cfg.tx_intl,
									 pdTRUE,
									 0,
									 prvAutoReloadTimerCallback );

	if (xAutoReloadTimer != NULL)
	{
		xTimer2Started = xTimerStart( xAutoReloadTimer, 0 );
	}
}

enum cmd_status cmd_adv_start_exec(char *cmd)
{
	int ret = -1;
	int argc;
	char *argv[2];
	argc = cmd_parse_argv(cmd, argv, 2);

	ret = xr_ble_gap_register_callback(gap_event_handler);
	if (ret) {
		XR_LOGE(ADV_TAG, "xr_ble_gap_register_callback failed, error code = %x", ret);
	}

	ret = xr_ble_gap_set_device_name(adv_test_cfg.remote_name_adv);
	if (ret) {
			XR_LOGE(ADV_TAG, "set name  failed, error code = %x", ret);
	}

	ret = xr_ble_gap_config_adv_data(&ble_adv_data);
	if (ret) {
		XR_LOGE(ADV_TAG, "config adv data failed, error code = %x", ret);
	}
	//config scan response data
	ret = xr_ble_gap_config_adv_data(&scan_rsp_data);
	if (ret) {
		XR_LOGE(ADV_TAG, "config scan response data failed, error code = %x", ret);
	}

	if (argv[0][0] == '1') {
		adv_tx_start();
	}
	if (argv[1][0] == '1') {
		adv_rx_start();
	}
}

enum cmd_status cmd_adv_parm_exec(char *cmd)
{
	int argc = PARM_NUM(*cmd);
	char *argv[7] = {0};
	int i;
	if (argc > 7 || argc < 0) {
		XR_LOGE(ADV_TAG, "parm error %d\n", argc);
		return CMD_STATUS_INVALID_ARG;
	}
	argc = cmd_parse_argv(cmd, argv, argc);
	if (argc != PARM_NUM(*cmd)) {
		XR_LOGE(ADV_TAG, "parm error %d %d\n", argc, PARM_NUM(*cmd));
		return CMD_STATUS_INVALID_ARG;
	}
	adv_test_cfg.state = 0;
	for (i = 1; i < argc ; i++) {
		if (argv[i][0] == '-') {
			switch (argv[i][1]) {
				case 's':
					strcpy(adv_test_cfg.remote_name_scan, &argv[i][2]);
					adv_test_cfg.state = adv_test_cfg.state + 1;
					COM_D("scan target name:%s\n", adv_test_cfg.remote_name_scan);
					break;
				case 'a':
					strcpy(adv_test_cfg.remote_name_adv, &argv[i][2]);
					adv_test_cfg.state = adv_test_cfg.state + 2;
					COM_D("adv name:%s\n", adv_test_cfg.remote_name_adv);
					break;
				case 'b':
					str_to_bda(&argv[i][2], adv_test_cfg.remote_bdaddr);
					strcpy(adv_test_cfg.remote_bdaddr_str, &argv[i][2]);
					COM_D("scan target bdaddr:%s\n", adv_test_cfg.remote_bdaddr_str);
					break;
				case 'n':
					adv_test_cfg.tx_num = str_to_int(&argv[i][2]);
					COM_D("tx_num is %d\n", adv_test_cfg.tx_num);
					break;
				case 'e':
					if(argv[i][2] == '1')
						adv_test_cfg.external = true;
					else
						adv_test_cfg.external = false;
					COM_D("external is %d\n", adv_test_cfg.external);
					break;
				case 'i':
					adv_test_cfg.tx_intl = str_to_int(&argv[i][2]) * 8 / 5;
					COM_D("interval is %d\n", adv_test_cfg.tx_intl);
					break;
				default :
					COM_D("%s\n", ble_trx_help);
					return CMD_STATUS_INVALID_ARG;
			}
		}
	}
}

/*
    $test <scan name> <adv name>
    $parm parm_num <scan name> <adv name> <bdaddr> <pkt_num> <external> <interval>
*/
static const struct cmd_data g_adv_cmds[] = {
	{ "start",               cmd_adv_start_exec  },
	{ "parm",                cmd_adv_parm_exec   },
};

enum cmd_status cmd_adv_exec(char *cmd)
{
	return cmd_exec(cmd, g_adv_cmds, cmd_nitems(g_adv_cmds));
}

