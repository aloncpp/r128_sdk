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
#include <hal_mem.h>
#include <hal_thread.h>
#include <md5.h>

//#define RECV_CALLBACK_TEST

/* print md5 check result */
static int g_rpd_verbose = 0;

#define configAPPLICATION_RPDATA_DEMO_PRIORITY	\
	(configMAX_PRIORITIES > 20 ? configMAX_PRIORITIES - 8 : configMAX_PRIORITIES - 3)

static void rpdata_demo_usage(void)
{
	printf("Usgae: rpdata_demo [option]\n");
	printf("-h,          rpdata help\n");
	printf("-m,          mode, 0-send; 1-recv; 2-recv+send\n");
	printf("-t,          type, type name\n");
	printf("-n,          name, id name\n");
	printf("             (type + name) specify unique data xfer\n");
	printf("-c,          string, send string\n");
	printf("-i,          single test, do send or recv test once\n");
	printf("-d,          dir, remote processor, 1-cm33;2-c906;3-dsp\n");
	printf("-s,          src dir, valid in mode 2\n");
	printf("\n");
	printf("RV -> DSP\n");
	printf("rpdata_demo -m 0 -d 3 -t RVtoDSP -n RVsendDSPrecv\n");
	printf("rpccli dsp rpdata_demo -m 1 -d 2 -t RVtoDSP -n RVsendDSPrecv\n");
	printf("\n");
	printf("RV -> M33\n");
	printf("rpdata_demo -m 0 -d 1 -t RVtoM33 -n RVsendM33recv\n");
	printf("rpccli arm rpdata_demo -m 1 -d 2 -t RVtoM33 -n RVsendM33recv\n");
	printf("\n");
	printf("RV <- M33\n");
	printf("rpdata_demo -m 1 -d 1 -t M33toRV -n RVrecvM33send\n");
	printf("rpccli arm rpdata_demo -m 0 -d 2 -t M33toRV -n RVrecvM33send\n");
	printf("\n");
	printf("RV -> DSP -> M33\n");
	printf("rpccli dsp rpdata_demo -d 2 -t RVtoDSP -n DSPrecvRVsend -s 1 -t DSPtoM33 -n M33recvDSPsend\n");
	printf("rpccli arm rpdata_demo -m 1 -d 3 -t DSPtoM33 -n M33recvDSPsend\n");
	printf("rpdata_demo -m 0 -d 3 -t RVtoDSP -n DSPrecvRVsend\n");
	printf("\n");
	printf("RV -> DSP -> M33 -> DSP -> RV\n");
	printf("rpccli dsp rpdata_demo -d 2 -t RD -n rs -s 1 -t DM -n ds\n");
	printf("rpccli arm rpdata_demo -d 3 -t DM -n ds -s 3 -t MD -n ms\n");
	printf("rpccli dsp rpdata_demo -d 1 -t MD -n ms -s 2 -t DR -n ds\n");
	printf("rpdata_demo -m 1 -d 3 -t DR -n ds\n");
	printf("rpdata_demo -m 0 -d 3 -t RD -n rs\n");
	printf("\n");
	printf("RV->M33->RV process(M33 do aec process: input mic+ref, and output aec data to RV)\n");
	printf("rpccli arm rpdata_demo -p -m 1 -d 2 -t ALGO -n AEC\n");
	printf("rpdata_demo -p -m 0 -d 1 -t ALGO -n AEC\n");
	printf("\n");
}

struct rpdata_arg_test {
	char type[32];
	char name[32];
	int dir;
	char stype[32];
	char sname[32];
	int sdir;
};

static int do_rpdata_send_test(struct rpdata_arg_test *targ, void *data, uint32_t len)
{
	rpdata_t *rpd;
	void *buffer;
	int ret = -1;

	printf("Cteate %s:%s.\n", targ->type, targ->name);
	rpd = rpdata_create(targ->dir, targ->type, targ->name, len);
	if (!rpd) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		return -1;
	}
#ifndef RECV_CALLBACK_TEST
	rpdata_set_recv_ringbuffer(rpd, 2 * rpdata_buffer_len(rpd));
#endif

	buffer = rpdata_buffer_addr(rpd);
	if (!buffer) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}

	memcpy(buffer, data, len);
	rpdata_wait_connect(rpd);
	ret = rpdata_send(rpd, 0, len);
	if (ret != 0) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}

exit:
	if (rpd)
		rpdata_destroy(rpd);
	return ret;
}

static int do_rpdata_recv_test(struct rpdata_arg_test *targ, void *data, uint32_t len)
{
	rpdata_t *rpd;
	char *rx_buf;
	int ret = -1;

	rx_buf = hal_malloc(len + 1);
	if (rx_buf == NULL) {
		printf("[%s] line:%d alloc rx buffer fialed.\n", __func__, __LINE__);
		return -1;
	}

	printf("connect to %s:%s.\n", targ->type, targ->name);
	rpd = rpdata_connect(targ->dir, targ->type, targ->name);
	if (!rpd) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		return -1;
	}
#ifndef RECV_CALLBACK_TEST
	rpdata_set_recv_ringbuffer(rpd, 2 * rpdata_buffer_len(rpd));
#endif

	while (1) {
		ret = rpdata_recv(rpd, rx_buf, len, 10000);
		if (ret <= 0) {
			printf("rpdata recv timeout \n");
			goto exit;
		}
		rx_buf[ret] = 0;
		printf("%s, len:%d\n", rx_buf, ret);
		if (!memcmp("quit", rx_buf, 4))
			break;
	}

exit:
	if (rx_buf)
		hal_free(rx_buf);
	if (rpd)
		rpdata_destroy(rpd);
	return ret;
}

static void data_fill(void *buffer, uint32_t len)
{
	int i;
	int data_len = len - 16;

	memset(buffer, 0, len);
	for (i = 0; i < len;) {
		*(int *)(buffer + i) = rand();
		i += sizeof(int);
	}
	md5(buffer, data_len,  buffer + data_len);
}

static int data_check(void *buffer, uint32_t len)
{
	unsigned char res[16];
	int data_len = len - 16;

	md5(buffer, data_len, res);
	if (!memcmp(buffer + data_len, res, 16))
		return 0;
	return -1;
}

static void rpdata_auto_send(void *arg)
{
	struct rpdata_arg_test targ;
	rpdata_t *rpd;
	void *buffer;
	int ret = -1;
	uint32_t len = 512;


	memcpy(&targ, arg, sizeof(struct rpdata_arg_test));
	printf("dir:%d, type:%s, name:%s\n",
		targ.dir, targ.type, targ.name);

	rpd = rpdata_create(targ.dir, targ.type, targ.name, 512);
	if (!rpd) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}
#ifndef RECV_CALLBACK_TEST
	rpdata_set_recv_ringbuffer(rpd, 2 * rpdata_buffer_len(rpd));
#endif

	buffer = rpdata_buffer_addr(rpd);
	if (!buffer) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}

	rpdata_wait_connect(rpd);
	while (1) {
		data_fill(buffer, len);
		ret = rpdata_send(rpd, 0, len);
		if (ret != 0) {
			printf("[%s] line:%d \n", __func__, __LINE__);
			goto exit;
		}
		hal_msleep(10);
	}

exit:
	if (rpd)
		rpdata_destroy(rpd);
	printf("rpdata auto send test finish\n");
	vTaskDelete(NULL);
}

static int rpd_demo_recv_cb(rpdata_t *rpd, void *data, uint32_t data_len)
{
	static int count = 0;
	if (data_check(data, data_len) < 0) {
		printf("check md5 failed\n");
	}
	if (!g_rpd_verbose)
		return 0;
	if (count++ % g_rpd_verbose == 0)
		printf("check md5 ok(print interval %d)\n", g_rpd_verbose);
	return 0;
}

struct rpdata_cbs rpd_demo_cbs = {
	.recv_cb = rpd_demo_recv_cb,
};

static int rpd_demo_process_cb(rpdata_t *rpd, void *buffer, uint32_t data_len)
{
	void *mic_data, *ref_data, *aec_data;
	uint32_t mic_offset = 128;
	uint32_t ref_offset = 128 + 640;
	uint32_t aec_offset = 128 + 640 + 320;
	uint32_t total_len = 1408;

	/* aec process
	 * input:
	 * params:
	 * simulator: 128bytes
	 * mic data: 2ch+16K+16bit, 10ms, 160*4=640
	 * ref data: 1ch+16K+16bit, 10ms, 160*2=320
	 * outout:
	 * aec data: 1ch+16K+16bit, 10ms, 160*2=320
	 *
	 * total= 128 + 640 + 320 + 320 = 1408
	 * 1408 is cacheline lenght
	 */

	mic_data = buffer + mic_offset;
	ref_data = buffer + ref_offset;
	aec_data = buffer + aec_offset;

	if (data_len != total_len) {
		printf("expected len:%d but:%d\n", total_len, data_len);
		return -1;
	}
	if (data_check(mic_data, ref_data - mic_data) < 0) {
		printf("check mic data md5 failed\n");
	}
	if (data_check(ref_data, aec_data - ref_data) < 0) {
		printf("check ref data md5 failed\n");
	}
	data_fill(aec_data, total_len - aec_offset);
	return 0;
}

struct rpdata_cbs rpd_demo_process_cbs = {
	.recv_cb = rpd_demo_process_cb,
};

static int rpd_demo_recv_and_send_cb(rpdata_t *rpd, void *data, uint32_t data_len)
{
	rpdata_t *rpd_send = NULL;
	void *buffer;
	int ret;

	if (data_check(data, data_len) < 0) {
		printf("[%s] line:%d check md5 failed\n", __func__, __LINE__);
	}
	rpd_send = rpdata_get_private_data(rpd);
	if (!rpd_send)
		return -1;

	buffer = rpdata_buffer_addr(rpd_send);
	memcpy(buffer, data, data_len);
	ret = rpdata_send(rpd_send, 0, data_len);
	if (ret != 0) {
		printf("rpdata_send failed\n");
		return ret;
	}

	return 0;
}

struct rpdata_cbs rpd_demo_rs_cbs = {
	.recv_cb = rpd_demo_recv_and_send_cb,
};

static void rpdata_auto_recv(void *arg)
{
	struct rpdata_arg_test targ;
	rpdata_t *rpd;
	void *rx_buf = NULL;
	int len, ret;

	memcpy(&targ, arg, sizeof(struct rpdata_arg_test));
	printf("dir:%d, type:%s, name:%s\n",
		targ.dir, targ.type, targ.name);

	rpd = rpdata_connect(targ.dir, targ.type, targ.name);
	if (!rpd) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}
#ifndef RECV_CALLBACK_TEST
	rpdata_set_recv_ringbuffer(rpd, 2 * rpdata_buffer_len(rpd));
#endif

	len = rpdata_buffer_len(rpd);
	if (len <= 0) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}

	rx_buf = hal_malloc(len);
	if (!rx_buf) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}

#ifdef RECV_CALLBACK_TEST
	rpdata_set_recv_cb(rpd, &rpd_demo_cbs);
	while (1) {
		hal_msleep(10);
	}
#else
	while (1) {
		ret = rpdata_recv(rpd, rx_buf, len, 10000);
		if (ret <= 0) {
			printf("rpdata recv timeout \n");
			goto exit;
		}
		if (data_check(rx_buf, ret) < 0) {
			printf("check md5 failed\n");
		}
	}
#endif
exit:
	if (rx_buf)
		hal_free(rx_buf);
	if (rpd)
		rpdata_destroy(rpd);
	printf("rpdata auto recv test finish\n");
	vTaskDelete(NULL);
}

static void rpdata_auto_recv_and_send(void *arg)
{
	struct rpdata_arg_test targ;
	rpdata_t *rpd_recv = NULL, *rpd_send = NULL;
	void *buf_recv = NULL, *buf_send = NULL;
	uint32_t len = 512;
	int ret;

	memcpy(&targ, arg, sizeof(struct rpdata_arg_test));
	printf("recv dir:%d, type:%s, name:%s\n",
		targ.dir, targ.type, targ.name);
	printf("send dir:%d, type:%s, name:%s\n",
		targ.sdir, targ.stype, targ.sname);

	rpd_send = rpdata_create(targ.sdir, targ.stype, targ.sname, len);
	if (!rpd_send) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}

	buf_send = rpdata_buffer_addr(rpd_send);
	if (!buf_send) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}

	rpd_recv = rpdata_connect(targ.dir, targ.type, targ.name);
	if (!rpd_recv) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}
#ifndef RECV_CALLBACK_TEST
	rpdata_set_recv_ringbuffer(rpd_recv, 2 * rpdata_buffer_len(rpd_recv));
	rpdata_set_recv_ringbuffer(rpd_send, 2 * rpdata_buffer_len(rpd_send));
#endif

	buf_recv = hal_malloc(len);
	if (!buf_recv) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}

	rpdata_wait_connect(rpd_send);
#ifdef RECV_CALLBACK_TEST
	rpdata_set_private_data(rpd_recv, rpd_send);
	rpdata_set_recv_cb(rpd_recv, &rpd_demo_rs_cbs);
	while (1) {
		hal_msleep(10);
	}
#else
	while (1) {
		ret = rpdata_recv(rpd_recv, buf_recv, len, 10000);
		if (ret <= 0) {
			printf("rpdata recv timeout \n");
			goto exit;
		}
		if (data_check(buf_recv, ret) < 0) {
			printf("check md5 failed\n");
		}
		memcpy(buf_send, buf_recv, ret);
		ret = rpdata_send(rpd_send, 0, ret);
		if (ret != 0) {
			printf("rpdata send failed\n");
			goto exit;
		}
	}
#endif
exit:
	if (buf_recv)
		hal_free(buf_recv);
	if (rpd_send)
		rpdata_destroy(rpd_send);
	if (rpd_recv)
		rpdata_destroy(rpd_recv);
	printf("rpdata auto recv_and_send test finish\n");
	vTaskDelete(NULL);
}

static void rpdata_process_send(void *arg)
{
	struct rpdata_arg_test targ;
	rpdata_t *rpd;
	void *buffer;
	int ret = -1;
	uint32_t len;
	void *params, *mic_data, *ref_data, *aec_data;
	uint32_t params_offset = 0;
	uint32_t mic_offset = 128;
	uint32_t ref_offset = 128 + 640;
	uint32_t aec_offset = 128 + 640 + 320;

	/* aec process
	 * input:
	 * params:
	 * simulator: 128bytes
	 * mic data: 2ch+16K+16bit, 10ms, 160*4=640
	 * ref data: 1ch+16K+16bit, 10ms, 160*2=320
	 * outout:
	 * aec data: 1ch+16K+16bit, 10ms, 160*2=320
	 *
	 * total= 128 + 640 + 320 + 320 = 1408
	 * 1408 is cacheline lenght
	 * */
	len = 1408;

	memcpy(&targ, arg, sizeof(struct rpdata_arg_test));
	printf("dir:%d, type:%s, name:%s\n",
		targ.dir, targ.type, targ.name);

	rpd = rpdata_connect(targ.dir, targ.type, targ.name);
	if (!rpd) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}
#ifndef RECV_CALLBACK_TEST
	rpdata_set_recv_ringbuffer(rpd, 2 * rpdata_buffer_len(rpd));
#endif

	buffer = rpdata_buffer_addr(rpd);
	if (!buffer) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}
	params = buffer + params_offset;
	mic_data = buffer + mic_offset;
	ref_data = buffer + ref_offset;
	aec_data = buffer + aec_offset;

	rpdata_wait_connect(rpd);

	data_fill(params, mic_offset - params_offset);
	data_fill(mic_data, ref_offset - mic_offset);
	data_fill(ref_data, aec_offset - ref_offset);
	memset(aec_data, 0, len - aec_offset);

	ret = rpdata_process(rpd, 0, len);
	if (ret != 0) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}

	if (data_check(aec_data, len - aec_offset) < 0) {
		printf("aec data check failed\n");
	} else {
		printf("aec data check ok\n");
	}

exit:
	if (rpd)
		rpdata_destroy(rpd);
	vTaskDelete(NULL);
}

static void rpdata_process_recv(void *arg)
{
	struct rpdata_arg_test targ;
	rpdata_t *rpd;
	uint32_t len;
	void *rx_buf = NULL;
	int ret;

	/* aec process
	 * input:
	 * params:
	 * simulator: 128bytes
	 * mic data: 2ch+16K+16bit, 10ms, 160*4=640
	 * ref data: 1ch+16K+16bit, 10ms, 160*2=320
	 * outout:
	 * aec data: 1ch+16K+16bit, 10ms, 160*2=320
	 *
	 * total= 128 + 640 + 320 + 320 = 1408
	 * 1408 is cacheline lenght
	 * */
	len = 1408;

	memcpy(&targ, arg, sizeof(struct rpdata_arg_test));
	printf("dir:%d, type:%s, name:%s\n",
		targ.dir, targ.type, targ.name);

	rpd = rpdata_create(targ.dir, targ.type, targ.name, len);
	if (!rpd) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}
#ifndef RECV_CALLBACK_TEST
	rpdata_set_recv_ringbuffer(rpd, 2 * rpdata_buffer_len(rpd));
#endif

#ifndef RECV_CALLBACK_TEST
	rx_buf = hal_malloc(rpdata_buffer_len(rpd));
	if (!rx_buf) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}

	ret = rpdata_recv(rpd, rx_buf, len, 30*1000);
	if (ret <= 0) {
		printf("[%s] : Timeout!\n", __func__);
		goto exit;
	}
	rpd_demo_process_cb(rpd, rx_buf, ret);

#else
	rpdata_set_recv_cb(rpd , &rpd_demo_process_cbs);
	hal_sleep(30);
#endif
exit:
	if (rx_buf)
		hal_free(rx_buf);
	if (rpd)
		rpdata_destroy(rpd);
	printf("rpdata recv process finish\n");
	vTaskDelete(NULL);
}

static int do_rpdata_process_test(struct rpdata_arg_test *targ, int mode)
{
	hal_thread_t handle;

	if (mode == 0)
		handle = hal_thread_create(rpdata_process_send, targ,
				"rpd_send_process", 512, HAL_THREAD_PRIORITY_APP);
	else if (mode == 1)
		handle = hal_thread_create(rpdata_process_recv, targ,
				"rpd_recv_process", 512, HAL_THREAD_PRIORITY_APP);
	return 0;
}

static int do_rpdata_auto_test(struct rpdata_arg_test *targ, int mode)
{
	hal_thread_t handle;

	if (mode == 0)
		handle = hal_thread_create(rpdata_auto_send, targ, "rpd_send_test",
				512, HAL_THREAD_PRIORITY_APP);
	else if (mode == 1)
		handle = hal_thread_create(rpdata_auto_recv, targ, "rpd_recv_test",
				512, HAL_THREAD_PRIORITY_APP);
	else if (mode == 2)
		handle = hal_thread_create(rpdata_auto_recv_and_send, targ, "rpd_rs_test",
				512, HAL_THREAD_PRIORITY_APP);
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

static int cmd_rpdata_demo(int argc, char *argv[])
{
	int c, mode = 2;
	int single_test = 0, process_test = 0;
	int get_sdir_arg = 0;
	/* string/data must be cache_line align */
	char string[128] = "rpdata test string";
	static struct rpdata_arg_test targ = {
		.type = "RVtoDSP",
		.name = "DSPrecvRVsend",
		.dir  = RPDATA_DIR_DSP,
		.stype = "DSPtoM33",
		.sname = "M33recvDSPsend",
		.sdir  = RPDATA_DIR_CM33,
	};

	optind = 0;
	while ((c = getopt(argc, argv, "hm:t:n:c:s:d:iv:p")) != -1) {
		switch (c) {
		case 'm':
			mode = atoi(optarg);
			break;
		case 't':
			if (!get_sdir_arg)
				strncpy(targ.type, optarg, sizeof(targ.type));
			else
				strncpy(targ.stype, optarg, sizeof(targ.stype));
			break;
		case 'n':
			if (!get_sdir_arg)
				strncpy(targ.name, optarg, sizeof(targ.name));
			else
				strncpy(targ.sname, optarg, sizeof(targ.sname));
			break;
		case 'c':
			strncpy(string, optarg, sizeof(string));
			break;
		case 's':
			targ.sdir = atoi(optarg);
			get_sdir_arg = 1;
			break;
		case 'd':
			targ.dir = atoi(optarg);
			get_sdir_arg = 0;
			break;
		case 'i':
			single_test = 1;
			break;
		case 'v':
			g_rpd_verbose = atoi(optarg);
			return 0;
		case 'p':
			process_test = 1;
			break;
		case 'h':
		default:
			goto usage;
		}
	}

	if (mode != 0 && mode != 1 && mode != 2)
		goto usage;

	if (check_dir(targ.dir) < 0 || check_dir(targ.sdir) < 0)
		goto usage;

	if (process_test) {
		do_rpdata_process_test(&targ, mode);
		return 0;
	}

	if (!single_test) {
		do_rpdata_auto_test(&targ, mode);
		return 0;
	}

	if (mode == 0)
		do_rpdata_send_test(&targ, string, sizeof(string));
	else if (mode == 1)
		do_rpdata_recv_test(&targ, string, sizeof(string));

	return 0;
usage:
	rpdata_demo_usage();
	return -1;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_rpdata_demo, rpdata_demo, rpdata test demo);
