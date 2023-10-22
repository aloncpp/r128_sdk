/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the people's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.
*
*
* THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
* PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
* WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
* THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
* OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
* IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
* NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
* OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <console.h>

#include <AudioSystem.h>

#include "wav_parser.h"
#include <hal_thread.h>
#include <hal_time.h>

static int g_loop_count = 0;
/*static int8_t g_play_sine = 0;*/
static int8_t g_play_or_cap = 0;
static int8_t g_record_then_play = 0;
static int8_t g_bits = 0;
static int g_run_time = 0;
static int g_at_task_id = 0;
static int g_ar_task_id = 0;
static int g_rate = 0;
static int g_channels = 0;
static char g_wav_path[128] = "16K_16bit_1ch";
static int g_tmp_arg = 0;
static char g_at_name[12] = "default";
static char g_ar_name[12] = "default";
static int g_ar_forward_port = 0;

#define LOCAL_PRIORITIES	\
	(configMAX_PRIORITIES > 20 ? configMAX_PRIORITIES - 8 : configMAX_PRIORITIES - 3)

struct as_test_data {
	int loop_count;
	int type;		/* 0:wav, 1:data */
	uint32_t rate,channels;
	void *ptr;
	int data_bytes;
	uint8_t bits;
	int8_t sec;
	int8_t record_then_play;
};

#ifndef CONFIG_ARCH_DSP
static void dump_wav_header(wav_header_t *header)
{
	char *ptr = (char *)&header->riffType;
	printf("riffType:     %c%c%c%c\n", ptr[0], ptr[1], ptr[2], ptr[3]);
	ptr = (char *)&header->waveType;
	printf("waveType:     %c%c%c%c\n", ptr[0], ptr[1], ptr[2], ptr[3]);
	printf("channels:     %u\n", header->numChannels);
	printf("rate:         %u\n", header->sampleRate);
	printf("bits:         %u\n", header->bitsPerSample);
	printf("align:        %u\n", header->blockAlign);
	printf("data size:    %u\n", header->dataSize);
}

static int check_wav_header(wav_header_t *header, wav_hw_params_t *hwparams)
{
	if (!header)
		return -1;
	dump_wav_header(header);

	if (header->riffType != WAV_RIFF)
		return -1;
	if (header->waveType != WAV_WAVE)
		return -1;

	hwparams->rate = header->sampleRate;
	hwparams->channels = header->numChannels;
	/* ignore bit endian */
	switch (header->bitsPerSample) {
	case 8:
		hwparams->format = SND_PCM_FORMAT_U8;
		break;
	case 16:
		hwparams->format = SND_PCM_FORMAT_S16_LE;
		break;
	case 24:
		switch (header->blockAlign/header->numChannels) {
			case 4:
				hwparams->format = SND_PCM_FORMAT_S24_LE;
				break;
			case 3:
				/*hwparams->format = SND_PCM_FORMAT_S24_3LE;*/
			default:
				printf("unknown format..\n");
				return -1;
		}
		break;
	case 32:
		hwparams->format = SND_PCM_FORMAT_S32_LE;
		break;
	default:
		break;
	}

	return 0;
}
#endif

#define PI (3.1415926)
static int sine_generate(void *buf, uint32_t len, uint32_t rate, uint32_t channels, uint8_t bits, float amp)
{
	int sine_hz = 1000;
	int sine_point, sine_cycle, sine_point_bytes;
	int i,j;
	int accuracy = INT16_MAX;
	int frame_bytes;

	if (amp > 1.0)
		amp = 1.0;
	else if (amp < 0.01)
		amp = 0.01;

	frame_bytes = channels * bits / 8;
	sine_point = rate / sine_hz;
	sine_point_bytes = frame_bytes * sine_point;
	sine_cycle = len / sine_point_bytes;
	if (bits == 16) {
		int16_t *data = buf;
		accuracy = INT16_MAX;
		for (j = 0; j < sine_point; j++) {
			int16_t value = (int16_t)(amp * accuracy * sin(2 * (double)PI * j / sine_point));
			if (channels == 1) {
				data[j] = value;
			} else if (channels == 2) {
				data[2 * j] = value;
				data[2 * j + 1] = value;
			} else {
				printf("unsupport channels:%d\n", channels);
				while(1);
			}
		}
	} else if (bits == 32) {
		int32_t *data = buf;
		accuracy = INT32_MAX;
		for (j = 0; j < sine_point; j++) {
			int32_t value = (int32_t)(amp * accuracy * sin(2 * (double)PI * j / sine_point));
			if (channels == 1) {
				data[j] = value;
			} else if (channels == 2) {
				data[2 * j] = value;
				data[2 * j + 1] = value;
			} else {
				printf("unsupport channels:%d\n", channels);
				while(1);
			}
		}
	}

	for (i = 1; i < sine_cycle; i++) {
		memcpy(buf + i * sine_point_bytes, buf, sine_point_bytes);
		/*printf("[%s] line:%d buf:%p, dest:%p, ofs:%u\n", __func__, __LINE__,*/
			/*buf, buf + i * sine_point_bytes, i * sine_point_bytes);	*/
	}

	return sine_cycle * sine_point_bytes;
}

static void play_sine(tAudioTrack *at, uint32_t rate, uint8_t channels, uint8_t bits, int sec)
{
	int frame_loop = 480;
	int count, frame_bytes;
	void *buf;
	int len, size = 0;

	frame_bytes = frame_loop * channels * (bits == 16 ? 2 : 4);
	count = rate * sec / frame_loop;

	buf = malloc(frame_bytes);
	/*printf("[%s] line:%d malloc %u bytes\n", __func__, __LINE__, frame_bytes);*/
	if (!buf)
		return;

	len = sine_generate(buf, frame_bytes, rate, channels, bits, 0.8);
	while (count--) {
		size = AudioTrackWrite(at, buf, len);
		if (size != len) {
			printf("at write return %d\n", size);
			break;
		}
	}
	free(buf);
}

static void at_sine_task(void *arg)
{
	tAudioTrack *at;
	int channels = g_channels;
	int rate = g_rate;
	int sec = g_run_time;
	uint8_t bits = g_bits;

#if 1
	at = AudioTrackCreate(g_at_name);
#else
	at = AudioTrackCreateWithStream(g_at_name, AUDIO_STREAM_MUSIC);
#endif
	if (!at) {
		printf("at create failed\n");
		goto err;
	}
#if 0
	AudioTrackResampleCtrl(at, 1);
#endif

	AudioTrackSetup(at, rate, channels, bits);
	play_sine(at, rate, channels, bits, sec);

	AudioTrackStop(at);

	AudioTrackDestroy(at);
err:
	hal_thread_stop(NULL);
}

static void at_sine_task_create()
{
	hal_thread_t handle;
	char buf[32];

	snprintf(buf, sizeof(buf), "at_sine%d", g_at_task_id);
	g_at_task_id++;
	handle = hal_thread_create(at_sine_task, NULL, buf, 8192, HAL_THREAD_PRIORITY_APP);
}

#ifndef CONFIG_ARCH_DSP
static void play_fs_wav(tAudioTrack *at, int count)
{
	wav_header_t wav_header;
	wav_hw_params_t wav_hwparams;
	uint32_t rate, channels;
	snd_pcm_format_t format;
	int size, fd, bits = 16, buf_size;
	uint8_t *buf = NULL;

	fd = open(g_wav_path, O_RDONLY);
	if (fd < 0)
		return;
	read(fd, &wav_header, sizeof(wav_header_t));

	if (check_wav_header(&wav_header, &wav_hwparams) != 0) {
		printf("check wav header failed\n");
		return;
	}

	rate = wav_hwparams.rate;
	format = wav_hwparams.format;
	channels = wav_hwparams.channels;

	if (format == SND_PCM_FORMAT_S16_LE) {
		bits = 16;
		AudioTrackSetup(at, rate, channels, 16);
	} else if (format == SND_PCM_FORMAT_S32_LE) {
		bits = 32;
		AudioTrackSetup(at, rate, channels, 32);
	}

	buf_size = bits / 8 * channels * rate / 50; /* 20ms buffer */
	buf = malloc(buf_size);
	if (!buf) {
		printf("no memory\n");
		return;
	}

	while (count-- > 0) {
		uint32_t total = wav_header.dataSize;
		while (total) {
			size = read(fd, buf, buf_size);
			if (size <= 0)
				break;
			size = AudioTrackWrite(at, (void *)buf, size);
			if (size < 0)
				break;
			total -= size;
		}
		lseek(fd, -wav_header.dataSize, SEEK_CUR);
	}
	close(fd);
	free(buf);

	return;
}
#endif

static void at_task(void *arg)
{
	tAudioTrack *at;
	struct as_test_data *data = arg;

	at = AudioTrackCreate(g_at_name);
	if (!at) {
		printf("at create failed\n");
		goto err;
	}

	if (data->type == 0)  {
#ifndef CONFIG_ARCH_DSP
		if (!access(g_wav_path, F_OK))
			play_fs_wav(at, data->loop_count);
#endif
	} else if (data->type == 1 && data->ptr != NULL){
		/* play pcm data */
		int value = 0;
		int count = data->loop_count;
		AudioTrackSetup(at, data->rate, data->channels, data->bits);
		while (count--) {
			AudioTrackWrite(at, data->ptr, data->data_bytes);
			value++;
			printf("[%s] line:%d playback count=%d\n", __func__, __LINE__, value);
		}
	}
	AudioTrackStop(at);

	AudioTrackDestroy(at);
err:
	if (data->ptr)
		free(data->ptr);
	free(data);
	hal_thread_stop(NULL);
}

static void at_task_create(struct as_test_data *data)
{
	hal_thread_t handle;
	char buf[32];
	struct as_test_data *d;

	d = malloc(sizeof(struct as_test_data));
	if (!data) {
		d->type = 0;
		d->ptr = NULL;
		d->loop_count = g_loop_count;
	} else {
		memcpy(d, data, sizeof(struct as_test_data));
	}
	snprintf(buf, sizeof(buf), "at_task%d", g_at_task_id);
	g_at_task_id++;
	handle = hal_thread_create(at_task, d, buf, 2048, HAL_THREAD_PRIORITY_APP);
}

#include <sys/time.h>
#if 0
#include "speexrate/speex_resampler.h"
/*#include <xtensa/xtbsp.h>*/
/*#include "xtensa_timer.h"*/
static unsigned int div_of_us_cycle = 400000000 / 1000000;
#if 0
#define TIMESTAMP_INIT()	\
	hal_tick_t _start, _end;
#define TIMESTAMP_START()	\
do { \
	_start = hal_tick_get(); \
} while (0)
#define TIMESTAMP_END()		\
do { \
	_end = hal_tick_get(); \
	printf("TICK: %u\n", _end - _start); \
} while (0)
#else
#define TIMESTAMP_INIT()	 \
	int64_t _start, _end;
#define TIMESTAMP_START()	\
do { \
	_start = hal_gettime_ns(); \
} while (0)

#define TIMESTAMP_END()		\
do { \
	_end = hal_gettime_ns(); \
} while (0)

#define TIMESTAMP_PRINTF()	\
do { \
	uint64_t delta; \
	uint64_t sec, ms, us; \
	delta = _end - _start; \
	sec = delta / 1000000000ULL; \
	ms = delta % 1000000000ULL / 1000000UL; \
	us = delta % 1000000UL / 1000; \
	printf("speed %d sec, %d ms, %d us\n", \
		sec, ms, us); \
} while (0)
#endif

static void resample_test()
{
	int ret;
	SpeexResamplerState *st;
	int16_t *src;
	int16_t *dst;
	uint32_t frame_bytes = 2*2;
	int count;
	int in_rate = 16000;
	int out_rate = 48000;
	uint32_t dst_frames = g_tmp_arg;
	uint32_t src_frames, bak_dst_frames, bak_src_frames;
	int i;
	uint32_t src_bytes;

#if 1
	src_frames = in_rate * dst_frames / out_rate;
#else
	src_frames = 320;
#endif
#if 0
	if (in_rate * dst_frames % out_rate != 0)
		src_frames++;
#endif
	src_bytes = src_frames * frame_bytes;

	bak_src_frames = src_frames;
	bak_dst_frames = dst_frames;

	src = malloc(src_bytes);
	dst = malloc(dst_frames * frame_bytes);
	st = speex_resampler_init_frac(2, in_rate, out_rate, in_rate, out_rate, 3, &ret);
	for (i = 0; i < src_bytes / 2; i++)
		src[i] = i%0xffff;

	count = g_loop_count;

	printf("in_rate:%u, in_frames:%u, out_rate:%u, out_frames:%u\n",
		in_rate, src_frames, out_rate, dst_frames);

	TIMESTAMP_INIT();
	uint32_t start, delta;
#if 0
        start = xthal_get_ccount();
#else
#endif
	TIMESTAMP_START();
	while (count-- > 0) {
		ret = speex_resampler_process_interleaved_int(st, src, &src_frames, dst, &dst_frames);
		if (ret != 0) {
			printf("resample failed, ret = %d\n", ret);
			break;
		}
		if (src_frames != bak_src_frames) {
			/*_debug("src_frames=%d, expected src_frames=%d", src_frames, bak_src_frames);*/
			src_frames = bak_src_frames;
		} else {
			/*_info("get src_frames = expected src_frames = %d", src_frames);*/
		}
		if (dst_frames != bak_dst_frames) {
			/*_err("dst_frames=%d, expected dst_frames=%d", dst_frames, bak_dst_frames);*/
			dst_frames = bak_dst_frames;
		} else {
			/*_info("get dst_frames = expected dst_frames = %d", dst_frames);*/
		}
		/*memcpy(src, dst, src_bytes);*/
	}
	TIMESTAMP_END();
#if 0
        delta = xthal_get_ccount() - start;
	printf("delta=%u, spend time:%uus\n", delta, delta / div_of_us_cycle);
#else
	TIMESTAMP_PRINTF();
#endif

	/*printf("src:%p, src_frames:%u, dst:%p, dst_frames:%u\n",*/
			/*src, src_frames, dst, dst_frames);*/
	speex_resampler_destroy(st);
}
#endif

extern int adb_forward_create_with_rawdata(int port);
extern int adb_forward_send(int port, void *data, unsigned len);
extern int adb_forward_end(int port);
static void ar_task(void *arg)
{
	tAudioRecord *ar;
	struct as_test_data *data = arg;
	uint32_t rate = data->rate;
	uint32_t channels = data->channels;
	uint8_t bits = data->bits;
	int total, read_size, size, read = 0;
	int frame_bytes = bits / 8  * channels;
	int frames_bytes_loop = frame_bytes * rate / 100; /* 10ms */
	void *buf = NULL;

	if (data->sec)
		total = data->sec * rate * frame_bytes;
	else
		total = frames_bytes_loop; /* 10ms buffer */

#ifdef CONFIG_COMPONENTS_USB_GADGET_ADB_FORWARD
	int _port = g_ar_forward_port;
	if (g_ar_forward_port > 0)
		g_ar_forward_port = 0;
	if (adb_forward_create_with_rawdata(_port) < 0)
		_port = -1;
#endif
	ar = AudioRecordCreate(g_ar_name);
	if (!ar) {
		printf("ar create failed\n");
		goto err;
	}
#if 0
	AudioRecordResampleCtrl(ar, 1);
#endif

	buf = malloc(total);
	if (!buf) {
		printf("no memory\n");
		goto err;
	}

#if 0
	{
	uint8_t maps[3] = {2, 1, 0};
	AudioRecordChannelMap(ar, maps, sizeof(maps));
	}
#endif
	AudioRecordSetup(ar, rate, channels, bits);
	AudioRecordStart(ar);

	printf("[%s] line:%d buf:%p, %d\n", __func__, __LINE__, buf, total);
	while (data->loop_count--) {
		if (data->sec)
			total = data->sec * rate * frame_bytes;
		else
			total = frames_bytes_loop;
		read = 0;
		while (total > 0) {
			if (total > frames_bytes_loop)
				size = frames_bytes_loop;
			else
				size = total;
			read_size = AudioRecordRead(ar, buf + read, size);
			if (read_size != frames_bytes_loop) {
				printf("read_size(%d) != frames_bytes_loop(%d)\n", read_size, frames_bytes_loop);
				break;
			}
#ifdef CONFIG_COMPONENTS_USB_GADGET_ADB_FORWARD
			adb_forward_send(_port, buf + read, size);
#endif
			total -= read_size;
			read += read_size;
			/*printf("[%s] line:%d residue:%d read=%u\n", __func__, __LINE__, total, read);*/
		}
		if (read_size < 0)
			break;
	}
	AudioRecordStop(ar);
	AudioRecordDestroy(ar);
	ar = NULL;
#ifdef CONFIG_COMPONENTS_USB_GADGET_ADB_FORWARD
	adb_forward_end(_port);
	/* don't destroy port for repeating record */
	/*adb_forward_destroy(_port);*/
#endif

	hal_msleep(500);
	if (data->record_then_play != 0) {
		struct as_test_data d = {
			.loop_count = 1,
			.type = 1,
			.rate = rate,
			.channels = channels,
			.ptr = buf,
			.data_bytes = read,
			.bits = bits,
		};
		at_task_create(&d);
		buf = NULL;
	}

err:
	if (ar)
		AudioRecordDestroy(ar);
	free(data);
	if (buf)
		free(buf);
	hal_thread_stop(NULL);
}

static void ar_task_create(struct as_test_data *data)
{
	hal_thread_t handle;
	char buf[32];
	struct as_test_data *d;

	d = malloc(sizeof(struct as_test_data));
	if (!data) {
		d->type = 0;
		d->ptr = NULL;
		d->rate = g_rate;
		d->channels = g_channels;
		d->bits = g_bits;
		d->record_then_play = g_record_then_play;
		d->loop_count = g_loop_count;
		d->sec = g_run_time;
	} else {
		memcpy(d, data, sizeof(struct as_test_data));
	}
	snprintf(buf, sizeof(buf), "ar_task%d", g_ar_task_id);
	g_ar_task_id++;
	handle = hal_thread_create(ar_task, d, buf, 2048, HAL_THREAD_PRIORITY_APP);
}

static void as_test_usage()
{
	printf("Usgae: as_test [option]\n");
	printf("-h,          as_test help\n");
	printf("-s,          stream, 0-playback; 1-capture; 2-playback sine\n");
	printf("-d,          duration, sec\n");
	printf("-r,          rate\n");
	printf("-c,          channels\n");
	printf("-b,          bits\n");
	printf("-t,          capture and then playback\n");
	printf("-n,          AudioTrack name\n");
	printf("-m,          AudioRecord name\n");
	printf("-l,          loop count\n");
	printf("-f,          adb forward port\n");
	printf("\n");
	printf("play sine:\n");
	printf("as_test -s 2 -d 10 -r 48000\n");
	printf("capture:\n");
	printf("as_test -s 1 -d 0 -l 1000 -r 16000 -c 3\n");
	printf("capture and forward:\n");
	printf("as_test -s 1 -d 0 -l 1000 -r 16000 -c 3 -f 20227\n");
	printf("\n");
}

int cmd_as_test(int argc, char *argv[])
{
	int c = 0;
	g_play_or_cap = 0;
	g_loop_count = 1;
	g_run_time = 3;
	g_rate = 16000;
	g_channels = 2;
	g_bits = 16;
	g_record_then_play = 0;

	optind = 0;
	while ((c = getopt(argc, argv, "htl:s:ad:r:c:b:g:n:m:f:")) != -1) {
		switch (c) {
		case 'h':
			as_test_usage();
			return 0;
		case 'l':
			g_loop_count = atoi(optarg);
			break;
		case 's':
			/*
			 * 0: playback
			 * 1: capture
			 * 2: playback sine
			 * */
			g_play_or_cap = atoi(optarg);
			break;
		case 'a':
			/*resample_test();*/
			return 0;
		case 'd':
			g_run_time = atoi(optarg);
			break;
		case 'r':
			g_rate = atoi(optarg);
			break;
		case 'c':
			g_channels = atoi(optarg);
			break;
		case 'b':
			g_bits = atoi(optarg);
			break;
		case 't':
			g_record_then_play = 1;
			break;
		case 'g':
			g_tmp_arg = atoi(optarg);
			break;
		case 'n':
			strncpy(g_at_name, optarg, sizeof(g_at_name));
			break;
		case 'm':
			strncpy(g_ar_name, optarg, sizeof(g_ar_name));
			break;
		case 'f':
			g_ar_forward_port = atoi(optarg);
			break;
		default:
			/*printf("%s", APLAY_COMMOND_HELP);*/
			return -1;
		}
	}

	if (optind < argc) {
		strncpy(g_wav_path, argv[optind], sizeof(g_wav_path) - 1);
	} else {
		strcpy(g_wav_path, "16K_16bit_1ch");
	}

	switch (g_play_or_cap) {
	case 0:
		at_task_create(NULL);
		break;
	case 1:
		ar_task_create(NULL);
		break;
	case 2:
		at_sine_task_create(NULL);
		break;
	default:
		printf("unknown 's' command\n");
		break;
	}
	return  0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_as_test, as_test, audio system test);


static void as_volume_usage(void)
{
	printf("Usgae: as_volume [option]\n");
	printf("-h,          as_volume help\n");
	printf("-t,          volume type\n");
	printf("             1:system\n");
	printf("             2:music\n");
	printf("-m,          option mode\n");
	printf("             0:get\n");
	printf("             1:set\n");
	printf("             2:get range\n");
	printf("\n");
}

static int cmd_as_volume(int argc, char *argv[])
{
	int type = AUDIO_STREAM_SYSTEM;
	int volume_mode = 0;
	uint32_t volume_value = 0;
	int c = 0, ret = 0;

	optind = 0;
	while ((c = getopt(argc, argv, "ht:m:")) != -1) {
		switch (c) {
		case 't':
			type = atoi(optarg);
			break;
		case 'm':
			volume_mode = atoi(optarg);
			break;
		case 'h':
		default:
			as_volume_usage();
			return -1;
		}
	}

	if (optind < argc) {
		int value = atoi(argv[optind]);
		volume_value = (uint32_t)(value | value << 16);
	}

	ret = softvol_control_with_streamtype(type, &volume_value, volume_mode);
	if (ret != 0) {
		printf("softvol(t:%d, m:%d), control failed:%d\n", type, volume_mode, ret);
		return -1;
	}

	switch (volume_mode) {
	case 0: /* read */
		printf("softvol(%d) read, value=%d,%d\n", type,
				(volume_value & 0xffff),
				((volume_value >> 16) & 0xffff));
		break;
	case 1: /* write */
		printf("softvol(%d) write, value=%d,%d\n", type,
				(volume_value & 0xffff),
				((volume_value >> 16) & 0xffff));
		break;
	case 2: /* read range */
		printf("softvol(%d) read, min=%u, max=%u\n", type,
				(volume_value & 0xffff),
				((volume_value >> 16) & 0xffff));
		break;
	}

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_as_volume, as_volume, audio system volume control);
