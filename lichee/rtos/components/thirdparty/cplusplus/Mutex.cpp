/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 */
#include "Mutex.h"

using namespace AW;


Mutex::Mutex(const char *name)
{
    mID = xSemaphoreCreateMutex();
}

bool Mutex::lock(int32_t millisec)
{
    int32_t tick;

    if (millisec < 0)
        tick = portMAX_DELAY;
    else
        tick = rt_tick_from_millisecond(millisec);

    return xSemaphoreTake(mID, tick) == RT_EOK;
}

bool Mutex::trylock()
{
    return lock(0);
}

void Mutex::unlock()
{
    xSemaphoreGive(mID);
}

Mutex::~Mutex()
{
    vSemaphoreDelete(mID);
}
