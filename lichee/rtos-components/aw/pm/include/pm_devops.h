
#ifndef _PM_DEVOPS_H_
#define _PM_DEVOPS_H_

#ifdef CONFIG_KERNEL_FREERTOS
#include <FreeRTOS.h>
#include <semphr.h>
#else
#error "PM do not support the RTOS!!"
#endif

#include <hal/aw_list.h>
#include "pm_base.h"

typedef enum {
	PM_DEVOPS_TYPE_PREPARED = 0,
	PM_DEVOPS_TYPE_SUSPEND,
	PM_DEVOPS_TYPE_SUSPEND_LATE,
	PM_DEVOPS_TYPE_SUSPEND_NOIRQ,
	PM_DEVOPS_TYPE_RESUME_NOIRQ,
	PM_DEVOPS_TYPE_RESUME_EARLY,
	PM_DEVOPS_TYPE_RESUME,
	PM_DEVOPS_TYPE_COMPLETE,

	PM_DEVOPS_TYPE_MAX,
	PM_DEVOPS_TYPE_BASE = PM_DEVOPS_TYPE_PREPARED,
} pm_devops_type_t;

#define pm_devops_type_valid(_t) \
	((_t) >= PM_DEVOPS_TYPE_BASE && (_t) < PM_DEVOPS_TYPE_MAX)
#define pm_devops_belong_suspend(_t) \
	((_t) >= PM_DEVOPS_TYPE_BASE && (_t) <= PM_DEVOPS_TYPE_SUSPEND_NOIRQ)
#define pm_devops_belong_resume(_t) \
	((_t) >= PM_DEVOPS_TYPE_RESUME_NOIRQ && (_t) < PM_DEVOPS_TYPE_MAX)



struct pm_device;
struct pm_devops {
	int (*prepared) (struct pm_device *dev, suspend_mode_t mode);
	int (*suspend) (struct pm_device *dev, suspend_mode_t mode);
	int (*suspend_late) (struct pm_device *dev, suspend_mode_t mode);
	int (*suspend_noirq) (struct pm_device *dev, suspend_mode_t mode);
	int (*resume_noirq) (struct pm_device *dev, suspend_mode_t mode);
	int (*resume_early) (struct pm_device *dev, suspend_mode_t mode);
	int (*resume) (struct pm_device *dev, suspend_mode_t mode);
	int (*complete) (struct pm_device *dev, suspend_mode_t mode);
};

struct pm_device {
	const char       *name;
	struct list_head  node;
	struct pm_devops  *ops;
	void             *data;
};

int pm_devops_init(void);
int pm_devops_exit(void);
int pm_devops_register(struct pm_device *dev);
int pm_devops_unregister(struct pm_device *dev);

int pm_devops_prepared(suspend_mode_t mode);
int pm_devops_suspend(suspend_mode_t mode);
int pm_devops_suspend_late(suspend_mode_t mode);
int pm_devops_suspend_noirq(suspend_mode_t mode);

void pm_devops_resume_noirq(suspend_mode_t mode);
void pm_devops_resume_early(suspend_mode_t mode);
void pm_devops_resume(suspend_mode_t mode);
void pm_devops_complete(suspend_mode_t mode);

const char *pm_devops_type2string(int type);

#endif

