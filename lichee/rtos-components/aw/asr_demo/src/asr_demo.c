//#define ONLINE_ASR_DEBUG

#include <stdlib.h>
#include <string.h>
#include <librws.h>
#define CONFIG_ASR_DEMO_TEST_CMD
#ifdef CONFIG_ASR_DEMO_TEST_CMD
#include <console.h>
#endif
#include <hal_thread.h>
#include <hal_sem.h>
#include <hal_mem.h>
#include <hal_time.h>
#include <hal_queue.h>
#include "asr_rws_client.h"
#include "asr_input.h"
#include "asr_result_parser.h"
#include "asr_resources.h"
#include "dsp_controller.h"
#include "ring.h"
#include "iot_controller.h"
#include "asr_demo_config.h"
#ifdef USE_PLAYER_MGR
#include "player_mgr.h"
#endif

static struct ring_debug_info_t audio_data_debug_info = {
	.port = SAVE_AUDIO_DATA_PORT,
	.file_path = SAVE_AUDIO_DATA_PATH,
};

static struct ring_debug_info_t audio_raw_data_debug_info = {
	.port = SAVE_AUDIO_RAW_DATA_PORT,
	.file_path = SAVE_AUDIO_RAW_DATA_PATH,
};

int asr_demo_send_cmd(void *_hdl, int cmd_id, unsigned long int arg);

struct asr_demo_t {
	volatile int run;
	int online;
	int no_dsp;
	int volume;

#ifdef USE_PLAYER_MGR
	void *player_mgr;
#endif
	int id;
	int response_id;
	void *iot_controller;
	void *asr_input;
	void *asr_rws;
	hal_sem_t offline_sem;
	void *dsp_controller;
	void *audio_data_ring;
#ifdef DSP_ASR_DUMP_INPUT_DATA
	void *audio_raw_data_ring;
	int save_raw_data;
#endif
	hal_queue_t cmd_queue;
	void *thread;
};

struct asr_cmd_t {
	int cmd_id;
	unsigned long int arg;
};

//dsp_controller
#ifdef DSP_ASR_DUMP_INPUT_DATA
static int dsp_controller_raw_data_send(void *priv, int vad_flag, void *data, int size)
{
	struct asr_demo_t *hdl = (struct asr_demo_t *)priv;

	if (size != (DSP_ASR_FRAME_SIZE * 4)) {
		printf("size error! %d\n", size);
		return size;
	}
	ring_send_data(hdl->audio_raw_data_ring, data, size);
	return size;
}
#endif

static int dsp_controller_data_send(void *priv, void *data, int size)
{
	struct asr_demo_t *hdl = (struct asr_demo_t *)priv;

	ring_send_data(hdl->audio_data_ring, data, size);

	return size;
}

static int dsp_controller_word_id_send(void *priv, int word_id, int cnt)
{
	struct asr_demo_t *hdl = (struct asr_demo_t *)priv;

	return asr_demo_send_cmd(hdl, word_id, (unsigned long int)cnt);
}

//asr_input
int asr_input_data_send(void *priv, void *data, int size)
{
	struct asr_demo_t *hdl = (struct asr_demo_t *)priv;

	ring_send_data(hdl->audio_data_ring, data, size);

	return size;
}

static inline void input_stop(struct asr_demo_t *hdl)
{
	if (!hdl || !hdl->asr_input)
		return;

	void *asr_input = hdl->asr_input;

	asr_input_stop(asr_input, 0);
	hdl->asr_input = NULL;
	asr_input_destroy(asr_input);
}

static inline int input_start(struct asr_demo_t *hdl)
{
	struct asr_input_config_t input_config = {
		.rate = 16000,
		.ch = 1,
		.bit = 16,
		.data_send = asr_input_data_send,
		.priv = hdl,
	};

	hdl->asr_input = asr_input_create(&input_config);
	if (!hdl->asr_input) {
		printf("asr_input_create failed!\n");
		goto err;
	}

	if(0 > asr_input_start(hdl->asr_input)) {
		printf("asr_input_start failed!\n");
		goto err;
	}

	return 0;
err:
	asr_input_destroy(hdl->asr_input);
	hdl->asr_input = NULL;
	return -1;
}

//online_asr
#define OFFLINE_THREAD_BUFFER_SIZE	(DSP_ASR_FRAME_SIZE * 4)
#define OFFLINE_THREAD_STACK_SIZE	(OFFLINE_THREAD_BUFFER_SIZE / sizeof(long) + 256)
static void offline_recv_data_thread(void *arg)
{
	struct asr_demo_t *hdl = (struct asr_demo_t *)arg;
	unsigned char data[OFFLINE_THREAD_BUFFER_SIZE];
	int ret = 0;

	while (1) {
#ifdef DSP_ASR_DUMP_INPUT_DATA
		if (hdl->audio_raw_data_ring && hdl->save_raw_data) {
			while(ret = ring_recv_data(hdl->audio_raw_data_ring, data, sizeof(data)));
		}
#endif
		if (hdl->audio_data_ring) {
			while(ret = ring_recv_data(hdl->audio_data_ring, data, sizeof(data)));
		}

		if (ret == 0 && hdl->asr_input && asr_input_is_stop(hdl->asr_input))
			goto end;
		else if (ret == 0 && hdl->dsp_controller && dsp_controller_get_eos(hdl->dsp_controller))
			goto end;

		hal_msleep(10);
	}
end:
	ring_recv_done(hdl->audio_data_ring);
#ifdef DSP_ASR_DUMP_INPUT_DATA
	ring_recv_done(hdl->audio_raw_data_ring);
#endif
	if (hdl->offline_sem) {
		if (hal_sem_post(hdl->offline_sem))
			printf("hal_sem_post failed!\n");
	}

	printf("%s exit\n", __func__);
	hal_thread_stop(NULL);
}

int online_asr_data_recv(void *priv, void *data, int size)
{
	struct asr_demo_t *hdl = (struct asr_demo_t *)priv;
	void *asr_input = hdl->asr_input;
	int ret = 0;

	//送符合规则的数据，丢弃原始数据
#ifdef DSP_ASR_DUMP_INPUT_DATA
	if (hdl->audio_raw_data_ring && hdl->save_raw_data) {
		while(ret = ring_recv_data(hdl->audio_raw_data_ring, data, size));
	}
#endif

	if (hdl->audio_data_ring)
		ret = ring_recv_data(hdl->audio_data_ring, data, size);

	if(ret < 0) {
		printf("ring_recv_data error! ret: %d\n", ret);
	}

	if (ret == 0 && asr_input && asr_input_is_stop(asr_input))
		goto end;
	else if (ret == 0 && hdl->dsp_controller && dsp_controller_get_eos(hdl->dsp_controller))
		goto end;
	else
		return ret;
end:
	ring_recv_done(hdl->audio_data_ring);
#ifdef DSP_ASR_DUMP_INPUT_DATA
	ring_recv_done(hdl->audio_raw_data_ring);
#endif
	return -1;
}

const char *online_timeout_json = "{ \"nlpResult\": { \"intent\": { \"name\": \"local\" }, \"ttsLink\": \"/data/asr/online_timeout.mp3\" }, \"status\": \"ok\" }";
const char *online_again_json = "{ \"nlpResult\": { \"intent\": { \"name\": \"local\" }, \"ttsLink\": \"/data/asr/again.mp3\" }, \"status\": \"ok\" }";
static int online_asr_result_cb(void *priv, const char *text, int length)
{
	struct asr_demo_t *hdl = (struct asr_demo_t *)priv;
	struct asr_result_t *result = NULL;

	if(!hdl ||!hdl->asr_rws) {
		return 0;
	}

	if (!text && length < 0) {
		printf("no result\n");
#ifdef ONLINE_ASR_DEBUG
		result = asr_result_create(online_again_json, asr_rws_get_id(hdl->asr_rws));
		if (!result)
			printf("asr_result_create failed!\n");
#else
		return 0;
#endif
	} else if (!text && length > 0) {
		printf("force stop, no result\n");
		return 0;
	} else if (!text || !length) {
		printf("wait online asr result timeout\n");
		result = asr_result_create(online_timeout_json, asr_rws_get_id(hdl->asr_rws));
		if (!result)
			printf("asr_result_create failed!\n");
	} else if (!strcmp(text, ".eof!")) {
		printf("eof\n");
#ifdef ONLINE_ASR_DEBUG
		result = asr_result_create(online_again_json, asr_rws_get_id(hdl->asr_rws));
		if (!result)
			printf("asr_result_create failed!\n");
#else
		return 0;
#endif
	} else {
		printf("%s: Socket text: %s\n", __func__, text);
		result = asr_result_create(text, asr_rws_get_id(hdl->asr_rws));
		if (!result) {
			printf("asr_result_create failed!\n");
#ifdef ONLINE_ASR_DEBUG
			result = asr_result_create(online_again_json, asr_rws_get_id(hdl->asr_rws));
			if (!result)
				printf("asr_result_create failed!\n");
#endif
		}
	}

	asr_demo_send_cmd(hdl, CMD_ID_ASR_RESULT, (unsigned long int)result);
	return 0;
}

static inline void rws_client_stop(struct asr_demo_t *hdl)
{
	if (!hdl || !hdl->asr_rws)
		return;

	void *asr_rws = hdl->asr_rws;
	asr_rws_client_stop(asr_rws, 0);
	if (0 > asr_rws_client_wait_stop(asr_rws, 1000))
		printf("asr_rws_client_wait_stop failed!\n");

	hdl->asr_rws = NULL;
	//asr_rws_client_destroy(asr_rws);
}

static inline int rws_client_start(struct asr_demo_t *hdl)
{
	struct asr_rws_client_config_t rws_config = {
		.get_audio_data_fun = online_asr_data_recv,
		.result_cb = online_asr_result_cb,
		.priv = hdl,
		.timeout_ms = ONLINE_ASR_TIMEOUT_MS,
		.id = hdl->id,
		.scheme = "ws",
		.host = ONLINE_ASR_HOST,
		.path = ONLINE_ASR_PATH,
		.port = ONLINE_ASR_PORT,
		.cert = ONLINE_ASR_CERT,
	};

	hdl->asr_rws = asr_rws_client_create();
	if (!hdl->asr_rws) {
		printf("create failed!\n");
		goto err;
	}

	int ret = asr_rws_client_config(hdl->asr_rws, &rws_config);
	if (ret) {
		printf("config failed!\n");
		goto err;
	}

	ret = asr_rws_client_start(hdl->asr_rws);
	if (ret) {
		printf("start failed!\n");
		asr_rws_client_stop(hdl->asr_rws, 1);
		goto err;
	}

	return 0;
err:
	asr_rws_client_destroy(hdl->asr_rws);
	hdl->asr_rws = NULL;
	return -1;
}

static void online_asr_stop(struct asr_demo_t *hdl)
{
	rws_client_stop(hdl);
	input_stop(hdl);
	if (hdl->offline_sem) {
		while (0 > hal_sem_timedwait(hdl->offline_sem, pdMS_TO_TICKS(1000))) {
			printf("wait %s timeout!\n", __func__);
		}
		hal_sem_delete(hdl->offline_sem);
		hdl->offline_sem = NULL;
	}
}

static int online_asr_start(struct asr_demo_t *hdl)
{
	if (!hdl->online) {
#if (SAVE_AUDIO_DATA_PATH || SAVE_AUDIO_DATA_PORT \
	|| SAVE_AUDIO_RAW_DATA_PATH || SAVE_AUDIO_RAW_DATA_PORT)
	//由于没有联网，不必起送数据到云端，起一个接收数据的线程以保存数据
		hdl->offline_sem = hal_sem_create(0);
		if (hdl->offline_sem)
			hal_thread_create(offline_recv_data_thread, hdl, "offline_dump", OFFLINE_THREAD_STACK_SIZE, 5);
#endif
		return 0;
	}
	printf("start online asr!\n");

	if (hdl->no_dsp) {
		if (input_start(hdl)) {
			return -1;
		}
	}

	if (rws_client_start(hdl)) {
		input_stop(hdl);
		return -1;
	}

	return 0;
}

#define PLAY_TTS		(1<<0)
#define RESUME_RES		(1<<1)
#define PAUSE_RES		(1<<2)
/* asr result cmd */
static inline int is_asr_result_cmd(int cmd_id)
{
	if (cmd_id == CMD_ID_ASR_RESULT)
		return 1;

	return 0;
}

static inline int process_asr_result_cmd(struct asr_demo_t *hdl, int cmd_id, struct asr_result_t *result, const char **url)
{
	int ret = 0;

	dsp_controller_disable_vad(hdl->dsp_controller);
	if (result && result->tts_link && result->id >= hdl->id) {
		hdl->response_id = result->id;
		printf("tts_link: %s\n", result->tts_link);
		*url = result->tts_link;
		return RESUME_RES | PLAY_TTS;
	}

	return 0;
}

static inline void process_asr_result_gc(struct asr_demo_t *hdl, int cmd_id, struct asr_result_t *result)
{
	if (result && result->tts_link && result->id >= hdl->id) {
#ifdef USE_PLAYER_MGR
	for (int i = 0; i < result->res_num; i++) {
		int flag = i ? RES_ADD_TO_PLAYLIST : RES_FORCE_PLAY;

		printf("res_list[%d]: %s\n", i, result->res_list[i]);
		player_mgr_play_res(hdl->player_mgr, result->res_list[i], flag, NULL);
	}
#endif
	} else if (result && result->tts_link && result->id < hdl->id) {
		printf("word_id cnt(%d) > result id(%d) , ignore online result\n", hdl->id, result->id);
	} else {
		printf("no result or no tts\n");
	}

	online_asr_stop(hdl);
	asr_result_destroy(result);
}
/* asr result cmd */

/* wakeup cmd */
static inline int is_wakeup_cmd(int cmd_id)
{
	if (cmd_id == CMD_ID_WAKEUP)
		return 1;

	return 0;
}

static inline int process_wakeup_cmd(struct asr_demo_t *hdl, int cmd_id, int word_id_cnt)
{
	hdl->id = word_id_cnt;
	printf("update word_id cnt: %d\n", word_id_cnt);
	dsp_controller_disable_vad(hdl->dsp_controller);
	online_asr_stop(hdl);
	dsp_controller_enable_vad(hdl->dsp_controller);

	ring_recv_drop(hdl->audio_data_ring);
#ifdef DSP_ASR_DUMP_INPUT_DATA
	ring_recv_drop(hdl->audio_raw_data_ring);
#endif
	online_asr_start(hdl);

	return PAUSE_RES | PLAY_TTS;
}
/* wakeup cmd */

/* status report cmd */
static inline int is_status_report_cmd(int cmd_id)
{
	if (cmd_id == CMD_ID_STATUS_REPORT)
		return 1;

	return 0;
}

static inline int process_status_report_cmd(struct asr_demo_t *hdl, int cmd_id, const char *url)
{
	if (url) {
#ifdef USE_PLAYER_MGR
		player_mgr_play_tts(hdl->player_mgr, url, RES_ADD_TO_PLAYLIST | RES_IMPORTANT, NULL);
#endif
	}

	return 0;
}
/* status report cmd */

/* device cmd */
static inline int is_device_cmd(int cmd_id)
{
	if (cmd_id >= CMD_ID_POWER_ON_1 && cmd_id <= CMD_ID_FAN_LOW_SPEED_2)
		return 1;

	return 0;
}

static inline int process_device_cmd(struct asr_demo_t *hdl, int cmd_id)
{
	//借用关机命令停止播放
	if (cmd_id >= CMD_ID_POWER_OFF_1 && cmd_id <= CMD_ID_POWER_OFF_3) {
#ifdef USE_PLAYER_MGR
		player_mgr_play_res(hdl->player_mgr, NULL, RES_FORCE_PLAY, NULL);
#endif
	}
	// TODO

	return RESUME_RES | PLAY_TTS;
}
/* device cmd */

/* volume cmd */
extern int snd_ctl_set(const char *name, const char *elem, unsigned int val);
#define MIN_VOLUME	(0)
#define MAX_VOLUME	(6)

static inline int is_volume_cmd(int cmd_id)
{
	if (cmd_id >= CMD_ID_MAX_VOLUME && cmd_id <= CMD_ID_LOWER_VOLUME_2)
		return 1;

	return 0;
}

static inline int process_volume_cmd(struct asr_demo_t *hdl, int cmd_id, const char **url)
{
	if (cmd_id == CMD_ID_HIGHER_VOLUME_1 || cmd_id == CMD_ID_HIGHER_VOLUME_2) {
		if (hdl->volume < MAX_VOLUME)
			hdl->volume++;
		else
			*url = get_tts_from_cmd_id(CMD_ID_MAX_VOLUME);
	} else if (cmd_id == CMD_ID_LOWER_VOLUME_1 || cmd_id == CMD_ID_LOWER_VOLUME_2) {
		if (hdl->volume > MIN_VOLUME)
			hdl->volume--;
		else
			*url = get_tts_from_cmd_id(CMD_ID_MIN_VOLUME);
	} else if (cmd_id == CMD_ID_MAX_VOLUME) {
		hdl->volume = MAX_VOLUME;
	} else if (cmd_id == CMD_ID_MIN_VOLUME) {
		hdl->volume = MIN_VOLUME;
	}
	snd_ctl_set("audiocodecdac", "LINEOUT volume", hdl->volume);
	printf("volume: %u (range: %u-%u)\n", hdl->volume, MIN_VOLUME, MAX_VOLUME);

	return RESUME_RES | PLAY_TTS;
}
/* volume cmd */

/* offline cmd */
static inline int is_offline_cmd(int cmd_id)
{
	if (cmd_id >= 0)
		return 1;

	return 0;
}

static inline int process_offline_cmd(struct asr_demo_t *hdl, int cmd_id, int word_id_cnt, const char **url)
{
	dsp_controller_disable_vad(hdl->dsp_controller);

	hdl->id = word_id_cnt;
	printf("update word_id cnt: %d\n", word_id_cnt);
	if (cmd_id == CMD_ID_TIMEOUT && (word_id_cnt - 1) == hdl->response_id) {
		printf("word_id cnt(%d) + 1 = response_id id(%d) , ignore offline timeout\n", word_id_cnt, hdl->response_id);
		return 0;
	}

	if (is_device_cmd(cmd_id)) {
		return process_device_cmd(hdl, cmd_id);
	} else if (is_volume_cmd(cmd_id)) {
		return process_volume_cmd(hdl, cmd_id, url);
	} else {
		return RESUME_RES | PLAY_TTS;
	}
}
/* offline cmd */

/*
cmd type:
1. status report cmd
2. online cmd
3. offline cmd
	3.1 wakeup cmd
	3.2 device cmd
	3.3 volume cmd
	3.4 other
*/

//asr_demo
static inline int asr_demo_do_cmd(struct asr_demo_t *hdl, struct asr_cmd_t *cmd)
{
	int cmd_id = cmd->cmd_id;
	int action = 0;
	const char *url = get_tts_from_cmd_id(cmd_id);

	//process cmd
	if (is_status_report_cmd(cmd_id)) {
		action = process_status_report_cmd(hdl, cmd_id, (const char *)cmd->arg);
	} else if (is_wakeup_cmd(cmd_id)) {
		action = process_wakeup_cmd(hdl, cmd_id, (int)cmd->arg);
	} else if (is_asr_result_cmd(cmd_id)) {
		action = process_asr_result_cmd(hdl, cmd_id, (struct asr_result_t *)cmd->arg, &url);
	} else if (is_offline_cmd(cmd_id) && !is_wakeup_cmd(cmd_id)) {
		action = process_offline_cmd(hdl, cmd_id, (int)cmd->arg, &url);
	}else {
		printf("unknown cmd_id: %d %lx\n", cmd->cmd_id, cmd->arg);
	}

	//tts response
#ifdef USE_PLAYER_MGR
	if (action & PAUSE_RES)
		player_mgr_disable_auto_resume_res_player(hdl->player_mgr);
	if (action & RESUME_RES)
		player_mgr_enable_auto_resume_res_player(hdl->player_mgr);
	if (action & PLAY_TTS)
		player_mgr_play_tts(hdl->player_mgr, url, RES_FORCE_PLAY, NULL);
#endif

	//gc
	if (is_asr_result_cmd(cmd_id)) {
		process_asr_result_gc(hdl, cmd_id, (struct asr_result_t *)cmd->arg);
	}

	return 0;
}

static void asr_demo_thread(void *arg)
{
	struct asr_demo_t *hdl = (struct asr_demo_t *)arg;
	struct asr_cmd_t cmd;

	while(hdl->run) {
		if (0 > hal_queue_recv(hdl->cmd_queue, &cmd, 200))
			continue;
		asr_demo_do_cmd(hdl, &cmd);
	}

	hal_thread_stop(NULL);
}

int asr_demo_set_online(void *_hdl, int val);
static void iot_event_cb(void *priv, enum iot_event_t event)
{
	printf("priv: %lx, event: %d\n", (unsigned long)priv, (int)event);

	struct asr_demo_t *hdl = (struct asr_demo_t *)priv;
	const char *url = NULL;

	switch(event) {

	case IOT_EVENT_WAIT_WIFI_CONFIG:
		url = "/data/asr/wait_wifi_config.mp3";
		goto send_cmd;
    case IOT_EVENT_GET_WIFI_CONFIG:
		url = "/data/asr/get_wifi_config.mp3";
		goto send_cmd;
	case IOT_EVENT_NET_CONNECT:
		asr_demo_set_online(hdl, 1);
		url = "/data/asr/net_connect.mp3";
		goto send_cmd;
	case IOT_EVENT_NET_DISCONNECT:
		asr_demo_set_online(hdl, 0);
		url = "/data/asr/net_disconnect.mp3";
		goto send_cmd;
	case IOT_EVENT_ERROR:
	case IOT_EVENT_NONE:
	default:
		break;
	}
	return;
send_cmd:
	asr_demo_send_cmd(hdl, CMD_ID_STATUS_REPORT, (unsigned long int)url);
}

int asr_demo_set_save_raw_data(void *_hdl, int val)
{
	struct asr_demo_t *hdl = (struct asr_demo_t *)_hdl;

	hdl->save_raw_data = val ? 1 : 0;
}

int asr_demo_set_reset(void *_hdl)
{
	struct asr_demo_t *hdl = (struct asr_demo_t *)_hdl;

	return iot_controller_reset_config(hdl->iot_controller);
}

int asr_demo_set_online(void *_hdl, int val)
{
	struct asr_demo_t *hdl = (struct asr_demo_t *)_hdl;

	hdl->online = val ? 1 : 0;
}

int asr_demo_set_no_dsp(void *_hdl, int val)
{
	struct asr_demo_t *hdl = (struct asr_demo_t *)_hdl;

	hdl->no_dsp = val ? 1 : 0;
}

int asr_demo_send_cmd(void *_hdl, int cmd_id, unsigned long int arg)
{
	struct asr_demo_t *hdl = (struct asr_demo_t *)_hdl;
	struct asr_cmd_t cmd = {
		.cmd_id = cmd_id,
		.arg = arg,
	};

	printf("send_cmd: %d, arg: %lx\n", cmd_id, arg);
	if (0 > hal_queue_send_wait(hdl->cmd_queue, &cmd, 200)) {
		printf("cmd send failed!\n");
		return -1;
	}

	return 0;
}

void asr_demo_destroy(void *_hdl)
{
	struct asr_demo_t *hdl = (struct asr_demo_t *)_hdl;

	if (hdl) {
		if (hdl->iot_controller) {
			iot_controller_stop(hdl->iot_controller);
			iot_controller_destroy(hdl->iot_controller);
			hdl->iot_controller = NULL;
		}

		if (hdl->audio_data_ring) {
			ring_destroy(hdl->audio_data_ring);
			hdl->audio_data_ring = NULL;
		}

#ifdef DSP_ASR_DUMP_INPUT_DATA
		if (hdl->audio_raw_data_ring) {
			ring_destroy(hdl->audio_raw_data_ring);
			hdl->audio_raw_data_ring = NULL;
		}
#endif

		if (hdl->dsp_controller) {
			dsp_controller_destroy(hdl->dsp_controller);
			hdl->dsp_controller = NULL;
		}
#ifdef USE_PLAYER_MGR
		if (hdl->player_mgr) {
			player_mgr_destroy(hdl->player_mgr);
			hdl->player_mgr = NULL;
		}
#endif
		if (hdl->cmd_queue) {
			hal_queue_delete(hdl->cmd_queue);
			hdl->cmd_queue = NULL;
		}
		hal_free(hdl);
	}
}

void *asr_demo_create(void)
{
	struct asr_demo_t *hdl = hal_malloc(sizeof(*hdl));
	struct dsp_controller_config_t dspc_config = {
#ifdef DSP_ASR_DUMP_INPUT_DATA
		.audio_raw_data_send = dsp_controller_raw_data_send,
#else
		.audio_raw_data_send = NULL,
#endif
		.audio_data_send = dsp_controller_data_send,
		.word_id_send = dsp_controller_word_id_send,
		.vad_bos_buffer_len = DSP_ASR_VAD_BUF_LEN,
		.priv = hdl,
		.timeout_ms = DSP_ASR_TIMEOUT_MS,
	};
	struct iot_controller_config_t iotc_config = {
		.wifi_config_path = SOFTAP_AP_CONFIG_PATH,
		.ssid = NULL,
		.psk = NULL,
		.ap_ssid = SOFTAP_AP_SSID,
		.ap_psk = SOFTAP_AP_PSK,
		.cb = iot_event_cb,
		.priv = hdl,
	};

	if (!hdl) {
		printf("no memory!\n");
		goto err;
	}
	memset(hdl, 0, sizeof(*hdl));

#ifdef USE_PLAYER_MGR
	hdl->player_mgr = player_mgr_create();
	if(!hdl->player_mgr) {
		printf("player_mgr_create failed!\n");
		goto err;
	}
#endif

	hdl->cmd_queue = hal_queue_create("asr_cmd", sizeof(struct asr_cmd_t), 6);
	if (!hdl->cmd_queue) {
		printf("no memory!\n");
		goto err;
	}

	hdl->dsp_controller = dsp_controller_create(&dspc_config);
	if (!hdl->dsp_controller) {
		printf("dsp_controller_create failed!\n");
		goto err;
	}

	hdl->audio_data_ring = ring_create(AUDIO_DATA_RING_SIZE, "audio_data", &audio_data_debug_info);
	if (!hdl->audio_data_ring) {
		printf("ring_create failed!\n");
		goto err;
	}

#ifdef DSP_ASR_DUMP_INPUT_DATA
	hdl->audio_raw_data_ring = ring_create(AUDIO_RAW_DATA_RING_SIZE, "audio_raw_data", &audio_raw_data_debug_info);
	if (!hdl->audio_raw_data_ring) {
		printf("ring_create failed!\n");
		goto err;
	}
#endif

	hdl->volume = 3;

	hdl->iot_controller = iot_controller_create(&iotc_config);
	if (!hdl->iot_controller) {
		printf("iot_controller_create failed!\n");
		goto err;
	}

	asr_demo_send_cmd(hdl, CMD_ID_STATUS_REPORT, (unsigned long int)welcome_url);

	iot_controller_start(hdl->iot_controller);

	return hdl;
err:
	asr_demo_destroy(hdl);
	return NULL;
}

int asr_demo_start(void *_hdl)
{
	struct asr_demo_t *hdl = (struct asr_demo_t *)_hdl;

	snd_ctl_set("audiocodecdac", "LINEOUT volume", hdl->volume);

	if (!hdl->no_dsp) {
		dsp_controller_start(hdl->dsp_controller);
	}

	hdl->run = 1;
	hdl->thread = hal_thread_create(asr_demo_thread, hdl, "asr_demo", 1024, 5);
	if (!hdl->thread) {
		printf("hal_thread_create failed!\n");
	}
	hal_thread_start(hdl->thread);

	return 0;
err:
	return -1;
}

int asr_demo_stop(void *_hdl, int force)
{
	struct asr_demo_t *hdl = (struct asr_demo_t *)_hdl;

	hdl->run = 0;
	if (!hdl->no_dsp) {
		dsp_controller_stop(hdl->dsp_controller);
	}

	return 0;
}

#ifdef CONFIG_ASR_DEMO_TEST_CMD
static void *asr_demo_hdl = NULL;
int cmd_asr_demo(int argc, char ** argv)
{
	if (argc >= 2 && !strcmp(argv[1], "start")) {
		printf("start asr demo!\n");
		asr_demo_hdl = asr_demo_create();
		if (!asr_demo_hdl) {
			printf("no memory!\n");
			goto err;
		}

		if (argc == 3 && !strcmp(argv[2], "no_dsp"))
			asr_demo_set_no_dsp(asr_demo_hdl, 1);

		if (0 > asr_demo_start(asr_demo_hdl)) {
			printf("asr_demo_start failed!\n");
			goto err;
		}
	} else if (argc == 2 && !strcmp(argv[1], "stop") && asr_demo_hdl){
		printf("stop asr demo!\n");
		asr_demo_stop(asr_demo_hdl, 0);
		hal_msleep(1 * 1000);
		asr_demo_destroy(asr_demo_hdl);
		asr_demo_hdl = NULL;
	} else if (argc == 3 && !strcmp(argv[1], "cmd")){
		int cmd_id = atoi(argv[2]);
		asr_demo_send_cmd(asr_demo_hdl, cmd_id, 0lu);
	} else if (argc == 2 && !strcmp(argv[1], "online") && asr_demo_hdl){
		asr_demo_set_online(asr_demo_hdl, 1);
		printf("asr_demo set online\n");
	} else if (argc == 2 && !strcmp(argv[1], "offline") && asr_demo_hdl){
		asr_demo_set_online(asr_demo_hdl, 0);
		printf("asr_demo set offline\n");
	} else if (argc == 2 && !strcmp(argv[1], "reset") && asr_demo_hdl){
		asr_demo_set_reset(asr_demo_hdl);
		printf("asr_demo set reset\n");
	} else if (argc == 2 && !strcmp(argv[1], "debug") && asr_demo_hdl){
		asr_demo_set_save_raw_data(asr_demo_hdl, 1);
		printf("asr_demo set debug\n");
	} else if (argc == 2 && !strcmp(argv[1], "normal") && asr_demo_hdl){
		asr_demo_set_save_raw_data(asr_demo_hdl, 0);
		printf("asr_demo set normal\n");
	}

	return 0;
err:
	asr_demo_destroy(asr_demo_hdl);
	asr_demo_hdl = NULL;
	return -1;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_asr_demo, asr_demo, asr demo);
#endif
