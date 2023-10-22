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

#include "prj_config.h"

#include "net_ctrl.h"
#include "sys_monitor.h"
#include "sys_monitor_debug.h"
#include "net/wlan/wlan_ext_req.h"

#if PRJCONF_TEMP_MONITOR_EN
#define TEMPERATURE_HIGH_THRESH        150 //celsius degree
#define TEMP_VOL_INDICATE_PERIOD       5
#define HIBERNATE_WAKEUP_PERIOD        10 //unit: second

extern int cmd_reboot(int argc, char ** argv);
void temp_protect_task(void *dev)
{
	SYS_MONITOR_ERR("Temperature thresh high overflow!!!\n");

	/* TODO: use pm API set pm hib wakeup time and enter hibernation */
	//pm_enter_hibernation(0, NULL);

	/* TODO: get hibernation result,if failed to enter hibernation,
	 * reboot system. */
	cmd_reboot(0, NULL);
	XR_OS_ThreadDelete(NULL);
}

void temp_monitor_cb(wlan_ext_temp_volt_event_data_t *temp_volt_data)
{
	SYS_MONITOR_LOG(1, "************** temp_volt_data **************\n");
	SYS_MONITOR_LOG(1, "ind_flags:0x%04X\n", temp_volt_data->ind_flags);
	SYS_MONITOR_LOG(1, "tmp_now:%.02f°C\n", (float)temp_volt_data->tmp_now / 16);
	SYS_MONITOR_LOG(1, "tmp_max:%.02f°C\n", (float)temp_volt_data->tmp_max / 16);
	SYS_MONITOR_LOG(1, "tmp_min:%.02f°C\n", (float)temp_volt_data->tmp_min / 16);
	SYS_MONITOR_LOG(1, "volt_now:%.02fV\n", (float)temp_volt_data->volt_now / 16);
	SYS_MONITOR_LOG(1, "volt_max:%.02fV\n", (float)temp_volt_data->volt_max / 16);
	SYS_MONITOR_LOG(1, "volt_min:%.02fV\n", (float)temp_volt_data->volt_min / 16);
	SYS_MONITOR_LOG(1, "********************************************\n");
	if (temp_volt_data->ind_flags & WLAN_EXT_TEMP_THRESH_HIGH_OVERFLOW) {
		XR_OS_Thread_t thread;
		XR_OS_ThreadSetInvalid(&thread);
		if (XR_OS_ThreadCreate(&thread,
		                    "temp_protect",
		                    temp_protect_task,
		                    NULL,
		                    XR_OS_THREAD_PRIO_APP,
		                    4 * 1024) != XR_OS_OK) {
			SYS_MONITOR_ERR("create temp_protect_task failed\n");
		}
		return;
	}
}
#endif

void temp_monitor_init(void)
{
#if PRJCONF_TEMP_MONITOR_EN
	wlan_ext_temp_volt_thresh_set_t param;
	memset(&param, 0, sizeof(wlan_ext_temp_volt_thresh_set_t));
	param.TempHighEn = 1;
	param.TempHighThresh = TEMPERATURE_HIGH_THRESH * 16;
	param.TempVoltIndPeriod = TEMP_VOL_INDICATE_PERIOD;
	wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_TEMP_VOLT_THRESH, (uint32_t)(uintptr_t)(&param));
	wlan_ext_set_temp_volt_event_cb(temp_monitor_cb);
	SYS_MONITOR_DBG("TempHighEn:%d\n", param.TempHighEn);
	SYS_MONITOR_DBG("TempHighThresh:%.02f°C\n", (float)param.TempHighThresh / 16);
	SYS_MONITOR_DBG("TempVoltIndPeriod:%d\n", param.TempVoltIndPeriod);
#endif
}

