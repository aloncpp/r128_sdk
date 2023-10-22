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
#include "bt_manager.h"
#include <AudioSystem.h>

static enum cmd_status btcli_hfp_help(char *cmd);

static const char *s_hfp_event_str[] = { "BTMG_HFP_HF_BVRA_EVT",
                                         "BTMG_HFP_HF_CIND_CALL_EVT",
                                         "BTMG_HFP_HF_CIND_CALL_SETUP_EVT",
                                         "BTMG_HFP_HF_CIND_CALL_HELD_EVT",
                                         "BTMG_HFP_HF_CIND_SERVICE_AVAILABILITY_EVT",
                                         "BTMG_HFP_HF_CIND_SIGNAL_STRENGTH_EVT",
                                         "BTMG_HFP_HF_CIND_ROAMING_STATUS_EVT",
                                         "BTMG_HFP_HF_CIND_BATTERY_LEVEL_EVT",
                                         "BTMG_HFP_HF_COPS_CURRENT_OPERATOR_EVT",
                                         "BTMG_HFP_HF_BTRH_EVT",
                                         "BTMG_HFP_HF_CLIP_EVT",
                                         "BTMG_HFP_HF_CCWA_EVT",
                                         "BTMG_HFP_HF_CLCC_EVT",
                                         "BTMG_HFP_HF_VOLUME_CONTROL_EVT",
                                         "BTMG_HFP_HF_CNUM_EVT",
                                         "BTMG_HFP_HF_BSIR_EVT",
                                         "BTMG_HFP_HF_BINP_EVT",
                                         "BTMG_HFP_HF_RING_IND_EVT" };

static const char *_hfp_event_to_string(btmg_hfp_hf_event_t event)
{
    return s_hfp_event_str[event - 2];
}

#ifdef XRADIO_HFP_ADJUST_VOLUME
#define XRADIO_AMIXER_HFP_MAX 63
static uint8_t bluedroid_amixer(uint8_t volume)
{
    int ret = 0;
    int type = AUDIO_STREAM_SYSTEM;
    uint32_t volume_value = 0;
    uint8_t max_volume = 0;

    ret = softvol_control_with_streamtype(type, &volume_value, 2);
    if (ret != 0) {
        CMD_ERR("get softvol range failed:%d\n", ret);
        return -1;
    }
    max_volume = (volume_value >> 16) & 0xffff;
    volume_value = (value * max_volume / 100) & 0xffff;
    ret = softvol_control_with_streamtype(type, &volume_value, 1);
    if (ret != 0) {
        CMD_ERR("set softvol failed:%d\n", ret);
        return -1;
    }

    return ret;
}
#endif

void btcli_hfp_hf_connection_state_cb(const char *bd_addr, btmg_hfp_hf_connection_state_t state)
{
    if (state == BTMG_HFP_HF_DISCONNECTED) {
        CMD_DBG("hfp hf disconnected with device: %s\n", bd_addr);
    } else if (state == BTMG_HFP_HF_CONNECTING) {
        CMD_DBG("hfp hf connecting with device: %s\n", bd_addr);
    } else if (state == BTMG_HFP_HF_CONNECTED) {
        CMD_DBG("hfp hf connected with device: %s\n", bd_addr);
    } else if (state == BTMG_HFP_HF_SLC_CONNECTED) {
        CMD_DBG("hfp hf slc_connected with device: %s\n", bd_addr);
    } else if (state == BTMG_HFP_HF_DISCONNECTING) {
        CMD_DBG("hfp hf disconnecting with device: %s\n", bd_addr);
    }
}

void btcli_hfp_hf_event_cb(btmg_hfp_hf_event_t event, void *data)
{
    CMD_DBG("event:%s \n", _hfp_event_to_string(event));
}

/* btcli hfp voice_rec <0/1> */
static enum cmd_status btcli_hfp_voice_recognition(char *cmd)
{
    btmg_err ret;
    int start = 0;

    /* get param */
    int cnt = cmd_sscanf(cmd, "%d ", &start);
    if (cnt != 1) {
        CMD_ERR("invalid param number %d\n", cnt);
        return CMD_STATUS_INVALID_ARG;
    }

    if (start == 1) {
        if ((ret = btmg_hfp_hf_start_voice_recognition()) != BT_OK) {
            CMD_ERR("return failed: %d\n", ret);
            return CMD_STATUS_FAIL;
        }
    } else if (start == 0) {
        if ((ret = btmg_hfp_hf_stop_voice_recognition() != BT_OK)) {
            CMD_ERR("return failed: %d\n", ret);
            return CMD_STATUS_FAIL;
        }
    } else {
        CMD_ERR("invalid param %d\n", start);
        return CMD_STATUS_INVALID_ARG;
    }

    return CMD_STATUS_OK;
}

/* btcli hfp vol_update <spk/mic> <vol> */
static enum cmd_status btcli_hfp_vol_update(char *cmd)
{
    btmg_err ret;
    char devc[10] = { 0 };
    int vol;

    /* get param */
    int cnt = cmd_sscanf(cmd, "%s %d", devc, &vol);
    if (cnt != 2) {
        CMD_ERR("invalid param number %d\n", cnt);
        return CMD_STATUS_INVALID_ARG;
    }

    if (!cmd_strcmp(devc, "spk")) {
        if ((ret = btmg_hfp_hf_spk_vol_update(vol)) != BT_OK) {
            CMD_ERR("return failed: %d\n", ret);
            return CMD_STATUS_FAIL;
        }
#ifdef XRADIO_HFP_ADJUST_VOLUME
        int ret = -1;
        CMD_DBG("adjusting the volume by ourselves\n");
        vol = XRADIO_AMIXER_HFP_MAX - (uint32_t)vol * 4;
        CMD_DBG("volume is %d\n", vol);
        ret = bluedroid_amixer(vol);
        if (ret != XR_OK) {
            CMD_ERR("Set vol by ourself failed \n");
        }
#endif
    } else if (!cmd_strcmp(devc, "mic")) {
        if ((ret = btmg_hfp_hf_mic_vol_update(vol)) != BT_OK) {
            CMD_ERR("return failed: %d\n", ret);
            return CMD_STATUS_FAIL;
        }
    } else {
        CMD_ERR("invalid param %s\n", devc);
        return CMD_STATUS_INVALID_ARG;
    }

    return CMD_STATUS_OK;
}

/* btcli hfp dial 10086 */
static enum cmd_status btcli_hfp_dial(char *cmd)
{
    btmg_err ret;
    char number[30] = { 0 };

    /* get param */
    int cnt = cmd_sscanf(cmd, "%s", number);
    if (cnt != 1) {
        CMD_ERR("invalid param number %d\n", cnt);
        return CMD_STATUS_INVALID_ARG;
    }

    if ((ret = btmg_hfp_hf_dial(number)) != BT_OK) {
        CMD_ERR("return failed: %d\n", ret);
        return CMD_STATUS_FAIL;
    }

    return CMD_STATUS_OK;
}

/* btcli hfp dial_mem 2 */
static enum cmd_status btcli_hfp_dial_mem(char *cmd)
{
    btmg_err ret;
    int loc;

    /* get param */
    int cnt = cmd_sscanf(cmd, "%d", &loc);
    if (cnt != 1) {
        CMD_ERR("invalid param number %d\n", cnt);
        return CMD_STATUS_INVALID_ARG;
    }

    if ((ret = btmg_hfp_hf_dial_memory(loc)) != BT_OK) {
        CMD_ERR("return failed: %d\n", ret);
        return CMD_STATUS_FAIL;
    }

    return CMD_STATUS_OK;
}

/* btcli hfp chld 0 0 */
static enum cmd_status btcli_hfp_chld(char *cmd)
{
    btmg_err ret;
    int chld;
    int idx = 0;

    /* get param */
    int cnt = cmd_sscanf(cmd, "%d %d", &chld, &idx);
    if (cnt != 2 && (chld == BTMG_HF_CHLD_TYPE_REL_X || chld == BTMG_HF_CHLD_TYPE_PRIV_X)) {
        CMD_ERR("invalid param number %d\n", cnt);
        return CMD_STATUS_INVALID_ARG;
    }

    if ((ret = btmg_hfp_hf_send_chld_cmd((btmg_hf_chld_type_t)chld, idx)) != BT_OK) {
        CMD_ERR("return failed: %d\n", ret);
        return CMD_STATUS_FAIL;
    }

    return CMD_STATUS_OK;
}

/* btcli hfp btrh <hold/accept/reject> */
static enum cmd_status btcli_hfp_btrh(char *cmd)
{
    btmg_err ret;
    char btrhcmd[10] = { 0 };
    btmg_hf_btrh_cmd_t btrh;

    /* get param */
    int cnt = cmd_sscanf(cmd, "%s", btrhcmd);
    if (cnt != 1) {
        CMD_ERR("invalid param number %d\n", cnt);
        return CMD_STATUS_INVALID_ARG;
    }

    if (!cmd_strcmp(btrhcmd, "hold"))
        btrh = BTMG_HF_BTRH_CMD_HOLD;
    else if (!cmd_strcmp(btrhcmd, "accept"))
        btrh = BTMG_HF_BTRH_CMD_ACCEPT;
    else if (!cmd_strcmp(btrhcmd, "reject"))
        btrh = BTMG_HF_BTRH_CMD_REJECT;
    else {
        CMD_ERR("invalid param %s\n", btrhcmd);
        return CMD_STATUS_INVALID_ARG;
    }

    if ((ret = btmg_hfp_hf_send_btrh_cmd(btrh)) != BT_OK) {
        CMD_ERR("return failed: %d\n", ret);
        return CMD_STATUS_FAIL;
    }
    return CMD_STATUS_OK;
}

/* btcli hfp answer */
static enum cmd_status btcli_hfp_answer(char *cmd)
{
    btmg_err ret;

    if ((ret = btmg_hfp_hf_answer_call() != BT_OK)) {
        CMD_ERR("return failed: %d\n", ret);
        return CMD_STATUS_FAIL;
    }
    return CMD_STATUS_OK;
}

/* btcli hfp reject */
static enum cmd_status btcli_hfp_reject(char *cmd)
{
    btmg_err ret;

    if ((ret = btmg_hfp_hf_reject_call()) != BT_OK) {
        CMD_ERR("return failed: %d\n", ret);
        return CMD_STATUS_FAIL;
    }
    return CMD_STATUS_OK;
}

/* btcli hfp query <call/name> */
static enum cmd_status btcli_hfp_query(char *cmd)
{
    btmg_err ret;
    char query[10] = { 0 };

    /* get param */
    int cnt = cmd_sscanf(cmd, "%s", query);
    if (cnt != 1) {
        CMD_ERR("invalid param number %d\n", cnt);
        return CMD_STATUS_INVALID_ARG;
    }

    if (!cmd_strcmp(query, "call")) {
        if ((ret = btmg_hfp_hf_query_calls()) != BT_OK) {
            CMD_ERR("return failed: %d\n", ret);
            return CMD_STATUS_FAIL;
        }
    } else if (!cmd_strcmp(query, "name")) {
        if ((ret = btmg_hfp_hf_query_operator()) != BT_OK) {
            CMD_ERR("return failed: %d\n", ret);
            return CMD_STATUS_FAIL;
        }
    } else {
        CMD_ERR("invalid param %s\n", query);
        return CMD_STATUS_INVALID_ARG;
    }

    return CMD_STATUS_OK;
}

/* btcli hfp number */
static enum cmd_status btcli_hfp_phone_number(char *cmd)
{
    btmg_err ret;

    if ((ret = btmg_hfp_hf_query_number()) != BT_OK) {
        CMD_ERR("return failed: %d\n", ret);
        return CMD_STATUS_FAIL;
    }
    return CMD_STATUS_OK;
}

/* btcli hfp dtmf <0-9/#/ * > */
static enum cmd_status btcli_hfp_dtmf(char *cmd)
{
    btmg_err ret;
    char c = '\0';

    /* get param */
    int cnt = cmd_sscanf(cmd, "%c", &c);
    if (cnt != 1) {
        CMD_ERR("invalid param number %d\n", cnt);
        return CMD_STATUS_INVALID_ARG;
    }

    if ((ret = btmg_hfp_hf_send_dtmf(c)) != BT_OK) {
        CMD_ERR("return failed: %d\n", ret);
        return CMD_STATUS_FAIL;
    }
    return CMD_STATUS_OK;
}

/* btcli hfp last_vnum */
static enum cmd_status btcli_hfp_last_vnum(char *cmd)
{
    btmg_err ret;

    if ((ret = btmg_hfp_hf_request_last_voice_tag_number()) != BT_OK) {
        CMD_ERR("return failed: %d\n", ret);
        return CMD_STATUS_FAIL;
    }
    return CMD_STATUS_OK;
}

/* btcli hfp nrec_close */
static enum cmd_status btcli_hfp_nrec_close(char *cmd)
{
    btmg_err ret;

    if ((ret = btmg_hfp_hf_send_nrec()) != BT_OK) {
        CMD_ERR("return failed: %d\n", ret);
        return CMD_STATUS_FAIL;
    }
    return CMD_STATUS_OK;
}

static const struct cmd_data hfp_cmds[] = {
    { "voice_rec",  btcli_hfp_voice_recognition,  CMD_DESC("<state：1/0>") },
    { "vol_update", btcli_hfp_vol_update,         CMD_DESC("<spk/mic> <vol>")},
    { "dial",       btcli_hfp_dial,               CMD_DESC("<phone_num>")},
    { "dial_mem",   btcli_hfp_dial_mem,           CMD_DESC("<local>")},
    { "chld",       btcli_hfp_chld,               CMD_DESC("<type：0~6> <index>")},
    { "btrh",       btcli_hfp_btrh,               CMD_DESC("<hold/accept/reject>")},
    { "answer",     btcli_hfp_answer,             CMD_DESC("No parameters")},
    { "reject",     btcli_hfp_reject,             CMD_DESC("No parameters")},
    { "query",      btcli_hfp_query,              CMD_DESC("<call/name>")},
    { "number",     btcli_hfp_phone_number,        CMD_DESC("No parameters")},
    { "dtmf",       btcli_hfp_dtmf,               CMD_DESC("<code：0-9，#，*，A-D>")},
    { "last_vnum",  btcli_hfp_last_vnum,          CMD_DESC("No parameters")},
    { "nrec_close", btcli_hfp_nrec_close,         CMD_DESC("No parameters")},
    { "help",       btcli_hfp_help,               CMD_DESC(CMD_HELP_DESC) },
};

static enum cmd_status btcli_hfp_help(char *cmd)
{
	return cmd_help_exec(hfp_cmds, cmd_nitems(hfp_cmds), 10);
}

enum cmd_status btcli_hfp(char *cmd)
{
    return cmd_exec(cmd, hfp_cmds, cmd_nitems(hfp_cmds));
}
