#include <stdlib.h>
#include <unistd.h>
#include "string.h"
#include "kfifoqueue.h"
#include "audiofifo.h"
#include "player_app.h"
#include "xplayer.h"	/*for get the pthread sched struct*/

//#define LOOP_CNT 5

typedef enum audio_status
{
    AUDIO_STATUS_PLAYING,
    AUDIO_STATUS_STOPPED,
    AUDIO_STATUS_ERROR,
} audio_status;

struct AudioFifoImpl {
    struct AudioFifoS base; /* it must be first */
    struct CdxFifoStreamS *fifobase;
    //OS_Mutex_t   audio_mutex;
    pthread_mutex_t   audio_mutex;
    pthread_t  start_thread;
    audio_status status;
    player_base *player;
    unsigned int in_size;
    char has_play;
    char has_free;
};

static void player_play_callback(player_events event, void *data, void *arg)
{
    struct AudioFifoImpl *impl;
    impl = (struct AudioFifoImpl *)arg;

    printf("%s event:%d\n", __func__, event);

    switch (event) {
    case PLAYER_EVENTS_MEDIA_STOPPED:
    case PLAYER_EVENTS_MEDIA_ERROR:
    case PLAYER_EVENTS_MEDIA_PLAYBACK_COMPLETE:
        impl->status = AUDIO_STATUS_STOPPED;
        break;
    default:
        break;
    }
}
static int player_play_start(struct AudioFifoImpl *impl)
{
    char url[32];
    sprintf(url, "fifo://%p", impl->fifobase);
    impl->player->set_callback(impl->player, player_play_callback, impl);
	return impl->player->play(impl->player, (const char *)url);
}

static int player_play_stop(struct AudioFifoImpl *impl)
{
    impl->player->stop(impl->player);
    return 0;
}

static void *audio_play_task(void *arg)
{
    int ret;
    struct AudioFifoImpl *impl;

    impl = (struct AudioFifoImpl *)arg;

	ret = player_play_start(impl);
    if (ret) {
		printf("error<audio_play_task>AUDIO_STATUS_ERROR\n");
        impl->status = AUDIO_STATUS_ERROR;
    }
	return NULL;
}
/* regist to AudioFifoSetPlayer --> .set_player --> audio_fifo_set_player  */
static int audio_fifo_set_player(struct AudioFifoS *audiofifo, player_base *player)
{
    struct AudioFifoImpl *impl;

    impl = (struct AudioFifoImpl *)audiofifo;
    impl->player = player;
    return 0;
}

static int audio_fifo_start(struct AudioFifoS *audiofifo)
{
    struct AudioFifoImpl *impl;

    impl = (struct AudioFifoImpl *)audiofifo;

    pthread_mutex_lock(&impl->audio_mutex);
	if (impl->status == AUDIO_STATUS_PLAYING) {
		printf("error<audio_fifo_start>impl->status == PLAYING\n");
		goto err;
    }

	/* create kfifo_stream */
    impl->fifobase = kfifo_stream_create();
    if (impl->fifobase == NULL) {
		printf("error<audio_fifo_start>fifobase == NULL \n");
		goto err;
    }

    impl->status = AUDIO_STATUS_PLAYING;
    impl->has_play = 0;
    impl->in_size = 0;
    impl->has_free = 0;
    pthread_mutex_unlock(&impl->audio_mutex);
	return 0;

err:
    pthread_mutex_unlock(&impl->audio_mutex);
    return -1;
}


static int audio_fifo_put_data(struct AudioFifoS *audiofifo, void *inData, int dataLen)
{
    uint32_t avail;
    uint32_t in_len;
    uint32_t reserve_len;
    uint32_t has_in_len = 0;
    struct AudioFifoImpl *impl;

    impl = (struct AudioFifoImpl *)audiofifo;

	pthread_mutex_lock(&impl->audio_mutex);
	if (impl->status != AUDIO_STATUS_PLAYING) {
		printf("error<audio_fifo_put_data>:status != PLAYING\n");
		pthread_mutex_unlock(&impl->audio_mutex);
        return 0;
    }

    while ((has_in_len != dataLen) && (impl->status == AUDIO_STATUS_PLAYING)) {
        reserve_len = dataLen - has_in_len;
        CdxFifoStreamLock(impl->fifobase);
        avail = CdxFifoStreamAvail(impl->fifobase);
        in_len = avail > reserve_len ? reserve_len : avail;
        CdxFifoStreamIn(impl->fifobase, (char *)inData + has_in_len, in_len);
        has_in_len += in_len;
        impl->in_size += in_len;
        CdxFifoStreamUnlock(impl->fifobase);

		/* notes: create the audio_play_task thread */
		if ((impl->in_size >= (4 * 1024)) && (impl->has_play == 0)) {  /* it must be 4k */
			pthread_attr_t attr;
			pthread_attr_init(&attr);
			struct sched_param sched;
			sched.sched_priority = 3; /*the OS set 3*/
			pthread_attr_setschedparam(&attr, &sched);
			pthread_attr_setstacksize(&attr, 32768); /* OS SET 1024*/
			pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

			if (pthread_create(&impl->start_thread, &attr, audio_play_task, impl)) {
				printf("<<play_fifo>>audio play task create failed.\n");
                impl->status = AUDIO_STATUS_ERROR;
			}
			pthread_setname_np(&impl->start_thread, "AudioPlayTaskThread");
			impl->has_play = 1;
		} /* end of create the audio_play_task thread */

        if (has_in_len != dataLen) {
			usleep(10*1000);
		}
    }
	pthread_mutex_unlock(&impl->audio_mutex);
    return has_in_len;
}


static int audio_fifo_stop(struct AudioFifoS *audiofifo, bool stop_immediately)
{
    int ret;
    unsigned int waittime = 0;
    struct AudioFifoImpl *impl;

    impl = (struct AudioFifoImpl *)audiofifo;

	pthread_mutex_lock(&impl->audio_mutex);
	if (impl->status != AUDIO_STATUS_PLAYING) {
        goto err;
    }
    CdxFifoStreamSeteos(impl->fifobase);

	if (stop_immediately) {
        printf("stop immediately\n");
        player_play_stop(impl);
    } else {
        if (impl->has_play == 0) {
            ret = player_play_start(impl);
            if (ret) {
                impl->status = AUDIO_STATUS_ERROR;
            }
        }
        while ((impl->status == AUDIO_STATUS_PLAYING) && (CdxFifoStreamValid(impl->fifobase) != 0)) {
			usleep(100*1000);
		}
        while ((impl->status == AUDIO_STATUS_PLAYING) && (waittime < 5000)) {
            usleep(200*1000);
            waittime += 200;
        }
        if (impl->status == AUDIO_STATUS_PLAYING)
            player_play_stop(impl);
    }

err:
    if (!impl->has_free) {
        impl->status = AUDIO_STATUS_STOPPED;
        impl->has_free = 1;
        kfifo_stream_destroy(impl->fifobase);
    }
//    OS_MutexUnlock(&impl->audio_mutex);

	pthread_mutex_unlock(&impl->audio_mutex);

	return 0;
}

static const struct AudioFifoOpsS AudioFifoOps = {
    .set_player = audio_fifo_set_player,
    .start      = audio_fifo_start,
    .put_data   = audio_fifo_put_data,
    .stop       = audio_fifo_stop,
};


struct AudioFifoS *audio_fifo_create()
{
    struct AudioFifoImpl *impl;

    impl = malloc(sizeof(*impl));
    if (impl == NULL) {
        printf("error<audio_fifo_create>fail\n");
        return NULL;
    }
    memset(impl, 0, sizeof(*impl));

    impl->status = AUDIO_STATUS_STOPPED;
    impl->has_free = 1;
    impl->base.ops = &AudioFifoOps;		/* regist the function */
	pthread_mutex_init(&impl->audio_mutex, NULL);

    return &impl->base;
}

int audio_fifo_destroy(struct AudioFifoS *audiofifo)
{
    struct AudioFifoImpl *impl;

    impl = (struct AudioFifoImpl *)audiofifo;

	pthread_mutex_destroy(&impl->audio_mutex);
    free(impl);
	printf("< %s : %d > done\n", __func__, __LINE__);
    return 0;
}

int audio_fifo_looping(struct AudioFifoS *audiofifo, int loop_cnt)
{

    int ret;
    struct AudioFifoImpl *impl;
	int eos = 0;
    impl = (struct AudioFifoImpl *)audiofifo;
	int cnt = 0;
	int isLooping = 0;
	pthread_mutex_lock(&impl->audio_mutex);
	CdxFifoStreamSeteos(impl->fifobase);
	/*for loop playback*/
	while (1) {
		usleep(500*1000);
		CdxFifoStreamLock(impl->fifobase);
        eos = CdxFifoStreamIseos(impl->fifobase);
		CdxFifoStreamUnlock(impl->fifobase);
		if (!eos) {
			CdxFifoStreamSeteos(impl->fifobase);
			cnt++;
	//		printf("<%s : %d > cnt:%d\n", __func__,__LINE__, cnt);
		}
		isLooping = player_isLooping(impl->player);

		if (cnt == loop_cnt || isLooping == 0) {
			break;
		}
	}/*for loop playback end*/

	pthread_mutex_unlock(&impl->audio_mutex);

	return 0;
}
