/******************************************************************************
 *
 *  Copyright (C) 2014 Google, Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "common/bt_defs.h"
#include "common/bt_trace.h"
#include "osi/alarm.h"
#include "osi/allocator.h"
#include "osi/list.h"
//#include "xr_timer.h"
#include "btc/btc_task.h"
#include "btc/btc_alarm.h"
#include "osi/mutex.h"

#include "kernel/os/os.h"

#ifdef CONFIG_COMPONENTS_BT_PM
#include "bt_pm.h"
#endif

#define OS_Timer_t XR_OS_Timer_t
#define OS_Time_t XR_OS_Time_t
#define OS_Status XR_OS_Status
#define OS_TIMER_ONCE XR_OS_TIMER_ONCE
#define OS_OK XR_OS_OK
#define OS_FAIL XR_OS_FAIL
#define OS_TIMER_PERIODIC XR_OS_TIMER_PERIODIC
#define OS_GetTicks XR_OS_GetTicks
#define OS_TicksToMSecs XR_OS_TicksToMSecs
#define OS_TimerStart XR_OS_TimerStart
#define OS_TimerChangePeriod XR_OS_TimerChangePeriod
#define OS_TimerDelete XR_OS_TimerDelete
#define OS_TimerStop XR_OS_TimerStop
#define OS_TimerCreate XR_OS_TimerCreate
#define OS_TimerIsValid XR_OS_TimerIsValid
#define OS_TimerIsActive XR_OS_TimerIsActive

typedef struct alarm_t {
    /* timer id point to here */
    OS_Timer_t alarm_hdl;
    osi_alarm_callback_t cb;
    void *cb_data;
    OS_Time_t deadline_ms;
#if 0
    int64_t deadline_us;
#endif
} osi_alarm_t;

enum {
    ALARM_STATE_IDLE,
    ALARM_STATE_OPEN,
};

static osi_mutex_t alarm_mutex;
static int alarm_state;

static struct alarm_t alarm_cbs[ALARM_CBS_NUM];

static osi_alarm_err_t alarm_free(osi_alarm_t *alarm);
static osi_alarm_err_t alarm_set(osi_alarm_t *alarm, period_ms_t timeout, bool is_periodic);

int osi_alarm_create_mux(void)
{
    if (alarm_state != ALARM_STATE_IDLE) {
        OSI_TRACE_WARNING("%s, invalid state %d\n", __func__, alarm_state);
        return -1;
    }
    osi_mutex_new(&alarm_mutex);
    return 0;
}

int osi_alarm_delete_mux(void)
{
    if (alarm_state != ALARM_STATE_IDLE) {
        OSI_TRACE_WARNING("%s, invalid state %d\n", __func__, alarm_state);
        return -1;
    }
    osi_mutex_free(&alarm_mutex);
    return 0;
}

void osi_alarm_init(void)
{
    assert(osi_mutex_is_valid(&alarm_mutex));

    osi_mutex_lock(&alarm_mutex, OSI_MUTEX_MAX_TIMEOUT);
    if (alarm_state != ALARM_STATE_IDLE) {
        OSI_TRACE_WARNING("%s, invalid state %d\n", __func__, alarm_state);
        goto end;
    }
    memset(alarm_cbs, 0x00, sizeof(alarm_cbs));
    alarm_state = ALARM_STATE_OPEN;

end:
    osi_mutex_unlock(&alarm_mutex);
}

void osi_alarm_deinit(void)
{
    assert(osi_mutex_is_valid(&alarm_mutex));

    osi_mutex_lock(&alarm_mutex, OSI_MUTEX_MAX_TIMEOUT);
    if (alarm_state != ALARM_STATE_OPEN) {
        OSI_TRACE_WARNING("%s, invalid state %d\n", __func__, alarm_state);
        goto end;
    }

    for (int i = 0; i < ALARM_CBS_NUM; i++) {
        if (OS_TimerIsValid(&alarm_cbs[i].alarm_hdl)) {
            alarm_free(&alarm_cbs[i]);
        }
    }
    alarm_state = ALARM_STATE_IDLE;

end:
    osi_mutex_unlock(&alarm_mutex);
}

static struct alarm_t *alarm_cbs_lookfor_available(void)
{
    int i;

    for (i = 0; i < ALARM_CBS_NUM; i++) {
        if (!OS_TimerIsValid(&alarm_cbs[i].alarm_hdl)) { //available = not used
            OSI_TRACE_DEBUG("%s %d %p\n", __func__, i, &alarm_cbs[i]);
            return &alarm_cbs[i];
        }
    }

    return NULL;
}

static void alarm_cb_handler(struct alarm_t *alarm)
{
    OSI_TRACE_DEBUG("TimerID %p\n", alarm);
    if (alarm_state != ALARM_STATE_OPEN) {
        OSI_TRACE_WARNING("%s, invalid state %d\n", __func__, alarm_state);
        return;
    }
    btc_msg_t msg;
    btc_alarm_args_t arg;
    msg.sig = BTC_SIG_API_CALL;
    msg.pid = BTC_PID_ALARM;
    arg.cb = alarm->cb;
    arg.cb_data = alarm->cb_data;
    btc_transfer_context(&msg, &arg, sizeof(btc_alarm_args_t), NULL);
}

osi_alarm_t *osi_alarm_new(const char *alarm_name, osi_alarm_callback_t callback, void *data, period_ms_t timer_expire)
{
    assert(osi_mutex_is_valid(&alarm_mutex));

    struct alarm_t *timer_id = NULL;

    osi_mutex_lock(&alarm_mutex, OSI_MUTEX_MAX_TIMEOUT);
    if (alarm_state != ALARM_STATE_OPEN) {
        OSI_TRACE_ERROR("%s, invalid state %d\n", __func__, alarm_state);
        timer_id = NULL;
        goto end;
    }

    timer_id = alarm_cbs_lookfor_available();

    if (!timer_id) {
        OSI_TRACE_ERROR("%s alarm_cbs exhausted\n", __func__);
        timer_id = NULL;
        goto end;
    }

    OS_Status ret = OS_TimerCreate(&timer_id->alarm_hdl, OS_TIMER_ONCE, callback, data, 10000/* whatever */);
    if (ret != OS_OK) {
        OSI_TRACE_ERROR("%s failed to create timer, err 0x%x\n", __func__, ret);
        timer_id = NULL;
        goto end;
    }

    timer_id->cb = callback;
    timer_id->cb_data = data;
    timer_id->deadline_ms = 0;

#if 0
    xr_timer_create_args_t tca;
    tca.callback = (xr_timer_cb_t)alarm_cb_handler;
    tca.arg = timer_id;
    tca.dispatch_method = XRADIO_TIMER_TASK;
    tca.name = alarm_name;

    xr_err_t stat = xr_timer_create(&tca, &timer_id->alarm_hdl);
    if (stat != XRADIO_OK) {
        OSI_TRACE_ERROR("%s failed to create timer, err 0x%x\n", __func__, stat);
        timer_id = NULL;
        goto end;
    }
#endif

end:
    osi_mutex_unlock(&alarm_mutex);
    return timer_id;
}

osi_alarm_t *osi_alarm_periodic_new(const char *alarm_name, osi_alarm_callback_t callback, void *data, period_ms_t timer_expire)
{
    assert(osi_mutex_is_valid(&alarm_mutex));

    struct alarm_t *timer_id = NULL;

    osi_mutex_lock(&alarm_mutex, OSI_MUTEX_MAX_TIMEOUT);
    if (alarm_state != ALARM_STATE_OPEN) {
        OSI_TRACE_ERROR("%s, invalid state %d\n", __func__, alarm_state);
        timer_id = NULL;
        goto end;
    }

    timer_id = alarm_cbs_lookfor_available();

    if (!timer_id) {
        OSI_TRACE_ERROR("%s alarm_cbs exhausted\n", __func__);
        timer_id = NULL;
        goto end;
    }

    OS_Status ret = OS_TimerCreate(&timer_id->alarm_hdl, OS_TIMER_PERIODIC, callback, data, 10000/* whatever */);
    if (ret != OS_OK) {
        OSI_TRACE_ERROR("%s failed to create timer, err 0x%x\n", __func__, ret);
        timer_id = NULL;
        goto end;
    }

    timer_id->cb = callback;
    timer_id->cb_data = data;
    timer_id->deadline_ms = 0;

#if 0
    xr_timer_create_args_t tca;
    tca.callback = (xr_timer_cb_t)alarm_cb_handler;
    tca.arg = timer_id;
    tca.dispatch_method = XRADIO_TIMER_TASK;
    tca.name = alarm_name;

    xr_err_t stat = xr_timer_create(&tca, &timer_id->alarm_hdl);
    if (stat != XRADIO_OK) {
        OSI_TRACE_ERROR("%s failed to create timer, err 0x%x\n", __func__, stat);
        timer_id = NULL;
        goto end;
    }
#endif

end:
    osi_mutex_unlock(&alarm_mutex);
    return timer_id;
}


static osi_alarm_err_t alarm_free(osi_alarm_t *alarm)
{
    if (!alarm || !OS_TimerIsValid(&alarm->alarm_hdl)) {
        OSI_TRACE_ERROR("%s null\n", __func__);
        return OSI_ALARM_ERR_INVALID_ARG;
    }

    OS_TimerStop(&alarm->alarm_hdl);
    OS_Status ret = OS_TimerDelete(&alarm->alarm_hdl);
    if (ret != OS_OK) {
        OSI_TRACE_ERROR("%s failed to delete timer, err 0x%x\n", __func__, ret);
        return OSI_ALARM_ERR_FAIL;
    }

#if 0
    xr_timer_stop(alarm->alarm_hdl);
    xr_err_t stat = xr_timer_delete(alarm->alarm_hdl);
    if (stat != XRADIO_OK) {
        OSI_TRACE_ERROR("%s failed to delete timer, err 0x%x\n", __func__, stat);
        return OSI_ALARM_ERR_FAIL;
    }
#endif

    memset(alarm, 0, sizeof(osi_alarm_t));
    return OSI_ALARM_ERR_PASS;
}

void osi_alarm_free(osi_alarm_t *alarm)
{
    assert(osi_mutex_is_valid(&alarm_mutex));

    osi_mutex_lock(&alarm_mutex, OSI_MUTEX_MAX_TIMEOUT);
    if (alarm_state != ALARM_STATE_OPEN) {
        OSI_TRACE_ERROR("%s, invalid state %d\n", __func__, alarm_state);
        goto end;
    }
    alarm_free(alarm);

end:
    osi_mutex_unlock(&alarm_mutex);
    return;
}

static osi_alarm_err_t alarm_set(osi_alarm_t *alarm, period_ms_t timeout, bool is_periodic)
{
    assert(osi_mutex_is_valid(&alarm_mutex));

    osi_alarm_err_t ret = OSI_ALARM_ERR_PASS;
    osi_mutex_lock(&alarm_mutex, OSI_MUTEX_MAX_TIMEOUT);
    if (alarm_state != ALARM_STATE_OPEN) {
        OSI_TRACE_ERROR("%s, invalid state %d\n", __func__, alarm_state);
        ret = OSI_ALARM_ERR_INVALID_STATE;
        goto end;
    }

    if (!alarm || !OS_TimerIsValid(&alarm->alarm_hdl)) {
        OSI_TRACE_ERROR("%s null\n", __func__);
        ret = OSI_ALARM_ERR_INVALID_ARG;
        goto end;
    }

#if 0
    int64_t timeout_us = 1000 * (int64_t)timeout;
#endif

    OS_Status stat;
    if (is_periodic) {
#if 0
        stat = xr_timer_start_periodic(alarm->alarm_hdl, (uint64_t)timeout_us);
#else
        OSI_TRACE_ERROR("not valid now!\n");
        stat = OS_FAIL;
#endif
    } else {
    	OS_TimerChangePeriod(&alarm->alarm_hdl, timeout);
    	stat = OS_TimerStart(&alarm->alarm_hdl);
#if 0
        stat = xr_timer_start_once(alarm->alarm_hdl, (uint64_t)timeout_us);
#endif
    }
    if (stat != OS_OK) {
        OSI_TRACE_ERROR("%s failed to start timer, err 0x%x\n", __func__, stat);
        ret = OSI_ALARM_ERR_FAIL;
        goto end;
    }

    alarm->deadline_ms = is_periodic ? 0 : (timeout + OS_TicksToMSecs(OS_GetTicks()));
#if 0
    alarm->deadline_us = is_periodic ? 0 : (timeout_us + xr_timer_get_time());
#endif

end:
    osi_mutex_unlock(&alarm_mutex);
    return ret;
}

osi_alarm_err_t osi_alarm_set(osi_alarm_t *alarm, period_ms_t timeout)
{
    return alarm_set(alarm, timeout, false);
}

osi_alarm_err_t osi_alarm_set_periodic(osi_alarm_t *alarm, period_ms_t period)
{
    return alarm_set(alarm, period, true);
}

osi_alarm_err_t osi_alarm_cancel(osi_alarm_t *alarm)
{
    int ret = OSI_ALARM_ERR_PASS;
    osi_mutex_lock(&alarm_mutex, OSI_MUTEX_MAX_TIMEOUT);
    if (alarm_state != ALARM_STATE_OPEN) {
        OSI_TRACE_ERROR("%s, invalid state %d\n", __func__, alarm_state);
        ret = OSI_ALARM_ERR_INVALID_STATE;
        goto end;
    }

    if (!alarm || !OS_TimerIsValid(&alarm->alarm_hdl)) {
        OSI_TRACE_ERROR("%s null\n", __func__);
        ret = OSI_ALARM_ERR_INVALID_ARG;
        goto end;
    }

    OS_Status stat = OS_TimerStop(&alarm->alarm_hdl);
    if (stat != OS_OK) {
        OSI_TRACE_DEBUG("%s failed to stop timer, err 0x%x\n", __func__, stat);
        ret = OSI_ALARM_ERR_FAIL;
        goto end;
    }

#if 0
    xr_err_t stat = xr_timer_stop(alarm->alarm_hdl);
    if (stat != XRADIO_OK) {
        OSI_TRACE_DEBUG("%s failed to stop timer, err 0x%x\n", __func__, stat);
        ret = OSI_ALARM_ERR_FAIL;
        goto end;
    }
#endif

end:
    osi_mutex_unlock(&alarm_mutex);
    return ret;
}

period_ms_t osi_alarm_get_remaining_ms(const osi_alarm_t *alarm)
{
    assert(osi_mutex_is_valid(&alarm_mutex));
    period_ms_t dt_ms = 0;

    osi_mutex_lock(&alarm_mutex, OSI_MUTEX_MAX_TIMEOUT);
    dt_ms = alarm->deadline_ms - OS_TicksToMSecs(OS_GetTicks());//TODO: wrap around bug
    osi_mutex_unlock(&alarm_mutex);

    return (dt_ms > 0) ? dt_ms : 0;

#if 0
    int64_t dt_us = 0;

    osi_mutex_lock(&alarm_mutex, OSI_MUTEX_MAX_TIMEOUT);
    dt_us = alarm->deadline_us - xr_timer_get_time(); //TODO: wrap around bug
    osi_mutex_unlock(&alarm_mutex);

    return (dt_us > 0) ? (period_ms_t)(dt_us / 1000) : 0;
#endif
}

uint32_t osi_time_get_os_boottime_ms(void)
{
	return OS_TicksToMSecs(OS_GetTicks());
#if 0
    return (uint32_t)(xr_timer_get_time() / 1000);
#endif
}
