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
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

#include <rpdata.h>
#include <console.h>

#include <hal_time.h>

#include <AudioSystem.h>

#ifdef CONFIG_COMPONENTS_USB_GADGET_ADB_FORWARD
#include <adb_forward.h>
#endif


static int g_rpd_verbose = 0;
static int g_rpd_forward_port = 0;
#define LOCAL_PRIORITIES 	\
	(configMAX_PRIORITIES > 20 ? configMAX_PRIORITIES - 8 : configMAX_PRIORITIES - 3)

static void rpdata_demo_usage(void)
{
	printf("Usgae: rpdata_demo [option]\n");
	printf("-h,          rpdata help\n");
	printf("-m,          mode, 0-send; 1-recv;\n");
	printf("-t,          type, type name\n");
	printf("-n,          name, id name\n");
	printf("             (type + name) specify unique data xfer\n");
	printf("-d,          dir, remote processor, 1-cm33;2-c906;3-dsp\n");
	printf("\n");
	printf("DSP -> RV\n");
	printf("rpccli dsp rpdata_audio -m 0 -d 2 -t DSPtoRVAudio -n RVrecvDSPsend\n");
	printf("rpdata_audio -m 1 -d 3 -t DSPtoRVAudio -n RVrecvDSPsend\n");
	printf("\n");
}

struct rpdata_arg_test {
	char type[32];
	char name[32];
	int dir;
};

#define RPDATA_AUDIO_CHECK_CHAR 	(0xaa)

static int audio_data_process(void *in, void *out, uint32_t len)
{
#if 0
	/* mono channel data */
	memset(out, RPDATA_AUDIO_CHECK_CHAR, len);
#else
	memcpy(out, in, len);
#endif
	return 0;
}

static void rpdata_audio_send(void *arg)
{
	struct rpdata_arg_test targ;
	rpdata_t *rpd;
	void *buffer = NULL;
	int ret = -1;
	tAudioRecord *ar;
	int rate = 16000;
	int bits = 16;
	int channels = 3;
	int frame_bytes = bits / 8 * channels;
	uint32_t len = frame_bytes * rate / 100; /* 10ms */
#if 0
	uint32_t rpdata_len = bits / 8 * 1 * rate / 100; /* data after processing(aec, mono ch) */
#else
	uint32_t rpdata_len = len ;
#endif
	void *record_buf = NULL;

	memcpy(&targ, arg, sizeof(struct rpdata_arg_test));
	printf("dir:%d, type:%s, name:%s\n",
		targ.dir, targ.type, targ.name);

	rpd = rpdata_create(targ.dir, targ.type, targ.name, rpdata_len);
	if (!rpd) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}

	buffer = rpdata_buffer_addr(rpd);
	if (!buffer) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}

	ar = AudioRecordCreate("default");
	if (!ar) {
		printf("ar create failed\n");
		goto exit;
	}

	record_buf = malloc(len);
	if (!record_buf) {
		printf("no memory\n");
		goto exit;
	}

	AudioRecordSetup(ar, rate, channels, bits);
	AudioRecordStart(ar);

	while (1) {
		ret = AudioRecordRead(ar, record_buf, len);
		if (ret != len) {
			printf("ret(%d) != len(%d)\n", ret, len);
			break;
		}
		if (rpdata_is_connect(rpd) != 0)
			continue;
		audio_data_process(record_buf, buffer, len);
		ret = rpdata_send(rpd, 0, rpdata_len);
		if (ret != 0) {
			printf("[%s] line:%d \n", __func__, __LINE__);
			goto exit;
		}
	}

exit:
	if (ar)
		AudioRecordDestroy(ar);
	if (rpd)
		rpdata_destroy(rpd);
	printf("rpdata auto send test finish\n");
	vTaskDelete(NULL);
}


static int do_rpdata_audio_send(struct rpdata_arg_test *targ)
{
	TaskHandle_t handle;

	xTaskCreate(rpdata_audio_send, "rpd_audio_send", 1024, targ,
			LOCAL_PRIORITIES, &handle);
	hal_msleep(500);
	return 0;
}

#ifdef CONFIG_COMPONENTS_USB_GADGET_ADB_FORWARD
static int rpd_audio_forward_cb(rpdata_t *rpd, void *data, uint32_t data_len)
{
	int ret;

	if (!g_rpd_forward_port)
		return 0;

	ret = adb_forward_send(g_rpd_forward_port, data, data_len);
	/*printf("[%s] line:%d ret=%d, data_len=%d\n", __func__, __LINE__, ret, data_len);*/
	return 0;
}

struct rpdata_cbs rpd_audio_forward_cbs = {
	.recv_cb = rpd_audio_forward_cb,
};
#endif

static int rpd_audio_recv_cb(rpdata_t *rpd, void *data, uint32_t data_len)
{
	static int count = 0;

#if 1
	int i;
	for (i = 0; i < data_len; i++) {
		uint8_t *c = data + i;
		if (*c  != RPDATA_AUDIO_CHECK_CHAR) {
			printf("data check failed, expected 0x%x but 0x%x\n",
				RPDATA_AUDIO_CHECK_CHAR, *c);
			return 0;
		}
	}
#endif
	if (!g_rpd_verbose)
		return 0;
	if (count++ % g_rpd_verbose == 0)
		printf("audio data check ok(print interval %d)\n", g_rpd_verbose);
	return 0;
}

struct rpdata_cbs rpd_audio_cbs = {
	.recv_cb = rpd_audio_recv_cb,
};

static void rpdata_audio_recv(void *arg)
{
	struct rpdata_arg_test targ;
	rpdata_t *rpd;
	void *buffer = NULL;

	memcpy(&targ, arg, sizeof(struct rpdata_arg_test));
	printf("dir:%d, type:%s, name:%s\n",
		targ.dir, targ.type, targ.name);

	rpd = rpdata_connect(targ.dir, targ.type, targ.name);
	if (!rpd) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}

	buffer = rpdata_buffer_addr(rpd);
	if (!buffer) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}

#ifdef CONFIG_COMPONENTS_USB_GADGET_ADB_FORWARD
	rpdata_set_recv_cb(rpd, &rpd_audio_forward_cbs);
#else
	rpdata_set_recv_cb(rpd, &rpd_audio_cbs);
#endif
	while (1) {
		hal_msleep(10000);
	}
exit:
	if (rpd)
		rpdata_destroy(rpd);
	printf("rpdata audio recv test finish\n");
	vTaskDelete(NULL);
}

static int do_rpdata_audio_recv(struct rpdata_arg_test *targ)
{
	TaskHandle_t handle;

	xTaskCreate(rpdata_audio_recv, "rpd_audio_recv", 1024, targ,
			LOCAL_PRIORITIES, &handle);
	hal_msleep(500);
	return 0;
}

static int check_dir(int dir)
{
	switch (dir) {
	case RPDATA_DIR_CM33:
	case RPDATA_DIR_RV:
	case RPDATA_DIR_DSP:
		return 0;
	default:
		return -1;
	}
}

static int cmd_rpdata_audio(int argc, char *argv[])
{
	int c, mode = 2;
	struct rpdata_arg_test targ = {
		.type = "RVtoDSPAudio",
		.name = "RVrecvDSPsend",
		.dir  = RPDATA_DIR_DSP,
	};

	optind = 0;
	while ((c = getopt(argc, argv, "hm:t:n:d:v:f:")) != -1) {
		switch (c) {
		case 'm':
			mode = atoi(optarg);
			break;
		case 't':
			strncpy(targ.type, optarg, sizeof(targ.type));
			break;
		case 'n':
			strncpy(targ.name, optarg, sizeof(targ.name));
			break;
		case 'd':
			targ.dir = atoi(optarg);
			break;
		case 'v':
			g_rpd_verbose = atoi(optarg);
			return 0;
		case 'f':
			g_rpd_forward_port = atoi(optarg);
#ifdef CONFIG_COMPONENTS_USB_GADGET_ADB_FORWARD
			adb_forward_create_with_rawdata(g_rpd_forward_port);
#endif
			break;
		case 'h':
		default:
			goto usage;
		}
	}

	if (mode != 0 && mode != 1)
		goto usage;

	if (check_dir(targ.dir) < 0)
		goto usage;

	if (mode == 0)
		do_rpdata_audio_send(&targ);
	else if (mode == 1)
		do_rpdata_audio_recv(&targ);

	return 0;
usage:
	rpdata_demo_usage();
	return -1;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_rpdata_audio, rpdata_audio, rpdata audio demo);
