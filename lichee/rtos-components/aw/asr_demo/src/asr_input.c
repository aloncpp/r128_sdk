#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <hal_mem.h>
#include <hal_sem.h>
#include <hal_thread.h>
#include <AudioSystem.h>
#include "asr_input.h"

#define AUDIO_BUF_SIZE      (320 * 3)       // 320*n 1<n<=10
#define SEM_TIMEOUT_MS		(1000)

struct asr_input_t {
	volatile uint32_t run;
	uint32_t rate;
	uint8_t ch;
	uint8_t bit;

	asr_data_send_t data_send;
	void *priv;
	hal_sem_t sem;
	void *thread;
};

void asr_input_thread(void *arg)
{
	struct asr_input_t *hdl = (struct asr_input_t *)arg;
	unsigned char buf[AUDIO_BUF_SIZE];
	int exit = 0;

	tAudioRecord *ar = AudioRecordCreate("default");
	if (!ar) {
		printf("AudioRecordCreate failed\n");
		goto err1;
	}

	AudioRecordSetup(ar, hdl->rate, hdl->ch, hdl->bit);
	AudioRecordStart(ar);

	if (hal_sem_post(hdl->sem))
		printf("hal_sem_post failed!\n");

	while(hdl->run) {
		int read_size = AudioRecordRead(ar, buf, AUDIO_BUF_SIZE);
		if (hdl->run && read_size > 0) {
			hdl->data_send(hdl->priv, buf, read_size);
		}
	}

	exit = 1;
	AudioRecordStop(ar);
	printf("%s stop\n", __func__);
err1:
	if(ar)
		AudioRecordDestroy(ar);
	if(!exit) {
		hdl->run = 0;
	}
	if (hal_sem_post(hdl->sem))
		printf("hal_sem_post failed!\n");
	hal_thread_stop(NULL);
	printf("%s exit\n", __func__);
}

int asr_input_start(void *_hdl)
{
	struct asr_input_t *hdl = (struct asr_input_t *)_hdl;

	printf("%s:%d\n", __func__, __LINE__);

	hdl->run = 1;
	hdl->thread = hal_thread_create(asr_input_thread, hdl, "asr_input_thread", 8192 + (AUDIO_BUF_SIZE + 4 - 1) / 4 , 5);
	hal_thread_start(hdl->thread);

	if (!hdl->thread) {
		printf("no memory!\n");
		goto err;
	}

	while (0 > hal_sem_timedwait(hdl->sem, pdMS_TO_TICKS(SEM_TIMEOUT_MS))) {
		printf("wait timeout!\n");
		//goto err;
	}

	if(!hdl->run) {
		printf("thread error!\n");
		goto err;
	}

	return 0;
err:
	return -1;
}

int asr_input_is_stop(void *_hdl)
{
	struct asr_input_t *hdl = (struct asr_input_t *)_hdl;

	return (hdl->run == 0) ? 1 : 0;
}

int asr_input_stop(void *_hdl, int force)
{
	struct asr_input_t *hdl = (struct asr_input_t *)_hdl;

	printf("%s:%d\n", __func__, __LINE__);

	hdl->run = 0;

	if (0 > hal_sem_timedwait(hdl->sem, pdMS_TO_TICKS(SEM_TIMEOUT_MS))) {
		printf("wait timeout!\n");
		goto err;
	}

	return 0;
err:
	return -1;
}

void asr_input_destroy(void *_hdl)
{
	struct asr_input_t *hdl = (struct asr_input_t *)_hdl;

	printf("%s:%d\n", __func__, __LINE__);
	if (hdl) {
		if (hdl->sem) {
			hal_sem_delete(hdl->sem);
			hdl->sem = NULL;
		}

		hal_free(hdl);
	}
}

void *asr_input_create(const struct asr_input_config_t *config)
{
	struct asr_input_t *hdl = hal_malloc(sizeof(*hdl));

	printf("%s:%d\n", __func__, __LINE__);

	if (!hdl) {
		printf("no memory!\n");
		goto err;
	}
	memset(hdl, 0, sizeof(*hdl));

	hdl->sem = hal_sem_create(0);
	if (!hdl->sem) {
		printf("no memory!\n");
		goto err;
	}

	hdl->rate = config->rate;
	hdl->ch = config->ch;
	hdl->bit = config->bit;

	hdl->data_send = config->data_send;
	hdl->priv = config->priv;

	return hdl;
err:
	asr_input_destroy(hdl);
	return NULL;
}
