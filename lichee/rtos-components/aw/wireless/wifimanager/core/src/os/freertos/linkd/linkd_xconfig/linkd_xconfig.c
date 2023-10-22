#include <wifimg.h>
#include <wifi_log.h>
#include <linkd.h>
#include "net/wlan/wlan.h"
#include "net/wlan/wlan_defs.h"
#include "lwip/netif.h"
#include "sc_assistant.h"
#include "wlan_smart_config.h"
#include "net_ctrl.h"
#include "net_init.h"

#define SC_TIME_OUT 120000
#define THREAD_STACK_SIZE       (4 * 1024)

static uint8_t sc_key_used = 0;
static char sc_key[17] = "1234567812345678";
static bool is_receive = false;
static XR_OS_Thread_t g_thread;
extern struct netif *g_wlan_netif;
proto_main_loop_para_t *main_loop_para = NULL;

static void sc_task(void *arg)
{
    wlan_smart_config_result_t sc_result;
	wifi_linkd_result_t linkd_result;

    memset(&sc_result, 0, sizeof(wlan_smart_config_result_t));

    if (wlan_smart_config_wait(SC_TIME_OUT) == WLAN_SMART_CONFIG_TIMEOUT) {
        WMG_DEBUG("%s get ssid and psk timeout\n", __func__);
		main_loop_para->result_cb(NULL);
        goto out;
    }

    if (wlan_smart_config_get_status() == SC_STATUS_COMPLETE) {
        smartconfig_get_result(&sc_result);
        WMG_DEBUG("ssid:%s psk:%s random:%d\n", (char *)sc_result.ssid,
                (char *)sc_result.passphrase, sc_result.random_num);
		is_receive = true;
		linkd_result.ssid = sc_result.ssid;
		linkd_result.psk = sc_result.passphrase;
        WMG_DEBUG("linkd_result.ssid:%s linkd_result.psk:%s \n", linkd_result.ssid, linkd_result.psk);
		main_loop_para->result_cb(&linkd_result);
    }

out:
    wlan_smart_config_stop();
    sc_assistant_deinit(g_wlan_netif);
    XR_OS_ThreadDelete(&g_thread);
}



void *_xconfig_mode_main_loop(void *arg)
{
	WMG_INFO("support smartconfig mode config net\n");
	main_loop_para = (proto_main_loop_para_t *)arg;
	wlan_smart_config_status_t sc_status;
	sc_assistant_fun_t sca_fun;
	sc_assistant_time_config_t config;

	if (XR_OS_ThreadIsValid(&g_thread)) {
		main_loop_para->result_cb(NULL);
		return NULL;
	}
	xr_wlan_on(WLAN_MODE_MONITOR);
	sc_assistant_get_fun(&sca_fun);
	config.time_total = SC_TIME_OUT;
	config.time_sw_ch_long = 100;
	config.time_sw_ch_short = 50;
	sc_assistant_init(g_wlan_netif, &sca_fun, &config);

	sc_status = wlan_smart_config_start(g_wlan_netif, sc_key_used ? sc_key : NULL);
	if (sc_status != WLAN_SMART_CONFIG_SUCCESS) {
		WMG_ERROR("smartconfig start fiald!\n");
		goto out;
	}

	if (XR_OS_ThreadCreate(&g_thread,
				"cmd_sc",
				sc_task,
				NULL,
				XR_OS_THREAD_PRIO_APP,
				THREAD_STACK_SIZE) != XR_OS_OK) {
		WMG_ERROR("create sc thread failed\n");
		goto out;
	}

	while(!is_receive){
		XR_OS_MSleep(100);
	}
	return NULL;
out:
	main_loop_para->result_cb(NULL);
	return NULL;

}
