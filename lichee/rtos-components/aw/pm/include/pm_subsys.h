
#ifndef _PM_SUBSYS_H_
#define _PM_SUBSYS_H_

#ifdef CONFIG_KERNEL_FREERTOS
#include <FreeRTOS.h>
#include <semphr.h>
#else
#error "PM do not support the RTOS!!"
#endif

#include <task.h>
#include <queue.h>

#define WAIT_TIMEOUT_TICKS  			(3000/portTICK_PERIOD_MS)

#define PM_SUBSYS_ACTION_OK_FLAG		(0x16aaf5f5)
#define PM_SUBSYS_ACTION_FAILED_FLAG		(0x16aaffff)

#define PM_SUBSYS_ACTION_HAS_GOT_RESULT		(0x16aafafa)

#ifdef CONFIG_ARCH_SUN20IW2
#define GPRCM_SYS_PRIV_REG0			(0x40050200)
#define RV_ACTION_REC_REG			(GPRCM_SYS_PRIV_REG0)
#define RV_RESUME_OK				(0x16aa0000)

#define GPRCM_SYS_PRIV_REG1			(0x40050204)
#define DSP_ACTION_REC_REG			(GPRCM_SYS_PRIV_REG1)
#define DSP_RESUME_OK				(0x16aa0000)
#endif

#define PM_SUBSYS_ONLINE			(0x1)
#define PM_SUBSYS_OFFLINE			(0x0)

typedef enum {
        PM_SUBSYS_ID_RISCV = 1,
        PM_SUBSYS_ID_DSP,
        PM_SUBSYS_ID_WLAN,
        PM_SUBSYS_ID_BT,

        PM_SUBSYS_ID_MAX,
        PM_SUBSYS_ID_BASE = PM_SUBSYS_ID_RISCV,
} pm_subsys_id_t;

#define pm_subsys_id_valid(_t) \
	((_t) >= PM_SUBSYS_ID_BASE && (_t) < PM_SUBSYS_ID_MAX)

typedef enum {
        PM_SUBSYS_ACTION_NOP = 0,

        PM_SUBSYS_ACTION_TO_SUSPEND,
        PM_SUBSYS_ACTION_TO_RESUME,
	PM_SUBSYS_ACTION_KEEP_AWAKE,
        PM_SUBSYS_ACTION_TO_NORMAL,

        PM_SUBSYS_ACTION_MAX,
        PM_SUBSYS_ACTION_BASE = PM_SUBSYS_ACTION_NOP,
} pm_subsys_action_t;

#define pm_subsys_action_valid(_t) \
	((_t) >= PM_SUBSYS_ACTION_BASE && (_t) < PM_SUBSYS_ACTION_MAX)

typedef enum {
        PM_SUBSYS_RESULT_NOP = 0,

        PM_SUBSYS_RESULT_SUSPEND_OK,
        PM_SUBSYS_RESULT_SUSPEND_FAILED,
        PM_SUBSYS_RESULT_SUSPEND_REJECTED,

        PM_SUBSYS_RESULT_RESUME_OK,
        PM_SUBSYS_RESULT_RESUME_FAILED, // no use

        PM_SUBSYS_RESULT_NOTHING,

        PM_SUBSYS_RESULT_MAX,
        PM_SUBSYS_RESULT_BASE = PM_SUBSYS_RESULT_NOP,
} pm_subsys_result_t;

#define pm_subsys_result_valid(_t) \
	((_t) >= PM_SUBSYS_RESULT_BASE && (_t) < PM_SUBSYS_RESULT_MAX)

typedef enum {
        PM_SUBSYS_STATUS_NORMAL = 0,
        PM_SUBSYS_STATUS_SUSPENDING,
        PM_SUBSYS_STATUS_SUSPENDED,
        PM_SUBSYS_STATUS_RESUMING,
        PM_SUBSYS_STATUS_ERROR,
	PM_SUBSYS_STATUS_KEEP_AWAKE,
} pm_subsys_status_t;

#define is_subsys_sync_finished(_x) \
	(PM_SUBSYS_STATUS_SUSPENDING != (_x) && \
	 PM_SUBSYS_STATUS_RESUMING != (_x))

typedef struct pm_subsys {
	const char *name;
	pm_subsys_id_t id;
	unsigned int status;
	unsigned int online;
	unsigned int event_pending;
	unsigned int sync_pending;
	/*notify subsys suspend or resume*/
	QueueHandle_t  xQueue_Signal;
	/*subsys return result of suspend/resume */
	QueueHandle_t  xQueue_Result;

	TaskHandle_t   xHandle_Signal;
	TaskHandle_t   xHandle_Watch;
} pm_subsys_t;

typedef struct pm_subsys_msg {
	pm_subsys_id_t id;
	suspend_mode_t mode;
	int            action;
} pm_subsys_msg_t;

int pm_subsys_init(void);
int pm_subsys_register(pm_subsys_id_t subsys_id, pm_subsys_t *subsys);
int pm_subsys_report_result(pm_subsys_id_t subsys_id, struct pm_subsys_msg *msg);
int pm_subsys_update(pm_subsys_id_t subsys_id, pm_subsys_action_t action);
int pm_subsys_notify(pm_subsys_action_t action, suspend_mode_t mode);
int pm_subsys_sync(void);

int pm_subsys_suspend_sync(suspend_mode_t mode);
int pm_subsys_resume_sync(suspend_mode_t mode);

void pm_subsys_notify_mask(pm_subsys_id_t subsys_id);
void pm_subsys_notify_unmask(pm_subsys_id_t subsys_id, int all);

uint32_t pm_subsys_check_in_status(pm_subsys_status_t status);
uint32_t pm_subsys_check_online(void);
int pm_subsys_event_pending(pm_subsys_id_t subsys_id);
#endif



