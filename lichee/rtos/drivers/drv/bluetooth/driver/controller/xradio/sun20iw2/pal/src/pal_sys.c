/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  System hooks.
 *
 *  Copyright (c) 2013-2019 Arm Ltd. All Rights Reserved.
 *
 *  Copyright (c) 2019-2020 Packetcraft, Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
/*************************************************************************************************/

#include "pal_sys.h"
#include "pal_dbg_io.h"
#include "pal_uart.h"

#include "kernel/os/os.h"
#include "pal_os.h"
#include <string.h>

#include "sys/interrupt.h"
#include "irqs-sun20iw2p1.h"
#include "compiler.h"
#include "interrupt.h"
#include "hal_interrupt.h"
#include "hal_time.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/


/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! \brief      Number of assertions. */
static uint32_t palSysAssertCount;

/*! \brief      Trap enabled flag. */
static volatile bool_t PalSysAssertTrapEnable;

static uint32_t palSysBusyCount;
static uint32_t isr_count_bt;
static uint32_t isr_count_ble;
static uint32_t g_irq_cnt;

static PalOsMutex_t csMutex;

/**************************************************************************************************
  Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Enter a critical section.
 */
/*************************************************************************************************/
__intr_text
__weak void arch_irq_disable_nested(void)
{
	arch_irq_disable();
	++g_irq_cnt;
}

__intr_text
__weak void arch_irq_enable_nested(void)
{
	if (g_irq_cnt > 0) {
		--g_irq_cnt;
	}

	if (g_irq_cnt == 0) {
		arch_irq_enable();
	}
}

__intr_text
void PalEnterCs(uint8_t cs_type)
{
	switch (cs_type) {
	case PAL_CS_BT_INTR:
		arch_irq_disable_nested();
		hal_disable_irq(BTC_BB_IRQn);
		hal_disable_irq(BTCOEX_IRQn);
		hal_disable_irq(BLE_LL_IRQn);
		isr_count_bt++;
		arch_irq_enable_nested();
		break;

	case PAL_CS_BREDR_INTR:
		hal_disable_irq(BTC_BB_IRQn);
		break;

	case PAL_CS_BLE_INTR:
		arch_irq_disable_nested();
		hal_disable_irq(BLE_LL_IRQn);
		isr_count_ble++;
		arch_irq_enable_nested();
		break;

	case PAL_CS_RTOS_CRITICAL:
		taskENTER_CRITICAL();
		break;

	case PAL_CS_RTOS_MUTEX:
		PalOsMutexLock(&csMutex, PAL_OS_WAIT_FOREVER);
		break;

	case PAL_CS_ALL_INTR:
		arch_irq_disable_nested();
		break;

	default:
		arch_irq_disable_nested();
		break;
	}
}

/*************************************************************************************************/
/*!
 *  \brief  Exit a critical section.
 */
/*************************************************************************************************/
__intr_text
void PalExitCs(uint8_t cs_type)
{
	switch (cs_type) {
	case PAL_CS_BT_INTR:
		arch_irq_disable_nested();
		if (isr_count_bt != 0) {
			isr_count_bt--;
			if (isr_count_bt == 0) {
				hal_enable_irq(BTC_BB_IRQn);
				hal_enable_irq(BTCOEX_IRQn);
				hal_enable_irq(BLE_LL_IRQn);
			}
		}
		arch_irq_enable_nested();
		break;

	case PAL_CS_BREDR_INTR:
		hal_enable_irq(BTC_BB_IRQn);
		break;

	case PAL_CS_BLE_INTR:
		arch_irq_disable_nested();
		if (isr_count_ble != 0) {
			isr_count_ble--;
			if (isr_count_ble == 0) {
				hal_enable_irq(BLE_LL_IRQn);
			}
		}
		arch_irq_enable_nested();
		break;

	case PAL_CS_RTOS_CRITICAL:
		taskEXIT_CRITICAL();
		break;

	case PAL_CS_RTOS_MUTEX:
		PalOsMutexUnlock(&csMutex);
		break;

	case PAL_CS_ALL_INTR:
		arch_irq_enable_nested();
		break;

	default:
		arch_irq_enable_nested();
		break;
	}
}

/*************************************************************************************************/
/*!
 *  \brief      Common platform initialization.
 */
/*************************************************************************************************/
void PalSysInit(void)
{
	palSysAssertCount = 0;
	PalSysAssertTrapEnable = FALSE;
	palSysBusyCount = 0;

	PalOsMutexCreate(&csMutex);

	/* Initialization GPIO for bt controller. */
	PalIoInit();
}

/*************************************************************************************************/
/*!
 *  \brief      Common platform deinit.
 */
/*************************************************************************************************/
void PalSysDeinit(void)
{
	palSysAssertCount = 0;
	PalSysAssertTrapEnable = FALSE;
	palSysBusyCount = 0;

	if (csMutex.handle != NULL) {
		PalOsMutexDelete(&csMutex);
	}

	PalIoDeInit();
}

/*************************************************************************************************/
/*!
 *  \brief      System fault trap.
 */
/*************************************************************************************************/
void PalSysAssertTrap(void)
{
	PalIoOn(PAL_DBG_IO_ID_ERROR);

	palSysAssertCount++;

	while (PalSysAssertTrapEnable)
		;
}

/*************************************************************************************************/
/*!
 *  \brief      Set system trap.
 *
 *  \param      enable    Enable assert trap or not.
 */
/*************************************************************************************************/
void PalSysSetTrap(bool_t enable)
{
	PalSysAssertTrapEnable = enable;
}

/*************************************************************************************************/
/*!
 *  \brief      Get assert count.
 */
/*************************************************************************************************/
uint32_t PalSysGetAssertCount(void)
{
	return palSysAssertCount;
}

/*************************************************************************************************/
/*!
 *  \brief      System sleep.
 */
/*************************************************************************************************/
void PalSysSleep(void)
{
	/* Clock management for low power mode. */
#if BB_CLK_RATE_HZ == 32768
	uint32_t rtcNow = XR_SLPTMR->COUNTER;

	if ((BbGetCurrentBod() == NULL) && PalUartGetState(PAL_UART_ID_CHCI) == PAL_UART_STATE_UNINIT) {
		if ((PalTimerGetState() == PAL_TIMER_STATE_BUSY &&
		    ((XR_SLPTMR->CC[3] - rtcNow) & PAL_MAX_RTC_COUNTER_VAL) > PAL_HFCLK_OSC_SETTLE_TICKS) ||
		    (PalTimerGetState() == PAL_TIMER_STATE_READY)) {
			/* disable HFCLK */
		}
	}
#endif

	/* CPU sleep. */
#ifdef __IAR_SYSTEMS_ICC__
	__wait_for_interrupt();
#endif
#ifdef __GNUC__
	__asm volatile ("wfi");
#endif
#ifdef __CC_ARM
	__wfi();
#endif
}

/*************************************************************************************************/
/*!
 *  \brief      Check if system is busy.
 *
 *  \return     TRUE if system is busy.
 */
/*************************************************************************************************/
bool_t PalSysIsBusy(void)
{
	bool_t sysIsBusy = FALSE;
	volatile unsigned long flags;

	flags = arch_irq_save();

	sysIsBusy = ((palSysBusyCount == 0) ? FALSE : TRUE);

	arch_irq_restore(flags);

	return sysIsBusy;
}

/*************************************************************************************************/
/*!
 *  \brief      Set system busy.
 *
 *  \return     none.
 */
/*************************************************************************************************/
void PalSysSetBusy(void)
{
	volatile unsigned long flags;

	flags = arch_irq_save();
	palSysBusyCount++;
	arch_irq_restore(flags);
}

/*************************************************************************************************/
/*!
 *  \brief      Set system idle.
 */
/*************************************************************************************************/
void PalSysSetIdle(void)
{
	volatile unsigned long flags;

	flags = arch_irq_save();
	if (palSysBusyCount) {
		palSysBusyCount--;
	}
	arch_irq_restore(flags);
}

/*************************************************************************************************/
/*!
*  \brief      common delay us
*
*  \param      delay    delay us
*
*/
/*************************************************************************************************/
void PalDelayUs(uint32_t delay)
{
	hal_udelay(delay);
}

/*************************************************************************************************/
/*!
*  \brief      common sleep ms
*
*  \param      msecs    sleep ms
*
*/
/*************************************************************************************************/
void PalMsleep(uint32_t msecs)
{
	hal_msleep(msecs);
}
