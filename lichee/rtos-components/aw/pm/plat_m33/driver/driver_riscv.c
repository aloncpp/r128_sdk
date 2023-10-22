
#include <errno.h>
#include <io.h>

#include "pm_adapt.h"
#include "pm_debug.h"
#include "pm_suspend.h"
#include "pm_notify.h"
#include "pm_wakelock.h"
#include "pm_subsys.h"
#include "pm_task.h"

#include "pm_rpcfunc.h"

#undef   PM_DEBUG_MODULE
#define  PM_DEBUG_MODULE  PM_DEBUG_M33

int riscv_notify_cb(suspend_mode_t mode, pm_event_t event, void *arg)
{
	int ret = 0;

	switch (event) {
	case PM_EVENT_SYS_PERPARED:
	case PM_EVENT_SYS_FINISHED:
		ret = pm_msgtorv_trigger_notify(mode, event);
		break;
	default:
		break;
	}

	return ret;
}

static pm_notify_t riscv_notify = {
	.name = "riscv_notify",
	.pm_notify_cb = riscv_notify_cb,
	.arg = NULL,
};

static pm_subsys_t riscv_subsys = {
	.name = "riscv_subsys",
	.status = PM_SUBSYS_STATUS_NORMAL,
	.xQueue_Signal = NULL,
	.xQueue_Result = NULL,
	.xHandle_Signal = NULL,
	.xHandle_Watch = NULL,
};

static int riscv_trigger_suspend(suspend_mode_t mode)
{
	return pm_msgtorv_trigger_suspend(mode);
}

#define PM_PMU_BASE				(0x40051400)
#define PM_SYS_LOW_POWER_CTRL_REG		(PM_PMU_BASE + 0x0100)
#define PM_RV_WUP_EN_MASK			(0x1 << 8)
#define PM_SYS_LOW_POWER_STATUS_REG		(PM_PMU_BASE + 0x0104)
#define PM_RV_ALIVE				(0x1 << 8)

#define PM_CCMU_AON_BASE			(0x4004c400)
#define PM_DPLL1_OUT_CFG_REG			(PM_CCMU_AON_BASE + 0x00a4)
#define PM_CK1_C906_EN_MASK			(0x1 << 7)
#define PM_CK1_C906_DIV_MASK			(0x7 << 4)
#define PM_SYS_CLK_CFG_REG			(PM_CCMU_AON_BASE + 0x00e0)
#define PM_CKPLL_C906_SEL_MASK			(0x1 << 17)

#define PM_CCMU_BASE				(0x4003c000)
#define PM_CPU_DSP_RV_CLK_GATING_CTRL_REG	(PM_CCMU_BASE + 0x0014)
#define PM_RISCV_CLK_GATING_MASK		(0x1 << 19)
#define PM_CPU_DSP_RV_RST_CTRL_REG		(PM_CCMU_BASE + 0x0018)
#define PM_RISCV_APB_SOFT_RST_MASK		(0x1 << 21)
#define PM_RISCV_CFG_RST_MASK			(0x1 << 19)
#define PM_RISCV_CORE_RST_MASK			(0x1 << 16)
#define PM_RISCV_CLK_CTRL_REG			(PM_CCMU_BASE + 0x0064)
#define PM_RISCV_CLK_EN_MASK			(0x1 << 31)
#define PM_RISCV_CLK_SEL_MASK			(0x3 << 4)
#define PM_RISCV_CLK_DIV_MASK			(0x3 << 0)

#define PM_RISCV_SYS_CFG_BASE			(0x40028000)
#define PM_RISCV_STA_ADDR0_REG			(PM_RISCV_SYS_CFG_BASE + 0x0004)
extern struct spare_rtos_head_t rtos_spare_head;
int c906_freq_store;
int pm_boot_c906(void)
{
	uint32_t reg_val;

	if (readl(PM_SYS_LOW_POWER_STATUS_REG) & PM_RV_ALIVE) {
		pm_log("---c906 alive, skip boot---\n");
		return 0;
	}

	pm_log("---pm resume boot c906---\n");

	/* set core in reset state */
	reg_val = readl(PM_CPU_DSP_RV_RST_CTRL_REG);
	writel(reg_val & ~PM_RISCV_CORE_RST_MASK, PM_CPU_DSP_RV_RST_CTRL_REG);

	/* rv wakeup enable */
	//sr32(PMC_BASE + 0x100, 8, 1, 1);
	writel(readl(PM_SYS_LOW_POWER_CTRL_REG) | PM_RV_WUP_EN_MASK, PM_SYS_LOW_POWER_CTRL_REG);
	/* wait for stability alive */
	while(!(readl(PM_SYS_LOW_POWER_STATUS_REG) & PM_RV_ALIVE));

	/* enable ck1 c906 output */
	//sr32(CCMU_AON_BASE+0xa4,  7, 1, 0x1);
	reg_val = readl(PM_DPLL1_OUT_CFG_REG);
	writel(reg_val | PM_CK1_C906_EN_MASK, PM_DPLL1_OUT_CFG_REG);

	/* restore freq instead of set freq */
	/* set clk_ck1_c906 clk to 480M */
	//sr32(CCMU_AON_BASE+0xa4,  4, 3, 0x1);
	reg_val = readl(PM_DPLL1_OUT_CFG_REG);
	writel((reg_val & ~PM_CK1_C906_DIV_MASK)
		| (0x1 << 4), PM_DPLL1_OUT_CFG_REG);

	/* set clk_ck_c906 source to clk_ck1_c906 */
	//sr32(CCMU_AON_BASE+0xe0, 17, 1, 0x0);
	reg_val = readl(PM_SYS_CLK_CFG_REG);
	writel(reg_val & ~PM_CKPLL_C906_SEL_MASK, PM_SYS_CLK_CFG_REG);

	/* set clk_c906_sel source to clk_ck_c906 */
	//sr32(CCMU_BASE+0x064, 4, 2, 0x2);
	reg_val = readl(PM_RISCV_CLK_CTRL_REG);
	writel((reg_val & ~PM_RISCV_CLK_SEL_MASK)
		| (0x2 << 4), PM_RISCV_CLK_CTRL_REG);

	/* enable rv clk */
	//sr32(CCMU_BASE+0x064,31,1,1);
	reg_val = readl(PM_RISCV_CLK_CTRL_REG);
	writel(reg_val | PM_RISCV_CLK_EN_MASK , PM_RISCV_CLK_CTRL_REG);

	/* set clk_ck_c906_div to 480000000; */
	//sr32(CCMU_BASE+0x064, 0, 2, 0x0);
	reg_val = readl(PM_RISCV_CLK_CTRL_REG);
	writel(reg_val & ~PM_RISCV_CLK_DIV_MASK, PM_RISCV_CLK_CTRL_REG);

	//pm_dbg("C906 CPU Freq: %d MHz\n", c906_freq_store);

	/* rv clk gating */
	//sr32(CCMU_BASE+0x014,19,1,1);
	reg_val = readl(PM_CPU_DSP_RV_CLK_GATING_CTRL_REG);
	writel(reg_val | PM_RISCV_CLK_GATING_MASK, PM_CPU_DSP_RV_CLK_GATING_CTRL_REG);

	/* rv clk rst */
	//sr32(CCMU_BASE+0x018,19,1,1);
	reg_val = readl(PM_CPU_DSP_RV_RST_CTRL_REG);
	writel(reg_val | PM_RISCV_CFG_RST_MASK, PM_CPU_DSP_RV_RST_CTRL_REG);

	/* rv sys apb soft rst */
	//sr32(CCMU_BASE+0x018,21,1,1);
	reg_val = readl(PM_CPU_DSP_RV_RST_CTRL_REG);
	writel(reg_val | PM_RISCV_APB_SOFT_RST_MASK, PM_CPU_DSP_RV_RST_CTRL_REG);

	/* rv start address */
#if CONFIG_ARCH_RISCV_START_ADDRESS
	writel(CONFIG_ARCH_RISCV_START_ADDRESS, PM_RISCV_STA_ADDR0_REG);
#else
	writel(CONFIG_ARCH_RISCV_START_ADDRESS, rtos_spare_head.reserved[0]);
#endif

	/* rv core reset */
	//sr32(CCMU_BASE+0x18,16,1,1);
	reg_val = readl(PM_CPU_DSP_RV_RST_CTRL_REG);
	writel(reg_val | PM_RISCV_CORE_RST_MASK, PM_CPU_DSP_RV_RST_CTRL_REG);

	return 0;
}

#define GPRCM_SYS_PRIV_REG0	(0x40050200)
#define RV_SUSPEND_OK		(PM_SUBSYS_ACTION_OK_FLAG)
#define RV_SUSPEND_FAILED	(PM_SUBSYS_ACTION_FAILED_FLAG)
#define RV_RESUME_TRIGGER	(0x0)
#define RV_RESUME_OK		(0x16aa0000)
int riscv_trigger_resume(suspend_mode_t mode)
{
	/* try poweron risc-v */
	int ret;

	/* ensure that c906 core rst is in reset state before open RV_SW
	 * release the core rst after start addr is set
	 * the start addr reg can only be writed in this case:
	 *     1. RV_SW on;
	 *     2. c906 gating on.
	 */
	ret = pm_boot_c906();

	if (ret)
		pm_err("resume boot c906 failed\n");
	else
		writel(RV_RESUME_TRIGGER, GPRCM_SYS_PRIV_REG0);

	return ret;
}

static void riscv_subsys_do_signal(void *arg)
{
	int ret;
	pm_subsys_msg_t msg;
	BaseType_t xReturned;
	pm_subsys_t *subsys = (pm_subsys_t *) arg;

	while (1) {
		if (pdPASS == xQueueReceive(subsys->xQueue_Signal, &msg, portMAX_DELAY)) {
			switch (msg.action) {
			/* about suspend*/
			case PM_SUBSYS_ACTION_TO_SUSPEND:
				if (PM_SUBSYS_STATUS_NORMAL != subsys->status) {
					pm_warn("SUSPEND WARNING: subsys(%s) status is %d, will not be suspened.\n",
						subsys->name ? subsys->name : "NULL", subsys->status);
					msg.action = PM_SUBSYS_RESULT_NOTHING;
					pm_subsys_report_result(PM_SUBSYS_ID_RISCV, &msg);
					break;
				}
				subsys->status = PM_SUBSYS_STATUS_SUSPENDING;
				ret = riscv_trigger_suspend(msg.mode);
				if (ret) {
					subsys->status = PM_SUBSYS_STATUS_NORMAL;
					msg.action = PM_SUBSYS_RESULT_SUSPEND_REJECTED;
					pm_subsys_report_result(PM_SUBSYS_ID_RISCV, &msg);
					pm_err("subsys(%s) trigger suspend(%s) failed(%d).\n",
							subsys->name ? subsys->name : "NULL",
							pm_mode2string(msg.mode),
							ret);
				}
				break;
			/* about resume*/
			case PM_SUBSYS_ACTION_TO_RESUME:
				if (PM_SUBSYS_STATUS_SUSPENDED != subsys->status) {
					pm_warn("RESUME WARNING: subsys(%s) status is %d, do not need to resume.\n",
						subsys->name ? subsys->name : "NULL", subsys->status);
					msg.action = PM_SUBSYS_RESULT_NOTHING;
					pm_subsys_report_result(PM_SUBSYS_ID_RISCV, &msg);
					break;
				}
				subsys->status = PM_SUBSYS_STATUS_RESUMING;
				ret = riscv_trigger_resume(msg.mode);
				if (ret) {
					pm_err("subsys(%s) trigger resume(%s) failed(%d).\n",
							subsys->name ? subsys->name : "NULL",
							pm_mode2string(msg.mode),
							ret);
				}
				break;
			case PM_SUBSYS_ACTION_KEEP_AWAKE:
				subsys->status = PM_SUBSYS_STATUS_KEEP_AWAKE;
				msg.action = PM_SUBSYS_RESULT_NOTHING;
				pm_subsys_report_result(PM_SUBSYS_ID_RISCV, &msg);
				pm_warn("subsys(%s) keeps awake\n", subsys->name ? subsys->name : "NULL");
				break;
			case PM_SUBSYS_ACTION_TO_NORMAL:
				subsys->status = PM_SUBSYS_STATUS_NORMAL;
				msg.action = PM_SUBSYS_RESULT_NOTHING;
				pm_subsys_report_result(PM_SUBSYS_ID_RISCV, &msg);
				pm_log("subsys(%s) change status to normal\n", subsys->name ? subsys->name : "NULL");
				break;
			default:
				break;
			}
		}
	}
}

static pm_subsys_result_t riscv_check_to_suspend_result(void)
{
	if (readl(GPRCM_SYS_PRIV_REG0) == RV_SUSPEND_OK) {
		writel(PM_SUBSYS_ACTION_HAS_GOT_RESULT, GPRCM_SYS_PRIV_REG0);
		return PM_SUBSYS_RESULT_SUSPEND_OK;
	}

	if (readl(GPRCM_SYS_PRIV_REG0) == RV_SUSPEND_FAILED) {
		writel(PM_SUBSYS_ACTION_HAS_GOT_RESULT, GPRCM_SYS_PRIV_REG0);
		return PM_SUBSYS_RESULT_SUSPEND_FAILED;
	}

	/*we read prcm to get result*/
	return PM_SUBSYS_RESULT_NOP;
}

static pm_subsys_result_t riscv_check_to_resume_result(void)
{
	if (readl(GPRCM_SYS_PRIV_REG0) == RV_RESUME_OK) {
		writel(PM_SUBSYS_ACTION_HAS_GOT_RESULT, GPRCM_SYS_PRIV_REG0);
		return PM_SUBSYS_RESULT_RESUME_OK;
	}

	/*we read rtc to get result*/
	return PM_SUBSYS_RESULT_NOP;
}

static void riscv_subsys_do_watch(void *arg)
{
	int ret;
	pm_subsys_msg_t msg;
	pm_subsys_t *subsys = (pm_subsys_t *) arg;

	const TickType_t xFrequency = 10;
	TickType_t xLastWakeTime;


	while (1) {
		xLastWakeTime = xTaskGetTickCount();
		vTaskDelayUntil( &xLastWakeTime, xFrequency );

		switch (subsys->status) {
		case PM_SUBSYS_STATUS_SUSPENDING:
			ret = riscv_check_to_suspend_result();
			switch (ret)  {
				case PM_SUBSYS_RESULT_SUSPEND_OK:
					subsys->status = PM_SUBSYS_STATUS_SUSPENDED;
					msg.action = PM_SUBSYS_RESULT_SUSPEND_OK;
					pm_subsys_report_result(PM_SUBSYS_ID_RISCV, &msg);
					break;
				case PM_SUBSYS_RESULT_SUSPEND_FAILED:
					subsys->status = PM_SUBSYS_STATUS_NORMAL;
					msg.action = PM_SUBSYS_RESULT_SUSPEND_FAILED;
					pm_subsys_report_result(PM_SUBSYS_ID_RISCV, &msg);
					break;
				default:
					break;
			}
			break;
		case PM_SUBSYS_STATUS_RESUMING:
			ret = riscv_check_to_resume_result();
			switch (ret)  {
				case PM_SUBSYS_RESULT_RESUME_OK:
					subsys->status = PM_SUBSYS_STATUS_NORMAL;
					msg.action = PM_SUBSYS_RESULT_RESUME_OK;
					pm_subsys_report_result(PM_SUBSYS_ID_RISCV, &msg);
					break;
				default:
					break;
			}
			break;
		default:
			break;
		}
	}
}

int riscv_subsys_init(void)
{
	int ret = 0;
	BaseType_t xReturned;
	pm_subsys_t *subsys = &riscv_subsys;

	subsys->xQueue_Signal = xQueueCreate(1, sizeof( pm_subsys_msg_t ));
	if (NULL == subsys->xQueue_Signal) {
		pm_err("Create riscv subsys Signal xQueue failed.\n");
		ret = -EPERM;
		goto err_xQueue_Signal;
	}

	subsys->xQueue_Result = xQueueCreate(1, sizeof( pm_subsys_msg_t ));
	if (NULL == subsys->xQueue_Result) {
		pm_err("Create riscv subsys Result xQueue failed.\n");
		ret = -EPERM;
		goto err_xQueue_Result;
	}

	xReturned = xTaskCreate(
			riscv_subsys_do_signal,
			"riscv_subsys_signal",
			(1 * 1024) / sizeof(StackType_t),
			(void *)subsys,
			PM_TASK_PRIORITY,
			(TaskHandle_t * const)&subsys->xHandle_Signal);

	if (pdPASS != xReturned) {
		pm_err("create riscv signal thread failed\n");
		ret = -EPERM;
		goto err_xHandle_Signal;
	}


	xReturned = xTaskCreate(
			riscv_subsys_do_watch,
			"riscv_subsys_watch",
			(1 * 1024) / sizeof(StackType_t),
			(void *)subsys,
			PM_TASK_PRIORITY,
			(TaskHandle_t * const)&subsys->xHandle_Watch);

	if (pdPASS != xReturned) {
		pm_err("create riscv watch thread failed\n");
		ret = -EPERM;
		goto err_xHandle_Watch;
	}

	/* the task can not freeze. */
	ret = pm_task_register(subsys->xHandle_Signal, PM_TASK_TYPE_PM);
	if (ret) {
		pm_err("register riscv signal pm_task failed\n");
		goto err_Register_Signal_task;
	}

	ret = pm_task_register(subsys->xHandle_Watch, PM_TASK_TYPE_PM);
	if (ret) {
		pm_err("register riscv watch pm_task failed\n");
		goto err_Register_Watch_task;
	}

	/* register subsys driver to pm*/
	ret = pm_subsys_register(PM_SUBSYS_ID_RISCV, subsys);
	if (ret) {
		pm_err("register riscv subsys failed\n");
		goto err_Register_subsys_dsp;
	}

	goto out;

err_Register_subsys_dsp:
	pm_task_unregister(subsys->xHandle_Watch);
err_Register_Watch_task:
	pm_task_unregister(subsys->xHandle_Signal);
err_Register_Signal_task:
	vTaskDelete(subsys->xHandle_Watch);
err_xHandle_Watch:
	vTaskDelete(subsys->xHandle_Signal);
err_xHandle_Signal:
	vQueueDelete(subsys->xQueue_Result);
err_xQueue_Result:
	vQueueDelete(subsys->xQueue_Signal);
err_xQueue_Signal:
out:
	return ret;
}


int pm_riscv_init(void)
{
	pm_notify_register(&riscv_notify);

	riscv_subsys_init();
	return 0;
}


