#include <stdio.h>

#include "adt.h"
#include <aw_common.h>
#include <aw-alsa-lib/pcm.h>


#define ADT_QUEUE_SIZE		(8)

struct adt_pcm_queue {
	void *pcm_data;
	unsigned int length;
	unsigned int data_size;
	unsigned int used;
};

struct aw_data_transfer {
	snd_pcm_t *handle;
	struct adt_pcm_queue pcm_queue[ADT_QUEUE_SIZE];
	QueueHandle_t work_queue;
	/*QueueHandle_t mutex*/
	TaskHandle_t task;
};

#define ADT_CARD_NAME	"hw:snddaudio1"

extern int set_param(snd_pcm_t *handle, snd_pcm_format_t format,
			unsigned int rate, unsigned int channels,
			snd_pcm_uframes_t period_size,
			snd_pcm_uframes_t buffer_size);
extern int pcm_read(snd_pcm_t *handle, const char *data, snd_pcm_uframes_t frames_total,
		unsigned int frame_bytes);
extern int pcm_write(snd_pcm_t *handle, char *data, snd_pcm_uframes_t frames_total,
		unsigned int frame_bytes);

enum {
	ADT_QUEUE_SEND = 0,
	ADT_QUEUE_EXIT,
};

struct pcm_queue_item {
	struct adt_pcm_queue *queue;
	unsigned int cmd;
};

static void adt_task(void *arg)
{
	aw_data_transfer_t *adt = arg;
	BaseType_t xStatus = 0;
	const TickType_t xTicksToWait = portMAX_DELAY;
	struct pcm_queue_item item;

	while (1) {
		xStatus = xQueueReceive(adt->work_queue, &item, xTicksToWait);
		if (xStatus != pdPASS) {
			printf("xQueueReceive failed..\n");
			continue;
		}
		switch (item.cmd) {
		case ADT_QUEUE_SEND:
		{
			struct adt_pcm_queue *queue = item.queue;
			/* Note:len should be snd_pcm_frames_to_bytes(adt->handle, 1) align */
			pcm_write(adt->handle, queue->pcm_data,
				snd_pcm_bytes_to_frames(adt->handle, queue->data_size),
				snd_pcm_frames_to_bytes(adt->handle, 1));
			queue->used = 0;
		}
			break;
		case ADT_QUEUE_EXIT:
			goto exit;
		}
	}
exit:
	if (adt->handle) {
		snd_pcm_drain(adt->handle);
		snd_pcm_close(adt->handle);
	}
	if (adt)
		free(adt);
	vTaskDelete(NULL);
}

aw_data_transfer_t *aw_data_transfer_create(unsigned int mode, unsigned int ch,
					unsigned int rate, unsigned int chunk_bytes)
{
	aw_data_transfer_t *adt;
	snd_pcm_stream_t stream;
	int ret, i;
	unsigned int period_size = 512;
	unsigned int periods = 4;

	switch (mode) {
	case ADT_MODE_RECV:
		stream = SND_PCM_STREAM_CAPTURE;
		break;
	case ADT_MODE_SEND:
		stream = SND_PCM_STREAM_PLAYBACK;
		break;
	default:
		printf("unknown mode:%u\n", mode);
		return NULL;
	}

	adt = calloc(1, sizeof(*adt));
	if (!adt)
		return NULL;
	ret = snd_pcm_open(&adt->handle, ADT_CARD_NAME, stream, 0);
	if (ret < 0)
		goto err;

	ret = set_param(adt->handle, SND_PCM_FORMAT_S16_LE, rate, ch,
				period_size, period_size * periods);
	if (ret < 0)
		goto err;

	adt->work_queue = xQueueCreate(ADT_QUEUE_SIZE, sizeof(struct pcm_queue_item));
	if (!adt->work_queue)
		goto err;

	unsigned int length = snd_pcm_frames_to_bytes(adt->handle, chunk_bytes);
	for (i = 0; i < ADT_QUEUE_SIZE; i++) {
		adt->pcm_queue[i].used = 0;
		adt->pcm_queue[i].length = length;
		adt->pcm_queue[i].pcm_data = malloc(length);
		if (!adt->pcm_queue[i].pcm_data)
			goto err;
	}

	ret = xTaskCreate(adt_task, "adt-task", 2*1024, adt, configAPPLICATION_NORMAL_PRIORITY, &adt->task);
	if (ret != pdPASS)
		goto err;

	return adt;
err:
	if (adt->handle)
		snd_pcm_close(adt->handle);
	if (adt->work_queue)
		vQueueDelete(adt->work_queue);
	if (adt)
		free(adt);
	return NULL;
}

aw_data_transfer_t *aw_data_transfer_simple_create(unsigned int mode)
{
	return aw_data_transfer_create(mode, 1, 48000, 2048);
}

void aw_data_transfer_destroy(aw_data_transfer_t *adt)
{
	struct pcm_queue_item item;
	TickType_t timeout = pdMS_TO_TICKS(20);
	int ret;

	if (!adt)
		return;

	item.cmd = ADT_QUEUE_EXIT;
	ret = xQueueSendToBack(adt->work_queue, (void *)&item, timeout);
	if (ret == errQUEUE_FULL) {
		printf("xQueueSendToBack failed...\n");
		if (adt->handle) {
			snd_pcm_drain(adt->handle);
			snd_pcm_close(adt->handle);
		}
		if (adt)
			free(adt);
	}
	return;
}

static inline struct adt_pcm_queue *get_free_pcm_queue(aw_data_transfer_t *adt)
{
	int i;
	struct adt_pcm_queue *queue = NULL;

	for (i = 0; i < ADT_QUEUE_SIZE; i++) {
		queue = &adt->pcm_queue[i];
		if (!queue->used)
			return queue;
	}
	return NULL;
}

int aw_data_transfer_send(aw_data_transfer_t *adt, void *data, unsigned int len)
{
	int ret;
	struct pcm_queue_item item;
	TickType_t timeout = pdMS_TO_TICKS(200);
	int offset = 0;

	if (!adt)
		return -1;
	while (1) {
		int send_len = 0;
		item.queue = get_free_pcm_queue(adt);
		if (!item.queue) {
			/*printf("no free pcm queue\n");*/
			vTaskDelay(pdMS_TO_TICKS(10));
			continue;
		}
		item.queue->used = 1;
		if (item.queue->length >= len) {
			send_len = len;
		} else {
			send_len = item.queue->length;
		}
		item.cmd = ADT_QUEUE_SEND;
		item.queue->data_size = send_len;
		memcpy(item.queue->pcm_data, data + offset, send_len);
		ret = xQueueSendToBack(adt->work_queue, (void *)&item, timeout);
		offset += send_len;
		if (offset >= len)
			break;
	}

	return ret;
}

#if 0
int aw_data_transfer_recv(aw_data_transfer_t *adt, void *data, unsigned int len)
{
	int ret;
	if (!adt)
		return -1;
	/* Note:len should be snd_pcm_frames_to_bytes(adt->handle, 1) align */
	ret = pcm_read(adt->handle, data,
			snd_pcm_bytes_to_frames(adt->handle, len),
			snd_pcm_frames_to_bytes(adt->handle, 1));
	return ret > 0 ? snd_pcm_frames_to_bytes(adt->handle, ret) : ret;
}
#endif

int aw_data_transfer_bytes_to_frame(aw_data_transfer_t *adt, unsigned int bytes)
{
	if (!adt || !adt->handle)
		return -1;
	return snd_pcm_bytes_to_frames(adt->handle, bytes);
}

#if 0
#include <console.h>
#include <delay.h>

static int adt_send_test(void)
{
	char *data;
	int size = 100, count = 200;
	aw_data_transfer_t *adt;
	int i;

	data = malloc(size);
	if (!data)
		return -1;
	adt = aw_data_transfer_simple_create(ADT_MODE_SEND);
	if (!adt)
		goto err;

	for (i = 0; i < count; i++) {
		int ret;
		memset(data, i, size);
		ret = aw_data_transfer_send(adt, data, size);
#if 0
		if (ret != size) {
			printf("ret=%d, size=%d\n", ret, size);
			break;
		}
#endif
	}
	printf("bytes=%d, frame=%d\n", size * count,
			aw_data_transfer_bytes_to_frame(adt, size * count));

err:
	if (adt)
		aw_data_transfer_destroy(adt);
	if (data)
		free(data);
	return 0;
}
#if 0
static int adt_recv_test(void)
{
	char *data;
	int size = 10240, total_size;
	aw_data_transfer_t *adt;

	data = malloc(size);
	if (!data)
		return -1;
	adt = aw_data_transfer_simple_create(ADT_MODE_RECV);
	if (!adt)
		goto err;

	while(1) {
		int ret;
		ret = aw_data_transfer_recv(adt, data, size);
		if (ret != size) {
			printf("ret=%d, size=%d\n", ret, size);
			break;
		}
		total_size += size;
		printf("..total recv %d bytes\n", total_size);
	}
	printf("total recv %d bytes\n", total_size);

err:
	if (adt)
		aw_data_transfer_destroy(adt);
	if (data)
		free(data);
	return 0;
}
#else
static int adt_recv_test(void)
{
	return 0;
}
#endif

static int cmd_adttest(int argc, char *argv[])
{
	int c;
	while ((c = getopt(argc, argv, "sr")) != -1) {
		switch (c) {
		case 's':
			adt_send_test();
			break;
		case 'r':
			adt_recv_test();
			break;
		}
	}
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_adttest, adttest, aw data transfer test);
#endif
