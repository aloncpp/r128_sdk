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
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "cmd_util.h"
#include "bt_manager.h"

static enum cmd_status btcli_sink_help(char *cmd);

void btcli_a2dp_sink_connection_state_cb(const char *bd_addr,
                                         btmg_a2dp_sink_connection_state_t state)
{
    if (state == BTMG_A2DP_SINK_DISCONNECTED) {
        CMD_DBG("A2DP sink disconnected with device: %s\n", bd_addr);
        btmg_adapter_set_scanmode(BTMG_SCAN_MODE_CONNECTABLE_DISCOVERABLE);
    } else if (state == BTMG_A2DP_SINK_CONNECTING) {
        CMD_DBG("A2DP sink connecting with device: %s\n", bd_addr);
    } else if (state == BTMG_A2DP_SINK_CONNECTED) {
        btmg_adapter_set_scanmode(BTMG_SCAN_MODE_NONE);
        CMD_DBG("A2DP sink connected with device: %s\n", bd_addr);
    } else if (state == BTMG_A2DP_SINK_DISCONNECTING) {
        CMD_DBG("A2DP sink disconnecting with device: %s\n", bd_addr);
    }
}

void btcli_a2dp_sink_audio_state_cb(const char *bd_addr, btmg_a2dp_sink_audio_state_t state)
{
    if (state == BTMG_A2DP_SINK_AUDIO_SUSPENDED) {
        CMD_DBG("A2DP sink audio suspended with device: %s\n", bd_addr);
    } else if (state == BTMG_A2DP_SINK_AUDIO_STOPPED) {
        CMD_DBG("A2DP sink audio stopped with device: %s\n", bd_addr);
    } else if (state == BTMG_A2DP_SINK_AUDIO_STARTED) {
        CMD_DBG("A2DP sink audio started with device: %s\n", bd_addr);
    }
}

void btcli_a2dp_sink_stream_cb(const char *bd_addr, uint16_t channels, uint16_t sampling,
                               uint8_t *data, uint32_t len)
{
    (void)data;

#if 0
    CMD_DBG("A2DP sink device: %s, pcm_config: channels:%d, sampling:%d, data len:%d\n", bd_addr,
             channels, sampling, len);
#endif
}

/* btcli a2dp_snk connect <device mac> */
enum cmd_status btcli_sink_connect(char *cmd)
{
    int argc;
    char *argv[1];

    argc = cmd_parse_argv(cmd, argv, 1);
    if (argc < 1) {
        CMD_ERR("invalid param number %d\n", argc);
        return CMD_STATUS_INVALID_ARG;
    }

    btmg_a2dp_sink_connect(argv[0]);

    return CMD_STATUS_OK;
}

/* btcli a2dp_snk disconnect <device mac> */
enum cmd_status btcli_sink_disconnect(char *cmd)
{
    int argc;
    char *argv[1];

    argc = cmd_parse_argv(cmd, argv, 1);
    if (argc < 1) {
        CMD_ERR("invalid param number %d\n", argc);
        return CMD_STATUS_INVALID_ARG;
    }

    btmg_a2dp_sink_disconnect(argv[0]);

    return CMD_STATUS_OK;
}

static const struct cmd_data a2dp_sink_cmds[] = {
    { "connect",    btcli_sink_connect,    CMD_DESC("<device mac>")},
    { "disconnect", btcli_sink_disconnect, CMD_DESC("<device mac>")},
    { "help",       btcli_sink_help,       CMD_DESC(CMD_HELP_DESC) },
};

/* btcli a2dp_snk help */
static enum cmd_status btcli_sink_help(char *cmd)
{
	return cmd_help_exec(a2dp_sink_cmds, cmd_nitems(a2dp_sink_cmds), 10);
}

enum cmd_status btcli_a2dp_sink(char *cmd)
{
    return cmd_exec(cmd, a2dp_sink_cmds, cmd_nitems(a2dp_sink_cmds));
}
