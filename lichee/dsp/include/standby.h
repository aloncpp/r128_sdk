#ifndef __STANDBY_H
#define __STANDBY_H

#include <FreeRTOS.h>
#include <task.h>

#include "bitops.h"
#include "message_manage.h"
#include "aw_common.h"
#include "sunxi_hal_common.h"
#include "platform.h"

#define invoke_function(x, y) \
	({                    \
		if ((x))        \
			(x)((y)); \
	})

typedef enum {
	arisc_power_on = 0,
	arisc_power_retention = 1,
	arisc_power_off = 3,
} arisc_power_state_t;

typedef enum {
	arisc_system_shutdown = 0,
	arisc_system_reboot = 1,
	arisc_system_reset = 2
} arisc_system_state_t;

/**
 * we want to simple this struct with arch code
 * this used with platform.
 *
 * platform_data: platform define data
 *
 * debug_flow: NIPM debug flow, eg. write code to rtc
 */
struct platform_standby {
	void *platform_data;
	int ms_to_wakeup;
	struct message *msg;
	int (*pre_standby_enter)(void *);
	int (*post_standby_enter)(void *);

	int (*pre_standby_exit)(void *);
	int (*post_standby_exit)(void *);

	int (*ddr_suspend)(void *);
	int (*ddr_resume)(void *);

	int (*pll_suspend)(void *);
	int (*pll_resume)(void *);

	int (*bus_suspend)(void *);
	int (*bus_resume)(void *);

	int (*osc_suspend)(void *);
	int (*osc_resume)(void *);

	int (*power_suspend)(void *);
	int (*power_resume)(void *);

	int (*cpu_suspend)(void *);
	int (*cpu_resume)(void *);

	int (*standby_enter)(struct message *);
	int (*standby_exit)(struct message *);

	int (*start_wait_wakeup)(void *);
	int (*stop_wait_wakeup)(void *);
	int (*set_wakeup_source)(struct message *);
	int (*clear_wakeup_source)(struct message *);

	int (*debug_flow)(void *);
};

void task_standby(void *parg);

int cpu_op(struct message *pmessage);
int sys_op(struct message *pmessage);
void standby_platform_message(struct message *msg);
struct platform_standby *platform_standby_get(void);
void standby_wait_wakeup(struct platform_standby *ps);
void standby_wakeup(struct platform_standby *ps);
void standby_wakeup_isr(struct platform_standby *ps, BaseType_t *pxHigherPriorityTaskWoken);
void __standby_wakeup_system(struct platform_standby *ps);
void standby_set_delay_wakeup(int ms);
s32 standby_dram_crc_enable(void);
s32 standby_set_dram_crc_paras(u32 enable, u32 src, u32 len);
u32 standby_dram_crc(void);


#endif /* __STANDBY_H */

