/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief      System PM implementation.
 *
 *  Copyright (c) 2020-2021 Xradio, Inc.
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

#ifndef PAL_SYS_PM_H
#define PAL_SYS_PM_H

#include "pal_os.h"

#define PAL_BTC_SLPTMR_IRQn (9)
#define PAL_BTC_BB_IRQn     (110)
#define PAL_BLE_LL_IRQn     (114)

typedef enum {
	PAL_PM_WAKESRC_ALWAYS_WAKEUP = 0,
	PAL_PM_WAKESRC_MIGHT_WAKEUP,
	PAL_PM_WAKESRC_SOFT_WAKEUP,

	PAL_PM_WAKESRC_TYPE_MAX,
	PAL_PM_WAKESRC_TYPE_BASE = PAL_PM_WAKESRC_ALWAYS_WAKEUP,
} pal_wakesrc_type_t;


typedef enum {
	PAL_PM_RELAX_SLEEPY = 0,
	PAL_PM_RELAX_WAKEUP,

	PAL_PM_RELAX_MAX,
	PAL_PM_RELAX_BASE = PAL_PM_RELAX_SLEEPY,
} pal_pm_relax_type_t;

typedef enum {
	PAL_PM_TASK_TYPE_APP = 0,

	PAL_PM_TASK_TYPE_PM,
	PAL_PM_TASK_TYPE_SYS,
	PAL_PM_TASK_TYPE_DSP,
	PAL_PM_TASK_TYPE_RISCV,
	PAL_PM_TASK_TYPE_BT,
	PAL_PM_TASK_TYPE_WLAN,

	PAL_PM_TASK_TYPE_MAX,

	PAL_PM_TASK_TYPE_BASE = PAL_PM_TASK_TYPE_PM,
} pal_pm_task_type_t;

typedef enum {
	PAL_PM_MODE_ON = 0,
	PAL_PM_MODE_SLEEP,
	PAL_PM_MODE_STANDBY,
	PAL_PM_MODE_HIBERNATION,

	PAL_PM_MODE_MAX,
	PAL_PM_MODE_BASE = PAL_PM_MODE_ON,
} pal_suspend_mode_t;

enum pal_pm_wakelock_t {
	PAL_PM_WL_TYPE_UNKOWN = 0,
	PAL_PM_WL_TYPE_WAIT_ONCE,
	PAL_PM_WL_TYPE_WAIT_INC,
	PAL_PM_WL_TYPE_WAIT_TIMEOUT,
};

typedef enum {
	PAL_PM_SUBSYS_STATUS_NORMAL = 0,
	PAL_PM_SUBSYS_STATUS_SUSPENDING,
	PAL_PM_SUBSYS_STATUS_SUSPENDED,
	PAL_PM_SUBSYS_STATUS_RESUMING,
	PAL_PM_SUBSYS_STATUS_ERROR,
	PAL_PM_SUBSYS_STATUS_KEEP_AWAKE,
} pal_pm_subsys_status_t;

struct pal_pm_device;
struct pal_pm_devops {
	int32_t (*prepared) (struct pal_pm_device *dev, pal_suspend_mode_t mode);
	int32_t (*suspend) (struct pal_pm_device *dev, pal_suspend_mode_t mode);
	int32_t (*suspend_late) (struct pal_pm_device *dev, pal_suspend_mode_t mode);
	int32_t (*suspend_noirq) (struct pal_pm_device *dev, pal_suspend_mode_t mode);
	int32_t (*resume_noirq) (struct pal_pm_device *dev, pal_suspend_mode_t mode);
	int32_t (*resume_early) (struct pal_pm_device *dev, pal_suspend_mode_t mode);
	int32_t (*resume) (struct pal_pm_device *dev, pal_suspend_mode_t mode);
	int32_t (*complete) (struct pal_pm_device *dev, pal_suspend_mode_t mode);
};

struct pal_list_head {
	struct pal_list_head *next, *prev;
};

struct pal_pm_device {
	const char       *name;
	struct pal_list_head  node;
	struct pal_pm_devops  *ops;
	void             *data;
};

struct pal_wakelock {
	const char              *name;

	/* pm core use, do not edit */
	struct pal_list_head        node;
	uint32_t                expires;
	uint16_t                ref;
	uint16_t                type;
};

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/
/* pm */
int32_t PalPmDevopsRegister(struct pal_pm_device *dev);
int32_t PalPmDevopsUnregister(struct pal_pm_device *dev);
int32_t PalPmTaskRegister(PalOsThreadHandle_t xHandle, pal_pm_task_type_t type);
int32_t PalPmTaskUnregister(PalOsThreadHandle_t xHandle);
void PalPmWakecntInc(int32_t irq);
void PalPmStayAwake(int32_t irq);
void PalPmRelax(int32_t irq, pal_pm_relax_type_t wakeup);
void PalPmWakelocksSetname(struct pal_wakelock *wl, const char *name);
int32_t PalPmWakelockAcquire(struct pal_wakelock *wl, enum pal_pm_wakelock_t type, uint32_t timeout);
int32_t PalPmWakelockRelease(struct pal_wakelock *wl);
uint32_t PalPmSubsysCheckInStatus(pal_pm_subsys_status_t status);

/* wakesrc */
int32_t PalPmWakesrcRegister(const int32_t irq, const char *name, const uint32_t type);
int32_t PalPmWakesrcUnregister(int32_t irq);
int32_t PalPmSetWakeirq(const int32_t irq);
int32_t PalPmClearWakeirq(const int32_t irq);

#endif /*PAL_SYS_PM_H*/
