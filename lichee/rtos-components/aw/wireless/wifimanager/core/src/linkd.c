#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <wifimg.h>
#include <wifi_log.h>
#include <linkd.h>
#include <os_net_thread.h>

static wmg_linkd_object_t wmg_linkd_object;

typedef struct {
	proto_main_loop main_loop;
	char name[10];
}main_loop_function_t;

static void linkd_mode_result_cb(wifi_linkd_result_t *linkd_result)
{
	int ssid_len, psk_len;

	if(linkd_result == NULL){
		WMG_ERROR("get ssid and psk fail\n");
		wmg_linkd_object.result_state = WMG_LINKD_RESULT_FAIL;
		return;
	}

	ssid_len = strlen(linkd_result->ssid);
	psk_len = strlen(linkd_result->psk);
	if(((ssid_len < 0) || (ssid_len > SSID_MAX_LEN)) || ((psk_len < 0) || (psk_len > SSID_MAX_LEN)))
	{
		WMG_ERROR("get ssid and psk size larger than buff size\n");
		wmg_linkd_object.result_state = WMG_LINKD_RESULT_FAIL;
	} else {
		strncpy(wmg_linkd_object.ssid_result, linkd_result->ssid, ssid_len);
		strncpy(wmg_linkd_object.psk_result, linkd_result->psk, psk_len);
		wmg_linkd_object.result_state = WMG_LINKD_RESULT_SUCCESS;
		WMG_DEBUG("get ssid(%s) and psk(%s) success\n", wmg_linkd_object.ssid_result, wmg_linkd_object.psk_result);
	}
}

void *__attribute__((weak)) _ble_mode_main_loop(void *arg)
{
	WMG_WARNG("wk: unspport ble config net\n");
	proto_main_loop_para_t *main_loop_para = (proto_main_loop_para_t *)arg;
	main_loop_para->result_cb(NULL);
}

void *__attribute__((weak)) _softap_mode_main_loop(void *arg)
{
	WMG_WARNG("wk: unsupport softap config net\n");
	proto_main_loop_para_t *main_loop_para = (proto_main_loop_para_t *)arg;
	main_loop_para->result_cb(NULL);
}

void *__attribute__((weak)) _xconfig_mode_main_loop(void *arg)
{
	WMG_WARNG("wk: unsupport xconfig config net\n");
	proto_main_loop_para_t *main_loop_para = (proto_main_loop_para_t *)arg;
	main_loop_para->result_cb(NULL);
}

void *__attribute__((weak)) _soundwave_mode_main_loop(void *arg)
{
	WMG_WARNG("wk: unsupport sounddwave config net\n");
	proto_main_loop_para_t *main_loop_para = (proto_main_loop_para_t *)arg;
	main_loop_para->result_cb(NULL);
}

main_loop_function_t main_loop_function[WMG_LINKD_PROTO_MAX] = {
	{_ble_mode_main_loop, "ble"},
	{_softap_mode_main_loop, "softap"},
	{_xconfig_mode_main_loop, "xconfig"},
	{_soundwave_mode_main_loop, "soundwave"},
};

static wmg_status_t wmg_linkd_init()
{
	wmg_linkd_object.linkd_state = WMG_LINKD_IDEL;
	wmg_linkd_object.main_loop_para.result_cb = linkd_mode_result_cb;
	return WMG_STATUS_SUCCESS;
}

static wmg_status_t wmg_linkd_protocol_enable(wifi_linkd_mode_t mode, void *proto_function_param)
{
	wmg_status_t ret = WMG_STATUS_FAIL;
	int main_loop_function_num = (int)mode;

	if(wmg_linkd_object.linkd_state != WMG_LINKD_IDEL) {
		WMG_ERROR("Wmg linkd busy, running %d mode now\n",wmg_linkd_object.linkd_mode_state);
		return WMG_STATUS_BUSY;
	}

	WMG_INFO("%s protocol linkd mode\n", main_loop_function[main_loop_function_num].name);

	if(!os_net_thread_create(&wmg_linkd_object.thread,NULL, main_loop_function[main_loop_function_num].main_loop, proto_function_param, 20, 4096)){
		WMG_DEBUG("create %s mode main loop pthread success\n", main_loop_function[main_loop_function_num].name);
		wmg_linkd_object.linkd_state = WMG_LINKD_RUNNING;
		wmg_linkd_object.linkd_mode_state = mode;
		return WMG_STATUS_SUCCESS;
	} else {
		WMG_ERROR("create %s mode main loop pthread fail\n", main_loop_function[main_loop_function_num].name);
		wmg_linkd_object.linkd_state = WMG_LINKD_IDEL;
		wmg_linkd_object.linkd_mode_state = WMG_LINKD_MODE_NONE;
		return WMG_STATUS_FAIL;
	}
}

static wmg_status_t wmg_linkd_protocol_get_results(wifi_linkd_result_t *linkd_result, int second)
{
	wmg_status_t ret = WMG_STATUS_FAIL;
	int wait_second = DEFAULT_SECOND;
	if(second > 0) {
		wait_second = second;
	}
	while((wait_second > 0) && (wmg_linkd_object.result_state == WMG_LINKD_RESULT_INVALIN)) {
		sleep(1);
		wait_second--;
	}
	if(wait_second < 0) {
		linkd_result->ssid = NULL;
		linkd_result->psk = NULL;
		os_net_thread_delete(wmg_linkd_object.thread);
		return WMG_STATUS_TIMEOUT;
	} else if (wmg_linkd_object.result_state == WMG_LINKD_RESULT_SUCCESS) {
		strncpy(linkd_result->ssid, wmg_linkd_object.ssid_result, strlen(wmg_linkd_object.ssid_result));
		strncpy(linkd_result->psk, wmg_linkd_object.psk_result, strlen(wmg_linkd_object.psk_result));
		ret  = WMG_STATUS_SUCCESS;
	} else {
		ret = WMG_STATUS_FAIL;
	}
	wmg_linkd_object.result_state == WMG_LINKD_RESULT_INVALIN;

	return ret;
}

static void wmg_linkd_deinit(void)
{
	wmg_linkd_object.thread = (void *) -1;
	wmg_linkd_object.linkd_mode_state = WMG_LINKD_MODE_NONE;
	memset(wmg_linkd_object.ssid_result, 0, SSID_MAX_LEN);
	memset(wmg_linkd_object.psk_result, 0, PSK_MAX_LEN);
	wmg_linkd_object.result_state = WMG_LINKD_RESULT_INVALIN;
	wmg_linkd_object.linkd_state = WMG_LINKD_OFF;
}

static wmg_linkd_object_t wmg_linkd_object = {
	.ssid_result = {0},
	.psk_result = {0},
	.result_state = WMG_LINKD_RESULT_INVALIN,
	.thread = (void *)-1,
	.linkd_state = WMG_LINKD_OFF,
	.linkd_mode_state = WMG_LINKD_MODE_NONE,
	.linkd_init = wmg_linkd_init,
	.linkd_protocol_enable = wmg_linkd_protocol_enable,
	.linkd_protocol_get_results = wmg_linkd_protocol_get_results,
	.linkd_deinit = wmg_linkd_deinit,
};

wmg_status_t wmg_linkd_protocol(wifi_linkd_mode_t mode, void *params, int second, wifi_linkd_result_t *linkd_result)
{
	wmg_status_t ret = WMG_STATUS_FAIL;

	if(wmg_linkd_object.linkd_state == WMG_LINKD_OFF) {
		WMG_DEBUG("Wmg link isn't init, init now\n");
		wmg_linkd_object.linkd_init();
	}

	if(mode >= WMG_LINKD_MODE_NONE) {
		WMG_INFO("Unsupport protocol linkd mode\n");
		return WMG_STATUS_UNSUPPORTED;
	}

	if(!wmg_linkd_object.linkd_protocol_enable(mode, (void *)&wmg_linkd_object.main_loop_para)) {
		ret = wmg_linkd_object.linkd_protocol_get_results(linkd_result, second);
		wmg_linkd_object.linkd_deinit();
		return ret;
	}

	return WMG_STATUS_FAIL;
}
