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

#ifndef _GENERIC_ONOFF_SRV_H_
#define _GENERIC_ONOFF_SRV_H_

#include "ble/bluetooth/bluetooth.h"
#include "ble/bluetooth/mesh.h"
#include "bluetooth/mesh/transition.h"

extern const struct bt_mesh_model_cb bt_mesh_gen_onoff_srv_cb;
extern const struct bt_mesh_model_op gen_onoff_srv_op[];

/** @def BT_MESH_GEN_ONOFF_SRV_PUB_DEFINE
 *
 *  A helper to define a generic onoff server publication context
 *
 *  @param _name       Name given to the publication context variable.
 */
#define BT_MESH_GEN_ONOFF_SRV_PUB_DEFINE(_name) \
	BT_MESH_MODEL_PUB_DEFINE(_name, NULL, 2 + 3)


#define BT_MESH_MODEL_GEN_ONOFF_SRV(srv, pub)	\
		BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_GEN_ONOFF_SRV,\
					gen_onoff_srv_op, pub, srv, &bt_mesh_gen_onoff_srv_cb)

typedef void (*bt_mesh_gen_onoff_set_cb_t)(const struct bt_mesh_model *model,
                                 uint8_t present_onoff, uint8_t target_onoff,
                                 const struct bt_mesh_transition_status *opt);

typedef void (*bt_mesh_gen_onoff_get_cb_t)(const struct bt_mesh_model *model,
	                             uint8_t *onoff);

struct bt_mesh_gen_onoff_srv {
	struct transition transition;

	uint8_t  onoff;
	uint8_t  target_onoff;

	uint8_t  last_tid;
	uint16_t last_src_addr;
	uint16_t last_dst_addr;
	int64_t last_msg_timestamp;

	/* Callback to be called for informing the user application to update the value */
	bt_mesh_gen_onoff_set_cb_t set_cb;
	bt_mesh_gen_onoff_get_cb_t get_cb;
};

int bt_mesh_gen_onoff_srv_set(struct bt_mesh_model *model, uint8_t onoff,
	                          struct bt_mesh_transition_params *opt);

//void bt_mesh_gen_onoff_srv_publish(struct bt_mesh_model *model);

static inline void bt_mesh_gen_onoff_srv_set_cb(struct bt_mesh_model *model,
                                             bt_mesh_gen_onoff_set_cb_t set,
                                             bt_mesh_gen_onoff_get_cb_t get)
{
	((struct bt_mesh_gen_onoff_srv *)model->user_data)->set_cb = set;
	((struct bt_mesh_gen_onoff_srv *)model->user_data)->get_cb = get;
}

#endif
