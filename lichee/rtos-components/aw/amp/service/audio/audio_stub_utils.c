#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <reent.h>

#include "sunxi_amp.h"
#include <hal_cache.h>
#include <hal_mutex.h>
#include <console.h>

#include <AudioSystem.h>



#define LOG_COLOR_NONE		"\e[0m"
#define LOG_COLOR_BLUE		"\e[34m"

#define aa_info(fmt, args...)	\
    printf(LOG_COLOR_BLUE "[STUB][%s](%d) " fmt "\n" LOG_COLOR_NONE, \
			__func__, __LINE__, ##args)

#define LOCAL_PRIORITIES 	\
	(configMAX_PRIORITIES > 20 ? configMAX_PRIORITIES - 8 : configMAX_PRIORITIES - 3)

typedef int (*audio_remote_test_case)(const int argc, const char **argv);

typedef struct {
	const char *func_desc;
	audio_remote_test_case func;
	const int argc;
	const char **argv;
} audio_remote_funclist_t;

#define HEXDUMP(ptr, size) \
do { \
	int i; \
	char *p = (char *)ptr; \
	aa_info(""); \
	for (i = 0; i < size; i++) { \
		printf("0x%x ", *(p+i)); \
		if ((i+1)%16 == 0) \
			printf("\n"); \
	} \
	aa_info(""); \
} while (0)

static int audio_test(const int argc, const char **argv)
{
	int ret;
	uint8_t *buf;
	uint32_t len = 64;

	aa_info("");
	ret = audio_test1();
	aa_info("audio_test1 return %d", ret);

	ret = audio_test2(1, 2);
	aa_info("audio_test2 return %d", ret);

	buf = amp_align_malloc(len);
	if (!buf) {
		aa_info("no memory");
		return -1;
	}
	memset(buf, 0x55, len);
	aa_info("3 buf=%p, len=%d", buf, len);
	ret = audio_test3_set(buf, len);
	aa_info("audio_test3 return %d, buf=%p", ret, buf);

	aa_info("4 buf=%p, len=%d", buf, len);
	ret = audio_test4_get(buf, len);
	aa_info("audio_test4 return %d, buf=%p", ret, buf);
	HEXDUMP(buf, len);

	amp_align_free(buf);

	return 0;
}

#ifndef CONFIG_AMP_AUDIO_PB_API_NONE

static int audio_track_open_close(const int argc, const char **argv)
{
#ifdef CONFIG_AMP_AUDIO_PB_API_ALIAS
	tAudioTrackRM *at;
#else
	tAudioTrack *at;
#endif
	int ret;

	aa_info("");
#ifdef CONFIG_AMP_AUDIO_PB_API_ALIAS
	at = AudioTrackCreateRM("default");
#else
	at = AudioTrackCreate("amp");
#endif
	if (!at) {
		aa_info("at create failed");
		return -1;
	}
#ifdef CONFIG_AMP_AUDIO_PB_API_ALIAS
	ret = AudioTrackDestroyRM(at);
#else
	ret = AudioTrackDestroy(at);
#endif
	if (ret != 0)
		aa_info("at destroy failed");

	return 0;
}
#if 0
#include "wav_parser.h"
#include "audio_wav.h"
static void audio_track(void *arg)
{
#ifdef CONFIG_AMP_AUDIO_PB_API_ALIAS
	tAudioTrackRM *at;
#else
	tAudioTrack *at;
#endif
	int ret;
	int id = *(int *)arg;
	uint8_t bits = 16;

	aa_info("audio track task num=%d", id);
#ifdef CONFIG_AMP_AUDIO_PB_API_ALIAS
	at = AudioTrackCreateRM("default");
#else
	at = AudioTrackCreate("amp");
#endif
	if (!at) {
		aa_info("at create failed");
		goto destroy_at;
	}

	wav_header_t *wav_header = NULL;
	wav_file_t *wav_file;
	wav_hw_params_t wav_hwparams;
	uint32_t rate, channels;
	snd_pcm_format_t format;
	int size;
	int count = 1;

	wav_file = find_builtin_wav_file("16K_16bit_1ch");
	if (!wav_file)
		goto destroy_at;

	wav_header = (wav_header_t *)wav_file->start;
	if (check_wav_header(wav_header, &wav_hwparams) != 0) {
		aa_info("check wav header failed");
		goto destroy_at;
	}

	rate = wav_hwparams.rate;
	format = wav_hwparams.format;
	channels = wav_hwparams.channels;
	if (format == SND_PCM_FORMAT_S16_LE)
		bits = 16;
	else if (format == SND_PCM_FORMAT_S32_LE)
		bits = 32;
	else
		aa_info("unknown format%d", format);

#ifdef CONFIG_AMP_AUDIO_PB_API_ALIAS
	ret = AudioTrackSetupRM(at, rate, channels, bits);
#else
	ret = AudioTrackSetup(at, rate, channels, bits);
#endif
	if (ret != 0) {
		aa_info("at setup failed");
		goto destroy_at;
	}

	while (count-- > 0) {
#ifdef CONFIG_AMP_AUDIO_PB_API_ALIAS
		size = AudioTrackWriteRM(at, (void *)(wav_file->start + sizeof(wav_header_t)),
			wav_header->dataSize);
#else
		size = AudioTrackWrite(at, (void *)(wav_file->start + sizeof(wav_header_t)),
			wav_header->dataSize);
#endif
	}

#ifdef CONFIG_AMP_AUDIO_PB_API_ALIAS
	ret = AudioTrackStopRM(at);
#else
	ret = AudioTrackStop(at);
#endif
	if (ret != 0)
		aa_info("at stop failed");
destroy_at:
	if (at) {
#ifdef CONFIG_AMP_AUDIO_PB_API_ALIAS
		ret = AudioTrackDestroyRM(at);
#else
		ret = AudioTrackDestroy(at);
#endif
		if (ret != 0)
			aa_info("at destroy failed");
	}

	vTaskDelete(NULL);
}
#endif

static int g_at_rm_task_id = 0;
static int audio_track_play(const int argc, const char **argv)
{
	TaskHandle_t handle;
	char buf[32];
#if 0
	snprintf(buf, sizeof(buf), "at_rm_task%d", g_at_rm_task_id++);
	xTaskCreate(audio_track, buf, 2048, &g_at_rm_task_id,
			LOCAL_PRIORITIES,
			&handle);
#endif
	return 0;
}
#endif /* CONFIG_AMP_AUDIO_PB_API_NONE */

#ifndef CONFIG_AMP_AUDIO_CAP_API_NONE
static int audio_record_open_close(const int argc, const char **argv)
{
#ifdef CONFIG_AMP_AUDIO_CAP_API_ALIAS
	tAudioRecordRM *ar;
#else
	tAudioRecord *ar;
#endif
	int ret;

#ifdef CONFIG_AMP_AUDIO_CAP_API_ALIAS
	ar = AudioRecordCreateRM("default");
#else
	ar = AudioRecordCreate("amp");
#endif
	if (!ar) {
		aa_info("ar create failed");
		return -1;
	}
#ifdef CONFIG_AMP_AUDIO_CAP_API_ALIAS
	ret = AudioRecordDestroyRM(ar);
#else
	ret = AudioRecordDestroy(ar);
#endif
	if (ret != 0)
		aa_info("ar destroy failed");

	return 0;
}

static void audio_record(void *arg)
{
#ifdef CONFIG_AMP_AUDIO_CAP_API_ALIAS
	tAudioRecordRM *ar;
#else
	tAudioRecord *ar;
#endif
#ifdef CONFIG_AMP_AUDIO_PB_API_ALIAS
	tAudioTrackRM *at;
#else
	tAudioTrack *at;
#endif
	int ret;
	int id = *(int *)arg;
	int total, read_size, size, read = 0;
	unsigned int sec = 1;
	unsigned int rate = 16000;
	unsigned int channels = 2;
	uint8_t bits = 16;
	unsigned int frame_bytes = channels * 2;
	int frames_bytes_loop = frame_bytes * rate / 100; /* 10ms */
	void *buf = NULL;

	total = sec * rate * frame_bytes;

	aa_info("audio record task num=%d", id);
#ifdef CONFIG_AMP_AUDIO_CAP_API_ALIAS
	ar = AudioRecordCreateRM("default");
#else
	ar = AudioRecordCreate("amp");
#endif
	if (!ar) {
		aa_info("ar create failed");
		goto err;
	}

	buf = malloc(total);
	if (!buf) {
		printf("no memory\n");
		goto err;
	}

#ifdef CONFIG_AMP_AUDIO_CAP_API_ALIAS
	AudioRecordSetupRM(ar, rate, channels, bits);
	AudioRecordStartRM(ar);
#else
	AudioRecordSetup(ar, rate, channels, bits);
	AudioRecordStart(ar);
#endif
	while (total > 0) {
		if (total > frames_bytes_loop)
			size = frames_bytes_loop;
		else
			size = total;
#ifdef CONFIG_AMP_AUDIO_CAP_API_ALIAS
		read_size = AudioRecordReadRM(ar, buf + read, size);
#else
		read_size = AudioRecordRead(ar, buf + read, size);
#endif
		if (read_size != frames_bytes_loop) {
			aa_info("read_size(%d) != frames_bytes_loop(%d)", read_size, frames_bytes_loop);
			break;
		}
		total -= read_size;
		read += read_size;
	}
#ifdef CONFIG_AMP_AUDIO_CAP_API_ALIAS
	AudioRecordStopRM(ar);
	AudioRecordDestroyRM(ar);
#else
	AudioRecordStop(ar);
	AudioRecordDestroy(ar);
#endif
	ar = NULL;

	vTaskDelay(pdMS_TO_TICKS(500));


#ifdef CONFIG_AMP_AUDIO_PB_API_ALIAS
	at = AudioTrackCreateRM("default");
#else
	at = AudioTrackCreate("amp");
#endif
	if (!at) {
		aa_info("at create failed");
		goto err;
	}

#ifdef CONFIG_AMP_AUDIO_PB_API_ALIAS
	ret = AudioTrackSetupRM(at, rate, channels, bits);
#else
	ret = AudioTrackSetup(at, rate, channels, bits);
#endif
	if (ret != 0) {
		aa_info("at setup failed");
		goto err;
	}

#ifdef CONFIG_AMP_AUDIO_PB_API_ALIAS
	AudioTrackWriteRM(at, (void *)buf, read);
#else
	AudioTrackWrite(at, (void *)buf, read);
#endif

#ifdef CONFIG_AMP_AUDIO_PB_API_ALIAS
	ret = AudioTrackStopRM(at);
#else
	ret = AudioTrackStop(at);
#endif
	if (ret != 0)
		aa_info("at stop failed");

err:
	if (ar)
#ifdef CONFIG_AMP_AUDIO_CAP_API_ALIAS
		AudioRecordDestroyRM(ar);
#else
		AudioRecordDestroy(ar);
#endif
	if (buf)
		free(buf);
	if (at) {
#ifdef CONFIG_AMP_AUDIO_PB_API_ALIAS
		ret = AudioTrackDestroyRM(at);
#else
		ret = AudioTrackDestroy(at);
#endif
		if (ret != 0)
			aa_info("at destroy failed");
	}
	vTaskDelete(NULL);
}

static int g_ar_rm_task_id = 0;
static int audio_record_capture(const int argc, const char **argv)
{
	TaskHandle_t handle;
	char buf[32];

	snprintf(buf, sizeof(buf), "ar_rm_task%d", g_ar_rm_task_id++);
	xTaskCreate(audio_record, buf, 2048, &g_ar_rm_task_id,
			LOCAL_PRIORITIES,
			&handle);
	return 0;
}
#endif /* CONFIG_AMP_AUDIO_CAP_API_NONE */

static audio_remote_funclist_t audio_remote_funclist[] = {
	{
		"audio test",
		audio_test,
		0,
		NULL
	},
#ifndef CONFIG_AMP_AUDIO_PB_API_NONE
	{
		"audio track open close",
		audio_track_open_close,
		0,
		NULL
	},
#if 0
	{
		"audio track play",
		audio_track_play,
		0,
		NULL
	},
#endif
#endif
#ifndef CONFIG_AMP_AUDIO_CAP_API_NONE
	{
		"audio record open close",
		audio_record_open_close,
		0,
		NULL
	},
	{
		"audio record capture",
		audio_record_capture,
		0,
		NULL
	},
#endif
};

static int cmd_audioremote(int argc, char **argv)
{
	int index;
	char *cmd_list = "list";

	if (argc == 2 && !memcmp(argv[1], cmd_list, strlen(cmd_list))) {
		int i;

		printf("%s        %s\n", "index", "func desc");
		for (i = 0; i < ARRAY_SIZE(audio_remote_funclist); i++) {
			printf("%02d           %s\n", i, audio_remote_funclist[i].func_desc);
		}
		return 0;
	}
	if (argc <= 1) {
		printf("Usage: audiorm index [arg] \n");
		return -1;
	}
	index = atoi(argv[1]);

	if (index < 0 || index >= ARRAY_SIZE(audio_remote_funclist)) {
		printf("index=%d, not in rage:0~%d\n", index,
			ARRAY_SIZE(audio_remote_funclist) - 1);
		return -1;
	}
	audio_remote_funclist[index].func(audio_remote_funclist[index].argc,
					audio_remote_funclist[index].argv);

	return 0;
}

FINSH_FUNCTION_EXPORT_CMD(cmd_audioremote, audiorm, audio remote test);
