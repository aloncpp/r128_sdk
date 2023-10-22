
#include <osal/hal_interrupt.h>
#include <arch/arm/mach/sun20iw2p1/irqs-sun20iw2p1.h>

#include <hal_gpio.h>
#include <hal_atomic.h>
#include <console.h>
#include <io.h>
#include <errno.h>

#include "pm_debug.h"
#include "pm_adapt.h"
#include "pm_wakesrc.h"
#include "pm_wakeres.h"
#include "pm_devops.h"
#include "pm_notify.h"
#include "pm_state.h"
#include "pm_wakecnt.h"
#include "pm_m33_wakesrc.h"
#include "pm_m33_platops.h"

#include "hal_prcm.h"

#undef  PM_DEBUG_MODULE
#define PM_DEBUG_MODULE  PM_DEBUG_WAKESRC

#define ws_containerof(ptr_module) \
        __containerof(ptr_module, struct pm_wakesrc_settled, list)

/* record all register and active wakeupsrc*/
static uint32_t wakesrc_active = 0;
/* record which wakesrc has been take over. set to 1 means m33 has take over the irq. */
static uint32_t wakesrc_rvdsp_active = 0;

/*
 * affinity cpu:
 * 0: M33
 * 1: DSP/RSICV
 */
/* record all wakesrc that are not on m33. */
static uint32_t wakesrc_affinity = 0;
/* mask wakesrc that are not on m33 */
static uint32_t wakesrc_affinity_mask = 0;
/* record wakesrc on dsp */
static uint32_t wakesrc_dsp_affinity = 0;
/* record wakesrc on riscv */
static uint32_t wakesrc_riscv_affinity = 0;

/*last event that wakeup system*/
static volatile uint32_t wakesrc_event  = 0;
static volatile int pm_wakeup_irq  = INVALID_IRQn;
static volatile uint8_t pm_suspend_abort = 0;

extern uint32_t arch_nvic_get_enable_irq(int32_t irq);

static struct pm_wakesrc_settled wakesrc_array[PM_WAKEUP_SRC_MAX] = {
	{PM_WAKEUP_SRC_WKIO0,    AR200A_WUP_IRQn,      "SRC_WKIO0"    , PM_RES_STANDBY_WUP,    }, //0  AR200A_WUP_IRQn,
	{PM_WAKEUP_SRC_WKIO1,    AR200A_WUP_IRQn,      "SRC_WKIO1"    , PM_RES_STANDBY_WUP,    }, //1  AR200A_WUP_IRQn,
	{PM_WAKEUP_SRC_WKIO2,    AR200A_WUP_IRQn,      "SRC_WKIO2"    , PM_RES_STANDBY_WUP,    }, //2  AR200A_WUP_IRQn,
	{PM_WAKEUP_SRC_WKIO3,    AR200A_WUP_IRQn,      "SRC_WKIO3"    , PM_RES_STANDBY_WUP,    }, //3  AR200A_WUP_IRQn,
	{PM_WAKEUP_SRC_WKIO4,    AR200A_WUP_IRQn,      "SRC_WKIO4"    , PM_RES_STANDBY_WUP,    }, //4  AR200A_WUP_IRQn,
	{PM_WAKEUP_SRC_WKIO5,    AR200A_WUP_IRQn,      "SRC_WKIO5"    , PM_RES_STANDBY_WUP,    }, //5  AR200A_WUP_IRQn,
	{PM_WAKEUP_SRC_WKIO6,    AR200A_WUP_IRQn,      "SRC_WKIO6"    , PM_RES_STANDBY_WUP,    }, //6  AR200A_WUP_IRQn,
	{PM_WAKEUP_SRC_WKIO7,    AR200A_WUP_IRQn,      "SRC_WKIO7"    , PM_RES_STANDBY_WUP,    }, //7  AR200A_WUP_IRQn,
	{PM_WAKEUP_SRC_WKIO8,    AR200A_WUP_IRQn,      "SRC_WKIO8"    , PM_RES_STANDBY_WUP,    }, //8  AR200A_WUP_IRQn,
	{PM_WAKEUP_SRC_WKIO9,    AR200A_WUP_IRQn,      "SRC_WKIO9"    , PM_RES_STANDBY_WUP,    }, //9  AR200A_WUP_IRQn,
	{PM_WAKEUP_SRC_WKTMR,    AR200A_WUP_IRQn,      "SRC_WKTMR"    , PM_RES_STANDBY_WUP,    }, //10 AR200A_WUP_IRQn,
	{PM_WAKEUP_SRC_ALARM0,   ALARM0_IRQn,          "SRC_ALARM0"   , PM_RES_STANDBY_RTC,    }, //11 ALARM0_IRQn,
	{PM_WAKEUP_SRC_ALARM1,   ALARM1_IRQn,          "SRC_ALARM1"   , PM_RES_STANDBY_RTC,    }, //12 ALARM1_IRQn,
	{PM_WAKEUP_SRC_WKTIMER0, WKUP_TIMER_IRQ0_IRQn, "SRC_WKTIMER0,", PM_RES_STANDBY_WUPTMR, }, //13 WKUP_TIMER_IRQ0_IRQn,
	{PM_WAKEUP_SRC_WKTIMER1, WKUP_TIMER_IRQ1_IRQn, "SRC_WKTIMER1,", PM_RES_STANDBY_WUPTMR, }, //14 WKUP_TIMER_IRQ1_IRQn,
	{PM_WAKEUP_SRC_WKTIMER2, WKUP_TIMER_IRQ2_IRQn, "SRC_WKTIMER2,", PM_RES_STANDBY_WUPTMR, }, //15 WKUP_TIMER_IRQ2_IRQn,
	{PM_WAKEUP_SRC_LPUART0,  LPUART0_IRQn,         "SRC_LPUART0"  , PM_RES_STANDBY_LPUART, }, //16 LPUART0_IRQn,
	{PM_WAKEUP_SRC_LPUART1,  LPUART1_IRQn,         "SRC_LPUART1"  , PM_RES_STANDBY_LPUART, }, //17 LPUART1_IRQn,
	{PM_WAKEUP_SRC_GPADC,    GPADC_IRQn,           "SRC_GPADC,"   , PM_RES_STANDBY_GPADC,  }, //18 GPADC_IRQn,
	{PM_WAKEUP_SRC_MAD,      MAD_WAKE_IRQn,        "SRC_MAD,"     , PM_RES_STANDBY_MAD,    }, //19 MAD_WAKE_IRQn,
	{PM_WAKEUP_SRC_WLAN,     WLAN_IRQn,            "SRC_WLAN"     , PM_RES_STANDBY_WLAN,   }, //20 WLAN_IRQn,
	{PM_WAKEUP_SRC_BT,       BTCOEX_IRQn,	       "SRC_BT"       , PM_RES_STANDBY_BT,     }, //21 BTC_SLPTMR_IRQn,
	{PM_WAKEUP_SRC_BLE,      BTC_SLPTMR_IRQn,      "SRC_BLE"      , PM_RES_STANDBY_BLE,    }, //22 BLE_LL_IRQn,
	{PM_WAKEUP_SRC_GPIOA,    GPIOA_IRQn,           "SRC_GPIOA"    , PM_RES_STANDBY_GPIO,   }, //23 GPIOA_IRQn, NO USE
	{PM_WAKEUP_SRC_GPIOB,    GPIOB_IRQn,           "SRC_GPIOB"    , PM_RES_STANDBY_GPIO,   }, //24 GPIOB_IRQn,
	{PM_WAKEUP_SRC_GPIOC,    GPIOC_IRQn,           "SRC_GPIOC"    , PM_RES_STANDBY_GPIO,   }, //25 GPIOC_IRQn,
	{PM_WAKEUP_SRC_DEVICE,   INVALID_IRQn,         "SRC_DEVICE"   , PM_RES_STANDBY_SLEEP,  }, //26 all active device or IRQn,
};


static hal_irqreturn_t pm_wakesrc_rvdsp_handler(void *data);
static int pm_wakesrc_rvdsp_enable(struct pm_wakesrc_settled *ws);
static int pm_wakesrc_rvdsp_disable(struct pm_wakesrc_settled *ws);

static int pm_wakesrc_rvdsp_enable_allwakeirq(void);
static int pm_wakesrc_rvdsp_disable_allwakeirq(void);

static hal_spinlock_t pm_wakeup_rec_lock = {
	.owner = 0,
	.counter = 0,
	.spin_lock = {
		.slock = 0,
	},
};

static hal_spinlock_t pm_wakesrc_active_lock = {
	.owner = 0,
	.counter = 0,
	.spin_lock = {
		.slock = 0,
	},
};

struct pm_wakesrc_settled *pm_wakesrc_get_by_id(wakesrc_id_t id)
{
	if (!wakesrc_id_valid(id))
		return NULL;

	return &wakesrc_array[id];
}

uint32_t pm_wakesrc_id_get_by_irq(const int irq)
{
	uint32_t val = 0;
	int id;

	for (id = PM_WAKEUP_SRC_BASE; id < PM_WAKEUP_SRC_MAX; id++) {
		if (wakesrc_array[id].irq == irq)
			val |= (0x1 << id);
	}

	return val;
}

static uint8_t pm_irq_is_wakeup_armed(const int irq)
{
	uint32_t val = 0;
	int id;

	for (id = PM_WAKEUP_SRC_BASE; id < PM_WAKEUP_SRC_MAX; id++) {
		if (wakesrc_array[id].irq == irq)
			val = 1;
	}

	return val;
}


uint32_t pm_wakesrc_get_active(void)
{
	return wakesrc_active;
}

uint32_t pm_wakesrc_get_event(void)
{
	return wakesrc_event;
}

uint8_t pm_wakesrc_get_suspend_abort(void)
{
	return pm_suspend_abort;
}

int pm_wakesrc_get_wakeup_irq(void)
{
	return pm_wakeup_irq;
}

void pm_system_wakeup(void)
{
	pm_suspend_abort = 1;
}

void pm_wakesrc_rec_wakeup_irq(const int irq)
{
	int id_sum = 0;;
	struct pm_wakesrc *ws = NULL;

	id_sum = pm_wakesrc_id_get_by_irq(irq);
	if (!id_sum & wakesrc_active)
		return;

	/* Do not lead waste of resources for lock in irq handler */
	if (pm_wakeup_irq == INVALID_IRQn) {
		hal_spin_lock(&pm_wakeup_rec_lock);
		/* Avoid simultaneously determining that the irq is empty */
		if (pm_wakeup_irq == INVALID_IRQn) {
			/* irq num in m33 */
			pm_wakeup_irq = irq;
			wakesrc_event = id_sum & wakesrc_active;
			ws = pm_wakesrc_find_enabled_by_irq(irq);
			if (ws->type == PM_WAKESRC_ALWAYS_WAKEUP) {
				pm_system_wakeup();
#if 0
/* increase by driver itself */
				pm_wakecnt_inc(irq);
#endif
			}
		}
		hal_spin_unlock(&pm_wakeup_rec_lock);
	}
}

void pm_wakesrc_clear_wakeup_irq(void)
{
	hal_spin_lock(&pm_wakeup_rec_lock);
	pm_suspend_abort = 0;
	pm_wakeup_irq = INVALID_IRQn;
	wakesrc_event = 0;
	hal_spin_unlock(&pm_wakeup_rec_lock);
}

static int cmd_pm_wakeup_irq(int argc, char **argv)
{
	int irq;

	irq = pm_wakesrc_get_wakeup_irq();
	if (pm_wakeup_irq == INVALID_IRQn)
		printf("pm_wakeup_irq: NULL\n");
	else
		printf("pm_wakeup_irq: %d\n", pm_wakeup_irq);

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_pm_wakeup_irq, pm_wakeup_irq, pm tools)

static hal_irqreturn_t pm_wakesrc_rvdsp_handler(void *data)
{
	uint32_t event = 0;
	struct pm_wakesrc_settled *ws = data;

	/*close the irq*/
	hal_disable_irq(ws->irq);
	hal_interrupt_clear_pending(ws->irq);

	/* report a event to resume the other core */
	pm_wakecnt_subcnt_inc(ws->irq);

	return HAL_IRQ_OK;
}

static int pm_wakesrc_rvdsp_enable(struct pm_wakesrc_settled *ws)
{
	u32 mask = 0x1<<(ws->id);
	if (!(wakesrc_rvdsp_active & mask)) {
		hal_spin_lock(&pm_wakesrc_active_lock);
		wakesrc_rvdsp_active |= 0x1<<(ws->id);
		hal_spin_unlock(&pm_wakesrc_active_lock);

		hal_request_irq(ws->irq, pm_wakesrc_rvdsp_handler, NULL, ws);
		hal_interrupt_clear_pending(ws->irq);
		hal_enable_irq(ws->irq);
	}

	return 0;
}

static int pm_wakesrc_rvdsp_disable(struct pm_wakesrc_settled *ws)
{
	u32 mask = 0x1<<(ws->id);

	if (wakesrc_rvdsp_active & mask) {
		hal_spin_lock(&pm_wakesrc_active_lock);
		wakesrc_rvdsp_active &= ~mask;
		hal_spin_unlock(&pm_wakesrc_active_lock);

		hal_disable_irq(ws->irq);
		/* clear pending in M33 to avoid suspend abort */
		hal_interrupt_clear_pending(ws->irq);
	}

	return 0;
}

static int pm_wakesrc_rvdsp_enable_allwakeirq(void)
{
	int i = 0;
	int ret = 0;

	uint32_t active_affinity = wakesrc_active & (wakesrc_affinity & ~wakesrc_affinity_mask);


	/* it should be 0 now. */
	pm_abort(wakesrc_rvdsp_active);

	/* 1, we try to register irq-handler for rv/dsp wakeup-source. */
	for (i=PM_WAKEUP_SRC_BASE; i<PM_WAKEUP_SRC_MAX; i++) {
		/* skip wakeup source if disabled or m33 active */
		if (!(active_affinity & (0x1<<i)))
			continue;

		pm_wakesrc_rvdsp_enable(&wakesrc_array[i]);
	}

	pm_log("wakesrc_rvdsp_active: 0x%x\n", wakesrc_rvdsp_active);

	return 0;
}


static int pm_wakesrc_rvdsp_disable_allwakeirq(void)
{
	int i = 0;
	for (i=PM_WAKEUP_SRC_BASE; i<PM_WAKEUP_SRC_MAX; i++) {
		/* skip disabled */
		if (!(wakesrc_rvdsp_active & (0x1<<i)))
			continue;

		pm_wakesrc_rvdsp_disable(&wakesrc_array[i]);
	}

	/* it should be 0 now. */
	pm_abort(wakesrc_rvdsp_active);

	return 0;
}

int pm_wakesrc_prepared(void)
{
	pm_wakesrc_rvdsp_enable_allwakeirq();

	pm_wakeres_update();

	return 0;
}

int pm_wakesrc_complete(void)
{
	pm_wakesrc_rvdsp_disable_allwakeirq();

	return 0;
}

int pm_wakesrc_active(wakesrc_id_t id, wakesrc_affinity_t core)
{
	uint32_t id_sum;

	if (!wakesrc_id_valid(id) || !wakesrc_affinity_valid(core)) {
		pm_invalid();
		return -EINVAL;
	}

	/* pm_wakesrc_settled may share the same irq num */
	id_sum = pm_wakesrc_id_get_by_irq(wakesrc_array[id].irq);
	if (wakesrc_active & id_sum)
		return -EEXIST;

	if (core != PM_WS_AFFINITY_M33)
		pm_wakesrc_update_irq_in_sub_core(id, wakesrc_array[id].irq);

	hal_spin_lock(&pm_wakesrc_active_lock);
	wakesrc_active |= id_sum;
	if (!!core)
		wakesrc_affinity |= id_sum;
	if (core == PM_WS_AFFINITY_RISCV)
		wakesrc_riscv_affinity |= id_sum;
	if (core == PM_WS_AFFINITY_DSP)
		wakesrc_dsp_affinity |= id_sum;
	hal_spin_unlock(&pm_wakesrc_active_lock);

	return 0;
}

int pm_wakesrc_deactive(wakesrc_id_t id, wakesrc_affinity_t core)
{
	uint32_t id_sum;

	if (!wakesrc_id_valid(id) || !wakesrc_affinity_valid(core)) {
		pm_invalid();
		return -EINVAL;
	}

	id_sum = pm_wakesrc_id_get_by_irq(wakesrc_array[id].irq);
	if (!(wakesrc_active & id_sum))
		return -ENODEV;
	else if ((core == PM_WS_AFFINITY_RISCV) && !(wakesrc_riscv_affinity & id_sum))
		return -ENODEV;
	else if ((core == PM_WS_AFFINITY_DSP) && !(wakesrc_dsp_affinity & id_sum))
		return -ENODEV;
	else if ((core == PM_WS_AFFINITY_M33) && (wakesrc_affinity & id_sum))
		return -ENODEV;

	if (core != PM_WS_AFFINITY_M33)
		pm_wakesrc_update_irq_in_sub_core(id, INVAL_IRQ_NUM);

	hal_spin_lock(&pm_wakesrc_active_lock);
	wakesrc_active   &= ~id_sum;
	wakesrc_affinity &= ~id_sum;
	if (core == PM_WS_AFFINITY_RISCV)
		wakesrc_riscv_affinity &= ~id_sum;
	if (core == PM_WS_AFFINITY_DSP)
		wakesrc_dsp_affinity &= ~id_sum;
	hal_spin_unlock(&pm_wakesrc_active_lock);

	return 0;
}

int pm_wakesrc_mask_affinity(wakesrc_affinity_t core)
{
	if (core == PM_WS_AFFINITY_RISCV)
		wakesrc_affinity_mask |= wakesrc_riscv_affinity;
	if (core == PM_WS_AFFINITY_DSP)
		wakesrc_affinity_mask |= wakesrc_dsp_affinity;

	return 0;
}

int pm_wakesrc_unmask_affinity(wakesrc_affinity_t core)
{
	if (core == PM_WS_AFFINITY_RISCV)
		wakesrc_affinity_mask &= ~wakesrc_riscv_affinity;
	if (core == PM_WS_AFFINITY_DSP)
		wakesrc_affinity_mask &= ~wakesrc_dsp_affinity;

	return 0;
}

uint32_t pm_wakesrc_pending_in_affinity(wakesrc_affinity_t core)
{
	uint32_t affinity = 0;

	if (core == PM_WS_AFFINITY_RISCV) {
		affinity = wakesrc_riscv_affinity;
	} else if (core == PM_WS_AFFINITY_DSP)
		affinity = wakesrc_dsp_affinity;

	return (wakesrc_event & wakesrc_rvdsp_active & affinity);
}

/**
 * it could be called when close cpu globe interrupt.
 * check if any wake-up interrupts are pending
 */
uint32_t pm_wakesrc_read_pending(void)
{
	int i = 0;
	volatile uint32_t val = 0;

	/*check wkio, or wktimer in pmu spec*/
	if (hal_interrupt_is_pending(AR200A_WUP_IRQn)) {
		val |= HAL_PMU_WakeupIOGetEventStatus();
		if (HAL_PMU_WakeupTimerIsPending()) {
			val |= 0x1 << PM_WAKEUP_SRC_WKTMR;
		}
	}

	/*check independent interrupt wake-up source */
	for (i=PM_WAKEUP_SRC_BASE; i<PM_WAKEUP_SRC_MAX; i++) {
		if ((wakesrc_array[i].irq == AR200A_WUP_IRQn) \
			|| (wakesrc_array[i].irq == INVALID_IRQn))
			continue;

		if (hal_interrupt_is_pending(wakesrc_array[i].irq))
			val |= 0x1 << wakesrc_array[i].id;
	}

	return val;
}

uint32_t pm_wakesrc_pending_is_for_affinity(wakesrc_affinity_t core)
{
	uint32_t pending_id = 0;
	uint32_t affinity_record = 0;

	pending_id = pm_wakesrc_read_pending();

	switch (core) {
	case PM_WS_AFFINITY_RISCV:
		affinity_record = wakesrc_riscv_affinity;
		break;
	case PM_WS_AFFINITY_DSP:
		affinity_record = wakesrc_dsp_affinity;
		break;
	default:
		break;
	}

	if (affinity_record & pending_id)
		return pending_id;
	
	return 0;
}

/* use in noirq environment */
uint32_t pm_wakesrc_check_irqs(void)
{
	uint32_t event;
	uint32_t irq_disabled = 0;

	/* Assert a irq on ohter core will set the pending regardless it is enabled or not on M33.
	 * Ensure to clear pending between other core suspended and the M33 take over their irqs.
	 * Check enable status before taking over irqs to avoid misjudgment.
	 */
	for (int i = PM_WAKEUP_SRC_BASE; i < PM_WAKEUP_SRC_MAX; i++) {
		if (!arch_nvic_get_enable_irq(wakesrc_array[i].irq))
			irq_disabled |= (0x1 << i);
	}

	event = pm_wakesrc_read_pending();

	return event & wakesrc_active & (~irq_disabled);
}

int pm_wakesrc_set_time_to_wakeup_ms(unsigned int ms)
{
	int ret = 0;

#ifndef CONFIG_PM_SIMULATED_RUNNING
	ret = pm_set_time_to_wakeup_ms(ms);
#endif

	return ret;
}

struct pm_wakeup_ops pm_wakeup_ops_m33 = {
	.check_suspend_abort 	= pm_wakesrc_get_suspend_abort,
	.check_wakeup_event 	= pm_wakesrc_check_irqs,
	.get_wakeup_event	= pm_wakesrc_get_event,
	.get_wakeup_irq		= pm_wakesrc_get_wakeup_irq,
	.irq_is_wakeup_armed	= pm_irq_is_wakeup_armed,
	.record_wakeup_irq	= pm_wakesrc_rec_wakeup_irq,
	.clear_wakeup_irq	= pm_wakesrc_clear_wakeup_irq,
	.system_wakeup 		= pm_system_wakeup,
	.set_time_to_wakeup_ms	= pm_wakesrc_set_time_to_wakeup_ms,
};

int pm_wakeup_ops_m33_init(void)
{
	int ret;

	ret = pm_wakeup_ops_register(&pm_wakeup_ops_m33);
	if (ret) {
		pm_err("m33 register pm_wakeup_ops failed\n");
		return -EFAULT;
	}

	return 0;
}

int pm_m33_wakesrc_init(void)
{

	return 0;
}
