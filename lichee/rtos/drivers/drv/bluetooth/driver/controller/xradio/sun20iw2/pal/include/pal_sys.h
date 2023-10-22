/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  System hooks.
 *
 *  Copyright (c) 2016-2019 Arm Ltd. All Rights Reserved.
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

#ifndef PAL_SYS_H
#define PAL_SYS_H

#include "pal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*! \addtogroup PAL_SYS
 *  \{ */

/**************************************************************************************************
  Macros
**************************************************************************************************/

/* Common error handling routines; for use with PAL implementation only. */

#ifdef DEBUG

/*! \brief      Parameter check. */
#define PAL_SYS_ASSERT(expr)     \
	do {                         \
		if (!(expr)) {           \
			PalSysAssertTrap();  \
		}                        \
	} while (0)

#else

/*! \brief      Parameter check (disabled). */
#define PAL_SYS_ASSERT(expr)

#endif

#define PAL_CS_ALL_INTR                 0x01
#define PAL_CS_BT_INTR                  0x02
#define PAL_CS_BREDR_INTR               0x04
#define PAL_CS_BLE_INTR                 0x06
#define PAL_CS_RTOS_CRITICAL            0x08
#define PAL_CS_RTOS_MUTEX               0x10

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/* Initialization */
void PalSysInit(void);

/* Deinit */
void PalSysDeinit(void);

/* Diagnostics */
void PalSysAssertTrap(void);
void PalSysSetTrap(bool_t enable);
uint32_t PalSysGetAssertCount(void);

/* Power Management */
void PalSysSleep(void);
bool_t PalSysIsBusy(void);
void PalSysSetBusy(void);
void PalSysSetIdle(void);

/* Critical Section */
void PalEnterCs(uint8_t cs_type);
void PalExitCs(uint8_t cs_type);

void PalDelayUs(uint32_t delay);
void PalMsleep(uint32_t msecs);

/*! \} */    /* PAL_SYS */

#ifdef __cplusplus
};
#endif

#endif  /* PAL_SYS_H */
