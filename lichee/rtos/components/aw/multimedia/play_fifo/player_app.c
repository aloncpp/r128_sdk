#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "xplayer.h"
#include "player_app.h"
#include "rtosSoundControl.h"
/* xradio */
/*#include "sys_ctrl.h"*/
#include "defs.h"
//#include "rtplayer.h"
//
#define PLAYER_LOGD(msg, arg...)      //printf("[PLAYER_DBG] <%s : %d> " msg "\n", __func__, __LINE__, ##arg)
#define PLAYER_LOGI(msg, arg...)      printf("[PLAYER_INFO] <%s : %d> " msg "\n", __func__, __LINE__, ##arg)
#define PLAYER_LOGW(msg, arg...)      printf("[PLAYER_WRN] <%s : %d> " msg "\n", __func__, __LINE__, ##arg)
#define PLAYER_LOGE(msg, arg...)      printf("[PLAYER_ERR] <%s : %d> " msg "\n", __func__, __LINE__, ##arg)


#if APP_PLAYER_SUPPORT_TONE
struct tone_base
{
    int (*play)(tone_base *base, char *url);
    int (*stop)(tone_base *base);
    int (*destroy)(tone_base *base);
    app_player_callback cb;
    void *arg;
};
#endif

typedef struct player_info
{
    char *url;
    uint32_t size;
#if APP_PLAYER_SUPPORT_TONE
    int pause_time;
#endif
} player_info;

typedef struct app_player
{
    player_base base;
    XPlayer *xplayer;
    SoundCtrl *sound;
    aplayer_states state;
    app_player_callback cb;
    player_info info;
    pthread_mutex_t lock;
    uint8_t mute;
#if APP_PLAYER_SUPPORT_TONE
    tone_base my_tone;
    tone_base *tone;
#endif
    int vol;
    uint16_t id;
    void *arg;
	uint8_t isLooping;
} app_player;
/*
static void tone_handler(event_msg *msg);

static void player_handler(event_msg *msg);
*/

static int player_stop(player_base *base);
static int player_seturl(player_base *base, const char *url);
static int player_pause(player_base *base);
static int player_resume(player_base *base);
static int player_seek(player_base *base, int ms);
static int player_tell(player_base *base);
static int player_size(player_base *base);
static int player_setvol(player_base *base, int vol);
static int player_getvol(player_base *base);
static int player_mute(player_base *base, bool is_mute);
static int player_is_mute(player_base *base);
static int player_control(player_base *base, player_cmd command, void *data);

#if APP_PLAYER_SUPPORT_TONE
static int player_playtone(player_base *base, tone_base *drv, char *url);
static int player_stoptone(player_base *base, tone_base *drv);
#endif

static void player_setcb(player_base *base, app_player_callback cb, void *arg);
static aplayer_states player_get_states(player_base *base);
static void player_null_callback(player_events event, void *data, void *arg);

#define APLAYER_ID_AND_STATE(id, state)     (((id) << 16) | (state))
#define APLAYER_GET_ID(id_state)            (((id_state) >> 16) & 0xFFFF)
#define APLAYER_GET_STATE(id_state)         ((id_state) & 0xFFFF)

static app_player *player_singleton = NULL;
static int play(app_player *impl)
{
	//XPlayerSetLooping(impl->xplayer, 1);
	if(XPlayerStart(impl->xplayer) != 0)
	{
        PLAYER_LOGE("start() return fail.");
		return -1;
	}
	XPlayerShowBuffer();
    PLAYER_LOGD("playing");
	return 0;
}
#if APP_PLAYER_SUPPORT_TONE
static inline void set_current_time(app_player *impl)
{
    if(XPlayerGetCurrentPosition(impl->xplayer, &impl->info.pause_time) != 0) {
        PLAYER_LOGW("tell() return fail.");
    }
}

static inline bool is_toning(app_player *impl)
{
    return impl->tone != NULL;
}
#endif
/*
static inline void set_player_handler(app_player *impl, void (**handler)(event_msg *msg))
{

#if APP_PLAYER_SUPPORT_TONE
    if (is_toning(impl))
        *handler = tone_handler;
    else
#endif

        *handler = player_handler;
}
*/
static int set_url(app_player *impl, char* pUrl)
{
    /* set url to the AwPlayer. */
    if(XPlayerSetDataSourceUrl(impl->xplayer,
                 (const char*)pUrl, NULL, NULL) != 0)
    {
       // PLAYER_LOGE("setDataSource() return fail.");
		printf("error:setDataSourceUrl() return fail.\n");
		return -1;
    }
    if ((!strncmp(pUrl, "http://", 7)) || (!strncmp(pUrl, "https://", 8))) {
		if(XPlayerPrepareAsync(impl->xplayer) != 0)
        {
            PLAYER_LOGE("prepareAsync() return fail.");
            return -1;
        }
    } else {
		play(impl);
        impl->state = APLAYER_STATES_PLAYING;
	}
	PLAYER_LOGI("done\n");
    return 0;
}

static void *wrap_realloc(void *p, uint32_t *osize, uint32_t nsize)
{
    if (p == NULL) {
        *osize = nsize;
        return malloc(nsize);
    }
    if (*osize >= nsize)
        return p;
    free(p);
//    PLAYER_LOGD("free %d, malloc %d", *osize, nsize);
    *osize = nsize;
    return malloc(nsize);
}

static inline app_player *get_player()
{
    return player_singleton;
}



static int reset(app_player *impl)
{
    if(XPlayerReset(impl->xplayer) != 0)
    {
//        PLAYER_LOGE("reset() return fail.");
		printf("error:reset() return fail.\n");
		return -1;
    }
    impl->id++;
    return 0;
}
/*
static void player_handler_preprocess(app_player *impl, player_events evt)
{
    //PLAYER_LOGI("event: %d", (int)evt);
	pthread_mutex_lock(&impl->lock);
    switch (evt)
    {
        case PLAYER_EVENTS_MEDIA_PREPARED:
#if APP_PLAYER_SUPPORT_TONE
            if (impl->info.pause_time != 0)
            {
                if(XPlayerSeekTo(impl->xplayer, impl->info.pause_time) != 0) {
                    //PLAYER_LOGE("seek() return fail.");
                    printf("seek() return fail.");
                }
                //PLAYER_LOGI("seek to %d ms.", impl->info.pause_time);
                impl->info.pause_time = 0;
            }
#endif
            impl->cb(evt, NULL, impl->arg);
            if (impl->state != APLAYER_STATES_PAUSE)
            {
                play(impl);
            }
            impl->state = APLAYER_STATES_PLAYING;

            break;
        case PLAYER_EVENTS_MEDIA_PLAYBACK_COMPLETE:
        case PLAYER_EVENTS_MEDIA_ERROR:
            reset(impl);
            impl->state = APLAYER_STATES_STOPPED;
            break;
        default:
            break;
    }
	pthread_mutex_unlock(&impl->lock);
    //OS_RecursiveMutexUnlock(&impl->lock);

    if (evt != PLAYER_EVENTS_MEDIA_PREPARED) {
        impl->cb(evt, NULL, impl->arg);
    }

    return;
}
*/
/*
#if APP_PLAYER_SUPPORT_TONE
static void tone_handler(event_msg *msg)
{
    app_player* impl = get_player();
    player_events state = (player_events)APLAYER_GET_STATE(msg->data);
    app_player_callback cb = impl->tone->cb;

    //OS_RecursiveMutexLock(&impl->lock, OS_WAIT_FOREVER);
	pthread_mutex_lock(&impl->lock);
    if (APLAYER_GET_ID(msg->data) != impl->id)
        goto out;

    PLAYER_LOGI("tone event: %d, state: %d", (int)state, (int)impl->state);

    switch (state)
    {
        case PLAYER_EVENTS_MEDIA_PREPARED:
			play(impl);
            break;
        case PLAYER_EVENTS_TONE_STOPED:
            impl->tone = NULL;
            reset(impl);
            if (impl->state != APLAYER_STATES_STOPPED && impl->state != APLAYER_STATES_INIT)
                set_url(impl, impl->info.url);
            cb(PLAYER_EVENTS_TONE_STOPED, NULL, impl->tone->arg);
            break;
        case PLAYER_EVENTS_MEDIA_PLAYBACK_COMPLETE:
            impl->tone = NULL;
            reset(impl);
            if (impl->state != APLAYER_STATES_STOPPED && impl->state != APLAYER_STATES_INIT)
                set_url(impl, impl->info.url);
            cb(PLAYER_EVENTS_TONE_COMPLETE, NULL, impl->tone->arg);
            break;
        case PLAYER_EVENTS_MEDIA_ERROR:
            impl->tone = NULL;
            reset(impl);
            if (impl->state != APLAYER_STATES_STOPPED && impl->state != APLAYER_STATES_INIT)
                set_url(impl, impl->info.url);
            cb(PLAYER_EVENTS_TONE_ERROR, NULL, impl->tone->arg);
            break;
        default:
            break;
    }
out:
	pthread_mutex_unlock(&impl->lock);
//    OS_RecursiveMutexUnlock(&impl->lock);
}
#endif
*/
/*
static void player_handler(event_msg *msg)
{
    app_player *impl = get_player();
    player_events state = (player_events)APLAYER_GET_STATE(msg->data);

	pthread_mutex_lock(&impl->lock);
    if (APLAYER_GET_ID(msg->data) != impl->id) {
		printf("error<player_handler>:APLAYER_GET_ID != impl->id\n");
		pthread_mutex_unlock(&impl->lock);
        return;
    }
	pthread_mutex_unlock(&impl->lock);
    player_handler_preprocess(impl, state);
}
*/

static int player_callback(void* pUserData, int msg, int ext1, void* param)
{
    app_player* impl = (app_player*)pUserData;

	switch(msg)
    {
        case AWPLAYER_MEDIA_INFO:
            switch(ext1)
            {
                case AW_MEDIA_INFO_NOT_SEEKABLE:
                    PLAYER_LOGI("media source is unseekable.");
                    break;
            }
            break;

        case AWPLAYER_MEDIA_ERROR:
            PLAYER_LOGW("open media source fail.\n"
                            "reason: maybe the network is bad, or the music file is not good.");
            //sys_handler_send(handler, APLAYER_ID_AND_STATE(impl->id, PLAYER_EVENTS_MEDIA_ERROR), 10000);
            break;

        case AWPLAYER_MEDIA_PREPARED:
            //sys_handler_send(handler, APLAYER_ID_AND_STATE(impl->id, PLAYER_EVENTS_MEDIA_PREPARED), 10000);
            break;

        case AWPLAYER_MEDIA_PLAYBACK_COMPLETE:
            PLAYER_LOGI("playback complete.");
            //sys_handler_send(handler, APLAYER_ID_AND_STATE(impl->id, PLAYER_EVENTS_MEDIA_PLAYBACK_COMPLETE), 10000);
            break;

        case AWPLAYER_MEDIA_SEEK_COMPLETE:
            PLAYER_LOGI("seek ok.");
            break;

        default:
            PLAYER_LOGD("unknown callback from AwPlayer");
            break;
    }

    PLAYER_LOGI("cedarx cb complete.");
    return 0;
}

/* create the app player context */
player_base *player_create()
{

	app_player *impl = NULL;

	impl = malloc(sizeof(*impl));
	if (impl == NULL) {
		return NULL;
	}
	memset(impl, 0, sizeof(*impl));

	/* create XPlayer */
	impl->xplayer = XPlayerCreate();
	if (impl->xplayer == NULL) {
		printf("XPlayer Create failed. exit!\n");
		goto err0;
	}

	/*set callback to player*/
    XPlayerSetNotifyCallback(impl->xplayer, player_callback, (void*)impl);

    /* check if the player work. */
    if (XPlayerInitCheck(impl->xplayer) != 0) {
        goto err1;
    }

	/* register the function of app_player */
    impl->base.play         = player_seturl;
	impl->base.stop         = player_stop;
	impl->base.pause        = player_pause;
    impl->base.resume       = player_resume;
    impl->base.seek         = player_seek;
    impl->base.tell         = player_tell;
    impl->base.size         = player_size;
    impl->base.setvol       = player_setvol;
    impl->base.getvol       = player_getvol;
    impl->base.mute         = player_mute;
    impl->base.is_mute      = player_is_mute;
    impl->base.control      = player_control;
#if APP_PLAYER_SUPPORT_TONE
    impl->base.play_tone    = player_playtone;
    impl->base.stop_tone    = player_stoptone;
#endif
    impl->base.set_callback = player_setcb;
    impl->base.get_status   = player_get_states;
    impl->cb                = player_null_callback;

	pthread_mutex_init(&impl->lock, NULL);

    impl->state = APLAYER_STATES_INIT;

    player_singleton = impl;

    return &impl->base;

    PLAYER_LOGE("create player failed, quit.");

err1:
    /* it will destroy SoundDevice too */
    XPlayerDestroy(impl->xplayer);
err0:
	free(impl);
	return NULL;
}

void player_destroy(player_base *base)
{
    app_player *impl = container_of(base, app_player, base);
    if (player_singleton == NULL)
        return;

    PLAYER_LOGI("destroy AwPlayer.");
    player_stop(base);
	pthread_mutex_destroy(&impl->lock);
    if (impl->xplayer != NULL)
        XPlayerDestroy(impl->xplayer);
    if (impl->info.url)
        free(impl->info.url);
    if (impl != NULL)
        free(impl);
    player_singleton = NULL;
}

static int player_seturl(player_base *base, const char *url)
{
    int ret = 0;
    app_player *impl = container_of(base, app_player, base);
    char *play_url = (char *)url;

    if (url == NULL)
        return -1;

	pthread_mutex_lock(&impl->lock);

#if APP_PLAYER_SUPPORT_TONE
    if (is_toning(impl))
    {
		pthread_mutex_unlock(&impl->lock);
		return -1;
    }
#endif

    impl->state = APLAYER_STATES_PREPARING;

    PLAYER_LOGI("request to play : %s", url);
#if APP_PLAYER_SUPPORT_TONE
    impl->info.pause_time = 0;
#endif
    impl->info.url = wrap_realloc(impl->info.url, &impl->info.size, strlen(play_url) + 1);
    memcpy(impl->info.url, play_url, strlen(play_url) + 1);
    ret = set_url(impl, impl->info.url);
    if (ret) {
		printf("error>>>player_seturl>>>ret:%d\n", ret);
        reset(impl);
        impl->state = APLAYER_STATES_STOPPED;
    }

	pthread_mutex_unlock(&impl->lock);

	return ret;
}

static int player_stop(player_base *base)
{
    app_player *impl = container_of(base, app_player, base);

	pthread_mutex_lock(&impl->lock);
#if APP_PLAYER_SUPPORT_TONE
    if (is_toning(impl))
    {
		pthread_mutex_unlock(&impl->lock);
        return -1;
    }
#endif
    reset(impl);
    impl->state = APLAYER_STATES_STOPPED;

	PLAYER_LOGI("done\n");
	pthread_mutex_unlock(&impl->lock);
	usleep(10*1000);

    return 0;
}

static int player_pause(player_base *base)
{
    app_player *impl = container_of(base, app_player, base);
    int ret = 0;

	pthread_mutex_lock(&impl->lock);
//    OS_RecursiveMutexLock(&impl->lock, OS_WAIT_FOREVER);
#if APP_PLAYER_SUPPORT_TONE
    if (is_toning(impl))
    {
		pthread_mutex_unlock(&impl->lock);
        //OS_RecursiveMutexUnlock(&impl->lock);
        return -1;
    }
#endif
    ret = XPlayerPause(impl->xplayer);
    impl->state = APLAYER_STATES_PAUSE;
//    OS_RecursiveMutexUnlock(&impl->lock);
	pthread_mutex_unlock(&impl->lock);
    return ret;
}

static int player_resume(player_base *base)
{
    app_player *impl = container_of(base, app_player, base);
    int ret = 0;

	pthread_mutex_lock(&impl->lock);
//	OS_RecursiveMutexLock(&impl->lock, OS_WAIT_FOREVER);
#if APP_PLAYER_SUPPORT_TONE
    if (is_toning(impl))
    {
//        OS_RecursiveMutexUnlock(&impl->lock);
		pthread_mutex_unlock(&impl->lock);
		return -1;
    }
#endif
    ret = play(impl);

#if APP_PLAYER_SUPPORT_TONE
    impl->info.pause_time = 0;
#endif
    impl->state = APLAYER_STATES_PLAYING;
//    OS_RecursiveMutexUnlock(&impl->lock);
	pthread_mutex_unlock(&impl->lock);

    return ret;
}

static int player_seek(player_base *base, int ms)
{
    app_player *impl = container_of(base, app_player, base);
    int ret = 0;

	pthread_mutex_lock(&impl->lock);
   // OS_RecursiveMutexLock(&impl->lock, OS_WAIT_FOREVER);
#if APP_PLAYER_SUPPORT_TONE
    if (is_toning(impl))
    {
		pthread_mutex_unlock(&impl->lock);
        //OS_RecursiveMutexUnlock(&impl->lock);
        return -1;
    }

    impl->info.pause_time = ms;
#endif

    if(XPlayerSeekTo(impl->xplayer, ms) != 0)
    {
        PLAYER_LOGE("seek() return fail.");
        //OS_RecursiveMutexUnlock(&impl->lock);
        pthread_mutex_unlock(&impl->lock);
		return -1;
    }
    PLAYER_LOGI("seek to %d ms.", ms);
   // OS_RecursiveMutexUnlock(&impl->lock);
	pthread_mutex_unlock(&impl->lock);
    return ret;
}

static int player_tell(player_base *base)
{
    app_player *impl = container_of(base, app_player, base);
    int ms;

	pthread_mutex_lock(&impl->lock);
//    OS_RecursiveMutexLock(&impl->lock, OS_WAIT_FOREVER);
    if(XPlayerGetCurrentPosition(impl->xplayer, &ms) != 0)
    {
        PLAYER_LOGW("tell() return fail.");
		pthread_mutex_unlock(&impl->lock);
//		OS_RecursiveMutexUnlock(&impl->lock);
        return 0;
    }
    PLAYER_LOGI("tell to %d ms.", ms);
	pthread_mutex_unlock(&impl->lock);
//    OS_RecursiveMutexUnlock(&impl->lock);
    return ms;
}

static int player_size(player_base *base)
{
    app_player *impl = container_of(base, app_player, base);
    int ms;

	pthread_mutex_lock(&impl->lock);
//    OS_RecursiveMutexLock(&impl->lock, OS_WAIT_FOREVER);
    if(XPlayerGetDuration(impl->xplayer, &ms) != 0)
    {
        PLAYER_LOGW("size() return fail.");
//        OS_RecursiveMutexUnlock(&impl->lock);
		pthread_mutex_unlock(&impl->lock);
        return 0;
    }
    PLAYER_LOGI("size to %d ms.", ms);
	pthread_mutex_unlock(&impl->lock);
//    OS_RecursiveMutexUnlock(&impl->lock);
    return ms;
}

static int player_setvol(player_base *base, int vol)
{
    app_player *impl = container_of(base, app_player, base);

    if (vol > 31)
    {
        PLAYER_LOGW("set vol %d larger than 31", vol);
        vol = 31;
    }
    else if (vol < 0)
    {
        PLAYER_LOGW("set vol %d lesser than 0", vol);
        vol = 0;
    }

    impl->vol = vol;
	/*
    audio_manager_handler(AUDIO_SND_CARD_DEFAULT, AUDIO_MANAGER_SET_VOLUME_LEVEL, AUDIO_OUT_DEV_SPK, vol);
    */
	return 0;
}

static int player_getvol(player_base *base)
{
    app_player *impl = container_of(base, app_player, base);
    return impl->vol;
}

static int player_mute(player_base *base, bool is_mute)
{
    app_player *impl = container_of(base, app_player, base);
    impl->mute = (uint8_t)is_mute;
	/*
    audio_manager_handler(AUDIO_SND_CARD_DEFAULT, AUDIO_MANAGER_SET_MUTE, AUDIO_OUT_DEV_SPK, is_mute);
    */
	return 0;
}

static int player_is_mute(player_base *base)
{
    app_player *impl = container_of(base, app_player, base);
    return impl->mute;
}

static aplayer_states player_get_states(player_base *base)
{
    app_player *impl = container_of(base, app_player, base);

    return impl->state;
}

static int player_control(player_base *base, player_cmd command, void *data)
{
    app_player *impl = container_of(base, app_player, base);
    switch (command) {
    case PLAYER_CMD_SET_OUTPUT_CONFIG:
        //RTSoundDeviceControl(impl->sound, SOUND_CONTROL_SET_OUTPUT_CONFIG, data);
        RTSoundDeviceControl(impl->sound, 0, data);
        break;
		/*
    case PLAYER_CMD_ADD_OUTPUT_CONFIG:
        RTSoundDeviceControl(impl->sound, SOUND_CONTROL_ADD_OUTPUT_CONFIG, data);
        break;
    case PLAYER_CMD_CLEAR_OUTPUT_CONFIG:
        RTSoundDeviceControl(impl->sound, SOUND_CONTROL_CLEAR_OUTPUT_CONFIG, data);
        break;
    case PLAYER_CMD_SET_EQ_MODE:
        RTSoundDeviceControl(impl->sound, SOUND_CONTROL_SET_EQ_MODE, data);
        break;
    case PLAYER_CMD_CLEAR_EQ_MODE:
        RTSoundDeviceControl(impl->sound, SOUND_CONTROL_CLEAR_EQ_MODE, data);
        break;
		*/
    default:
        break;
    }
    return 0;
}

#if APP_PLAYER_SUPPORT_TONE
static int player_playtone(player_base *base, tone_base *drv, char *url)
{
    app_player *impl = container_of(base, app_player, base);
    int ret = 0;

	pthread_mutex_lock(&impl->lock);
//    OS_RecursiveMutexLock(&impl->lock, OS_WAIT_FOREVER);
    if (is_toning(impl))
    {
//        OS_RecursiveMutexUnlock(&impl->lock);
		pthread_mutex_unlock(&impl->lock);
		return -1;
    }
    set_current_time(impl);
    reset(impl);
    impl->tone = drv;
    ret = drv->play(drv, url);
//    OS_RecursiveMutexUnlock(&impl->lock);
	pthread_mutex_unlock(&impl->lock);
    /* TODO: how to resume music if use other tone drv? */

    return ret;
}

static int player_stoptone(player_base *base, tone_base *drv)
{
    app_player *impl = container_of(base, app_player, base);

	pthread_mutex_lock(&impl->lock);
    //OS_RecursiveMutexLock(&impl->lock, OS_WAIT_FOREVER);
    if (!is_toning(impl))
    {
//        OS_RecursiveMutexUnlock(&impl->lock);
		pthread_mutex_unlock(&impl->lock);
        return -1;
    }
    drv->stop(drv);
//    OS_RecursiveMutexUnlock(&impl->lock);
	pthread_mutex_lock(&impl->lock);

    return 0;
}
#endif

static void player_null_callback(player_events event, void *data, void *arg)
{
    PLAYER_LOGI("cb event:%d", event);
}

static void player_setcb(player_base *base, app_player_callback cb, void *arg)
{
    app_player *impl = container_of(base, app_player, base);
    if (!cb)
        impl->cb = player_null_callback;
    else
        impl->cb = cb;
    impl->arg = arg;
}

void player_setloop(player_base *base, int flag)
{
	app_player *impl = container_of(base, app_player, base);
	XPlayerSetLooping(impl->xplayer, flag);
	impl->isLooping = flag;
	return;
}

int player_isLooping(player_base *base)
{
	app_player *impl = container_of(base, app_player, base);
	return impl->isLooping;
}
