#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <hal_atomic.h>
#include <console.h>

#include "pm_base.h"
#include "pm_adapt.h"
#include "pm_debug.h"
#include "pm_wakesrc.h"
#include "pm_rpcfunc.h"
#include "pm_init.h"
#include "pm_devops.h"
#include "pm_wakecnt.h"
#include "pm_subsys.h"

#define PM_WS_IRQ_COLLECTOR	PM_WS_AFFINITY_M33

#define pm_wakesrc_containerof(ptr_module) \
        __containerof(ptr_module, struct pm_wakesrc, node)

static SemaphoreHandle_t pm_wakesrc_mutex = NULL;
static hal_spinlock_t pm_wakesrc_lock;
static struct list_head pm_wakesrc_register_list = LIST_HEAD_INIT(pm_wakesrc_register_list);
/* irq in the sub core will be taken over by the main pm core */
static int sub_core_irq[PM_WAKEUP_SRC_MAX] = {INVAL_IRQ_NUM};

static char *type_name[] = {
	"ALWAYS",
	"MIGHT",
	"SOFT",
};

int pm_wakesrc_init(void)
{
	pm_wakesrc_mutex = xSemaphoreCreateMutex();
	hal_spin_lock_init(&pm_wakesrc_lock);

	return 0;
}

struct pm_wakesrc *pm_wakesrc_find_registered_by_irq(int irq)
{
	struct list_head *node = NULL;
	struct list_head *list = &pm_wakesrc_register_list;
	struct list_head *list_save = NULL;
	struct pm_wakesrc *ws = NULL;

	list_for_each_safe(node, list_save, list) {
		ws = pm_wakesrc_containerof(node);
		if (ws->irq == irq)
			return ws;
	}

	return NULL;
}

struct pm_wakesrc *pm_wakesrc_find_enabled_by_irq(int irq)
{
	struct list_head *node = NULL;
	struct list_head *list = &pm_wakesrc_register_list;
	struct list_head *list_save = NULL;
	struct pm_wakesrc *ws = NULL;

	list_for_each_safe(node, list_save, list) {
		ws = pm_wakesrc_containerof(node);
		if ((ws->irq == irq) && (ws->enabled))
			return ws;
	}
	return NULL;
}

int pm_wakesrc_type_check_num(wakesrc_type_t type)
{
	struct list_head *node = NULL;
	struct list_head *list = &pm_wakesrc_register_list;
	struct list_head *list_save = NULL;
	struct pm_wakesrc *ws = NULL;
	int num_wakesrc = 0;

	if (!wakesrc_type_valid(type))
		return -EINVAL;

	list_for_each_safe(node, list_save, list) {
		ws = pm_wakesrc_containerof(node);
		if (ws->enabled && (ws->type == type))
			num_wakesrc++;
	}

	return num_wakesrc;
}

int pm_wakesrc_register(const int irq, const char *name, const unsigned int type)
{
	int i = 0;
	struct pm_wakesrc *ws = NULL;
	struct pm_wakesrc_settled *ws_settled = NULL;

	if (!name || !wakesrc_type_valid(type) || ((0 > irq) && (irq > PM_SOFT_WAKEUP_IRQ_BASE))) {
		pm_err("%s(%d): Invalid paras\n", __func__, __LINE__);
		return -EINVAL;
	}

#ifdef CONFIG_ARCH_DSP
	/* RINTC_IRQ_MASK */
	if ((type == PM_WAKESRC_SOFT_WAKEUP) && (irq < -65000)) {
			pm_err("%s(%d): Invalid irq for soft wakeup\n", __func__, __LINE__);
			return -EINVAL;
	}
#endif

	if (type == PM_WAKESRC_SOFT_WAKEUP) {
		if (irq > PM_SOFT_WAKEUP_IRQ_BASE) {
			pm_err("%s(%d): Invalid irq for soft wakeup\n", __func__, __LINE__);
			return -EINVAL;
		}
	} else {
		for (i = PM_WAKEUP_SRC_BASE; i < PM_WAKEUP_SRC_MAX; i++) {
			ws_settled = pm_wakesrc_get_by_id(i);
			if (ws_settled->irq == irq)
				break;
		}
		if (i == PM_WAKEUP_SRC_MAX) {
			pm_err("%s(%d): get settled wakesrc failed\n", __func__, __LINE__);
			return -ENODEV;
		}
	}

	if (pm_wakesrc_find_registered_by_irq(irq)) {
		pm_err("%s(%d): irq %d is already registered\n", __func__, __LINE__, irq);
		return -EEXIST;
	}


	if (pdTRUE != xSemaphoreTake(pm_wakesrc_mutex, portMAX_DELAY)) {
		pm_semapbusy(pm_wakesrc_mutex);
		pm_err("%s(%d): register take semaphore failed\n", __func__, __LINE__);
		return -EFAULT;
	}

	ws = malloc(sizeof(struct pm_wakesrc));
	if (!ws) {
		pm_err("%s(%d): register alloc failed\n", __func__, __LINE__);
		return -ENOMEM;
	}
	if (type == PM_WAKESRC_SOFT_WAKEUP) {
		ws->id = PM_SOFT_WAKESRC_MAJOR_ID + irq;
	} else if (ws_settled) {
		ws->id = ws_settled->id;
	}
#ifdef CONFIG_ARCH_ARM_CORTEX_M33
	ws->affinity = PM_WS_AFFINITY_M33;
#elif CONFIG_ARCH_RISCV_C906
	ws->affinity = PM_WS_AFFINITY_RISCV;
#elif CONFIG_ARCH_DSP
	ws->affinity = PM_WS_AFFINITY_DSP;
#endif
	ws->irq = irq;
	ws->enabled = 0;
	snprintf(ws->name, PM_WAKESRC_NAME_LENTH, name);
	ws->cnt.event_cnt = 0;
	ws->cnt.active_cnt = 0;
	ws->cnt.relax_cnt = 0;
	ws->cnt.wakeup_cnt = 0;
	hal_spin_lock_init(&ws->lock);
	ws->type = type;
	list_add_tail(&ws->node, &pm_wakesrc_register_list);

	xSemaphoreGive(pm_wakesrc_mutex);

	return 0;
}

int pm_wakesrc_unregister(int irq)
{
	struct pm_wakesrc **pws = NULL;
	struct pm_wakesrc *ws = NULL;


	ws = pm_wakesrc_find_registered_by_irq(irq);
	if (!ws) {
		pm_err("%s(%d): irq %d hasn't registered\n", __func__, __LINE__, ws->irq);
		return -EINVAL;
	}

	if (ws->enabled) {
		pm_warn("%s(%d): irq %d remain enabled while unregister, try to release irq...\n", __func__, __LINE__, ws->irq);
		if (pm_clear_wakeirq(ws->irq)) {
			pm_err("%s(%d): try to release irq %d failed. pm_wakesrc unregisters failed\n", __func__, __LINE__, ws->irq);
			return -EFAULT;
		} else
			pm_warn("%s(%d): irq %d disabled down\n", __func__, __LINE__, ws->irq);
	}

	if (pdTRUE != xSemaphoreTake(pm_wakesrc_mutex, portMAX_DELAY)) {
		pm_semapbusy(pm_wakesrc_mutex);
		return -EBUSY;
	}

	list_del(&ws->node);
	ws->id = 0;
	ws->affinity = 0;
	ws->irq = 0;
	ws->enabled = 0;
	memset(ws->name, 0, sizeof(char) * PM_WAKESRC_NAME_LENTH);
	ws->cnt.event_cnt = 0;
	ws->cnt.active_cnt = 0;
	ws->cnt.relax_cnt = 0;
	ws->cnt.wakeup_cnt = 0;
	hal_spin_lock_deinit(&ws->lock);
	ws->type = 0;

	free(ws);
	pws = &ws;
	*pws = NULL;

	xSemaphoreGive(pm_wakesrc_mutex);

	return 0;
}

int pm_always_wakeup(void)
{
	struct list_head *node = NULL;
	struct list_head *list = &pm_wakesrc_register_list;
	struct list_head *list_save = NULL;
	struct pm_wakesrc *ws = NULL;
	int might_wakeup = 0;

	list_for_each_safe(node, list_save, list) {
		ws = pm_wakesrc_containerof(node);
		if (ws->type == PM_WAKESRC_MIGHT_WAKEUP)
			might_wakeup++;
	}

	return !(might_wakeup);
}

int pm_set_wakeirq(const int irq)
{
	int ret = 0;
	int i;
	struct pm_wakesrc_settled *ws;
	struct pm_wakesrc *pmws = NULL;
	wakesrc_affinity_t affinity;

	pmws = pm_wakesrc_find_registered_by_irq(irq);
	if (!pmws) {
		pm_err("set wakeirq %d failed, irq is not registered\n", irq);
		return -ENODEV;
	}

	if(pmws->enabled) {
		pm_err("set wakeirq fail. wakesrc exist\n");
		return -EEXIST;
	}

#ifdef CONFIG_ARCH_ARM_CORTEX_M33
	affinity = PM_WS_AFFINITY_M33;
#endif

#ifdef CONFIG_ARCH_RISCV_C906
	affinity = PM_WS_AFFINITY_RISCV;
#endif

#ifdef CONFIG_ARCH_DSP
	affinity = PM_WS_AFFINITY_DSP;
#endif
	if (pmws->type == PM_WAKESRC_SOFT_WAKEUP) {
		pmws->enabled = 1;
		return ret;
	}

	for (i = PM_WAKEUP_SRC_BASE; i < PM_WAKEUP_SRC_MAX; i++) {
		ws = pm_wakesrc_get_by_id(i);
		if (ws->irq == irq)
			break;
	}

	if (i == PM_WAKEUP_SRC_MAX) {
		pm_err("wakeirq_enable failed, irq %d invalid\n", irq);
		return -EINVAL;
	}

	ret = pm_set_wakesrc(ws->id, affinity, 1);
	if (ret)
		pm_err("set wakeirq failed, irq: %d, wakesrc settled: %s\n",ws->irq, ws->name);

	if (pmws)
		pmws->enabled = 1;

	return ret;
}

int pm_clear_wakeirq(const int irq)
{
	int ret = 0;
	int i;
	struct pm_wakesrc_settled *ws;
	wakesrc_affinity_t affinity;

	struct pm_wakesrc *pmws = NULL;
	pmws = pm_wakesrc_find_registered_by_irq(irq);
	if (!pmws) {
		pm_err("clear wakeirq %d failed, irq is not registered\n", irq);
		return -ENODEV;
	}

	if (!pmws->enabled) {
		pm_err("clear wakeirq failed, irq is not enabled");
		return -ENODEV;
	}

	if (pm_wakesrc_is_active(pmws)) {
		pm_err("clear wakeirq failed, pm_wakesrc irq: %d is active\n", irq);
		 return -EBUSY;
	}

#ifdef CONFIG_ARCH_ARM_CORTEX_M33
	affinity = PM_WS_AFFINITY_M33;
#endif

#ifdef CONFIG_ARCH_RISCV_C906
	affinity = PM_WS_AFFINITY_RISCV;
#endif

#ifdef CONFIG_ARCH_DSP
	affinity = PM_WS_AFFINITY_DSP;
#endif

	if (pmws->type == PM_WAKESRC_SOFT_WAKEUP) {
		pmws->enabled = 0;
		return ret;
	}

	for (i = PM_WAKEUP_SRC_BASE; i < PM_WAKEUP_SRC_MAX; i++) {
		ws = pm_wakesrc_get_by_id(i);
		if (ws->irq == irq)
			break;
	}

	if (i == PM_WAKEUP_SRC_MAX) {
		pm_err("wakeirq_enable find pm_wakesrc_settled failed, irq %d invalid\n", irq);
		return -EINVAL;
	}

	ret = pm_set_wakesrc(ws->id, affinity, 0);
	if (ret)
		pm_err("clear wakeirq failed, irq: %d, wakesrc settled: %s\n",ws->irq, ws->name);

	if (pmws)
		pmws->enabled = 0;

	return ret;
}

int pm_wakesrc_is_active(struct pm_wakesrc *ws)
{
	int ret = 0;

	if (!ws || !pm_wakesrc_find_enabled_by_irq(ws->irq))
		return -ENODEV;

	hal_spin_lock(&ws->lock);
	if (ws->cnt.active_cnt != ws->cnt.relax_cnt)
		ret = 1;
	hal_spin_unlock(&ws->lock);

	return ret;
}

int pm_wakesrc_is_disabled(int irq)
{
	struct pm_wakesrc *ws = NULL;

	ws = pm_wakesrc_find_enabled_by_irq(irq);
	if (!ws)
		return -ENODEV;

	if (ws->enabled)
		return 0;
	else
		return 1;
}

void pm_wakesrc_update_irq_in_sub_core(int id, int irq)
{
		if (id < PM_WAKEUP_SRC_MAX)
			sub_core_irq[id] = irq;
}

int pm_wakesrc_irq_in_sub_core(int irq)
{
	int i;

	if (irq == INVAL_IRQ_NUM)
		return 0;

	for (i = 0; i < PM_WAKEUP_SRC_MAX; i++) {
		if (irq == sub_core_irq[i])
			return 1;
	}

	return 0;
}

int pm_wakesrc_soft_wakeup(int irq, wakesrc_action_t action, int keep_ws_enabled)
{
	struct pm_wakesrc *ws = NULL;
	wakesrc_affinity_t affinity;

	if (!wakesrc_action_valid(action))
		return -EINVAL;

	ws = pm_wakesrc_find_enabled_by_irq(irq);
	if (!ws)
		return -EINVAL;

	if (action == PM_WAKESRC_ACTION_WAKEUP_SYSTEM) {
		pm_stay_awake(irq);
		pm_relax(irq, PM_RELAX_WAKEUP);
	} else {
		pm_stay_awake(irq);
		pm_relax(irq, PM_RELAX_SLEEPY);
	}

	if (!keep_ws_enabled)
		pm_clear_wakeirq(irq);

#ifdef CONFIG_ARCH_ARM_CORTEX_M33
	affinity = PM_WS_AFFINITY_M33;
#endif

#ifdef CONFIG_ARCH_RISCV_C906
	affinity = PM_WS_AFFINITY_RISCV;
#endif

#ifdef CONFIG_ARCH_DSP
	affinity = PM_WS_AFFINITY_DSP;
#endif

	return pm_subsys_soft_wakeup(affinity, ws->irq, action);
}

static int pm_wakesrc_list_info(int argc, char **argv)
{
	struct list_head *node = NULL;
	struct list_head *list = &pm_wakesrc_register_list;
	struct list_head *list_save = NULL;
	struct pm_wakesrc *ws = NULL;

	printf("wakesrc     id        irq      type     event_cnt   active_cnt   relax_cnt   wakeup_cnt   enabled\n");
	list_for_each_safe(node, list_save, list) {
		ws = pm_wakesrc_containerof(node);
		printf("%-10s  %08x  %-7d  %-10s  %-10ld  %-10ld  %-10ld  %-10ld  %-10d\n",
			ws->name, ws->id, ws->irq, type_name[ws->type],
			ws->cnt.event_cnt, ws->cnt.active_cnt, ws->cnt.relax_cnt, ws->cnt.wakeup_cnt,
			ws->enabled);
	}

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(pm_wakesrc_list_info, pm_list_wakesrc, pm tools)
