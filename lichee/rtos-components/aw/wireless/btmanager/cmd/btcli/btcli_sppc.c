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

static enum cmd_status btcli_sppc_help(char *cmd);

void btcli_sppc_conn_status_cb(const char *bd_addr, btmg_spp_connection_state_t state)
{
    if (state == BTMG_SPP_DISCONNECTED) {
        CMD_DBG("spp client disconnected with device: %s\n", bd_addr);
    } else if (state == BTMG_SPP_CONNECTING) {
        CMD_DBG("spp client connecting with device: %s\n", bd_addr);
    } else if (state == BTMG_SPP_CONNECTED) {
        CMD_DBG("spp client connected with device: %s\n", bd_addr);
    } else if (state == BTMG_SPP_DISCONNECTING) {
        CMD_DBG("spp client disconnecting with device: %s\n", bd_addr);
    } else if (state == BTMG_SPP_CONNECT_FAILED) {
        CMD_DBG("spp client connect with device: %s failed!\n", bd_addr);
    } else if (state == BTMG_SPP_DISCONNEC_FAILED) {
        CMD_DBG("spp client disconnect with device: %s failed!\n", bd_addr);
    }
}

void btcli_sppc_recvdata_cb(const char *bd_addr, char *data, int data_len)
{
    char recv_data[data_len + 1];

    memcpy(recv_data, data, data_len);
    recv_data[data_len] = '\0';

    CMD_DBG("sppc recv from dev:[%s],[len=%d][data:%s]\n", bd_addr, data_len,
            recv_data);
}

/* btcli sppc connect <device mac> */
static enum cmd_status btcli_sppc_connect(char *cmd)
{
    int argc;
    char *argv[1];

    argc = cmd_parse_argv(cmd, argv, 1);
    if (argc != 1) {
        CMD_ERR("invalid param number %d\n", argc);
        return CMD_STATUS_INVALID_ARG;
    }

    btmg_sppc_connect(argv[0]);

    return CMD_STATUS_OK;
}

/* btcli sppc disconnect <device mac> */
static enum cmd_status btcli_sppc_disconnect(char *cmd)
{
    int argc;
    char *argv[1];

    argc = cmd_parse_argv(cmd, argv, 1);
    if (argc != 1) {
        CMD_ERR("invalid param number %d\n", argc);
        return CMD_STATUS_INVALID_ARG;
    }

    btmg_sppc_disconnect(argv[0]);

    return CMD_STATUS_OK;
}

/* btcli sppc write <data> */
static enum cmd_status btcli_sppc_write(char *cmd)
{
    int argc;
    char *argv[1];

    argc = cmd_parse_argv(cmd, argv, 1);
    if (argc != 1) {
        CMD_ERR("invalid param number %d\n", argc);
        return CMD_STATUS_INVALID_ARG;
    }

    btmg_sppc_write(argv[0], strlen(argv[0]));

    return CMD_STATUS_OK;
}

static const struct cmd_data sppc_cmds[] = {
    { "connect",    btcli_sppc_connect,     CMD_DESC("<device mac>")},
    { "disconnect", btcli_sppc_disconnect,  CMD_DESC("<device mac>")},
    { "write",      btcli_sppc_write,       CMD_DESC("<data>")},
    { "help",       btcli_sppc_help,      CMD_DESC(CMD_HELP_DESC)},
};

/* btcli sppc help */
static enum cmd_status btcli_sppc_help(char *cmd)
{
	return cmd_help_exec(sppc_cmds, cmd_nitems(sppc_cmds), 10);
}

enum cmd_status btcli_sppc(char *cmd)
{
    return cmd_exec(cmd, sppc_cmds, cmd_nitems(sppc_cmds));
}
