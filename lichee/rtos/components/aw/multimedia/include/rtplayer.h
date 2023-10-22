#ifndef RTPLAYER_H
#define RTPLAYER_H

#include <xplayer.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
  *The response of state change notices APP what current state of player.
  */
typedef enum RTplayerNotifyAppType
{
    RTPLAYER_NOTIFY_PREPARED			          = 0,
    RTPLAYER_NOTIFY_PLAYBACK_COMPLETE       = 1,
    RTPLAYER_NOTIFY_SEEK_COMPLETE		 = 2,
    RTPLAYER_NOTIFY_MEDIA_ERROR			 = 3,
    RTPLAYER_NOTIFY_NOT_SEEKABLE			 = 4,
    RTPLAYER_NOTIFY_BUFFER_START			 = 5, /*this means no enough data to play*/
    RTPLAYER_NOTIFY_BUFFER_END			 = 6, /*this means got enough data to play*/
    RTPLAYER_NOTIFY_DOWNLOAD_START		 = 7,//not support now
    RTPLAYER_NOTIFY_DOWNLOAD_END	         = 8,//not support now
    RTPLAYER_NOTIFY_DOWNLOAD_ERROR           = 9,//not support now
    RTPLAYER_NOTIFY_AUDIO_FRAME                    = 10,//notify the decoded audio frame
    RTPLAYER_NOTIFY_DETAIL_INFO = 11,
}RTplayerNotifyAppType;

typedef enum RTplayerMediaErrorType
{
    RTPLAYER_MEDIA_ERROR_UNKNOWN			= 1,
    RTPLAYER_MEDIA_ERROR_OUT_OF_MEMORY	= 2,//not support now
    RTPLAYER_MEDIA_ERROR_IO				= 3,
    RTPLAYER_MEDIA_ERROR_UNSUPPORTED	= 4,
    RTPLAYER_MEDIA_ERROR_TIMED_OUT		= 5,//not support now
}RTplayerMediaErrorType;

typedef enum XplayerSoundCardType {
    CARD_DEFAULT = 0,
#ifdef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM_HW_MULTI_PCM
    CARD_MULTI  = 6,
#endif
#ifdef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM_HW_AMP
    CARD_AMP_PB = 7,
#endif
#ifdef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM_HW_BT
    CARD_BT_SRC = 8,
#endif
} XplayerSoundCardType;

/*
typedef struct AudioPcmData
{
    unsigned char* pData;
    unsigned int   nSize;
    unsigned int samplerate;
    unsigned int channels;
    int accuracy;
} AudioPcmData;
*/

/** define of player_callback_t function
*
* @param userData playback set by setDataSource
* @param msg audio player callback to smartbox
* @param id playback set by setDataSource
* @param ext1 not use now
* @param ext2 not use now
*/
typedef void (*player_callback_t)(void* userData,int msg, int id, int ext1, int ext2);


typedef struct RTPlayerContext
{
    XPlayer*					mXPlayer;
    void*						mUserData;
    int                            mId;
    player_callback_t		mNotifier;
    SoundCtrl*			    mSoundCtrl;
    MediaInfo*                  mMediaInfo;
}RTPlayer;

typedef  int status_t;

/** init player on smartbox that create framework audio player
*
* @return framework audio player handle if success; otherwise return NULL
*/
void *player_init(void);


/** set data source to audio player used by URL and local prompt
*
* @param handle create at player_init function
* @param userData indicate the smarbox class that when callback used
* @param url the link that playback used
* @param id indicate the smarbox playback index use when callback
* @return 0 if success; otherwise return the error code
*/
status_t setDataSource_url(void* handle,void* userData, const char *url, int id);


/** set data source to audio player used by TTS
*
* @param handle create at player_init function
* @param userData indicate the smarbox class that when callback used
* @param id indicate the smarbox playback index use when callback
* @return 0 if success; otherwise return the error code
*/
status_t setDataSource(void* handle,void* userData, int id);


/** prepare the framework audio player sync mode
*
* @param handle create at player_init function
* @param userData indicate the smarbox class that when callback used
* @param id indicate the smarbox playback index use when callback
* @return 0 if success; otherwise return the error code
*/
status_t prepare(void* handle);


/** prepare the framework audio player async mode and should callback notify smartbox
*
* @param handle create at player_init function
* @return 0 if success; otherwise return the error code
*/
status_t prepareAsync(void* handle);


/** start playback called by smartbox
*
* @param handle create at player_init function
* @return 0 if success; otherwise return the error code
*/
status_t start(void* handle);


/** stop playback called by smartbox
*
* @param handle create at player_init function
* @return 0 if success; otherwise return the error code
*/
status_t stop(void* handle);


/** pause playback called by smartbox
*
* @param handle create at player_init function
* @return 0 if success; otherwise return the error code
*/
status_t pause_l(void* handle);


/** playback seek function called by smartbox
*
* @param handle create at player_init function
* @param sec that seek to time second
* @return 0 if success; otherwise return the error code
*/
status_t seekTo(void* handle, int sec);


/** reset the framwork function when playback end
*
* @param handle create at player_init function
* @return 0 if success; otherwise return the error code
*/
status_t reset(void* handle);


MediaInfo*  getMediaInfo(void* handle);

/** set the framework audio player to looping mode
*
* @param handle create at player_init function
* @return 0 if success; otherwise return the error code
*/
status_t setLooping(void* handle, int loop);

status_t setSoundCard(void* handle, int card);

status_t changeSoundCard(void* handle, int card);

/** get music total duration
*
* @param handle create at player_init function
* @param sec indicate the music total duration
* @return 0 if success; otherwise return the error code
*/
status_t getDuration(void* handle, int *sec);


/** get current playback duration
*
* @param handle create at player_init function
* @param sec indicate the music current playback duration
* @return 0 if success; otherwise return the error code
*/
status_t getCurrentPosition(void* handle, int *sec);


/** deinit player on smartbox
*
* @param handle create at player_init function
*/
void player_deinit(void* handle);


/** write data to audio player used by TTS
*
* @param handle create at player_init function
* @param buffer the point that TTS data store
* @param size the size of TTS data normally the size is 2048
* @return 0 if success; otherwise return the error code
*/
status_t WriteData(void* handle, unsigned char* buffer, int size);


/** write data to audio player used by TTS and the end
*
* @param handle create at player_init function
* @param buffer the point that TTS data store
* @param size the size of TTS data normally the size is less than 2048
* @return 0 if success; otherwise return the error code
*/
status_t writeDatawithEOS(void* handle, unsigned char* buffer, int size);


/** get the ring buffer available size
*
* @param handle create at player_init function
* @return the available size
*/
long long int getAvailiableSize(void* handle);


/** register callback that framework callback to smartbox
*
* @param handle create at player_init function
* @param fn callback function to framework audio player
*/
void registerCallback(void* handle, void* userData, player_callback_t fn);

void player_show_buffer(void);

#ifdef __cplusplus
}
#endif

#endif
