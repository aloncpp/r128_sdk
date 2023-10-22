#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "xr_a2dp_api.h"
#include "btmg_log.h"
#include "btmg_common.h"
#include "btmg_audio.h"
#include "bt_manager.h"

static const char *s_a2d_conn_state_str[] = { "Disconnected", "Connecting", "Connected",
                                              "Disconnecting" };
static const char *s_a2d_audio_state_str[] = { "Suspended", "Stopped", "Started" };

static uint32_t sample_rate = 16000;
static uint8_t channels = 2;
static int second_byte = 0;
static uint64_t time_total = 0;
static uint64_t data_total = 0;

static void pcm_config(uint8_t *codec_info)
{
    char oct0 = codec_info[0];

    if (oct0 & (0x01 << 6)) {
        sample_rate = 32000;
    } else if (oct0 & (0x01 << 5)) {
        sample_rate = 44100;
    } else if (oct0 & (0x01 << 4)) {
        sample_rate = 48000;
    }

    if ((oct0 & 0x0F) == 0x08) {
        channels = 1;
    }

    bt_audio_pcm_config(sample_rate, channels, BT_AUDIO_TYPE_A2DP_SNK);
    second_byte = sample_rate * channels * 2;

    BTMG_INFO("audio_config: channels:%d, sampling:%d", channels, sample_rate);
}

static void bt_a2dp_sink_cb(xr_a2d_cb_event_t event, xr_a2d_cb_param_t *param)
{
    dev_node_t *dev_node = NULL;

    switch (event) {
    case XR_A2D_CONNECTION_STATE_EVT: {
        bda2str(param->conn_stat.remote_bda, bda_str);
        BTMG_INFO("a2dp sink %s, dev:%s", s_a2d_conn_state_str[param->conn_stat.state], bda_str);
        if (btmg_cb_p[CB_MAIN] && btmg_cb_p[CB_MAIN]->btmg_a2dp_sink_cb.conn_state_cb) {
            if (param->conn_stat.state == XR_A2D_CONNECTION_STATE_DISCONNECTED) {
                if (connected_devices->sem_flag) {
                    XR_OS_SemaphoreRelease(&(connected_devices->sem));
                }
                dev_node = btmg_dev_list_find_device(connected_devices, bda_str);
                if (dev_node != NULL) {
                    BTMG_DEBUG("remove device %s from connected_devices", bda_str);
                    dev_node->profile &= ~A2DP_SNK_DEV;
                    btmg_dev_list_remove_device(connected_devices, bda_str);
                }
                btmg_cb_p[CB_MAIN]->btmg_a2dp_sink_cb.conn_state_cb(bda_str, BTMG_A2DP_SINK_DISCONNECTED);
            } else if (param->conn_stat.state == XR_A2D_CONNECTION_STATE_CONNECTING) {
                btmg_cb_p[CB_MAIN]->btmg_a2dp_sink_cb.conn_state_cb(bda_str, BTMG_A2DP_SINK_CONNECTING);
            } else if (param->conn_stat.state == XR_A2D_CONNECTION_STATE_CONNECTED) {
                dev_node = btmg_dev_list_find_device(connected_devices, bda_str);
                if (dev_node == NULL) {
                    btmg_dev_list_add_device(connected_devices, NULL, bda_str, A2DP_SNK_DEV);
                } else {
                    dev_node->profile |= A2DP_SNK_DEV;
                }
                BTMG_DEBUG("add device %s into connected_devices", bda_str);
                btmg_cb_p[CB_MAIN]->btmg_a2dp_sink_cb.conn_state_cb(bda_str, BTMG_A2DP_SINK_CONNECTED);
            } else if (param->conn_stat.state == XR_A2D_CONNECTION_STATE_DISCONNECTING) {
#ifndef CONFIG_A2DP_SINK_AUDIO_CB
                sys_handler_send(bt_audio_event_handle, BT_AUDIO_EVENT_A2DP_SNK_STOP, 1);
#endif
                btmg_cb_p[CB_MAIN]->btmg_a2dp_sink_cb.conn_state_cb(bda_str, BTMG_A2DP_SINK_DISCONNECTING);
            }
        }
        break;
    }
    case XR_A2D_AUDIO_STATE_EVT: {
        bda2str(param->audio_stat.remote_bda, bda_str);
        BTMG_INFO("a2dp sink audio state: %s", s_a2d_audio_state_str[param->audio_stat.state]);
        if (btmg_cb_p[CB_MAIN] && btmg_cb_p[CB_MAIN]->btmg_a2dp_sink_cb.audio_state_cb) {
            if (param->audio_stat.state == XR_A2D_AUDIO_STATE_STARTED) {
#ifndef CONFIG_A2DP_SINK_AUDIO_CB
                sys_handler_send(bt_audio_event_handle, BT_AUDIO_EVENT_A2DP_SNK_START, 1);
#endif
                btmg_cb_p[CB_MAIN]->btmg_a2dp_sink_cb.audio_state_cb(bda_str, BTMG_A2DP_SINK_AUDIO_STARTED);
                time_total = 0;
                data_total = 0;
            } else if (param->audio_stat.state == XR_A2D_AUDIO_STATE_STOPPED) {
#ifndef CONFIG_A2DP_SINK_AUDIO_CB
                sys_handler_send(bt_audio_event_handle, BT_AUDIO_EVENT_A2DP_SNK_STOP, 1);
#endif
                btmg_cb_p[CB_MAIN]->btmg_a2dp_sink_cb.audio_state_cb(bda_str, BTMG_A2DP_SINK_AUDIO_STOPPED);
            } else if (param->audio_stat.state == XR_A2D_AUDIO_STATE_REMOTE_SUSPEND) {
#ifndef CONFIG_A2DP_SINK_AUDIO_CB
                sys_handler_send(bt_audio_event_handle, BT_AUDIO_EVENT_A2DP_SNK_STOP, 1);
#endif
                btmg_cb_p[CB_MAIN]->btmg_a2dp_sink_cb.audio_state_cb(bda_str,
                                                            BTMG_A2DP_SINK_AUDIO_SUSPENDED);
            }
        }
        break;
    }
    case XR_A2D_AUDIO_CFG_EVT: {
        BTMG_INFO("A2DP audio stream configuration, codec type %d", param->audio_cfg.mcc.type);
        if (param->audio_cfg.mcc.type == XR_A2D_MCT_SBC) {
            pcm_config(param->audio_cfg.mcc.cie.sbc);
        }
        break;
    }
    case XR_A2D_PROF_STATE_EVT: {
        BTMG_INFO("a2dp sink %s success",
                  (param->a2d_prof_stat.init_state == XR_A2D_INIT_SUCCESS) ? "init" : "deinit");
        break;
    }
    default:
        BTMG_ERROR("unhandled evt %d", event);
        break;
    }
}

static void bt_a2dp_sink_data_cb(const uint8_t *data, uint32_t len)
{
    if (len <= 0 || data == NULL) {
        return;
    }

#ifdef CONFIG_A2DP_SINK_AUDIO_CB
    if (btmg_cb_p[CB_MAIN] && btmg_cb_p[CB_MAIN]->btmg_a2dp_sink_cb.stream_cb) {
        btmg_cb_p[CB_MAIN]->btmg_a2dp_sink_cb.stream_cb(bda_str, channels, sample_rate, data, len);
    }
    return;
#endif

    if (bt_check_debug_mask(EX_DBG_A2DP_SINK_BT_RATE)) {
        static uint64_t data_count = 0;
        int speed = 0;
        int time_ms;

        data_count += len;
        time_ms = btmg_interval_time((void *)bt_a2dp_sink_data_cb, 1000 * 10);
        if (time_ms) {
            speed = data_count * 1000 / time_ms;
            time_total += time_ms;
            data_total += data_count;
            BTMG_INFO("cache[%d] speed[%d] [now:%s] [all:%s]",
                      bt_audio_get_cache(BT_AUDIO_TYPE_A2DP_SNK), speed,
                      (speed >= second_byte)? "+":"-",
                      (data_total * 1000 >= time_total * second_byte)? "+++":"---");

            data_count = 0;
        }
    }

    bt_audio_write_unblock((uint8_t *)data, len, BT_AUDIO_TYPE_A2DP_SNK);
}

btmg_err bt_a2dp_sink_init(void)
{
    xr_a2d_register_callback(bt_a2dp_sink_cb);
    xr_a2d_sink_register_data_callback(bt_a2dp_sink_data_cb);
    xr_a2d_sink_init();
    bt_audio_init(BT_AUDIO_TYPE_A2DP_SNK, 200);

    return BT_OK;
}

btmg_err bt_a2dp_sink_connect(const char *addr)
{
    xr_err_t ret;
    xr_bd_addr_t remote_bda = {0};

    str2bda(addr, remote_bda);

    if ((ret = xr_a2d_sink_connect(remote_bda)) != XR_OK) {
        BTMG_ERROR("return failed: %d", ret);
        return BT_FAIL;
    }

    return BT_OK;
}

btmg_err bt_a2dp_sink_disconnect(const char *addr)
{
    xr_err_t ret;
    xr_bd_addr_t remote_bda = {0};

    str2bda(addr, remote_bda);

    if ((ret = xr_a2d_sink_disconnect(remote_bda)) != XR_OK) {
        BTMG_ERROR("return failed: %d", ret);
        return BT_FAIL;
    }

    return BT_OK;
}

btmg_err bt_a2dp_sink_deinit(void)
{
    if (xr_a2d_sink_deinit() != XR_OK)
        return BT_FAIL;

    bt_audio_deinit(BT_AUDIO_TYPE_A2DP_SNK);

    return BT_OK;
}
