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
#include "ble/bluetooth/bluetooth.h"
#include "ble/bluetooth/mesh.h"
#include "ble/bluetooth/mesh/mesh_common.h"
#include "zephyr.h"
#include "bluetooth/mesh/access.h"
#include "common/log.h"

void model_cli_internal_get_msg_ctx(struct model_cli_internal *cli,
                struct bt_mesh_msg_ctx *ctx)
{
	ctx->net_idx = cli->net_idx;
	ctx->app_idx = cli->app_idx;
	ctx->send_rel = cli->send_rel;
	ctx->send_ttl = cli->send_ttl;
}

void model_cli_internal_release(struct model_cli_internal *cli)
{
	k_sem_give(&cli->op_sync);
}

int model_cli_internal_param_get(struct model_cli_internal *cli,
	            void **param, uint32_t op)
{
	if (cli->op_pending != op) {
		return -EINVAL;
	}

	*param = cli->op_param;

	return 0;
}

int model_cli_internal_prepare(struct model_cli_internal *cli,
	            void *param, uint32_t op)
{
	if (!cli) {
		BT_ERR("No available Configuration Client context!");
		return -EINVAL;
	}

	if (cli->op_pending) {
		BT_WARN("Another synchronous operation pending");
		return -EBUSY;
	}

	cli->op_param = param;
	cli->op_pending = op;

	return 0;
}

void model_cli_internal_reset(struct model_cli_internal *cli)
{
	cli->op_pending = 0U;
	cli->op_param = NULL;
}

int model_cli_internal_wait(struct model_cli_internal *cli)
{
	int err;

	/* TODO: it might be better to use event observer */
	err = k_sem_take(&cli->op_sync, cli->msg_timeout);

	model_cli_internal_reset(cli);

	return err;
}

int model_cli_internal_init(struct model_cli_internal *cli)
{
	if (k_sem_init(&cli->op_sync, 0, 1) != 0) {
		return -ENOMEM;
	}

	cli->app_idx = 0U;
	cli->net_idx = 0U;
	cli->send_rel = 0U;
	cli->send_ttl = BT_MESH_TTL_DEFAULT;
	cli->op_pending = 0U;
	cli->op_param = NULL;
	cli->msg_timeout = K_SECONDS(2);

	return 0;
}


