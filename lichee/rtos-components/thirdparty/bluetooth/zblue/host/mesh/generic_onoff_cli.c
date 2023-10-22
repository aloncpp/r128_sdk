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
#include <stdio.h>
#include <string.h>

#include "ble/bluetooth/bluetooth.h"
#include "ble/bluetooth/mesh.h"
#include "ble/bluetooth/mesh/generic_onoff_cli.h"
#include "ble/bluetooth/mesh/mesh_common.h"
#include "ble/bluetooth/mesh/transition.h"

#include "host/mesh/net.h"
#include "host/mesh/transport.h"

#include "common/log.h"
#include "mesh.h"

#define GEN_ONOFF_OFF (0x00)
#define GEN_ONOFF_ON  (0x01)

struct gen_onoff_status_param {
	/* client information */

	/* server information */
	u8_t present_onoff;	//the present value of Generic OnOff state
	u8_t target_onoff;	//the target value of Generic OnOff state
	u8_t remain_time;	//remaining time
};

int bt_mesh_gen_onoff_cli_set(struct bt_mesh_model *model, u16_t addr,
	                      u8_t onoff, struct bt_mesh_transition_params *opt)
{
	int err;
	struct bt_mesh_gen_onoff_cli *goo_cli = model->user_data;
	struct gen_onoff_status_param param;
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_GEN_ONOFF_SET, 4);
	struct bt_mesh_msg_ctx ctx;

	if (onoff > GEN_ONOFF_ON) {
		BT_WARN("onoff value should be bool %d\n", onoff);
		return -EINVAL;
	}

	model_cli_internal_get_msg_ctx(&goo_cli->cli, &ctx);
	ctx.addr = addr;

	bt_mesh_model_msg_init(&msg, OP_GEN_ONOFF_SET);

	net_buf_simple_add_u8(&msg, onoff);
	net_buf_simple_add_u8(&msg, goo_cli->tid++);

	if (opt) {
		if (opt->trans_time == 0) {
			BT_WARN("Transition Time is zero.");
			return -EINVAL;
		}
		else {
			net_buf_simple_add_u8(&msg, opt->trans_time);
			net_buf_simple_add_u8(&msg, opt->delay);
		}
	}

	model_cli_internal_prepare(&goo_cli->cli, &param, OP_GEN_ONOFF_STATUS);

	err = bt_mesh_model_send(model, &ctx, &msg, NULL, NULL);
	if (err) {
		BT_ERR("Unable to send Generic OnOff Set message.");
		model_cli_internal_reset(&goo_cli->cli);
		return err;
	}

	err = model_cli_internal_wait(&goo_cli->cli);

//	err = (param.present_onoff == onoff
//		   || param.target_onoff == onoff) ? 0 : -EINVAL;

	return err;
}

int bt_mesh_gen_onoff_cli_set_unack(struct bt_mesh_model *model, u16_t addr,
	                      u8_t onoff, struct bt_mesh_transition_params *opt)
{
	int err;
	struct bt_mesh_gen_onoff_cli *goo_cli = model->user_data;
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_GEN_ONOFF_SET_UNACK, 4);
	struct bt_mesh_msg_ctx ctx;

	if (onoff > GEN_ONOFF_ON) {
		BT_WARN("onoff value should be bool %d\n", onoff);
		return -EINVAL;
	}

	model_cli_internal_get_msg_ctx(&goo_cli->cli, &ctx);
	ctx.addr = addr;

	bt_mesh_model_msg_init(&msg, OP_GEN_ONOFF_SET_UNACK);

	net_buf_simple_add_u8(&msg, onoff);
	net_buf_simple_add_u8(&msg, goo_cli->tid++);

	if (opt) {
		if (opt->trans_time == 0) {
			BT_WARN("Transition Time is zero.");
			return -EINVAL;
		}
		else {
			net_buf_simple_add_u8(&msg, opt->trans_time);
			net_buf_simple_add_u8(&msg, opt->delay);
		}
	}

	err = bt_mesh_model_send(model, &ctx, &msg, NULL, NULL);
	if (err) {
		BT_ERR("Unable to send Generic OnOff Set message.");
		return err;
	}

	return err;
}

int bt_mesh_gen_onoff_cli_get(struct bt_mesh_model *model, u16_t addr,
	                          u8_t *present, u8_t *target, u8_t *remain)
{
	int err;
	struct bt_mesh_gen_onoff_cli *goo_cli = model->user_data;
	struct gen_onoff_status_param param;
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_GEN_ONOFF_GET, 0);
	struct bt_mesh_msg_ctx ctx;

	model_cli_internal_get_msg_ctx(&goo_cli->cli, &ctx);
	ctx.addr = addr;

	bt_mesh_model_msg_init(&msg, OP_GEN_ONOFF_GET);

	model_cli_internal_prepare(&goo_cli->cli, (void **)&param, OP_GEN_ONOFF_STATUS);

	err = bt_mesh_model_send(model, &ctx, &msg, NULL, NULL);
	if (err) {
		BT_ERR("Unable to send Generic OnOff Get message.");
		model_cli_internal_reset(&goo_cli->cli);
		return err;
	}

	err = model_cli_internal_wait(&goo_cli->cli);
	if (present)
		*present = param.present_onoff;
	if (target)
		*target = param.target_onoff;
	if (remain)
		*remain = param.remain_time;

	return err;
}

/* Generic OnOff Client message handles */
static void gen_onoff_status(struct bt_mesh_model *model,
			     struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	struct bt_mesh_gen_onoff_cli *goo_cli = model->user_data;
	struct gen_onoff_status_param *param;

	if (buf->len != 1 && buf->len != 3) {
		BT_ERR("Invalid Generic OnOff Status length %d", buf->len);
		return ;
	}

	/* Publish message. */
	if (!BT_MESH_ADDR_IS_UNICAST(ctx->recv_dst)) {
		u8_t present_onoff, target_onoff;
		struct bt_mesh_transition_remain_time rt;

		present_onoff = net_buf_simple_pull_u8(buf);
		if (buf->len) {
			target_onoff = net_buf_simple_pull_u8(buf);
			rt.remain_time =
				transition_time_to_ms(net_buf_simple_pull_u8(buf));
			goo_cli->status_cb(model, present_onoff, target_onoff, &rt);
		} else {
			goo_cli->status_cb(model, present_onoff, present_onoff, NULL);
		}

		return;
	}

	/* Unicast message. */
	if (model_cli_internal_param_get(&goo_cli->cli,
		    (void **)&param, OP_GEN_ONOFF_STATUS) != 0) {
		BT_WARN("Unexpected Model Subscription Status message");
		return;
	}

	param->present_onoff = net_buf_simple_pull_u8(buf);
	if (buf->len) {
		param->target_onoff = net_buf_simple_pull_u8(buf);
		param->remain_time  = net_buf_simple_pull_u8(buf);
	} else {
		param->target_onoff = param->present_onoff;
		param->remain_time  = INSTANTANEOUS_TRANS_TIME;
	}

	model_cli_internal_release(&goo_cli->cli);
}

static int gen_onoff_cli_init(struct bt_mesh_model *model)
{
	struct bt_mesh_gen_onoff_cli *goo_cli;

	if (model->user_data != NULL) {
		goo_cli = model->user_data;

		if (model_cli_internal_init(&goo_cli->cli) != 0) {
			BT_ERR("Generic onoff client model create failed.");
			return -ENOMEM;
		}
	} else {
		goo_cli = k_malloc(sizeof(*goo_cli));
		if (goo_cli == NULL) {
			BT_ERR("Generic onoff client model create failed.");
			return -ENOMEM;
		}

		memset(goo_cli, 0, sizeof(*goo_cli));

		if (model_cli_internal_init(&goo_cli->cli) != 0) {
			k_free(goo_cli);
			BT_ERR("Generic onoff client model create failed.");
			return -ENOMEM;
		}

		model->user_data = goo_cli;
	}

	return 0;
}

/* Mapping of message handles for Generic OnOff Client*/
const struct bt_mesh_model_op gen_onoff_cli_op[] = {
	{ OP_GEN_ONOFF_STATUS, 1, gen_onoff_status },
	BT_MESH_MODEL_OP_END,
};

const struct bt_mesh_model_cb bt_mesh_gen_onoff_cb = {
	.init = gen_onoff_cli_init,
};

