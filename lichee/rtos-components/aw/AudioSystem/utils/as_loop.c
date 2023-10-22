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

#include <hal_thread.h>
#include <hal_sem.h>
#include <hal_mutex.h>
#include <hal_time.h>

#ifdef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM_UTILS_LOOP_AEC
#include <echo_cancellation.h>
#endif


static unsigned int g_delay_time = 100;

typedef struct {
	void *data;
	unsigned int data_len;
	unsigned int data_frames;
	unsigned int rate,channels;
	unsigned int w_ptr, r_ptr;
	hal_sem_t w_sem, r_sem;
	unsigned char w_wait, r_wait;
	unsigned char empty;
	unsigned char exit_flag;
	hal_mutex_t mutex;
} data_xfer_t;

typedef struct {
	uint16_t crate;
	uint16_t cchannels;
	uint16_t cbits;
	char cname[32];

	uint16_t prate;
	uint16_t pchannels;
	uint16_t pbits;
	char pname[32];

	int8_t c_abort;
	int8_t p_abort;
} asloop_t;


static data_xfer_t g_data_xfer;
static asloop_t *g_asloop = NULL;

static int data_xfer_create(data_xfer_t *xfer,
		unsigned int rate, unsigned int channels, unsigned int ms)
{
	if (xfer->data) {
		printf("xfer->data already malloc, aloop maybe running?\n");
		return -1;
	}

	xfer->channels = channels;
	xfer->rate = rate;
	xfer->w_ptr = xfer->r_ptr = 0;
	xfer->w_wait = xfer->r_wait = 0;
	xfer->empty = 1;
	xfer->exit_flag = 0;

	xfer->data_frames = xfer->rate * ms / 1000;
	xfer->data_len = xfer->data_frames * xfer->channels * 2;
	/*printf("[%s] line:%d data xfer size=%d\n", __func__, __LINE__, xfer->data_len);*/
	xfer->data  = malloc(xfer->data_len);
	if (!xfer->data) {
		printf("no memory\n");
		return -1;
	}

	xfer->w_sem = hal_sem_create(0);
	xfer->r_sem = hal_sem_create(0);
	xfer->mutex = hal_mutex_create();

	return 0;
}

/* write available */
static inline unsigned int data_w_avail(data_xfer_t *xfer)
{
	unsigned int residue;

	if (xfer->w_ptr < xfer->r_ptr)
		residue = xfer->r_ptr - xfer->w_ptr;
	else if (xfer->w_ptr == xfer->r_ptr && xfer->empty != 1)
		residue = 0;
	else
		residue = xfer->data_frames - xfer->w_ptr + xfer->r_ptr;
	return residue;
}

/* read available */
static inline unsigned int data_r_avail(data_xfer_t *xfer)
{
	unsigned int residue;

	if (xfer->w_ptr > xfer->r_ptr)
		residue = xfer->w_ptr - xfer->r_ptr;
	else if (xfer->w_ptr == xfer->r_ptr && xfer->empty == 1)
		residue = 0;
	else
		residue = xfer->data_frames - xfer->r_ptr + xfer->w_ptr;
	return residue;
}

/* read:0; write:1 */
static inline int data_sem_wait(data_xfer_t *xfer, int rw)
{
	unsigned char *wait;
	hal_sem_t sem;
	int ret;

	if (rw) {
		wait = &xfer->w_wait;
		sem = xfer->w_sem;
	} else {
		wait = &xfer->r_wait;
		sem = xfer->r_sem;
	}

	*wait = 1;
	hal_mutex_unlock(xfer->mutex);
	ret = hal_sem_wait(sem);
	hal_mutex_lock(xfer->mutex);
	*wait = 0;

	return ret;
}

static inline int data_sem_emit(data_xfer_t *xfer, int rw)
{
	unsigned char *wait;
	hal_sem_t sem;

	if (rw) {
		wait = &xfer->w_wait;
		sem = xfer->w_sem;
	} else {
		wait = &xfer->r_wait;
		sem = xfer->r_sem;
	}

	if (*wait == 0)
		return 0;
	return hal_sem_post(sem);
}

#define DUMP_DATA_INFO() \
do { \
	printf("[%s](%d) residue=%u, size=%u, w_p=%u, r_p=%u, empty=%u\n", \
			__func__, __LINE__, \
			residue, size, xfer->w_ptr, xfer->r_ptr, xfer->empty); \
} while (0)

static int data_push(data_xfer_t *xfer, void *data, unsigned int size)
{
	int ret = 0;
	unsigned int bytes, ofs;
	unsigned int residue;

	hal_mutex_lock(xfer->mutex);
	residue = data_w_avail(xfer);
	/*DUMP_DATA_INFO();*/
	while (residue < size) {
		data_sem_wait(xfer, 1);
		residue = data_w_avail(xfer);
		if (xfer->exit_flag) {
			hal_mutex_unlock(xfer->mutex);
			return -1;
		}
	}

	bytes = size;
	ofs = xfer->w_ptr;
	residue = xfer->data_len - ofs;
	if (size > residue) {
		memcpy(xfer->data + ofs, data, residue);
		memcpy(xfer->data, data + residue, bytes - residue);
		xfer->w_ptr = bytes - residue;
	} else {
		memcpy(xfer->data + ofs, data, bytes);
		xfer->w_ptr += size;
	}
	xfer->empty = 0;

	ret = data_sem_emit(xfer, 0);
	hal_mutex_unlock(xfer->mutex);

	return ret;
}

static int data_pop(data_xfer_t *xfer, void *data, unsigned int size)
{
	int ret = 0;
	unsigned int bytes, ofs;
	unsigned int residue;

	hal_mutex_lock(xfer->mutex);
	residue = data_r_avail(xfer);
	/*DUMP_DATA_INFO();*/
	while (residue < size) {
		data_sem_wait(xfer, 0);
		residue = data_r_avail(xfer);
		if (xfer->exit_flag) {
			hal_mutex_unlock(xfer->mutex);
			return -1;
		}
	}

	/*TODO: rate,ch,format not equal */
	bytes = size;
	ofs = xfer->r_ptr;
	residue = xfer->data_len - ofs;
	if (bytes > residue) {
		memcpy(data, xfer->data + ofs, residue);
		memcpy(data + residue, xfer->data, bytes - residue);
		xfer->r_ptr = bytes - residue;
	} else {
		memcpy(data, xfer->data + ofs, bytes);
		xfer->r_ptr += size;
	}
	if (xfer->r_ptr == xfer->w_ptr)
		xfer->empty = 1;

	ret = data_sem_emit(xfer, 1);

	hal_mutex_unlock(xfer->mutex);

	return ret;
}

static int data_wait_enough_data(data_xfer_t *xfer, unsigned int frames)
{
	unsigned int residue;

	hal_mutex_lock(xfer->mutex);
	residue = data_r_avail(xfer);
	/*DUMP_DATA_INFO();*/
	while (residue < frames) {
		data_sem_wait(xfer, 0);
		residue = data_r_avail(xfer);
		if (xfer->exit_flag) {
			hal_mutex_unlock(xfer->mutex);
			return -1;
		}
	}
	hal_mutex_unlock(xfer->mutex);
	return 0;
}

static void playback_task(void *arg)
{
	asloop_t *asl = (asloop_t *)arg;
	tAudioTrack *at;
	char *audiobuf = NULL;
	unsigned int chunk_bytes;
	unsigned int frames_loop = 320;

	printf("[%s] line:%d playback start\n", __func__, __LINE__);


	at = AudioTrackCreate(asl->pname);
	if (!at) {
		printf("at create failed\n");
		goto pb_exit;
	}

	chunk_bytes = frames_loop * asl->pchannels * asl->pbits / 8;
	audiobuf = malloc(chunk_bytes);
	if (!audiobuf) {
		printf("no memory...\n");
		goto pb_exit;
	}

	AudioTrackSetup(at, asl->prate, asl->pchannels, asl->pbits);

	/* wait enough data */
	if (data_wait_enough_data(&g_data_xfer, g_data_xfer.data_frames / 2) < 0)
		goto pb_exit;

	while (!asl->p_abort) {
		long f = chunk_bytes;

		/* read data from data buffer */
		/*printf("[%s] line:%d f=%d\n", __func__, __LINE__, chunk_bytes);	*/
		data_pop(&g_data_xfer, audiobuf, f);
		AudioTrackWrite(at, audiobuf, f);
	}

	AudioTrackStop(at);

pb_exit:
	if (audiobuf)
		free(audiobuf);
	if (at != NULL) {
		AudioTrackDestroy(at);
		at = NULL;
	}
	asl->p_abort = 2;
	hal_thread_stop(NULL);
}

#ifndef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM_UTILS_LOOP_AEC
static long aec_process(int16_t *buf, uint32_t size, uint32_t ch, uint32_t bits)
{
	int ofs_src = 0, ofs_dst = 0;
	int byte = bits / 8;

	while (1) {
		memcpy(buf + ofs_dst, buf + ofs_src, byte);
		ofs_dst++;
		ofs_src += ch;
		if (ofs_dst * ch * byte >= size)
			break;
	}
	return size / ch;
}
#else

#define AEC_SAMPLE_COUNT	(160)
static void *pAecmInst = NULL;
static long aec_process(int16_t *buf, uint32_t size, uint32_t ch, uint32_t bits)
{
	int i, ret;
	int16_t far_frame[AEC_SAMPLE_COUNT];
	int16_t near_frame[AEC_SAMPLE_COUNT];
	int total_frame = size / ch / 2;

	if (ch != 3 || bits != 16 || total_frame != AEC_SAMPLE_COUNT)
		return -1;

	for (i = 0; i < total_frame; i++) {
		near_frame[i] = (buf[3 * i + 0] / 2) + (buf[3 * i + 1] / 2);
		far_frame[i] = buf[3 * i + 2];
	}

	ret = WebRtcAec_BufferFarend(pAecmInst, far_frame, AEC_SAMPLE_COUNT);
	if (ret < 0)  {
		printf("Aec BufferFarend failed, return!\n");
		return -1;
	}

	ret = WebRtcAec_Process(pAecmInst, near_frame, NULL, buf, NULL, AEC_SAMPLE_COUNT, 0, 0);
	if (ret < 0)  {
		printf("Aec Process failed, return!\n");
		return -1;
	}

	return size / ch;
}

static int aec_init(uint32_t rate)
{
	int ret = 0;
	AecConfig config = {0};

	if (pAecmInst != NULL)
		return -1;

	ret = WebRtcAec_Create(&pAecmInst);
	if (ret < 0) {
		printf("Aec_Create error\n");
		return ret;
	}

	ret = WebRtcAec_Init(pAecmInst, rate, rate);
	if (ret < 0) {
		printf("Aec_Init error\n");
		return ret;
	}

	config.nlpMode = kAecNlpAggressive;
	ret = WebRtcAec_set_config(pAecmInst, config);
	if (ret < 0) {
		printf("Aec_set_config error\n");
		return ret;
	}
	return ret;
}

static int aec_release(void)
{
	WebRtcAec_Free(pAecmInst);
	pAecmInst = NULL;
	return 0;
}
#endif

static void capture_task(void *arg)
{
	asloop_t *asl = (asloop_t *)arg;
	tAudioRecord *ar;
	char *audiobuf = NULL;
	unsigned int chunk_bytes;
#ifdef AEC_SAMPLE_COUNT
	unsigned int frames_loop = 160;
#else
	unsigned int frames_loop = 320;
#endif

	printf("[%s] line:%d capture start\n", __func__, __LINE__);

	ar = AudioRecordCreate(asl->cname);
	if (!ar) {
		printf("ar create failed\n");
		goto cap_exit;
	}

	chunk_bytes = frames_loop * asl->cchannels * asl->cbits / 8;

	audiobuf = malloc(chunk_bytes);
	if (!audiobuf) {
		printf("no memory...\n");
		goto cap_exit;
	}

	AudioRecordSetup(ar, asl->crate, asl->cchannels, asl->cbits);
	AudioRecordStart(ar);

#ifdef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM_UTILS_LOOP_AEC
	if (aec_init(asl->crate) < 0)
		goto cap_exit;
#endif

	while (!asl->c_abort) {
		long f = chunk_bytes;
		f = AudioRecordRead(ar, audiobuf, f);
		if (f < 0) {
			printf("ar read error, return %ld\n", f);
			break;
		}
		/* do process */
		f = aec_process((int16_t *)audiobuf, f, asl->cchannels, asl->cbits);

		/*printf("[%s] line:%d f=%d\n", __func__, __LINE__, f);	*/
		/* fill into buffer and notify */
		data_push(&g_data_xfer, audiobuf, f);
	}

	AudioRecordStop(ar);

cap_exit:
	if (audiobuf)
		free(audiobuf);
	if (ar != NULL) {
		AudioRecordDestroy(ar);
		ar = NULL;
	}
	asl->c_abort = 2;
	hal_thread_stop(NULL);
}

static int cap_task_create(asloop_t *asl)
{
	hal_thread_t handle;

	handle = hal_thread_create(capture_task, asl, "ASL_Cap", 4096, HAL_THREAD_PRIORITY_APP);
	if (!handle) {
		printf("ASL_Cap task create failed\n");
		return -1;
	}
	return 0;
}

static int pb_task_create(asloop_t *asl)
{
	hal_thread_t handle;

	handle = hal_thread_create(playback_task, asl, "ASL_Pb", 4096, HAL_THREAD_PRIORITY_APP);
	if (!handle) {
		printf("ASL_Pb task create failed\n");
		return -1;
	}
	return 0;
}

static void killall_task(void)
{
	data_xfer_t *xfer = &g_data_xfer;
	int timeout = 2000;

	if (!xfer->data)
		return;
	if (!g_asloop)
		return;

	g_asloop->c_abort = 1;
	g_asloop->p_abort = 1;
	xfer->exit_flag = 1;

	while (timeout > 0) {
		int ms = 100;

		data_sem_emit(xfer, 0);
		data_sem_emit(xfer, 1);
		hal_msleep(ms);
		timeout -= ms;
		if (g_asloop->c_abort == 2 && g_asloop->p_abort == 2) {
			printf("cap,pb all released\n");
			break;
		}
	}
	if (timeout <= 0)
		printf("kill cap,pb task timeout\n");

	free(xfer->data);
	hal_sem_delete(xfer->w_sem);
	hal_sem_delete(xfer->r_sem);
	hal_mutex_delete(xfer->mutex);
	memset(xfer, 0, sizeof(data_xfer_t));
	free(g_asloop);
	g_asloop = NULL;

#ifdef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM_UTILS_LOOP_AEC
	aec_release();
#endif
}

static void usage(void)
{
	printf("Usage: asloop [option]\n");
	printf("-D,          AudioRecord name\n");
	printf("-d,          AudioTrack name\n");
	printf("-R,          Audio Record rate\n");
	printf("-r,          Audio Track rate\n");
	printf("-C,          Audio Record channels\n");
	printf("-c,          Audio Track channels\n");
	printf("-B,          Audio Record bits\n");
	printf("-b,          Audio Track bits\n");
	printf("-t,          delay time\n");
	printf("-k,          kill asloop\n");
	printf("\n");
}

int cmd_asloop(int argc, char ** argv)
{
	int c, ret;
	asloop_t asl = {0};

	/* default setting */
	asl.crate = 16000;
	asl.cchannels = 3;
	asl.cbits = 16;
	strcpy(asl.cname, "default");

	asl.prate = 16000;
	asl.pchannels = 1;
	asl.pbits = 16;
	strcpy(asl.pname, "default");

	optind = 0;
	while ((c = getopt(argc, argv, "D:d:R:r:C:c:B:b:kt:h")) != -1) {
		switch (c) {
		case 'D':
			strncpy(asl.cname, optarg, sizeof(asl.cname));
			break;
		case 'd':
			strncpy(asl.pname, optarg, sizeof(asl.pname));
			break;
		case 'R':
			asl.crate = atoi(optarg);
			break;
		case 'r':
			asl.prate = atoi(optarg);
			break;
		case 'C':
			asl.cchannels = atoi(optarg);
			break;
		case 'c':
			asl.pchannels = atoi(optarg);
			break;
		case 'B':
			asl.cbits = atoi(optarg);
			break;
		case 'b':
			asl.pbits = atoi(optarg);
			break;
		case 't':
			g_delay_time = atoi(optarg);
			break;
		case 'k':
			killall_task();
			goto error_exit;
		case 'h':
		default:
			usage();
			return -1;
		}
	}

	if (g_asloop) {
		printf("asloop already running\n");
		return -1;
	}

	g_asloop = malloc(sizeof(asloop_t));
	if (!g_asloop) {
		printf("no memory\n");
		return -1;
	}
	memcpy(g_asloop, &asl, sizeof(asloop_t));

	ret = data_xfer_create(&g_data_xfer, g_asloop->crate, 1, g_delay_time);
	if (ret != 0)
		goto error_exit;
	ret = cap_task_create(g_asloop);
	if (ret != 0)
		goto task_exit;
	ret = pb_task_create(g_asloop);
	if (ret != 0)
		goto task_exit;

	return 0;

task_exit:
	g_asloop->c_abort = 1;
	g_asloop->p_abort = 1;
	hal_msleep(500);
error_exit:
	if (g_asloop) {
		free(g_asloop);
		g_asloop = NULL;
	}

	return 0;
}

FINSH_FUNCTION_EXPORT_CMD(cmd_asloop, asloop, capture and play loopback);
