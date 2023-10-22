#include <wmg_p2p.h>
#include <wifi_log.h>

#ifdef OS_NET_FREERTOS_OS
#include <freertos_p2p.h>
#endif

#ifdef OS_NET_LINUX_OS
#include <linux_p2p.h>
#endif

#include <string.h>

/*
 * wmg_p2p_object_t - wifi p2p private core data
 *
 * @init_flag: indicate wifi p2p is initialized or not
 * @p2p_enable: indicate wifi p2p is enable or not
 * @p2p_state: current state of wifi ap
 * @p2p_msg_cb: message callback function
 * @p2p_msg_cb_arg: message callback parameter
 * @p2p_config: p2p config
 * @platform_inf: platform interface
 */
typedef struct {
	wmg_bool_t init_flag;
	wmg_bool_t p2p_enable;
	wifi_p2p_state_t p2p_state;
	wifi_msg_cb_t p2p_msg_cb;
	void *p2p_msg_cb_arg;
	wifi_p2p_config_t p2p_config;
	wmg_p2p_inf_object_t* platform_inf[PLATFORM_MAX];
} wmg_p2p_object_t;

static wmg_p2p_object_t p2p_object;

char p2p_config_dev_name[SSID_MAX_LEN] = {0};
char p2p_config_psk[PSK_MAX_LEN] = {0};

static wifi_p2p_config_t p2p_config = {
	.dev_name = p2p_config_dev_name,
	.listen_time = 10,
};

wmg_p2p_inf_object_t *__attribute__((weak)) p2p_linux_inf_object_register(void)
{
	WMG_DEBUG("wk: p2p linux inf\n");
	return NULL;
}

wmg_p2p_inf_object_t *__attribute__((weak)) p2p_rtos_inf_object_register(void)
{
	WMG_DEBUG("wk: p2p rtos inf\n");
	return NULL;
}

wmg_p2p_inf_object_t *__attribute__((weak)) p2p_xrlink_inf_object_register(void)
{
	WMG_DEBUG("wk: p2p xrlink inf\n");
	return NULL;
}

static void p2p_event_notify(wifi_p2p_event_t event)
{
	wifi_msg_data_t msg;

	switch (event) {
		case WIFI_P2P_GROUP_DHCP_SUCCESS:
			p2p_object.p2p_state = WIFI_P2P_CONNECTD_GC;
			break;
		case WIFI_P2P_GROUP_DNS_SUCCESS:
			p2p_object.p2p_state = WIFI_P2P_CONNECTD_GO;
			break;
		case WIFI_P2P_GROUP_DHCP_DNS_FAILURE:
		case WIFI_P2P_GROUP_DHCP_FAILURE:
		case WIFI_P2P_GROUP_DNS_FAILURE:
			p2p_object.p2p_state = WIFI_P2P_DISCONNECTD;
		default:
			break;
	}

	if (p2p_object.p2p_msg_cb) {
		msg.id = WIFI_MSG_ID_P2P_CN_EVENT;
		msg.data.p2p_event = event;
		if(p2p_object.p2p_msg_cb_arg) {
			msg.private_data = p2p_object.p2p_msg_cb_arg;
		}
		p2p_object.p2p_msg_cb(&msg);
	}
}

static wmg_status_t p2p_enable(void *param, void *cb_msg)
{
	wmg_status_t ret;

	ret = p2p_object.platform_inf[PLATFORM]->p2p_platform_extension(P2P_CMD_ENABLE, param, NULL);
	if (ret) {
		WMG_ERROR("p2p enable faile\n");
		return WMG_STATUS_FAIL;
	}
	p2p_object.p2p_state = WIFI_P2P_ENABLE;

	return ret;
}

static wmg_status_t p2p_disable(void *param, void *cb_msg)
{
	wmg_status_t ret;

	if (p2p_object.p2p_state == WIFI_P2P_DISABLE) {
		WMG_WARNG("p2p already disabled\n");
		return WMG_STATUS_UNHANDLED;
	}

	WMG_DEBUG("p2p disabling...\n");

	ret = p2p_object.platform_inf[PLATFORM]->p2p_platform_extension(P2P_CMD_DISABLE, NULL, NULL);
	if (ret) {
		WMG_ERROR("p2o disable faile\n");
	}

	p2p_object.p2p_state = WIFI_P2P_DISABLE;

	return ret;
}

static wmg_status_t p2p_find(void *param, void *cb_msg)
{
	if(p2p_object.p2p_state != WIFI_P2P_ENABLE) {
		WMG_ERROR("p2p has not been enable\n");
		return WMG_STATUS_FAIL;
	}

	return p2p_object.platform_inf[PLATFORM]->p2p_platform_extension(P2P_CMD_FIND, (void *)param, NULL);
}

static wmg_status_t p2p_connect(void *param,void *cb_msg)
{
	if(p2p_object.p2p_state != WIFI_P2P_ENABLE) {
		WMG_ERROR("p2p has not been enable\n");
		return WMG_STATUS_FAIL;
	}

	return p2p_object.platform_inf[PLATFORM]->p2p_platform_extension(P2P_CMD_CONNECT, (void *)param, NULL);
}

static wmg_status_t p2p_disconnect(void *param,void *cb_msg)
{
	if(p2p_object.p2p_state == WIFI_P2P_DISCONNECTD) {
		WMG_WARNG("p2p has disconnect\n");
		return WMG_STATUS_UNHANDLED;
	}

	return p2p_object.platform_inf[PLATFORM]->p2p_platform_extension(P2P_CMD_DISCONNECT, NULL, NULL);
}

static wmg_status_t p2p_get_info(void *param,void *cb_msg)
{
	if(p2p_object.p2p_state != WIFI_P2P_ENABLE) {
		WMG_ERROR("p2p has not been enable\n");
		return WMG_STATUS_FAIL;
	}

	return p2p_object.platform_inf[PLATFORM]->p2p_platform_extension(P2P_CMD_GET_INFO, (void *)param, NULL);
}

static wmg_status_t p2p_register_msg_cb(void *param, void *cb_msg)
{
	common_msg_cb_t *msg_cb_attr = (common_msg_cb_t *)param;

	p2p_object.p2p_msg_cb = *msg_cb_attr->msg_cb;
	p2p_object.p2p_msg_cb_arg = msg_cb_attr->msg_cb_arg;

	return WMG_STATUS_SUCCESS;
}

static wmg_status_t p2p_get_state(void *param,void *cb_msg)
{
	wifi_wmg_state_t *wmg_p2p_state = (wifi_wmg_state_t *)param;

	wmg_p2p_state->p2p_state = p2p_object.p2p_state;

	return WMG_STATUS_SUCCESS;
}

static wmg_status_t p2p_mode_init(void)
{
	if (p2p_object.init_flag == WMG_FALSE) {
		WMG_DEBUG("p2p mode init now\n");
		p2p_object.platform_inf[PLATFORM_LINUX] = p2p_linux_inf_object_register();
		p2p_object.platform_inf[PLATFORM_RTOS] = p2p_rtos_inf_object_register();
		p2p_object.platform_inf[PLATFORM_XRLINK] = p2p_xrlink_inf_object_register();

		if(NULL != p2p_object.platform_inf[PLATFORM]) {
			if(p2p_object.platform_inf[PLATFORM]->p2p_inf_init != NULL){
				if(p2p_object.platform_inf[PLATFORM]->p2p_inf_init(p2p_event_notify,NULL)){
					return WMG_STATUS_FAIL;
				}
			}
		}
		p2p_object.init_flag = WMG_TRUE;
	} else {
		WMG_DEBUG("p2p mode already init\n");
	}
	return WMG_STATUS_SUCCESS;
}

static wmg_status_t p2p_mode_deinit(void)
{
	if (p2p_object.init_flag == WMG_TRUE) {
		WMG_DEBUG("p2p mode deinit now\n");
		if(p2p_object.platform_inf[PLATFORM]->p2p_inf_deinit != NULL){
			p2p_object.platform_inf[PLATFORM]->p2p_inf_deinit(NULL);
		}
		p2p_object.init_flag = WMG_FALSE;
		p2p_object.p2p_msg_cb = NULL;
		p2p_object.p2p_msg_cb_arg = NULL;
	} else {
		WMG_DEBUG("p2p mode already deinit\n");
	}
	return WMG_STATUS_SUCCESS;
}

static wmg_status_t p2p_mode_enable(int* erro_code)
{
	wmg_status_t ret;

	if(p2p_object.init_flag == WMG_FALSE) {
		WMG_ERROR("p2p mode has not been init\n");
		return WMG_STATUS_FAIL;
	}
	if (p2p_object.p2p_enable == WMG_TRUE) {
		WMG_WARNG("p2p mode already enabled\n");
		return WMG_STATUS_UNHANDLED;
	}

	WMG_DEBUG("p2p enabling...\n");

	ret = p2p_object.platform_inf[PLATFORM]->p2p_inf_enable(NULL);
	if (ret) {
		WMG_ERROR("p2p mode enable failed\n");
		return WMG_STATUS_FAIL;
	}

	p2p_object.p2p_enable = WMG_TRUE;

	WMG_DEBUG("p2p mode enable success\n");
	return ret;
}

static wmg_status_t p2p_mode_disable(int* erro_code)
{
	wmg_status_t ret;

	if(p2p_object.init_flag == WMG_FALSE) {
		WMG_WARNG("p2p mode has not been init\n");
		return WMG_STATUS_UNHANDLED;
	}
	if (p2p_object.p2p_enable == WMG_FALSE) {
		WMG_WARNG("p2p already disabled\n");
		return WMG_STATUS_UNHANDLED;
	}

	WMG_DEBUG("p2p mode disabling...\n");

	p2p_object.platform_inf[PLATFORM]->p2p_inf_disable(NULL);
	p2p_object.p2p_enable = WMG_FALSE;

	WMG_DEBUG("p2p mode disabled\n");

	return WMG_STATUS_SUCCESS;
}

static wmg_status_t p2p_mode_ctl(int cmd, void *param, void *cb_msg)
{
	if(p2p_object.init_flag == WMG_FALSE) {
		WMG_ERROR("p2p has not been initialized\n");
		return WMG_STATUS_FAIL;
	}
	if(p2p_object.p2p_enable == WMG_FALSE) {
		WMG_ERROR("p2p already disabled\n");
		return WMG_STATUS_FAIL;
	}

	WMG_DEBUG("= p2p_mode_ctl cmd: %d =\n", cmd);

	switch (cmd) {
		case WMG_P2P_CMD_ENABLE:
			return p2p_enable(param,cb_msg);
		case WMG_P2P_CMD_DISABLE:
			return p2p_disable(param,cb_msg);
		case WMG_P2P_CMD_FIND:
			return p2p_find(param,cb_msg);
		case WMG_P2P_CMD_CONNECT:
			return p2p_connect(param,cb_msg);
		case WMG_P2P_CMD_DISCONNECT:
			return p2p_disconnect(param,cb_msg);
		case WMG_P2P_CMD_GET_INFO:
			return p2p_get_info(param,cb_msg);
		case WMG_P2P_CMD_REGISTER_MSG_CB:
			return p2p_register_msg_cb(param,cb_msg);
		case WMG_P2P_CMD_GET_STATE:
			return p2p_get_state(param,cb_msg);
		default:
			return WMG_STATUS_FAIL;
	}
}

static wmg_p2p_object_t p2p_object = {
	.init_flag = WMG_FALSE,
	.p2p_enable = WMG_FALSE,
	.p2p_state = WIFI_P2P_DISABLE,
	.p2p_msg_cb = NULL,
	.p2p_msg_cb_arg = NULL,
};

static mode_opt_t p2p_mode_opt = {
	.mode_enable = p2p_mode_enable,
	.mode_disable = p2p_mode_disable,
	.mode_ctl = p2p_mode_ctl,
};

static mode_object_t p2p_mode_object = {
	.mode_name = "p2p",
	.init = p2p_mode_init,
	.deinit = p2p_mode_deinit,
	.mode_opt = &p2p_mode_opt,
	.private_data = &p2p_object,
};

mode_object_t* wmg_p2p_register_object(void)
{
	return &p2p_mode_object;
}
