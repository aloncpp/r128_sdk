#ifndef __BT_A2DP_WORKER_H__
#define __BT_A2DP_WORKER_H__

#include <stdint.h>

#define A2DP_QUEUE_LEN 36
#define A2DP_QUEUE_SZIE 1

#define AVK_PCM_BUFF_SIZE 8192
#define AVK_PCM_PERIOD_SIZE 1024

#define PCM_FORMAT SND_PCM_FORMAT_S16_LE
#define PCM_STEAM SND_PCM_STREAM_PLAYBACK

#define	BT_MIN(a,b) (((a)<(b))?(a):(b))


struct a2dp_transport {
	uint32_t sampling;
	uint32_t channels;
};

enum MsgQueue{
	MSG_A2DP_STREAM = 0,
	MSG_A2DP_EXIT,
};

uint8_t aw_bt_a2dp_worker_signal(enum MsgQueue msg);

void aw_bt_a2dp_worker_stop(void *arg);

uint8_t aw_bt_a2dp_worker_start(void *arg);

int32_t aw_bt_a2dp_output_pcm(uint8_t *buf,uint32_t len);

#endif
