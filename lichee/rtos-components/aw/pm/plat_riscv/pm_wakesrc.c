
#include <osal/hal_interrupt.h>
#include <arch/riscv/sun20iw2p1/irqs-sun20iw2p1.h>

#include <hal_gpio.h>

#include "io.h"
#include <errno.h>

#include "pm_debug.h"
#include "pm_adapt.h"
#include "pm_wakesrc.h"
#include "pm_wakeres.h"
#include "pm_devops.h"
#include "pm_notify.h"
#include "pm_state.h"
#include "pm_wakecnt.h"

#include "hal_prcm.h"

#undef  PM_DEBUG_MODULE
#define PM_DEBUG_MODULE  PM_DEBUG_WAKESRC

#define ws_containerof(ptr_module) \
        __containerof(ptr_module, struct pm_wakesrc_settled, list)

#define INVALID_IRQn -20
static struct pm_wakesrc_settled wakesrc_array[PM_WAKEUP_SRC_MAX] = {
	{PM_WAKEUP_SRC_WKIO0,    AR200A_WUP_IRQn,      "SRC_WKIO0"    , PM_RES_STANDBY_NULL,    }, //0  AR200A_WUP_IRQn,
	{PM_WAKEUP_SRC_WKIO1,    AR200A_WUP_IRQn,      "SRC_WKIO1"    , PM_RES_STANDBY_NULL,    }, //1  AR200A_WUP_IRQn,
	{PM_WAKEUP_SRC_WKIO2,    AR200A_WUP_IRQn,      "SRC_WKIO2"    , PM_RES_STANDBY_NULL,    }, //2  AR200A_WUP_IRQn,
	{PM_WAKEUP_SRC_WKIO3,    AR200A_WUP_IRQn,      "SRC_WKIO3"    , PM_RES_STANDBY_NULL,    }, //3  AR200A_WUP_IRQn,
	{PM_WAKEUP_SRC_WKIO4,    AR200A_WUP_IRQn,      "SRC_WKIO4"    , PM_RES_STANDBY_NULL,    }, //4  AR200A_WUP_IRQn,
	{PM_WAKEUP_SRC_WKIO5,    AR200A_WUP_IRQn,      "SRC_WKIO5"    , PM_RES_STANDBY_NULL,    }, //5  AR200A_WUP_IRQn,
	{PM_WAKEUP_SRC_WKIO6,    AR200A_WUP_IRQn,      "SRC_WKIO6"    , PM_RES_STANDBY_NULL,    }, //6  AR200A_WUP_IRQn,
	{PM_WAKEUP_SRC_WKIO7,    AR200A_WUP_IRQn,      "SRC_WKIO7"    , PM_RES_STANDBY_NULL,    }, //7  AR200A_WUP_IRQn,
	{PM_WAKEUP_SRC_WKIO8,    AR200A_WUP_IRQn,      "SRC_WKIO8"    , PM_RES_STANDBY_NULL,    }, //8  AR200A_WUP_IRQn,
	{PM_WAKEUP_SRC_WKIO9,    AR200A_WUP_IRQn,      "SRC_WKIO9"    , PM_RES_STANDBY_NULL,    }, //9  AR200A_WUP_IRQn,
	{PM_WAKEUP_SRC_WKTMR,    AR200A_WUP_IRQn,      "SRC_WKTMR"    , PM_RES_STANDBY_NULL,    }, //10 AR200A_WUP_IRQn,
	{PM_WAKEUP_SRC_ALARM0,   ALARM0_IRQn,          "SRC_ALARM0"   , PM_RES_STANDBY_NULL,    }, //11 ALARM0_IRQn,
	{PM_WAKEUP_SRC_ALARM1,   ALARM1_IRQn,          "SRC_ALARM1"   , PM_RES_STANDBY_NULL,    }, //12 ALARM1_IRQn,
	{PM_WAKEUP_SRC_WKTIMER0, WKUP_TIMER_IRQ0_IRQn, "SRC_WKTIMER0,", PM_RES_STANDBY_NULL,    }, //13 WKUP_TIMER_IRQ0_IRQn,
	{PM_WAKEUP_SRC_WKTIMER1, WKUP_TIMER_IRQ1_IRQn, "SRC_WKTIMER1,", PM_RES_STANDBY_NULL,    }, //14 WKUP_TIMER_IRQ1_IRQn,
	{PM_WAKEUP_SRC_WKTIMER2, WKUP_TIMER_IRQ2_IRQn, "SRC_WKTIMER2,", PM_RES_STANDBY_NULL,    }, //15 WKUP_TIMER_IRQ2_IRQn,
	{PM_WAKEUP_SRC_LPUART0,  LPUART0_IRQn,         "SRC_LPUART0"  , PM_RES_STANDBY_NULL,    }, //16 LPUART0_IRQn,
	{PM_WAKEUP_SRC_LPUART1,  LPUART1_IRQn,         "SRC_LPUART1"  , PM_RES_STANDBY_NULL,    }, //17 LPUART1_IRQn,
	{PM_WAKEUP_SRC_GPADC,    GPADC_IRQn,           "SRC_GPADC,"   , PM_RES_STANDBY_NULL,    }, //18 GPADC_IRQn,
	{PM_WAKEUP_SRC_MAD,      MAD_WAKE_IRQn,        "SRC_MAD,"     , PM_RES_STANDBY_NULL,    }, //19 MAD_WAKE_IRQn,
	{PM_WAKEUP_SRC_WLAN,     WLAN_IRQn,            "SRC_WLAN"     , PM_RES_STANDBY_NULL,    }, //20 WLAN_IRQn,
	{PM_WAKEUP_SRC_BT,       BTCOEX_IRQn,          "SRC_BT"       , PM_RES_STANDBY_NULL,    }, //21 BTCOEX_IRQn,
	{PM_WAKEUP_SRC_BLE,      BLE_LL_IRQn,          "SRC_BLE"      , PM_RES_STANDBY_NULL,    }, //22 BLE_LL_IRQn,
	{PM_WAKEUP_SRC_GPIOA,    GPIOA_IRQn,           "SRC_GPIOA"    , PM_RES_STANDBY_NULL,    }, //23 GPIOA_IRQn, NO USE
	{PM_WAKEUP_SRC_GPIOB,    GPIOB_IRQn,           "SRC_GPIOB"    , PM_RES_STANDBY_NULL,    }, //24 GPIOB_IRQn,
	{PM_WAKEUP_SRC_GPIOC,    GPIOC_IRQn,           "SRC_GPIOC"    , PM_RES_STANDBY_NULL,    }, //25 GPIOC_IRQn,
	{PM_WAKEUP_SRC_DEVICE,   INVALID_IRQn,         "SRC_DEVICE"   , PM_RES_STANDBY_NULL,    }, //26 all active device or IRQn,
};

struct pm_wakesrc_settled *pm_wakesrc_get_by_id(wakesrc_id_t id)
{
	if (!wakesrc_id_valid(id))
		return NULL;

	return &wakesrc_array[id];
}

int pm_riscv_wakesrc_init(void)
{
	return 0;
}
