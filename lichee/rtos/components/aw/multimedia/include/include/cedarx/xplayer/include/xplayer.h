/*
* Copyright (c) 2008-2016 Allwinner Technology Co. Ltd.
* All rights reserved.
*
* File : xplayer.h
* Description : xplayer
* History :
*   Author  : AL3
*   Date    : 2015/05/05
*   Comment : first version
*
*/

#ifndef XPLAYER_H
#define XPLAYER_H

//#include <semaphore.h>
#include <pthread.h>
#include <stdint.h>
#include "CdxKeyedVector.h"
//#include "cdx_config.h"       //* configuration file in "LiBRARY/"
#include <CdxEnumCommon.h>
#include "CdxTypes.h"
#include "soundControl.h"

#include "mediaInfo.h"

#define AWPLAYER_CONFIG_DISABLE_VIDEO       1
#define AWPLAYER_CONFIG_DISABLE_AUDIO       0
#define AWPLAYER_CONFIG_DISABLE_SUBTITLE    1
#define AWPLAYER_CONFIG_DISALBE_MULTI_AUDIO 0

#ifdef __cplusplus
extern "C" {
#endif

/* Since xplayer is operating system independent, there is no benefit and no
 * simple method to keep items in MediaEventType have the same value as items
 * in media_event_type of Android.
 */
enum MediaEventType {
    AWPLAYER_MEDIA_NOP = MEDIA_EVENT_VALID_RANGE_MIN, // = 0, interface test message
    AWPLAYER_MEDIA_PREPARED,
    AWPLAYER_MEDIA_PLAYBACK_COMPLETE,
    AWPLAYER_MEDIA_BUFFERING_UPDATE,
    AWPLAYER_MEDIA_SEEK_COMPLETE,
    AWPLAYER_MEDIA_SET_VIDEO_SIZE,
    AWPLAYER_MEDIA_STARTED,
    AWPLAYER_MEDIA_PAUSED,
    AWPLAYER_MEDIA_STOPPED,
    AWPLAYER_MEDIA_SKIPPED,
    AWPLAYER_MEDIA_TIMED_TEXT,
    AWPLAYER_MEDIA_ERROR,
    AWPLAYER_MEDIA_INFO,
    AWPLAYER_MEDIA_SUBTITLE_DATA,
    AWPLAYER_MEDIA_CHANGE_URL,

    AWPLAYER_MEDIA_LOG_RECORDER,

    AWPLAYER_EXTEND_MEDIA_INFO,
    AWPLAYER_MEDIA_META_DATA,
    AWPLAYER_MEDIA_EVENT_MAX,
};
CHECK_MEDIA_EVENT_MAX_VALID(AWPLAYER_MEDIA_EVENT_MAX)

// av/include/media/mediaplayer.h
enum MediaInfoType
{
    AW_MEDIA_INFO_UNKNOWN = 1,
    AW_MEDIA_INFO_STARTED_AS_NEXT = 2,
    AW_MEDIA_INFO_RENDERING_START = 3,
    AW_MEDIA_INFO_BUFFERING_START = 701,
    AW_MEDIA_INFO_BUFFERING_END = 702,

    AW_MEDIA_INFO_NOT_SEEKABLE = 801,

    AW_MEDIA_INFO_DOWNLOAD_START  = 10086,
    AW_MEDIA_INFO_DOWNLOAD_END   = 10087,
    AW_MEDIA_INFO_DOWNLOAD_ERROR  = 10088,

    AW_MEDIA_INFO_DETAIL  = 10089,
};

#if NO_USE
enum ExMediaInfoType
{
    AW_EX_IOREQ_ACCESS     = 1,
    AW_EX_IOREQ_OPEN      = 2,
    AW_EX_IOREQ_OPENDIR    = 3,
    AW_EX_IOREQ_READDIR    = 4,
    AW_EX_IOREQ_CLOSEDIR   = 5,
};
#endif

//   0xx: Reserved
//   1xx: Android Player errors. Something went wrong inside the MediaPlayer.
//   2xx: Media errors (e.g Codec not supported). There is a problem with the
//        media itself.
//   3xx: Runtime errors. Some extraordinary condition arose making the playback
//        impossible.
//
enum MediaErrorType
{
    // 0xx
    AW_MEDIA_ERROR_UNKNOWN = 1,
    // 1xx
    AW_MEDIA_ERROR_SERVER_DIED = 100,
    // 2xx
    AW_MEDIA_ERROR_NOT_VALID_FOR_PROGRESSIVE_PLAYBACK = 200,
    // 3xx
    // 9xx
    AW_MEDIA_ERROR_OUT_OF_MEMORY = 900,
    // 4xx
    AW_MEDIA_ERROR_IO = -1004,
    AW_MEDIA_ERROR_MALFORMED = -1007,
    AW_MEDIA_ERROR_UNSUPPORTED = -1010,
    AW_MEDIA_ERROR_TIMED_OUT = -110,
};

#if NO_USE
typedef enum AwApplicationType {
    APP_DEFAULT,
    APP_STREAMING, // for miracast and so on
    APP_CMCC_WASU,
    APP_CMCC_LOCAL,
} AwApplicationType;

typedef struct XPlayerConfig_t {
    AwApplicationType appType;
    int livemode;
} XPlayerConfig_t;
#endif

typedef int (*XPlayerNotifyCallback)(void* pUser,
                        			 int msg, int ext1, void* para);

typedef enum XPlayerTimeoutType {
	XPLAYER_PLAYBACK_TIMEOUT,
	XPLAYER_PREPARE_TIMEOUT,
	XPLAYER_TCPREAD_TIMEOUT,
} XPlayerTimeoutType;

typedef struct XPlayerBufferConfig
{
	int maxStreamBufferSize;
	int maxStreamFrameCount;
	int maxBitStreamBufferSize;
	int maxBitStreamFrameCount;
	int maxPcmBufferSize;
} XPlayerBufferConfig;

typedef struct PlayerContext XPlayer;

XPlayer* XPlayerCreate();

void XPlayerDestroy(XPlayer* p);

#if VIDEO_SUPPORT
int XPlayerConfig(XPlayer* p, const XPlayerConfig_t *config);
#endif

int XPlayerSetNotifyCallback(XPlayer* p,
                             XPlayerNotifyCallback notifier,
                             void* pUserData);

int XPlayerInitCheck(XPlayer* p);

#if NO_USE
int XPlayerSetUID(XPlayer* p, int nUid);
#endif

int XPlayerSetDataSourceUrl(XPlayer* p, const char* pUrl,
                            void* httpService, const CdxKeyedVectorT* pHeaders);

#if NO_USE
int XPlayerSetDataSourceFd(XPlayer* p, int fd,
                           int64_t nOffset, int64_t nLength);
#endif

// for IStreamSource in android
int XPlayerSetDataSourceStream(XPlayer* p, const char* pStreamUri);

int XPlayerPrepare(XPlayer* p);

int XPlayerPrepareAsync(XPlayer* p);

int XPlayerStart(XPlayer* p);

int XPlayerStop(XPlayer* p);

int XPlayerPause(XPlayer* p);

int XPlayerIsPlaying(XPlayer* p);

int XPlayerSeekTo(XPlayer* p, int nSeekTimeMs);

#if SET_SPEED_SUPPORT
int XPlayerSetSpeed(XPlayer* p, int nSpeed);
#endif

int XPlayerGetCurrentPosition(XPlayer* p, int* msec);

int XPlayerGetDuration(XPlayer* p, int* msec);

int XPlayerReset(XPlayer* p);

int XPlayerSetLooping(XPlayer* p, int bLoop);

int XPlayerSetSoundCard(XPlayer* p, int card);

int XPlayerGetSoundCard(XPlayer* p);
/*
	Recommand prepare timeout is multi-times to playback timeout, and
	playback timeout is over 100ms.
*/
int XPlayerSetTimeout(XPlayer* p, XPlayerTimeoutType type, unsigned int timeout_ms);

#if SUBTITLE_SUPPORT
int XPlayerGetSubDelay(XPlayer* p);

int XPlayerSetSubDelay(XPlayer* p, int nTimeMs);

int XPlayerGetSubCharset(XPlayer* p, char *charset);

int XPlayerSetSubCharset(XPlayer* p, const char* strFormat);

void XPlayerSetSubCtrl(XPlayer* p, const SubCtrl* subctrl);

int XPlayerSwitchSubtitle(XPlayer* pl, int nStreamIndex);

int XPlayerSetExternalSubUrl(XPlayer* p, const char* fileName);

int XPlayerSetExternalSubFd(XPlayer* p, int fd, int64_t offset, int64_t len, int fdSub);
#endif

#if VIDEO_SUPPORT
int XPlayerSetVideoSurfaceTexture(XPlayer* p, const LayerCtrl* surfaceTexture);
#endif

void XPlayerSetAudioSink(XPlayer* p, const SoundCtrl* audioSink);

#if VIDEO_SUPPORT
void XPlayerSetDeinterlace(XPlayer* p, const Deinterlace* di);
#endif

#if NO_USE
MediaInfo* XPlayerGetMediaInfo(XPlayer* p);
#endif

int XPlayerSwitchAudio(XPlayer* p, int nStreamIndex);

int XPlayerGetPlaybackSettings(XPlayer* p,XAudioPlaybackRate *rate);

int XPlayerSetPlaybackSettings(XPlayer* p,const XAudioPlaybackRate *rate);

void XPlayerSetHttpBuffer(XPlayer* p, int size, int threshold);

void XPlayerSetBuffer(XPlayer* p, const XPlayerBufferConfig *bufcfg);

void XPlayerSetPcmFrameSize(XPlayer* p, const int pcm_frame_size);

void XPlayerShowVersion(void);

void XPlayerShowBuffer(void);

void XPlayerSetAacSbr(XPlayer* p, const int use_sbr);

#ifdef __cplusplus
}
#endif

#endif  // AWPLAYER
