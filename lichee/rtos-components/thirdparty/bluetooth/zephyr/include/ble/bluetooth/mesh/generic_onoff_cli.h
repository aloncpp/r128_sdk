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

#ifndef _GENERIC_ONOFF_CLI_H_
#define _GENERIC_ONOFF_CLI_H_

#include "ble/bluetooth/bluetooth.h"
#include "ble/bluetooth/mesh.h"
#include "bluetooth/mesh/mesh_common.h"
#include "bluetooth/mesh/transition.h"

/** @def BT_MESH_GEN_ONOFF_CLI_PUB_DEFINE
 *
 *  A helper to define a generic onoff server publication context
 *
 *  @param _name       Name given to the publication context variable.
 */
#define BT_MESH_GEN_ONOFF_CLI_PUB_DEFINE(_name) \
	BT_MESH_MODEL_PUB_DEFINE(_name, NULL, 2 + 4)


#define BT_MESH_MODEL_GEN_ONOFF_CLI(cli, pub)	\
		BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_GEN_ONOFF_CLI, 	\
					gen_onoff_cli_op, pub, cli, &bt_mesh_gen_onoff_cb)

typedef void (*bt_mesh_gen_onoff_cli_status_cb_t)(
	const struct bt_mesh_model *model, uint8_t present,
	uint8_t target, const struct bt_mesh_transition_remain_time *opt);

struct bt_mesh_gen_onoff_cli {
	struct model_cli_internal cli;
	uint8_t tid;

	bt_mesh_gen_onoff_cli_status_cb_t status_cb;
};

extern struct bt_mesh_model *onoff_cli_model;

/*These funcition are called to set/get generic states*/
int bt_mesh_gen_onoff_cli_set(struct bt_mesh_model *model, uint16_t addr,
                          uint8_t onoff, struct bt_mesh_transition_params *opt);

int bt_mesh_gen_onoff_cli_set_unack(struct bt_mesh_model *model, uint16_t addr,
	                      uint8_t onoff, struct bt_mesh_transition_params *opt);

int bt_mesh_gen_onoff_cli_get(struct bt_mesh_model *model, uint16_t addr,
	                      uint8_t *present, uint8_t *target, uint8_t *remain);

static inline int bt_mesh_gen_onoff_cli_set_status_cb(
	         struct bt_mesh_model *model, bt_mesh_gen_onoff_cli_status_cb_t cb)
{
	if (!model->user_data)
		return -EINVAL;
}

static inline void bt_mesh_gen_onoff_cli_set_net_idx(
	                      struct bt_mesh_model *model, uint16_t net_idx)
{
	model_cli_internal_set_net_idx(model->user_data, net_idx);
}

static inline void bt_mesh_gen_onoff_cli_set_app_idx(
	                      struct bt_mesh_model *model, uint16_t app_idx)
{
	model_cli_internal_set_app_idx(model->user_data, app_idx);
}

static inline void bt_mesh_gen_onoff_cli_set_send_rel(
	                      struct bt_mesh_model *model, bool send_rel)
{
	model_cli_internal_set_send_rel(model->user_data, send_rel);
}

static inline void bt_mesh_gen_onoff_cli_set_ttl(
	                      struct bt_mesh_model *model, uint8_t send_ttl)
{
	model_cli_internal_set_ttl(model->user_data, send_ttl);
}

static inline void bt_mesh_gen_onoff_cli_set_timeout(
	                      struct bt_mesh_model *model, int32_t timeout)
{
	model_cli_internal_set_timeout(model->user_data, timeout);
}

extern const struct bt_mesh_model_op gen_onoff_cli_op[];
extern const struct bt_mesh_model_cb bt_mesh_gen_onoff_cb;

#endif
