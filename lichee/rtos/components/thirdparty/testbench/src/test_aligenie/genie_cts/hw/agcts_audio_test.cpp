/**
 * Copyright (C) 2018 Alibaba.inc, All rights reserved.
 *
 * @file:    agcts_audio_test.cpp
 * @brief:
 * @author:  wangxiaowei.wxw@alibaba-inc.com
 * @date:    2019/7/26
 * @version: 1.0
 */
#include <stdbool.h>
#include "agcts.h"
#include <aw-alsa-lib/pcm.h>

const unsigned char pcm_data[] = {
#include "agcts_audio_test_pcm_48K_mono_16bit.h"
};


typedef enum
{
    FORMAT_INVALID= -1,
    FORMAT_UNSPECIFIED = 0,
    FORMAT_PCM_S16,
    FORMAT_PCM_S32
} SampleFormat;

typedef struct {
    snd_pcm_t *handle;
    snd_pcm_format_t format;
    unsigned int rate;
    unsigned int channels;
    snd_pcm_uframes_t period_size;
    snd_pcm_uframes_t buffer_size;

    snd_pcm_uframes_t frame_bytes;
    snd_pcm_uframes_t chunk_size;

} aw_audio_mgr_t;

static aw_audio_mgr_t g_audio_mgr = {0};

static snd_pcm_format_t aw_snd_format_convert(int32_t format)
{
    snd_pcm_format_t f = SND_PCM_FORMAT_UNKNOWN;
    switch (format) {
    case FORMAT_PCM_S16:
        f = SND_PCM_FORMAT_S16_LE;
        break;
    case FORMAT_PCM_S32:
        f = SND_PCM_FORMAT_S32_LE;
        break;
    default:
        printf("unknown format\n");
        break;
    }
    return f;
}

static void xrun(snd_pcm_t *handle)
{
    int ret;
    printf("Xrun...\n");
    ret = snd_pcm_prepare(handle);
    if (ret < 0) {
        printf("prepare failed in xrun. return %d\n", ret);
    }
}

/**
 * @brief Create audio track
 * @param[in] sampleRate Audio pcm sample rate
 * @param[in] channelCount Audio pcm channel count
 * @param[in] format Audio pcm format
 * @return int32_t 0 : success; other: fail
 */
int32_t open(int32_t sampleRate, int32_t channelCount, int32_t format)
{
    int32_t ret=0;
    snd_pcm_hw_params_t *params;
    snd_pcm_sw_params_t *sw_params;

    if (g_audio_mgr.handle != NULL) {
        printf("g_audio_mgr handle not NULL\n");
        return -1;
    }

    ret = snd_pcm_open(&g_audio_mgr.handle, "audiocodec", SND_PCM_STREAM_PLAYBACK, 0);
    if (ret < 0) {
        printf("snd_pcm_open failed:%d\n", ret);
        return ret;
    }

    g_audio_mgr.format = aw_snd_format_convert(format);
    g_audio_mgr.rate = sampleRate;
    g_audio_mgr.channels = channelCount;
    g_audio_mgr.period_size = 2048;
    g_audio_mgr.buffer_size = 8192;

    /* HW params */
	snd_pcm_hw_params_alloca(&params);
	ret =  snd_pcm_hw_params_any(g_audio_mgr.handle, params);
	if (ret < 0) {
		printf("no configurations available\n");
		goto err;
	}
	snd_pcm_hw_params_set_access(g_audio_mgr.handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
	snd_pcm_hw_params_set_format(g_audio_mgr.handle, params, aw_snd_format_convert(format));
	snd_pcm_hw_params_set_channels(g_audio_mgr.handle, params, channelCount);
	snd_pcm_hw_params_set_rate(g_audio_mgr.handle, params, g_audio_mgr.rate, 0);
	snd_pcm_hw_params_set_period_size(g_audio_mgr.handle, params, g_audio_mgr.period_size, 0);
	snd_pcm_hw_params_set_buffer_size(g_audio_mgr.handle, params, g_audio_mgr.buffer_size);
	ret = snd_pcm_hw_params(g_audio_mgr.handle, params);
	if (ret < 0) {
		printf("Unable to install hw prams!\n");
		goto err;
	}

	/* SW params */
	snd_pcm_sw_params_alloca(&sw_params);
	snd_pcm_sw_params_current(g_audio_mgr.handle, sw_params);
	snd_pcm_sw_params_set_start_threshold(g_audio_mgr.handle, sw_params, g_audio_mgr.buffer_size);
	snd_pcm_sw_params_set_stop_threshold(g_audio_mgr.handle, sw_params, g_audio_mgr.buffer_size);
	snd_pcm_sw_params_set_avail_min(g_audio_mgr.handle, sw_params, g_audio_mgr.period_size);
	ret = snd_pcm_sw_params(g_audio_mgr.handle ,sw_params);
	if (ret < 0) {
		printf("Unable to install sw prams!\n");
		goto err;
	}

    g_audio_mgr.frame_bytes = snd_pcm_frames_to_bytes(g_audio_mgr.handle, 1);
    g_audio_mgr.chunk_size = g_audio_mgr.period_size;
    //g_audio_mgr.chunk_size = 400;

    return ret;
err:
    snd_pcm_close(g_audio_mgr.handle);
    g_audio_mgr.handle = NULL;
    return ret;
}

/**
 * @brief Start audio track
 * @return int32_t 0 : success; other: fail
 */
int32_t start()
{
    int32_t ret=0;

    //ret = snd_pcm_prepare(g_audio_mgr.handle);
    if (ret < 0) {
        printf("snd_pcm_prepare failed!, return %d\n", ret);
        return ret;
    }

    return ret;
}

/**
 * @brief Pause track, geniesdk will call start to make it active again
 * @return int32_t 0 : success; other: fail
 */
int32_t pause()
{
    int32_t ret=0;

    ret = snd_pcm_pause(g_audio_mgr.handle, 1);
    if (ret < 0) {
        printf("snd_pcm_pause failed!, return %d\n", ret);
        return ret;
    }

    return ret;

}
/**
 * @brief Flush track when track in pause or stopped state
 * @return int32_t 0 : success; other: fail
 */
int32_t flush()
{
    int32_t ret=0;

    ret = snd_pcm_drain(g_audio_mgr.handle);
    if (ret < 0) {
        printf("snd_pcm_drain failed!, return %d\n", ret);
        return ret;
    }

    return ret;

}
/**
 * @brief Stop audio track, geniesdk will call start to make it active again
 *         stop will cause more to make it active to pause
 * @return int32_t 0 : success; other: fail
 */
int32_t stop()
{
    int32_t ret=0;

    ret = snd_pcm_drop(g_audio_mgr.handle);
    if (ret < 0) {
        printf("snd_pcm_drop failed!, return %d\n", ret);
        return ret;
    }

    return ret;

}

/**
 * @brief Delete audio track
 * @return int32_t 0 : success; other: fail
 */
int32_t close()
{
    int32_t ret=0;

    ret = snd_pcm_close(g_audio_mgr.handle);
    if (ret < 0) {
        printf("snd_pcm_close failed!, return %d\n", ret);
        return ret;
    }

    return ret;

}

/**
 * @brief Write pcm data to audio track must be block mode
 * @param[in] buffer Pcm data buffer
 * @param[in] size Pcm data length
 * @return int32_t Actual write size
 */
int32_t write(uint8_t* buffer, int32_t size)
{
    unsigned int frame_bytes = g_audio_mgr.frame_bytes;
    snd_pcm_sframes_t frames_total = size / frame_bytes;
    snd_pcm_sframes_t frames = 0;
    snd_pcm_sframes_t r, result = 0;


    while (frames_total > 0) {
        frames = (frames_total > g_audio_mgr.chunk_size) ? g_audio_mgr.chunk_size : frames_total;
        r = snd_pcm_writei(g_audio_mgr.handle, buffer, frames);
        if (r != frames) {
            printf("snd_pcm_writei return %ld\n", r);
        }
        if (r == -EAGAIN) {
            usleep(10000);
            continue;
        } else if (r == -EPIPE) {
            xrun(g_audio_mgr.handle);
            continue;
        } else if (r < 0) {
            printf("snd_pcm_writei failed, return %d\n", r);
            return r;
        }
        buffer += (r * frame_bytes);
        frames_total -= r;
        result += r;
    }

    return (result*frame_bytes);

}


int _agcts_audio_alsa_test(void)
{
    int ret = _AGCTS_FAIL;
    int32_t samplerate = 48000;
    int32_t channelcount = 1;
    int32_t format = FORMAT_PCM_S16;
    uint8_t *databuffer = (uint8_t *)pcm_data;
    int32_t datalen = sizeof(pcm_data);
    int32_t samplesize = 2304;

    int32_t result = 0;

    result = open(samplerate,channelcount,format);

    if(result != 0) {
        _AGCTS_LOGD("%s: alsa open failed!!! reslt:%d\n", __FUNCTION__,result);
        return ret;
    }

    result = start();

    if(result != 0) {
        _AGCTS_LOGD("%s: alsa start failed!!! result:%d\n", __FUNCTION__,result);
        return ret;
    }

    while (datalen > 0) {
        samplesize = (datalen > samplesize) ? samplesize : datalen;
        _AGCTS_LOGD("%s: alsa write datalen:%d samplesize:%d\n", __FUNCTION__,datalen,samplesize);
        result = write(databuffer, samplesize);
        if (result == samplesize) {
            datalen -= result;
            databuffer += result;
        } else {
            _AGCTS_LOGD("%s: alsa write failed!!!.result:%d\n", __FUNCTION__,result);
            return ret;
        }
    }

    result = stop();

    if(result != 0) {
        _AGCTS_LOGD("%s: alsa stop failed!!! result:%d\n", __FUNCTION__,result);
        return ret;
    }

    result = close();

    if(result != 0) {
        _AGCTS_LOGD("%s: alsa close failed!!! result:%d\n", __FUNCTION__,result);
        return ret;
    }

    ret = _AGCTS_SUCCESS;
    return ret;
}

AGCTS_TEST_CASE agcts_audio_test_cases[] = {
    {(char *)"audio ALSA API", _agcts_audio_alsa_test},
};

void agcts_audio_test()
{
    _AGCTS_TEST_BEGIN

    int i;
    for (i = 0; i < sizeof(agcts_audio_test_cases) / sizeof(AGCTS_TEST_CASE); i++) {
        agcts_run_test(&agcts_audio_test_cases[i]);
    }

    _AGCTS_TEST_END
}
