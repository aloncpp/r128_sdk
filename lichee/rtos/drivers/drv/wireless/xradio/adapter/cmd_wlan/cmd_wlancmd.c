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

#include "sys/endian.h"
#include "cmd_util.h"
#include "../net_ctrl/net_ctrl.h"
#include "../net_ctrl/net_init.h"
#include "net/wlan/wlan_ext_req.h"
#include "net/wlan/wlan_defs.h"
#include "net/wlan/wlan.h"
#include "net/wlan/wlan_frame.h"
#include "sysinfo/sysinfo.h"
#include "kernel/os/os_errno.h"
#include <math.h>

#define LOW_POWER_IP_SERVICE        0
#define NONCONNECT_SNIFF            0
#define MONITOR_RX_HT40_PHY_INFO    0
#define AP_SNIFFER                  0
#define BEACON_TX                   0

static enum cmd_status cmd_wlan_stop_exec(char *cmd)
{
	net_sys_stop();

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_wlan_start_exec(char *cmd)
{
	struct sysinfo *sysinfo = sysinfo_get();
	if (sysinfo == NULL) {
		CMD_ERR("failed to get sysinfo %p\n", sysinfo);
		return CMD_STATUS_FAIL;
	}

	net_sys_start(sysinfo->wlan_mode);

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_wlan_set_pm_dtim(char *cmd)
{
	int ret, cnt;
	uint32_t period;

	cnt = cmd_sscanf(cmd, "p=%d", &period);
	if (cnt != 1) {
		CMD_ERR("cnt %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_PM_DTIM, period);

	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_wlan_set_ps_mode(char *cmd)
{
	int ret, cnt;
	uint32_t ps_mode;
	uint32_t ps_ip = 0;
	uint32_t ps_cp = 0;
	wlan_ext_ps_cfg_t ps_cfg;

	if (cmd_strncmp(cmd, "enable", 6) == 0) {
		ps_mode = 1;
		cnt = cmd_sscanf((cmd + 6), " ip=%d cp=%d", &ps_ip, &ps_cp);
		if (cnt != 2) {
			ps_ip = 0;
			ps_cp = 0;
			CMD_ERR("cnt %d\n", cnt);
		}
	} else if (cmd_strncmp(cmd, "ps_enable", 9) == 0) {
		ps_mode = 2;
		cnt = cmd_sscanf((cmd + 9), " ip=%d cp=%d", &ps_ip, &ps_cp);
		if (cnt != 2) {
			ps_ip = 0;
			ps_cp = 0;
			CMD_ERR("cnt %d\n", cnt);
		}
	} else if (cmd_strncmp(cmd, "disable", 7) == 0) {
		ps_mode = 0;
	} else {
		CMD_ERR("invalid argument '%s'\n", cmd);
		return CMD_STATUS_INVALID_ARG;
	}

	cmd_memset(&ps_cfg, 0, sizeof(wlan_ext_ps_cfg_t));
	ps_cfg.ps_mode = ps_mode;
	ps_cfg.ps_idle_period = ps_ip;
	ps_cfg.ps_change_period = ps_cp;
	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_PS_CFG, (uint32_t)(uintptr_t)&ps_cfg);
	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_wlan_set_ampdu(char *cmd)
{
	int ret, cnt;
	int num;

	cnt = cmd_sscanf(cmd, "l=%d", &num);
	if (cnt != 1) {
		CMD_ERR("cnt %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_AMPDU_TXNUM, num);

	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_wlan_set_retry(char *cmd)
{
	int ret;
	int retry_cnt, cnt;

	cnt = cmd_sscanf(cmd, "n=%d", &retry_cnt);
	if (cnt != 1) {
		CMD_ERR("cnt %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_TX_RETRY_CNT, retry_cnt);

	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_wlan_set_pm_tx_null_period(char *cmd)
{
	int ret;
	int period, cnt;

	cnt = cmd_sscanf(cmd, "p=%d", &period);
	if (cnt != 1) {
		CMD_ERR("cnt %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_PM_TX_NULL_PERIOD, period);

	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	return CMD_STATUS_OK;
}

#define SUPPORT_BCN_AUTO_OFFSET            0

static enum cmd_status cmd_wlan_get_bcn_stat(char *cmd)
{
	int ret, i;
	unsigned int sum_cnt = 0;
	int sum_avg = 0;
	wlan_ext_bcn_status_t bcn_status;
#if SUPPORT_BCN_AUTO_OFFSET
	wlan_ext_bcn_auto_offset_t bcn_auto_offset_info;
#endif
	char dly_info[][20] = {
		"(<0      )",
		"(<500us  )",
		"(<1000us )",
		"(<2000us )",
		"(<4000us )",
		"(<8000us )",
		"(<16000us)",
		"(>16000us)",
	};

	cmd_memset(&bcn_status, 0, sizeof(wlan_ext_bcn_status_t));
#if SUPPORT_BCN_AUTO_OFFSET
	cmd_memset(&bcn_auto_offset_info, 0, sizeof(wlan_ext_bcn_auto_offset_t));
#endif
	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_GET_BCN_STATUS, (uint32_t)(uintptr_t)&bcn_status);

	if (ret == -2) {
		CMD_ERR("%s: WLAN_EXT_CMD_GET_BCN_STATUS command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: WLAN_EXT_CMD_GET_BCN_STATUS command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

#if SUPPORT_BCN_AUTO_OFFSET
	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_AUTO_BCN_OFFSET_READ, (uint32_t)&bcn_auto_offset_info);
	if (ret == -2) {
		CMD_ERR("%s: WLAN_EXT_CMD_AUTO_BCN_OFFSET_READ command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: WLAN_EXT_CMD_AUTO_BCN_OFFSET_READ command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}
#endif

	CMD_LOG(1, "\nAPP AP Beacon Delay Stat Info=================\n");
	for (i = 0; i < 8; i++) {
		CMD_LOG(1, "cnt %d %s: %d\n",
		        i, dly_info[i],
		        bcn_status.bcn_delay_cnt[i]);
		sum_cnt += bcn_status.bcn_delay_cnt[i];
	}

	if (sum_cnt)
		sum_avg = bcn_status.bcn_delay_sum / sum_cnt;

	CMD_LOG(1, "Dur: %d, Max: %d, Rxed: %d, Missed: %d\n",
	        bcn_status.bcn_duration,
	        bcn_status.bcn_delay_max,
	        bcn_status.bcn_rx_cnt,
	        bcn_status.bcn_miss_cnt);
	CMD_LOG(1, "Sum: %d, Cnt: %d, Ava: %d\n",
	        bcn_status.bcn_delay_sum,
	        sum_cnt,
	        sum_avg);

#if SUPPORT_BCN_AUTO_OFFSET
	CMD_LOG(1, "\nAPP Beacon Auto Offset Info===================\n");
	for (i = 0; i < BCN_AUTO_OFFSET_DLY_TIME_CNT; i++) {
		CMD_LOG(1, "Bcn Dly Time(us) %d: \t%d\n", i, bcn_auto_offset_info.bcn_dly_time[i]);
	}
	CMD_LOG(1, "Bcn Auto Offset Open Cnt: %d\n", bcn_auto_offset_info.bcn_auto_offset_open_cnt);
	CMD_LOG(1, "APP Beacon Auto Offset Info===================\n");
#endif

	CMD_LOG(1, "APP AP Beacon Delay Stat Info=================\n");

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_wlan_set_bcn_win_us(char *cmd)
{
	int ret;
	int bcn_win, cnt;

	cnt = cmd_sscanf(cmd, "w=%d", &bcn_win);
	if (cnt != 1) {
		CMD_ERR("cnt %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_BCN_WIN_US, bcn_win);

	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_wlan_get_cur_tx_rate(char *cmd)
{
	int ret;
	int rate;

	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_GET_TX_RATE, (uint32_t)(uintptr_t)(&rate));

	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}
	CMD_LOG(1, "current rate:%d\n", rate);

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_wlan_get_pm_dtim(char *cmd)
{
	int ret;
	wlan_ext_pm_dtim_t dtim;

	cmd_memset(&dtim, 0, sizeof(wlan_ext_pm_dtim_t));
	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_GET_PM_DTIM, (uint32_t)(uintptr_t)(&dtim));
	CMD_LOG(1, "AP DTIM set:%d, STA DTIM set:%d\n",
	        dtim.pm_join_dtim_period, dtim.pm_dtim_period_extend);

	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_wlan_get_cur_signal(char *cmd)
{
	int ret;
	wlan_ext_signal_t signal;

	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_GET_SIGNAL, (uint32_t)(uintptr_t)(&signal));

	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}
	CMD_LOG(1, "current rssi:%d(snr, 0.5db), noise:%d(dbm), level:%d(dbm)\n", signal.rssi,
	        signal.noise, signal.rssi / 2 + signal.noise);

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_wlan_set_scan_param(char *cmd)
{
	int ret, cnt;
	int num_probes;
	int probe_delay;
	int min_dwell;
	int max_dwell;
	wlan_ext_scan_param_t param;

	cnt = cmd_sscanf(cmd, "n=%d d=%d min=%d max=%d",
	                 &num_probes, &probe_delay, &min_dwell, &max_dwell);
	if (cnt != 4) {
		CMD_ERR("cnt %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	memset(&param, 0, sizeof(wlan_ext_scan_param_t));
	param.num_probes = num_probes;
	param.probe_delay = probe_delay;
	param.min_dwell = min_dwell;
	param.max_dwell = max_dwell;
	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_SCAN_PARAM, (uint32_t)(uintptr_t)(&param));

	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_wlan_set_listen_interval(char *cmd)
{
	int ret, cnt;
	uint32_t listen_interval;

	cnt = cmd_sscanf(cmd, "l=%d", &listen_interval);
	if (cnt != 1) {
		CMD_ERR("cnt %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_LISTEN_INTERVAL, listen_interval);

	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	return CMD_STATUS_OK;
}

#if LOW_POWER_IP_SERVICE
#include <lwip/sockets.h>
#include <lwip/err.h>
#include <lwip/sys.h>
#include "netif/etharp.h"
static enum cmd_status cmd_wlan_set_auto_scan(char *cmd)
{

	int ret, cnt;
	int enable;
	int interval;
	wlan_ext_auto_scan_param_t param;

	if (cmd_strncmp(cmd, "enable", 6) == 0) {
		enable = 1;
		cnt = cmd_sscanf((cmd + 6), " i=%d", &interval);
		if (cnt != 1) {
			interval = 0;
			CMD_ERR("cnt %d\n", cnt);
		}
	} else if (cmd_strncmp(cmd, "disable", 7) == 0) {
		enable = 0;
		interval = 0;
	} else {
		CMD_ERR("invalid argument '%s'\n", cmd);
		return CMD_STATUS_INVALID_ARG;
	}

	memset(&param, 0, sizeof(wlan_ext_auto_scan_param_t));
	param.auto_scan_enable = enable;
	param.auto_scan_interval = interval;
	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_AUTO_SCAN, (uint32_t)(uintptr_t)(&param));

	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	return CMD_STATUS_ACKED;
}

#define CONNECTION_DATA           "hello!"
#define P2P_WAKEUP_PACKET_DATA    "for wakeup packet!"
#define P2P_KEEP_ALIVE_DATA       "for keeping alive!"
#define EXIT_DATA                 "exit!"
#define MAX_DATA_SIZE             128

int ser_udp_flag;
int sock_ser;
int sock_remote;
struct sockaddr_in remote_addr;
socklen_t addr_len = sizeof(remote_addr);
static enum cmd_status cmd_wlan_server_start(char *cmd)
{
	struct sockaddr_in local_addr;
	int port, cnt, ret;

	cnt = cmd_sscanf(cmd, "port=%d udp=%d", &port, &ser_udp_flag);
	if (cnt != 2) {
		CMD_ERR("cnt %d\n", cnt);
		//return CMD_STATUS_INVALID_ARG;
	}

	if (ser_udp_flag) {
		sock_ser = socket(AF_INET, SOCK_DGRAM, 0);
	} else {
		sock_ser = socket(AF_INET, SOCK_STREAM, 0);
	}

	int tmp = 1;
	ret = setsockopt(sock_ser, SOL_SOCKET, SO_REUSEADDR, &tmp, sizeof(int));
	if (ret != 0) {
		CMD_ERR("setsockopt(SO_REUSEADDR) failed, err %d\n", (int)(uintptr_t)XR_OS_GetErrno);
		closesocket(sock_ser);
		return -1;
	}

	/* bind socket to port */
	memset(&local_addr, 0, sizeof(struct sockaddr_in));
	local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	local_addr.sin_port = htons(port);
	local_addr.sin_family = AF_INET;
	ret = bind(sock_ser, (struct sockaddr *)&local_addr, sizeof(struct sockaddr_in));
	if (ret < 0) {
		CMD_ERR("Failed to bind socket %d, err %d\n", sock_ser, (int)(uintptr_t)XR_OS_GetErrno);
		closesocket(sock_ser);
		return CMD_STATUS_ACKED;
	}

	if (ser_udp_flag) {
		uint32_t data_len;
		uint8_t *data_buf = NULL;
		data_buf = cmd_malloc(MAX_DATA_SIZE);
		if (data_buf == NULL) {
			CMD_ERR("malloc() failed!\n");
			return CMD_STATUS_ACKED;
		}

		printf("UDP bind with port %d\n", port);
		cmd_memset(data_buf, 0, MAX_DATA_SIZE);
		data_len = recvfrom(sock_ser, data_buf, MAX_DATA_SIZE, 0,
		                    (struct sockaddr *)&remote_addr, &addr_len);
		if (data_len > 0) {
			printf("UDP recv connection from %s:%d\n",
			    inet_ntoa(remote_addr.sin_addr), ntohs(remote_addr.sin_port));
		} else {
			printf("fail to recv connection from remote socket\n");
		}
		cmd_free(data_buf);
	} else {
		printf("TCP listen at port %d\n", port);
		ret = listen(sock_ser, 1);
		if (ret < 0) {
			CMD_ERR("Failed to listen socket %d, err %d\n", sock_ser, (int)(uintptr_t)XR_OS_GetErrno);
			closesocket(sock_ser);
			return CMD_STATUS_ACKED;
		}

		ret = sizeof(struct sockaddr_in);
		while (1) {
			sock_remote = accept(sock_ser, (struct sockaddr *)&remote_addr,
			                     (socklen_t *)&ret);
			if (sock_remote >= 0)
				break;
		}
	}

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_wlan_server_send(char *cmd)
{
	int cnt, ret, i;
	int send_cnt, period, len;
	uint8_t *data_buf = NULL;
	cnt = cmd_sscanf(cmd, "cnt=%d p=%d l=%d", &send_cnt, &period, &len);
	if (cnt != 3) {
		CMD_ERR("cnt %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	if (len > MAX_DATA_SIZE) {
		CMD_ERR("invalid length %d, should be small than %d\n", len, MAX_DATA_SIZE);
		return CMD_STATUS_INVALID_ARG;
	}
	data_buf = cmd_malloc(len);
	for (i = 0; i < len; i++) {
		data_buf[i] = i;
	}

	//UDP remote_addr was set when receive before
	printf("%s send to %s:%d\n", ser_udp_flag ? "UDP" : "TCP",
	       inet_ntoa(remote_addr.sin_addr), ntohs(remote_addr.sin_port));
	for (i = 0; i < send_cnt; i++) {
		printf("cnt:%d send data %d bytes\n", i, len);
		if (ser_udp_flag) {
				ret = sendto(sock_ser, data_buf, len, 0, (struct sockaddr *)&remote_addr, addr_len);
		} else {
				ret = send(sock_remote, data_buf, len, 0);
		}
		if (ret <= 0) {
			printf("send return %d, err %d\n", ret, (int)(uintptr_t)XR_OS_GetErrno);
			break;
		}
		XR_OS_MSleep(period);
	}
	cmd_free(data_buf);

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_wlan_server_recv(char *cmd)
{
	int cnt, recv_cnt;
	uint32_t data_len;
	uint8_t *data_buf = NULL;

	cnt = cmd_sscanf(cmd, "cnt=%d", &recv_cnt);
	if (cnt != 1) {
		CMD_ERR("cnt %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	data_buf = cmd_malloc(MAX_DATA_SIZE);
	if (data_buf == NULL) {
		CMD_ERR("malloc() failed!\n");
		return CMD_STATUS_ACKED;
	}
	if (ser_udp_flag) {
		int i = 0;
		printf("UDP recv from %s:%d\n", inet_ntoa(remote_addr.sin_addr), ntohs(remote_addr.sin_port));
		while (1) {
			cmd_memset(data_buf, 0, MAX_DATA_SIZE);
			data_len = recvfrom(sock_ser, data_buf, MAX_DATA_SIZE, 0,
			                    (struct sockaddr *)&remote_addr, &addr_len);
			if (data_len > 0) {
				printf("cnt:%d recv data %d bytes\n", i, data_len);
				i++;
				if (i >= recv_cnt)
					break;
			} else {
				data_len = 0;
			}
		}
	} else {
		printf("TCP recv from %s:%d\n", inet_ntoa(remote_addr.sin_addr), ntohs(remote_addr.sin_port));
		for (int i = 0; i < recv_cnt; i++) {
			cmd_memset(data_buf, 0, MAX_DATA_SIZE);
			data_len = recv(sock_remote, data_buf, MAX_DATA_SIZE, 0);
			if (data_len) {
				printf("cnt:%d recv data %d bytes\n", i, data_len);
			}
		}
	}
	cmd_free(data_buf);
	return CMD_STATUS_OK;
}

static enum cmd_status cmd_wlan_server_stop(char *cmd)
{
	printf("close server socket!\n");
	closesocket(sock_ser);
	return CMD_STATUS_OK;
}

int cli_udp_flag;
int sock_cli;
int ser_port;
uint32_t ser_ip[4];
struct sockaddr_in ser_addr;
socklen_t ser_addr_len = sizeof(ser_addr);
static enum cmd_status cmd_wlan_client_start(char *cmd)
{
	int cnt, ret;
	uint8_t str_ip_addr[100];

	cnt = cmd_sscanf(cmd, "ser=%d.%d.%d.%d port=%d udp=%d",
	                 &ser_ip[0], &ser_ip[1], &ser_ip[2], &ser_ip[3], &ser_port,
	                 &cli_udp_flag);
	if (cnt != 6) {
		CMD_ERR("cnt %d\n", cnt);
		//return CMD_STATUS_INVALID_ARG;
	}

	memset(str_ip_addr, 0, 100);
	sprintf((char *)str_ip_addr, "%d.%d.%d.%d", ser_ip[0], ser_ip[1],
	        ser_ip[2], ser_ip[3]);

	if (cli_udp_flag) {
		sock_cli = socket(AF_INET, SOCK_DGRAM, 0);
	} else {
		sock_cli = socket(AF_INET, SOCK_STREAM, 0);
	}

	ser_addr.sin_family = AF_INET;
	ser_addr.sin_addr.s_addr = inet_addr((char *)str_ip_addr);
	ser_addr.sin_port = htons(ser_port);
	if (cli_udp_flag) {
		printf("send connection to %s:%d...\n", str_ip_addr, ser_port);
		ret = sendto(sock_cli, CONNECTION_DATA, sizeof(CONNECTION_DATA),
		             0, (struct sockaddr *)&ser_addr, ser_addr_len);
	} else {
		printf("try to connect %s:%d...\n", str_ip_addr, ser_port);
		ret = connect(sock_cli, (struct sockaddr *)&ser_addr, sizeof(struct sockaddr));
		if (ret < 0)
			printf("Connect failed! ret = %d\n", ret);
		else {
			printf("Connect OK!\n");
		}
	}
	return CMD_STATUS_OK;
}

static enum cmd_status cmd_wlan_client_send(char *cmd)
{
	int cnt, ret, i;
	int send_cnt, period, len;
	uint8_t *data_buf = NULL;
	cnt = cmd_sscanf(cmd, "cnt=%d p=%d l=%d", &send_cnt, &period, &len);
	if (cnt != 3) {
		CMD_ERR("cnt %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	if (len > MAX_DATA_SIZE) {
		CMD_ERR("invalid length %d, should be small than %d\n", len, MAX_DATA_SIZE);
		return CMD_STATUS_INVALID_ARG;
	}
	data_buf = cmd_malloc(len);
	for (i = 0; i < len; i++) {
		data_buf[i] = i;
	}

	//ser_addr was set when client start
	printf("%s send to %s:%d\n", cli_udp_flag?"UDP":"TCP", inet_ntoa(ser_addr.sin_addr), ntohs(ser_addr.sin_port));
	for (i = 0; i < send_cnt; i++) {
		printf("cnt:%d send data %d bytes\n", i, len);
		if (cli_udp_flag) {
			ret = sendto(sock_cli, data_buf, len, 0, (struct sockaddr *)&ser_addr, ser_addr_len);
		} else {
			ret = send(sock_cli, data_buf, len, 0);
		}
		if (ret <= 0) {
			printf("send return %d, err %d\n", ret, (int)(uintptr_t)XR_OS_GetErrno);
			break;
		}
		XR_OS_MSleep(period);
	}
	cmd_free(data_buf);

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_wlan_client_recv(char *cmd)
{
	int cnt, recv_cnt;
	uint32_t data_len;
	uint8_t *data_buf = NULL;

	cnt = cmd_sscanf(cmd, "cnt=%d", &recv_cnt);
	if (cnt != 1) {
		CMD_ERR("cnt %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	data_buf = malloc(MAX_DATA_SIZE);
	if (data_buf == NULL) {
		CMD_ERR("malloc() failed!\n");
		return CMD_STATUS_ACKED;
	}
	if (cli_udp_flag) {
		int i = 0;
		printf("UDP recv from %s:%d\n", inet_ntoa(ser_addr.sin_addr), ntohs(ser_addr.sin_port));
		while (1) {
			cmd_memset(data_buf, 0, MAX_DATA_SIZE);
			data_len = recvfrom(sock_cli, data_buf, MAX_DATA_SIZE, 0,
			                    (struct sockaddr *)&ser_addr, &ser_addr_len);
			if (data_len > 0) {
				printf("cnt:%d recv data %d bytes\n", i, data_len);
				i++;
				if (i >= recv_cnt)
					break;
			} else {
				data_len = 0;
			}
		}
	} else {
		printf("TCP recv from %s:%d\n", inet_ntoa(ser_addr.sin_addr), ntohs(ser_addr.sin_port));
		for (int i = 0; i < recv_cnt; i++) {
			cmd_memset(data_buf, 0, MAX_DATA_SIZE);
			data_len = recv(sock_cli, data_buf, MAX_DATA_SIZE, 0);
			if (data_len) {
				printf("cnt:%d recv data %d bytes\n", i, data_len);
			}
		}
	}
	cmd_free(data_buf);
	return CMD_STATUS_OK;
}

static enum cmd_status cmd_wlan_client_stop(char *cmd)
{
	//send(sock_cli_tcp, EXIT_DATA, strlen(EXIT_DATA), 0);
	printf("close client socket!\n");
	closesocket(sock_cli);
	return CMD_STATUS_OK;
}

static enum cmd_status cmd_wlan_set_p2p_server(char *cmd)
{
	int ret, cnt, i;
	int mac[6];
	int ser_num, enable;
	int udp_flag = 0;
	wlan_ext_p2p_svr_set_t svr;
	struct sockaddr_in local_addr;
	socklen_t len = sizeof(struct sockaddr);

	cnt = cmd_sscanf(cmd, "s=%d e=%d udp=%d mac=%x:%x:%x:%x:%x:%x",
	                 &ser_num, &enable, &udp_flag,
	                 &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
	if (cnt != 9) {
		CMD_ERR("cnt %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	//set server param
	if (ser_num > 2) {
		CMD_ERR("invalid server number %d, should be 0 to 2\n", ser_num);
		return CMD_STATUS_INVALID_ARG;
	}
	cmd_memset(&svr, 0, sizeof(wlan_ext_p2p_svr_set_t));
#if LWIP_IPV4 && LWIP_IPV6
	cmd_memcpy(svr.SrcIPv4Addr, &(g_wlan_netif->ip_addr.u_addr), 4);
#else
	cmd_memcpy(svr.SrcIPv4Addr, &(g_wlan_netif->ip_addr.addr), 4);
#endif
	getsockname(sock_cli, (struct sockaddr *)&local_addr, &len);
	svr.P2PServerCfgs[ser_num].DstPort = ser_port;
	svr.P2PServerCfgs[ser_num].SrcPort = ntohs(local_addr.sin_port);
	for (i = 0; i < 4; i++) {
		svr.P2PServerCfgs[ser_num].DstIPv4Addr[i] = ser_ip[i];
	}
//  ret = etharp_find_addr(g_wlan_netif, (const ip4_addr_t *)&(server_addr.sin_addr.s_addr),
//                         &ethaddr_ret, (const ip4_addr_t **)&ipaddr_ret);
	for (i = 0;i < 6;i++) {
		svr.P2PServerCfgs[ser_num].DstMacAddr[i] = mac[i];
	}
	svr.P2PServerCfgs[ser_num].TcpSeqInit = getsockack(sock_cli);
	svr.P2PServerCfgs[ser_num].TcpAckInit = getsockseq(sock_cli);
	svr.P2PServerCfgs[ser_num].IPIdInit = 1;
	svr.P2PServerCfgs[ser_num].Enable = enable;
	svr.P2PServerCfgs[ser_num].TcpOrUdp = udp_flag?0x02:0x01;
	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_P2P_SVR, (uint32_t)(uintptr_t)(&svr));


	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_wlan_set_p2p_wkpacket(char *cmd)
{
	int ret, cnt, i;
	int ser_num, enable, payload_len;
	wlan_ext_p2p_wkp_param_set_t wkp_param;

	cnt = cmd_sscanf(cmd, "s=%d e=%d pl=%d", &ser_num, &enable, &payload_len);
	if (cnt != 3) {
		CMD_ERR("cnt %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	//set wakeup packet param
	if (ser_num > 2) {
		CMD_ERR("invalid server number %d, should be 0 to 2\n", ser_num);
		return CMD_STATUS_INVALID_ARG;
	}
	cmd_memset(&wkp_param, 0, sizeof(wlan_ext_p2p_wkp_param_set_t));
	wkp_param.P2PWkpParamCfgs[ser_num].Enable = enable;
	wkp_param.P2PWkpParamCfgs[ser_num].PayloadLen = payload_len;
	for (i = 0; i < payload_len; i++) {
		wkp_param.P2PWkpParamCfgs[ser_num].Payload[i] = i;
	}
	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_P2P_WKP_CFG, (uint32_t)(uintptr_t)(&wkp_param));

	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_wlan_set_p2p_wakeupip(char *cmd)
{
	int ret, cnt, i;
	int ser_num, enable, ip[4];
	wlan_ext_p2p_wkp_param_set_t wkp_param;

	cnt = cmd_sscanf(cmd, "s=%d e=%d ser=%d.%d.%d.%d", &ser_num, &enable, &ip[0], &ip[1], &ip[2], &ip[3]);
	if (cnt != 6) {
		CMD_ERR("cnt %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	//set wakeup packet param
	if (ser_num > 7) {
		CMD_ERR("invalid server number %d, should be 0 to 7\n", ser_num);
		return CMD_STATUS_INVALID_ARG;
	}
	cmd_memset(&wkp_param, 0, sizeof(wlan_ext_p2p_wkp_param_set_t));
	wkp_param.Enable = enable;
	for (i = 0; i < 4; i++) {
		wkp_param.P2PIpv4FilterCfgs[ser_num].Ipv4Filter[i] = ip[i];
	}
	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_P2P_WKP_CFG, (uint32_t)(uintptr_t)(&wkp_param));

	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_wlan_set_p2p_kpalive(char *cmd)
{
	int ret, cnt, i;
	int ser_num, enable;
	int period, timeout, retry, payload_len;
	wlan_ext_p2p_kpalive_param_set_t param;

	cnt = cmd_sscanf(cmd, "s=%d e=%d p=%d t=%d r=%d pl=%d",
	                 &ser_num, &enable, &period, &timeout, &retry,
	                 &payload_len);
	if (cnt != 6) {
		CMD_ERR("cnt %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	//set keep alive param
	if (ser_num > 2) {
		CMD_ERR("invalid server number %d, should be 0 to 2\n", ser_num);
		return CMD_STATUS_INVALID_ARG;
	}
	cmd_memset(&param, 0, sizeof(wlan_ext_p2p_kpalive_param_set_t));
	param.KeepAlivePeriod_s = period;
	param.TxTimeOut_s = timeout;
	param.TxRetryLimit = retry;
	param.P2PKeepAliveParamCfgs[ser_num].Enable = enable;
	param.P2PKeepAliveParamCfgs[ser_num].PayloadLen = payload_len;
	for (i = 0; i < payload_len; i++) {
		param.P2PKeepAliveParamCfgs[ser_num].Payload[i] = i;
	}
	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_P2P_KPALIVE_CFG, (uint32_t)(uintptr_t)(&param));

	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_wlan_set_p2p_hostsleep(char *cmd)
{
	int ret, cnt;
	int hostsleep;

	cnt = cmd_sscanf(cmd, "%d", &hostsleep);
	if (cnt != 1) {
		CMD_ERR("cnt %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_P2P_HOST_SLEEP, hostsleep);

	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	return CMD_STATUS_OK;
}
#endif

const char *wmm_type[] = {
	"AC_VO",
	"AC_VI",
	"AC_BE",
	"AC_BK",
};

const char *wmm_phymode[] = {
	"auto",
	"11b",
	"11g",
	"11ng",
	"all",
};

static enum cmd_status cmd_wlan_set_edca_param(char *cmd)
{
	int ret, cnt;
	int temp_type, temp_cwmax, temp_cwmin, temp_aifsn, temp_mode;

	cnt = cmd_sscanf(cmd, "type=%d mode=%d max=%d min=%d n=%d",
	                 &temp_type, &temp_mode, &temp_cwmax, &temp_cwmin, &temp_aifsn);
	if (cnt != 5) {
		CMD_ERR("cnt %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}
	wlan_ext_edca_param_t param;
	cmd_memset(&param, 0, sizeof(wlan_ext_edca_param_t));
	param.type = temp_type;
	param.mode = temp_mode;
	param.cwmax = temp_cwmax;
	param.cwmin = temp_cwmin;
	param.aifsn = temp_aifsn;
	CMD_LOG(1, "target type = %s\ntarget mode = %s\ntarget cwmax = %d\ntarget cwmin = %d\n"
	        "target aifsn = %d\n", wmm_type[param.type], wmm_phymode[param.mode],
	        ((int)pow(2,param.cwmax) - 1), ((int)pow(2,param.cwmin) - 1), param.aifsn);

	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_EDCA_PARAM, (uint32_t)(uintptr_t)(&param));
	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_wlan_get_edca_param(char *cmd)
{
	wlan_ext_edca_param_t param;
	int ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_GET_EDCA_PARAM, (uint32_t)(uintptr_t)(&param));
	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	return CMD_STATUS_OK;
}

#if NONCONNECT_SNIFF
static enum cmd_status cmd_wlan_set_rcv_special_frm(char *cmd)
{
	int ret;
	uint32_t enable, type;
	wlan_ext_rcv_spec_frm_param_set_t param;

	int cnt;
	cnt = cmd_sscanf(cmd, "e=%d t=0x%x", &enable, &type);
	if (cnt != 2) {
		CMD_ERR("cnt %d\n", cnt);
	}
	memset(&param, 0, sizeof(wlan_ext_rcv_spec_frm_param_set_t));
	param.Enable = enable;
	param.u32RecvSpecFrameCfg = type;

	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_RCV_SPECIAL_FRM, (uint32_t)(uintptr_t)(&param));

	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_wlan_send_raw_frm_cfg(char *cmd)
{
	int ret, cnt;
	uint32_t cfg, enable;
	wlan_ext_send_raw_frm_param_set_t param;

	cnt = cmd_sscanf(cmd, "e=%d c=0x%x", &enable, &cfg);
	if (cnt != 2) {
		CMD_ERR("cnt %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	memset(&param, 0, sizeof(wlan_ext_send_raw_frm_param_set_t));
	param.Enable = enable;
	param.u16SendRawFrameCfg = cfg;
	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_SEND_RAW_FRM_CFG, (uint32_t)(uintptr_t)(&param));

	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_wlan_switch_channel_cfg(char *cmd)
{
	int ret, cnt;
	uint32_t band, flag, channel, sc_time;
	wlan_ext_switch_channel_param_set_t param;

	cnt = cmd_sscanf(cmd, "b=%d f=0x%x c=%d t=%d",
		&band, &flag, &channel, &sc_time);
	if (cnt != 4) {
		CMD_ERR("cnt %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	memset(&param, 0, sizeof(wlan_ext_switch_channel_param_set_t));
	param.Enable = 1;
	param.Band = band;
	param.Flag = flag;
	param.ChannelNum = channel;
	param.SwitchChannelTime = sc_time;
	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_SWITCH_CHN_CFG, (uint32_t)(uintptr_t)(&param));

	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_wlan_get_cur_channel(char *cmd)
{
	int ret;
	uint32_t channel;

	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_GET_CURRENT_CHN, (uint32_t)(uintptr_t)(&channel));
	printf("current channel is %d\n", channel);
	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_wlan_sniff_kp_active(char *cmd)
{
	int ret, cnt;
	uint32_t enable, cfg;
	wlan_ext_sniff_kp_active_set_t param;

	cnt = cmd_sscanf(cmd, "e=%d c=0x%x", &enable, &cfg);
	if (cnt != 2) {
		CMD_ERR("cnt %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	memset(&param, 0, sizeof(wlan_ext_sniff_kp_active_set_t));
	param.Enable = enable;
	param.u32Config = cfg;
	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_SNIFF_KP_ACTIVE, (uint32_t)(uintptr_t)(&param));

	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_wlan_set_sniff_auto_wakeup_host(char *cmd)
{
	int ret, cnt;
	int cfg, type, enable, wkp_time, kp_time, flag, start_time, channel;
	wlan_ext_sniff_sync_param_set_t param;

	cnt = cmd_sscanf(cmd, "cfg=0x%x t=0x%x e=%d c=%d wt=%d kt=%d f=0x%x st=%d",
	                 &cfg, &type, &enable, &channel, &wkp_time, &kp_time, &flag,
	                 &start_time);
	if (cnt != 8) {
		printf("cnt %d\n", cnt);
		//return CMD_STATUS_INVALID_ARG;
	}
	param.Enable = enable;
	param.ChannelNum = channel;
	param.SyncFrameType = type;//data
	param.u32SniffSyncCfg = cfg;// | SNIFF_SYNC_DISABLE_TIMER;
	param.time_sync_at_host.WakeupPeriod_ms = wkp_time;//ms
	param.time_sync_at_host.KeepActivePeriod_ms = kp_time;//ms
	param.time_sync_at_host.Flag = flag;//SNIFF_AUTO_WAKEUP_FRAME_SEND_TO_HOST;
	param.time_sync_at_host.StartTime = start_time;//us
	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_SNIFF_SYNC_CFG, (uint32_t)(uintptr_t)(&param));

	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_wlan_set_sniff_auto_wakeup_wifi(char *cmd)
{
	int ret, cnt;
	int cfg, type, channel, flag, SyncDTIM, MaxLostSyncPacket;
	int TSFOffset, AdaptiveExpansion, KeepActiveNumAfterLostSync, ActiveTime, MaxAdaptiveExpansionLimit;
	int MaxKeepAliveTime;
	wlan_ext_sniff_sync_param_set_t param;

	cnt = cmd_sscanf(cmd, "cfg=0x%x t=0x%x c=%d f=0x%x d=%d ml=%d tsf=%d ae=%d kaa=%d at=%d ma=%d mk=%d",
	    &cfg, &type, &channel, &flag, &SyncDTIM, &MaxLostSyncPacket,    &TSFOffset,
	    &AdaptiveExpansion, &KeepActiveNumAfterLostSync, &ActiveTime, &MaxAdaptiveExpansionLimit,
	    &MaxKeepAliveTime);
	if (cnt != 12) {
		CMD_ERR("cnt %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}
	param.Enable = 1;
	param.ChannelNum = channel;
	param.SyncFrameType = type;
	param.u32SniffSyncCfg = cfg;

	param.time_sync_at_wifi.Flag = flag;
	param.time_sync_at_wifi.SyncDTIM = SyncDTIM;
	param.time_sync_at_wifi.MaxLostSyncPacket = MaxLostSyncPacket;
	param.time_sync_at_wifi.TSFOffset = TSFOffset;
	param.time_sync_at_wifi.AdaptiveExpansion = AdaptiveExpansion;
	param.time_sync_at_wifi.KeepActiveNumAfterLostSync = KeepActiveNumAfterLostSync;
	param.time_sync_at_wifi.ActiveTime = ActiveTime;
	param.time_sync_at_wifi.MaxAdaptiveExpansionLimit = MaxAdaptiveExpansionLimit;
	param.time_sync_at_wifi.MaxKeepAliveTime = MaxKeepAliveTime;
	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_SNIFF_SYNC_CFG, (uint32_t)(uintptr_t)(&param));

	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	return CMD_STATUS_OK;
}

static wlan_ext_frm_filter_set_t g_frm_filter;
static enum cmd_status cmd_wlan_rcv_frm_filter_mac(char *cmd)
{
	int ret = 0, cnt, i, filter;

	uint32_t A1[6];
	uint32_t A2[6];
	uint32_t A3[6];

	cnt = cmd_sscanf(cmd, "f=%d a1=%x:%x:%x:%x:%x:%x a2=%x:%x:%x:%x:%x:%x a3=%x:%x:%x:%x:%x:%x",
	                 &filter,
	                 &A1[0], &A1[1], &A1[2], &A1[3], &A1[4], &A1[5],
	                 &A2[0], &A2[1], &A2[2], &A2[3], &A2[4], &A2[5],
	                 &A3[0], &A3[1], &A3[2], &A3[3], &A3[4], &A3[5]);
	if (cnt != 19) {
		CMD_ERR("cnt %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	if (filter == 1) {
		for (i = 0; i < 6; i++) {
			g_frm_filter.Filter1.MacAddrA1[i] = (uint8_t)A1[i];
			g_frm_filter.Filter1.MacAddrA2[i] = (uint8_t)A2[i];
			g_frm_filter.Filter1.MacAddrA3[i] = (uint8_t)A3[i];
		}
	} else if (filter == 2) {
		for (i = 0; i < 6; i++) {
			g_frm_filter.Filter2.MacAddrA1[i] = (uint8_t)A1[i];
			g_frm_filter.Filter2.MacAddrA2[i] = (uint8_t)A2[i];
			g_frm_filter.Filter2.MacAddrA3[i] = (uint8_t)A3[i];
		}
	} else {
		CMD_ERR("%s: invalid filter type:%d\n", __func__, filter);
		return CMD_STATUS_ACKED;
	}

	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}
	return CMD_STATUS_OK;
}

static enum cmd_status cmd_wlan_set_frm_filter(char *cmd)
{
	int ret;
	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_FRM_FILTER, (uint32_t)(uintptr_t)(&g_frm_filter));

	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_wlan_rcv_frm_filter1_ie_cfg(char *cmd)
{
	int ret = 0, cnt, i;
	rcv_frm_filter_t *param;
	param = &(g_frm_filter.Filter1);

	int enable, filter, and_mask, or_mask, frm_type, mac_mask, ie_id, ie_len;
	uint32_t oui[10];

	cnt = cmd_sscanf(cmd, "e=%d f=0x%x am=0x%x om=0x%x t=0x%x mm=0x%x ii=%d il=%d oui=%x %x %x %x %x %x %x %x %x %x ",
	    &enable, &filter, &and_mask, &or_mask, &frm_type, &mac_mask, &ie_id, &ie_len,
	    &oui[0], &oui[1], &oui[2], &oui[3], &oui[4],
	    &oui[5], &oui[6], &oui[7], &oui[8], &oui[9]);
	if (cnt != 18) {
		printf("cnt %d\n", cnt);
		if ((cnt == 6) || (cnt == 1)) {
			ie_len = 0;
		} else {
			return CMD_STATUS_INVALID_ARG;
		}
	}

	if (enable == 0) {
		g_frm_filter.Filter1Cfg = 0;
		cmd_memset(param, 0, sizeof(rcv_frm_filter_t));
		return CMD_STATUS_OK;
	}

	g_frm_filter.Filter1Cfg = 1;
	param->FilterEnable = filter;
	param->AndOperationMask = and_mask;
	param->OrOperationMask = or_mask;
	param->FrameType = frm_type;
	param->MacAddrMask = mac_mask;
	param->IeCfg.ElementId = ie_id;
	param->IeCfg.Length = ie_len;
	if (cnt >= 8) {
	//CMD_WRN("IE Id:%d, Len:%d\n", ie_id, ie_len);
		for (i = 0; i < ie_len; i++) {
			param->IeCfg.OUI[i] = (uint8_t)oui[i];
			//printf("OUI: num:%d origin::%x, set:%x\n", i, oui[i], param->IeCfg.OUI[i]);
		}
	}
	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_wlan_rcv_frm_filter2_ie_cfg(char *cmd)
{
	int ret = 0, cnt, i;
	rcv_frm_filter_t *param;
	param = &(g_frm_filter.Filter2);

	int enable, filter, and_mask, or_mask, frm_type, mac_mask, ie_id, ie_len;
	uint32_t oui[10];

	cnt = cmd_sscanf(cmd, "e=%d f=0x%x am=0x%x om=0x%x t=0x%x mm=0x%x ii=%d il=%d oui=%x %x %x %x %x %x %x %x %x %x ",
	    &enable, &filter, &and_mask, &or_mask, &frm_type, &mac_mask, &ie_id, &ie_len,
	    &oui[0], &oui[1], &oui[2], &oui[3], &oui[4],
	    &oui[5], &oui[6], &oui[7], &oui[8], &oui[9]);
	if (cnt != 18) {
		printf("cnt %d\n", cnt);
		//return CMD_STATUS_INVALID_ARG;
	}

	if (enable == 0) {
		g_frm_filter.Filter2Cfg = 0;
		cmd_memset(param, 0, sizeof(rcv_frm_filter_t));
		return CMD_STATUS_OK;
	}

	g_frm_filter.Filter2Cfg = 1;
	param->FilterEnable = filter;
	param->AndOperationMask = and_mask;
	param->OrOperationMask = or_mask;
	param->FrameType = frm_type;
	param->MacAddrMask = mac_mask;
	param->IeCfg.ElementId = ie_id;
	param->IeCfg.Length = ie_len;
	for (i = 0; i < ie_len; i++)
		param->IeCfg.OUI[i] = (uint8_t)oui[i];

	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_wlan_rcv_frm_filter1_pl_cfg(char *cmd)
{
	int ret = 0, cnt, i;
	rcv_frm_filter_t *param;
	param = &(g_frm_filter.Filter1);

	int enable, filter, and_mask, or_mask, frm_type, mac_mask, pl_offset, pl_len;
	uint32_t payload[10];

	cnt = cmd_sscanf(cmd, "e=%d f=0x%x am=0x%x om=0x%x t=0x%x mm=0x%x po=%d pl=%d p=%x %x %x %x %x %x %x %x %x %x ",
	    &enable, &filter, &and_mask, &or_mask, &frm_type, &mac_mask, &pl_offset, &pl_len,
	    &payload[0], &payload[1], &payload[2], &payload[3], &payload[4],
	    &payload[5], &payload[6], &payload[7], &payload[8], &payload[9]);
	if (cnt != 18) {
		printf("cnt %d\n", cnt);
		//return CMD_STATUS_INVALID_ARG;
	}

	if (enable == 0) {
		g_frm_filter.Filter1Cfg = 0;
		cmd_memset(param, 0, sizeof(rcv_frm_filter_t));
		return CMD_STATUS_ACKED;
	}

	g_frm_filter.Filter1Cfg = 1;
	param->FilterEnable = filter;
	param->AndOperationMask = and_mask;
	param->OrOperationMask = or_mask;
	param->FrameType = frm_type;
	param->MacAddrMask = mac_mask;
	param->PayloadCfg.PayloadOffset = pl_offset;
	param->PayloadCfg.PayloadLength = pl_len;
	for (i = 0; i < pl_len; i++)
		param->PayloadCfg.Payload[i] = (uint8_t)payload[i];

	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_wlan_rcv_frm_filter2_pl_cfg(char *cmd)
{
	int ret = 0, cnt, i;
	rcv_frm_filter_t *param;
	param = &(g_frm_filter.Filter2);

	int enable, filter, and_mask, or_mask, frm_type, mac_mask, pl_offset, pl_len;
	uint32_t payload[10];

	cnt = cmd_sscanf(cmd, "e=%d f=0x%x am=0x%x om=0x%x t=0x%x mm=0x%x po=%d pl=%d p=%x %x %x %x %x %x %x %x %x %x ",
	    &enable, &filter, &and_mask, &or_mask, &frm_type, &mac_mask, &pl_offset, &pl_len,
	    &payload[0], &payload[1], &payload[2], &payload[3], &payload[4],
	    &payload[5], &payload[6], &payload[7], &payload[8], &payload[9]);
	if (cnt != 18) {
		printf("cnt %d\n", cnt);
		//return CMD_STATUS_INVALID_ARG;
	}

	if (enable == 0) {
		g_frm_filter.Filter2Cfg = 0;
		cmd_memset(param, 0, sizeof(rcv_frm_filter_t));
		return CMD_STATUS_ACKED;
	}

	g_frm_filter.Filter2Cfg = 1;
	param->FilterEnable = filter;
	param->AndOperationMask = and_mask;
	param->OrOperationMask = or_mask;
	param->FrameType = frm_type;
	param->MacAddrMask = mac_mask;
	param->PayloadCfg.PayloadOffset = pl_offset;
	param->PayloadCfg.PayloadLength = pl_len;
	for (i = 0; i < pl_len; i++)
		param->PayloadCfg.Payload[i] = (uint8_t)payload[i];

	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	return CMD_STATUS_OK;
}


#define FRM_TYPE_ASRQ       (0)
#define FRM_TYPE_ASRSP      (1)
#define FRM_TYPE_RSRQ       (2)
#define FRM_TYPE_RSRSP      (3)
#define FRM_TYPE_PBRQ       (4)
#define FRM_TYPE_PBRSP      (5)
#define FRM_TYPE_BCN        (6)
#define FRM_TYPE_ATIM       (7)
#define FRM_TYPE_DAS        (8)
#define FRM_TYPE_AUTH       (9)
#define FRM_TYPE_DAUTH      (10)
#define FRM_TYPE_ACTION     (11)
#define FRM_TYPE_NULLDATA   (12)
#define FRM_TYPE_DATA       (13)
#define FRM_TYPE_QOSDATA    (14)
#define FRM_TYPE_ARPREQ     (15)
#define FRM_TYPE_SA_QUERY   (16)
#define FRM_TYPE_MANAGER    (17)
#define FRM_TYPE_ALL_DATA   (18)
#define FRM_TYPE_MAX        (19)

static wlan_ext_temp_frm_set_t frm;
static uint8_t bssid[6] = {0x14, 0x72, 0x58, 0x36, 0x90, 0xaa};
static uint8_t src_mac[6] = {0x14, 0x72, 0x58, 0x36, 0x90, 0xaa};
static uint8_t dest_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static const char *ssid = "AP_test";
static uint8_t ap_channel = 1;
static uint8_t src_ip[4] = {192, 168, 51, 123};

void buf_dump(unsigned char *buf, int len)
{
	int i;
	for (i = 1; i < len + 1; i++) {
		printf("%02X ", buf[i-1]);
		if (i % 16 == 0)
			printf("\n");
	}
}

void construct_frame(int frm_type)
{
	switch (frm_type) {
	case FRM_TYPE_ASRQ:
		frm.FrmLength = wlan_construct_assocreq((uint8_t *)frm.Frame, MAX_FRM_LEN, src_mac, dest_mac, bssid, (uint8_t *)ssid, strlen(ssid));
		printf("send raw frame associate req (%d)\n", frm.FrmLength);
		break;
	case FRM_TYPE_ASRSP:
		frm.FrmLength = wlan_construct_assocrsp((uint8_t *)frm.Frame, MAX_FRM_LEN, src_mac, dest_mac, bssid);
		printf("send raw frame associate rsp (%d)\n", frm.FrmLength);
		break;
	case FRM_TYPE_RSRQ:
		frm.FrmLength = wlan_construct_reassocreq((uint8_t *)frm.Frame, MAX_FRM_LEN, src_mac, dest_mac, bssid, (uint8_t *)ssid, strlen(ssid));
		printf("send raw frame reassociate req (%d)\n", frm.FrmLength);
		break;
	case FRM_TYPE_RSRSP:
		frm.FrmLength = wlan_construct_reassocrsp((uint8_t *)frm.Frame, MAX_FRM_LEN, src_mac, dest_mac, bssid);
		printf("send raw frame reassociate rsp (%d)\n", frm.FrmLength);
		break;
	case FRM_TYPE_PBRQ:
		frm.FrmLength = wlan_construct_probereq((uint8_t *)frm.Frame, MAX_FRM_LEN, src_mac, dest_mac, bssid, (uint8_t *)ssid, strlen(ssid));
		printf("send raw frame probe req (%d)\n", frm.FrmLength);
		break;
	case FRM_TYPE_PBRSP:
		frm.FrmLength = wlan_construct_probersp((uint8_t *)frm.Frame, MAX_FRM_LEN, src_mac, dest_mac, bssid, (uint8_t *)ssid, strlen(ssid), ap_channel);
		printf("send raw frame probe rsp (%d)\n", frm.FrmLength);
		break;
	case FRM_TYPE_BCN:
		frm.FrmLength = wlan_construct_beacon((uint8_t *)frm.Frame, MAX_FRM_LEN, src_mac, dest_mac, bssid, (uint8_t *)ssid, strlen(ssid), ap_channel);
		printf("send raw frame beacon (%d)\n", frm.FrmLength);
		break;
	case FRM_TYPE_ATIM:
		frm.FrmLength = wlan_construct_atim((uint8_t *)frm.Frame, MAX_FRM_LEN, src_mac, dest_mac, bssid);
		printf("send raw frame atim (%d)\n", frm.FrmLength);
		break;
	case FRM_TYPE_DAS:
		frm.FrmLength = wlan_construct_disassocreq((uint8_t *)frm.Frame, MAX_FRM_LEN, src_mac, dest_mac, bssid);
		printf("send raw frame disassociate req (%d)\n", frm.FrmLength);
		break;
	case FRM_TYPE_AUTH:
		frm.FrmLength = wlan_construct_auth((uint8_t *)frm.Frame, MAX_FRM_LEN, src_mac, dest_mac, bssid);
		printf("send raw frame auth (%d)\n", frm.FrmLength);
		break;
	case FRM_TYPE_DAUTH:
		frm.FrmLength = wlan_construct_deauth((uint8_t *)frm.Frame, MAX_FRM_LEN, src_mac, dest_mac, bssid);
		printf("send raw frame deauth (%d)\n", frm.FrmLength);
		break;
	case FRM_TYPE_ACTION:
		frm.FrmLength = wlan_construct_action((uint8_t *)frm.Frame, MAX_FRM_LEN, src_mac, dest_mac, bssid);
		printf("send raw frame action (%d)\n", frm.FrmLength);
		break;
	case FRM_TYPE_NULLDATA:
		frm.FrmLength = wlan_construct_nulldata((uint8_t *)frm.Frame, MAX_FRM_LEN, src_mac, dest_mac, bssid);
		printf("send raw frame nulldata (%d)\n", frm.FrmLength);
		break;
	case FRM_TYPE_DATA:
		frm.FrmLength = wlan_construct_data((uint8_t *)frm.Frame, MAX_FRM_LEN, src_mac, dest_mac, bssid);
		printf("send raw frame data (%d)\n", frm.FrmLength);
		break;
	case FRM_TYPE_QOSDATA:
		frm.FrmLength = wlan_construct_qosdata((uint8_t *)frm.Frame, MAX_FRM_LEN, src_mac, dest_mac, bssid);
		printf("send raw frame qosdata (%d)\n", frm.FrmLength);
		break;
	case FRM_TYPE_ARPREQ:
		frm.FrmLength = wlan_construct_arpreq((uint8_t *)frm.Frame, MAX_FRM_LEN, src_mac, dest_mac, bssid, src_ip);
		printf("send raw frame arp req (%d)\n", frm.FrmLength);
		break;
	case FRM_TYPE_SA_QUERY:
		frm.FrmLength = wlan_construct_sa_query((uint8_t *)frm.Frame, MAX_FRM_LEN, src_mac, dest_mac, bssid);
		printf("send raw frame sa query (%d)\n", frm.FrmLength);
		break;
	}
}

static enum cmd_status cmd_wlan_set_temp_frm(char *cmd)
{
	int ret, cnt, frm_type;
	cnt = cmd_sscanf(cmd, "t=%d", &frm_type);
	if (cnt != 1) {
		CMD_ERR("cnt %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}
	construct_frame(frm_type);
	if (frm_type != FRM_TYPE_BCN && frm_type != FRM_TYPE_PBRSP) {
		printf("force add TSF\n");
		for (int i = 0; i < frm.FrmLength - 24; i++) {
			frm.Frame[frm.FrmLength - 1 - i + 10] = frm.Frame[frm.FrmLength - 1 - i];
		}
		frm.Frame[32] = 0x64;
		frm.Frame[33] = 0;
		frm.FrmLength += 10;
	}
	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_TEMP_FRM, (uint32_t)(uintptr_t)(&frm));

	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_wlan_send_sync_frm(char *cmd)
{
	int ret, cnt;
	uint32_t bcn_intvl, enable;
	wlan_ext_send_sync_frm_set_t param;

	cnt = cmd_sscanf(cmd, "e=%d b=%d", &enable,
					&bcn_intvl);
	if (cnt != 2) {
		CMD_ERR("cnt %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	memset(&param, 0, sizeof(wlan_ext_send_sync_frm_set_t));
	param.Enable = enable;
	param.BcnInterval = bcn_intvl;
	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_SYNC_FRM_SEND, (uint32_t)(uintptr_t)(&param));

	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_wlan_send_raw_frm(char *cmd)
{
	int ret = 0, i, j, cnt, ext_ch, cur_ch, tmp_ch;
	int frm_cnt, frm_period, frm_type;
	uint32_t mac_sa[6];
	uint32_t mac_da[6];
	uint32_t mac_bssid[6];

	cnt = cmd_sscanf(cmd, "c=%d p=%d t=%d ec=%d da=%x:%x:%x:%x:%x:%x sa=%x:%x:%x:%x:%x:%x bssid=%x:%x:%x:%x:%x:%x",
	                &frm_cnt, &frm_period, &frm_type, &ext_ch,
	                &mac_da[0], &mac_da[1], &mac_da[2], &mac_da[3], &mac_da[4], &mac_da[5],
	                &mac_sa[0], &mac_sa[1], &mac_sa[2], &mac_sa[3], &mac_sa[4], &mac_sa[5],
	                &mac_bssid[0], &mac_bssid[1], &mac_bssid[2], &mac_bssid[3], &mac_bssid[4], &mac_bssid[5]);
	if (cnt < 4) {
		CMD_ERR("invalid param cnt %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}
	if (cnt >= 10) {
		for (i = 0; i < 6; i++)
			dest_mac[i] = (uint8_t)mac_da[i];
	}
	if (cnt >= 16) {
		for (i = 0; i < 6; i++)
			src_mac[i] = (uint8_t)mac_sa[i];
	}
	if (cnt == 22) {
		for (i = 0; i < 6; i++)
			bssid[i] = (uint8_t)mac_bssid[i];
	}
	printf("dest mac address:%02x:%02x:%02x:%02x:%02x:%02x\n",
	        dest_mac[0], dest_mac[1], dest_mac[2], dest_mac[3], dest_mac[4], dest_mac[5]);
	printf("src mac address:%02x:%02x:%02x:%02x:%02x:%02x\n",
	        src_mac[0], src_mac[1], src_mac[2], src_mac[3], src_mac[4], src_mac[5]);
	printf("bssid address:%02x:%02x:%02x:%02x:%02x:%02x\n",
	        bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);

	switch (frm_type) {
	case FRM_TYPE_MANAGER:
		printf("send all manager frame\n");
		cnt = 0;
		for (i = 0; i < frm_cnt; i++) {
			for (j = 0; j < 12; j++) {
				cmd_memset((uint8_t *)frm.Frame, 0, MAX_FRM_LEN);
				construct_frame(j);
				wlan_send_raw_frame(g_wlan_netif, IEEE80211_FC_STYPE_AUTH, (uint8_t *)frm.Frame, frm.FrmLength);
				XR_OS_MSleep(frm_period);
				printf("******************************** cnt:%d ********************************\n", cnt++);
			}
		}
		return CMD_STATUS_OK;
	case FRM_TYPE_ALL_DATA:
		printf("send all data frame\n");
		cnt = 0;
		for (i = 0; i < frm_cnt; i++) {
			for (j = 0; j < 6; j++)
				dest_mac[j] = (uint8_t)mac_da[j];
			for (j = 12; j < 15; j++) {
				cmd_memset((uint8_t *)frm.Frame, 0, MAX_FRM_LEN);
				construct_frame(j);
				wlan_send_raw_frame(g_wlan_netif, IEEE80211_FC_STYPE_AUTH, (uint8_t *)frm.Frame, frm.FrmLength);
				XR_OS_MSleep(frm_period);
				printf("******************************** cnt:%d ********************************\n", cnt++);
			}
			//send broadcast data frame
			cmd_memset(dest_mac, 0xff, 6);
			cmd_memset((uint8_t *)frm.Frame, 0, MAX_FRM_LEN);
			construct_frame(FRM_TYPE_DATA);
			wlan_send_raw_frame(g_wlan_netif, IEEE80211_FC_STYPE_AUTH, (uint8_t *)frm.Frame, frm.FrmLength);
			XR_OS_MSleep(frm_period);
			printf("******************************** cnt:%d ********************************\n", cnt++);
		}
		return CMD_STATUS_OK;
	case FRM_TYPE_MAX:
		{
			printf("send all frame\n");
			wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_GET_CURRENT_CHN, (uint32_t)(uintptr_t)(&cur_ch));
			wlan_ext_switch_channel_param_set_t param1;
			memset(&param1, 0, sizeof(wlan_ext_switch_channel_param_set_t));
			param1.Enable = 1;
			param1.SwitchChannelTime = 5000;//SEND_RAW_FRAME_MAX_SWITCH_CHANNEL_TIME;
			cnt = 0;
			for (i = 0; i < frm_cnt; i++) {
				for (j = 0; j < 6; j++)
					dest_mac[j] = (uint8_t)mac_da[j];
				for (j = 0; j < 17; j++) {
					cmd_memset((uint8_t *)frm.Frame, 0, MAX_FRM_LEN);
					construct_frame(j);
					wlan_send_raw_frame(g_wlan_netif, IEEE80211_FC_STYPE_AUTH, (uint8_t *)frm.Frame, frm.FrmLength);
					XR_OS_MSleep(frm_period);
					if (ext_ch) {
						param1.ChannelNum = ext_ch;
						wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_SWITCH_CHN_CFG, (uint32_t)(uintptr_t)(&param1));
						do {
							XR_OS_MSleep(10);
							wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_GET_CURRENT_CHN, (uint32_t)(uintptr_t)(&tmp_ch));
						} while (tmp_ch != ext_ch);
						printf("switch to channel %d\n", ext_ch);
						wlan_send_raw_frame(g_wlan_netif, IEEE80211_FC_STYPE_AUTH, (uint8_t *)frm.Frame, frm.FrmLength);
						XR_OS_MSleep(100);
						param1.ChannelNum = cur_ch;
						wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_SWITCH_CHN_CFG, (uint32_t)(uintptr_t)(&param1));
						do {
							XR_OS_MSleep(5);
							wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_GET_CURRENT_CHN, (uint32_t)(uintptr_t)(&tmp_ch));
						} while (tmp_ch != cur_ch);
						printf("switch to channel %d\n", cur_ch);
						XR_OS_MSleep(frm_period);
					}
					printf("******************************** cnt:%d ********************************\n", cnt++);
				}
				cmd_memset(dest_mac, 0xff, 6);
				cmd_memset((uint8_t *)frm.Frame, 0, MAX_FRM_LEN);
				construct_frame(FRM_TYPE_DATA);
				wlan_send_raw_frame(g_wlan_netif, IEEE80211_FC_STYPE_AUTH, (uint8_t *)frm.Frame, frm.FrmLength);
				XR_OS_MSleep(frm_period);
				if (ext_ch) {
					param1.ChannelNum = ext_ch;
					wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_SWITCH_CHN_CFG, (uint32_t)(uintptr_t)(&param1));
					do {
						XR_OS_MSleep(10);
						wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_GET_CURRENT_CHN, (uint32_t)(uintptr_t)(&tmp_ch));
					} while (tmp_ch != ext_ch);
					printf("switch to channel %d\n", ext_ch);
					wlan_send_raw_frame(g_wlan_netif, IEEE80211_FC_STYPE_AUTH, (uint8_t *)frm.Frame, frm.FrmLength);
					XR_OS_MSleep(100);
					param1.ChannelNum = cur_ch;
					wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_SWITCH_CHN_CFG, (uint32_t)(uintptr_t)(&param1));
					do {
						XR_OS_MSleep(5);
						wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_GET_CURRENT_CHN, (uint32_t)(uintptr_t)(&tmp_ch));
					} while (tmp_ch != cur_ch);
					printf("switch to channel %d\n", cur_ch);
					XR_OS_MSleep(frm_period);
				}
				printf("******************************** cnt:%d ********************************\n", cnt++);
			}
		}
		return CMD_STATUS_OK;
	default:
		construct_frame(frm_type);
		break;
	}
	buf_dump((uint8_t *)frm.Frame, frm.FrmLength);

	for (i = 0; i < frm_cnt; i++) {
		ret = wlan_send_raw_frame(g_wlan_netif, IEEE80211_FC_STYPE_AUTH, (uint8_t *)frm.Frame, frm.FrmLength);
		printf("cnt:%d\n", i+1);
		if (ret == -1) {
			CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
			return CMD_STATUS_ACKED;
		}
		XR_OS_MSleep(frm_period);
	}

	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_wlan_set_sniff_ext_param(char *cmd)
{
	int ret, cnt;
	int enable, retry, expenRight, dtim;
	wlan_ext_sniff_extern_param_set_t param;

	cnt = cmd_sscanf(cmd, "e=0x%x r=%d er=%d d=%d",
	    &enable, &retry, &expenRight, &dtim);
	if (cnt != 4) {
		CMD_ERR("cnt %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	param.SniffExtFuncEnable = enable;
	param.SniffRetryAfterLostSync = retry;
	param.SniffAdaptiveExpenRight = expenRight;
	param.SniffRetryDtim = dtim;
	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_SNIFF_EXTERN_CFG, (uint32_t)(uintptr_t)(&param));

	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_wlan_get_rcv_stat(char *cmd)
{
	int ret, i;
	unsigned int sum_cnt = 0;
	int sum_avg = 0;
	wlan_ext_bcn_status_t bcn_status;

	char dly_info[][20] = {
		"(<0      )",
		"(<500us  )",
		"(<1000us )",
		"(<2000us )",
		"(<4000us )",
		"(<8000us )",
		"(<16000us)",
		"(>16000us)",
	};

	cmd_memset(&bcn_status, 0, sizeof(wlan_ext_bcn_status_t));
	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_GET_SNIFF_STAT, (uint32_t)(uintptr_t)&bcn_status);

	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	printf("\nAPP AP Beacon Delay Stat Info=================\n");
	for (i = 0; i < 8; i++) {
		printf("cnt %d %s: %d\n",
		    i, dly_info[i],
		    bcn_status.bcn_delay_cnt[i]);
		sum_cnt += bcn_status.bcn_delay_cnt[i];
	}

	if (sum_cnt)
		sum_avg = bcn_status.bcn_delay_sum / sum_cnt;

	printf("Dur: %d, Max: 0x%X, Rxed: %d, Missed: 0x%X\n",
	    bcn_status.bcn_duration,
	    bcn_status.bcn_delay_max,
	    bcn_status.bcn_rx_cnt,
	    bcn_status.bcn_miss_cnt);
	printf("Sum: %d, Cnt: %d, Ava: %d\n",
	    bcn_status.bcn_delay_sum,
	    sum_cnt,
	    sum_avg);
	printf("APP AP Beacon Delay Stat Info=================\n");

	return CMD_STATUS_OK;
}

static uint32_t g_frm_rcv_cnt = 0;
static uint32_t g_frm_rcv_mac_cnt = 0;
void sta_rx_cb(uint8_t *data, uint32_t len, void *info)
{
	if (!info) {
		printf("%s(), info NULL\n", __func__);
		return;
	}

	struct ieee80211_frame *wh;
	uint8_t filter_mac[6] = {0x14, 0x72, 0x58, 0x36, 0x90, 0xaa};
	wh = (struct ieee80211_frame *)data;
	g_frm_rcv_cnt++;
	if (memcmp(wh->i_addr3, filter_mac, 6) == 0) {
		g_frm_rcv_mac_cnt++;
	}
	char *str_frm_type;
	switch (wh->i_fc[0] & IEEE80211_FC0_TYPE_MASK) {
	case IEEE80211_FC0_TYPE_MGT:
		switch (wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK) {
		case IEEE80211_FC_STYPE_ASSOC_REQ:
			str_frm_type = "association req";
			break;
		case IEEE80211_FC_STYPE_ASSOC_RESP:
			str_frm_type = "association resp";
			break;
		case IEEE80211_FC_STYPE_REASSOC_REQ:
			str_frm_type = "reassociation req";
			break;
		case IEEE80211_FC_STYPE_REASSOC_RESP:
			str_frm_type = "reassociation resp";
			break;
		case IEEE80211_FC_STYPE_PROBE_REQ:
			str_frm_type = "probe req";
			break;
		case IEEE80211_FC_STYPE_PROBE_RESP:
			str_frm_type = "probe resp";
			break;
		case IEEE80211_FC_STYPE_BEACON:
			str_frm_type = "beacon";
			break;
		case IEEE80211_FC_STYPE_ATIM:
			str_frm_type = "atim";
			break;
		case IEEE80211_FC_STYPE_DISASSOC:
			str_frm_type = "disassociation";
			break;
		case IEEE80211_FC_STYPE_AUTH:
			str_frm_type = "authentication";
			break;
		case IEEE80211_FC_STYPE_DEAUTH:
			str_frm_type = "deauthentication";
			break;
		case IEEE80211_FC_STYPE_ACTION:
			str_frm_type = "action";
			break;
		default:
			str_frm_type = "unknown mgmt";
			break;
		}
		break;
	case IEEE80211_FC0_TYPE_CTL:
		str_frm_type = "control";
		break;
	case IEEE80211_FC0_TYPE_DATA:
#if 0
		switch (wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK) {
		case IEEE80211_FC0_SUBTYPE_DATA:
			str_frm_type = "data";
			break;
		case IEEE80211_FC0_SUBTYPE_QOS:
			str_frm_type = "qos data";
			break;
		case IEEE80211_FC0_SUBTYPE_NODATA:
			str_frm_type = "null data";
			break;
		default:
			str_frm_type = "unknown data";
			break;
		}
#else
		str_frm_type = "data";
#endif
		break;
	default:
		str_frm_type = "unknown";
		break;
	}
	printf("recv pack type:%s cnt:%d, mac cnt:%d, len:%d\n",
		str_frm_type, g_frm_rcv_cnt, g_frm_rcv_mac_cnt, len);
}

static enum cmd_status cmd_wlan_set_rcv_cb(char *cmd)
{
	int cnt, enable;

	cnt = cmd_sscanf(cmd, "e=%d", &enable);
	if (cnt != 1) {
		CMD_ERR("cnt %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}
	if (enable)
		wlan_monitor_set_rx_cb(g_wlan_netif, sta_rx_cb);
	else
		wlan_monitor_set_rx_cb(g_wlan_netif, NULL);
	return CMD_STATUS_OK;
}

static enum cmd_status cmd_wlan_set_even_ie(char *cmd)
{
	int cnt, enable;
	uint8_t ie[IE_NUM] = {0xDD, 0x05, 0x00, 0x50, 0xF2, 0x05, 0x01};

	cnt = cmd_sscanf(cmd, "e=%d", &enable);
	if (cnt != 1) {
		CMD_ERR("cnt %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}
	if (enable) {
		ie[6]++;
		printf("Set event beacon!\n");
		wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_UPDATE_TEMP_IE, (uint32_t)(uintptr_t)(ie));
	} else {
		printf("Set normal beacon!\n");
		wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_UPDATE_TEMP_IE, (uint32_t)(uintptr_t)(ie));
	}
	return CMD_STATUS_OK;
}

#endif

static enum cmd_status cmd_wlan_set_bcn_win_cfg(char *cmd)
{
	int ret, cnt;
	int start_num, stop_num, amp_us, max_num;
	wlan_ext_bcn_win_param_set_t param;

	cnt = cmd_sscanf(cmd, "start=%d stop=%d amp=%d max=%d",
		&start_num, &stop_num, &amp_us, &max_num);
	if (cnt != 4) {
		CMD_ERR("cnt %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	param.BeaconWindowAdjStartNum = start_num;
	param.BeaconWindowAdjStopNum = stop_num;
	param.BeaconWindowAdjAmpUs = amp_us;
	param.BeaconWindowMaxStartNum = max_num;
	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_BCN_WIN_CFG, (uint32_t)(uintptr_t)(&param));

	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	return CMD_STATUS_OK;
}

// net wlan scan_freq n=1 c=7
// net wlan scan_freq n=4 c=1 6 7 14
static enum cmd_status cmd_wlan_scan_freq(char *cmd)
{
	int ret, cnt;
	int num;
	int chn[14];
	int freq[14];
	wlan_ext_scan_freq_t param;

	cnt = cmd_sscanf(cmd, "n=%d c=%d %d %d %d %d %d %d %d %d %d %d %d %d %d", &num,
	                &chn[0], &chn[1], &chn[2], &chn[3], &chn[4], &chn[5], &chn[6],
	                &chn[7], &chn[8], &chn[9], &chn[10], &chn[11], &chn[12], &chn[13]);
	if (cnt < 2 || cnt > 15) {
		CMD_ERR("cnt %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	if (num != cnt - 1) {
		CMD_ERR("%s: invalid num:%d\n", __func__, num);
		return CMD_STATUS_ACKED;
	}
	for (int i = 0; i < num; i++) {
		if (chn[i] == 14) {
			freq[i] = 2484;
		} else {
			freq[i] = 2407 + 5 * chn[i];
		}
	}
	param.freq_num = num;
	param.freq_list = freq;
	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_SCAN_FREQ, (uint32_t)(uintptr_t)(&param));

	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_wlan_pm(char *cmd)
{
	int ret, cnt;
	int enable;

	cnt = cmd_sscanf(cmd, "e=%d", &enable);
	if (cnt != 1) {
		CMD_ERR("cnt %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}
	ret = wlan_set_ps_mode(g_wlan_netif, enable);

	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_wlan_get_temp_volt(char *cmd)
{
	int ret;
	wlan_ext_temp_volt_get_t param;

	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_GET_TEMP_VOLT, (uint32_t)(uintptr_t)(&param));

	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}
	CMD_LOG(1, "Temperature:%.02f°C\n", (float)param.Temperature / 16);
	CMD_LOG(1, "Voltage:%.02fV\n", (float)param.Voltage / 16);

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_wlan_set_temp_volt_auto_upload(char *cmd)
{
	int ret, cnt;
	int enable, period;

	cnt = cmd_sscanf(cmd, "e=%d p=%d", &enable, &period);
	if (cnt != 2) {
		CMD_ERR("cnt %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}
	wlan_ext_temp_volt_auto_upload_set_t param;
	cmd_memset(&param, 0, sizeof(wlan_ext_temp_volt_auto_upload_set_t));
	param.Enable = enable;
	param.UploadPeriod = period;

	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_TEMP_VOLT_AUTO_UPLOAD, (uint32_t)(uintptr_t)(&param));

	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_wlan_set_temp_volt_thresh(char *cmd)
{
	int ret, cnt;
	int temp_high_en, temp_low_en, volt_high_en, volt_low_en;
	float temp_high_th, temp_low_th, volt_high_th, volt_low_th;

	cnt = cmd_sscanf(cmd, "THe=%d TH=%f TLe=%d TL=%f VHe=%d VH=%f VLe=%d VL=%f",
	                 &temp_high_en, &temp_high_th, &temp_low_en, &temp_low_th,
	                 &volt_high_en, &volt_high_th, &volt_low_en, &volt_low_th);
	if (cnt != 8) {
		CMD_ERR("cnt %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}
	wlan_ext_temp_volt_thresh_set_t param;
	cmd_memset(&param, 0, sizeof(wlan_ext_temp_volt_thresh_set_t));
	param.TempHighEn = temp_high_en;
	param.TempHighThresh = temp_high_th * 16;
	param.TempLowEn = temp_low_en;
	param.TempLowThresh = temp_low_th * 16;
	param.VoltHighEn = volt_high_en;
	param.VoltHighThresh = volt_high_th * 16;
	param.VoltLowEn = volt_low_en;
	param.VoltLowThresh = volt_low_th * 16;
	param.TempVoltIndPeriod = 5;
	//param.TempVoltFallbackIndEn = 1;
	//param.TempUseDeltaEn = 1;
	//param.TempVoltFixedRefEn = 1;
	//param.TempJitterCnt = 50;

	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_TEMP_VOLT_THRESH, (uint32_t)(uintptr_t)(&param));

	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_wlan_set_bcn_freq_offs_time(char *cmd)
{
	int ret, cnt;
	int time;

	cnt = cmd_sscanf(cmd, "t=%d", &time);
	if (cnt != 1) {
		CMD_ERR("cnt %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_BCN_FREQ_OFFS_TIME, time);

	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_wlan_set_lfclock_param(char *cmd)
{
	int ret, cnt;
	int type, freq, count;

	cnt = cmd_sscanf(cmd, "t=%d f=%d c=%d",    &type, &freq, &count);
	if (cnt != 3) {
		CMD_ERR("cnt %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}
	wlan_ext_lfclock_param_t param;
	cmd_memset(&param, 0, sizeof(wlan_ext_lfclock_param_t));
	param.SysLfclockType = type;
	param.SysLfclockFreq = freq;
	param.PwrCalibCount = count;
	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_LFCLOCK_PARAM, (uint32_t)(uintptr_t)(&param));

	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_wlan_set_bcn_rx_11b_only(char *cmd)
{
	int ret, cnt;
	uint32_t bcn_rx_11b_only_en;

	cnt = cmd_sscanf(cmd, "e=%d", &bcn_rx_11b_only_en);

	if (cnt != 1) {
		CMD_ERR("cnt %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_BCN_RX_11B_ONLY, (uint32_t)bcn_rx_11b_only_en);

	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	return CMD_STATUS_OK;
}

/* net wlan set_bcn_auto m=<mode> <b=bcn_num> t=<threshold_num> p=<percentage> l=<lost_bcn_max>
 * net wlan set_bcn_auto_offset m=7 b=6 t=2 p=20 l=2
 *  <mode>: [0-7], 0: close auto offset awlays, 1: 1000us-2000us, 2: 2000us-3000us, 3: 3000us-4000us ...
 *  <bcn_num>:       [0-255], the number of statistics beacons
 *  <threshold_num>: [0-255], the threshold for open Auto Offset Function
 *  <percentage>:    [0-255], the percentage of adjust
 *  <lost_bcn_max>:  [0-255], the number of consecutively lost beacon
 */
#if SUPPORT_BCN_AUTO_OFFSET
static enum cmd_status cmd_wlan_set_bcn_auto_offset(char *cmd)
{
	int ret, cnt;
	uint32_t auto_offset_mode;
	uint32_t bcn_statistics;
	uint32_t auto_offset_open_threshold;
	uint32_t auto_offset_percentage;
	uint32_t bcn_lost_max_num;

	wlan_ext_bcn_auto_offset_t bcn_auto_offset;

	cnt = cmd_sscanf(cmd, "m=%d b=%d t=%d p=%d l=%d", &auto_offset_mode, &bcn_statistics,
	           &auto_offset_open_threshold, &auto_offset_percentage, &bcn_lost_max_num);

	if (cnt != 5) {
		CMD_ERR("cnt %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	bcn_auto_offset.auto_offset_mode = (uint8_t)auto_offset_mode;
	bcn_auto_offset.bcn_statistics = (uint8_t)bcn_statistics;
	bcn_auto_offset.auto_offset_open_threshold = (uint8_t)auto_offset_open_threshold;
	bcn_auto_offset.auto_offset_percentage = (uint8_t)auto_offset_percentage;
	bcn_auto_offset.bcn_lost_max_num = (uint8_t)bcn_lost_max_num;

	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_AUTO_BCN_OFFSET_SET, (uint32_t)&bcn_auto_offset);

	if (ret == -2) {
		CMD_ERR("%s: WLAN_EXT_CMD_AUTO_BCN_OFFSET_SET command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: WLAN_EXT_CMD_AUTO_BCN_OFFSET_SET command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}


	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_AUTO_BCN_OFFSET_READ, (uint32_t)&bcn_auto_offset);
	if (ret == -2) {
		CMD_ERR("%s: WLAN_EXT_CMD_AUTO_BCN_OFFSET_READ command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: WLAN_EXT_CMD_AUTO_BCN_OFFSET_READ command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	CMD_LOG(1, "mode:%d, statistics:%d, threshold:%d, percentage:%d%%, BcnlostMax:%d\n",
	bcn_auto_offset.auto_offset_mode, bcn_auto_offset.bcn_statistics,
	bcn_auto_offset.auto_offset_open_threshold, bcn_auto_offset.auto_offset_percentage,
	bcn_auto_offset.bcn_lost_max_num);

	return CMD_STATUS_OK;
}
#endif

static enum cmd_status cmd_wlan_set_pre_rx_bcn(char *cmd)
{
	int ret, cnt;
	uint32_t enable, flag, stop_num;
	wlan_ext_pre_rx_bcn_t param;

	cnt = cmd_sscanf(cmd, "e=%d f=0x%x s=%d", &enable, &flag, &stop_num);

	if (cnt != 3) {
		CMD_ERR("cnt %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}
	param.enable = enable;
	param.flags = flag;
	param.stop_num = stop_num;

	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_PRE_RX_BCN, (uint32_t)(uintptr_t)&param);

	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	return CMD_STATUS_OK;
}

#if MONITOR_RX_HT40_PHY_INFO
int enable_rx_ext_frame(void) /* fw only monitor work*/
{
	int ret = 0;

	MIB_SET_RX_EXT_FRAME ext_frame_param;
	memset(&ext_frame_param, 0, sizeof(MIB_SET_RX_EXT_FRAME));
	ext_frame_param.rx_enable = 1;
	ext_frame_param.filter_en = RX_EXT_FRAME_FILTER_SIZE;
	ext_frame_param.size_min = 24;
	ext_frame_param.size_max = 600; /* 9bit data, 512*/

	if (g_wlan_netif == NULL) {
		CMD_ERR("%s, %d, warning: netif is null\n", __func__, __LINE__);
		return -1;
	}

	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_MIMO_PARAM, (uint32_t)(uintptr_t)&ext_frame_param);
	if (ret == -2) {
		CMD_ERR("invalid arg\n");
	} else if (ret == -1) {
		CMD_ERR("exec failed\n");
	}
	return ret;
}

int disable_rx_ext_frame(void) /* fw only monitor work*/
{
	int ret = 0;

	MIB_SET_RX_EXT_FRAME ext_frame_param;
	memset(&ext_frame_param, 0, sizeof(MIB_SET_RX_EXT_FRAME));
	ext_frame_param.rx_enable = 0;

	if (g_wlan_netif == NULL) {
		CMD_ERR("%s, %d, warning: netif is null\n", __func__, __LINE__);
		return -1;
	}

	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_MIMO_PARAM, (uint32_t)(uintptr_t)&ext_frame_param);
	if (ret == -2) {
		CMD_ERR("invalid arg\n");
	} else if (ret == -1) {
		CMD_ERR("exec failed\n");
	}
	return ret;
}

static enum cmd_status cmd_mimo_start_exec(char *cmd)
{
	enable_rx_ext_frame();
	return CMD_STATUS_OK;
}

/* $ sniffer_stop */
static enum cmd_status cmd_mimo_stop_exec(char *cmd)
{
	disable_rx_ext_frame();
	return CMD_STATUS_OK;
}
#endif

#if AP_SNIFFER
static uint32_t sniffer_rx_total_cnt;
static uint32_t statistics_start_time;
static uint32_t statistics_end_time;
static int pkt_print_enable;

//#define AUTH_FRAME_OPEN_LEN 30
#define AUTH_FRAME_OPEN_LEN 38
#define BEACON_FRAME_LEN 256
static uint8_t auth_frame_open[AUTH_FRAME_OPEN_LEN];
static uint8_t beacon_frame_buf[BEACON_FRAME_LEN];
typedef struct {
	uint8_t *data;
	uint32_t len;
} frame_data;
static frame_data auth_frame;

static uint8_t auth_tx_cnt;

static uint8_t monitor_run = 0;
static XR_OS_Thread_t monitor_thread;

static uint8_t assist_test_ap_chan;
static uint8_t assist_test_ap_mac[6] = {0x6c, 0x59, 0x40, 0x9d, 0x68, 0x6a};
static uint8_t sniffer_run = 0;
static uint8_t sniffer_channels[] = { 1, 6, 11, 7, 13, 2, 3, 4, 5, 8, 9, 10, 12, 14 };
static XR_OS_Thread_t sniffer_thread;

extern int wlan_send_raw_frame(struct netif *nif, int type,
                uint8_t *buffer, int len);
extern int xradio_drv_cmd(int argc, const char **arg);

static int char2hex(char c, u8_t *x)
{
	if (c >= '0' && c <= '9') {
		*x = c - '0';
	} else if (c >= 'a' && c <= 'f') {
		*x = c - 'a' + 10;
	} else if (c >= 'A' && c <= 'F') {
		*x = c - 'A' + 10;
	} else {
		return -EINVAL;
	}

	return 0;
}

static int addr_from_str(char *str, uint8_t *addr)
{
	int i, j;
	uint8_t tmp;

	if (strlen(str) != 17U) {
		return -EINVAL;
	}

	for (i = 0, j = 1; *str != '\0'; str++, j++) {
		if (!(j % 3) && (*str != ':')) {
			return -EINVAL;
		} else if (*str == ':') {
			i++;
			continue;
		}

		addr[i] = addr[i] << 4;

		if (char2hex(*str, &tmp) < 0) {
			return -EINVAL;
		}

		addr[i] |= tmp;
	}

	return 0;
}

static enum cmd_status cmd_sniffer_set_assist_test_ap_addr_exec(char *cmd)
{
	char *argv[4];
	char buf[35] = {0};
	char buf1[50] = {0};

	argv[0] = "addr";
	argv[1] = "ap";
	cmd_sscanf(cmd, "%s", buf);
	sprintf(buf1, buf);
	argv[2] = buf1;
	addr_from_str(buf, assist_test_ap_mac);
	argv[3] = buf;
	CMD_DBG("assist_test_ap_addr %02x:%02x:%02x:%02x:%02x:%02x\n",
		assist_test_ap_mac[0],assist_test_ap_mac[1], assist_test_ap_mac[2],
		assist_test_ap_mac[3], assist_test_ap_mac[4], assist_test_ap_mac[5]);
	xradio_drv_cmd(4, (const char **)argv);

	return CMD_STATUS_OK;
}

static void auth_frame_creat(void)
{
	auth_frame_open[0] = 0xB0; 		/* auth frame */
	auth_frame_open[2] = 0x3A; 		/* duartion */
	auth_frame_open[3] = 0x01;

	memcpy(&auth_frame_open[10], g_wlan_netif->hwaddr, IEEE80211_ADDR_LEN);
	auth_frame_open[22] = 0x06;        /* seq number */
	//auth_frame_open[24] = 0x0;         /* open system */
	auth_frame_open[26] = 0x01;        /* auth seq num */
	auth_frame_open[30] = 221; 		   /* auth special ie */
	auth_frame_open[31] = 1;
	auth_frame_open[32] = auth_tx_cnt;
	//printf("cnt %d\n", auth_cnt);
	auth_tx_cnt ++;
	if (auth_tx_cnt == 255)
		auth_tx_cnt = 0;
	//auth_frame_open[32] = 'f';
	//auth_frame_open[33] = 'i';
	//auth_frame_open[34] = 'l';
	//auth_frame_open[35] = 't';
	//auth_frame_open[36] = 'e';
	//auth_frame_open[37] = 'r';

	struct ieee80211_frame *frame = NULL;
	frame = (struct ieee80211_frame *)auth_frame_open;
	auth_frame.data = (uint8_t *)frame;
	memcpy(frame->i_addr1, assist_test_ap_mac, IEEE80211_ADDR_LEN);
	memcpy(frame->i_addr3, assist_test_ap_mac, IEEE80211_ADDR_LEN);
	auth_frame.len = AUTH_FRAME_OPEN_LEN;
}

static void send_auth_frame(void)
{
	wlan_send_raw_frame(g_wlan_netif,IEEE80211_FC_STYPE_AUTH,
		auth_frame.data, auth_frame.len);
}

#if BEACON_TX
static uint8_t beacon_addr[6];
static uint8_t beacon_chan;
static char beacon_ssid[32];
static uint32_t beacon_len;
static frame_data beacon_frame;

static void beacon_frame_creat(void)
{
	wlan_ap_config_t config;
	cmd_memset(&config, 0, sizeof(config));
	config.field = WLAN_AP_FIELD_SSID;
	if (wlan_ap_get_config(&config) != 0) {
		CMD_ERR("get config failed\n");
		return;
	}
	CMD_DBG("ssid:%s,ssid_len %d\n", config.u.ssid.ssid,
	                     config.u.ssid.ssid_len);
	memcpy(beacon_ssid, config.u.ssid.ssid, config.u.ssid.ssid_len);
	memcpy(beacon_addr, g_wlan_netif->hwaddr, IEEE80211_ADDR_LEN);
	beacon_len = wlan_construct_beacon(beacon_frame_buf, BEACON_FRAME_LEN, beacon_addr, NULL, beacon_addr,
	                                   (uint8_t *)beacon_ssid, config.u.ssid.ssid_len, beacon_chan);
	beacon_frame.data = beacon_frame_buf;
	beacon_frame.len = beacon_len;
	CMD_DBG("beacon_len %d\n", beacon_len);
}

static void send_beacon_frame(void)
{
	wlan_send_raw_frame(g_wlan_netif,IEEE80211_FC_STYPE_AUTH,
		beacon_frame.data, beacon_frame.len);
}
#endif

static int wifi_set_channel(uint8_t chan)
{
	if (g_wlan_netif == NULL) {
		CMD_ERR("%s, %d, warning: netif is null\n", __func__, __LINE__);
		return 0;
	}

	enum wlan_mode mode = wlan_if_get_mode(g_wlan_netif);

	//CMD_DBG("chann %d\n", chan);

	if (mode == WLAN_MODE_HOSTAP) {
		wlan_ext_sniffer_param_t sniffer;
		if (chan == 0) {
			sniffer.dwell_time = 0;
		} else {
			sniffer.dwell_time = 120 * 1000 * 1000;  //more time for geting ssid and pwd
		}
		sniffer.channel = chan;
		if (wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_SNIFFER, (uint32_t)(uintptr_t)&sniffer) != 0) {
			CMD_ERR("set sniffer fail, chan %u, dwell time %u\r\n",
			    sniffer.channel, sniffer.dwell_time);
			return -1;
		}
		if (chan != 0) {
			if (chan == assist_test_ap_chan) {
				auth_frame_creat();
				send_auth_frame();
			}
		}
		return 0;
	}
	return -1;
}

static void recv_rawframe(uint8_t *data, uint32_t len, void *info)
{
	struct frame_info *p;
	struct ieee80211_frame *frame;
	p = (struct frame_info *)info;

#if MONITOR_RX_HT40_PHY_INFO
	if (p->type == 0xff) {
		RX_EXT_FRAMES_IND *frame_ind;
		frame_ind = (RX_EXT_FRAMES_IND *)data;
		if (pkt_print_enable == 1) {/* rx 40M pkt print */
			CMD_DBG("40M:frame %d, drop %d\n", frame_ind->frame_num, frame_ind->frame_drop);
			int i = 0;
			for (i = 0; i < frame_ind->frame_num; i++) {
				CMD_DBG("40M:chan=%d, rate=%d, size=%d, flag=0x%04x rssi=%d,"
				    "reserved=0x%04x\n",
				    frame_ind->frames[i].chanNumber,
				    frame_ind->frames[i].rxedRate,
				    frame_ind->frames[i].frame_size,
				    frame_ind->frames[i].flags,
				    frame_ind->frames[i].Rssi,
				    frame_ind->frames[i].reserved);
			}
		}
		return;
	}

	//return ; /* for ht40/mimo test only, delete ht20 pkt */
#endif

	if (pkt_print_enable == 1) {/* rx 20M pkt print */
		CMD_LOG(1, "20M:rx data %p, len %u, ", data, len);
		if (p) {
			CMD_LOG(1, "20M:type 0x%02x, rx ch %d, ap ch %d, rssi %u, alg %d\n",
			    p->type, p->recv_channel, p->ap_channel, p->rssi, p->alg);
		} else {
			CMD_LOG(1, "info NULL\n");
		}
	}

	frame = (struct ieee80211_frame *)data;
#if 0
// ####broadcast pkt print######
	//ap forward pkt
	//addr1 bd: ff:ff:ff:ff:ff:ff  rx
	//addr2 #32ap 88:25:93:66:da:aa  tx
	//addr3 mate9  44:c3:46:45:5f:4f   src

	if (((frame->i_addr2[0] == 0x88 && frame->i_addr2[1] == 0x25 && frame->i_addr2[2] == 0x93) &&
	    (frame->i_addr3[0] == 0x44 && frame->i_addr3[1] == 0xc3 && frame->i_addr3[2] == 0x46)) &&
	    ((frame->i_addr1[0] == 0xff && frame->i_addr1[1] == 0xff && frame->i_addr1[2] == 0xff))) {
		//printf("ap:len %d, seq %d\n", len, ((*(uint16_t *)(&frame->i_seq[0])) >> 4));
	}

	//mate9 to #32ap
	//addr1 #32ap 88:25:93:66:da:aa  rx
	//addr2 mate9  44:c3:46:45:5f:4f  tx
	//addr3 bd: ff:ff:ff:ff:ff:ff   dst
		if (((frame->i_addr1[0] == 0x88 && frame->i_addr1[1] == 0x25 && frame->i_addr1[2] == 0x93) &&
		    (frame->i_addr2[0] == 0x44 && frame->i_addr2[1] == 0xc3 && frame->i_addr2[2] == 0x46)) &&
		    ((frame->i_addr3[0] == 0xff && frame->i_addr3[1] == 0xff && frame->i_addr3[2] == 0xff))) {
			//printf("sta:len %d, seq %d\n", len, ((*(uint16_t *)(&frame->i_seq[0])) >> 4));
		}

//####multicast pkt print######
		//ap forward pkt
		//addr1 bd: 01:00:5e:xx:xx:xx rx
		//addr2 #32ap 88:25:93:66:da:aa  tx
		//addr3 mate9  44:c3:46:45:5f:4f   src

		if (((frame->i_addr2[0] == 0x88 && frame->i_addr2[1] == 0x25 && frame->i_addr2[2] == 0x93) &&
		    (frame->i_addr3[0] == 0x44 && frame->i_addr3[1] == 0xc3 && frame->i_addr3[2] == 0x46)) &&
		    ((frame->i_addr1[0] == 0x01 && frame->i_addr1[1] == 0x00 && frame->i_addr1[2] == 0x5e))) {
			//printf("ap m:len %d, seq %d\n", len, ((*(uint16_t *)(&frame->i_seq[0])) >> 4));
		}

		//mate9 to #32ap
		//addr1 #32ap 88:25:93:66:da:aa  rx
		//addr2 mate9  44:c3:46:45:5f:4f  tx
		//addr3 bd: 01:00:5e:xx:xx:xx    dst

		if (((frame->i_addr1[0] == 0x88 && frame->i_addr1[1] == 0x25 && frame->i_addr1[2] == 0x93) &&
		    (frame->i_addr2[0] == 0x44 && frame->i_addr2[1] == 0xc3 && frame->i_addr2[2] == 0x46)) &&
		    ((frame->i_addr3[0] == 0x01 && frame->i_addr3[1] == 0x00 && frame->i_addr3[2] == 0x5e))) {
			printf("sta m:len %d, seq %d\n",
			len, ((*(uint16_t *)(&frame->i_seq[0])) >> 4));
		}
#endif

	sniffer_rx_total_cnt++;
	if (info) {
		if ((p->type == IEEE80211_FC_STYPE_BEACON) ||
		    (p->type == IEEE80211_FC_STYPE_PROBE_RESP)) {
			if (!memcmp(assist_test_ap_mac, frame->i_addr2, 6)) {
				assist_test_ap_chan = p->ap_channel;
				//printf("assist_test_ap_chan %d\n", assist_test_ap_chan);
			}
			if (p->ap_channel > 14 || p->ap_channel < 1) {
				CMD_DBG("p->ap_channel=%d\r\n", p->ap_channel);
				return;
			}
		}

		uint16_t auth_transaction = *((uint16_t*)(&data[26]));
		if (frame) {
			if(p->type == IEEE80211_FC_STYPE_AUTH) {
				if (auth_transaction == 2) { //auth_transaction=2, auth seq num, auth resp
					return;
				} else if(auth_transaction == 1){ //auth req
					if (memcmp(frame->i_addr1, g_wlan_netif->hwaddr, 6) == 0){
						CMD_DBG("rx sta con req\n");
						if (wifi_set_channel(0) != 0) {
							CMD_ERR("wifi_set_channel() 0 fail\n");
						}
						sniffer_run = 0;
					}
				}
			}
		}
	}
}

static int wifi_set_promisc(int enable)
{
	enum wlan_mode mode;
	if (g_wlan_netif == NULL) {
		CMD_ERR("%s, %d, warning: netif is null\n", __func__, __LINE__);
		return -1;
	}

	mode = wlan_if_get_mode(g_wlan_netif);
	if (mode != WLAN_MODE_HOSTAP && mode != WLAN_MODE_MONITOR) {
		CMD_ERR("wlan mode is wrong\n");
		return -1;
	}

	return wlan_monitor_set_rx_cb(g_wlan_netif, enable ? recv_rawframe : NULL);
}

static int wifi_softap_set_sniffer(int enable)
{
	return wifi_set_promisc(enable);
}

static enum cmd_status sniffer_chan_test(char *cmd)
{
	int chan;
	cmd_sscanf(cmd, "c=%d", &chan);
	CMD_DBG("set sniffer test chan %d\n", chan);

	wlan_ap_config_t config;
	cmd_memset(&config, 0, sizeof(config));
	config.field = WLAN_AP_FIELD_CHANNEL;
	if (wlan_ap_get_config(&config) != 0) {
		CMD_ERR("get config failed\n");
		return CMD_STATUS_FAIL;
	}
	CMD_DBG("softap channel: %d\n", config.u.channel);

#if MONITOR_RX_HT40_PHY_INFO
	enable_rx_ext_frame();
#endif

	// init sniffer mode, register sniffer callback
	if (wifi_softap_set_sniffer(1) != 0) {
		CMD_ERR("wifi_softap_set_sniffer() fail\n");
	}

	if (wifi_set_channel(chan) != 0) {
		CMD_ERR("wifi_set_channel() %u fail\n", chan);
	}
	return CMD_STATUS_OK;
}

static void sniffer_task(void *arg)
{
	int i;
	char *ssid = "ap_sniffer";
	int ssid_len = strlen(ssid);
	char *pwd = "12345678";
	wlan_ap_config_t config;
	uint32_t sniffer_time = ((uint32_t)(uintptr_t)arg) & 0xffff;
	uint32_t softap_time = (((uint32_t)(uintptr_t)arg) >> 16) & 0xffff;


	CMD_DBG("sniffer time %u ms, softap time %u ms\n", sniffer_time, softap_time);
	wlan_ap_disable();
	if (wlan_ap_set((uint8_t *)ssid, ssid_len, (uint8_t *)pwd)) {
		CMD_ERR("creat ap failed\n");
	}

	config.field = WLAN_AP_FIELD_MAX_NUM_STA;
	config.u.max_num_sta = 4;
	wlan_ap_set_config(&config);

	config.field = WLAN_AP_FIELD_CHANNEL;
	config.u.channel = 6; /* 6 channel */
	wlan_ap_set_config(&config);

	config.field = WLAN_AP_FIELD_BEACON_INT;
	config.u.beacon_int = 1000;  /* beacon interval 1s */
	wlan_ap_set_config(&config);

	wlan_ap_enable();
	XR_OS_MSleep(500);


	cmd_memset(&config, 0, sizeof(config));
	config.field = WLAN_AP_FIELD_CHANNEL;
	if (wlan_ap_get_config(&config) != 0) {
		CMD_ERR("get config failed\n");
		return;
	}
#if BEACON_TX
	beacon_chan = config.u.channel;
	beacon_frame_creat();
#endif
	CMD_DBG("softap channel: %d\n", config.u.channel);
#if MONITOR_RX_HT40_PHY_INFO
	enable_rx_ext_frame();
#endif
	// init sniffer mode, register sniffer callback
	if (wifi_softap_set_sniffer(1) != 0) {
		CMD_ERR("wifi_softap_set_sniffer() fail\n");
	}

	while (sniffer_run) {
		for (i = 0; i < nitems(sniffer_channels); ++i) {
			if (!sniffer_run) {
				break;
			}
			// switch to sniffer mode
			if (wifi_set_channel(sniffer_channels[i]) != 0) {
				CMD_ERR("wifi_set_channel() %u fail\n", sniffer_channels[i]);
			}
			XR_OS_MSleep(sniffer_time);

			// switch back to softap mode
			if (wifi_set_channel(0) != 0) {
				CMD_ERR("wifi_set_channel() 0 fail\n");
			}
#if BEACON_TX
			send_beacon_frame();
#endif
			XR_OS_MSleep(softap_time);
		}
	}

	CMD_DBG("sniffer task stop\n");
#if MONITOR_RX_HT40_PHY_INFO
	disable_rx_ext_frame();
#endif
	// deinit sniffer mode, unregister sniffer callback
	if (wifi_set_channel(0) != 0) {
		CMD_ERR("wifi_set_channel() 0 fail\n");
	}

	if (wifi_softap_set_sniffer(0) != 0) {
		CMD_ERR("wifi_softap_set_sniffer() stop fail\n");
	}

	XR_OS_ThreadDelete(&sniffer_thread);
}

/* $ sniffer_start t1=100 t2=100 */
static enum cmd_status cmd_sniffer_start_exec(char *cmd)
{
	uint32_t sniffer_time, softap_time, time;
	struct netif *nif = g_wlan_netif;

	if (wlan_if_get_mode(nif) != WLAN_MODE_HOSTAP) {
		CMD_ERR("create sniffer task should run in softAP mode!\n");
		return CMD_STATUS_FAIL;
	}

	if (cmd_sscanf(cmd, "t1=%u t2=%u", &sniffer_time, &softap_time) == 2) {
		time = (sniffer_time & 0xffff) | (softap_time << 16);
		sniffer_run = 1;
		if (XR_OS_ThreadCreate(&sniffer_thread,
							"sniffer",
							sniffer_task,
							(void *)(uintptr_t)time,
							XR_OS_THREAD_PRIO_APP,
							2 * 1024) != XR_OS_OK) {
			CMD_ERR("[ERR] create sniffer task failed\n");
			return CMD_STATUS_FAIL;
		}
		return CMD_STATUS_OK;
	} else {
		return CMD_STATUS_INVALID_ARG;
	}
}

/* $ sniffer_stop */
static enum cmd_status cmd_sniffer_stop_exec(char *cmd)
{
	sniffer_run = 0;
	while (XR_OS_ThreadIsValid(&sniffer_thread)) {
		XR_OS_MSleep(1); /* wait for thread termination */
	}
	return CMD_STATUS_OK;
}

static enum cmd_status monitor_chan_test(char *cmd)
{
	int ret;
	int chan;
	CMD_DBG("monitor start\n");
	cmd_sscanf(cmd, "c=%d", &chan);

	net_switch_mode(WLAN_MODE_MONITOR);
#if MONITOR_RX_HT40_PHY_INFO
	enable_rx_ext_frame();
#endif
	wifi_set_promisc(1);

	CMD_DBG("set chan %d\n", chan);
	ret = wlan_monitor_set_channel(g_wlan_netif, chan);
	if (ret != 0) {
		CMD_ERR("set chan fail %d\n", chan);
	}

	return CMD_STATUS_OK;
}

static void monitor_task(void *arg)
{
	int i;
	int ret;
	CMD_DBG("monitor start\n");

	net_switch_mode(WLAN_MODE_MONITOR);
#if MONITOR_RX_HT40_PHY_INFO
	enable_rx_ext_frame();
#endif
	wifi_set_promisc(1);
	while (monitor_run) {
		for (i = 0; i < nitems(sniffer_channels); ++i) {
			//printf("chan %d\n", sniffer_channels[i]);
			ret = wlan_monitor_set_channel(g_wlan_netif, (int16_t)sniffer_channels[i]);
			if (ret != 0) {
				CMD_ERR("set chan fail\n");
			}
			if (sniffer_channels[i] == assist_test_ap_chan) {
				auth_frame_creat();
				send_auth_frame();
			}
			XR_OS_MSleep(100);
		}
	}
#if MONITOR_RX_HT40_PHY_INFO
	disable_rx_ext_frame();
#endif
	wifi_set_promisc(0);
	net_switch_mode(WLAN_MODE_STA);
	CMD_DBG("monitor stop\n");

	XR_OS_ThreadDelete(&monitor_thread);
}

/* monitro start */
static enum cmd_status cmd_monitor_start_exec(char *cmd)
{
		monitor_run = 1;
		if (XR_OS_ThreadCreate(&monitor_thread,
							"monitor",
							monitor_task,
							NULL,
							XR_OS_THREAD_PRIO_APP,
							2 * 1024) != XR_OS_OK) {
			CMD_ERR("[ERR] create sniffer task failed\n");
			return CMD_STATUS_FAIL;
		}
		return CMD_STATUS_OK;
}

/* $ monitor_stop */
static enum cmd_status cmd_monitor_stop_exec(char *cmd)
{
	monitor_run = 0;
	while (XR_OS_ThreadIsValid(&monitor_thread)) {
		XR_OS_MSleep(1); /* wait for thread termination */
	}
	return CMD_STATUS_OK;
}

static enum cmd_status cmd_sniffer_statistics_start_exec(char *cmd)
{
	sniffer_rx_total_cnt = 0;
	statistics_start_time =  XR_OS_JiffiesToMSecs(XR_OS_GetJiffies());

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_sniffer_statistics_stop_exec(char *cmd)
{
	statistics_end_time = XR_OS_JiffiesToMSecs(XR_OS_GetJiffies());
	CMD_DBG("rx cnt %u, time total %u s %u ms, avg_ps %u\n",
	    sniffer_rx_total_cnt,
	    (statistics_end_time-statistics_start_time) / 1000,
	    (statistics_end_time-statistics_start_time) % 1000,
	     1000 * sniffer_rx_total_cnt/(statistics_end_time-statistics_start_time));
	return CMD_STATUS_OK;
}

static enum cmd_status cmd_sniffer_pkt_print_enable_exec(char *cmd)
{
	int cnt;

	cnt = cmd_sscanf(cmd, "%d", &pkt_print_enable);
	if (cnt != 1) {
		CMD_ERR("cnt %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	return CMD_STATUS_OK;
}
#endif

static enum cmd_status cmd_wlan_set_stay_awake_tmo(char *cmd)
{
	int ret, cnt;
	uint32_t tmo;

	cnt = cmd_sscanf(cmd, "t=%d", &tmo);
	if (cnt != 1) {
		CMD_ERR("cnt %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_STAY_AWAKE_TMO, tmo);

	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	return CMD_STATUS_ACKED;
}

static enum cmd_status cmd_wlan_set_chk_bcn_without_data(char *cmd)
{
	int ret, cnt;
	uint32_t enable, count;
	wlan_ext_chk_bcn_without_data_set_t param;

	cnt = cmd_sscanf(cmd, "e=%d c=%d", &enable, &count);

	if (cnt != 2) {
		CMD_ERR("cnt %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}
	param.enable = enable;
	param.beacon_count = count;
	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_BCN_WITHOUT_DATA, (uint32_t)(uintptr_t)&param);

	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_wlan_set_bcn_tim_no_data_tmo(char *cmd)
{
	int ret, cnt;
	uint32_t enable, tmo;
	wlan_ext_bcn_tim_no_data_tmo_set_t param;

	cnt = cmd_sscanf(cmd, "e=%d t=%d", &enable, &tmo);

	if (cnt != 2) {
		CMD_ERR("cnt %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}
	param.enable = enable;
	param.timeout_ms = tmo;
	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_BCN_TIM_NO_DATA_TMO, (uint32_t)(uintptr_t)&param);

	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	return CMD_STATUS_OK;
}

/* eg: net wlan set_filter_type ft=0x1 :FW filter AP BAR frame after Host standby
       net wlan set_filter_type ft=0x2 :FW filter AP PING frame after Host standby
       net wlan set_filter_type ft=0x3 :FW filter AP BAR frame and Ping after Host standby
       net wlan set_filter_type ft=0x0 :disable filter */
static enum cmd_status cmd_wlan_set_filter_type(char *cmd)
{
	int ret;
	uint32_t type;
	int cnt;

	cnt = cmd_sscanf(cmd, "ft=0x%x", &type);
	if (cnt != 1) {
		CMD_ERR("cnt %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_FILTER_TYPE, type);
	if (type & FILTER_PACKET_BAR) {
		CMD_LOG(1, "type:0x%x,filter BAR frame\n", type);
	}
	if (type & FILTER_PACKET_PING) {
		CMD_LOG(1, "type:0x%x,filter PING frame\n", type);
	}

	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}
	return CMD_STATUS_OK;
}

/* eg: net wlan bcn_lost_comp e=1 dl=1 ci=1 cc=5 */
static enum cmd_status cmd_wlan_set_bcn_lost_comp(char *cmd)
{
	int ret, cnt;
	int enable;
	int dtim_lost;
	int comp_interval;
	int comp_count;
	wlan_ext_bcn_lost_comp_set_t param;

	cnt = cmd_sscanf(cmd, "e=%d dl=%d ci=%d cc=%d",
		&enable, &dtim_lost, &comp_interval, &comp_count);
	if (cnt != 4) {
		CMD_ERR("cnt %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	memset(&param, 0, sizeof(wlan_ext_bcn_lost_comp_set_t));
	param.Enable = enable;
	param.DtimLostNum = dtim_lost;
	param.CompInterval = comp_interval;
	param.CompCnt = comp_count;
	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_BCN_LOST_COMP, (uint32_t)(uintptr_t)(&param));

	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	return CMD_STATUS_ACKED;
}

/* eg: net wlan arp_reply_keep_alive p=48 senIp=192.168.51.100 tarIp=192.168.51.1 tarMac=xx:xx:xx:xx:xx:xx */
static enum cmd_status cmd_wlan_arp_reply_keep_alive(char *cmd)
{
	int ret, cnt;
	uint32_t ArpKeepAlivePeriod;
	uint32_t sernder_ip[4];
	uint32_t target_ip[4];
	uint32_t target_mac[6];

	cnt = cmd_sscanf(cmd, "p=%d senIp=%d.%d.%d.%d tarIp=%d.%d.%d.%d tarMac=%x:%x:%x:%x:%x:%x",
		&ArpKeepAlivePeriod, &sernder_ip[0], &sernder_ip[1], &sernder_ip[2], &sernder_ip[3],
		&target_ip[0], &target_ip[1], &target_ip[2], &target_ip[3], &target_mac[0], &target_mac[1],
		&target_mac[2], &target_mac[3], &target_mac[4], &target_mac[5]);
	if (cnt != 15) {
		CMD_ERR("cnt %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	wlan_ext_arp_kpalive_set_t param;
	memset(&param, 0, sizeof(wlan_ext_arp_kpalive_set_t));
	param.ArpKeepAlivePeriod = (uint16_t)ArpKeepAlivePeriod;
	for (int i = 0; i < 4; i++) {
		param.SenderIpv4IpAddress[i] = (uint8_t)sernder_ip[i];
		param.TargetIpv4IpAddress[i] = (uint8_t)target_ip[i];
	}
	for (int i = 0; i < 6; i++) {
		param.TargetMacAddress[i] = (uint8_t)target_mac[i];
	}
	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_ARP_KPALIVE, (uint32_t)(uintptr_t)(&param));

	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	return CMD_STATUS_ACKED;
}

static enum cmd_status cmd_wlan_get_bcn_rssi(char *cmd)
{
	int ret;
	wlan_ext_bcn_rssi_dbm_get_t bcn_rssi;

	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_GET_BCN_RSSI_DBM, (uint32_t)(uintptr_t)(&bcn_rssi));

	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}
	CMD_LOG(1, "bcn rssi:\n");
	CMD_LOG(1, "rssi_ewma:%d\n", bcn_rssi.rssi_ewma);
	CMD_LOG(1, "rssi_new:%d\n", bcn_rssi.rssi_new);

	return CMD_STATUS_ACKED;
}

static enum cmd_status cmd_wlan_get_stats_code(char *cmd)
{
	int ret;
	wlan_ext_stats_code_get_t param;

	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_GET_STATS_CODE, (uint32_t)(uintptr_t)(&param));

	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}
	CMD_LOG(1, "reason code:%d status code:%d\n", param.reason_code, param.status_code);

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_wlan_set_bss_loss_thold(char *cmd)
{
	int ret, cnt;
	uint32_t bss_loss_thold, link_loss_thold;
	wlan_ext_bss_loss_thold_set_t param;

	cnt = cmd_sscanf(cmd, "b=%d l=%d", &bss_loss_thold, &link_loss_thold);
	if (cnt != 2) {
		CMD_ERR("cnt %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	cmd_memset(&param, 0, sizeof(wlan_ext_bss_loss_thold_set_t));
	param.bss_loss_thold = bss_loss_thold;
	param.link_loss_thold = link_loss_thold;
	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_BSS_LOSS_THOLD, (uint32_t)(uintptr_t)(&param));

	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_wlan_set_upload_prb_req(char *cmd)
{
	int ret;
	uint32_t enable;
	int cnt;

	cnt = cmd_sscanf(cmd, "e=%d", &enable);
	if (cnt != 1) {
		CMD_ERR("cnt %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_UPLOAD_PRB_REQ, enable);
	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_wlan_set_auth_tmo_and_tries(char *cmd)
{
	int ret;
	int cnt;
	int tmo, tries;
	wlan_ext_mgmt_timeout_and_tries_set_t param;

	cnt = cmd_sscanf(cmd, "tmo=%d, tries=%d", &tmo, &tries);
	if (cnt != 2) {
		CMD_ERR("cnt %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	cmd_memset(&param, 0, sizeof(wlan_ext_mgmt_timeout_and_tries_set_t));
	param.timeout = tmo;
	param.tries = tries;
	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_AUTH_TMO_AND_TRIES, (uint32_t)(uintptr_t)(&param));

	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_wlan_set_assoc_tmo_and_tries(char *cmd)
{
	int ret;
	int cnt;
	int tmo, tries;
	wlan_ext_mgmt_timeout_and_tries_set_t param;

	cnt = cmd_sscanf(cmd, "tmo=%d, tries=%d", &tmo, &tries);
	if (cnt != 2) {
		CMD_ERR("cnt %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	cmd_memset(&param, 0, sizeof(wlan_ext_mgmt_timeout_and_tries_set_t));
	param.timeout = tmo;
	param.tries = tries;
	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_ASSOC_TMO_AND_TRIES, (uint32_t)(uintptr_t)(&param));

	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_wlan_set_auto_power(char *cmd)
{
	int ret, cnt;
	float pwr_step;
	uint32_t period, rssi_thres, max_rssi, min_rssi;
	wlan_ext_auto_power_t param;

	cnt = cmd_sscanf(cmd, "p=%d s=%f t=%d max=%d min=%d", &period, &pwr_step, &rssi_thres, &max_rssi, &min_rssi);

	if (cnt != 5) {
		CMD_ERR("cnt %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}
	param.period = period;
	param.pwr_step = pwr_step * 10;
	param.rssi_thres = rssi_thres;
	param.max_rssi = max_rssi;
	param.min_rssi = min_rssi;
	ret = wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_AUTO_POWER, (uint32_t)(uintptr_t)&param);

	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_wlan_set_low_power_param(char *cmd)
{
	int ret, cnt;
	uint32_t period;

	cnt = cmd_sscanf(cmd, "p=%d", &period);
	if (cnt != 1) {
		CMD_ERR("cnt %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	ret = wlan_ext_low_power_param_set_default(g_wlan_netif, period);

	if (ret == -2) {
		CMD_ERR("%s: command '%s' invalid arg\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	} else if (ret == -1) {
		CMD_ERR("%s: command '%s' exec failed\n", __func__, cmd);
		return CMD_STATUS_ACKED;
	}

	return CMD_STATUS_OK;
}

/*
 * wlan commands
 */

static enum cmd_status cmd_wlan_help_exec(char *cmd);

static const struct cmd_data g_wlan_cmds[] = {
	{ "stop",                   cmd_wlan_stop_exec },
	{ "start",                  cmd_wlan_start_exec },
	{ "pm_dtim",                cmd_wlan_set_pm_dtim },
	{ "get_pm_dtim",            cmd_wlan_get_pm_dtim },
	{ "set_lsn_intv",           cmd_wlan_set_listen_interval },
	{ "ps_mode",                cmd_wlan_set_ps_mode },
	{ "pm",                     cmd_wlan_pm },
	{ "ampdu",                  cmd_wlan_set_ampdu },
	{ "retry",                  cmd_wlan_set_retry },
	{ "null_prd",               cmd_wlan_set_pm_tx_null_period },
	{ "bcn_win_cfg",            cmd_wlan_set_bcn_win_cfg },
	{ "bcn_win",                cmd_wlan_set_bcn_win_us },
	{ "get_bcn_stat",           cmd_wlan_get_bcn_stat },
	{ "get_cur_rate",           cmd_wlan_get_cur_tx_rate },
	{ "get_cur_signal",         cmd_wlan_get_cur_signal },
	{ "scan_freq",              cmd_wlan_scan_freq },
	{ "set_scan_param",         cmd_wlan_set_scan_param },
	{ "get_temp_volt",          cmd_wlan_get_temp_volt },
	{ "set_tv_upload",          cmd_wlan_set_temp_volt_auto_upload },
	{ "set_tv_thresh",          cmd_wlan_set_temp_volt_thresh },
	{ "set_edca_param",         cmd_wlan_set_edca_param },
	{ "get_edca_param",         cmd_wlan_get_edca_param },
	{ "set_bcn_freq_offs_time", cmd_wlan_set_bcn_freq_offs_time },
	{ "set_lfclock_param",      cmd_wlan_set_lfclock_param },
	{ "set_bcn_rx_11b_only",    cmd_wlan_set_bcn_rx_11b_only },
	{ "get_bcn_rssi",           cmd_wlan_get_bcn_rssi },
	{ "get_reason_code",        cmd_wlan_get_stats_code },
	{ "set_bss_loss_thold",     cmd_wlan_set_bss_loss_thold },
	{ "set_upload_prb_req",     cmd_wlan_set_upload_prb_req },
	{ "set_auth_tmo_and_tries", cmd_wlan_set_auth_tmo_and_tries },
	{ "set_assoc_tmo_and_tries",cmd_wlan_set_assoc_tmo_and_tries },
	{ "bcn_lost_comp",          cmd_wlan_set_bcn_lost_comp },
	{ "arp_reply_keep_alive",   cmd_wlan_arp_reply_keep_alive },
	{ "set_filter_type",        cmd_wlan_set_filter_type },
#if SUPPORT_BCN_AUTO_OFFSET
	{ "set_bcn_auto_offset",    cmd_wlan_set_bcn_auto_offset },
	{ "set_stay_awake_tmo",     cmd_wlan_set_stay_awake_tmo },
#endif
	{ "set_chk_bcn_without_data", cmd_wlan_set_chk_bcn_without_data },
	{ "set_bcn_tim_no_data_tmo", cmd_wlan_set_bcn_tim_no_data_tmo },
	{ "set_pre_rx_bcn",         cmd_wlan_set_pre_rx_bcn },
	{ "set_auto_power",         cmd_wlan_set_auto_power },
	{ "set_lp_param",           cmd_wlan_set_low_power_param },
#if LOW_POWER_IP_SERVICE
	{ "set_auto_scan",          cmd_wlan_set_auto_scan },
	{ "ser_start",              cmd_wlan_server_start },
	{ "ser_send",               cmd_wlan_server_send },
	{ "ser_recv",               cmd_wlan_server_recv },
	{ "ser_stop",               cmd_wlan_server_stop },
	{ "cli_start",              cmd_wlan_client_start },
	{ "cli_send",               cmd_wlan_client_send },
	{ "cli_recv",               cmd_wlan_client_recv },
	{ "cli_stop",               cmd_wlan_client_stop },
	{ "p2p_server",             cmd_wlan_set_p2p_server },
	{ "p2p_wkpacket",           cmd_wlan_set_p2p_wkpacket },
	{ "p2p_wakeupip",           cmd_wlan_set_p2p_wakeupip },
	{ "p2p_kpalive",            cmd_wlan_set_p2p_kpalive },
	{ "p2p_hostsleep",          cmd_wlan_set_p2p_hostsleep },
#endif
#if MONITOR_RX_HT40_PHY_INFO
	{ "ht40_rx_start",          cmd_mimo_start_exec},
	{ "ht40_rx_stop",           cmd_mimo_stop_exec},
#endif
#if AP_SNIFFER
	{ "sniffer_start",          cmd_sniffer_start_exec},
	{ "sniffer_stop",           cmd_sniffer_stop_exec},
	{ "sniffer_set_addr",       cmd_sniffer_set_assist_test_ap_addr_exec},
	{ "sniffer_chan",           sniffer_chan_test},
	{ "sniffer_info_start",     cmd_sniffer_statistics_start_exec},
	{ "sniffer_info_stop",      cmd_sniffer_statistics_stop_exec},
	{ "sniffer_pkt_print",      cmd_sniffer_pkt_print_enable_exec},

	{ "monitor_start",          cmd_monitor_start_exec},
	{ "monitor_stop",           cmd_monitor_stop_exec},
	{ "monitor_chan",           monitor_chan_test},
#endif

#if NONCONNECT_SNIFF
	{ "rcv_spec_frm",           cmd_wlan_set_rcv_special_frm },
	{ "raw_frm_cfg",            cmd_wlan_send_raw_frm_cfg },
	{ "sniff_kp_active",        cmd_wlan_sniff_kp_active },
	{ "sniff_auto_wkp_host",    cmd_wlan_set_sniff_auto_wakeup_host },
	{ "sniff_auto_wkp_wifi",    cmd_wlan_set_sniff_auto_wakeup_wifi },
	{ "send_raw_frm",           cmd_wlan_send_raw_frm },
	{ "switch_chn",             cmd_wlan_switch_channel_cfg },
	{ "get_chn",                cmd_wlan_get_cur_channel },
	{ "rcv_filter_mac",         cmd_wlan_rcv_frm_filter_mac },
	{ "set_frm_filter",         cmd_wlan_set_frm_filter },
	{ "rcv_filter1_ie",         cmd_wlan_rcv_frm_filter1_ie_cfg },
	{ "rcv_filter2_ie",         cmd_wlan_rcv_frm_filter2_ie_cfg },
	{ "rcv_filter1_pl",         cmd_wlan_rcv_frm_filter1_pl_cfg },
	{ "rcv_filter2_pl",         cmd_wlan_rcv_frm_filter2_pl_cfg },
	{ "set_temp_frm",           cmd_wlan_set_temp_frm },
	{ "send_sync_frm",          cmd_wlan_send_sync_frm },
	{ "set_ext_param",          cmd_wlan_set_sniff_ext_param },
	{ "get_rcv_stat",           cmd_wlan_get_rcv_stat },
	{ "set_rcv_cb",             cmd_wlan_set_rcv_cb },
	{ "set_even_ie",            cmd_wlan_set_even_ie },
#endif
	{ "help",                   cmd_wlan_help_exec },
};

static enum cmd_status cmd_wlan_help_exec(char *cmd)
{
	return cmd_help_exec(g_wlan_cmds, cmd_nitems(g_wlan_cmds), 16);
}

enum cmd_status cmd_wlan_exec(char *cmd)
{
	if ((g_wlan_netif == NULL) && (cmd_strcmp(cmd, "start") != 0)) {
		CMD_ERR("wlan no start\n");
		return CMD_STATUS_FAIL;
	}

	return cmd_exec(cmd, g_wlan_cmds, cmd_nitems(g_wlan_cmds));
}
