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
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include "cmd_util.h"
#include "bt_manager.h"

static enum cmd_status btcli_spps_help(char *cmd);

void btcli_spps_conn_status_cb(const char *bd_addr, btmg_spp_connection_state_t state)
{
    if (state == BTMG_SPP_DISCONNECTED) {
        CMD_DBG("spp server disconnected with device: %s\n", bd_addr);
    } else if (state == BTMG_SPP_CONNECTING) {
        CMD_DBG("spp server connecting with device: %s\n", bd_addr);
    } else if (state == BTMG_SPP_CONNECTED) {
        CMD_DBG("spp server connected with device: %s\n", bd_addr);
    } else if (state == BTMG_SPP_DISCONNECTING) {
        CMD_DBG("spp server disconnecting with device: %s\n", bd_addr);
    } else if (state == BTMG_SPP_CONNECT_FAILED) {
        CMD_DBG("spp server connect with device: %s failed!\n", bd_addr);
    } else if (state == BTMG_SPP_DISCONNEC_FAILED) {
        CMD_DBG("spp server disconnect with device: %s failed!\n", bd_addr);
    }
}

void btcli_spps_recvdata_cb(const char *bd_addr, char *data, int data_len)
{
    char recv_data[data_len + 1];

    memcpy(recv_data, data, data_len);
    recv_data[data_len] = '\0';
    CMD_DBG("spps recv from dev:[%s][len=%d][data:%s]\n", bd_addr, data_len,
            recv_data);
}

/* btcli spps start <scn> */
static enum cmd_status btcli_spps_start(char *cmd)
{
    int argc;
    int scn = 0;
    char *argv[1];
    argc = cmd_parse_argv(cmd, argv, 1);

    if (argc != 1) {
        CMD_ERR("invalid param number %d\n", argc);
        return CMD_STATUS_INVALID_ARG;
    }

    scn = cmd_atoi(argv[0]);

    if (btmg_spps_start(scn) != BT_OK) {
        CMD_ERR("spps start fail\n");
    }

    return CMD_STATUS_OK;
}

/* btcli spps stop */
static enum cmd_status btcli_spps_stop(char *cmd)
{
    int argc;
    char *argv[1];
    argc = cmd_parse_argv(cmd, argv, 1);

    if (argc > 1) {
        CMD_ERR("invalid param number %d\n", argc);
        return CMD_STATUS_INVALID_ARG;
    }

    btmg_spps_stop();

    return CMD_STATUS_OK;
}

/* btcli spps disconnect <device mac> */
static enum cmd_status btcli_spps_disconnect(char *cmd)
{
    int argc;
    char *argv[1];
    argc = cmd_parse_argv(cmd, argv, 1);

    if (argc != 1) {
        CMD_ERR("invalid param number %d\n", argc);
        return CMD_STATUS_INVALID_ARG;
    }

    btmg_spps_disconnect(argv[0]);

    return CMD_STATUS_OK;
}

/* btcli spps write <data>*/
static enum cmd_status btcli_spps_write(char *cmd)
{
    int argc;
    char *argv[1];
    argc = cmd_parse_argv(cmd, argv, 1);

    if (argc != 1) {
        CMD_ERR("invalid param number %d\n", argc);
        return CMD_STATUS_INVALID_ARG;
    }

    btmg_spps_write(argv[0], strlen(argv[0]));

    return CMD_STATUS_OK;
}

static const struct cmd_data spps_cmds[] = {
    { "start",      btcli_spps_start,      CMD_DESC("<scn>")},
    { "stop",       btcli_spps_stop,       CMD_DESC("No parameters")},
    { "disconnect", btcli_spps_disconnect, CMD_DESC("<device mac>")},
    { "write",      btcli_spps_write,      CMD_DESC("<data>")},
    { "help",       btcli_spps_help,       CMD_DESC(CMD_HELP_DESC)},
};

/* btcli spps help */
static enum cmd_status btcli_spps_help(char *cmd)
{
	return cmd_help_exec(spps_cmds, cmd_nitems(spps_cmds), 10);
}

enum cmd_status btcli_spps(char *cmd)
{
    return cmd_exec(cmd, spps_cmds, cmd_nitems(spps_cmds));
}
