
#ifndef RTOS_SOUND_CONTROL_H
#define RTOS_SOUND_CONTROL_H

#include <pcm.h>
#include "captureControl2.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
typedef int (*AudioFrameCallback)(void* pUser, void* para);

typedef struct CapturePcmData
{
    unsigned char* pData;
    int   nSize;
    unsigned int samplerate;
    unsigned int channels;
    int accuracy;
} CapturePcmData;
*/

typedef enum CaptureStatus_t
{
    STATUS_START = 0,
    STATUS_PAUSE ,
    STATUS_STOP
}CaptureStatus;

typedef struct CaptureCtrlContext_t
{
    CaptureCtrl                   base;
    snd_pcm_uframes_t           chunk_size;
    snd_pcm_format_t            alsa_format;
    snd_pcm_hw_params_t         *alsa_hwparams;
    snd_pcm_t                   *alsa_handler;
    snd_pcm_access_t            alsa_access_type;
    snd_pcm_stream_t            alsa_open_mode;
    int                nSampleRate;
    unsigned int                nChannelNum;
    unsigned int                         alsa_fragcount;
    int                         alsa_can_pause;
    size_t                      bytes_per_sample;
    CaptureStatus                 sound_status;
    int                         mVolume;
    pthread_mutex_t             mutex;
    //AudioFrameCallback mAudioframeCallback;
    //void*                pUserData;
}CaptureCtrlContext;

CaptureCtrl* RTCaptureDeviceCreate();

void RTCaptureDeviceDestroy(CaptureCtrl* s);

void RTCaptureDeviceSetFormat(CaptureCtrl* s,CdxCapbkCfg* cfg);

int RTCaptureDeviceStart(CaptureCtrl* s);

int RTCaptureDeviceStop(CaptureCtrl* s);

int RTCaptureDeviceRead(CaptureCtrl* s, void* pData, int nDataSize);
#if 0
int RTCaptureDevicePause(CaptureCtrl* s);

int RTCaptureDeviceFlush(CaptureCtrl* s,void *block);

int RTCaptureDeviceWrite(CaptureCtrl* s, void* pData, int nDataSize);

int RTCaptureDeviceReset(CaptureCtrl* s);

int RTCaptureDeviceGetCachedTime(CaptureCtrl* s);
int RTCaptureDeviceGetFrameCount(CaptureCtrl* s);
int RTCaptureDeviceSetPlaybackRate(CaptureCtrl* s,const XAudioPlaybackRate *rate);

int RTCaptureDeviceSetVolume(CaptureCtrl* s,int volume);

int RTCaptureDeviceControl(CaptureCtrl* s, int cmd, void* para);
#endif
static CaptureControlOpsT mCaptureControlOps =
{
    .destroy          =   RTCaptureDeviceDestroy,
    .setFormat        =   RTCaptureDeviceSetFormat,
    .start            =   RTCaptureDeviceStart,
    .stop             =   RTCaptureDeviceStop,
    .read             =   RTCaptureDeviceRead,
#if 0
    .pause            =   RTCaptureDevicePause,
    .flush            =  RTCaptureDeviceFlush,
    .write            =   RTCaptureDeviceWrite,
    .reset            =   RTCaptureDeviceReset,
    .getCachedTime    =   RTCaptureDeviceGetCachedTime,
    .getFrameCount    =   RTCaptureDeviceGetFrameCount,
    .setPlaybackRate  =   RTCaptureDeviceSetPlaybackRate,
    .control          =   RTCaptureDeviceControl,
#endif    
};

#ifdef __cplusplus
}
#endif

#endif
