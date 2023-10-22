/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief      irq driver definition.
 *
 *  Copyright (c) 2018 ARM Ltd. All Rights Reserved.
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

#ifndef _PAL_IRQ_H
#define _PAL_IRQ_H

#include "pal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************************************
  Data Types
**************************************************************************************************/

typedef enum {
	PAL_IRQ_TYPE_BLE,
	PAL_IRQ_TYPE_BB,
	PAL_IRQ_TYPE_WLANCOEX,
	PAL_IRQ_TYPE_DBG,
	PAL_IRQ_TYPE_SLPTMR
} PAL_IRQ_TYPE;

typedef int (*PalIrqHandler_t)(void);

int PalBtIrqRegister(PAL_IRQ_TYPE type, PalIrqHandler_t handler);

int PalBtIrqUnregister(PAL_IRQ_TYPE type);

void PalInterruptClearPending(int32_t irq);
uint32_t PalInterruptGetNest(void);
void PalDisableIrq(int32_t irq);
int32_t PalEnableIrq(int32_t irq);


#ifdef __cplusplus
};
#endif

#endif /* _PAL_IRQ_H */
