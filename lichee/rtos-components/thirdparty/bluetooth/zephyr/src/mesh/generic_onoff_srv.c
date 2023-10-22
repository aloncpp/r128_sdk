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

#include "sys/defs.h"
#include "ble/bluetooth/bluetooth.h"
#include "ble/bluetooth/mesh.h"
#include "ble/bluetooth/mesh/generic_onoff_srv.h"
#include "ble/bluetooth/mesh/mesh_common.h"
#include "ble/bluetooth/mesh/transition.h"
#include "common/log.h"
#include "mesh.h"


#define GEN_ONOFF_OFF (0x00)
#define GEN_ONOFF_ON  (0x01)

uint16_t model_find_available_key(struct bt_mesh_model *mod)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(mod->keys); i++) {
        if (mod->keys[i] != BT_MESH_KEY_UNUSED)
            return mod->keys[i];
	}

	return BT_MESH_KEY_UNUSED;
}


int bt_mesh_gen_onoff_srv_set(struct bt_mesh_model *model,
	                         uint8_t onoff, struct bt_mesh_transition_params *opt)
{
	int err;
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_GEN_ONOFF_SET_UNACK, 4);
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = BT_MESH_KEY_ANY,
		.app_idx = model_find_available_key(model),
		.addr = bt_mesh_model_elem(model)->addr,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};

	bt_mesh_model_msg_init(&msg, OP_GEN_ONOFF_SET_UNACK);

	net_buf_simple_add_u8(&msg, onoff);
	net_buf_simple_add_u8(&msg, onoff); //avoid same tid

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

/* Generic OnOff Server message handles */
void gen_onoff_publish(struct bt_mesh_model *model)
{
	int err;
	uint8_t rt;
	uint8_t onoff;
	struct net_buf_simple *msg = model->pub->msg;
	struct bt_mesh_gen_onoff_srv *srv = model->user_data;

	if (model->pub->addr == BT_MESH_ADDR_UNASSIGNED) {
		return;
	}

	srv->get_cb(model, &onoff);
	if (onoff > GEN_ONOFF_ON) {
		BT_ERR("onoff value should be bool %d\n", onoff);
		return;
	}

	bt_mesh_model_msg_init(msg, OP_GEN_ONOFF_STATUS);
	net_buf_simple_add_u8(msg, onoff);

	if (transition_is_started(&srv->transition)) {
		transition_get_remain_time(&srv->transition, &rt);
		net_buf_simple_add_u8(msg, srv->target_onoff);
		net_buf_simple_add_u8(msg, rt);
	}

	err = bt_mesh_model_publish(model);
	if (err) {
		BT_ERR("bt_mesh_model_publish err %d\n", err);
	}

	return;
}

/* Opcode handle */
static void gen_onoff_get(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 3 + 4);
	struct bt_mesh_gen_onoff_srv *srv = model->user_data;
	uint8_t onoff;
	uint8_t rt;

	srv->get_cb(model, &onoff);
	if (onoff > GEN_ONOFF_ON) {
		BT_ERR("onoff value should be bool %d\n", onoff);
		return;
	}

	bt_mesh_model_msg_init(msg, OP_GEN_ONOFF_STATUS);
	net_buf_simple_add_u8(msg, onoff);

	if (transition_is_started(&srv->transition)) {
		transition_get_remain_time(&srv->transition, &rt);
		net_buf_simple_add_u8(msg, srv->target_onoff);
		net_buf_simple_add_u8(msg, rt);
	}

	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		printf("Unable to send GEN_ONOFF_SRV Status response\n");
	}
}

static void gen_onoff_set_unack(struct bt_mesh_model *model,
				struct bt_mesh_msg_ctx *ctx,
				struct net_buf_simple *buf)
{
	uint8_t tid, target_onoff, trans_time, delay;
	int64_t now;
	struct bt_mesh_gen_onoff_srv *srv = model->user_data;

	target_onoff = net_buf_simple_pull_u8(buf);
	tid = net_buf_simple_pull_u8(buf);

	if (target_onoff > GEN_ONOFF_ON) {
		BT_WARN("onoff value should be bool %d\n", target_onoff);
		return;
	}

	now = k_uptime_get();
	if (srv->last_tid == tid &&
	    srv->last_src_addr == ctx->addr &&
	    srv->last_dst_addr == ctx->recv_dst &&
	    (now - srv->last_msg_timestamp <= K_SECONDS(6))) {
		return;
	}

	switch (buf->len) {
	case 0x00:	/* No optional fields are available */
		trans_time = INSTANTANEOUS_TRANS_TIME;
		delay = 0U;
		break;
	case 0x02:	/* Optional fields are available */
		trans_time = net_buf_simple_pull_u8(buf);
		if ((trans_time & 0x3F) == 0x3F) {
			return;
		}
		delay = net_buf_simple_pull_u8(buf);
		break;
	default:
		return;
	}

	transition_stop(&srv->transition);

	srv->get_cb(model, &srv->onoff);
	if (srv->onoff > GEN_ONOFF_ON) {
		BT_ERR("onoff value should be bool %d\n", srv->onoff);
		return;
	}

	srv->last_tid 				= tid;
	srv->last_src_addr 			= ctx->addr;
	srv->last_dst_addr 			= ctx->recv_dst;
	srv->last_msg_timestamp 	= now;
	srv->target_onoff 			= target_onoff;

	if (srv->target_onoff == srv->onoff) {
		gen_onoff_publish(model);
		return;
	}

	/* For Instantaneous Transition */
	if (transition_is_instantaneous(&srv->transition, trans_time)) {
		srv->onoff = srv->target_onoff;
		srv->set_cb(model, srv->onoff, srv->target_onoff, NULL);
	} else {
		transition_prepare(&srv->transition, trans_time, delay);
		transition_start(&srv->transition);
	}

	gen_onoff_publish(model);
}

static void gen_onoff_set(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	uint8_t tid, target_onoff, trans_time, delay;
	/*
		tid	 	Transaction Identifier
		onoff 	The target value of the Generic OnOff state
		tt 		Transition Time
		delay 	Message execution delay in 5 millisecond steps
	*/
	int64_t now;
	struct bt_mesh_gen_onoff_srv *srv = model->user_data;

	target_onoff = net_buf_simple_pull_u8(buf);
	tid = net_buf_simple_pull_u8(buf);

	if (target_onoff > GEN_ONOFF_ON) {
		BT_WARN("onoff value should be bool %d\n", target_onoff);
		return;
	}

	now = k_uptime_get();
	if (srv->last_tid == tid &&
	    srv->last_src_addr == ctx->addr &&
	    srv->last_dst_addr == ctx->recv_dst &&
	    (now - srv->last_msg_timestamp <= K_SECONDS(6))) {
	    gen_onoff_get(model, ctx, buf);
		return;
	}

	switch (buf->len) {
	case 0x00:	/* No optional fields are available */
		trans_time = INSTANTANEOUS_TRANS_TIME;
		delay = 0U;
		break;
	case 0x02:	/* Optional fields are available */
		trans_time = net_buf_simple_pull_u8(buf);
		if ((trans_time & 0x3F) == 0x3F) {
			return;
		}
		delay = net_buf_simple_pull_u8(buf);
		break;
	default:
		return;
	}

	transition_stop(&srv->transition);

	srv->get_cb(model, &srv->onoff);
    if (srv->onoff > GEN_ONOFF_ON) {
		BT_WARN("onoff value should be bool %d\n", srv->onoff);
		return;
	}

	srv->last_tid 				= tid;
	srv->last_src_addr 			= ctx->addr;
	srv->last_dst_addr 			= ctx->recv_dst;
	srv->last_msg_timestamp 	= now;
	srv->target_onoff 			= target_onoff;

	if (srv->target_onoff == srv->onoff) {
        gen_onoff_get(model, ctx, buf);
		gen_onoff_publish(model);
		return;
	}

	/* For Instantaneous Transition */
	if (transition_is_instantaneous(&srv->transition, trans_time)) {
		srv->onoff = srv->target_onoff;
		srv->set_cb(model, srv->onoff, srv->target_onoff, NULL);
	} else {
		transition_prepare(&srv->transition, trans_time, delay);
		transition_start(&srv->transition);
	}

	gen_onoff_get(model, ctx, buf);
	gen_onoff_publish(model);
}

/* Mapping of message handles for Generic OnOff Server(0x1000)*/
const struct bt_mesh_model_op gen_onoff_srv_op[] = {
	{ OP_GEN_ONOFF_GET, 		0, gen_onoff_get },
	{ OP_GEN_ONOFF_SET, 		2, gen_onoff_set },
	{ OP_GEN_ONOFF_SET_UNACK,   2, gen_onoff_set_unack },
	BT_MESH_MODEL_OP_END,
};

static void gen_onoff_transition_cb(struct transition *tt, uint32_t total,
	                              uint32_t steps, void *arg)
{
	struct bt_mesh_model *model = arg;
	struct bt_mesh_gen_onoff_srv *srv = model->user_data;
	struct bt_mesh_transition_status param = {
		.total_steps = total,
		.present_steps = steps,
	};

	if (total == steps) {
		srv->onoff = srv->target_onoff;
		srv->set_cb(model, srv->onoff, srv->target_onoff, NULL);
		gen_onoff_publish(model);
		printf("!set present(%d), target(%d)\n", srv->onoff, srv->target_onoff);
	} else {
    	printf("set present(%d), target(%d)\n", srv->onoff, srv->target_onoff);(void)param;
		srv->set_cb(model, srv->onoff, srv->target_onoff, &param);
	}
}

static int gen_onoff_srv_init(struct bt_mesh_model *model)
{
	struct bt_mesh_gen_onoff_srv *srv = model->user_data;

	return transition_init(&srv->transition, gen_onoff_transition_cb, (void *)model);
}

const struct bt_mesh_model_cb bt_mesh_gen_onoff_srv_cb = {
	.init = (void *)gen_onoff_srv_init,
};
