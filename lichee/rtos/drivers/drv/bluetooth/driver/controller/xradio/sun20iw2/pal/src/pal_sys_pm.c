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
#include "pm_base.h"
#include "pm_devops.h"
#include "pm_task.h"
#include "pm_wakecnt.h"
#include "pm_wakelock.h"
#include "pm_wakesrc.h"
#include "pm_subsys.h"
#include "ccu-sun20iw2-aon.h"
#include "irqs-sun20iw2p1.h"
#include "pal_dbg_io.h"
#include "pal_sys_pm.h"

int32_t PalPmDevopsRegister(struct pal_pm_device *dev)
{
	return pm_devops_register((struct pm_device *)dev);
}

int32_t PalPmDevopsUnregister(struct pal_pm_device *dev)
{
	return pm_devops_unregister((struct pm_device *)dev);
}

int32_t PalPmTaskRegister(PalOsThreadHandle_t xHandle, pal_pm_task_type_t type)
{
	return pm_task_register(xHandle, (pm_task_type_t)type);
}

int32_t PalPmTaskUnregister(PalOsThreadHandle_t xHandle)
{
	return pm_task_unregister(xHandle);
}

void PalPmWakecntInc(int32_t irq)
{
	pm_wakecnt_inc(irq);
}

void PalPmStayAwake(int32_t irq)
{
	pm_stay_awake(irq);
}

void PalPmRelax(int32_t irq, pal_pm_relax_type_t wakeup)
{
	pm_relax(irq, (pm_relax_type_t)wakeup);
}

void PalPmWakelocksSetname(struct pal_wakelock *wl, const char *name)
{
	pm_wakelocks_setname((struct wakelock *)wl, name);
}

int32_t PalPmWakelockAcquire(struct pal_wakelock *wl, enum pal_pm_wakelock_t type, uint32_t timeout)
{
	return pm_wakelocks_acquire((struct wakelock *)wl, (enum pm_wakelock_t)type, timeout);
}

int32_t PalPmWakelockRelease(struct pal_wakelock *wl)
{
	return pm_wakelocks_release((struct wakelock *)wl);
}

uint32_t PalPmSubsysCheckInStatus(pal_pm_subsys_status_t status)
{
	return pm_subsys_check_in_status((pm_subsys_status_t)status);
}

int32_t PalPmWakesrcRegister(const int32_t irq, const char *name, const uint32_t type)
{
	return pm_wakesrc_register(irq, name, type);
}

int32_t PalPmWakesrcUnregister(int32_t irq)
{
	return pm_wakesrc_unregister(irq);
}

int32_t PalPmSetWakeirq(const int32_t irq)
{
	return pm_set_wakeirq(irq);
}
int32_t PalPmClearWakeirq(const int32_t irq)
{
	return pm_clear_wakeirq(irq);
}
