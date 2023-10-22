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

static enum cmd_status btcli_ag_help(char *cmd);

static const char *s_hfp_event_str[] = { "BTMG_HFP_AG_CONNECTION_STATE_EVT",
                                         "BTMG_HFP_AG_AUDIO_STATE_EVT",
                                         "BTMG_HFP_AG_BVRA_RESPONSE_EVT",
                                         "BTMG_HFP_AG_VOLUME_CONTROL_EVT",
                                         "BTMG_HFP_AG_UNAT_RESPONSE_EVT",
                                         "BTMG_HFP_AG_IND_UPDATE_EVT",
                                         "BTMG_HFP_AG_CIND_RESPONSE_EVT",
                                         "BTMG_HFP_AG_COPS_RESPONSE_EVT",
                                         "BTMG_HFP_AG_CLCC_RESPONSE_EVT",
                                         "BTMG_HFP_AG_CNUM_RESPONSE_EVT",
                                         "BTMG_HFP_AG_VTS_RESPONSE_EVT",
                                         "BTMG_HFP_AG_NREC_RESPONSE_EVT",
                                         "BTMG_HFP_AG_ATA_RESPONSE_EVT",
                                         "BTMG_HFP_AG_CHUP_RESPONSE_EVT",
                                         "BTMG_HFP_AG_DIAL_EVT",
                                         "BTMG_HFP_AG_WBS_RESPONSE_EVT",
                                         "BTMG_HFP_AG_BCS_RESPONSE_EVT"};

static const char *_hfp_event_to_string(btmg_hfp_ag_event_t event)
{
    return s_hfp_event_str[event];
}

void btcli_hfp_ag_connection_state_cb(const char *bd_addr, btmg_hfp_ag_connection_state_t state)
{
    if (state == BTMG_HFP_AG_DISCONNECTED) {
        CMD_DBG("hfp hf disconnected with device: %s\n", bd_addr);
    } else if (state == BTMG_HFP_AG_CONNECTING) {
        CMD_DBG("hfp hf connecting with device: %s\n", bd_addr);
    } else if (state == BTMG_HFP_AG_CONNECTED) {
        CMD_DBG("hfp hf connected with device: %s\n", bd_addr);
    } else if (state == BTMG_HFP_AG_SLC_CONNECTED) {
        CMD_DBG("hfp hf slc_connected with device: %s\n", bd_addr);
    } else if (state == BTMG_HFP_AG_DISCONNECTING) {
        CMD_DBG("hfp hf disconnecting with device: %s\n", bd_addr);
    }
}

void btcli_hfp_ag_event_cb(btmg_hfp_ag_event_t event, void *data)
{
    switch (event) {
        case BTMG_HFP_AG_CONNECTION_STATE_EVT: {
            break;
        }
        case BTMG_HFP_AG_AUDIO_STATE_EVT: {
            break;
        }
        case BTMG_HFP_AG_BVRA_RESPONSE_EVT: {
            break;
        }
        case BTMG_HFP_AG_VOLUME_CONTROL_EVT: {
            break;
        }
        case BTMG_HFP_AG_UNAT_RESPONSE_EVT: {
            break;
        }
        case BTMG_HFP_AG_IND_UPDATE_EVT: {
            break;
        }
        case BTMG_HFP_AG_CIND_RESPONSE_EVT: {
            btmg_ag_cind_t *cind = (btmg_ag_cind_t *)data;
            cind->call_status = 0;
            cind->call_setup_status = 0;
            cind->svc = 1;
            cind->signal_strength = 4;
            cind->roam = 0;
            cind->battery_level = 3;
            cind->call_held_status = 0;
            break;
        }
        case BTMG_HFP_AG_COPS_RESPONSE_EVT: {
            break;
        }
        case BTMG_HFP_AG_CLCC_RESPONSE_EVT: {
            break;
        }
        case BTMG_HFP_AG_CNUM_RESPONSE_EVT: {
            break;
        }
        case BTMG_HFP_AG_VTS_RESPONSE_EVT: {
            break;
        }
        case BTMG_HFP_AG_NREC_RESPONSE_EVT: {
            break;
        }
        case BTMG_HFP_AG_ATA_RESPONSE_EVT: { // HF Asnwer Incoming Call
            char *number = {"123456"};
		    strcpy(data, number);
            break;
        }
        case BTMG_HFP_AG_CHUP_RESPONSE_EVT: { // HF Reject Incoming Call.
            char *number = {"123456"};
		    strcpy(data, number);
            break;
        }
        case BTMG_HFP_AG_DIAL_EVT: {
            break;
        }
        case BTMG_HFP_AG_WBS_RESPONSE_EVT: {
            break;
        }
        case BTMG_HFP_AG_BCS_RESPONSE_EVT: {
            break;
        }
        break;
    default:
        break;
    }
    CMD_DBG("event:%s \n", _hfp_event_to_string(event));
}

void btcli_hfp_ag_audio_incoming_cb(const uint8_t *buf, uint32_t sz)
{
    return;
}

uint32_t btcli_hfp_ag_audio_outgoing_cb(uint8_t *buf, uint32_t sz)
{
    return sz;
}

/* btcli ag connect <device mac> */
enum cmd_status btcli_ag_connect(char *cmd)
{
    int argc;
    char *argv[1];

    argc = cmd_parse_argv(cmd, argv, 1);
    if (argc < 1) {
        CMD_ERR("invalid param number %d\n", argc);
        return CMD_STATUS_INVALID_ARG;
    }

    btmg_hfp_ag_connect(argv[0]);

    return CMD_STATUS_OK;
}

/* btcli ag connect <device mac> */
enum cmd_status btcli_ag_disconnect(char *cmd)
{
    int argc;
    char *argv[1];

    argc = cmd_parse_argv(cmd, argv, 1);
    if (argc < 1) {
        CMD_ERR("invalid param number %d\n", argc);
        return CMD_STATUS_INVALID_ARG;
    }

    btmg_hfp_ag_disconnect(argv[0]);

    return CMD_STATUS_OK;
}

/* btcli ag disconnect <device mac> */
enum cmd_status btcli_ag_connect_audio(char *cmd)
{
    int argc;
    char *argv[1];

    argc = cmd_parse_argv(cmd, argv, 1);
    if (argc < 1) {
        CMD_ERR("invalid param number %d\n", argc);
        return CMD_STATUS_INVALID_ARG;
    }

    btmg_hfp_ag_connect_audio(argv[0]);

    return CMD_STATUS_OK;
}

static const struct cmd_data hfp_cmds[] = {
    { "connect",    btcli_ag_connect,           CMD_DESC("<device mac>")},
    { "disconnect", btcli_ag_disconnect,     CMD_DESC("<device mac>")},
    { "audioconnect", btcli_ag_connect_audio,     CMD_DESC("<device mac>")},
    { "help",       btcli_ag_help,              CMD_DESC(CMD_HELP_DESC) },
};

static enum cmd_status btcli_ag_help(char *cmd)
{
	return cmd_help_exec(hfp_cmds, cmd_nitems(hfp_cmds), 10);
}

enum cmd_status btcli_ag(char *cmd)
{
    return cmd_exec(cmd, hfp_cmds, cmd_nitems(hfp_cmds));
}
