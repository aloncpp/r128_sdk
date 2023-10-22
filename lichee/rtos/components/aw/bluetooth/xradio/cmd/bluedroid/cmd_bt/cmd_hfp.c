/*
 * Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of XRADIO TECHNOLOGY CO., LTD. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "cmd_util.h"

#include "xr_hf_client_api.h"
#include "bt_utils.h"
#include "bt_app_audio.h"

#include "unlock_buffer.h"
#include <aw-alsa-lib/control.h>


#define CMD_HFP_TEST_ALSA 1

static enum cmd_status cmd_hfp_help_exec(char *cmd);

#ifndef CMD_HFP_TEST_ALSA
static fifo_t fifo;
static char fifo_buffer[2048];
static uint8_t out_data[1024];
static uint32_t bt_app_hf_client_outgoing_cb(uint8_t *p_buf, uint32_t sz)
{
	if(fifo_getlen(&fifo) >= sz) {
		fifo_pop(&fifo, out_data, sz);
		memcpy(p_buf, out_data, sz);
		return sz;
	}
	return 0;
}

static void bt_app_hf_client_incoming_cb(const uint8_t *buf, uint32_t sz)
{
	fifo_push(&fifo, buf, sz);
	xr_hf_client_outgoing_data_ready();
}

#else

static uint32_t bt_app_hf_client_outgoing_cb(uint8_t *p_buf, uint32_t sz)
{
	if (bt_app_audio_read_unblock(p_buf, sz) == sz) {
		return sz;
	}
	return 0;
}

static void bt_app_hf_client_incoming_cb(const uint8_t *buf, uint32_t sz)
{
	bt_app_audio_write_unblock(buf, sz, BT_APP_AUDIO_TYPE_HFP);
	xr_hf_client_outgoing_data_ready();
}
#endif

#ifdef XRADIO_HFP_ADJUST_VOLUME
#define XRADIO_AMIXER_HFP_MAX 63
static uint8_t bluedroid_amixer(uint8_t volume)
{
	snd_ctl_info_t info;
	char g_card_name[20] = "audiocodec";
	if (volume >= 0) {
#ifdef CONFIG_DRIVERS_SOUND
		if (snd_ctl_get_bynum(g_card_name, 0, &info) != 0) {
			printf("snd_ctl_get failed\n");
			return XR_FAIL;
		}
		snd_ctl_set_bynum(g_card_name, 0, volume);
#endif
	}
	return XR_OK;
}
#endif

/* callback for hfp */
static void bt_app_hf_cb(xr_hf_client_cb_event_t event, xr_hf_client_cb_param_t *param)
{
	CMD_DBG("hf event %d\n", event);

	switch (event) {
	case XR_HF_CLIENT_CONNECTION_STATE_EVT:
		CMD_DBG("XR_HF_CLIENT_CONNECTION_STATE_EVT: con-state(%d), peer(%d), chld(%d), "XR_BD_ADDR_STR"\n",
			param->conn_stat.state, param->conn_stat.peer_feat, param->conn_stat.chld_feat, XR_BD_ADDR_HEX(param->conn_stat.remote_bda));
		cmd_write_event(CMD_EVENT_HFP_NOTIFY, "evt(%d):connection(%d)", XR_HF_CLIENT_CONNECTION_STATE_EVT, param->conn_stat.state);
		break;
	case XR_HF_CLIENT_AUDIO_STATE_EVT:
		CMD_DBG("XR_HF_CLIENT_AUDIO_STATE_EVT: audio-state(%d), "XR_BD_ADDR_STR"\n", param->audio_stat.state, XR_BD_ADDR_HEX(param->audio_stat.remote_bda));
		//cmd_write_event(CMD_EVENT_HFP_NOTIFY, "evt(%d):audio(%d)", XR_HF_CLIENT_AUDIO_STATE_EVT, param->audio_stat.state);
		if (param->audio_stat.state == XR_HF_CLIENT_AUDIO_STATE_CONNECTED ||
			param->audio_stat.state == XR_HF_CLIENT_AUDIO_STATE_CONNECTED_MSBC) {
#ifdef CMD_HFP_TEST_ALSA
			bt_app_audio_config(8000, 1, BT_APP_AUDIO_TYPE_HFP);
			sys_handler_send(bt_app_audio_ctrl, BT_APP_AUDIO_EVENTS_HFP_START, 1);
#endif
			int ret = xr_hf_client_register_data_callback(bt_app_hf_client_incoming_cb,
												bt_app_hf_client_outgoing_cb);
		} else if (param->audio_stat.state == 0) {
#ifdef CMD_HFP_TEST_ALSA
			sys_handler_send(bt_app_audio_ctrl, BT_APP_AUDIO_EVENTS_HFP_STOP, 1);
#endif
		}
		break;
	case XR_HF_CLIENT_BVRA_EVT:
		CMD_DBG("XR_HF_CLIENT_BVRA_EVT: recog-state(%d)\n", param->bvra.value);
		cmd_write_event(CMD_EVENT_HFP_NOTIFY, "evt(%d):recognition(%d)", XR_HF_CLIENT_BVRA_EVT, param->bvra.value);
		break;
	case XR_HF_CLIENT_CIND_CALL_EVT:
		CMD_DBG("XR_HF_CLIENT_CIND_CALL_EVT: call-state(%d)\n", param->call.status);
		cmd_write_event(CMD_EVENT_HFP_NOTIFY, "evt(%d):call(%d)", XR_HF_CLIENT_CIND_CALL_EVT, param->call.status);
		break;
	case XR_HF_CLIENT_CIND_CALL_SETUP_EVT:
		CMD_DBG("XR_HF_CLIENT_CIND_CALL_SETUP_EVT: call-setup-state(%d)\n", param->call_setup.status);
		if (param->call_setup.status)
			cmd_write_event(CMD_EVENT_HFP_NOTIFY, "evt(%d):call(2)", XR_HF_CLIENT_CIND_CALL_EVT);
		break;
	case XR_HF_CLIENT_CIND_CALL_HELD_EVT:
		CMD_DBG("XR_HF_CLIENT_CIND_CALL_HELD_EVT: held-state(%d)\n", param->call_held.status);
		cmd_write_event(CMD_EVENT_HFP_NOTIFY, "evt(%d):held(%d)", XR_HF_CLIENT_CIND_CALL_HELD_EVT, param->call_held.status);
		break;
	case XR_HF_CLIENT_CIND_SERVICE_AVAILABILITY_EVT:
		CMD_DBG("XR_HF_CLIENT_CIND_SERVICE_AVAILABILITY_EVT: service-avail(%d)\n", param->service_availability.status);
		break;
	case XR_HF_CLIENT_CIND_SIGNAL_STRENGTH_EVT:
		CMD_DBG("XR_HF_CLIENT_CIND_SIGNAL_STRENGTH_EVT: signal_strength(%d)\n", param->signal_strength.value);
		break;
	case XR_HF_CLIENT_CIND_ROAMING_STATUS_EVT:
		CMD_DBG("XR_HF_CLIENT_CIND_ROAMING_STATUS_EVT: roaming(%d)\n", param->roaming.status);
		break;
	case XR_HF_CLIENT_CIND_BATTERY_LEVEL_EVT:
		CMD_DBG("XR_HF_CLIENT_CIND_BATTERY_LEVEL_EVT: battery_level(%d)\n", param->battery_level.value);
		break;
	case XR_HF_CLIENT_COPS_CURRENT_OPERATOR_EVT:
		CMD_DBG("XR_HF_CLIENT_COPS_CURRENT_OPERATOR_EVT: cops(%s)\n", param->cops.name);
		break;
	case XR_HF_CLIENT_BTRH_EVT:
		CMD_DBG("XR_HF_CLIENT_BTRH_EVT: btrh(%d)\n", param->btrh.status);
		break;
	case XR_HF_CLIENT_CLIP_EVT:
		CMD_DBG("XR_HF_CLIENT_CLIP_EVT: number(%s)\n", param->clip.number);
		break;
	case XR_HF_CLIENT_CCWA_EVT:
		CMD_DBG("XR_HF_CLIENT_CCWA_EVT: number(%s)\n", param->ccwa.number);
		break;
	case XR_HF_CLIENT_CLCC_EVT:
		CMD_DBG("XR_HF_CLIENT_CLCC_EVT: idx(%d), dir(%d), status(%d), mpty(%d), number(%s)\n",
				 param->clcc.idx, param->clcc.dir, param->clcc.status, param->clcc.mpty, param->clcc.number);
		break;
	case XR_HF_CLIENT_VOLUME_CONTROL_EVT:
		CMD_DBG("XR_HF_CLIENT_VOLUME_CONTROL_EVT: target(%d), vol(%d)\n",
				 param->volume_control.type, param->volume_control.volume);
		cmd_write_event(CMD_EVENT_HFP_NOTIFY, "evt(%d):volume(%d)", XR_HF_CLIENT_VOLUME_CONTROL_EVT, param->volume_control.volume);
#ifdef XRADIO_HFP_ADJUST_VOLUME
		int ret = -1;
		int vol = param->volume_control.volume;
		CMD_DBG("adjusting the volume by ourselves\n");
		vol = XRADIO_AMIXER_HFP_MAX - (uint32_t)vol * 4;
		printf("volume is %d\n", vol);
		ret = bluedroid_amixer(vol);
		if (ret != XR_OK) {
			CMD_ERR("Set vol by ourself failed\n");
		}
#endif
		break;
	case XR_HF_CLIENT_AT_RESPONSE_EVT:
		CMD_DBG("XR_HF_CLIENT_AT_RESPONSE_EVT: code(%d), cme(%d)\n",
				 param->at_response.code, param->at_response.cme);
		cmd_write_event(CMD_EVENT_HFP_NOTIFY, "evt(%d):resp(%d),cme(%d)", XR_HF_CLIENT_AT_RESPONSE_EVT, param->at_response.code, param->at_response.cme);
		break;
	case XR_HF_CLIENT_CNUM_EVT:
		CMD_DBG("XR_HF_CLIENT_CNUM_EVT: number(%s), type(%d)\n", param->cnum.number, param->cnum.type);
		cmd_write_event(CMD_EVENT_HFP_NOTIFY, "evt(%d):cnum(%s)", XR_HF_CLIENT_CNUM_EVT, param->cnum.number);
		break;
	case XR_HF_CLIENT_BSIR_EVT:
		CMD_DBG("XR_HF_CLIENT_BSIR_EVT: bsir-provide(%d)\n", param->bsir.state);
		break;
	case XR_HF_CLIENT_BINP_EVT:
		CMD_DBG("XR_HF_CLIENT_BINP_EVT: number(%s)\n", param->binp.number);
		break;
	case XR_HF_CLIENT_RING_IND_EVT:
		CMD_DBG("XR_HF_CLIENT_RING_IND_EVT: \n");
		break;
	default:
		CMD_ERR("Invalid hfp event: %d\n", event);
		break;
	}
}


/* $hfp init */
static enum cmd_status cmd_hfp_init_exec(char *cmd)
{
	xr_err_t ret;
	if ((ret = xr_hf_client_register_callback(bt_app_hf_cb)) != XR_OK) {
		CMD_ERR("return failed: %d\n", ret);
		return CMD_STATUS_FAIL;
	}

	if ((ret = xr_hf_client_init()) != XR_OK) {
		CMD_ERR("return failed: %d\n", ret);
		return CMD_STATUS_FAIL;
	}
	return CMD_STATUS_OK;
}

/* $hfp deinit */
static enum cmd_status cmd_hfp_deinit_exec(char *cmd)
{
	xr_err_t ret;
	if ((ret = xr_hf_client_deinit()) != XR_OK) {
		CMD_ERR("return failed: %d\n", ret);
		return CMD_STATUS_FAIL;
	}
	return CMD_STATUS_OK;
}

/* $hfp connect 31:AF:00:CC:BB:AA */
static enum cmd_status cmd_hfp_connect_exec(char *cmd)
{
	xr_err_t ret;
	xr_bd_addr_t remote_bda = { 0 };
	int argc;
	char *argv[1];

	argc = cmd_parse_argv(cmd, argv, 1);
	if (argc != 1) {
		CMD_ERR("invalid param number %d\n", argc);
		return CMD_STATUS_INVALID_ARG;
	}

	str2bda(argv[0], remote_bda);
	bda2str_dump("hfp connect", remote_bda);

	if ((ret = xr_hf_client_connect(remote_bda)) != XR_OK) {
		CMD_ERR("return failed: %d\n", ret);
		return CMD_STATUS_FAIL;
	}
	return CMD_STATUS_OK;
}


/* $hfp disconnect <addr> */
static enum cmd_status cmd_hfp_disconnect_exec(char *cmd)
{
	xr_err_t ret;
	xr_bd_addr_t remote_bda = { 0 };
	int argc;
	char *argv[1];

	argc = cmd_parse_argv(cmd, argv, 1);
	if (argc != 1) {
		CMD_ERR("invalid param number %d\n", argc);
		return CMD_STATUS_INVALID_ARG;
	}

	str2bda(argv[0], remote_bda);
	bda2str_dump("sink_connect", remote_bda);


	if ((ret = xr_hf_client_disconnect(remote_bda)) != XR_OK) {
		CMD_ERR("return failed: %d\n", ret);
		return CMD_STATUS_FAIL;
	}
	return CMD_STATUS_OK;
}

/* $hfp audio <conn/disconn> 31:AF:00:CC:BB:AA */
static enum cmd_status cmd_hfp_audio_exec(char *cmd)
{
	xr_err_t ret;
	xr_bd_addr_t remote_bda = { 0 };
	int argc;
	char *argv[2];

	argc = cmd_parse_argv(cmd, argv, 2);
	if (argc != 2) {
		CMD_ERR("invalid param number %d\n", argc);
		return CMD_STATUS_INVALID_ARG;
	}

	str2bda(argv[1], remote_bda);
	bda2str_dump("sink_connect", remote_bda);

	if (!cmd_strcmp(argv[0], "conn")) {
		if ((ret = xr_hf_client_connect_audio(remote_bda)) != XR_OK)
		{
			CMD_ERR("return failed: %d\n", ret);
			return CMD_STATUS_FAIL;
		}
	} else if (!cmd_strcmp(argv[0], "disconn")) {
		if ((ret = xr_hf_client_disconnect_audio(remote_bda)) != XR_OK) {
			CMD_ERR("return failed: %d\n", ret);
			return CMD_STATUS_FAIL;
		}
	} else {
		CMD_ERR("invalid param %s\n", argv[0]);
		return CMD_STATUS_INVALID_ARG;
	}

	return CMD_STATUS_OK;
}

/* $hfp voice_rec <1/0> */
static enum cmd_status cmd_hfp_voice_recognition_exec(char *cmd)
{
	xr_err_t ret;
	int start = 0;

	/* get param */
	int cnt = cmd_sscanf(cmd, "%d ", &start);
	if (cnt != 1) {
		CMD_ERR("invalid param number %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	if (start == 1) {
		if ((ret = xr_hf_client_start_voice_recognition()) != XR_OK) {
			CMD_ERR("return failed: %d\n", ret);
			return CMD_STATUS_FAIL;
		}
	} else if (start == 0) {
		if ((ret = xr_hf_client_stop_voice_recognition()) != XR_OK) {
			CMD_ERR("return failed: %d\n", ret);
			return CMD_STATUS_FAIL;
		}
	} else {
		CMD_ERR("invalid param %d\n", start);
		return CMD_STATUS_INVALID_ARG;
	}

	return CMD_STATUS_OK;
}

/* $hfp vol_update <spk/mic> <vol> */
static enum cmd_status cmd_hfp_vol_update_exec(char *cmd)
{
	xr_err_t ret;
	char devc[10] = {0};
	int vol;
	xr_hf_volume_control_target_t dev;

	/* get param */
	int cnt = cmd_sscanf(cmd, "%s %d", devc, &vol);
	if (cnt != 2) {
		CMD_ERR("invalid param number %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	if (!cmd_strcmp(devc, "spk"))
		dev = XR_HF_VOLUME_CONTROL_TARGET_SPK;
	else if (!cmd_strcmp(devc, "mic"))
		dev = XR_HF_VOLUME_CONTROL_TARGET_MIC;
	else {
		CMD_ERR("invalid param %s\n", devc);
		return CMD_STATUS_INVALID_ARG;
	}

	if ((ret = xr_hf_client_volume_update(dev, vol)) != XR_OK) {
		CMD_ERR("return failed: %d\n", ret);
		return CMD_STATUS_FAIL;
	}
#ifdef XRADIO_HFP_ADJUST_VOLUME
	if (dev == XR_HF_VOLUME_CONTROL_TARGET_SPK) {
		int ret = -1;
		CMD_DBG("adjusting the volume by ourselves\n");
		vol = XRADIO_AMIXER_HFP_MAX - (uint32_t)vol * 4;
		printf("volume is %d\n", vol);
		ret = bluedroid_amixer(vol);
		if (ret != XR_OK) {
			CMD_ERR("Set vol by ourself failed\n");
		}
	}
#endif

	return CMD_STATUS_OK;
}

/* $hfp dial 13722240761 */
static enum cmd_status cmd_hfp_dial_exec(char *cmd)
{
	xr_err_t ret;
	char number[30] = {0};

	/* get param */
	int cnt = cmd_sscanf(cmd, "%s", number);
	if (cnt != 1) {
		CMD_ERR("invalid param number %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	if ((ret = xr_hf_client_dial(number)) != XR_OK) {
		CMD_ERR("return failed: %d\n", ret);
		return CMD_STATUS_FAIL;
	}

	return CMD_STATUS_OK;
}

/* $hfp dial_mem 2 */
static enum cmd_status cmd_hfp_dial_mem_exec(char *cmd)
{
	xr_err_t ret;
	int loc;

	/* get param */
	int cnt = cmd_sscanf(cmd, "%d", &loc);
	if (cnt != 1) {
		CMD_ERR("invalid param number %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	if ((ret = xr_hf_client_dial_memory(loc)) != XR_OK) {
		CMD_ERR("return failed: %d\n", ret);
		return CMD_STATUS_FAIL;
	}

	return CMD_STATUS_OK;
}

/* $hfp chld 0 0 */
static enum cmd_status cmd_hfp_chld_exec(char *cmd)
{
	xr_err_t ret;
	int chld;
	int idx = 0;

	/* get param */
	int cnt = cmd_sscanf(cmd, "%d %d", &chld, &idx);
	if (cnt != 2 && (chld == XR_HF_CHLD_TYPE_REL_X || chld == XR_HF_CHLD_TYPE_PRIV_X)) {
		CMD_ERR("invalid param number %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	if ((ret = xr_hf_client_send_chld_cmd((xr_hf_chld_type_t)chld, idx)) != XR_OK) {
		CMD_ERR("return failed: %d\n", ret);
		return CMD_STATUS_FAIL;
	}

	return CMD_STATUS_OK;
}

/* $hfp btrh <hold/accept/reject> */
static enum cmd_status cmd_hfp_btrh_exec(char *cmd)
{
	xr_err_t ret;
	char btrhcmd[10] = {0};
	xr_hf_btrh_cmd_t btrh;

	/* get param */
	int cnt = cmd_sscanf(cmd, "%s", btrhcmd);
	if (cnt != 1) {
		CMD_ERR("invalid param number %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	if (!cmd_strcmp(btrhcmd, "hold"))
		btrh = XR_HF_BTRH_CMD_HOLD;
	else if (!cmd_strcmp(btrhcmd, "accept"))
		btrh = XR_HF_BTRH_CMD_ACCEPT;
	else if (!cmd_strcmp(btrhcmd, "reject"))
		btrh = XR_HF_BTRH_CMD_REJECT;
	else {
		CMD_ERR("invalid param %s\n", btrhcmd);
		return CMD_STATUS_INVALID_ARG;
	}

	if ((ret = xr_hf_client_send_btrh_cmd(btrh)) != XR_OK) {
		CMD_ERR("return failed: %d\n", ret);
		return CMD_STATUS_FAIL;
	}
	return CMD_STATUS_OK;
}

/* $hfp answer */
static enum cmd_status cmd_hfp_answer_exec(char *cmd)
{
	xr_err_t ret;
	if ((ret = xr_hf_client_answer_call()) != XR_OK) {
		CMD_ERR("return failed: %d\n", ret);
		return CMD_STATUS_FAIL;
	}
	return CMD_STATUS_OK;
}

/* $hfp reject */
static enum cmd_status cmd_hfp_reject_exec(char *cmd)
{
	xr_err_t ret;
	if ((ret = xr_hf_client_reject_call()) != XR_OK) {
		CMD_ERR("return failed: %d\n", ret);
		return CMD_STATUS_FAIL;
	}
	return CMD_STATUS_OK;
}

/* $hfp query <call/name> */
static enum cmd_status cmd_hfp_query_exec(char *cmd)
{
	xr_err_t ret;
	char query[10] = {0};

	/* get param */
	int cnt = cmd_sscanf(cmd, "%s", query);
	if (cnt != 1) {
		CMD_ERR("invalid param number %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	if (!cmd_strcmp(query, "call")) {
		if ((ret = xr_hf_client_query_current_calls()) != XR_OK) {
			CMD_ERR("return failed: %d\n", ret);
			return CMD_STATUS_FAIL;
		}
	} else if (!cmd_strcmp(query, "name")) {
		if ((ret = xr_hf_client_query_current_operator_name()) != XR_OK) {
			CMD_ERR("return failed: %d\n", ret);
			return CMD_STATUS_FAIL;
		}
	} else{
		CMD_ERR("invalid param %s\n", query);
		return CMD_STATUS_INVALID_ARG;
	}

	return CMD_STATUS_OK;
}

/* $hfp subinfo */
static enum cmd_status cmd_hfp_subinfo_exec(char *cmd)
{
	xr_err_t ret;
	if ((ret = xr_hf_client_retrieve_subscriber_info()) != XR_OK) {
		CMD_ERR("return failed: %d\n", ret);
		return CMD_STATUS_FAIL;
	}
	return CMD_STATUS_OK;
}

/* $hfp dtmf <0-9/#/ * > */
static enum cmd_status cmd_hfp_dtmf_exec(char *cmd)
{
	xr_err_t ret;
	char c = '\0';

	/* get param */
	int cnt = cmd_sscanf(cmd, "%c", &c);
	if (cnt != 1) {
		CMD_ERR("invalid param number %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	if ((ret = xr_hf_client_send_dtmf(c)) != XR_OK) {
		CMD_ERR("return failed: %d\n", ret);
		return CMD_STATUS_FAIL;
	}
	return CMD_STATUS_OK;
}

/* $hfp last_vnum */
static enum cmd_status cmd_hfp_last_vnum_exec(char *cmd)
{
	xr_err_t ret;
	if ((ret = xr_hf_client_request_last_voice_tag_number()) != XR_OK) {
		CMD_ERR("return failed: %d\n", ret);
		return CMD_STATUS_FAIL;
	}
	return CMD_STATUS_OK;
}

/* $hfp nrec_close */
static enum cmd_status cmd_hfp_nrec_close_exec(char *cmd)
{
	xr_err_t ret;
	if ((ret = xr_hf_client_send_nrec()) != XR_OK) {
		CMD_ERR("return failed: %d\n", ret);
		return CMD_STATUS_FAIL;
	}
	return CMD_STATUS_OK;
}

void set_hfp_codec(bool flag);
/* $hfp set_codec 0 */
static enum cmd_status cmd_hfp_set_codec_exec(char *cmd)
{
	int start = 0;
	xr_err_t ret;

	/* get param */
	int cnt = cmd_sscanf(cmd, "%d ", &start);
	if (cnt != 1) {
		CMD_ERR("invalid param number %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	if(start)
		set_hfp_codec(true);
	else
		set_hfp_codec(false);
	return CMD_STATUS_OK;
}

/*
    $hfp init
    $hfp deinit
    $hfp connect <addr>
        $hfp connect 31:AF:00:CC:BB:AA
    $hfp disconnect <addr>
    $hfp audio <conn/disconn> <addr>
        $hfp audio conn 31:AF:00:CC:BB:AA
    $hfp voice_rec <1/0>
    $hfp vol_update <spk/mic> <vol>
        $hfp vol_update spk 15
    $hfp dial <numbers>
        $hfp dial 13722240761
    $hfp dial_mem <location>
        $hfp dial_mem 2
    $hfp chld <type> <idx>
        $hfp chld 0 0
    $hfp btrh <hold/accept/reject>
        $hfp btrh hold
    $hfp answer
    $hfp reject
    $hfp query <call/name>
    $hfp subinfo
    $hfp dtmf <0-9/#/ * /A-D>
    $hfp last_vnum
    $hfp nrec_close
    $hfp set_codec <mode:on/off>
*/
static const struct cmd_data g_hfp_cmds[] = {
	{ "init",       cmd_hfp_init_exec,              CMD_DESC("No parameters") },
	{ "deinit",     cmd_hfp_deinit_exec,            CMD_DESC("No parameters") },
	{ "connect",    cmd_hfp_connect_exec,           CMD_DESC("<bd_addr>") },
	{ "disconnect", cmd_hfp_disconnect_exec,        CMD_DESC("<bd_addr>") },
	{ "audio",      cmd_hfp_audio_exec,             CMD_DESC("<ctrl:conn/disconn> <bd_addr>") },
	{ "voice_rec",  cmd_hfp_voice_recognition_exec, CMD_DESC("<state：1/0>") },
	{ "vol_update", cmd_hfp_vol_update_exec,        CMD_DESC("<spk/mic> <vol>") },
	{ "dial",       cmd_hfp_dial_exec,              CMD_DESC("<phone_num>") },
	{ "dial_mem",   cmd_hfp_dial_mem_exec,          CMD_DESC("<local>") },
	{ "chld",       cmd_hfp_chld_exec,              CMD_DESC("<type：0~6> <index>") },
	{ "btrh",       cmd_hfp_btrh_exec,              CMD_DESC("<hold/accept/reject>") },
	{ "answer",     cmd_hfp_answer_exec,            CMD_DESC("No parameters") },
	{ "reject",     cmd_hfp_reject_exec,            CMD_DESC("No parameters") },
	{ "query",      cmd_hfp_query_exec,             CMD_DESC("<call/name>") },
	{ "subinfo",    cmd_hfp_subinfo_exec,           CMD_DESC("No parameters") },
	{ "dtmf",       cmd_hfp_dtmf_exec,              CMD_DESC("<code：0-9，#，*，A-D>") },
	{ "last_vnum",  cmd_hfp_last_vnum_exec,         CMD_DESC("No parameters") },
	{ "nrec_close", cmd_hfp_nrec_close_exec,        CMD_DESC("No parameters") },
	{ "set_codec",  cmd_hfp_set_codec_exec,         CMD_DESC("<mode：1/0>") },
	{ "help",       cmd_hfp_help_exec,              CMD_DESC(CMD_HELP_DESC) },
};

static enum cmd_status cmd_hfp_help_exec(char *cmd)
{
	return cmd_help_exec(g_hfp_cmds, cmd_nitems(g_hfp_cmds), 10);
}

enum cmd_status cmd_hfp_exec(char *cmd)
{
	return cmd_exec(cmd, g_hfp_cmds, cmd_nitems(g_hfp_cmds));
}
