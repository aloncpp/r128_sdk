#ifndef __IOT_CONTROLLER_H__
#define __IOT_CONTROLLER_H__

enum iot_event_t {
    IOT_EVENT_ERROR = -1,
    IOT_EVENT_NONE = 0,
    IOT_EVENT_WAIT_WIFI_CONFIG,
    IOT_EVENT_GET_WIFI_CONFIG,
    IOT_EVENT_NET_CONNECT,
    IOT_EVENT_NET_DISCONNECT,
};

typedef void (*iot_event_cb_t)(void *priv, enum iot_event_t event);

struct iot_controller_config_t {
    const char *wifi_config_path;
    unsigned char *ap_ssid;
    unsigned char *ap_psk;
    unsigned char *ssid;
    unsigned char *psk;

    iot_event_cb_t cb;
    void *priv;
};

void *iot_controller_create(struct iot_controller_config_t *config);
void iot_controller_destroy(void *_hdl);
int iot_controller_start(void *_hdl);
int iot_controller_stop(void *_hdl);
int iot_controller_reset_config(void *_hdl);

#endif /* __IOT_CONTROLLER_H__ */

