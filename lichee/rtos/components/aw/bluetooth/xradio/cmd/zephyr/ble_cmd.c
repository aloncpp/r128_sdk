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
#include "cmd_gatt.h"
#include "cmd_ble.h"
#include "cmd_mesh.h"
#include "cmd_adv.h"

#if 0
static const struct cmd_data g_ble_cmds[] = {
	{ "ble",       cmd_ble_exec },
	{ "gatt",       cmd_gatt_exec },
	{ "mesh",       cmd_mesh_exec },
};
#endif

static void msh_ble_exec(int argc, char *argv[])
{
	cmd2_main_exec(argc, argv, cmd_ble_exec);
}

static void msh_gatt_exec(int argc, char *argv[])
{
	cmd2_main_exec(argc, argv, cmd_gatt_exec);
}

#ifdef CONFIG_BT_MESH
static void msh_mesh_exec(int argc, char *argv[])
{
	cmd2_main_exec(argc, argv, cmd_mesh_exec);
}
#endif

static void msh_adv_exec(int argc, char *argv[])
{
	cmd2_main_exec(argc, argv, cmd_adv_exec);
}

FINSH_FUNCTION_EXPORT_CMD(msh_ble_exec, ble, ble testcmd);
FINSH_FUNCTION_EXPORT_CMD(msh_gatt_exec, gatt, ble testcmd);

#ifdef CONFIG_BT_MESH
FINSH_FUNCTION_EXPORT_CMD(msh_mesh_exec, mesh, ble testcmd);
#endif

FINSH_FUNCTION_EXPORT_CMD(msh_adv_exec, adv, adv testcmd);
