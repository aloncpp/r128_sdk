#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hal_sem.h>
#include <hal_mem.h>
#include <hal_mutex.h>
//#include <hal_time.h>
#include <hal_thread.h>
#include <hal_time.h>

#include <wifimg.h>
#include "soft_ap_config.h"
#include "net_ctrl.h"
int rtos_main(void *snd_cmd);

#include "iot_controller.h"

#define CONFIG_IOT_CONTROLLER_SAVE_CONFIG_TO_FILE
#ifdef CONFIG_IOT_CONTROLLER_SAVE_CONFIG_TO_FILE
#include "file_opt.h"
#endif

#define CONFIG_IOT_CONTROLLER_TEST_CMD
#ifdef CONFIG_IOT_CONTROLLER_TEST_CMD
#include <console.h>
#endif

#define SEM_TIMEOUT_MS		(1000)

struct iot_controller_t {
    volatile int run;
	void *thread;
	hal_sem_t sem;
	hal_mutex_t mutex;
	int net_state;
	int allow_cb;

	char *wifi_config_path;
    unsigned char ap_ssid[32];
    unsigned char ap_psk[32];
    unsigned char ssid[32];
    unsigned char psk[32];

	iot_event_cb_t cb;
	void *priv;
};

#define CONFIG_CHECK_CORRECT    (0)
#define CONFIG_CHECK_BAD        (-1)
#define CONFIG_EXIST            (2)
#define CONFIG_NOT_EXIST        (-2)
#define CONFIG_LOAD_SUCCESS     (0)
#define CONFIG_LOAD_ERROR       (-1)
#define CONFIG_SAVE_SUCCESS     (0)
#define CONFIG_SAVE_ERROR       (-1)
#define CONFIG_CLEAN_SUCCESS     (0)
#define CONFIG_CLEAN_ERROR       (-1)

static inline const char *iot_event_to_str(enum iot_event_t event)
{
	switch(event) {
	case IOT_EVENT_ERROR:
		return "IOT_EVENT_ERROR";
	case IOT_EVENT_NONE:
		return "IOT_EVENT_NONE";
	case IOT_EVENT_WAIT_WIFI_CONFIG:
		return "IOT_EVENT_WAIT_WIFI_CONFIG";
	case IOT_EVENT_GET_WIFI_CONFIG:
		return "IOT_EVENT_GET_WIFI_CONFIG";
	case IOT_EVENT_NET_CONNECT:
		return "IOT_EVENT_NET_CONNECT";
	case IOT_EVENT_NET_DISCONNECT:
		return "IOT_EVENT_NET_DISCONNECT";
	default:
		return "IOT_EVENT_UNKNOWN";
	}
}

#ifdef CONFIG_IOT_CONTROLLER_SAVE_CONFIG_TO_FILE
static inline int save_wifi_config(struct iot_controller_t *hdl)
{
	unsigned char data[32+32+4];
	int len = 0;
	int ssid_len = strlen(hdl->ssid);
	int psk_len = strlen(hdl->psk);

	if (!hdl->wifi_config_path) {
		printf("no path to save wifi condfig!\n");
		return CONFIG_SAVE_SUCCESS;
	}

	if (ssid_len <= 0)
		return CONFIG_SAVE_ERROR;

	if (strstr(hdl->ssid, "\r\n"))
		return CONFIG_SAVE_ERROR;

	if (strstr(hdl->psk, "\r\n"))
		return CONFIG_SAVE_ERROR;

	memcpy(&data[len], hdl->ssid, ssid_len);
	len += ssid_len;
	memcpy(&data[len], "\r\n", 2);
	len += 2;
	if (psk_len > 0) {
		memcpy(&data[len], hdl->psk, psk_len);
		len += psk_len;
	}
	memcpy(&data[len], "\r\n", 2);
	len += 2;

	if (len == save_data_to_file(hdl->wifi_config_path, data, len))
    	return CONFIG_SAVE_SUCCESS;
	else
		return CONFIG_SAVE_ERROR;
}

static inline int load_wifi_config(struct iot_controller_t *hdl)
{
	unsigned char data[32+32+4+1];
	int len = 0;
	int ssid_len;
	int psk_len;
	unsigned char *ptr;

	if (!hdl->wifi_config_path) {
		printf("no path to load wifi condfig!\n");
		return CONFIG_NOT_EXIST;
	}

	data[32+32+4] = 0;
	len = load_data_from_file(hdl->wifi_config_path, data, sizeof(data));
	if (len <= 4) {
		printf("read %s failed!\n", hdl->wifi_config_path);
		return CONFIG_LOAD_ERROR;
	}

	ptr = strstr(data, "\r\n");
	if (!ptr) {
		printf("parser ssid failed!\n");
		return CONFIG_LOAD_ERROR;
	}

	ssid_len = ptr - data;
	memset(ptr, 0, 2);

	ptr = strstr(&data[ssid_len + 2], "\r\n");
	if (!ptr) {
		printf("parser psk failed!\n");
		return CONFIG_LOAD_ERROR;
	}

	psk_len = ptr - &data[ssid_len + 2];
	memset(ptr, 0, 2);

	memset(hdl->ssid, 0, sizeof(hdl->ssid));
	memset(hdl->psk, 0, sizeof(hdl->psk));

	strncpy(hdl->ssid, data, ssid_len + 1);
	strncpy(hdl->psk, &data[ssid_len + 2], psk_len + 1);

    return CONFIG_LOAD_SUCCESS;
}

static inline int check_wifi_config(struct iot_controller_t *hdl)
{
	unsigned char ssid[32];
	unsigned char psk[32];
	memcpy(ssid, hdl->ssid, sizeof(ssid));
	memcpy(psk, hdl->psk, sizeof(psk));
	if (load_wifi_config(hdl) == CONFIG_LOAD_SUCCESS) {
		if (!strcmp(ssid, hdl->ssid) && !strcmp(psk, hdl->psk))
			return CONFIG_CHECK_CORRECT;
		memcpy(hdl->ssid, ssid, sizeof(hdl->ssid));
		memcpy(hdl->psk, psk, sizeof(hdl->psk));
	}
    return CONFIG_CHECK_BAD;
}

static inline int clean_wifi_config(struct iot_controller_t *hdl)
{
	remove(hdl->wifi_config_path);
    return CONFIG_CLEAN_SUCCESS;
}
#else
static inline int save_wifi_config(struct iot_controller_t *hdl)
{
    return CONFIG_SAVE_SUCCESS;
}

static inline int load_wifi_config(struct iot_controller_t *hdl)
{
    return CONFIG_NOT_EXIST;
}

static inline int check_wifi_config(struct iot_controller_t *hdl)
{
    return CONFIG_CHECK_BAD;
}

static inline int clean_wifi_config(struct iot_controller_t *hdl)
{
    return CONFIG_CLEAN_SUCCESS;
}
#endif

static inline void iot_event_cb(struct iot_controller_t *hdl, enum iot_event_t event)
{
	int report = 1;
	hal_mutex_lock(hdl->mutex);
	switch(event) {
	case IOT_EVENT_NET_DISCONNECT:
		if (hdl->net_state == 0)
			report = 0;
	case IOT_EVENT_ERROR:
	case IOT_EVENT_WAIT_WIFI_CONFIG:
    case IOT_EVENT_GET_WIFI_CONFIG:
		hdl->net_state = 0;
		break;
	case IOT_EVENT_NET_CONNECT:
		if (hdl->net_state == 1)
			report = 0;
		hdl->net_state = 1;
		break;
	case IOT_EVENT_NONE:
	default:
		break;
	}
	if (hdl->cb && report)
		hdl->cb(hdl->priv, event);
	hal_mutex_unlock(hdl->mutex);
}

static void *g_hdl = NULL;

static void wifi_msg_cb(wifi_msg_data_t *msg)
{
	printf("<%s:%d> id: %d\n", __func__, __LINE__, msg->id);
	if (!msg->private_data) {
		printf("<%s:%d> private_data=NULL!\n", __func__, __LINE__);
		return;
	}

	struct iot_controller_t *hdl = *(struct iot_controller_t **)msg->private_data;

	if (!hdl->allow_cb)
		return;

	switch(msg->id) {
	case WIFI_MSG_ID_STA_CN_EVENT:
		printf("<%s:%d> STA_CN_EVENT: %d\n", __func__, __LINE__, msg->data.state);
		switch(msg->data.event) {
		case WIFI_DISCONNECTED:
			iot_event_cb(hdl, IOT_EVENT_NET_DISCONNECT);
			break;
		case WIFI_DHCP_SUCCESS:
			iot_event_cb(hdl, IOT_EVENT_NET_CONNECT);
			break;
		default:
			break;
		}
		break;
	case WIFI_MSG_ID_STA_STATE_CHANGE:
		printf("<%s:%d> STA_STATE_CHANGE: %d\n", __func__, __LINE__, msg->data.state);
		switch(msg->data.state) {
		case WIFI_STA_NET_CONNECTED:
			iot_event_cb(hdl, IOT_EVENT_NET_CONNECT);
			break;
		case WIFI_STA_DISCONNECTED:
			iot_event_cb(hdl, IOT_EVENT_NET_DISCONNECT);
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}

static inline void register_wifi_event_cb(struct iot_controller_t *hdl) {
	hdl->allow_cb = 1;
	if (!g_hdl) {
		g_hdl = hdl;
		wifi_register_msg_cb(wifi_msg_cb, &g_hdl);
	} else if (g_hdl != hdl) {
		void *hdl_last = g_hdl;
		g_hdl = hdl;
		printf("g_hdl: change %lx to %lx\n", (unsigned long)hdl_last, (unsigned long)hdl);
	}
}

static inline void unregister_wifi_event_cb(struct iot_controller_t *hdl) {
	hdl->allow_cb = 0;
	if (g_hdl == hdl) {
		g_hdl = NULL;
	}
}

static soft_ap_config_result *g_config = NULL;

static inline void get_config_from_soft_ap_config(struct iot_controller_t *hdl, soft_ap_config_result *result)
{
    memcpy(hdl->ssid, result->ssid, sizeof(hdl->ssid));
    memcpy(hdl->psk, result->psk, sizeof(hdl->psk));
}

static void get_config_cb(soft_ap_config_result *result, SOFT_AP_CONFIG_STA state)
{
    memcpy(g_config, result, sizeof(*g_config));
    printf("ssid:%s psk:%s state:%d\n", g_config->ssid, g_config->psk, state);
    g_config = NULL;
}

static void iot_controller_thread(void *arg)
{
	struct iot_controller_t *hdl = (struct iot_controller_t *)arg;
	struct netif *nif;
    soft_ap_config_result wifi_config;
	wifi_ap_config_t ap_config;
	ap_config.ssid = hdl->ap_ssid;
	ap_config.psk = hdl->ap_psk;
	ap_config.sec = strlen(hdl->ap_psk) ? WIFI_SEC_WPA2_PSK : WIFI_SEC_NONE;
	ap_config.channel = 6;

	if (hal_sem_post(hdl->sem))
		printf("hal_sem_post failed!\n");

	//init wifi manager
	rtos_main("");

	//判断是否传入wifi信息
	if(strlen(hdl->ssid))
		goto try_connect;

    //判断是否已经配网
    switch(load_wifi_config(hdl)) {
        case CONFIG_LOAD_SUCCESS:
            goto try_connect;
        case CONFIG_NOT_EXIST:
        default:
            printf("load wifi config failed!\n");
            break;
    }

try_get_config:
    //走配网流程
	iot_event_cb(hdl, IOT_EVENT_WAIT_WIFI_CONFIG);
	if (wifi_on(WIFI_AP)) {
		printf("wifi_on ap failed!\n");
		goto exit;
	}
	if (wifi_ap_enable(&ap_config)) {
		printf("wifi_ap_enable failed!\n");
		goto exit;
	}
	hal_msleep(1000);
    while (hdl->run) {
        memset(&wifi_config, 0, sizeof(wifi_config));
        int is_start = 0;
        nif = g_wlan_netif;
		if (!nif) {
			printf("nif=NULL!\n");
			hal_msleep(100);
			continue;
		}
        g_config = &wifi_config;
        soft_ap_config_set_cb(get_config_cb);
        while(hdl->run && NETIF_IS_AVAILABLE(nif) && g_config) {
            if (!is_start) {
                soft_ap_config_start();
                is_start = 1;
            }
            hal_msleep(100);
			// TODO: timeout
        }
		hal_msleep(1000);//xr_softap_task will crash when call soft_ap_config_stop fast
        soft_ap_config_stop();
        if (!g_config) {
			iot_event_cb(hdl, IOT_EVENT_GET_WIFI_CONFIG);
			hal_msleep(1000);
			break;
		}
    }
    get_config_from_soft_ap_config(hdl, &wifi_config);

try_connect:
    //连接网络
    if (!hdl->run)
        goto exit;
	if (wifi_on(WIFI_STATION)) {
		printf("wifi on sta mode failed\n");
		goto exit;
	}
	printf("wifi on sta mode success\n");

	register_wifi_event_cb(hdl);

    while(hdl->run) {
        wifi_sta_cn_para_t cn_para;
	    cn_para.ssid = hdl->ssid;
	    cn_para.password = hdl->psk;
	    cn_para.fast_connect = 0;
	    cn_para.sec = WIFI_SEC_WPA2_PSK;
	    if (wifi_sta_connect(&cn_para)) {
		    printf("wifi connect ap failed!\n");
            hal_msleep(500);
			continue;
			// TODO: timeout
            //if ()
            //    goto try_get_config;
	    }
		break;
    }
	printf("wifi connect ap success!\n");
	//iot_event_cb(hdl, IOT_EVENT_NET_CONNECT);

try_save_config:
    //保存配网信息
    if (!hdl->run)
        goto exit;
    switch(check_wifi_config(hdl)) {
        case CONFIG_CHECK_CORRECT:
            goto done;
        default:
            break;
    }

    if (!hdl->run)
        goto exit;
    switch(save_wifi_config(hdl)) {
        case CONFIG_SAVE_SUCCESS:
            break;
        case CONFIG_EXIST:
            clean_wifi_config(hdl);
            goto try_save_config;
        default:
            printf("save wifi config failed!\n");
            goto exit;
    }

done:
    //监控网络状态
    while(hdl->run) {
        hal_msleep(2000);
		nif = g_wlan_netif;
		if (!nif)
			continue;
		if(NET_IS_IP4_VALID(nif) && netif_is_link_up(nif)) {
			iot_event_cb(hdl, IOT_EVENT_NET_CONNECT);
		} else {
			iot_event_cb(hdl, IOT_EVENT_NET_DISCONNECT);
		}
    }

exit:
	unregister_wifi_event_cb(hdl);
	if (hal_sem_post(hdl->sem))
		printf("hal_sem_post failed!\n");
	hal_thread_stop(NULL);
	printf("%s exit\n", __func__);
}

void iot_controller_destroy(void *_hdl)
{
	struct iot_controller_t *hdl = hal_malloc(sizeof(*hdl));

	if (hdl) {
		if (hdl->wifi_config_path) {
			hal_free(hdl->wifi_config_path);
			hdl->wifi_config_path = NULL;
		}

		if (hdl->mutex) {
			hal_mutex_delete(hdl->mutex);
			hdl->mutex = NULL;
		}

		if (hdl->sem) {
			hal_sem_delete(hdl->sem);
			hdl->sem = NULL;
		}

		hal_free(hdl);
	}
}

void *iot_controller_create(struct iot_controller_config_t *config)
{
	struct iot_controller_t *hdl = hal_malloc(sizeof(*hdl));

	if (!hdl) {
		printf("no memory!\n");
		goto err;
	}
	memset(hdl, 0, sizeof(*hdl));

	if (config->wifi_config_path) {
		int len = strlen(config->wifi_config_path);
		hdl->wifi_config_path = hal_malloc(len + 1);
		if (!hdl->wifi_config_path) {
			printf("no memory!\n");
			goto err;
		}
		memcpy(hdl->wifi_config_path, config->wifi_config_path, len + 1);
	}

	if (config->ap_ssid &&config->ap_psk) {
		strncpy(hdl->ap_ssid, config->ap_ssid, sizeof(hdl->ap_ssid));
		strncpy(hdl->ap_psk, config->ap_psk, sizeof(hdl->ap_psk));
	} else {
		strncpy(hdl->ap_ssid, "default_ssid", sizeof(hdl->ap_ssid));
		strncpy(hdl->ap_psk, "default_psk", sizeof(hdl->ap_psk));
	}

	if (config->ssid &&config->psk) {
		strncpy(hdl->ssid, config->ssid, sizeof(hdl->ssid));
		strncpy(hdl->psk, config->psk, sizeof(hdl->psk));
	}
	hdl->cb = config->cb;
	hdl->priv = config->priv;

	hdl->sem = hal_sem_create(0);
	if (!hdl->sem) {
		printf("no memory!\n");
		goto err;
	}

	hdl->mutex = hal_mutex_create();
	if (!hdl->mutex) {
		printf("no memory!\n");
		goto err;
	}

	return hdl;
err:
	iot_controller_destroy(hdl);
	return NULL;
}

int iot_controller_stop(void *_hdl)
{
	struct iot_controller_t *hdl = (struct iot_controller_t *)_hdl;

	hdl->run = 0;

	while (0 > hal_sem_timedwait(hdl->sem, pdMS_TO_TICKS(SEM_TIMEOUT_MS))) {
		printf("wait timeout!\n");
		//goto err;
	}

	return 0;
}

int iot_controller_start(void *_hdl)
{
	struct iot_controller_t *hdl = (struct iot_controller_t *)_hdl;

	hdl->run = 1;
	hdl->net_state = 0;
	hdl->thread = hal_thread_create(iot_controller_thread, hdl, "iot_controller_thread", 1024, 5);
	hal_thread_start(hdl->thread);

	while (0 > hal_sem_timedwait(hdl->sem, pdMS_TO_TICKS(SEM_TIMEOUT_MS))) {
		printf("wait timeout!\n");
		//goto err;
	}

	return 0;
}

int iot_controller_reset_config(void *_hdl)
{
	struct iot_controller_t *hdl = (struct iot_controller_t *)_hdl;

	iot_controller_stop(hdl);
	clean_wifi_config(hdl);
	memset(hdl->ssid, 0, sizeof(hdl->ssid));
	memset(hdl->psk, 0, sizeof(hdl->psk));
	iot_controller_start(hdl);
	return 0;
}

#ifdef CONFIG_IOT_CONTROLLER_TEST_CMD
static void *iot_controller_hdl = NULL;

static void iot_event_test_cb(void *priv, enum iot_event_t event)
{
	printf("priv: %lx, event: %d(%s)\n", (unsigned long)priv, (int)event, iot_event_to_str(event));
}

static struct iot_controller_config_t iotc_config = {
	.wifi_config_path = "/data/wifi_config.txt",
	.ap_ssid = "iotc_ap",
	.ap_psk = "1qaz@WSX",
	.ssid = NULL,
	.psk = NULL,
	.cb = iot_event_test_cb,
	.priv = (void *)0x12345678,
};
int cmd_iot_controller(int argc, char ** argv)
{

	if (argc >= 2 && !strcmp(argv[1], "start")) {
		printf("start iot controller test!\n");
		iot_controller_hdl = iot_controller_create(&iotc_config);
		if (!iot_controller_hdl) {
			printf("no memory!\n");
			goto err;
		}

		iot_controller_start(iot_controller_hdl);
	} else if (argc == 2 && !strcmp(argv[1], "stop") && iot_controller_hdl){
		printf("stop iot controller test!\n");
		iot_controller_stop(iot_controller_hdl);
		iot_controller_destroy(iot_controller_hdl);
		iot_controller_hdl = NULL;
	}

	return 0;
err:
	iot_controller_destroy(iot_controller_hdl);
	iot_controller_hdl = NULL;
	return -1;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_iot_controller, iot_controller, asr demo);
#endif