
#ifndef _PM_BASE_H_
#define _PM_BASE_H_

#include <stdint.h>

/**
 * @brief Defined all supported low power state.
 * @note:
 *       PM_MODE_ON is used for test.
 *       PM_MODE_SLEEP is used for devices wakeup system. In this mode CPU is
 *         in WFI mode and running at low frequency, all devices are powered on.
 *         set to work mode if you want this device to wakeup system, or else
 *         disable this device to save power.
 *       PM_MODE_STANDBY is used for network or some special wakeup sources to
 *         wakeup system. In this mode CPU and all devices has been powered off,
 *         network can work normally and can wakeup system by received data from
 *         network. Also some special wakeup sources like wakeup timer or IO can
 *         wakeup system if you set this wakeup sources properly.
 *       PM_MODE_HIBERNATION is used for some special wakeup sources to wakeup system.
 *         System will restartup when wakeup. In this mode CPU and all devices
 *         has been powered off beside network. Only some special wakeup sources
 *         can startup system, and can get wakeup event at startup.
 */
typedef enum {
	PM_MODE_ON = 0,
	PM_MODE_SLEEP,
	PM_MODE_STANDBY,
	PM_MODE_HIBERNATION,

	PM_MODE_MAX,
	PM_MODE_BASE = PM_MODE_ON,
} suspend_mode_t;

#define pm_suspend_mode_valid(_t) \
	((_t) >= PM_MODE_BASE && (_t) < PM_MODE_MAX)

/* remain temporarily */
struct pm_suspend_stat {
	const char *name;
	void *failed_unit;
	uint32_t cnt;
};

struct pm_wakeup_ops {
	void (*system_wakeup)(void);
	uint8_t (*check_suspend_abort)(void);
	uint32_t (*check_wakeup_event)(void);

	uint8_t (*irq_is_wakeup_armed)(const int irq);
	void (*record_wakeup_irq)(const int irq);
	void (*clear_wakeup_irq)(void);

	uint32_t (*get_wakeup_event)(void);
	int (*get_wakeup_irq)(void);
	int (*set_time_to_wakeup_ms)(unsigned int ms);
};

const char *pm_mode2string(int mode);
int pm_suspend_assert(void);

void pm_suspend_mode_change(suspend_mode_t mode);

int pm_wakeup_ops_register(struct pm_wakeup_ops *ops);
int pm_hal_record_wakeup_irq(const int irq);
int pm_hal_clear_wakeup_irq(void);
int pm_hal_get_wakeup_event(uint32_t *event);
int pm_hal_get_wakeup_irq(int *irq);
int pm_hal_set_time_to_wakeup_ms(unsigned int ms);

#endif

