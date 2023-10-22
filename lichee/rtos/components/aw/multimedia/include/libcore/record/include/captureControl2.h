/*
 * Copyright (c) 2008-2016 Allwinner Technology Co. Ltd.
 * All rights reserved.
 *
 * File : soundControl.h
 * Description : soundControl
 * History :
 *
 */

#ifndef SOUND_CONTROL_H
#define SOUND_CONTROL_H
#include "cdx_log.h"
//#include "adecoder.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum XAudioTimestretchStretchMode  {
    AUDIO_TIMESTRETCH_STRETCH_DEFAULT            = 0,
    AUDIO_TIMESTRETCH_STRETCH_SPEECH             = 1,
} XAudioTimestretchStretchMode;

typedef enum XAudioTimestretchFallbackMode  {
    XAUDIO_TIMESTRETCH_FALLBACK_CUT_REPEAT     = -1,
    XAUDIO_TIMESTRETCH_FALLBACK_DEFAULT        = 0,
    XAUDIO_TIMESTRETCH_FALLBACK_MUTE           = 1,
    XAUDIO_TIMESTRETCH_FALLBACK_FAIL           = 2,
} XAudioTimestretchFallbackMode;

typedef struct XAudioPlaybackRate {
    float mSpeed;
    float mPitch;
    enum XAudioTimestretchStretchMode  mStretchMode;
    enum XAudioTimestretchFallbackMode mFallbackMode;
}XAudioPlaybackRate;

typedef struct {
    int                      nChannels;
    int                      nSamplerate;
    int                      nBitpersample;
} CdxCapbkCfg;

typedef struct CaptureCtrl CaptureCtrl;

typedef struct CaptureControlOpsS CaptureControlOpsT;

struct CaptureControlOpsS
{
    void (*destroy)(CaptureCtrl* c);

    void (*setFormat)(CaptureCtrl* c, CdxCapbkCfg* cfg);

    int (*start)(CaptureCtrl* c);

    int (*stop)(CaptureCtrl* c);
    
    int (*read)(CaptureCtrl* c, void* pData, int nDataSize);
#if 0
    int (*pause)(CaptureCtrl* s);

    int (*flush)(CaptureCtrl* s, void *block);

    int (*write)(CaptureCtrl* s, void* pData, int nDataSize);

    int (*reset)(CaptureCtrl* s);

    int (*getCachedTime)(CaptureCtrl* s);

    int (*getFrameCount)(CaptureCtrl* s);

    int (*setPlaybackRate)(CaptureCtrl* s,const XAudioPlaybackRate *rate);

    int (*control)(CaptureCtrl* s, int cmd, void* para);
#endif    
};

struct CaptureCtrl
{
    const struct CaptureControlOpsS* ops;
};

typedef enum _CaptureCtrlCmd
{
/*
    Set para area...
*/
    SOUND_CONTROL_SET_OPTION_START = 100,
    SOUND_CONTROL_SET_CLBK_EOS,
/*
    Get para area...
*/
    SOUND_CONTROL_GET_OPTION_START = 200,
/*
    Query area...
*/
    SOUND_CONTROL_QUERY_OPTION_START = 300,
    SOUND_CONTROL_QUERY_IF_GAPLESS_PLAY,
/*
    Capture Stream Control area...
*/
    SOUND_CONTROL_SET_OUTPUT_CONFIG = 400,
    SOUND_CONTROL_CLEAR_OUTPUT_CONFIG,
}CaptureCtrlCmd;

static inline void CaptureDeviceDestroy(CaptureCtrl* c)
{
    CDX_CHECK(c);
    CDX_CHECK(c->ops);
    CDX_CHECK(c->ops->destroy);
    return c->ops->destroy(c);
}

static inline void CaptureDeviceSetFormat(CaptureCtrl* c, CdxCapbkCfg* cfg)
{
    CDX_CHECK(c);
    CDX_CHECK(c->ops);
    CDX_CHECK(c->ops->setFormat);
    return c->ops->setFormat(c, cfg);
}

static inline int CaptureDeviceStart(CaptureCtrl* c)
{
    CDX_CHECK(c);
    CDX_CHECK(c->ops);
    CDX_CHECK(c->ops->start);
    return c->ops->start(c);
}

static inline int CaptureDeviceStop(CaptureCtrl* c)
{
    CDX_CHECK(c);
    CDX_CHECK(c->ops);
    CDX_CHECK(c->ops->stop);
    return c->ops->stop(c);
}

static inline int CaptureDeviceRead(CaptureCtrl* c, void* pData, int nDataSize)
{
    CDX_CHECK(c);
    CDX_CHECK(c->ops);
    CDX_CHECK(c->ops->read);
    return c->ops->read(c, pData, nDataSize);
}

#if 0
static inline int CaptureDevicePause(CaptureCtrl* s)
{
    CDX_CHECK(s);
    CDX_CHECK(s->ops);
    CDX_CHECK(s->ops->pause);
    return s->ops->pause(s);
}


static inline int CaptureDeviceFlush(CaptureCtrl* s, void* block)
{
    CDX_CHECK(s);
    CDX_CHECK(s->ops);
    CDX_CHECK(s->ops->flush);
    return s->ops->flush(s, block);
}


static inline int CaptureDeviceWrite(CaptureCtrl* s, void* pData, int nDataSize)
{
    CDX_CHECK(s);
    CDX_CHECK(s->ops);
    CDX_CHECK(s->ops->write);
    return s->ops->write(s, pData, nDataSize);
}

static inline int CaptureDeviceReset(CaptureCtrl* s)
{
    CDX_CHECK(s);
    CDX_CHECK(s->ops);
    CDX_CHECK(s->ops->reset);
    return s->ops->reset(s);
}

static inline int CaptureDeviceGetCachedTime(CaptureCtrl* s)
{
    CDX_CHECK(s);
    CDX_CHECK(s->ops);
    CDX_CHECK(s->ops->getCachedTime);
    return s->ops->getCachedTime(s);
}

static inline int CaptureDeviceGetFrameCount(CaptureCtrl* s)
{
    CDX_CHECK(s);
    CDX_CHECK(s->ops);
    CDX_CHECK(s->ops->getFrameCount);
    return s->ops->getFrameCount(s);
}

static inline int CaptureDeviceSetPlaybackRate(CaptureCtrl* s,const XAudioPlaybackRate *rate)
{
    CDX_CHECK(s);
    CDX_CHECK(s->ops);
    return s->ops->setPlaybackRate(s,rate);
}

static inline int CaptureDeviceControl(CaptureCtrl* s, int cmd, void* para)
{
    CDX_CHECK(s);
    CDX_CHECK(s->ops);
    CDX_CHECK(s->ops->control);
    return s->ops->control(s, cmd, para);
}
#endif
CaptureCtrl* CaptureDeviceCreate();

#ifdef __cplusplus
}
#endif

#endif

