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
#include <zephyr.h>
#include "kernel/os/os.h"

#include "ble/bluetooth/bluetooth.h"
#include "ble/bluetooth/mesh.h"
#include "ble/bluetooth/mesh/transition.h"

#define OS_TIMER_PERIODIC XR_OS_TIMER_PERIODIC
#define OS_OK XR_OS_OK
#define OS_WAIT_FOREVER (uint32_t)XR_OS_WAIT_FOREVER
#define OS_TimerIsActive XR_OS_TimerIsActive
#define OS_TimerChangePeriod XR_OS_TimerChangePeriod
#define OS_TimerCreate XR_OS_TimerCreate
#define OS_TimerStop XR_OS_TimerStop
#define OS_TimerStart XR_OS_TimerStart
#define OS_TimerDelete XR_OS_TimerDelete

uint8_t transition_ms_to_time(uint32_t ms)
{
	uint8_t steps, resolution;
	uint32_t duration_remainder = ms;

	if (duration_remainder > 620000) {
		/* > 620 seconds -> resolution = 0b11 [10 minutes] */
		resolution = 0x03;
		steps = duration_remainder / 600000;
	} else if (duration_remainder > 62000) {
		/* > 62 seconds -> resolution = 0b10 [10 seconds] */
		resolution = 0x02;
		steps = duration_remainder / 10000;
	} else if (duration_remainder > 6200) {
		/* > 6.2 seconds -> resolution = 0b01 [1 seconds] */
		resolution = 0x01;
		steps = duration_remainder / 1000;
	} else if (duration_remainder > 0) {
		/* <= 6.2 seconds -> resolution = 0b00 [100 ms] */
		resolution = 0x00;
		steps = duration_remainder / 100;
	} else {
		resolution = 0x00;
		steps = 0x00;
	}

	return (resolution << 6) | steps;
}


/* Function to calculate Remaining Time (Start) */
void transition_get_remain_time(struct transition *tt, uint8_t *rt)
{
	uint32_t duration_remainder;
	int64_t now;

	if (tt->stage == TRANSITION_STAGES_STARTED) {
		*rt = tt->trans_time;
	} else if (tt->stage == TRANSITION_STAGES_DELAYED) {
		now = k_uptime_get();
		duration_remainder = tt->total * tt->quo_tt -
				     (now - tt->start_timestamp);

		*rt = transition_ms_to_time(duration_remainder);
	} else {
		*rt = 0;
	}
}

/*
 * Define in Mesh_Model_Specification v1.0 chapter 3.1.3.1.
 */
static const uint32_t transition_step_resolution_in_ms[] = {100U, 1000U, 10000U, 600000U};

uint32_t transition_time_to_ms(uint8_t trans_time)
{
	uint8_t steps_multiplier, resolution;

	resolution = (trans_time >> 6);
	return steps_multiplier = (trans_time & 0x3F) * transition_step_resolution_in_ms[resolution];
}

int transition_prepare(struct transition *tt, uint8_t trans_time, uint8_t delay)
{
	uint8_t steps_multiplier, resolution;

	tt->trans_time = trans_time;
	tt->delay = delay;

	resolution = (trans_time >> 6);
	steps_multiplier = (trans_time & 0x3F);
	if (steps_multiplier == 0U || steps_multiplier == 0x3F) {
		return -EINVAL;
	}

	tt->quo_tt = transition_step_resolution_in_ms[resolution];

	if (tt->priv_counter) {
		tt->counter = tt->priv_counter;
		tt->quo_tt = steps_multiplier * tt->quo_tt / tt->counter;
	} else {
		tt->counter = steps_multiplier;
	}

    printf("tt->counter %d, tt->quo_tt %d\n", tt->counter, tt->quo_tt);

	tt->total = tt->counter;

	return 0;
}

static void transition_timeout(void *arg)
{
	struct transition *tt = arg;
	if (tt == NULL)
		return;

	if (tt->stage == TRANSITION_STAGES_STARTED) {
		OS_TimerChangePeriod(&tt->timer, tt->quo_tt);
		tt->stage = TRANSITION_STAGES_DELAYED;
	}

	tt->cb(tt, tt->total, tt->total - tt->counter, tt->arg);

	if (tt->counter > 0) {
		tt->counter--;
	} else {
		tt->stage = TRANSITION_STAGES_FINISHED;
		transition_stop(tt);
	}
}

void transition_stop(struct transition *tt)
{
	if (OS_TimerIsActive(&tt->timer))
		OS_TimerStop(&tt->timer);
	tt->stage = TRANSITION_STAGES_IDLE;
}

int transition_start(struct transition *tt)
{
	if (OS_TimerIsActive(&tt->timer))
		return -EINVAL;

	tt->stage = TRANSITION_STAGES_STARTED;

	if (tt->delay) {
		OS_TimerChangePeriod(&tt->timer, tt->delay * 5U);
	} else {
		transition_timeout(tt);
	}

	OS_TimerStart(&tt->timer);

	return 0;
}

int transition_init(struct transition *tt, transition_cb_t cb, void *arg)
{
	tt->stage = TRANSITION_STAGES_IDLE;
	tt->priv_counter = 0;
	tt->cb = cb;
	tt->arg = arg;

	if (OS_TimerCreate(&tt->timer, OS_TIMER_PERIODIC, transition_timeout, (void*)tt, OS_WAIT_FOREVER) != OS_OK)
		return -ENOMEM;

	return 0;
}

void transition_deinit(struct transition *tt)
{
	OS_TimerDelete(&tt->timer);
}

