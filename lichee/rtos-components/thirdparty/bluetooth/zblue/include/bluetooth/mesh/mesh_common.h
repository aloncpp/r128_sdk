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

#ifndef _MESH_COMMON_H_
#define _MESH_COMMON_H_

#include "ble/bluetooth/bluetooth.h"
#include "ble/bluetooth/mesh.h"

/* Model Operation Codes */

/* Generic OnOff Message Opcode */
#define OP_GEN_ONOFF_GET            BT_MESH_MODEL_OP_2(0x82, 0x01)
#define OP_GEN_ONOFF_SET            BT_MESH_MODEL_OP_2(0x82, 0x02)
#define OP_GEN_ONOFF_SET_UNACK	    BT_MESH_MODEL_OP_2(0x82, 0x03)
#define OP_GEN_ONOFF_STATUS		    BT_MESH_MODEL_OP_2(0x82, 0x04)

struct model_cli_internal {
	/* Internal parameters for tracking message responses. */
	/** NetKey Index of the subnet to send the message on. */
	u16_t net_idx;

	/** AppKey Index to encrypt the message with. */
	u16_t app_idx;

	/** Force sending reliably by using segment acknowledgement */
	bool  send_rel;

	/** TTL, BT_MESH_TTL_DEFAULT for default TTL. */
	u8_t  send_ttl;

	struct k_sem          op_sync;
	u32_t                 op_pending;
	void                 *op_param;
	s32_t                 msg_timeout;
};

void model_cli_internal_get_msg_ctx(struct model_cli_internal *cli,
                struct bt_mesh_msg_ctx *ctx);

static inline void model_cli_internal_set_net_idx(
                struct model_cli_internal *cli, u16_t net_idx)
{
	cli->net_idx = net_idx;
}

static inline void model_cli_internal_set_app_idx(
                struct model_cli_internal *cli, u16_t app_idx)
{
	cli->app_idx = app_idx;
}

static inline void model_cli_internal_set_send_rel(
                struct model_cli_internal *cli, bool send_rel)
{
	cli->send_rel = send_rel;
}

static inline void model_cli_internal_set_ttl(
                struct model_cli_internal *cli, u8_t send_ttl)
{
	cli->send_ttl = send_ttl;
}

void model_cli_internal_release(struct model_cli_internal *cli);

int model_cli_internal_param_get(struct model_cli_internal *cli,
	            void **param, u32_t op);

int model_cli_internal_prepare(struct model_cli_internal *cli,
	            void *param, u32_t op);

void model_cli_internal_reset(struct model_cli_internal *cli);

int model_cli_internal_wait(struct model_cli_internal *cli);

static inline s32_t model_cli_internal_get_timeout(
                struct model_cli_internal *cli)
{
	return cli->msg_timeout;
}

static inline void model_cli_internal_set_timeout(
                struct model_cli_internal *cli, s32_t timeout)
{
	cli->msg_timeout = timeout;
}

int model_cli_internal_init(struct model_cli_internal *cli);


#endif
