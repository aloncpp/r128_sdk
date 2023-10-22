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

extern enum cmd_status cmd_a2dp_sink_exec(char *cmd);
extern enum cmd_status cmd_a2dp_source_exec(char *cmd);
extern enum cmd_status cmd_gap_exec(char *cmd);
extern enum cmd_status cmd_avrc_exec(char *cmd);
extern enum cmd_status cmd_hfp_exec(char *cmd);
extern enum cmd_status cmd_hfp_ag_exec(char *cmd);
extern enum cmd_status cmd_bt_init_exec(char *cmd);
extern enum cmd_status cmd_bt_deinit_exec(char *cmd);

#ifndef CONFIG_BT_DUAL_HOST
extern enum cmd_status cmd_gattc_exec(char *cmd);
extern enum cmd_status cmd_gatts_exec(char *cmd);
extern enum cmd_status cmd_prov_exec(char *cmd);
extern enum cmd_status cmd_node_exec(char *cmd);
extern enum cmd_status cmd_adv_exec(char *cmd);
#endif
extern enum cmd_status cmd_spp_exec(char *cmd);
static enum cmd_status cmd_main_help_exec(char *cmd);

static const struct cmd_data g_bt_cmds[] = {
	{ "init",           cmd_bt_init_exec,     CMD_DESC("Initialize BT module") },
	{ "deinit",         cmd_bt_deinit_exec,   CMD_DESC("De Initialize BT module") },
	{ "gap",            cmd_gap_exec,         CMD_DESC("Generic Access Profile, support inquiry,page, connect and other functions, use bt gap help view!") },
#ifdef CONFIG_BT_A2DP_ENABLE
	{ "a2dp_src",       cmd_a2dp_source_exec, CMD_DESC("Advanced Audio Distribution Profile (SOURCE), support paly songs and other functions,use bt a2dp_src help view!") },
	{ "a2dp_snk",       cmd_a2dp_sink_exec,   CMD_DESC("Advanced Audio Distribution Profile (SINK), support receive audio, use bt a2dp_snk help view!") },
	{ "avrc",           cmd_avrc_exec,        CMD_DESC("Audio/Video Remote Control Profile, support audio playback control and other functions, use bt avrc help view!") },
#endif
#ifdef CONFIG_BT_HFP_CLIENT_ENABLE
	{ "hfp",            cmd_hfp_exec,         CMD_DESC("Hands-free Profile, support answering, hanging up, rejecting, voice dialing and other functions, use bt hfp help view!") },
#endif
#ifdef CONFIG_BT_HFP_AG_ENABLE
	{"ag",              cmd_hfp_ag_exec,      CMD_DESC("Hands-free AudioGate Profile")},
#endif
#ifdef CONFIG_BT_SPP_ENABLED
	{ "spp",            cmd_spp_exec,         CMD_DESC("Serial Port Profile, support send data, use bt spp help view!") },
#endif
	{ "help",           cmd_main_help_exec,   CMD_DESC(CMD_HELP_DESC) },
#ifndef CONFIG_BT_DUAL_HOST
#ifdef CONFIG_BT_GATTC_ENABLE
	{ "gattc",          cmd_gattc_exec },
#endif
#ifdef CONFIG_BT_GATTS_ENABLE
	{ "gatts",          cmd_gatts_exec },
#endif
#ifdef CONFIG_BLE_MESH
	{ "prov",           cmd_prov_exec },
	{ "node",           cmd_node_exec },
#endif
	{ "adv",            cmd_adv_exec },
#endif
};

static enum cmd_status cmd_main_help_exec(char *cmd)
{
	return cmd_help_exec(g_bt_cmds, cmd_nitems(g_bt_cmds), 8);
}

static enum cmd_status cmd_bt_exec(char *cmd)
{
	return cmd_main_exec(cmd, g_bt_cmds, cmd_nitems(g_bt_cmds));
}

static void msh_bt_exec(int argc, char *argv[])
{
	char *ptr;
	ptr = cmd_conv_from_argv(argc, argv, 1);
	if (argc == 1) {
		cmd_help_exec(g_bt_cmds, cmd_nitems(g_bt_cmds), 8);
	} else {
		CMD_DBG("cmd: %s\n", ptr);
		cmd_bt_exec(ptr);
	}
}

FINSH_FUNCTION_EXPORT_CMD(msh_bt_exec, bt, bluetooth testcmd);
