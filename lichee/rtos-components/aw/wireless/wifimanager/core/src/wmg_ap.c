#include <wmg_ap.h>
#include <wifi_log.h>
#include <event.h>

#ifdef OS_NET_FREERTOS_OS
#include <freertos_ap.h>
#endif

#ifdef OS_NET_LINUX_OS
#include <linux_ap.h>
#endif

#include <string.h>

/*
 * wmg_ap_object_t - wifi ap private core data
 *
 * @init_flag: indicate wifi ap is initialized or not
 * @ap_enable: indicate wifi ap is enable or not
 * @ap_state: current state of wifi ap
 * @ap_msg_cb: message callback function
 * @ap_msg_cb_arg: message callback parameter
 * @ap_config: ap config
 * @platform_inf: platform interface
 */
typedef struct {
	wmg_bool_t init_flag;
	wmg_bool_t ap_enable;
	wifi_ap_state_t ap_state;
	wifi_msg_cb_t ap_msg_cb;
	void *ap_msg_cb_arg;
	wifi_ap_config_t ap_config;
	wmg_ap_inf_object_t* platform_inf[PLATFORM_MAX];
} wmg_ap_object_t;

static wmg_ap_object_t ap_object;

char ap_config_ssid[SSID_MAX_LEN] = {0};
char ap_config_psk[PSK_MAX_LEN] = {0};

static wifi_ap_config_t ap_config = {
	.ssid = ap_config_ssid,
	.psk = ap_config_psk,
	.sec = WIFI_SEC_WPA2_PSK,
	.channel = 6,
	.sta_num = 0,
};

wmg_ap_inf_object_t *__attribute__((weak)) ap_linux_inf_object_register(void)
{
	WMG_DEBUG("wk: ap linux inf\n");
	return NULL;
}

wmg_ap_inf_object_t *__attribute__((weak)) ap_rtos_inf_object_register(void)
{
	WMG_DEBUG("wk: ap rtos inf\n");
	return NULL;
}

wmg_ap_inf_object_t *__attribute__((weak)) ap_xrlink_inf_object_register(void)
{
	WMG_DEBUG("wk: ap xrlink\n");
	return NULL;
}

static void ap_event_notify(wifi_ap_event_t event)
{
	wifi_msg_data_t msg;

	if (ap_object.ap_msg_cb) {
		msg.id = WIFI_MSG_ID_AP_CN_EVENT;
		msg.data.ap_event = event;
		if(ap_object.ap_msg_cb_arg) {
			msg.private_data = ap_object.ap_msg_cb_arg;
		}
		ap_object.ap_msg_cb(&msg);
	}
}

static wmg_status_t ap_enable(void *param, void *cb_msg)
{
	wmg_status_t ret;

	ret = ap_object.platform_inf[PLATFORM]->ap_platform_extension(AP_CMD_ENABLE, (void *)param, NULL);
	if (ret) {
		WMG_ERROR("ap enable faile\n");
		return WMG_STATUS_FAIL;
	}
	ap_object.ap_state = WIFI_AP_ENABLE;

	return ret;
}

static wmg_status_t ap_disable(void *param, void *cb_msg)
{
	wmg_status_t ret;

	if (ap_object.ap_state == WIFI_AP_DISABLE) {
		WMG_DEBUG("ap already disable\n");
		return WMG_STATUS_UNHANDLED;
	}

	ret = ap_object.platform_inf[PLATFORM]->ap_platform_extension(AP_CMD_DISABLE, NULL, NULL);
	if (ret) {
		WMG_ERROR("ap disable faile\n");
		return WMG_STATUS_FAIL;
	}

	ap_object.ap_state = WIFI_AP_DISABLE;

	return ret;
}

static wmg_status_t ap_get_config(void *param, void *cb_msg)
{
	if(ap_object.ap_state != WIFI_AP_ENABLE) {
		WMG_WARNG("wifi ap has not been enable now\n");
		return WMG_STATUS_FAIL;
	}

	return ap_object.platform_inf[PLATFORM]->ap_platform_extension(AP_CMD_GET_CONFIG, (void *)param, NULL);
}

static wmg_status_t ap_set_scan_param(void *param,void *cb_msg)
{
	return WMG_STATUS_FAIL;
}

static wmg_status_t ap_get_scan_results(void *param,void *cb_msg)
{
	return ap_object.platform_inf[PLATFORM]->ap_platform_extension(AP_CMD_GET_SCAN_RESULTS, param, NULL);
}

static wmg_status_t ap_register_msg_cb(void *param, void *cb_msg)
{
	common_msg_cb_t *msg_cb_attr = (common_msg_cb_t *)param;

	ap_object.ap_msg_cb = *msg_cb_attr->msg_cb;
	ap_object.ap_msg_cb_arg = msg_cb_attr->msg_cb_arg;

	return WMG_STATUS_SUCCESS;
}

static wmg_status_t ap_get_state(void *param,void *cb_msg)
{
	wifi_wmg_state_t *wmg_ap_state = (wifi_wmg_state_t *)param;

	wmg_ap_state->ap_state = ap_object.ap_state;

	return WMG_STATUS_SUCCESS;
}

static wmg_status_t ap_vendor_send_data(void *param,void *cb_msg)
{
	return ap_object.platform_inf[PLATFORM]->ap_platform_extension(AP_CMD_VENDOR_SEND_DATA, param, NULL);
}

static wmg_status_t ap_vendor_register_rx_cb(void *param,void *cb_msg)
{
	return ap_object.platform_inf[PLATFORM]->ap_platform_extension(AP_CMD_VENDOR_REGISTER_RX_CB, param, NULL);
}

static wmg_status_t ap_mode_init(void)
{
	if (ap_object.init_flag == WMG_FALSE) {
		WMG_DEBUG("ap mode init now\n");
		ap_object.platform_inf[PLATFORM_LINUX] = ap_linux_inf_object_register();
		ap_object.platform_inf[PLATFORM_RTOS] = ap_rtos_inf_object_register();
		ap_object.platform_inf[PLATFORM_XRLINK] = ap_xrlink_inf_object_register();

		if(NULL != ap_object.platform_inf[PLATFORM]) {
			if(ap_object.platform_inf[PLATFORM]->ap_inf_init != NULL){
				if(ap_object.platform_inf[PLATFORM]->ap_inf_init(ap_event_notify)){
					return WMG_STATUS_FAIL;
				}
			}
		}
		ap_object.init_flag = WMG_TRUE;
	} else {
		WMG_DEBUG("ap mode already init\n");
	}
	return WMG_STATUS_SUCCESS;
}

static wmg_status_t ap_mode_deinit(void)
{
	if (ap_object.init_flag == WMG_TRUE) {
		WMG_DEBUG("ap mode deinit now\n");
		if(ap_object.platform_inf[PLATFORM]->ap_inf_deinit != NULL){
			ap_object.platform_inf[PLATFORM]->ap_inf_deinit(NULL);
		}
		ap_object.init_flag = WMG_FALSE;
		ap_object.ap_msg_cb = NULL;
		ap_object.ap_msg_cb_arg = NULL;
	} else {
		WMG_DEBUG("ap mode already deinit\n");
	}
	return WMG_STATUS_SUCCESS;
}

static wmg_status_t ap_mode_enable(int* erro_code)
{
	wmg_status_t ret;

	if(ap_object.init_flag == WMG_FALSE) {
		WMG_ERROR("ap has not been init\n");
		return WMG_STATUS_FAIL;
	}
	if (ap_object.ap_enable == WMG_TRUE) {
		WMG_DEBUG("ap mode already enable\n");
		return WMG_STATUS_SUCCESS;
	}

	WMG_DEBUG("ap mode enabling...\n");

	ret = ap_object.platform_inf[PLATFORM]->ap_inf_enable(NULL);
	if (ret) {
		WMG_ERROR("ap mode inf enable failed\n");
		return WMG_STATUS_FAIL;
	}
	ap_object.ap_state = WIFI_AP_ENABLE;
	WMG_DEBUG("ap mode success\n");

	ap_object.ap_enable = WMG_TRUE;

	WMG_DUMP("ap mode enable success\n");
	return ret;
}

static wmg_status_t ap_mode_disable(int* erro_code)
{
	wmg_status_t ret;

	if(ap_object.init_flag == WMG_FALSE) {
		WMG_WARNG("ap mode has not been init\n");
		return WMG_STATUS_UNHANDLED;
	}
	if (ap_object.ap_enable == WMG_FALSE) {
		WMG_WARNG("ap mode already disabled\n");
		return WMG_STATUS_UNHANDLED;
	}

	WMG_DEBUG("ap mode disabling...\n");

	ap_object.platform_inf[PLATFORM]->ap_inf_disable(NULL);
	ap_object.ap_enable = WMG_FALSE;

	WMG_DEBUG("ap mode disabled\n");

	return WMG_STATUS_SUCCESS;
}

static wmg_status_t ap_mode_ctl(int cmd, void *param, void *cb_msg)
{
	if(ap_object.init_flag == WMG_FALSE) {
		WMG_ERROR("ap has not been init\n");
		return WMG_STATUS_FAIL;
	}
	if(ap_object.ap_enable == WMG_FALSE) {
		WMG_ERROR("ap mode already disable\n");
		return WMG_STATUS_FAIL;
	}

	WMG_DEBUG("= ap_mode_ctl cmd: %d =\n", cmd);

	switch (cmd) {
		case WMG_AP_CMD_ENABLE:
			return ap_enable(param,cb_msg);
		case WMG_AP_CMD_DISABLE:
			return ap_disable(param,cb_msg);
		case WMG_AP_CMD_GET_CONFIG:
			return ap_get_config(param,cb_msg);
		case WMG_AP_CMD_SCAN_PARAM:
			return ap_set_scan_param(param,cb_msg);
		case WMG_AP_CMD_SCAN_RESULTS:
			return ap_get_scan_results(param,cb_msg);
		case WMG_AP_CMD_REGISTER_MSG_CB:
			return ap_register_msg_cb(param,cb_msg);
		case WMG_AP_CMD_GET_STATE:
			return ap_get_state(param,cb_msg);
		case WMG_AP_CMD_VENDOR_SEND_DATA:
			return ap_vendor_send_data(param,cb_msg);
		case WMG_AP_CMD_VENDOR_REGISTER_RX_CB:
			return ap_vendor_register_rx_cb(param,cb_msg);
		default:
			return WMG_STATUS_FAIL;
	}
}

static wmg_ap_object_t ap_object = {
	.init_flag = WMG_FALSE,
	.ap_enable = WMG_FALSE,
	.ap_state = WIFI_AP_DISABLE,
	.ap_msg_cb = NULL,
	.ap_msg_cb_arg = NULL,
};

static mode_opt_t ap_mode_opt = {
	.mode_enable = ap_mode_enable,
	.mode_disable = ap_mode_disable,
	.mode_ctl = ap_mode_ctl,
};

static mode_object_t ap_mode_object = {
	.mode_name = "ap",
	.init = ap_mode_init,
	.deinit = ap_mode_deinit,
	.mode_opt = &ap_mode_opt,
	.private_data = &ap_object,
};

mode_object_t* wmg_ap_register_object(void)
{
	return &ap_mode_object;
}
