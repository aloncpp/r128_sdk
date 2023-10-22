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

#ifndef _TRANSITION_H_
#define _TRANSITION_H_

#include "zephyr.h"

#include "ble/bluetooth/bluetooth.h"
#include "ble/bluetooth/mesh.h"

/**
 * According to Mesh Model Spec:
 * If the Transition Time field is not present and the Generic Default Transition
 * Time state is supported, the Generic Default Transition Time state shall be
 * used. Otherwise the transition shall be instantaneous.
 */
#define INSTANTANEOUS_TRANS_TIME      	0
//#define DEVICE_SPECIFIC_RESOLUTION 		10

struct transition;

struct bt_mesh_transition_params {
	uint8_t trans_time;	//transition time
	uint8_t delay;			//message execution delay in 5 millisecond steps
};

struct bt_mesh_transition_status {
	uint32_t total_steps;
	uint32_t present_steps;
};

struct bt_mesh_transition_remain_time {
	uint32_t remain_time;	//in millisecond
};

typedef void (*transition_cb_t)(struct transition *tt, uint32_t total, uint32_t steps, void *arg);

enum transition_stages {
	TRANSITION_STAGES_IDLE,
	TRANSITION_STAGES_STARTED,
	TRANSITION_STAGES_DELAYED,
	TRANSITION_STAGES_FINISHED,
};

struct transition {
	uint8_t stage;
	uint8_t trans_time;
	//uint8_t remain_time;			//Remaining Time
	uint8_t delay;
	uint16_t counter;
	uint16_t priv_counter;
	uint32_t total;
	uint32_t quo_tt;
	//uint32_t total_duration;
	int64_t start_timestamp;

	transition_cb_t cb;
	void *arg;

	OS_Timer_t timer;
};

int transition_init(struct transition *tt, transition_cb_t cb, void *arg);

void transition_deinit(struct transition *tt);

static inline void transition_set_cb(struct transition *tt, transition_cb_t cb)
{
	tt->cb = cb;
}

void transition_get_remain_time(struct transition *tt, uint8_t *rt);

uint32_t transition_time_to_ms(uint8_t trans_time);

uint8_t transition_ms_to_time(uint32_t ms);

static inline bool transition_is_instantaneous(struct transition *tt, uint8_t trans_time)
{
	return (trans_time & 0x3F) == 0;
}

static inline bool transition_is_started(struct transition *tt)
{
	return (tt->stage == TRANSITION_STAGES_STARTED) || (tt->stage == TRANSITION_STAGES_DELAYED);
}

int transition_prepare(struct transition *tt, uint8_t trans_time, uint8_t delay);

void transition_stop(struct transition *tt);

int transition_start(struct transition *tt);

#endif
