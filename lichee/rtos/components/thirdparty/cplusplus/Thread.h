/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2016/10/1      Bernard      The first version
 */

#pragma once

#include <freertos_system.h>

namespace AW
{

/** The Thread class allow defining, creating, and controlling thread functions in the system. */
class Thread
{
public:
    typedef void (*thread_func_t)(void *param);

    /** Allocate a new thread without starting execution
      @param   priority       initial priority of the thread function. (default: osPriorityNormal).
      @param   stack_size      stack size (in bytes) requirements for the thread function. (default: DEFAULT_STACK_SIZE).
      @param   stack_pointer  pointer to the stack area to be used by this thread (default: NULL).
    */
    Thread(uint32_t stack_size = 2048,
           uint8_t  priority = (configMAX_PRIORITIES * 2) / 3,
           uint32_t tick = 20,
           const char *name = "th");

    Thread(void (*entry)(void *p),
           void *p = NULL,
           uint32_t stack_size = 2048,
           uint8_t  priority = (configMAX_PRIORITIES * 2) / 3,
           uint32_t tick = 20,
           const char *name = "th");

    virtual ~Thread();

    bool start();

    static void sleep(int32_t millisec);

    void wait(int32_t millisec);
    void join(int32_t millisec = -1);

protected:
    virtual void run();

private:
    static void func(Thread *pThis);

private:
    TaskHandle_t _thread;

    thread_func_t _entry;
    void *_param;

    /* event for thread join */
    /* struct rt_event _event;*/
    EventGroupHandle_t _event;
    bool started;
};

}
