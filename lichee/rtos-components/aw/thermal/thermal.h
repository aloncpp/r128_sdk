#ifndef __THERMAL_H__
#define __THERMAL_H__

#include <aw_list.h>
#include <hal_timer.h>
#include <hal_atomic.h>
#include <queue.h>
#include <task.h>
#include <hal_thread.h>

/* use value which < 0K, to indicate an invalid/uninitialized temperature. Refer to linux kernel*/
#define THERMAL_TEMP_INVALID		(-274000)

#define THERMAL_TEMP_CRITICAL		(110000)

/* poling interval to check temperature */
#define POLLING_INTERVAL		(MS_TO_OSTICK(1000))

#define THERMAL_LOG_ERR_MASK		(0x1)
#define THERMAL_LOG_WARN_MASK		(0x2)
#define THERMAL_LOG_DBG_MASK		(0x4)
#define THERMAL_LOG_INF_MASK		(0x8)

extern int thermal_log_level;
#define thermal_err(fmt, arg...) do { if (thermal_log_level & THERMAL_LOG_ERR_MASK) printf(fmt, ##arg); } while(0)
#define thermal_warn(fmt, arg...) do { if (thermal_log_level & THERMAL_LOG_WARN_MASK) printf(fmt, ##arg); } while(0)
#define thermal_dbg(fmt, arg...) do { if (thermal_log_level & THERMAL_LOG_DBG_MASK) printf(fmt, ##arg); } while(0)
#define thermal_info(fmt, arg...) do { if (thermal_log_level & THERMAL_LOG_INF_MASK) printf(fmt, ##arg); } while(0)

#define THERMAL_CRIT_TEMP_SWITCH_MASK	(0x1 << 0)
#define THERMAL_EMUL_TEMP_SWITCH_MASK	(0x1 << 1)

typedef enum  {
    OS_PRIORITY_IDLE            = HAL_THREAD_PRIORITY_LOWEST,
    OS_PRIORITY_LOW             = HAL_THREAD_PRIORITY_LOWEST + 1,
    OS_PRIORITY_BELOW_NORMAL    = HAL_THREAD_PRIORITY_MIDDLE - 1,
    OS_PRIORITY_NORMAL          = HAL_THREAD_PRIORITY_MIDDLE,
    OS_PRIORITY_ABOVE_NORMAL    = HAL_THREAD_PRIORITY_MIDDLE + 1,
    OS_PRIORITY_HIGH            = HAL_THREAD_PRIORITY_HIGHEST - 1,
    OS_PRIORITY_REAL_TIME       = HAL_THREAD_PRIORITY_HIGHEST
} OS_Priority;

struct thermal_zone_device_ops {
	int (*get_temp)(void *data, int *temp);
	void (*shutdown)(void);
};

struct thermal_zone_device {
	void *priv;
	TaskHandle_t  xHandle;
	QueueHandle_t xQueue;
	struct thermal_zone_device_ops *ops;

	struct thermal_zone *zone;
};

struct thermal_zone_ops {
	int (*set_emul_temp)(void *data, const int emul_temp);
	int (*set_crit_temp)(void *data, const int crit_temp);
	int (*set_func_switch)(void *data, const int func_switch_mask);
};

struct thermal_zone {
	char *type;
	char mode;
	int id;
	int last_temperature;
	int temperature;
	int emul_temperature;
	int crit_temperature;
	int func_switch;

	struct thermal_zone_ops *ops;
	struct thermal_zone_device *zone_dev;
	unsigned int polling_interval;
	osal_timer_t polling_timer;
	hal_spinlock_t lock;
	struct list_head node;
};

int thermal_init(void);
void thermal_zone_deinit(void);
struct thermal_zone_device *thermal_zone_device_register(char *zone_name,
							struct thermal_zone_device_ops *ops,
							void *priv);
int thermal_zone_device_unregister(struct thermal_zone_device *zone_dev);


#endif
