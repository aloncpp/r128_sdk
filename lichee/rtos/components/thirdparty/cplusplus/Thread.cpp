/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 */
#include "Thread.h"

using namespace AW;

Thread::Thread(uint32_t stack_size,
               uint8_t  priority,
               uint32_t tick,
               const char *name)
    : _entry(NULL), _param(NULL), started(true)
{
    portBASE_TYPE ret;
    _event = xEventGroupCreate();

    ret = xTaskCreate((thread_func_t)func, name, stack_size, this, priority, &_thread);
    if(ret != pdPASS)
    {
        printf("Error creating task, status was %d\n", ret);
        return;
    }
}

Thread::Thread(void (*entry)(void *p),
               void *p,
               uint32_t stack_size,
               uint8_t  priority,
               uint32_t tick,
               const char *name)
    : _entry(entry), _param(p), started(true)
{
    portBASE_TYPE ret;
    _event = xEventGroupCreate();

    ret = xTaskCreate((thread_func_t)func, name, stack_size, this, priority, &_thread);
    if (ret != pdPASS) {
        printf("Error creating task, status was %d\n", ret);
        return;
    }
}

Thread::~Thread()
{
    vEventGroupDelete(_event);
    vTaskDelete(_thread);
}

bool Thread::start()
{
    started = true;
    return started;
}

void Thread::sleep(int32_t millisec)
{
    int32_t tick;

    if (millisec < 0)
        tick = 1;
    else
        tick = rt_tick_from_millisecond(millisec);

    vTaskDelay(tick);
}

void Thread::func(Thread *pThis)
{
    if (pThis->_entry != NULL)
    {
        pThis->_entry(pThis->_param);
    }
    else
    {
        pThis->run();
    }

    (void)xEventGroupSetBits(pThis->_event, 1 << 0);
    vTaskDelete(NULL);
}

void Thread::run()
{
    /* please overload this method */
    printf("Thread::run method should be overloaded!\n");
}

void Thread::wait(int32_t millisec)
{
    join(millisec);
}

void Thread::join(int32_t millisec)
{
    if (started)
    {
        int32_t tick;

        if (millisec < 0)
            tick = -1;
        else
            tick = rt_tick_from_millisecond(millisec);

        xEventGroupWaitBits(_event, 1 << 0, pdFALSE, pdFALSE, tick);
    }
}
