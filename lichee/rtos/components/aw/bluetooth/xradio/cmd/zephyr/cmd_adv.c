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

#if defined(CONFIG_BLEHOST)

#include "cmd_util.h"
#include "ble/bluetooth/bluetooth.h"

#define OS_ThreadIsValid XR_OS_ThreadIsValid
#define OS_ThreadCreate XR_OS_ThreadCreate
#define OS_ThreadDelete XR_OS_ThreadDelete
#define OS_MSleep XR_OS_MSleep
#define OS_THREAD_PRIO_APP XR_OS_THREAD_PRIO_APP
#define OS_OK XR_OS_OK

#define ADV_THREAD_STACK_SIZE        (2 * 1024)

#define ADV_CREAT_THREAD(THREAD, TASK, ARG) \
	{ \
		if (OS_ThreadIsValid(&THREAD)) { \
			CMD_ERR("adv task is running\n"); \
			return CMD_STATUS_FAIL; \
		} \
		if (OS_ThreadCreate(&THREAD, \
		                    "", \
		                    TASK, \
		                    (void *)ARG, \
		                    OS_THREAD_PRIO_APP, \
		                    ADV_THREAD_STACK_SIZE) != OS_OK) { \
			CMD_ERR("adv task create failed\n"); \
			return CMD_STATUS_FAIL; \
		} \
	}
#define ADV_DELETE_THREAD(THREAD)    OS_ThreadDelete(&THREAD)

static OS_Thread_t g_adv_start_thread;
static uint8_t g_task_end = 1;
static uint32_t gAdvIntMin, gAdvIntMax;
extern uint8_t selected_id;
#if 0
#define ADV_CONN_NAME_EX BT_LE_ADV_PARAM(BT_LE_ADV_OPT_NONE, \
                                         BT_GAP_ADV_FAST_INT_MIN_2, \
                                         0X00B0, NULL) /* 100~110ms */

struct bt_data adv_data[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(0xff, 0x01, 0x00), //manu_type: BT_DATA_MANUFACTURER_DATA
	BT_DATA_BYTES(BT_DATA_NAME_COMPLETE, 't', 'e', 's', 't'),
};
#endif

static int adv_start(uint8_t manu_type, uint16_t index, uint32_t chan)
{
	int err;

	struct bt_data adv_data[] = {
		BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
		BT_DATA_BYTES(manu_type, index & 0xFF, (index >> 8) & 0xFF), //manu_type: BT_DATA_MANUFACTURER_DATA
		BT_DATA_BYTES(BT_DATA_NAME_COMPLETE, 't', 'e', 's', 't'),
	};
	struct bt_le_adv_param adv_param[] = {
		{
		.options = BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_USE_IDENTITY,
		.interval_min = gAdvIntMin > 0 ? gAdvIntMin : BT_GAP_ADV_FAST_INT_MIN_2,
		.interval_max = gAdvIntMax > 0 ? gAdvIntMax : 0x00B0,
		.id = selected_id,
		},
	 };

	if (!(chan & 0x01))
		adv_param[0].options |= BT_LE_ADV_OPT_DISABLE_CHAN_37;
	if (!(chan & 0x02))
		adv_param[0].options |= BT_LE_ADV_OPT_DISABLE_CHAN_38;
	if (!(chan & 0x04))
		adv_param[0].options |= BT_LE_ADV_OPT_DISABLE_CHAN_39;

	err = bt_le_adv_start(adv_param, adv_data, ARRAY_SIZE(adv_data), NULL, 0);
	if (err) {
		CMD_ERR("Advertising failed to start (err %d)\n", err);
		return -1;
	}

	return 0;
}

static int adv_stop(void)
{
	return bt_le_adv_stop();
}

__weak void get_tick_adv_start(void)
{
}

static void adv_start_exec(void *cmd)
{
	int cnt;
	uint32_t index, adv_times, delay;

	(void)delay;
#if 0
	cnt = cmd_sscanf(cmd, "n=%d", &adv_times);
	if (cnt != 1 || adv_times > 0xFFFF) {
		CMD_ERR("invalid arg\n");
		goto exit;
	}
#else
	uint32_t interval, channel, manu_type;
	cnt = cmd_sscanf(cmd, "int=0x%x chl=0x%x manu_type=0x%x num=%d",
	                    &interval, &channel, &manu_type, &adv_times);
	if (cnt != 4 || adv_times > 0xFFFF) {
		CMD_ERR("invalid arg\n");
		goto exit;
	}

	gAdvIntMin = interval;
	gAdvIntMax = interval;

	uint32_t gAdvIntMin_ms = interval * 0.625;
	delay = (gAdvIntMin_ms / 2 + 3) > gAdvIntMin_ms ? gAdvIntMin_ms : (gAdvIntMin_ms / 2 + 3);
#endif

	CMD_DBG("Advertising times=%d\n", adv_times);
	CMD_DBG("Advertising config: interval=0x%x channel=0x%x manu_type=0x%x\n",
	                                interval, channel, manu_type);
	g_task_end = 0;

	while (!g_task_end) {
		for (index = 1; index <= adv_times; index++) {
			if (g_task_end)
				break;

			get_tick_adv_start();
			adv_start(manu_type, index, channel);
			OS_MSleep(delay);
			adv_stop();
			OS_MSleep(gAdvIntMin_ms - delay);
		}
		break;
	}
	CMD_DBG("Advertising end!\n");

exit:
	g_task_end = 1;

	ADV_DELETE_THREAD(g_adv_start_thread);
}


static enum cmd_status adv_start_task(char *arg)
{
	char *cmd = (char *)arg;

	ADV_CREAT_THREAD(g_adv_start_thread, adv_start_exec, cmd);
	return CMD_STATUS_OK;
}

static enum cmd_status adv_stop_task(char *arg)
{
	char *cmd = (char *)arg;
	(void)cmd;

	g_task_end = 1;
	CMD_DBG("adv task end\n");

	return CMD_STATUS_OK;
}

#if 0
static enum cmd_status adv_set_channel_exec(char *cmd)
{
	int cnt;
	uint32_t chl_map;

	cnt = cmd_sscanf(cmd, "c=0x%x", &chl_map);
	if (cnt != 1 || chl_map > 0x07) {
		CMD_ERR("invalid arg\n");
		return CMD_STATUS_FAIL;
	}

	if (g_task_end)
		gAdvChlMap = chl_map;

	return CMD_STATUS_OK;
}

static enum cmd_status adv_set_interval_exec(char *cmd)
{
	int cnt;
	uint32_t adv_min, adv_max;

	cnt = cmd_sscanf(cmd, "min=0x%x max=0x%x", &adv_min, &adv_max);
	if (cnt != 2) {
		CMD_ERR("invalid arg\n");
		return CMD_STATUS_FAIL;
	}

	if (g_task_end) {
		gAdvIntMin = adv_min;
		gAdvIntMax = adv_max;
	}

	return CMD_STATUS_OK;
}

static const struct cmd_data g_adv_set_cmds[] = {
	{ "channel",    adv_set_channel_exec },
	{ "interval",   adv_set_interval_exec },

};

static enum cmd_status cmd_set_exec(char *cmd)
{
	return cmd_exec(cmd, g_adv_set_cmds, cmd_nitems(g_adv_set_cmds));
}
#endif

static const struct cmd_data g_adv_cmds[] = {
#if 0
	{ "set",        cmd_set_exec },
#endif
	{ "start",      adv_start_task },
	{ "stop",       adv_stop_task },
};

enum cmd_status cmd_adv_exec(char *cmd)
{
	return cmd_exec(cmd, g_adv_cmds, cmd_nitems(g_adv_cmds));
}
#endif
