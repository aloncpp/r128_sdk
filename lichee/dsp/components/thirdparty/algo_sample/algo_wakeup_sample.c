#include <stdlib.h>
#include <console.h>

#include <aw-alsa-lib/pcm.h>

#include "adt.h"

#define ALGO_DEBUG
#ifdef ALGO_DEBUG
#define algo_print(fmt, arg...) \
	printf("[%s:%d] "fmt, __func__, __LINE__, ##arg);
#else
#define algo_print(fmt, arg...)
#endif

static aw_data_transfer_t *g_adt;

static char g_card[16] = "hw:audiocodec";
static int g_capture_duration = 5;
static unsigned int g_rate = 16000;
static unsigned int g_channels = 5;
static unsigned int g_frame_step = 160;
static unsigned int g_data_transfer_en = 0;

int pcm_read(snd_pcm_t *handle, const char *data, snd_pcm_uframes_t frames_total,
		unsigned int frame_bytes);
int set_param(snd_pcm_t *handle, snd_pcm_format_t format,
			unsigned int rate, unsigned int channels,
			snd_pcm_uframes_t period_size,
			snd_pcm_uframes_t buffer_size);

static void capture_loop(snd_pcm_t *handle, int frame_total)
{
	void *audio_data;
	/* 算法每次处理160帧数据，即160/16000=10ms音频数据 */
	int frame_step = g_frame_step;
	/* 160帧数据转换为字节：frame_step_bytes */
	unsigned int frame_step_bytes = snd_pcm_frames_to_bytes(handle, frame_step);
	int size;

	audio_data = malloc(frame_step_bytes);
	if (!audio_data)
		return ;

	while (frame_total > 0) {
		size = pcm_read(handle, audio_data, frame_step,
				snd_pcm_frames_to_bytes(handle, 1));
		if (size != frame_step) {
			printf("pcm_read return %d, expected %d\n", size, frame_step);
			break;
		}
		frame_total -= frame_step;


		/* 算法语音数据处理 */
		/*algo_process();*/

		/* 调试用，将处理后数据发送给CPUX */
		if (g_data_transfer_en)
			aw_data_transfer_send(g_adt, audio_data, frame_step_bytes);
	}
	return;
}

static void wakeup_demo_main(void *arg)
{
	snd_pcm_t *handle;
	int ret;
	snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;
	unsigned int rate = g_rate;
	unsigned int channels = g_channels; /* 3+2 */
	unsigned int frame_total = g_capture_duration * rate;


	ret = snd_pcm_open(&handle, g_card, SND_PCM_STREAM_CAPTURE, 0);
	if (ret < 0) {
		printf("snd_pcm_open failed\n");
		return;
	}

	algo_print("format:%d rate:%d ch:%d\n", format, rate, channels);
	algo_print("duration:%d sec, frame:%d\n", g_capture_duration, frame_total);
	algo_print("frame_step:%d, chunk_bytes:%d\n", g_frame_step, g_frame_step*channels*2);
	ret = set_param(handle, format, rate, channels, 1024, 4096);
	if (ret < 0) {
		printf("set_param failed\n");
		goto err;
	}

	capture_loop(handle, frame_total);

	ret = snd_pcm_drop(handle);
	if (ret < 0) {
		printf("snd_pcm_open failed\n");
		goto err;
	}

err:
	if (handle)
		snd_pcm_close(handle);

	return;
}

static int cmd_wakeupdemo(int argc, char *argv[])
{
	int c;
	g_data_transfer_en = 0;

	algo_print("Start...\n");

	while ((c = getopt(argc, argv, "r:c:d:s:t")) != -1) {
		switch (c) {
		case 'r':
			g_rate = atoi(optarg);
			break;
		case 'c':
			g_channels = atoi(optarg);
			break;
		case 'd':
			g_capture_duration = atoi(optarg);
			break;
		case 's':
			g_frame_step = atoi(optarg);
			break;
		case 't':
			g_data_transfer_en = 1;
			break;
		default:
			break;
		}
	}

	/* 算法初始化 */
	/*algo_init();*/

	/* 调试用,初始化数据转发功能 */
	if (g_data_transfer_en) {
#if 0
		g_adt = aw_data_transfer_simple_create(ADT_MODE_SEND);
#else
		g_adt = aw_data_transfer_create(ADT_MODE_SEND, g_channels,
					g_rate, g_frame_step*g_channels*2);
#endif
		if (!g_adt)
			return -1;
	}

	/* 录音 */
	xTaskCreate(wakeup_demo_main, "wakeup_demo_main", 0x1000, NULL, 1, NULL);

	/* 算法销毁 */
	/*algo_release();*/

	/* 调试用，注销数据转发功能 */
	if (g_data_transfer_en)
		aw_data_transfer_destroy(g_adt);

	algo_print("Finish...\n");
	return 0;
}

FINSH_FUNCTION_EXPORT_CMD(cmd_wakeupdemo, wakeupdemo, algorithm wakeup demo);
