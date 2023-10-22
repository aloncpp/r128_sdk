/*
 * Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of XRADIO TECHNOLOGY CO., LTD. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "recorder_test.h"
#include "ExampleCustomerWriter.h"
#include "xrecord.h"
#include <aw_common.h>
#include <console.h>

#if CONFIG_COMPONENTS_PM
#include <pm_base.h>
#include <pm_notify.h>
#include <pm_wakelock.h>
#endif

#include <sys/types.h>
#include <dirent.h>

#define RECORDER_LOGD(msg, arg...)      printf("[RECORDER_DBG] <%s : %d> " msg "\n", __func__, __LINE__, ##arg)
#define RECORDER_LOGI(msg, arg...)      printf("[RECORDER_INFO] <%s : %d> " msg "\n", __func__, __LINE__, ##arg)
#define RECORDER_LOGW(msg, arg...)      printf("[RECORDER_WRN] <%s : %d> " msg "\n", __func__, __LINE__, ##arg)
#define RECORDER_LOGE(msg, arg...)      printf("[RECORDER_ERR] <%s : %d> " msg "\n", __func__, __LINE__, ##arg)

#define CLEAN_THE_PATH 1

typedef struct recorder
{
    recorder_base base;
    XRecord *xrecorder;
    CaptureCtrl *cap;
} recorder;

static void showHelp(){
    printf("\n");
    printf("**************************\n");
    printf("* This is a simple audio recoder, when it is started, you can input commands to tell\n");
    printf("* what you want it to do.\n");
    printf("* Usage: \n");
    printf("*   cedarx_record amr 10 usb_msc 1 : this means record a 10s amr music in usb_msc/\n");
    printf("*   cedarx_record pcm 20 data 3 : this means record 3 pieces of 10s pcm music in data/\n");
    printf("**************************\n");
}

recorder *recorder_singleton = NULL;


#if CONFIG_COMPONENTS_PM
/*
 *
 * do the callback function before PM suspend happen
 *
 *
 */
static struct wakelock recorder_wakelock = {
    .name = "recorder_wakelock",
    .ref = 0,
};
static int cedarx_stop_record(void);

int recorder_destroy(recorder_base *base);
int RecCallbackFromPm(suspend_mode_t mode, pm_event_t event, void *arg)
{
    switch (event) {
    case PM_EVENT_SYS_PERPARED:
    {
        recorder_base *recorder_stop = NULL;
        if (recorder_singleton != NULL) {
            printf("************cedarx_recorder:end recording early\n");
            recorder_stop = &recorder_singleton->base;
            recorder_stop->stop(recorder_stop);
            recorder_destroy(recorder_stop);
            printf("************cedarx_recorder:end recording early......done\n");
        }
        break;
    }
    case PM_EVENT_SYS_FINISHED:
    {
        break;
    }
    default:
        break;
    }
    return 0;
}

static pm_notify_t pm_cdxrecorder = {
    .name = "pm_cdxrecorder",
    .pm_notify_cb = RecCallbackFromPm,
    .arg = NULL,
};
#endif

static int record_start(recorder_base *base, const char *url, const rec_cfg *cfg)
{
    recorder *impl = container_of(base, recorder, base);

    XRecordConfig audioConfig;

    if (cfg->type == XRECODER_AUDIO_ENCODE_PCM_TYPE)
    {
        audioConfig.nChan = cfg->chan_num;
        audioConfig.nSamplerate = cfg->sample_rate;
        audioConfig.nSamplerBits = cfg->sampler_bits;
        audioConfig.nBitrate = cfg->bitrate;
    }
    else if (cfg->type == XRECODER_AUDIO_ENCODE_AMR_TYPE)
    {
        audioConfig.nChan = 1;
        audioConfig.nSamplerate = 8000;//amr-nb 8000Hz amr-wb 16000Hz
        audioConfig.nSamplerBits = 16;
        audioConfig.nBitrate = 12200;//amr-nb 12200  amr-wb 23850
    } else {
        audioConfig.nChan = cfg->chan_num;
        audioConfig.nSamplerate = cfg->sample_rate;
        audioConfig.nSamplerBits = cfg->sampler_bits;
        audioConfig.nBitrate = cfg->bitrate;
    }

    XRecordSetDataDstUrl(impl->xrecorder, url, NULL, NULL);
    XRecordSetAudioEncodeType(impl->xrecorder, cfg->type, &audioConfig);

    XRecordPrepare(impl->xrecorder);
    XRecordStart(impl->xrecorder);
    RECORDER_LOGI("record start");
    return 0;
}

static int record_stop(recorder_base *base)
{
    recorder *impl = container_of(base, recorder, base);
    XRecordStop(impl->xrecorder);
    return 0;
}
extern CaptureCtrl* RTCaptureDeviceCreate();
recorder_base *recorder_create()
{
    if (recorder_singleton != NULL)
        return &recorder_singleton->base;

    recorder *impl = malloc(sizeof(*impl));
    if (impl == NULL)
        return NULL;
    memset(impl, 0, sizeof(*impl));

    impl->xrecorder = XRecordCreate();
    if (impl->xrecorder == NULL)
        goto failed;

    impl->cap = (void *)(uintptr_t)RTCaptureDeviceCreate();
    if (impl->cap == NULL)
        goto failed;
    XRecordSetAudioCap(impl->xrecorder, impl->cap);

    impl->base.start = record_start;
    impl->base.stop = record_stop;

    recorder_singleton = impl;

    return &impl->base;

failed:
    RECORDER_LOGE("recorder create failed");
    if (impl->xrecorder)
        XRecordDestroy(impl->xrecorder);
    if (impl)
        free(impl);
    return NULL;
}

int recorder_destroy(recorder_base *base)
{
    recorder *impl = container_of(base, recorder, base);

    if (impl->xrecorder) {
        XRecordDestroy(impl->xrecorder);
    }

    free(impl);

    recorder_singleton = NULL;

    return 0;
}

static int cedarx_record_test(int argc, char **argv)
{
	recorder_base *recorder;
	rec_cfg cfg;
	char file_url[64];
    int file_index = 0;
	CdxWriterT *writer;
	memset(file_url, 0, 64);
    if(argc == 5){
        if( !strncmp("amr", argv[1], sizeof("amr")-1) ){
            cfg.type = XRECODER_AUDIO_ENCODE_AMR_TYPE;
//            snprintf(file_url, 64, "file://data/%ds.amr", atoi(argv[2]));
            cfg.sample_rate = 8000;//8000
            cfg.chan_num = 1;//1
            cfg.bitrate = 12200;
            cfg.sampler_bits = 16;
        }
        else if( !strncmp("pcm", argv[1], sizeof("pcm")-1) ){
            cfg.type = XRECODER_AUDIO_ENCODE_PCM_TYPE;
//            snprintf(file_url, 64, "file://data/%ds.pcm", atoi(argv[2]));
            cfg.sample_rate = 8000;//8000
            cfg.chan_num = 1;//1
            cfg.bitrate = 12200;
            cfg.sampler_bits = 16;
        }
        else if( !strncmp("mp3", argv[1], sizeof("mp3")-1) ){
            cfg.type = XRECODER_AUDIO_ENCODE_MP3_TYPE;
//            snprintf(file_url, 64, "file://data/%ds.mp3", atoi(argv[2]));
            cfg.sample_rate = 16000;
            cfg.chan_num = 1;
            cfg.bitrate = 32000;
            cfg.sampler_bits = 16;
        } else {
            printf("now support!\n");
            return -1;
        }
    } else {
        printf("the parameter is error,usage is as following:\n");
        showHelp();
        return -1;
    }
    if ((sizeof(argv[3])-1) > 32) {
            printf("dir path length is too long\n");
            return -1;
        }
        DIR *dirp = opendir(argv[3]);
        if (dirp == NULL) {
            printf("target path:%s ain't exist\n", argv[3]);
            return -1;
        }else
            closedir(dirp);

#if CONFIG_COMPONENTS_PM
    int pm_ret;
    pm_ret = pm_notify_register(&pm_cdxrecorder);
    printf("**********cedarx_recorder:pm_ret=%d\n", pm_ret);
    if (pm_ret < 0) {
        printf("**********cedarx_recorder:pm_notify register fail, ret:%d\n", pm_ret);
        return -1;
    }
#endif
    int rec_cnt = atoi(argv[4]);
    printf("*******cedarx_record:going to record %d recordings\n", rec_cnt);
    for (file_index = 0; file_index < rec_cnt; file_index++) {

        snprintf(file_url, 64, "file://%s/%ds_%d.%s", argv[3], atoi(argv[2]), file_index,  argv[1]);

#if CLEAN_THE_PATH
        FILE *FileClean = fopen(file_url+7, "w");
        if (FileClean != NULL)
            fclose(FileClean);
#endif

        recorder = recorder_create();
        if (recorder == NULL) {
            printf("recorder create fail, exit\n");
            return -1;
        }

        printf("===start record %s now, last for %d s===\n", argv[1], atoi(argv[2]));
        recorder->start(recorder, file_url, &cfg);

        int time_gate = atoi(argv[2]);
        int cnt = 0;
        while(1) {
            sleep(1);
            cnt++;
            if (cnt >= time_gate||recorder_singleton == NULL)
                break;
        }
exit:
        if (recorder_singleton != NULL) {
            recorder->stop(recorder);
            recorder_destroy(recorder);
        }

        printf("record %s over.\n", file_url+7);
    }
#if CONFIG_COMPONENTS_PM
    pm_notify_unregister(pm_ret);
#endif
    return 0;
}

FINSH_FUNCTION_EXPORT_CMD(cedarx_record_test, cedarx_record, cedarx record test demo);


static int cedarx_stop_record(void)
{
    recorder_base *recorder_stop = NULL;
    printf("************cedarx_recorder:end recording early\n");
    if (recorder_singleton != NULL) {
        recorder_stop = &recorder_singleton->base;
        recorder_stop->stop(recorder_stop);
        recorder_destroy(recorder_stop);
    }

    printf("************cedarx_recorder:end recording early......done\n");
    return 0;
}

FINSH_FUNCTION_EXPORT_CMD(cedarx_stop_record, stop_record, stop cedarx record test demo);
