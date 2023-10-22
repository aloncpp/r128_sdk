#include <wmg_monitor.h>
#include <wifi_log.h>
#include <event.h>
#ifdef OS_NET_FREERTOS_OS
#include <freertos_monitor.h>
#endif

#ifdef OS_NET_LINUX_OS
#include <linux_monitor.h>
#endif

#include <string.h>

/*
 * wmg_monitor_object_t - wifi monitor private core data
 *
 * @init_flag: indicate wifi monitor is initialized or not
 * @monitor_enable: indicate wifi monitor is enable or not
 * @monitor_state: current state of wifi monitor
 * @monitor_msg_cb: message callback function
 * @monitor_msg_cb_arg: message callback parameter
 * @platform_inf: platform interface
 */
typedef struct {
	wmg_bool_t init_flag;
	wmg_bool_t monitor_enable;
	wifi_monitor_state_t monitor_state;
	wifi_msg_cb_t monitor_msg_cb;
	void *monitor_msg_cb_arg;
	wmg_monitor_inf_object_t* platform_inf[PLATFORM_MAX];
} wmg_monitor_object_t;

static wmg_monitor_object_t monitor_object;

wmg_monitor_inf_object_t *__attribute__((weak)) monitor_linux_inf_object_register(void)
{
	WMG_DEBUG("wk: mon linux inf\n");
	return NULL;
}

wmg_monitor_inf_object_t *__attribute__((weak)) monitor_rtos_inf_object_register(void)
{
	WMG_DEBUG("wk: mon rtos inf\n");
	return NULL;
}

wmg_monitor_inf_object_t *__attribute__((weak)) monitor_xrlink_inf_object_register(void)
{
	WMG_DEBUG("wk: mon xrlink inf\n");
	return NULL;
}

static void monitor_nl_data_notify(wifi_monitor_data_t *frame)
{
	wifi_msg_data_t msg;

	if (monitor_object.monitor_msg_cb) {
		msg.id = WIFI_MSG_ID_MONITOR;
		msg.data.frame = frame;
		if(monitor_object.monitor_msg_cb_arg) {
			msg.private_data = monitor_object.monitor_msg_cb_arg;
		}
		monitor_object.monitor_msg_cb(&msg);
	}
}

static wmg_status_t monitor_enable(void *param, void *cb_msg)
{
	wmg_status_t ret;
	wifi_msg_data_t msg;

	if (monitor_object.monitor_state == WIFI_MONITOR_ENABLE) {
		WMG_WARNG("mon is already enable\n");
		return WMG_STATUS_UNHANDLED;
	}

	WMG_DEBUG("mon enabling...\n");

	ret = monitor_object.platform_inf[PLATFORM]->monitor_platform_extension(MONITOR_CMD_ENABLE, param, NULL);
	if (ret) {
		WMG_ERROR("mon enable faile\n");
		return WMG_STATUS_FAIL;
	}
	monitor_object.monitor_state = WIFI_MONITOR_ENABLE;
	if (monitor_object.monitor_msg_cb) {
		msg.id = WIFI_MSG_ID_MONITOR;
		msg.data.mon_state = WIFI_MONITOR_ENABLE;
		if(monitor_object.monitor_msg_cb_arg) {
			msg.private_data = monitor_object.monitor_msg_cb_arg;
		}
		monitor_object.monitor_msg_cb(&msg);
	}

	return ret;
}

static wmg_status_t monitor_set_channel(void *param, void *cb_msg)
{
	if (monitor_object.monitor_state != WIFI_MONITOR_ENABLE) {
		WMG_WARNG("mon is not enabled\n");
		return WMG_STATUS_FAIL;
	}

	return monitor_object.platform_inf[PLATFORM]->monitor_platform_extension(MONITOR_CMD_SET_CHANNL, param, NULL);
}

static wmg_status_t monitor_disable(void *param, void *cb_msg)
{
	wmg_status_t ret;
	wifi_msg_data_t msg;

	if (monitor_object.monitor_state == WIFI_MONITOR_DISABLE) {
		WMG_WARNG("mon already disabled\n");
		return WMG_STATUS_UNHANDLED;
	}

	WMG_DEBUG("mon disabling...\n");

	ret = monitor_object.platform_inf[PLATFORM]->monitor_platform_extension(MONITOR_CMD_DISABLE, NULL, NULL);

	monitor_object.monitor_state = WIFI_MONITOR_DISABLE;
	if (monitor_object.monitor_msg_cb) {
		msg.id = WIFI_MSG_ID_MONITOR;
		msg.data.mon_state = WIFI_MONITOR_DISABLE;
		if(monitor_object.monitor_msg_cb_arg) {
			msg.private_data = monitor_object.monitor_msg_cb_arg;
		}
		monitor_object.monitor_msg_cb(&msg);
	}

	return ret;
}

static wmg_status_t monitor_register_msg_cb(void *param, void *cb_msg)
{
	common_msg_cb_t *msg_cb_attr = (common_msg_cb_t *)param;

	monitor_object.monitor_msg_cb = *msg_cb_attr->msg_cb;
	monitor_object.monitor_msg_cb_arg = msg_cb_attr->msg_cb_arg;

	return WMG_STATUS_SUCCESS;
}

static wmg_status_t monitor_get_state(void *param,void **cb_msg)
{
	wifi_wmg_state_t *wmg_monitor_state = (wifi_wmg_state_t *)param;

	wmg_monitor_state->monitor_state = monitor_object.monitor_state;

	return WMG_STATUS_SUCCESS;
}

static wmg_status_t monitor_vendor_send_data(void *param,void *cb_msg)
{
	return monitor_object.platform_inf[PLATFORM]->monitor_platform_extension(MONITOR_CMD_VENDOR_SEND_DATA, param, NULL);
}

static wmg_status_t monitor_vendor_register_rx_cb(void *param,void **cb_msg)
{
	return monitor_object.platform_inf[PLATFORM]->monitor_platform_extension(MONITOR_CMD_VENDOR_REGISTER_RX_CB, param, NULL);
}

static wmg_status_t monitor_mode_init(void)
{
	if (monitor_object.init_flag == WMG_FALSE) {
		WMG_DEBUG("mon mode init now\n");
		monitor_object.platform_inf[PLATFORM_LINUX] = monitor_linux_inf_object_register();
		monitor_object.platform_inf[PLATFORM_RTOS] = monitor_rtos_inf_object_register();
		monitor_object.platform_inf[PLATFORM_XRLINK] = monitor_xrlink_inf_object_register();

		if(NULL != monitor_object.platform_inf[PLATFORM]){
			if(monitor_object.platform_inf[PLATFORM]->monitor_inf_init != NULL){
				if(monitor_object.platform_inf[PLATFORM]->monitor_inf_init(monitor_nl_data_notify,NULL)){
					return WMG_STATUS_FAIL;
				}
			}
		}
		monitor_object.init_flag = WMG_TRUE;
	} else {
		WMG_DEBUG("mon mode already init\n");
	}
	return WMG_STATUS_SUCCESS;
}

static wmg_status_t monitor_mode_deinit(void)
{
	if (monitor_object.init_flag == WMG_TRUE) {
		WMG_DEBUG("mon mode deinit now\n");
		if(monitor_object.platform_inf[PLATFORM]->monitor_inf_deinit != NULL){
			monitor_object.platform_inf[PLATFORM]->monitor_inf_deinit(NULL);
		}
		monitor_object.init_flag = WMG_FALSE;
		monitor_object.monitor_enable = WMG_FALSE;
		monitor_object.monitor_state = WIFI_MONITOR_DISABLE;
		monitor_object.monitor_msg_cb = NULL;
		monitor_object.monitor_msg_cb_arg = NULL;
	} else {
		WMG_DEBUG("mon mode already deinit\n");
	}
	return WMG_STATUS_SUCCESS;
}

static wmg_status_t monitor_mode_enable(int* erro_code)
{
	wmg_status_t ret;

	if(monitor_object.init_flag == WMG_FALSE) {
		WMG_ERROR("mon mode has not been init\n");
		return WMG_STATUS_FAIL;
	}
	if (monitor_object.monitor_enable == WMG_TRUE) {
		WMG_WARNG("mon mode already enabled\n");
		return WMG_STATUS_SUCCESS;
	}

	WMG_DEBUG("mon monde enabling...\n");

	ret = monitor_object.platform_inf[PLATFORM]->monitor_inf_enable(NULL);
	if (ret) {
		WMG_ERROR("mon mode enable faile\n");
		return WMG_STATUS_FAIL;
	}
	monitor_object.monitor_enable = WMG_TRUE;

	WMG_DEBUG("mon mode enable success\n");
	return ret;
}

static wmg_status_t monitor_mode_disable(int* erro_code)
{
	wmg_status_t ret;

	if(monitor_object.init_flag == WMG_FALSE) {
		WMG_WARNG("mon has not been init\n");
		return WMG_STATUS_UNHANDLED;
	}
	if (monitor_object.monitor_enable == WMG_FALSE) {
		WMG_WARNG("mon already disabled\n");
		return WMG_STATUS_SUCCESS;
	}

	WMG_DEBUG("mon mode disabling...\n");

	monitor_object.platform_inf[PLATFORM]->monitor_inf_disable(NULL);
	monitor_object.monitor_enable = WMG_FALSE;

	WMG_DEBUG("mon mode disabled\n");

	return WMG_STATUS_SUCCESS;
}

static wmg_status_t monitor_mode_ctl(int cmd, void *param, void *cb_msg)
{
	if(monitor_object.init_flag == WMG_FALSE) {
		WMG_ERROR("mon has not been init\n");
		return WMG_STATUS_FAIL;
	}
	if(monitor_object.monitor_enable == WMG_FALSE) {
		WMG_ERROR("mon mode already disabled\n");
		return WMG_STATUS_FAIL;
	}

	switch (cmd) {
		case WMG_MONITOR_CMD_ENABLE:
			return monitor_enable(param,cb_msg);
		case WMG_MONITOR_CMD_SET_CHANNEL:
			return monitor_set_channel(param,cb_msg);
		case WMG_MONITOR_CMD_DISABLE:
			return monitor_disable(param,cb_msg);
		case WMG_MONITOR_CMD_REGISTER_MSG_CB:
			return monitor_register_msg_cb(param,cb_msg);
		case WMG_MONITOR_CMD_GET_STATE:
			return monitor_get_state(param,cb_msg);
		case WMG_MONITOR_CMD_VENDOR_SEND_DATA:
			return monitor_vendor_send_data(param,cb_msg);
		case WMG_MONITOR_CMD_VENDOR_REGISTER_RX_CB:
			return monitor_vendor_register_rx_cb(param,cb_msg);
		default:
			return WMG_STATUS_FAIL;
	}
}

static wmg_monitor_object_t monitor_object = {
	.init_flag = WMG_FALSE,
	.monitor_enable = WMG_FALSE,
	.monitor_state = WIFI_MONITOR_DISABLE,
	.monitor_msg_cb = NULL,
	.monitor_msg_cb_arg = NULL,
};

static mode_opt_t monitor_mode_opt = {
	.mode_enable = monitor_mode_enable,
	.mode_disable = monitor_mode_disable,
	.mode_ctl = monitor_mode_ctl,
};

static mode_object_t monitor_mode_object = {
	.mode_name = "monitor",
	.init = monitor_mode_init,
	.deinit = monitor_mode_deinit,
	.mode_opt = &monitor_mode_opt,
	.private_data = &monitor_object,
};

mode_object_t* wmg_monitor_register_object(void)
{
	return &monitor_mode_object;
}
