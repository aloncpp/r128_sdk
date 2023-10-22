#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
//#include <net/if.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <wifi_log.h>
#include <wmg_monitor.h>
#include <freertos_monitor.h>
#include <freertos_common.h>
#include "wlan.h"
#include "net_init.h"
#include "net_ctrl.h"
#include "sysinfo.h"
#include "cmd_util.h"

static wmg_monitor_inf_object_t monitor_inf_object;

static wmg_status_t freertos_monitor_mode_init(monitor_data_frame_cb_t monitor_data_frame_cb, void *para)
{
	struct sysinfo *sysinfo = sysinfo_get();
	if (wlan_get_init_status() == WLAN_STATUS_NO_INIT) {
		net_core_init();
		wlan_set_init_status(WLAN_STATUS_INITED);
	}
	if (sysinfo == NULL) {
		WMG_DEBUG("failed to get sysinfo %p\n", sysinfo);
		return WMG_STATUS_FAIL;
	}
	/*freertos no need*/
	return WMG_STATUS_SUCCESS;
}

static wmg_status_t freertos_monitor_mode_enable(void *para)
{
	int ret = 0;
	ret = net_switch_mode(WLAN_MODE_MONITOR);
	if (ret) {
		return WMG_STATUS_FAIL;
	} else {
		return WMG_STATUS_SUCCESS;
	}
}

static wmg_status_t freertos_monitor_mode_disable(void *para)
{
	net_close(g_wlan_netif);
	g_wlan_netif = NULL;
	return 0;
}

static wmg_status_t freertos_monitor_mode_deinit(void *para)
{
	return WMG_STATUS_SUCCESS;
}

static wmg_status_t freertos_monitor_enable()
{
	return WMG_STATUS_SUCCESS;
}

static wmg_status_t freertos_monitor_disable()
{
	return WMG_STATUS_SUCCESS;
}

static wmg_status_t freertos_monitor_connect()
{
	return WMG_STATUS_SUCCESS;
}

static wmg_status_t freertos_monitor_set_channel(uint8_t channel)
{
	return WMG_STATUS_SUCCESS;
}

static wmg_status_t freertos_monitor_disconnect()
{
	return WMG_STATUS_SUCCESS;
}

static wmg_status_t freertos_platform_extension(int cmd, void* cmd_para,int *erro_code)
{
	switch (cmd) {
		case MONITOR_CMD_ENABLE:
			return freertos_monitor_enable();
		case MONITOR_CMD_DISABLE:
			return freertos_monitor_disable();
		case MONITOR_CMD_SET_CHANNL:
			return freertos_monitor_set_channel((uint8_t)(intptr_t)cmd_para);
		default:
			return WMG_STATUS_FAIL;
	}
}

static wmg_monitor_inf_object_t monitor_inf_object = {
	.monitor_init_flag = WMG_FALSE,
	.monitor_enable = WMG_FALSE,
	.monitor_pid = (void *)-1,
	.monitor_state  = NULL,
	.monitor_data_frame_cb = NULL,
	.monitor_channel = 255,

	.monitor_inf_init = freertos_monitor_mode_init,
	.monitor_inf_deinit = freertos_monitor_mode_deinit,
	.monitor_inf_enable = freertos_monitor_mode_enable,
	.monitor_inf_disable = freertos_monitor_mode_disable,
	.monitor_platform_extension = freertos_platform_extension,
};

wmg_monitor_inf_object_t * monitor_rtos_inf_object_register(void)
{
	return &monitor_inf_object;
}
