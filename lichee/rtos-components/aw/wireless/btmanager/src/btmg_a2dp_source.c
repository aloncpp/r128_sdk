#include <stdbool.h>

#include "xr_a2dp_api.h"
#include "xr_gap_bt_api.h"
#include "btmg_log.h"
#include "btmg_common.h"
#include "btmg_audio.h"
#include "bt_manager.h"
#include "btmg_common.h"

#include "kernel/os/os_time.h"
#include "speex_resampler.h"

/* sub states of BT_AV_STATE_CONNECTED */
enum {
    BT_AV_MEDIA_STATE_IDLE,
    BT_AV_MEDIA_STATE_STARTING,
    BT_AV_MEDIA_STATE_STARTED,
    BT_AV_MEDIA_STATE_SUSPEND,
    BT_AV_MEDIA_STATE_STOPPING,
    BT_AV_MEDIA_STATE_STOPPED,
};

static bool is_play = false;
static bool is_init_successed = false;
static bool is_device_connected = false;
static int s_media_state = BT_AV_MEDIA_STATE_IDLE;
static const char *s_a2d_conn_state_str[] = { "Disconnected", "Connecting", "Connected",
                                              "Disconnecting" };
static const char *s_a2d_audio_state_str[] = { "Suspended", "Stopped", "Started" };

uint8_t src_pcm_channels;
uint32_t src_pcm_sampling;
uint8_t dst_pcm_channels;
uint32_t dst_pcm_sampling;
int st_quality;

SpeexResamplerState *st;

static uint8_t frame_bytes;
static int16_t *out_data = NULL;
static uint32_t out_frames = 4096;

static int32_t bt_a2dp_source_data_cb(uint8_t *data, int32_t len)
{
    int ret = 0;

    if (len <= 0 || data == NULL) {
        return 0;
    }

#ifdef CONFIG_A2DP_USE_AUDIO_SYSTEM
    if (btmg_cb_p[CB_MINOR] && btmg_cb_p[CB_MINOR]->btmg_a2dp_source_cb.audio_data_cb) {
        return btmg_cb_p[CB_MINOR]->btmg_a2dp_source_cb.audio_data_cb(data, len);
    }
#else
    ret = bt_audio_read_unblock(data, len, BT_AUDIO_TYPE_A2DP_SRC);
    if (bt_check_debug_mask(EX_DBG_A2DP_SOURCE_BT_RATE)) {
        static uint64_t data_count = 0;
        int speed = 0;
        int time_ms;

        data_count += ret;
        time_ms = btmg_interval_time((void *)bt_a2dp_source_data_cb, 1000 * 10);
        if (time_ms) {
            speed = data_count * 1000 / time_ms;
            BTMG_INFO("time_ms[%d] cache[%d] speed[%d]",
                       time_ms, bt_audio_get_cache(BT_AUDIO_TYPE_A2DP_SRC), speed);
            data_count = 0;
        }
    }
    if (ret < 0) {
        return 0;
    }
    return ret;
#endif
}

static void bt_a2dp_source_cb(xr_a2d_cb_event_t event, xr_a2d_cb_param_t *param)
{
    dev_node_t *dev_node = NULL;

    switch (event) {
    case XR_A2D_CONNECTION_STATE_EVT: {
        bda2str(param->conn_stat.remote_bda, bda_str);
        BTMG_DEBUG("a2dp source %s, dev:%s", s_a2d_conn_state_str[param->conn_stat.state], bda_str);
        if (param->conn_stat.state == XR_A2D_CONNECTION_STATE_CONNECTED) {
            is_device_connected = true;
            dev_node = btmg_dev_list_find_device(connected_devices, bda_str);
            if (dev_node == NULL) {
                btmg_dev_list_add_device(connected_devices, NULL, bda_str, A2DP_SRC_DEV);
            } else {
                dev_node->profile |= A2DP_SRC_DEV;
            }
            BTMG_DEBUG("add device %s into connected_devices", bda_str);
        }

        if (param->conn_stat.state == XR_A2D_CONNECTION_STATE_DISCONNECTED) {
            if (connected_devices->sem_flag) {
                XR_OS_SemaphoreRelease(&(connected_devices->sem));
            }
            is_device_connected = false;
            dev_node = btmg_dev_list_find_device(connected_devices, bda_str);
            if (dev_node != NULL) {
                BTMG_DEBUG("remove device %s from connected_devices", bda_str);
                dev_node->profile &= ~A2DP_SRC_DEV;
                btmg_dev_list_remove_device(connected_devices, bda_str);
            }
        }

        if (btmg_cb_p[CB_MAIN] && btmg_cb_p[CB_MAIN]->btmg_a2dp_source_cb.conn_state_cb) {
            if (param->conn_stat.state == XR_A2D_CONNECTION_STATE_DISCONNECTED) {
                btmg_cb_p[CB_MAIN]->btmg_a2dp_source_cb.conn_state_cb(bda_str,
                                                             BTMG_A2DP_SOURCE_DISCONNECTED);
            } else if (param->conn_stat.state == XR_A2D_CONNECTION_STATE_CONNECTING) {
                btmg_cb_p[CB_MAIN]->btmg_a2dp_source_cb.conn_state_cb(bda_str, BTMG_A2DP_SOURCE_CONNECTING);
            } else if (param->conn_stat.state == XR_A2D_CONNECTION_STATE_CONNECTED) {
                btmg_cb_p[CB_MAIN]->btmg_a2dp_source_cb.conn_state_cb(bda_str, BTMG_A2DP_SOURCE_CONNECTED);
            } else if (param->conn_stat.state == XR_A2D_CONNECTION_STATE_DISCONNECTING) {
                btmg_cb_p[CB_MAIN]->btmg_a2dp_source_cb.conn_state_cb(bda_str,
                                                             BTMG_A2DP_SOURCE_DISCONNECTING);
            }
        }
#ifdef CONFIG_A2DP_USE_AUDIO_SYSTEM
        if (btmg_cb_p[CB_MINOR] && btmg_cb_p[CB_MINOR]->btmg_a2dp_source_cb.conn_state_cb) {
            if (param->conn_stat.state == XR_A2D_CONNECTION_STATE_DISCONNECTED) {
                btmg_cb_p[CB_MINOR]->btmg_a2dp_source_cb.conn_state_cb(bda_str,
                                                             BTMG_A2DP_SOURCE_DISCONNECTED);
            } else if (param->conn_stat.state == XR_A2D_CONNECTION_STATE_CONNECTED) {
                btmg_cb_p[CB_MINOR]->btmg_a2dp_source_cb.conn_state_cb(bda_str, BTMG_A2DP_SOURCE_CONNECTED);
            }
        }
#endif
        break;
    }
    case XR_A2D_AUDIO_STATE_EVT: {
        bda2str(param->audio_stat.remote_bda, bda_str);
        if (btmg_cb_p[CB_MAIN] && btmg_cb_p[CB_MAIN]->btmg_a2dp_source_cb.audio_state_cb) {
            if (param->conn_stat.state == XR_A2D_AUDIO_STATE_REMOTE_SUSPEND) {
                BTMG_DEBUG("AUDIO_SUSPENDED");
                btmg_cb_p[CB_MAIN]->btmg_a2dp_source_cb.audio_state_cb(bda_str, BTMG_A2DP_SOURCE_AUDIO_SUSPENDED);
            } else if (param->conn_stat.state == XR_A2D_AUDIO_STATE_STOPPED) {
                BTMG_DEBUG("AUDIO_STOPPED");
                btmg_cb_p[CB_MAIN]->btmg_a2dp_source_cb.audio_state_cb(bda_str, BTMG_A2DP_SOURCE_AUDIO_STOPPED);
            } else if (param->conn_stat.state == XR_A2D_AUDIO_STATE_STARTED) {
                BTMG_DEBUG("AUDIO_STARTED");
                btmg_cb_p[CB_MAIN]->btmg_a2dp_source_cb.audio_state_cb(bda_str, BTMG_A2DP_SOURCE_AUDIO_STARTED);
            }
        }
        break;
    }
    case XR_A2D_PROF_STATE_EVT:
        BTMG_DEBUG("a2dp source %s success",
                  (param->a2d_prof_stat.init_state == XR_A2D_INIT_SUCCESS) ? "init" : "deinit");
        if (param->a2d_prof_stat.init_state == XR_A2D_INIT_SUCCESS) {
            is_init_successed = true;
        }
        break;
    case XR_A2D_MEDIA_CTRL_ACK_EVT:
        BTMG_DEBUG("a2dp media ctrl ack");
        break;
    default:
        BTMG_ERROR("unhandled evt %d", event);
        break;
    }
}

btmg_err bt_a2dp_source_init(void)
{
    xr_err_t ret;

    is_init_successed = false;
    is_device_connected = false;

    BTMG_DEBUG("");

    if ((ret = xr_a2d_register_callback(&bt_a2dp_source_cb)) != XR_OK) {
        BTMG_ERROR("a2d_source_reg_cb return failed: %d", ret);
        return BT_FAIL;
    }

    if ((ret = xr_a2d_source_register_data_callback(bt_a2dp_source_data_cb)) != XR_OK) {
        BTMG_ERROR("a2d_source_reg_data_cb return failed: %d", ret);
        return BT_FAIL;
    }

    if ((ret = xr_a2d_source_init()) != XR_OK) {
        BTMG_ERROR("return failed: %d", ret);
        return BT_FAIL;
    }

    src_pcm_channels = 2;
    src_pcm_sampling = 16000;
    dst_pcm_channels = 2;
    dst_pcm_sampling = 44100;
    st_quality = 3;

    bt_audio_init(BT_AUDIO_TYPE_A2DP_SRC, 0);

    if (out_data == NULL) {
        out_data = calloc(4096, sizeof(int16_t));
    }

    return BT_OK;
}

btmg_err bt_a2dp_source_deinit(void)
{
    xr_err_t ret;

    BTMG_DEBUG("");

    if ((ret = xr_a2d_source_deinit()) != XR_OK) {
        BTMG_ERROR("return failed: %d", ret);
        return BT_FAIL;
    }

    bt_audio_deinit(BT_AUDIO_TYPE_A2DP_SRC);

    if (st != NULL) {
        speex_resampler_destroy(st);
        st = NULL;
    }

    if (out_data != NULL) {
        free(out_data);
        out_data = NULL;
    }

    return BT_OK;
}

btmg_err bt_a2dp_source_connect(const char *addr)
{
    xr_err_t ret;
    xr_bd_addr_t remote_bda = {0};

    str2bda(addr, remote_bda);

    if ((ret = xr_a2d_source_connect(remote_bda)) != XR_OK) {
        BTMG_ERROR("return failed: %d", ret);
        return BT_FAIL;
    }

    return BT_OK;
}

btmg_err bt_a2dp_source_disconnect(const char *addr)
{
    xr_err_t ret;
    xr_bd_addr_t remote_bda = {0};

    str2bda(addr, remote_bda);

    if ((ret = xr_a2d_source_disconnect(remote_bda)) != XR_OK) {
        BTMG_ERROR("return failed: %d", ret);
        return BT_FAIL;
    }

    return BT_OK;
}

btmg_err bt_a2dp_source_set_audio_param(uint8_t channels, uint32_t sampling)
{
    int err;

    src_pcm_channels = channels;
    src_pcm_sampling = sampling;
    frame_bytes = src_pcm_channels * 2;

    BTMG_DEBUG("");

    if (st != NULL) {
        speex_resampler_destroy(st);
        st = NULL;
    }

    st = speex_resampler_init_frac(dst_pcm_channels,
                src_pcm_sampling, dst_pcm_sampling,
                src_pcm_sampling, dst_pcm_sampling,
                st_quality, &err);

    if (!st || err != 0) {
        BTMG_ERROR("resample fail,st:%p, ret:%d", st, err);
        return BT_FAIL;
    }

    speex_resampler_set_rate_frac(st,
            src_pcm_sampling, dst_pcm_sampling,
            src_pcm_sampling, dst_pcm_sampling);

    return BT_OK;
}

int bt_a2dp_source_send_data(uint8_t *data, int len)
{
    int ret = 0;
    int in_frames = 0;
    int out_data_len = 0;
    out_frames = 4096;

    if (!is_device_connected) {
        BTMG_ERROR("Device disconnected, send fail!");
        return 0;
    }
    if (bt_check_debug_mask(EX_DBG_A2DP_SOURCE_APP_WRITE_RATE)) {
        static uint64_t data_count = 0;
        int speed = 0;
        int time_ms;

        data_count += len;
        time_ms = btmg_interval_time((void *)bt_a2dp_source_send_data, 1000 * 10);
        if (time_ms) {
            speed = data_count * 1000 / time_ms;
            BTMG_INFO("time_ms[%d] cache[%d] speed[%d]",
                       time_ms, bt_audio_get_cache(BT_AUDIO_TYPE_A2DP_SRC), speed);
            data_count = 0;
        }
    }

    in_frames = len / frame_bytes;
    ret = speex_resampler_process_interleaved_int(st, (int16_t *)data, &in_frames,
                                        out_data, &out_frames);
    if (ret != 0) {
        BTMG_ERROR("resample error, ret=%d", ret);
    }
    out_data_len = out_frames * dst_pcm_channels * 2;

    ret = bt_audio_write((uint8_t *)out_data, out_data_len, 200, BT_AUDIO_TYPE_A2DP_SRC);

    return ret > 0 ? ret: 0;
}

btmg_err bt_a2dp_source_play_start(void)
{
    xr_err_t ret;

    BTMG_DEBUG("");

#ifndef CONFIG_A2DP_USE_AUDIO_SYSTEM
    bt_audio_reset(BT_AUDIO_TYPE_A2DP_SRC);
    is_play = true;
#endif

    if ((ret = xr_a2d_media_ctrl(XR_A2D_MEDIA_CTRL_START)) != XR_OK) {
        BTMG_ERROR("return failed: %d", ret);
        return BT_FAIL;
    }

    return BT_OK;
}

btmg_err bt_a2dp_source_play_stop(bool drop)
{
    xr_err_t ret;

    BTMG_DEBUG("");

#ifndef CONFIG_A2DP_USE_AUDIO_SYSTEM
    is_play = false;
    if (drop == false) {
        while (bt_audio_get_cache(BT_AUDIO_TYPE_A2DP_SRC) != 0)
            XR_OS_MSleep(10);
    }
#endif

    if ((ret = xr_a2d_media_ctrl(XR_A2D_MEDIA_CTRL_STOP)) != XR_OK) {
        BTMG_ERROR("return failed: %d", ret);
        return BT_FAIL;
    }

    return BT_OK;
}

bool bt_a2dp_source_is_ready(void)
{
    if (is_init_successed && is_device_connected) {
        return true;
    }

    return false;
}
