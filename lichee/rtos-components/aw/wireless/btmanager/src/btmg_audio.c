#include "btmg_log.h"
#include "btmg_atomic.h"
#include <unistd.h>
#include <string.h>
#include <AudioSystem.h>
#include "btmg_common.h"
#include "ringbuff.h"
#include "btmg_audio.h"

XR_OS_Thread_t g_bt_audio_a2dp_sink_thread;
XR_OS_Thread_t g_bt_audio_hfp_i_thread;
XR_OS_Thread_t g_bt_audio_hfp_o_thread;

#define RING_BUFFER_INCOMING_SIZE (1024 * 64)
#define RING_BUFFER_OUTGOING_SIZE (1024 * 64)

typedef struct {
    char audio_hw[24];
    uint8_t channels;
    uint32_t rate;
    uint8_t  bits;
    uint16_t cache_time;
    uint32_t cache;
    tAudioTrack *at;
} audio_track_handle_t;

typedef struct {
    char audio_hw[24];
    uint8_t channels;
    uint32_t rate;
    uint8_t  bits;
    uint16_t  cache_time;
    uint32_t cache;
    tAudioRecord *ar;
} audio_record_handle_t;

static audio_track_handle_t a2dp_sink_track = {
    .audio_hw = "default",
    .channels = 2,
    .rate = 44100,
    .bits = 16,
    .cache_time = 0,
    .at = NULL,
};

static audio_track_handle_t hfp_hf_track = {
    .audio_hw = "default",
    .channels = 1,
    .rate = 8000,
    .bits = 16,
    .at = NULL,
};

static audio_record_handle_t hfp_hf_record = {
    .audio_hw = "default",
    .channels = 1,
    .rate = 8000,
    .bits = 16,
    .ar = NULL,
};

typedef struct _bt_audio_ {
    bt_audio_type_t type;
    ringbuf_handle_t rb_incoming; //speaker
    ringbuf_handle_t rb_outgoing; //recoder
    audio_track_handle_t *audio_track;
    audio_record_handle_t *audio_record;
    bt_atomic_t audio_state;
} bt_audio_t;

bt_audio_t *bt_audio_hfp;
bt_audio_t *bt_audio_a2dp_sink;
bt_audio_t *bt_audio_a2dp_source;

typedef enum {
    BT_AUDIO_STATE_BIT_SPEAKER_ENABLE,
    BT_AUDIO_STATE_BIT_SPEAKER_THREAD_ENABLE,
    BT_AUDIO_STATE_BIT_SPEAKER_THREAD_SUSPEND,
    BT_AUDIO_STATE_BIT_SPEAKER_THREAD_CLOSE_DONE,
    BT_AUDIO_STATE_BIT_SPEAKER_THREAD_SETTING,

    BT_AUDIO_STATE_BIT_RECODER_ENABLE,
    BT_AUDIO_STATE_BIT_RECODER_THREAD_ENABLE,
    BT_AUDIO_STATE_BIT_RECODER_THREAD_SUSPEND,
    BT_AUDIO_STATE_BIT_RECODER_THREAD_CLOSE_DONE,
    BT_AUDIO_STATE_BIT_RECODER_THREAD_SETTING,
} bt_audio_state_bit;

void bt_audio_pcm_config(uint32_t sampling, uint8_t channels, bt_audio_type_t type)
{
    if (type == BT_AUDIO_TYPE_A2DP_SNK) {
        bt_audio_a2dp_sink->audio_track->rate = sampling;
        bt_audio_a2dp_sink->audio_track->channels = channels;
        if (bt_audio_a2dp_sink->audio_track->cache_time > 0) {
            bt_audio_a2dp_sink->audio_track->cache =
                sampling * channels * 2 * bt_audio_a2dp_sink->audio_track->cache_time / 1000;
        }
    } else if (type == BT_AUDIO_TYPE_HFP) {
        bt_audio_hfp->audio_track->rate = sampling;
        bt_audio_hfp->audio_track->channels = channels;
        if (bt_audio_hfp->audio_track->cache_time > 0) {
            bt_audio_hfp->audio_track->cache =
                sampling * channels * 2 * bt_audio_hfp->audio_track->cache_time / 1000;
        }
    }
}

int bt_audio_init(bt_audio_type_t type, uint16_t cache_time)
{
    int err = -1;

    err = sys_ctrl_create();
    if (err) {
        BTMG_ERROR("sys create failed");
        return err;
    }

    if (type == BT_AUDIO_TYPE_A2DP_SNK) {
        bt_audio_a2dp_sink = (bt_audio_t *)malloc(sizeof(bt_audio_t));
        if (bt_audio_a2dp_sink == NULL) {
            return -1;
        }
        memset(bt_audio_a2dp_sink, 0, sizeof(bt_audio_t));
        bt_audio_a2dp_sink->type = type;
        bt_atomic_set_val(&bt_audio_a2dp_sink->audio_state, 0);
        bt_audio_a2dp_sink->rb_incoming = rb_create(RING_BUFFER_INCOMING_SIZE);
        bt_audio_a2dp_sink->audio_track = (audio_track_handle_t *)malloc(sizeof(audio_track_handle_t));
        if (bt_audio_a2dp_sink->audio_track == NULL) {
            goto failed;
        }
        memcpy(bt_audio_a2dp_sink->audio_track, &a2dp_sink_track, sizeof(audio_track_handle_t));
        bt_audio_a2dp_sink->audio_track->cache_time = cache_time;
    } else if (type == BT_AUDIO_TYPE_HFP) {
        bt_audio_hfp = (bt_audio_t *)malloc(sizeof(bt_audio_t));
        if (bt_audio_hfp == NULL) {
            return -1;
        }
        memset(bt_audio_hfp, 0, sizeof(bt_audio_t));
        bt_audio_hfp->type = type;
        bt_atomic_set_val(&bt_audio_hfp->audio_state, 0);
        bt_audio_hfp->rb_incoming = rb_create(RING_BUFFER_INCOMING_SIZE);
        bt_audio_hfp->audio_track = (audio_track_handle_t *)malloc(sizeof(audio_track_handle_t));
        if (bt_audio_hfp->audio_track == NULL) {
            goto failed;
        }
        memcpy(bt_audio_hfp->audio_track, &hfp_hf_track, sizeof(audio_track_handle_t));
        bt_audio_hfp->audio_track->cache_time = cache_time;

        bt_audio_hfp->rb_outgoing = rb_create(RING_BUFFER_OUTGOING_SIZE);
        bt_audio_hfp->audio_record = (audio_record_handle_t *)malloc(sizeof(audio_record_handle_t));
        if (bt_audio_hfp->audio_record == NULL) {
            goto failed;
        }
        memcpy(bt_audio_hfp->audio_record, &hfp_hf_record, sizeof(audio_record_handle_t));
        bt_audio_hfp->audio_record->cache_time = cache_time;
    } else if (type == BT_AUDIO_TYPE_A2DP_SRC) {
        bt_audio_a2dp_source = (bt_audio_t *)malloc(sizeof(bt_audio_t));
        if (bt_audio_a2dp_source == NULL) {
            return -1;
        }
        memset(bt_audio_a2dp_source, 0, sizeof(bt_audio_t));
        bt_audio_a2dp_source->type = type;
        bt_audio_a2dp_source->rb_outgoing = rb_create(RING_BUFFER_OUTGOING_SIZE);
    }

    return 0;

failed:
    if (type == BT_AUDIO_TYPE_A2DP_SNK) {
        if (bt_audio_a2dp_sink != NULL) {
            free(bt_audio_a2dp_sink);
            bt_audio_a2dp_sink = NULL;
        }
    } else if (type == BT_AUDIO_TYPE_HFP) {
        if (bt_audio_hfp != NULL) {
            free(bt_audio_hfp);
            bt_audio_hfp = NULL;
        }
    } else if (type == BT_AUDIO_TYPE_A2DP_SRC) {
        if (bt_audio_a2dp_source != NULL) {
            free(bt_audio_a2dp_source);
            bt_audio_a2dp_source = NULL;
        }
    }
    return -1;
}

int bt_audio_deinit(bt_audio_type_t type)
{
    int ret = -1;

    if (type == BT_AUDIO_TYPE_A2DP_SNK) {
        bt_atomic_set_val(&bt_audio_a2dp_sink->audio_state, 0);
        ret = rb_destroy(bt_audio_a2dp_sink->rb_incoming);
        bt_audio_a2dp_sink->rb_incoming = NULL;
        if (bt_audio_a2dp_sink->audio_track != NULL) {
            free(bt_audio_a2dp_sink->audio_track);
            bt_audio_a2dp_sink->audio_track = NULL;
        }
        if (bt_audio_a2dp_sink != NULL) {
            free(bt_audio_a2dp_sink);
            bt_audio_a2dp_sink = NULL;
        }
    } else if (type == BT_AUDIO_TYPE_HFP) {
        bt_atomic_set_val(&bt_audio_hfp->audio_state, 0);
        ret = rb_destroy(bt_audio_hfp->rb_incoming);
        bt_audio_hfp->rb_incoming = NULL;
        if (bt_audio_hfp->audio_track != NULL) {
            free(bt_audio_hfp->audio_track);
            bt_audio_hfp->audio_track = NULL;
        }
        ret = rb_destroy(bt_audio_hfp->rb_outgoing);
        bt_audio_hfp->rb_outgoing = NULL;
        if (bt_audio_hfp->audio_record != NULL) {
            free(bt_audio_hfp->audio_record);
            bt_audio_hfp->audio_record = NULL;
        }
        if (bt_audio_hfp != NULL) {
            free(bt_audio_hfp);
            bt_audio_hfp = NULL;
        }
    } else if (type == BT_AUDIO_TYPE_A2DP_SRC) {
        ret = rb_destroy(bt_audio_a2dp_source->rb_outgoing);
        bt_audio_a2dp_source->rb_outgoing = NULL;
        if (bt_audio_a2dp_source != NULL) {
            free(bt_audio_a2dp_source);
            bt_audio_a2dp_source = NULL;
        }
    }

    return ret;
}

static int bt_audio_pcm_write(bt_audio_t *param, const uint8_t *data, uint32_t len)
{
    int ret = 0;

    if (param->audio_track->at == NULL) {
        BTMG_ERROR("audio_track is null");
        return -1;
    }
    ret = AudioTrackWrite(param->audio_track->at, (void *)data, len);
    if (ret < 0) {
        BTMG_ERROR("pcm write error:%d", ret);
    }
    return ret;
}

static int bt_audio_pcm_read(bt_audio_t *param, uint8_t *data, uint32_t len)
{
    int ret = 0;

    if (param->audio_record->ar == NULL) {
        BTMG_ERROR("audio record handle is null");
        return -1;
    }

    ret = AudioRecordRead(param->audio_record->ar, (void *)data, len);
    if (ret < 0) {
        BTMG_ERROR("pcm_read error:%d", ret);
    }

    return ret;
}

static void bt_audio_incoming_thread(void *_param) //speaker
{
    int ret;
    int data_cached = 0;
    uint64_t time_total = 0;
    uint64_t data_total = 0;
    char incoming_buffer[512];
    bt_audio_t *param = (bt_audio_t *)_param;
    int second_byte = param->audio_track->rate * param->audio_track->channels * 2;

    if (param == NULL || param->audio_track == NULL) {
        return;
    }
    BTMG_INFO("sample_rate: %d", param->audio_track->rate);
    BTMG_INFO("channels: %d", param->audio_track->channels);
    BTMG_INFO("cache time: %d ms", param->audio_track->cache_time);

    param->audio_track->at = AudioTrackCreate(param->audio_track->audio_hw);

    if (!param->audio_track->at) {
        BTMG_ERROR("audio_track create failed");
        goto failed1;
    }

    AudioTrackResampleCtrl(param->audio_track->at, 1);

    bt_atomic_set_bit(&param->audio_state, BT_AUDIO_STATE_BIT_SPEAKER_THREAD_SETTING);
    bt_atomic_set_bit(&param->audio_state, BT_AUDIO_STATE_BIT_SPEAKER_ENABLE);

    while (bt_atomic_test_bit(&param->audio_state, BT_AUDIO_STATE_BIT_SPEAKER_THREAD_ENABLE)) {
        if (bt_atomic_test_bit(&param->audio_state, BT_AUDIO_STATE_BIT_SPEAKER_THREAD_SUSPEND)) {
            usleep(1000);
            continue;
        }
        if (bt_atomic_test_bit(&param->audio_state, BT_AUDIO_STATE_BIT_SPEAKER_THREAD_SETTING)) {
            ret = AudioTrackSetup(param->audio_track->at,
                                  param->audio_track->rate,
                                  param->audio_track->channels,
                                  param->audio_track->bits);
            usleep(1000 * 200);
            if (ret < 0) {
                BTMG_ERROR("Audio Track Setup failed");
                goto failed1;
            }
            bt_atomic_clear_bit(&param->audio_state, BT_AUDIO_STATE_BIT_SPEAKER_THREAD_SETTING);
            continue;
        }
        if (data_cached == 0) {
            if (bt_audio_get_cache(param->type) < param->audio_track->cache) {
                usleep(1000);
                continue;
            }
            data_cached = 1;
            BTMG_INFO("Actual Cache size :%d Byte", bt_audio_get_cache(param->type));
        }
        //if no data,block 200ms here
        ret = bt_audio_read(incoming_buffer, 512, 200, param->type);
        if (ret > 0) {
            int write = bt_audio_pcm_write(param, (uint8_t *)incoming_buffer, ret);
            if (bt_check_debug_mask(EX_DBG_A2DP_SINK_AUDIO_WRITE_RATE)) {
                 static uint64_t data_count = 0;
                 int speed = 0;
                 int time_ms;

                 data_count += write;
                 time_ms = btmg_interval_time((void *)bt_audio_pcm_write, 1000 * 10);

                 if (time_ms) {
                    speed = data_count * 1000 / time_ms;
                    time_total += time_ms;
                    data_total += data_count;
                    BTMG_INFO("cache[%d] speed[%d] [now:%s] [all:%s]",
                               bt_audio_get_cache(param->type), speed,
                               (speed >= second_byte)? "+":"-",
                               ((data_total * 1000 >= time_total * second_byte)? "+++":"---"));
                    data_count = 0;
                 }
            }
        }
    }

failed1:
    ret = AudioTrackDestroy(param->audio_track->at);
failed2:
    param->audio_track->at = NULL;
    param->audio_track->cache = 0;
    if (ret < 0) {
        BTMG_ERROR("audio close error:%d", ret);
    }
    BTMG_INFO("TaskDelete");
    bt_atomic_set_bit(&param->audio_state, BT_AUDIO_STATE_BIT_SPEAKER_THREAD_CLOSE_DONE);
    XR_OS_ThreadDelete(NULL);

    return;
}

static void bt_audio_outgoing_thread(void *_param) //microphone
{
    int ret = 0;
    char outgoing_buffer[512];
    bt_audio_t *param = (bt_audio_t *)_param;

    if (param == NULL || param->audio_record == NULL) {
        return;
    }

    param->audio_record->ar = AudioRecordCreate(param->audio_record->audio_hw);
    if (!param->audio_record->ar) {
        BTMG_ERROR("audio_record create failed");
        goto failed1;
    }

    bt_atomic_set_bit(&param->audio_state, BT_AUDIO_STATE_BIT_RECODER_THREAD_SETTING);
    bt_atomic_set_bit(&param->audio_state, BT_AUDIO_STATE_BIT_RECODER_ENABLE);
    while (bt_atomic_test_bit(&param->audio_state, BT_AUDIO_STATE_BIT_RECODER_THREAD_ENABLE)) {
        if (bt_atomic_test_bit(&param->audio_state, BT_AUDIO_STATE_BIT_RECODER_THREAD_SUSPEND)) {
            usleep(1000);
            continue;
        }
        if (bt_atomic_test_bit(&param->audio_state, BT_AUDIO_STATE_BIT_RECODER_THREAD_SETTING)) {
            ret = AudioRecordSetup(param->audio_record->ar,
                                   param->audio_record->rate,
                                   param->audio_record->channels,
                                   param->audio_record->bits);

            if (ret < 0) {
                BTMG_ERROR("Audio Track Record failed");
                goto failed1;
            }
            bt_atomic_clear_bit(&param->audio_state, BT_AUDIO_STATE_BIT_RECODER_THREAD_SETTING);
            continue;
        }
        //block time = 256 / 8000 * 2;
        ret = bt_audio_pcm_read(param, outgoing_buffer, 512);
        if (ret > 0)
            bt_audio_write(outgoing_buffer, ret, 25, param->type);
    }

failed1:
    ret = AudioRecordDestroy(param->audio_record->ar);
    param->audio_record->ar = NULL;
    if (ret < 0) {
        BTMG_ERROR("audio close error:%d", ret);
    }
    BTMG_INFO("TaskDelete");
    bt_atomic_set_bit(&param->audio_state, BT_AUDIO_STATE_BIT_RECODER_THREAD_CLOSE_DONE);
    XR_OS_ThreadDelete(NULL);

    return;
}

static int bt_audio_start(bt_audio_type_t type)
{
    if (type == BT_AUDIO_TYPE_A2DP_SNK) {
        memset(&g_bt_audio_a2dp_sink_thread, 0, sizeof(g_bt_audio_a2dp_sink_thread));
        bt_audio_reset(type);
        bt_atomic_set_bit(&bt_audio_a2dp_sink->audio_state,
                          BT_AUDIO_STATE_BIT_SPEAKER_THREAD_ENABLE);
        XR_OS_ThreadCreate(&g_bt_audio_a2dp_sink_thread, "a2dp_sink_t", bt_audio_incoming_thread,
                           bt_audio_a2dp_sink, 4, 1024 * 8);
    } else if (type == BT_AUDIO_TYPE_HFP) {
        bt_atomic_set_bit(&bt_audio_hfp->audio_state, BT_AUDIO_STATE_BIT_SPEAKER_THREAD_ENABLE);
        bt_atomic_set_bit(&bt_audio_hfp->audio_state, BT_AUDIO_STATE_BIT_RECODER_THREAD_ENABLE);
        memset(&g_bt_audio_hfp_i_thread, 0, sizeof(g_bt_audio_hfp_i_thread));
        memset(&g_bt_audio_hfp_o_thread, 0, sizeof(g_bt_audio_hfp_o_thread));
        bt_audio_reset(type);
        XR_OS_ThreadCreate(&g_bt_audio_hfp_i_thread, "hfp_i_t", bt_audio_incoming_thread,
                           bt_audio_hfp, 2, 1024 * 6);
        usleep(1000 * 20);
        XR_OS_ThreadCreate(&g_bt_audio_hfp_o_thread, "hfp_o_t", bt_audio_outgoing_thread,
                           bt_audio_hfp, 2, 1024 * 6);
    }

    return 0;
}

static int bt_audio_stop(bt_audio_t *param)
{
    int speaker_thread_enable = 0;
    int recoder_thread_enable = 0;

    if (bt_atomic_test_bit(&param->audio_state, BT_AUDIO_STATE_BIT_SPEAKER_THREAD_ENABLE)) {
        speaker_thread_enable = 1;
        bt_atomic_clear_bit(&param->audio_state, BT_AUDIO_STATE_BIT_SPEAKER_THREAD_ENABLE);
    }

    if (bt_atomic_test_bit(&param->audio_state, BT_AUDIO_STATE_BIT_RECODER_THREAD_ENABLE)) {
        recoder_thread_enable = 1;
        bt_atomic_clear_bit(&param->audio_state, BT_AUDIO_STATE_BIT_RECODER_THREAD_ENABLE);
    }

    if (speaker_thread_enable) {
        while (!bt_atomic_test_bit(&param->audio_state,
                                   BT_AUDIO_STATE_BIT_SPEAKER_THREAD_CLOSE_DONE)) {
            usleep(1000);
        }
    }

    if (recoder_thread_enable) {
        while (!bt_atomic_test_bit(&param->audio_state,
                                   BT_AUDIO_STATE_BIT_RECODER_THREAD_CLOSE_DONE)) {
            usleep(1000);
        }
    }

    return 0;
}

void bt_audio_event_handle(event_msg *msg)
{
    int ret = -1;

    switch (msg->data) {
    case BT_AUDIO_EVENT_A2DP_SNK_START:
        bt_audio_start(BT_AUDIO_TYPE_A2DP_SNK);
        BTMG_DEBUG("start a2dp sink audio");
        break;
    case BT_AUDIO_EVENT_A2DP_SNK_STOP:
        bt_audio_stop(bt_audio_a2dp_sink);
        BTMG_DEBUG("stop a2dp sink audio");
        break;
    case BT_AUDIO_EVENT_HFP_START:
        bt_audio_start(BT_AUDIO_TYPE_HFP);
        BTMG_DEBUG("start hfp audio");
        break;
    case BT_AUDIO_EVENT_HFP_STOP:
        bt_audio_stop(bt_audio_hfp);
        BTMG_DEBUG("stop hfp audio");
        break;
    default:
        BTMG_ERROR("bt audio request err");
        break;
    }
}

int bt_audio_write(uint8_t *data, uint32_t len, uint32_t timeout, bt_audio_type_t type)
{
    if (type == BT_AUDIO_TYPE_A2DP_SRC && bt_audio_a2dp_source != NULL) {
        return rb_write(bt_audio_a2dp_source->rb_outgoing, data, len, timeout);
    } else if (type == BT_AUDIO_TYPE_HFP && bt_audio_hfp != NULL) {
        if (bt_atomic_test_bit(&bt_audio_hfp->audio_state, BT_AUDIO_STATE_BIT_SPEAKER_ENABLE)) {
            return rb_write(bt_audio_hfp->rb_outgoing, data, len, timeout);
        }
    }

    return -1;
}

int bt_audio_read(uint8_t *data, uint32_t len, uint32_t timeout, bt_audio_type_t type)
{

    if (type == BT_AUDIO_TYPE_A2DP_SNK && bt_audio_a2dp_sink != NULL) {
        if (bt_atomic_test_bit(&bt_audio_a2dp_sink->audio_state,
                               BT_AUDIO_STATE_BIT_SPEAKER_ENABLE)) {
            return rb_read(bt_audio_a2dp_sink->rb_incoming, data, len, timeout);
        }
    } else if (type == BT_AUDIO_TYPE_HFP && bt_audio_hfp != NULL) {
        if (bt_atomic_test_bit(&bt_audio_hfp->audio_state, BT_AUDIO_STATE_BIT_SPEAKER_ENABLE)) {
            return rb_read(bt_audio_hfp->rb_incoming, data, len, timeout);
        }
    }

    return -1;
}

int bt_audio_write_unblock(uint8_t *data, uint32_t len, bt_audio_type_t type)
{
    if (type == BT_AUDIO_TYPE_A2DP_SNK && bt_audio_a2dp_sink != NULL) {
        if (bt_atomic_test_bit(&bt_audio_a2dp_sink->audio_state,
                               BT_AUDIO_STATE_BIT_SPEAKER_ENABLE)) {
            return rb_unblock_write(bt_audio_a2dp_sink->rb_incoming, data, len);
        }
    } else if (type == BT_AUDIO_TYPE_HFP && bt_audio_hfp != NULL) {
        if (bt_atomic_test_bit(&bt_audio_hfp->audio_state, BT_AUDIO_STATE_BIT_SPEAKER_ENABLE)) {
            return rb_unblock_write(bt_audio_hfp->rb_incoming, data, len);
        }
    }

    return -1;
}

int bt_audio_read_unblock(uint8_t *data, uint32_t len, bt_audio_type_t type)
{
    if (type == BT_AUDIO_TYPE_HFP && bt_audio_hfp != NULL) {
        if (bt_atomic_test_bit(&bt_audio_hfp->audio_state, BT_AUDIO_STATE_BIT_RECODER_ENABLE)) {
            return rb_unblock_read(bt_audio_hfp->rb_outgoing, data, len);
        }
    } else if (type == BT_AUDIO_TYPE_A2DP_SRC && bt_audio_a2dp_source != NULL) {
        return rb_unblock_read(bt_audio_a2dp_source->rb_outgoing, data, len);
    }

    return -1;
}

int bt_audio_reset(bt_audio_type_t type)
{
    if (type == BT_AUDIO_TYPE_A2DP_SRC) {
        return rb_reset(bt_audio_a2dp_source->rb_outgoing);
    } else if (type == BT_AUDIO_TYPE_A2DP_SNK) {
        return rb_reset(bt_audio_a2dp_sink->rb_incoming);
    } else if (type == BT_AUDIO_TYPE_HFP) {
        rb_reset(bt_audio_hfp->rb_incoming);
        rb_reset(bt_audio_hfp->rb_outgoing);
        return 0;
    }

    return -1;
}

int bt_audio_get_cache(bt_audio_type_t type)
{
    if (type == BT_AUDIO_TYPE_A2DP_SRC) {
        return rb_get_filled_bytes(bt_audio_a2dp_source->rb_outgoing);
    } else if (type == BT_AUDIO_TYPE_A2DP_SNK) {
        return rb_get_filled_bytes(bt_audio_a2dp_sink->rb_incoming);
    }

    return -1;
}
