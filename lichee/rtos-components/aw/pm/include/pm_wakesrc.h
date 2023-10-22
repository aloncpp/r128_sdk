#ifndef _PM_WAKESRC_H_
#define _PM_WAKESRC_H_

#ifdef CONFIG_KERNEL_FREERTOS
#include <FreeRTOS.h>
#include <semphr.h>
#else
#error "PM do not support the RTOS!!"
#endif

#include <hal/aw_list.h>
#include <hal_atomic.h>
#include "pm_wakeres.h"

#define PM_WAKESRC_REGISTER_MAGIC	(0xadfc4215)
#define INVAL_IRQ_NUM			(-32)

#define PM_SOFT_WAKEUP_IRQ_BASE		(-33)
#define PM_SOFT_WAKESRC_MAJOR_ID	(PM_SOFT_WAKEUP_IRQ_BASE)

#define PM_WAKESRC_NAME_LENTH		(32U)

typedef enum {
	PM_WAKEUP_SRC_WKIO0 = 0,
	PM_WAKEUP_SRC_WKIO1,
	PM_WAKEUP_SRC_WKIO2,
	PM_WAKEUP_SRC_WKIO3,
	PM_WAKEUP_SRC_WKIO4,
	PM_WAKEUP_SRC_WKIO5,
	PM_WAKEUP_SRC_WKIO6,
	PM_WAKEUP_SRC_WKIO7,
	PM_WAKEUP_SRC_WKIO8,
	PM_WAKEUP_SRC_WKIO9,
	PM_WAKEUP_SRC_WKTMR, /*in pmu spec*/
	PM_WAKEUP_SRC_ALARM0, /* 11*/
	PM_WAKEUP_SRC_ALARM1,
	PM_WAKEUP_SRC_WKTIMER0,
	PM_WAKEUP_SRC_WKTIMER1,
	PM_WAKEUP_SRC_WKTIMER2,
	PM_WAKEUP_SRC_LPUART0,
	PM_WAKEUP_SRC_LPUART1,
	PM_WAKEUP_SRC_GPADC,
	PM_WAKEUP_SRC_MAD,
	PM_WAKEUP_SRC_WLAN,
	PM_WAKEUP_SRC_BT,
	PM_WAKEUP_SRC_BLE,
	PM_WAKEUP_SRC_GPIOA,
	PM_WAKEUP_SRC_GPIOB,
	PM_WAKEUP_SRC_GPIOC,
	PM_WAKEUP_SRC_DEVICE,/*Reserved*/

	PM_WAKEUP_SRC_MAX,
	PM_WAKEUP_SRC_BASE = PM_WAKEUP_SRC_WKIO0,

	//PM_WAEUP_SRC_ENUM_INTVAL = PM_ENUM_EXTERN,
	//PM_WAEUP_SRC_ENUM_INTVAL = (0x1 << ((sizeof(int)-1)*8)),
} wakesrc_id_t;

#define  wakesrc_id_valid(_id) \
	((_id) >= PM_WAKEUP_SRC_BASE && (_id) < PM_WAKEUP_SRC_MAX)
#define PM_WAKEUP_SRC_WKIOx(_n) (PM_WAKEUP_SRC_WKIO0 + (_n))

typedef enum {
	PM_WS_AFFINITY_M33 = 0,
	PM_WS_AFFINITY_DSP,
	PM_WS_AFFINITY_RISCV,

	PM_WS_AFFINITY_MAX,
	PM_WS_AFFINITY_BASE = PM_WS_AFFINITY_M33,
} wakesrc_affinity_t;
#define  wakesrc_affinity_valid(_id) \
	((_id) >= PM_WS_AFFINITY_BASE && (_id) < PM_WS_AFFINITY_MAX)

typedef enum {
	PM_WAKESRC_ALWAYS_WAKEUP = 0,
	PM_WAKESRC_MIGHT_WAKEUP,
	PM_WAKESRC_SOFT_WAKEUP,

	PM_WAKESRC_TYPE_MAX,
	PM_WAKESRC_TYPE_BASE = PM_WAKESRC_ALWAYS_WAKEUP,
} wakesrc_type_t;
#define  wakesrc_type_valid(_type) \
	((_type) >= PM_WAKESRC_TYPE_BASE && (_type) < PM_WAKESRC_TYPE_MAX)

typedef enum {
	PM_WAKESRC_ACTION_WAKEUP_SYSTEM = 0,
	PM_WAKESRC_ACTION_SLEEPY,

	PM_WAKESRC_ACTION_MAX,
	PM_WAKESRC_ACTION_BASE = PM_WAKESRC_ACTION_WAKEUP_SYSTEM,
} wakesrc_action_t;
#define  wakesrc_action_valid(_action) \
	((_action) >= PM_WAKESRC_ACTION_BASE && (_action) < PM_WAKESRC_ACTION_MAX)

struct pm_wakesrc_settled {
	const wakesrc_id_t id;
	const int          irq;
	const char        *name;
	struct pm_wakeres  res;
};

/* struct wakesrc_cnt - CNT record of wakesrc event
 * @event_count: Number of events have been handled.
 * @active_count: Number of times the wakeup source was activated.
 * @relax_count: Number of times the wakeup source was deactivated.
 * @wakeup_count: Number of times the wakeup source might abort suspend.
 */
struct pm_wakesrc_cnt {
	unsigned long event_cnt;
	unsigned long active_cnt;
	unsigned long relax_cnt;
	unsigned long wakeup_cnt;
};

struct pm_wakesrc {
	int id;
	wakesrc_affinity_t affinity;
	int irq;
	int enabled;
	char name[PM_WAKESRC_NAME_LENTH];
	struct pm_wakesrc_cnt cnt;
	hal_spinlock_t lock;
	wakesrc_type_t type;
	struct list_head  node;
};

struct pm_wakesrc_settled *pm_wakesrc_get_by_id(wakesrc_id_t id);

/* int pm_wakesrc_init(void); */

struct pm_wakesrc *pm_wakesrc_find_registered_by_irq(int irq);
struct pm_wakesrc *pm_wakesrc_find_enabled_by_irq(int irq);
int pm_wakesrc_type_check_num(wakesrc_type_t type);
int pm_wakesrc_is_active(struct pm_wakesrc *ws);
int pm_wakesrc_is_disabled(int irq);
int pm_always_wakeup(void);
int pm_wakesrc_register(const int irq, const char *name, const unsigned int type);
int pm_wakesrc_unregister(int irq);
int pm_set_wakeirq(const int irq);
int pm_clear_wakeirq(const int irq);

void pm_wakesrc_update_irq_in_sub_core(int id, int irq);
int pm_wakesrc_irq_in_sub_core(int irq);

int pm_wakesrc_soft_wakeup(int irq, wakesrc_action_t action, int keep_ws_enabled);

#endif

