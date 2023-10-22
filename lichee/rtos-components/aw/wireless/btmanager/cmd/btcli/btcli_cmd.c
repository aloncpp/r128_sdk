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

#include <console.h>
#include "cmd_util.h"
#include <unistd.h>
#include <getopt.h>
#include "btcli_common.h"
#include "bt_manager.h"

static enum cmd_status btcli_main_help(char *cmd);

static const struct cmd_data btcli_cmds[] = {
    { "init",            btcli_init,             CMD_DESC("Initialize bt adater")},
    { "deinit",          btcli_deinit,           CMD_DESC("DeInitialize bt adater")},
    { "debug",           btcli_debug,            CMD_DESC("debug [0~5]: set debug level")},
    { "ex_debug",        btcli_ex_debug,         CMD_DESC("ex_dbg [mask]: set ex debug mask")},
    { "scan_mode",       btcli_set_scanmode,     CMD_DESC("<0~2>:0-NONE,1-page scan,2-inquiry scan&page scan")},
    { "scan",            btcli_scan,             CMD_DESC("<on/off>: device discovery")},
    { "scan_list",       btcli_scan_list,        CMD_DESC("list available devices")},
    { "io_cap",          btcli_set_io_cap,       CMD_DESC("<0~3>:0-displayonly,1-displayyesno,2-keyboardonly,3-noinputnooutput")},
    { "get_dev_name",    btcli_get_device_name,  CMD_DESC("get remote device name")},
    { "get_name",        btcli_get_adapter_name, CMD_DESC("get bt adapter name")},
    { "set_name",        btcli_set_adapter_name, CMD_DESC("<name>:set bt adapter name")},
    { "get_mac",         btcli_get_adapter_mac,  CMD_DESC("get bt adapter address")},
    { "pincode",         btcli_pincode,          CMD_DESC("<0000~9999>:enter pincode ")},
    { "passkey",         btcli_passkey,          CMD_DESC("<000000~999999>:enter passkey")},
    { "passkey_confirm", btcli_passkey_confirm,  CMD_DESC("confirm passkey")},
    { "pairing_confirm", btcli_pairing_confirm,  CMD_DESC("confirm pairing")},
    { "paired_list",     btcli_paired_list,      CMD_DESC("list paired devices")},
    { "unpair_dev",      btcli_unpair_dev,       CMD_DESC("unpair bond devices")},
#ifdef CONFIG_BT_A2DP_ENABLE
    { "a2dp_src",        btcli_a2dp_source,      CMD_DESC("support paly songs and other functions,use a2dp_src help view")},
    { "a2dp_snk",        btcli_a2dp_sink,        CMD_DESC("support receive audio, use a2dp_snk help view")},
    { "avrc",            btcli_avrc,             CMD_DESC("support audio playback control and other functions, use avrc help view")},
#endif
#ifdef CONFIG_BT_HFP_CLIENT_ENABLE
    { "hfp",             btcli_hfp,              CMD_DESC("support answering, hanging up, rejecting, voice dialing and other functions, use hfp help view")},
#endif
#ifdef CONFIG_BT_HFP_AG_ENABLE
    { "ag",              btcli_ag,               CMD_DESC("support answering, hanging up, rejecting, voice dialing and other functions, use ag help view")},
#endif
#ifdef CONFIG_BT_SPP_ENABLED
    { "spps",            btcli_spps,             CMD_DESC("support data transmission, use spps help view")},
    { "sppc",            btcli_sppc,             CMD_DESC("support data transmission, use sppc help view")},
    { "ble",             btcli_ble,              CMD_DESC("support ble, use ble help view")},
    { "gatt",            btcli_gatt,             CMD_DESC("support gatt, use gatt help view")},
    { "help",            btcli_main_help,        CMD_DESC(CMD_HELP_DESC) },
#endif
};

/* btcli or btcli help */
static enum cmd_status btcli_main_help(char *cmd)
{
	return cmd_help_exec(btcli_cmds, cmd_nitems(btcli_cmds), 24);
}

static enum cmd_status cmd_btcli(char *cmd)
{
    return cmd_main_exec(cmd, btcli_cmds, cmd_nitems(btcli_cmds));
}

static void msh_btcli(int argc, char *argv[])
{
    char *ptr;
    ptr = cmd_conv_from_argv(argc, argv, 1);
    if (argc == 1) {
        cmd_help_exec(btcli_cmds, cmd_nitems(btcli_cmds), 8);
    } else {
        cmd_btcli(ptr);
	}
}

FINSH_FUNCTION_EXPORT_CMD(msh_btcli, btcli, bluetooth client testcmd);
