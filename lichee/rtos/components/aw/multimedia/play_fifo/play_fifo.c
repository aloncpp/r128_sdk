#include <stdio.h>
#include <stdint.h>
#include <FreeRTOS.h>
#include <task.h>
#include <portable.h>
#include <string.h>
#include <console.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <mqueue.h>
#include <fcntl.h>
#include <stdlib.h>
#include "FreeRTOS_POSIX/utils.h"
#include "rtplayer.h"
#include "xplayer.h"
#include "player_app.h"
#include "audiofifo.h"

#define LOGD(msg, arg...)      //printf("[PLAYER_DBG] <%s : %d> " msg "\n", __func__, __LINE__, ##arg)
#define LOGI(msg, arg...)      printf("[PLAYER_INFO] <%s : %d> " msg "\n", __func__, __LINE__, ##arg)
#define LOGW(msg, arg...)      printf("[PLAYER_WRN] <%s : %d> " msg "\n", __func__, __LINE__, ##arg)
#define LOGE(msg, arg...)      printf("[PLAYER_ERR] <%s : %d> " msg "\n", __func__, __LINE__, ##arg)


#define FIFO_LOOP 1
#define LOOP_CNT 5
#define DEAD_LOOP 1 /*0:disable the dead loop; 1: enable the dead loop*/

#define PLAYFIFO_DEFAULT_PRIORITY ((configMAX_PRIORITIES >> 1))
pthread_t play_task_threadID;
static player_base *player;

static int play_fifo_music()
{
	//FIL fp;
	FILE *fp;
    int ret = 0;
    //FRESULT result;
    void *file_buffer;
    unsigned int act_read;
    struct AudioFifoS *audiofifo;

	/* create audiofifo struct */
	audiofifo = audio_fifo_create();
    if (audiofifo == NULL) {
        printf("Error:audio_fifo_create failed\n");
		return -1;
    }

    file_buffer = (void *)malloc(1024);
    if (file_buffer == NULL) {
		LOGE("file buffer malloc fail");
		ret = -1;
        goto err1;
    }

	fp = fopen("/data/boot.mp3", "rb+");
	if (fp == NULL) {
		LOGE("error:fopen failed");
		ret = -1;
		goto err2;
	}
#if FIFO_LOOP
	player_setloop(player, 1);
#endif
	AudioFifoSetPlayer(audiofifo, player);
	AudioFifoStart(audiofifo);
	int i = 0;
	while (1) {
		act_read = fread(file_buffer, 1, 1024, fp);
        AudioFifoPutData(audiofifo, file_buffer, act_read);
		i++;
        if (act_read != 1024) {
			LOGI("fread_cnt:%d, act_read:%d", i, act_read);
			break;
		}
    }
#if FIFO_LOOP
#if DEAD_LOOP
	while(1) {
		ret = audio_fifo_looping(audiofifo, 1000);
	}
#else
	ret = audio_fifo_looping(audiofifo, LOOP_CNT);
	player_setloop(player, 0);
#endif
#endif
	AudioFifoStop(audiofifo, false);
	fclose(fp);

err2:
	free(file_buffer);
err1:
    audio_fifo_destroy(audiofifo);
    return ret;

}

static void* play_task(void* arg)
{
	int ret = 0;
	/*create player_app's struct of player */
	player = player_create();
	if (player == NULL) {
		LOGE("error: player_create fail");
		return NULL;
	}

	int i = 0;
	while(i < 1) {
	/* try to play fifo music */
		LOGI("play_fifo_music");
		ret = play_fifo_music();
		i++;
	}
	player_destroy(player);

	return (void *)(intptr_t)ret;
}


static int cmd_play_fifo(int argc, char **argv)
{
	const char *url = argv[1];

	/* create the play task Thread  */
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	struct sched_param sched;
	sched.sched_priority = PLAYFIFO_DEFAULT_PRIORITY;
	pthread_attr_setschedparam(&attr, &sched);
	pthread_attr_setstacksize(&attr, 32768);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	if (pthread_create(&play_task_threadID, &attr, play_task, NULL)) {
		LOGE("play task create failed. exit!");
		return -1;
	}
	pthread_setname_np(play_task_threadID, "PlayTaskThread");

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_play_fifo, play_fifo, play_fifo_demo);

static int cmd_exit_fifo(int arc, char **argv)
{
	player_setloop(player, 0);
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_exit_fifo, exit_fifo, exit_fifo_demo);
