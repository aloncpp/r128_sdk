#include <wifimg.h>
#include <wifi_log.h>
#include <linkd.h>

#include "soft_ap_config.h"
#include "net_ctrl.h"
#include "net_init.h"

static bool is_receive = false;
static proto_main_loop_para_t *main_loop_para = NULL;

static void soft_ap_config_callback(soft_ap_config_result *result, SOFT_AP_CONFIG_STA state)
{
	wifi_linkd_result_t linkd_result;
	linkd_result.ssid = result->ssid;
	linkd_result.psk = result->psk;
	main_loop_para->result_cb(&linkd_result);

	is_receive = true;
	WMG_INFO("ssid:%s psk:%s state:%d\n", result->ssid, result->psk, state);
}

void *_softap_mode_main_loop(void *arg)
{
	WMG_INFO("support softap mode config net\n");
	main_loop_para = (proto_main_loop_para_t *)arg;

	int ret = -1;
	wifi_ap_config_t ap_config;
	int soft_ap_has_start = 0;

	/* set soft_ap_config callback */
	soft_ap_config_set_cb(soft_ap_config_callback);
	WMG_INFO("recvfrom...\n");
	struct netif *nif = g_wlan_netif;
	while (!is_receive) {
		if (NETIF_IS_AVAILABLE(nif) && !soft_ap_has_start) {
			/* if the network is up, start the soft_ap_config */
			soft_ap_config_start();
			soft_ap_has_start = 1;
		}

		XR_OS_MSleep(100);
	}

	WMG_INFO("recvfrom success.\n");

	soft_ap_config_stop();
	is_receive = false;

	return NULL;
}
