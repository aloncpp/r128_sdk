/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 */
#include "Semaphore.h"

using namespace AW;

Semaphore::Semaphore(const char *name, int32_t count)
{
    mID = xSemaphoreCreateCounting(count, 0);
}

bool Semaphore::wait(int32_t millisec)
{
    int32_t tick;

    if (millisec < 0)
        tick = portMAX_DELAY;
    else
        tick = rt_tick_from_millisecond(millisec);

    return xSemaphoreTake(mID, tick) == RT_EOK;
}

void Semaphore::release(void)
{
    xSemaphoreGive(mID);
}

Semaphore::~Semaphore()
{
    vSemaphoreDelete(mID);
}
