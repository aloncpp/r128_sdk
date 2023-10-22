#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hal_sem.h>
#include <hal_mem.h>
#include <hal_mutex.h>
//#include <hal_time.h>
#include <hal_thread.h>
#include <rpdata.h>
#include <hal_time.h>
#include "ring.h"
#include "dsp_controller.h"
#include "crc32.h"
#include "asr_resources.h"
#include "asr_demo_config.h"

#define SEM_TIMEOUT_MS			(2000)

struct dsp_controller_t {
	volatile int run;
	unsigned int cnt;
	unsigned int wakeup_cnt;
	int enable_vad;
	void *thread;
	rpdata_t *rpd_ctl;
	void *rpd_ctl_buffer;
	rpdata_t *rpd_data;
	void *rpd_data_buffer;
	rpdata_t *rpd_vad;
	void *rpd_vad_buffer;
	hal_sem_t sem;
	hal_mutex_t mutex;

	audio_data_send_t audio_data_send;
	audio_raw_data_send_t audio_raw_data_send;
	word_id_send_t word_id_send;
	void *ring;
	int vad_bos_buffer_len;
	unsigned int timeout_ms;
	void *priv;

	int last_vad_flag;
	int last_enable_vad;
	int send_eos;
};

static inline void word_id_send_nolock(struct dsp_controller_t *hdl, int word_id)
{
	unsigned int cnt = ++hdl->cnt;

	hdl->word_id_send(hdl->priv, word_id, (unsigned long int)cnt);
	if (hdl->timeout_ms) {
		if (word_id == CMD_ID_WAKEUP) {
			hdl->wakeup_cnt = cnt;
		}
	}
}

static int rpd_data_recv_cb(rpdata_t *rpd, void *data, unsigned int data_len)
{
	struct dsp_controller_t *hdl = (struct dsp_controller_t *)rpdata_get_private_data(rpd);
	int vad_flag = *(int *)((char *)data + RPD_DATA_VAD_FLAG_OFFSET);
	int word_id = *(int *)((char *)data + RPD_DATA_WORD_ID_OFFSET);
	float confidence = *(float *)((char *)data + RPD_DATA_CONFIDENCE_OFFSET);
	int send_vad_bos = 0;
	unsigned char *dsp_data = (unsigned char *)((char *)data + RPD_DATA_RAW_DATA_OFFSET);
	unsigned int dsp_data_len = RPD_DATA_RAW_DATA_SIZE;
	unsigned char *dsp_output_data = (unsigned char *)((char *)data + RPD_DATA_OUT_DATA_OFFSET);
	unsigned int dsp_output_data_len = RPD_DATA_OUT_DATA_SIZE;

	if (!hdl)
		return 0;

	if (hdl->last_enable_vad != hdl->enable_vad) {
		printf("enable_vad: %d -> %d\n", hdl->last_enable_vad, hdl->enable_vad);
		hdl->last_enable_vad = hdl->enable_vad;
		ring_recv_drop(hdl->ring);
		if (hdl->last_enable_vad == 0)
			hdl->send_eos = 1;
	}

	if (hdl->last_vad_flag != vad_flag) {
		printf("vad_flag: %d -> %d\n", hdl->last_vad_flag, vad_flag);
		hdl->last_vad_flag = vad_flag;
		if (vad_flag)
			send_vad_bos = 1;
		else
			hdl->send_eos = 1;
	}

	if (hdl->audio_raw_data_send && hdl->enable_vad)
		hdl->audio_raw_data_send(hdl->priv, vad_flag, dsp_data, dsp_data_len);

	if (hdl->audio_data_send && hdl->enable_vad && vad_flag) {
		if (hdl->ring && send_vad_bos) {
			//将vad=1前的数据读取出来并上报
			unsigned char tmp[DSP_ASR_FRAME_SIZE];
			int total = 0;
			int recv;
			while(recv = ring_recv_data(hdl->ring, tmp, DSP_ASR_FRAME_SIZE)) {
				hdl->audio_data_send(hdl->priv, tmp, recv);
				total += recv;
			}
			printf("VAD_BOS_BUFFER_SIZE: %d (%d ms)\n", total, FRAME_SIZE_TO_MS(total));
		}
		//将本次vad=1的数据上报
		hdl->audio_data_send(hdl->priv, dsp_output_data, dsp_output_data_len);
	} else if (hdl->ring && hdl->enable_vad) {
		//将vad=1前的数据送到ring_buffer中
		ring_send_data(hdl->ring, dsp_output_data, dsp_output_data_len);
	}

	if (hdl->word_id_send && word_id > 0) {
		hal_mutex_lock(hdl->mutex);
		word_id_send_nolock(hdl, word_id);
		hal_mutex_unlock(hdl->mutex);
	}

	return 0;
}

static struct rpdata_cbs rpd_data_cb = {
	.recv_cb = rpd_data_recv_cb,
};

static void rpd_data_deinit(struct dsp_controller_t *hdl)
{
	if (hdl->rpd_data) {
		rpdata_destroy(hdl->rpd_data);
		hdl->rpd_data = NULL;
	}
}

static int rpd_data_init(struct dsp_controller_t *hdl)
{
	printf("RPD_DATA_DIR:%d, RPD_DATA_TYPE:%s, RPD_DATA_NAME:%s\n", RPD_DATA_DIR, RPD_DATA_TYPE, RPD_DATA_NAME);

	hdl->rpd_data = rpdata_connect(RPD_DATA_DIR, RPD_DATA_TYPE, RPD_DATA_NAME);
	if (!hdl->rpd_data) {
		printf("rpdata_connect failed!\n");
		return -1;
	}

	hdl->rpd_data_buffer = rpdata_buffer_addr(hdl->rpd_data);
	if (!hdl->rpd_data_buffer) {
		printf("rpdata_buffer_addr failed!\n");
		rpd_data_deinit(hdl);
		return -1;
	}

	rpdata_set_private_data(hdl->rpd_data, hdl);
	rpdata_set_recv_cb(hdl->rpd_data, &rpd_data_cb);
	return 0;
}

static void rpd_vad_deinit(struct dsp_controller_t *hdl)
{
	if (hdl->rpd_vad) {
		rpdata_destroy(hdl->rpd_vad);
		hdl->rpd_vad = NULL;
		hdl->rpd_vad_buffer = NULL;
	}
}

static int rpd_vad_init(struct dsp_controller_t *hdl)
{
	printf("RPD_VAD_DIR:%d, RPD_VAD_TYPE:%s, RPD_VAD_NAME:%s\n", RPD_VAD_DIR, RPD_VAD_TYPE, RPD_VAD_NAME);

	hdl->rpd_vad = rpdata_create(RPD_VAD_DIR, RPD_VAD_TYPE, RPD_VAD_NAME, RPD_VAD_SEND_SIZE);
	if (!hdl->rpd_vad) {
		printf("rpdata_connect failed!\n");
		goto err;
	}

	hdl->rpd_vad_buffer = rpdata_buffer_addr(hdl->rpd_vad);
	if (!hdl->rpd_vad_buffer) {
		printf("rpdata_buffer_addr failed!\n");
		goto err;
	}

	return 0;
err:
	rpd_vad_deinit(hdl);
	return -1;
}

static int rpd_vad_set_vad_enable(struct dsp_controller_t *hdl, int val)
{
	unsigned int*vad_enable = (unsigned int *)((char *)hdl->rpd_vad_buffer + RPD_VAD_ENABLE_OFFSET);

	*vad_enable = val;

	rpdata_wait_connect(hdl->rpd_vad);

	return rpdata_send(hdl->rpd_vad, 0, RPD_VAD_SEND_SIZE);
}

static void rpd_ctl_deinit(struct dsp_controller_t *hdl)
{
	if (hdl->rpd_ctl) {
		rpdata_destroy(hdl->rpd_ctl);
		hdl->rpd_ctl = NULL;
		hdl->rpd_ctl_buffer = NULL;
	}
}

static int rpd_ctl_init(struct dsp_controller_t *hdl)
{
	printf("RPD_CTL dir:%d, type:%s, name:%s\n", RPD_CTL_DIR, RPD_CTL_TYPE, RPD_CTL_NAME);

	hdl->rpd_ctl = rpdata_create(RPD_CTL_DIR, RPD_CTL_TYPE, RPD_CTL_NAME, RPD_CTL_SEND_SIZE);
	if (!hdl->rpd_ctl) {
		printf("rpdata_connect failed!\n");
		goto err;
	}

	hdl->rpd_ctl_buffer = rpdata_buffer_addr(hdl->rpd_ctl);
	if (!hdl->rpd_ctl_buffer) {
		printf("rpdata_buffer_addr failed!\n");
		goto err;
	}

	return 0;
err:
	rpd_ctl_deinit(hdl);
	return -1;
}

static int rpd_ctl_send_cmd(struct dsp_controller_t *hdl, unsigned int cmd)
{
	char *dst = (char *)hdl->rpd_ctl_buffer + RPD_CTL_CMD_OFFSET;
	dsp_ctrl_init_t ctrl_cmd = {
		.cmd = cmd
	};

	memcpy(dst + RPD_CTL_CMD_OFFSET, &ctrl_cmd, RPD_CTL_CMD_SIZE);

	rpdata_wait_connect(hdl->rpd_ctl);

	return rpdata_send(hdl->rpd_ctl, 0, RPD_CTL_SEND_SIZE);
}

static void dsp_controller_thread(void *arg)
{
	struct dsp_controller_t *hdl = (struct dsp_controller_t *)arg;
	int exit = 0;
	unsigned int wakeup_cnt = 0;
	unsigned int time_ms = 0;
	unsigned int active = 0;

	if (0 > rpd_data_init(hdl)) {
		printf("rpd_data_init failed!\n");
		goto err;
	}

	if (0 > rpd_vad_init(hdl)) {
		printf("rpd_vad_init failed!\n");
		goto err;
	}

	if (hal_sem_post(hdl->sem))
		printf("hal_sem_post failed!\n");

	while(hdl->run) {
		hal_msleep(50);
		if (!hdl->timeout_ms)
			continue;
		hal_mutex_lock(hdl->mutex);
		if (wakeup_cnt != hdl->wakeup_cnt) {
			wakeup_cnt = hdl->wakeup_cnt;
			active = 1;
			time_ms = 0;
			printf("active wakeup timeout!\n");
		}
		hal_mutex_unlock(hdl->mutex);
		if (!active)
			continue;
		hal_mutex_lock(hdl->mutex);
		if (active && hdl->cnt != hdl->wakeup_cnt) {
			wakeup_cnt = hdl->wakeup_cnt;
			active = 0;
			time_ms = 0;
			printf("cancel wakeup timeout!\n");
		}
		hal_mutex_unlock(hdl->mutex);
		if (!active)
			continue;
		time_ms += 50;
		if (time_ms >= hdl->timeout_ms) {
			printf("wakeup timeout!\n");
			hal_mutex_lock(hdl->mutex);
			word_id_send_nolock(hdl, CMD_ID_TIMEOUT);
			wakeup_cnt = hdl->wakeup_cnt;
			hal_mutex_unlock(hdl->mutex);
			active = 0;
			time_ms = 0;
		}
	}

	exit = 1;
	printf("%s stop\n", __func__);
err:
	rpd_vad_deinit(hdl);
	rpd_data_deinit(hdl);
	if(!exit) {
		hdl->run = 0;
	}
	if (hal_sem_post(hdl->sem))
		printf("hal_sem_post failed!\n");
	hal_thread_stop(NULL);
	printf("%s exit\n", __func__);
}

void dsp_controller_destroy(void *_hdl)
{
	struct dsp_controller_t *hdl = hal_malloc(sizeof(*hdl));

	if (hdl) {
		rpd_ctl_deinit(hdl);
		if (hdl->mutex) {
			hal_mutex_delete(hdl->mutex);
			hdl->mutex = NULL;
		}
		if (hdl->sem) {
			hal_sem_delete(hdl->sem);
			hdl->sem = NULL;
		}
		hal_free(hdl);
	}
}

void *dsp_controller_create(struct dsp_controller_config_t *config)
{
	struct dsp_controller_t *hdl = hal_malloc(sizeof(*hdl));

	if (!hdl) {
		printf("no memory!\n");
		goto err;
	}
	memset(hdl, 0, sizeof(*hdl));
	hdl->audio_raw_data_send = config->audio_raw_data_send;
	hdl->audio_data_send = config->audio_data_send;
	hdl->word_id_send = config->word_id_send;
	hdl->vad_bos_buffer_len = config->vad_bos_buffer_len;
	hdl->priv = config->priv;
	hdl->timeout_ms = config->timeout_ms;
	hdl->cnt = 0;
	hdl->wakeup_cnt = 1;

	hdl->sem = hal_sem_create(0);
	if (!hdl->sem) {
		printf("no memory!\n");
		goto err;
	}

	hdl->mutex = hal_mutex_create();
	if (!hdl->mutex) {
		printf("no memory!\n");
		goto err;
	}

	if (hdl->vad_bos_buffer_len) {
		hdl->ring = ring_create(hdl->vad_bos_buffer_len, "vad_bos", NULL);
		if (!hdl->ring) {
			printf("ring_create failed!\n");
			goto err;
		}
	}

	if (0 > rpd_ctl_init(hdl)) {
		printf("rpd_ctl_init failed!\n");
		goto err;
	}

	return hdl;
err:
	dsp_controller_destroy(hdl);
	return NULL;
}

//rpccli dsp asr_test -m 3
int dsp_controller_stop(void *_hdl)
{
	struct dsp_controller_t *hdl = (struct dsp_controller_t *)_hdl;

	hdl->run = 0;
	rpd_ctl_send_cmd(hdl, DSP_STOP);

	while (0 > hal_sem_timedwait(hdl->sem, pdMS_TO_TICKS(SEM_TIMEOUT_MS))) {
		printf("%s wait timeout!\n", __func__);
	}

	return 0;
}
//rpccli dsp asr_test -m 0 -d 2 -t DSPtoRVAsr -j DSPtoRVAsrdump -e 1 -o 1
int dsp_controller_start(void *_hdl)
{
	struct dsp_controller_t *hdl = (struct dsp_controller_t *)_hdl;

	rpd_ctl_send_cmd(hdl, DSP_ENABLE_ALG);
#ifdef DSP_ASR_DUMP_INPUT_DATA
	rpd_ctl_send_cmd(hdl, DSP_DUMP_MERGE_DATA);
#endif
	rpd_ctl_send_cmd(hdl, DSP_START);

	hdl->run = 1;
	hdl->thread = hal_thread_create(dsp_controller_thread, hdl, "dsp_controller_thread", 1024, 5);
	hal_thread_start(hdl->thread);

	while (0 > hal_sem_timedwait(hdl->sem, pdMS_TO_TICKS(SEM_TIMEOUT_MS))) {
		printf("%s wait timeout!\n", __func__);
	}

	return 0;
}

int dsp_controller_get_vad_flag(void *_hdl)
{
	struct dsp_controller_t *hdl = (struct dsp_controller_t *)_hdl;

	return hdl->last_vad_flag;
}

int dsp_controller_get_eos(void *_hdl)
{
	struct dsp_controller_t *hdl = (struct dsp_controller_t *)_hdl;

	return hdl->send_eos;
}

int dsp_controller_enable_vad(void *_hdl)
{
	struct dsp_controller_t *hdl = (struct dsp_controller_t *)_hdl;

	if (hdl) {
		printf("vad_enable: 1\n");
		hdl->send_eos = 0;
		hdl->enable_vad = 1;
		if (rpd_vad_set_vad_enable(hdl, 1)) {
			printf("rpd_vad_set_vad_enable 1 failed!\n");
		}
	}

	return 0;
}

int dsp_controller_disable_vad(void *_hdl)
{
	struct dsp_controller_t *hdl = (struct dsp_controller_t *)_hdl;

	if (hdl) {
		if (rpd_vad_set_vad_enable(hdl, 0)) {
			printf("rpd_vad_set_vad_enable 0 failed!\n");
		}
		hdl->enable_vad = 0;
		printf("vad_enable: 0\n");
	}

	return 0;
}

int dsp_controller_get_enable_vad(void *_hdl)
{
	struct dsp_controller_t *hdl = (struct dsp_controller_t *)_hdl;

	return hdl->enable_vad;
}