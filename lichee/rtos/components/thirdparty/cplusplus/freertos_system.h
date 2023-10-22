
#ifndef FREERTOS_SYSTEM_H
#define FREERTOS_SYSTEM_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <FreeRTOS.h>
#include <portmacro.h>
#include <FreeRTOSConfig.h>
#include <FreeRTOS.h>
#include <task.h>
#include <port_misc.h>
#include <queue.h>
#include <semphr.h>
#include <event_groups.h>

#define RT_EOK  0

#define RT_WEAK __attribute__((weak))

int32_t rt_tick_from_millisecond(int32_t millisec);

#endif  /*FREERTOS_SYSTEM_H*/
