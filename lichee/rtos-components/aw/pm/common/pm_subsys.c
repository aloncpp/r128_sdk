#include <stdlib.h>
#include <string.h>
#include <osal/hal_interrupt.h>
#include <errno.h>
#include <hal_time.h>

#include "pm_debug.h"
#include "pm_base.h"
#include "pm_subsys.h"
#include "pm_adapt.h"

#undef   PM_DEBUG_MODULE
#define  PM_DEBUG_MODULE  PM_DEBUG_SUBSYS

#define subsys_containerof(ptr_module) \
        __containerof(ptr_module, struct pm_subsys, node)

static pm_subsys_t *subsys_array[PM_SUBSYS_ID_MAX] = {0};
static QueueSetHandle_t subsys_set = NULL;
static int subsys_number = 0;
static volatile uint32_t pm_subsys_mask = 0;

int pm_subsys_init(void)
{
	subsys_set = xQueueCreateSet( PM_SUBSYS_ID_MAX*sizeof(pm_subsys_msg_t) );
	if (NULL == subsys_set) {
		pm_err("create subsys_set failed.\n");
	}

	return 0;
}

int pm_subsys_register(pm_subsys_id_t subsys_id, pm_subsys_t *subsys)
{
	if (!pm_subsys_id_valid(subsys_id) || !subsys \
			|| !subsys->xQueue_Signal \
			|| !subsys->xQueue_Result) {
		return -EPERM;
	}

	if (subsys_array[subsys_id]) {
		pm_err("subsys %d is already registered\n", subsys_id);
		return -EEXIST;
	}
	subsys_array[subsys_id] = subsys;
	subsys_array[subsys_id]->id = subsys_id;
	subsys_array[subsys_id]->online = PM_SUBSYS_ONLINE;
	xQueueAddToSet( subsys->xQueue_Result, subsys_set);

	subsys_number ++;

	return 0;
}

void pm_subsys_notify_mask(pm_subsys_id_t subsys_id)
{
	pm_subsys_mask |= (0x1 << subsys_id);
}

void pm_subsys_notify_unmask(pm_subsys_id_t subsys_id, int all)
{
	pm_subsys_mask &= ~(0x1 << subsys_id);
	if (all)
		pm_subsys_mask = 0;
}

int pm_subsys_report_result(pm_subsys_id_t subsys_id, struct pm_subsys_msg *msg)
{
	if (!pm_subsys_id_valid(subsys_id) || !subsys_array[subsys_id] || !subsys_array[subsys_id]->xQueue_Result)
		return -EINVAL;

	msg->id = subsys_id;
	if (subsys_array[subsys_id]->event_pending)
		subsys_array[subsys_id]->event_pending--;
	else
		pm_dbg("subsys[%d] has no event pending but send result[%d]\n", msg->action, msg->action);
	if (msg->action == PM_SUBSYS_RESULT_NOTHING)
		return 0;

	if (pdTRUE != xQueueSend(subsys_array[subsys_id]->xQueue_Result, msg, portMAX_DELAY)) {
		pm_err("subsys report result failed, send queue error\n");
		return -EFAULT;
	}

	return 0;
}

static int pm_subsys_send_signal(pm_subsys_id_t subsys_id, pm_subsys_msg_t *msg)
{
	if (!msg || !pm_subsys_id_valid(subsys_id))
		return -EINVAL;

	msg->id = subsys_id;

	if (pdTRUE != xQueueSend(subsys_array[subsys_id]->xQueue_Signal, msg, portMAX_DELAY)) {
		pm_err("send msg to subsys[%d] failed.\n", subsys_id);
		return -EFAULT;
	} else {
		subsys_array[subsys_id]->event_pending++;
		if ((msg->action == PM_SUBSYS_ACTION_TO_SUSPEND) || (msg->action == PM_SUBSYS_ACTION_TO_RESUME))
			subsys_array[subsys_id]->sync_pending = 1;
	}

	return 0;
}

int pm_subsys_update(pm_subsys_id_t subsys_id, pm_subsys_action_t action)
{
	pm_subsys_msg_t msg;

	if (!pm_subsys_id_valid(subsys_id) || \
		!pm_subsys_action_valid(action) ||\
		!subsys_array[subsys_id])
		return -EPERM;

	msg.action = action;
	pm_subsys_send_signal(subsys_id, &msg);

	return 0;
}

int pm_subsys_event_pending(pm_subsys_id_t subsys_id)
{
	if (!pm_subsys_id_valid(subsys_id) || !subsys_array[subsys_id])
		return 0;

	return subsys_array[subsys_id]->event_pending;
}

int pm_subsys_notify(pm_subsys_action_t action, suspend_mode_t mode)
{
	int id;
	pm_subsys_msg_t msg;

	if (!pm_subsys_action_valid(action)) {
		return -EINVAL;
	}

	msg.mode = mode;
	msg.action = action;

	for (id=PM_SUBSYS_ID_BASE; id<PM_SUBSYS_ID_MAX; id++) {
		pm_dbg("pm_subsys_mask: 0x%x\n", pm_subsys_mask);
		if (pm_subsys_mask & (0x1 << id))
			continue;

		if (subsys_array[id] && subsys_array[id]->xQueue_Signal) {
			if (!(pm_subsys_mask & (0x1 << id))) {
				pm_dbg(" notify subsys[%d] action: %d\n", id, action);
				pm_subsys_send_signal(id, &msg);
			}
		}
        }

	return 0;
}

int pm_subsys_sync(void)
{
	int id;
	unsigned int failed_subsys;
	pm_subsys_msg_t msg;
	unsigned int have_failed = 0;
	unsigned int sync_sum = 0;
	QueueSetMemberHandle_t xActivatedMember = NULL;

	if (!subsys_number)
		return 0;

	pm_dbg("pm subsys sync check pending... \n");
	for (id=PM_SUBSYS_ID_BASE; id<PM_SUBSYS_ID_MAX; id++) {
		if (!subsys_array[id])
			continue;

		/* ensure theat the subsys does receive the message and change status */
		while (pm_subsys_event_pending(id)) {
			hal_msleep(2);
		}
		if (subsys_array[id]->sync_pending)
			sync_sum |= (0x1 << id);
	}
	id = 0;
	if (!sync_sum)
		return 0;

	pm_log("pm subsys sync...\n");
wait:
	xActivatedMember = xQueueSelectFromSet(subsys_set, WAIT_TIMEOUT_TICKS);
	/* wait timeout */
	if (NULL == xActivatedMember) {
		pm_log("subsys sync timeout\n");
		goto wait;
	}

	xQueueReceive(xActivatedMember, &msg, portMAX_DELAY);
	if (!pm_subsys_id_valid(msg.id)) {
		pm_err("%s(%d): invalid subsys id", __func__, __LINE__);
		goto wait;
	}

	pm_dbg("subsys sync id: %d\n", msg.id);
	pm_dbg("subsys sync act: %d\n", msg.action);

	switch (msg.action) {
	case PM_SUBSYS_RESULT_SUSPEND_FAILED:
	case PM_SUBSYS_RESULT_SUSPEND_REJECTED:
	case PM_SUBSYS_RESULT_RESUME_FAILED:
		have_failed ++;
		failed_subsys |= 0x1 << msg.id;
		subsys_array[msg.id]->sync_pending = 0;
		break;
	case PM_SUBSYS_RESULT_SUSPEND_OK:
		if (subsys_array[msg.id]) {
			subsys_array[msg.id]->online = PM_SUBSYS_OFFLINE;
			subsys_array[msg.id]->sync_pending = 0;
		} else
			pm_err("%s(%d): subsys id(%d) has not registered", __func__, __LINE__, msg.id);
		break;
	case PM_SUBSYS_RESULT_RESUME_OK:
		if (subsys_array[msg.id]) {
			subsys_array[msg.id]->online = PM_SUBSYS_ONLINE;
			subsys_array[msg.id]->sync_pending = 0;
		} else
			pm_err("%s(%d): subsys id(%d) has not registered", __func__, __LINE__, msg.id);
		break;
	default:
		goto wait;
	}

	for (id=PM_SUBSYS_ID_BASE; id<PM_SUBSYS_ID_MAX; id++) {
		if (!subsys_array[id])
			continue;

		pm_dbg("susbys[%d] status: %d, sync_pending: %d\n", id, subsys_array[id]->status, subsys_array[id]->sync_pending);
		/* ensure that subsys result message does get processed */
		if (!is_subsys_sync_finished(subsys_array[id]->status) ||
				subsys_array[id]->sync_pending)
			goto wait;
	}
	pm_dbg("subsys sync end\n");

	if (have_failed) {
		pm_err("%s(%d): failed_subsys: 0x%x", __func__, __LINE__, failed_subsys);
		return -EPERM;
	}

	return 0;
}


int pm_subsys_suspend_sync(suspend_mode_t mode)
{
	int ret = 0;
	/*wait rv,dsp suspend.*/
	pm_subsys_notify(PM_SUBSYS_ACTION_TO_SUSPEND, mode);

	ret = pm_subsys_sync();

	/* need to resume all subsys.*/
	if (ret) {
		pm_subsys_notify(PM_SUBSYS_ACTION_TO_RESUME, mode);
		pm_subsys_sync();
	}

	return ret;
}

int pm_subsys_resume_sync(suspend_mode_t mode)
{
	pm_subsys_notify(PM_SUBSYS_ACTION_TO_RESUME, mode);

	pm_subsys_sync();

	return 0;
}

uint32_t pm_subsys_check_in_status(pm_subsys_status_t status)
{
	int id;
	uint32_t id_sum = 0;

	for (id = PM_SUBSYS_ID_BASE; id < PM_SUBSYS_ID_MAX; id++) {
		if (subsys_array[id] && (subsys_array[id]->status == status))
			id_sum |= (0x1 << id);
	}

	return id_sum;
}

uint32_t pm_subsys_check_online(void)
{
	int id;
	uint32_t id_sum = 0;

	for (id = PM_SUBSYS_ID_BASE; id < PM_SUBSYS_ID_MAX; id++) {
		if (subsys_array[id] && (subsys_array[id]->online == PM_SUBSYS_ONLINE))
			id_sum |= (0x1 << id);
	}

	return id_sum;
}
