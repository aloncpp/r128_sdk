/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief      UART driver definition.
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

#include "pal_irq.h"
#include "hal_interrupt.h"
#include "irqs-sun20iw2p1.h"

int PalBtIrqRegister(PAL_IRQ_TYPE type, PalIrqHandler_t handler)
{
	int32_t irq_type;

	switch (type) {
	case PAL_IRQ_TYPE_BLE:
		irq_type = BLE_LL_IRQn;
		break;
	case PAL_IRQ_TYPE_BB:
		irq_type = BTC_BB_IRQn;
		break;
	case PAL_IRQ_TYPE_WLANCOEX:
		irq_type = BTCOEX_IRQn;
		break;
	case PAL_IRQ_TYPE_DBG:
		irq_type = BTC_DBG_IRQn;
		break;
	case PAL_IRQ_TYPE_SLPTMR:
		irq_type = BTC_SLPTMR_IRQn;
		break;
	default:
		return -1;
	}

	hal_request_irq(irq_type, (hal_irq_handler_t)handler, NULL, NULL);
	hal_nvic_irq_set_priority(irq_type, CONFIG_ARCH_ARM_ARMV8M_IRQ_DEFAULT_PRIORITY);
	hal_interrupt_clear_pending(irq_type);
	hal_enable_irq(irq_type);

	return 0;
}

int PalBtIrqUnregister(PAL_IRQ_TYPE type)
{
	int32_t irq_type;

	switch (type) {
	case PAL_IRQ_TYPE_BLE:
		irq_type = BLE_LL_IRQn;
		break;
	case PAL_IRQ_TYPE_BB:
		irq_type = BTC_BB_IRQn;
		break;
	case PAL_IRQ_TYPE_WLANCOEX:
		irq_type = BTCOEX_IRQn;
		break;
	case PAL_IRQ_TYPE_DBG:
		irq_type = BTC_DBG_IRQn;
		break;
	case PAL_IRQ_TYPE_SLPTMR:
		irq_type = BTC_SLPTMR_IRQn;
		break;
	default:
		return -1;
	}

	hal_disable_irq(irq_type);
	hal_interrupt_clear_pending(irq_type);

	return 0;
}

void PalInterruptClearPending(int32_t irq)
{
	hal_interrupt_clear_pending(irq);
}

uint32_t PalInterruptGetNest(void)
{
	return hal_interrupt_get_nest();
}

void PalDisableIrq(int32_t irq)
{
	hal_disable_irq(irq);
}

int32_t PalEnableIrq(int32_t irq)
{
	return hal_enable_irq(irq);
}

