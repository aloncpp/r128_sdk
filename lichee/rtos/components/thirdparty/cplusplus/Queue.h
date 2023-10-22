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

#include <stdint.h>
#include <string.h>
#include <queue.h>

namespace AW {

/**
 * The Queue class allow to control, send, receive, or wait for messages.
 * A message can be a integer or pointer value  to a certain type T that is send
 * to a thread or interrupt service routine.
 * @param  T         data type of a single message element.
 * @param  queue_sz  maximum number of messages in queue.
 */
template<typename T, uint32_t queue_sz>
class Queue
{
public:
    /** Create and initialise a message Queue. */
    Queue()
    {
        mID = xQueueCreate( queue_sz, sizeof(T) );
    };

    ~Queue()
    {
        vQueueDelete(mID);
    };

    /** Put a message in a Queue.
      @param   data      message pointer.
      @param   millisec  timeout value or 0 in case of no time-out. (default: 0)
      @return  status code that indicates the execution status of the function.
    */
    portBASE_TYPE put(T& data, int32_t millisec = 0)
    {
        return xQueueSend( mID, &data, millisec);
    }

    /** Get a message or Wait for a message from a Queue.
      @param   millisec  timeout value or 0 in case of no time-out. (default: osWaitForever).
      @return  bool .
    */
    bool get(T& data, int32_t millisec = portMAX_DELAY)
    {
        int32_t tick;

        if (millisec < 0)
            tick = portMAX_DELAY;
        else
            tick = rt_tick_from_millisecond(millisec);

        return xQueueReceive(mID, &data, tick);
    }

private:
    QueueHandle_t mID;
};

}
