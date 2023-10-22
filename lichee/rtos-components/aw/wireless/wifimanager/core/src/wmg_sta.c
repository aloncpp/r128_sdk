#include <wifimg.h>
#include <wmg_common.h>
#include <wifi_log.h>
#include <wmg_sta.h>

#ifdef OS_NET_FREERTOS_OS
#include <freertos/freertos_sta.h>
#endif

#ifdef OS_NET_LINUX_OS
#include <linux/linux_sta.h>
#endif

#include <string.h>
#include <unistd.h>

/**
 * wmg_sta_object_t - wifi station private core data
 *
 * @init_flag: indicate wifi station is initialized or not
 * @sta_enable: indicate wifi station is enable or not
 * @sta_state: current state of wifi station
 * @sta_msg_cb: message callback function
 * @sta_msg_cb_arg: message callback parameter
 * @sta_auto_reconn: auto reconnect
 * @platform_inf: platform interface
 */
typedef struct {
	wmg_bool_t init_flag;
	wmg_bool_t sta_enable;
	wifi_sta_state_t sta_state;
	wifi_msg_cb_t sta_msg_cb;
	void *sta_msg_cb_arg;
	wmg_bool_t sta_auto_reconn;
	wmg_sta_inf_object_t* platform_inf[PLATFORM_MAX];
} wmg_sta_object_t;

static wmg_sta_object_t sta_object;

wmg_sta_inf_object_t *__attribute__((weak)) sta_linux_inf_object_register(void)
{
	WMG_DEBUG("wk: sta linux inf\n");
	return NULL;
}

wmg_sta_inf_object_t *__attribute__((weak)) sta_rtos_inf_object_register(void)
{
	WMG_DEBUG("wk: sta rtos inf\n");
	return NULL;
}

wmg_sta_inf_object_t *__attribute__((weak)) sta_xrlink_inf_object_register(void)
{
	WMG_DEBUG("wk: sta xrlink inf\n");
	return NULL;
}

static void sta_state_notify(wifi_sta_state_t state)
{
	wifi_msg_data_t msg;

	if (sta_object.sta_msg_cb) {
		msg.id = WIFI_MSG_ID_STA_STATE_CHANGE;
		msg.data.state = state;
		if(sta_object.sta_msg_cb_arg) {
			msg.private_data = sta_object.sta_msg_cb_arg;
		}
		sta_object.sta_msg_cb(&msg);
	}
}

static void sta_event_notify(wifi_sta_event_t event)
{
	wifi_msg_data_t msg;
	int recall_flag = 1;

	switch (event) {
	case WIFI_CONNECTED:
		sta_object.sta_state = WIFI_STA_CONNECTED;
		break;
	case WIFI_DHCP_START:
		sta_object.sta_state = WIFI_STA_OBTAINING_IP;
		break;
	case WIFI_DHCP_TIMEOUT:
		sta_object.sta_state = WIFI_STA_DHCP_TIMEOUT;
		break;
	case WIFI_DHCP_SUCCESS:
		sta_object.sta_state = WIFI_STA_NET_CONNECTED;
		break;
	case WIFI_TERMINATING:
	case WIFI_DISCONNECTED:
	case WIFI_ASSOC_REJECT:
	case WIFI_NETWORK_NOT_FOUND:
	case WIFI_PASSWORD_INCORRECT:
	case WIFI_CONNECT_TIMEOUT:
		sta_object.sta_state = WIFI_STA_DISCONNECTED;
		break;
	default:
		break;
	}

	if (sta_object.sta_msg_cb && recall_flag) {
		msg.id = WIFI_MSG_ID_STA_CN_EVENT;
		msg.data.event = event;
		if(sta_object.sta_msg_cb_arg) {
			msg.private_data = sta_object.sta_msg_cb_arg;
		}
		sta_object.sta_msg_cb(&msg);
	}
}

static wmg_status_t sta_connect(void *param,void *cb_msg)
{
	if (sta_object.sta_state == WIFI_STA_CONNECTING ||
		sta_object.sta_state == WIFI_STA_OBTAINING_IP) {
		WMG_ERROR("dev busy\n");
		return WMG_STATUS_UNHANDLED;
	}

	sta_object.sta_state = WIFI_STA_CONNECTING;
	return sta_object.platform_inf[PLATFORM]->sta_platform_extension(STA_CMD_CONNECT, param, NULL);
}

static wmg_status_t sta_disconnect(void *param,void *cb_msg)
{
	if(sta_object.sta_state != WIFI_STA_DISCONNECTED) {
		sta_object.sta_state = WIFI_STA_DISCONNECTING;
		return sta_object.platform_inf[PLATFORM]->sta_platform_extension(STA_CMD_DISCONNECT, NULL, NULL);
	}

	WMG_DEBUG("sta already disconnect\n");
	return WMG_STATUS_UNHANDLED;
}

static wmg_status_t sta_auto_reconnect(void *param,void *cb_msg)
{
	wmg_bool_t *enable = param;
	sta_object.sta_auto_reconn = *enable;
	return sta_object.platform_inf[PLATFORM]->sta_platform_extension(STA_CMD_SET_AUTO_RECONN, param, NULL);
}

static wmg_status_t sta_get_info(void *param,void *cb_msg)
{
	if (sta_object.sta_state < WIFI_STA_CONNECTED ||
		sta_object.sta_state > WIFI_STA_NET_CONNECTED) {
		WMG_ERROR("sta already disconnect\n");
		return WMG_STATUS_FAIL;
	}

	return sta_object.platform_inf[PLATFORM]->sta_platform_extension(STA_CMD_GET_INFO, param, NULL);
}

static wmg_status_t sta_list_networks(void *param,void *cb_msg)
{
	return sta_object.platform_inf[PLATFORM]->sta_platform_extension(STA_CMD_LIST_NETWORKS, param, NULL);
}

static wmg_status_t sta_remove_networks(void *param,void *cb_msg)
{
	return sta_object.platform_inf[PLATFORM]->sta_platform_extension(STA_CMD_REMOVE_NETWORKS, param, NULL);
}

static wmg_status_t sta_set_scan_param(void *param,void *cb_msg)
{
	return WMG_STATUS_FAIL;
}

static wmg_status_t sta_get_scan_results(void *param,void *cb_msg)
{
	if (sta_object.sta_state > WIFI_STA_CONNECTED &&
		sta_object.sta_state < WIFI_STA_NET_CONNECTED) {
		WMG_WARNG("dev busy(state:%d)\n",sta_object.sta_state);
		return WMG_STATUS_UNHANDLED;
	}

	return sta_object.platform_inf[PLATFORM]->sta_platform_extension(STA_CMD_GET_SCAN_RESULTS, param, NULL);
}

static wmg_status_t sta_register_msg_cb(void *param,void *cb_msg)
{
	common_msg_cb_t *msg_cb_attr = (common_msg_cb_t *)param;

	sta_object.sta_msg_cb = *msg_cb_attr->msg_cb;
	sta_object.sta_msg_cb_arg = msg_cb_attr->msg_cb_arg;

	return WMG_STATUS_SUCCESS;
}

static wmg_status_t sta_get_state(void *param,void *cb_msg)
{
	wifi_wmg_state_t *wmg_sta_state = (wifi_wmg_state_t *)param;

	wmg_sta_state->sta_state = sta_object.sta_state;

	return WMG_STATUS_SUCCESS;
}

#ifdef OS_NET_XRLINK_OS
static wmg_status_t sta_vendor_send_data(void *param,void *cb_msg)
{
	return sta_object.platform_inf[PLATFORM]->sta_platform_extension(STA_CMD_VENDOR_SEND_DATA, param, NULL);
}

static wmg_status_t sta_vendor_register_rx_cb(void *param,void *cb_msg)
{
	if(sta_object.init_flag == WMG_FALSE) {
		WMG_ERROR("sta has not been init\n");
		return WMG_STATUS_FAIL;
	}

	return sta_object.platform_inf[PLATFORM]->sta_platform_extension(STA_CMD_VENDOR_REGISTER_RX_CB, param, NULL);
}
#endif

static wmg_status_t sta_mode_init(void)
{
	if(sta_object.init_flag == WMG_FALSE) {
		WMG_DEBUG("sta mode init now\n");
		sta_object.platform_inf[PLATFORM_LINUX] = sta_linux_inf_object_register();
		sta_object.platform_inf[PLATFORM_RTOS] = sta_rtos_inf_object_register();
		sta_object.platform_inf[PLATFORM_XRLINK] = sta_xrlink_inf_object_register();

		if(NULL != sta_object.platform_inf[PLATFORM]) {
			if(sta_object.platform_inf[PLATFORM]->sta_inf_init != NULL){
				if(sta_object.platform_inf[PLATFORM]->sta_inf_init(sta_event_notify,NULL)){
					return WMG_STATUS_FAIL;
				}
			}
		}
		sta_object.init_flag = WMG_TRUE;
	} else {
		WMG_DEBUG("sta mode already init\n");
	}
	return WMG_STATUS_SUCCESS;
}

static wmg_status_t sta_mode_deinit(void)
{
	if(sta_object.init_flag == WMG_TRUE) {
		WMG_DEBUG("sta mode deinit now\n");
		if(sta_object.platform_inf[PLATFORM]->sta_inf_deinit != NULL){
			sta_object.platform_inf[PLATFORM]->sta_inf_deinit(NULL);
		}
		sta_object.init_flag = WMG_FALSE;
		sta_object.sta_state = WIFI_STA_IDLE;
		sta_object.sta_msg_cb = NULL;
		sta_object.sta_msg_cb_arg = NULL;
		sta_object.sta_auto_reconn = WMG_FALSE;
	} else {
		WMG_DEBUG("sta mode already deinit\n");
	}
	return WMG_STATUS_SUCCESS;
}

static wmg_status_t sta_mode_enable(int* erro_code)
{
	wmg_status_t ret;

	if(sta_object.init_flag == WMG_FALSE) {
		WMG_ERROR("sta has not been init\n");
		return WMG_STATUS_FAIL;
	}
	if (sta_object.sta_enable == WMG_TRUE) {
		WMG_DEBUG("stat already enable\n");
		return WMG_STATUS_SUCCESS;
	}

	WMG_DEBUG("sta mode enabling...\n");

	ret = sta_object.platform_inf[PLATFORM]->sta_inf_enable(NULL);
	if (ret) {
		WMG_ERROR("failed to enable sta mode\n");
		return WMG_STATUS_FAIL;
	}
	sta_object.sta_enable = WMG_TRUE;

	WMG_DEBUG("sta mode enable success\n");
	return ret;
}

static wmg_status_t sta_mode_disable(int* erro_code)
{
	wmg_status_t ret;

	if(sta_object.init_flag == WMG_FALSE) {
		WMG_ERROR("sta mode has not been init\n");
		return WMG_STATUS_SUCCESS;
	}
	if (sta_object.sta_enable == WMG_FALSE) {
		WMG_DEBUG("sta mode already disable\n");
		return WMG_STATUS_SUCCESS;
	}

	WMG_DEBUG("sta mode disabling...\n");

	sta_object.platform_inf[PLATFORM]->sta_inf_disable(NULL);
	sta_object.sta_enable = WMG_FALSE;

	WMG_DEBUG("sta mode disable success\n");

	return WMG_STATUS_SUCCESS;
}


static wmg_status_t sta_mode_ctl(int cmd, void *param, void *cb_msg)
{
	if(sta_object.init_flag == WMG_FALSE) {
		WMG_ERROR("sta has not been init\n");
		return WMG_STATUS_FAIL;
	}
	if (sta_object.sta_enable == WMG_FALSE) {
		WMG_ERROR("sta mode already disabled\n");
		return WMG_STATUS_FAIL;
	}

	WMG_DEBUG("= sta_mode_ctl cmd: %d =\n", cmd);

	switch (cmd) {
		case WMG_STA_CMD_CONNECT:
			return sta_connect(param,cb_msg);
		case WMG_STA_CMD_DISCONNECT:
			return sta_disconnect(param,cb_msg);
		case WMG_STA_CMD_AUTO_RECONNECT:
			return sta_auto_reconnect(param,cb_msg);
		case WMG_STA_CMD_GET_INFO:
			return sta_get_info(param,cb_msg);
		case WMG_STA_CMD_LIST_NETWORKS:
			return sta_list_networks(param,cb_msg);
		case WMG_STA_CMD_REMOVE_NETWORKS:
			return sta_remove_networks(param,cb_msg);
		case WMG_STA_CMD_SCAN_PARAM:
			return sta_set_scan_param(param,cb_msg);
		case WMG_STA_CMD_SCAN_RESULTS:
			return sta_get_scan_results(param,cb_msg);
		case WMG_STA_CMD_REGISTER_MSG_CB:
			return sta_register_msg_cb(param,cb_msg);
		case WMG_STA_CMD_GET_STATE:
			return sta_get_state(param,cb_msg);
#ifdef OS_NET_XRLINK_OS
		case WMG_STA_CMD_VENDOR_SEND_DATA:
			return sta_vendor_send_data(param,cb_msg);
		case WMG_STA_CMD_VENDOR_REGISTER_RX_CB:
			return sta_vendor_register_rx_cb(param,cb_msg);
#endif
		default:
		return WMG_STATUS_FAIL;
	}
}

static wmg_sta_object_t sta_object = {
	.init_flag = WMG_FALSE,
	.sta_enable = WMG_FALSE,
	.sta_state = WIFI_STA_IDLE,
	.sta_msg_cb = NULL,
	.sta_msg_cb_arg = NULL,
	.sta_auto_reconn = WMG_FALSE,
};

static mode_opt_t sta_mode_opt = {
	.mode_enable = sta_mode_enable,
	.mode_disable = sta_mode_disable,
	.mode_ctl = sta_mode_ctl,
};

static mode_object_t sta_mode_object = {
	.mode_name = "sta",
	.init = sta_mode_init,
	.deinit = sta_mode_deinit,
	.mode_opt = &sta_mode_opt,
	.private_data = &sta_object,
};

mode_object_t* wmg_sta_register_object(void)
{
	return &sta_mode_object;
}
