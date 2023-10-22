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
#include <stdbool.h>
#include <console.h>

#include <AudioSystem.h>
#include <pm_task.h>

#include <hal_time.h>
#include <hal_sem.h>
#include <hal_thread.h>

#include <aw-alsa-lib/control.h>

#include <pm_wakesrc.h>
#include <pm_devops.h>

#include <pm_rpcfunc.h>

#include <task.h>

#define LPVOICE_VERSION		"V0.1"

enum {
	LPV_STATE_UNKNOWN = 0,
	LPV_STATE_RUNNING,
	LPV_STATE_STOP,
};

enum {
	LPV_MODE_INACTIVE = 0,
	LPV_MODE_ACTIVE,
	LPV_MODE_PENDING,
};

static struct {
	hal_thread_t handle;
	hal_thread_t ar_st_handle;
	hal_sem_t sem_to_lpv;
	hal_sem_t sem_from_lpv;
	tAudioRecord *ar;
	uint32_t rate;
	uint32_t channels;
	uint32_t bits;
	uint16_t fake_wake;
	uint8_t state;
	uint8_t mode;
	uint8_t suspend_notify;
	uint8_t suspend_with_lpv;
	uint8_t resume_with_lpv;

	int irq;

	pm_notify_t notify;
	int notify_id;

	void *private_data;
} g_lpv = {
	.handle = NULL,
	.irq = 0,
	.notify_id = -1,
};

__attribute__((weak)) void *lpv_asr_init(uint32_t rate, uint32_t ch, uint32_t bits)
{
	return NULL;
}

__attribute__((weak)) int lpv_asr_process(void *handle, void *data, uint32_t size, void *out_data,
		int *word_id, int *vad_flag, float *confidence)
{
	return -1;
}

__attribute__((weak)) int lpv_asr_destroy(void *handle)
{
	return -1;
}

static int lpv_enable(bool enable)
{
	int ret = 0;
	int irq = -100;
	wakesrc_type_t type = PM_WAKESRC_SOFT_WAKEUP;

	if (g_lpv.irq != 0)
		goto ws_init_finish;

	ret = pm_wakesrc_register(irq, "lpv_ws", type);
	if (ret != 0) {
		printf("register lpv_ws failed\n");
		return -1;
	}
	g_lpv.irq = irq;
ws_init_finish:

	if (enable)
		ret = pm_set_wakeirq(g_lpv.irq);
	else
		ret = pm_clear_wakeirq(g_lpv.irq);
	if (ret)
		printf("%s lpv_ws failed\n", enable ? "enable" : "disable");
	else
		printf("%s lpv_ws irq: %d\n", enable ? "enable" : "disable", g_lpv.irq);

	return 0;
}

static bool is_lpv_mode(void)
{
	if (!g_lpv.suspend_notify)
		return false;
	if (!pm_wakesrc_is_disabled(g_lpv.irq))
		return true;
	return false;
}

static void thread_lpv_wakeup(void *arg)
{
	bool wake = *(bool *)arg;
	int ret;

	if (!g_lpv.irq)
		goto err;

	printf("[%s] line:%d info wake:%d\n", __func__, __LINE__, wake);
	/*
	 * wake=true, mode=LPV_MODE_ACTIVE, it means lpv->normal
	 * wake=false, mode=LPV_MODE_ACTIVE, it means lpv->mad standby
	 */
	if (wake)
		ret = pm_wakesrc_soft_wakeup(g_lpv.irq, PM_WAKESRC_ACTION_WAKEUP_SYSTEM, 1);
	else
		ret = pm_wakesrc_soft_wakeup(g_lpv.irq, PM_WAKESRC_ACTION_SLEEPY, 0);
	if (ret != 0) {
		printf("soft wakeup failed:%d\n", ret);
		goto err;
	}

err:
	hal_thread_stop(NULL);
}

static int lpv_wakeup(bool wake)
{
#if 0
	int ret = 0;

	if (!g_lpv.irq)
		return -1;

	printf("[%s] line:%d info wake:%d\n", __func__, __LINE__, wake);
	if (wake)
		ret = pm_wakesrc_soft_wakeup(g_lpv.irq, PM_WAKESRC_ACTION_WAKEUP_SYSTEM, 1);
	else
		ret = pm_wakesrc_soft_wakeup(g_lpv.irq, PM_WAKESRC_ACTION_SLEEPY, 0);
	if (ret != 1) {
		printf("soft wakeup failed:%d\n", ret);
		return -1;
	}
#else
	hal_thread_t handle;

	g_lpv.mode = LPV_MODE_PENDING;
	/* FIXME, pm_wakesrc_soft_wakeup spend too much time and make audio overrun, so move into
	 * thread to fix it.
	 */
	handle = hal_thread_create(thread_lpv_wakeup, &wake, "lpv_wakeup", 1024, HAL_THREAD_PRIORITY_APP);
#endif
	return 0;
}

static int lpv_feed(void *data, uint32_t size, void *out_data)
{
	int word_id = 0;
	int vad_flag = 0;
	float confidence = 0.0;
	int ret = 0;

	/* feed pcm data into algo */
	ret = lpv_asr_process(g_lpv.private_data, data, size, out_data,
		&word_id, &vad_flag, &confidence);
	if (ret != 0)
		return -1;

	if (word_id > 0) {
		printf("vad_flag = %d, word_id = %d\n", vad_flag, word_id);
		if (is_lpv_mode())
			lpv_wakeup(true);
	}

	return 0;
}

static void do_interleaved_convert(int16_t *in, int ch, int frame)
{
	int16_t sample[frame * ch];
	int i, j;

	for (i = 0; i < frame; i++) {
		for (j = 0; j < ch; j++) {
			sample[j * frame + i] = in[i * ch + j];
		}
	}
	memcpy(in, sample, frame * ch * sizeof(int16_t));
}

static int lpv_pm_notify_cb(suspend_mode_t mode, pm_event_t event, void *arg)
{
	printf("[%s] line:%d info mode=%d, evnet=%d\n", __func__, __LINE__, mode, event);

	switch (event) {
	case PM_EVENT_SYS_PERPARED:
		/* dsp get suspend notify */
		g_lpv.suspend_notify++;
		break;
	case PM_EVENT_SYS_FINISHED:
		/* dsp get resume notify */
		g_lpv.suspend_notify = 0;
		g_lpv.mode = LPV_MODE_INACTIVE;
		break;
	case PM_EVENT_PERPARED:
		/* dsp suspend notify */
		g_lpv.mode = LPV_MODE_PENDING;
		break;
	case PM_EVENT_FINISHED:
		/* dsp resume notify */
		pm_set_wakeirq(g_lpv.irq);
		g_lpv.mode = LPV_MODE_ACTIVE;
		break;
	default:
		break;
	}

	return 0;
}

static void lpv_pm_notify_init()
{
	int ret;
	static char notify_lpv_name[] = "lpv_notify";

	g_lpv.notify.name = notify_lpv_name;
	g_lpv.notify.pm_notify_cb = lpv_pm_notify_cb,

	ret = pm_notify_register(&g_lpv.notify);
	if (ret < 0)
		printf("lpv pm notify register failed, %d\n", ret);
	g_lpv.notify_id = ret;
	return;
}

#define LPV_MODE_TIMEOUT	(10000)
static void lpv_mode_detect(void)
{
	static TickType_t _last;
	TickType_t _now;

	if (g_lpv.mode == LPV_MODE_INACTIVE) {
		if (!is_lpv_mode())
			return;
		g_lpv.mode = LPV_MODE_ACTIVE;
		_last = xTaskGetTickCount();
	} else if (g_lpv.mode == LPV_MODE_ACTIVE){
		if (!is_lpv_mode()) {
			g_lpv.mode = LPV_MODE_INACTIVE;
			return;
		}
		_now = xTaskGetTickCount();
		if (OSTICK_TO_MS(_now - _last) > LPV_MODE_TIMEOUT) {
			_last = _now;
			printf("[%s] line:%d info lpv mode timeout\n", __func__, __LINE__);
			lpv_wakeup(false);
		}
	}

	return;
}

static void lpv_service(void *arg)
{
	void *local_buf = NULL;
	void *out_buf = NULL;
	uint32_t local_buf_size = 0;
	int size = 0;

	g_lpv.sem_to_lpv = hal_sem_create(0);
	g_lpv.sem_from_lpv = hal_sem_create(0);
	if (!g_lpv.sem_to_lpv || !g_lpv.sem_from_lpv)
		goto finish;
	g_lpv.state = LPV_STATE_RUNNING;

#define LPV_AUDIO_BUF_TIME	(10)
	local_buf_size = g_lpv.rate * g_lpv.channels * g_lpv.bits / 8 \
				/ 1000 * LPV_AUDIO_BUF_TIME;
	local_buf = malloc(local_buf_size);
	if (!local_buf)
		goto finish;

	out_buf = malloc(local_buf_size / g_lpv.channels);
	if (!out_buf)
		goto finish;
#if 0
	snd_ctl_set("audiocodecadc", "bind mad function", 1);
	snd_ctl_set("audiocodecadc", "mad standby channel sel function", 2);
#endif

	g_lpv.private_data = lpv_asr_init(g_lpv.rate, g_lpv.channels, g_lpv.bits);
	if (!g_lpv.private_data)
		printf("lpv asr init failed\n");

	lpv_pm_notify_init();

	g_lpv.ar = AudioRecordCreate("capture");

	AudioRecordSetup(g_lpv.ar, g_lpv.rate, g_lpv.channels, g_lpv.bits);
	AudioRecordStart(g_lpv.ar);

	while (1) {
		lpv_mode_detect();
		size = AudioRecordRead(g_lpv.ar, local_buf, local_buf_size);
		if (size != local_buf_size) {
			printf("size(%d) != local_buf_size(%u)\n",
				size, local_buf_size);
			goto finish;
		}
		do_interleaved_convert(local_buf, g_lpv.channels, local_buf_size /
			(g_lpv.channels * g_lpv.bits / 8));
		lpv_feed(local_buf, local_buf_size, out_buf);
		if (g_lpv.state != LPV_STATE_RUNNING)
			goto finish;
	}

finish:
	if (g_lpv.ar)
		AudioRecordDestroy(g_lpv.ar);
	if (local_buf)
		free(local_buf);
	if (out_buf)
		free(out_buf);
	if (g_lpv.sem_to_lpv)
		hal_sem_delete(g_lpv.sem_to_lpv);
	if (g_lpv.sem_from_lpv)
		hal_sem_delete(g_lpv.sem_from_lpv);
	if (g_lpv.private_data)
		lpv_asr_destroy(g_lpv.private_data);
	if (g_lpv.notify_id > 0)
		pm_notify_unregister(g_lpv.notify_id);
	if (g_lpv.irq != 0)
		pm_wakesrc_unregister(g_lpv.irq);

	memset(&g_lpv, 0, sizeof(g_lpv));
	printf("[%s] exit\n", __func__);
	hal_thread_stop(NULL);
}

static int lpvoice_service_main(void)
{
	if (g_lpv.handle != NULL) {
		printf("lpv alread running...\n");
		return -1;
	}
	g_lpv.handle = hal_thread_create(lpv_service, NULL, "LP-Voice",
		1024*48, HAL_THREAD_PRIORITY_APP);

	return 0;
}

static void lpvoice_usage(void)
{
	printf("Usgae: lpv [option]\n");
	printf("-v,          lpv version\n");
	printf("-h,          lpv help\n");
	printf("-s,          lpv start\n");
	printf("-e,          lpv mode, 1-enable; 0-disable\n");
	printf("-f,          lpv fake wake, 1-wake; 0-standby\n");
	printf("\n");
}

static int cmd_lpvoice(int argc, char *argv[])
{
	int c = 0;
	int start = 0;
	int enable_lpv = -1, fake_wake = -1;

	if (!g_lpv.handle)
		memset(&g_lpv, 0, sizeof(g_lpv));

	g_lpv.rate = 16000;
	g_lpv.channels = 3;
	g_lpv.bits = 16;

	/*g_lpv.resume_with_lpv = 1;*/

	optind = 0;
	while ((c = getopt(argc, argv, "hvse:f:k")) != -1) {
		switch (c) {
		case 'v':
			printf("LP-Voice versino: %s\n", LPVOICE_VERSION);
			return 0;
		case 's':
			start = 1;
			break;
		case 'e':
			enable_lpv = atoi(optarg);
			break;
		case 'f':
			fake_wake = atoi(optarg);
			break;
		case 'k':
			g_lpv.state = LPV_STATE_STOP;
			return 0;
		case 'h':
			lpvoice_usage();
			break;
		}
	}
	if (start)
		return lpvoice_service_main();

	if (enable_lpv >= 0)
		return lpv_enable(enable_lpv);

	if (fake_wake >= 0)
		return lpv_wakeup(fake_wake);

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_lpvoice, lpv, lpvoice serivce control);
