
#ifndef _PM_NOTIFY_H_
#define _PM_NOTIFY_H_

#ifdef CONFIG_KERNEL_FREERTOS
#include <FreeRTOS.h>
#include <semphr.h>
#else
#error "PM do not support the RTOS!!"
#endif

/* The type SYS_PREPARED and SYS_FINISHED are used to notify the pm entry/exit event.
 * The rest are used to notify that the pm prepare/finish to run in the specific core.
 */
typedef enum {
	PM_EVENT_SYS_PERPARED = 0,
	PM_EVENT_PERPARED,
	PM_EVENT_FINISHED,
	PM_EVENT_SYS_FINISHED,

	PM_EVENT_TYPE_MAX,
	PM_EVENT_TYPE_BASE = PM_EVENT_SYS_PERPARED,
} pm_event_t;

#define pm_event_valid(_t) \
	((_t) >= PM_EVENT_TYPE_BASE ||\
	 (_t) <  PM_EVENT_TYPE_MAX)

#define PM_NOTIFY_USED_MAGIC   0xdeadbeaf
#define PM_NOTIFY_ARRAY_MAX    32


typedef int (*pm_notify_cb_t)(suspend_mode_t mode, pm_event_t event, void *arg);
typedef struct pm_notify {
	const char *name;
	uint32_t has_notify;
	pm_notify_cb_t pm_notify_cb;
	void          *arg;
} pm_notify_t;


int pm_notify_init(void);
int pm_notify_exit(void);
/*
 * failed, return error <0
 * succeed, return id(0~PM_NOTIFY_ARRAY_MAX).
 */
int pm_notify_register(pm_notify_t *nt);
/*
 * id is pm_notify_register returned value.
 */
int pm_notify_unregister(int id);
int pm_notify_event(suspend_mode_t mode, pm_event_t event);

#endif

