#ifndef __STANDBY_WAKEUP_H
#define __STANDBY_WAKEUP_H
#include <stdint.h>
#include <FreeRTOS.h>

#define STANDBY_WAKEUP_TASK_NAME 	"standby_wakeup"
#define TASK_STANDBY_WAKEUP_STACK_LEN 	1024

#define STANDBY_WAKEUP_POWER  1
#define STANDBY_WAKEUP_IR     2
#define STANDBY_WAKEUP_OTHERS 3
#define STANDBY_WAKEUP_MAD    4
#define STANDBY_WAKEUP_TEXT   5
#define STANDBY_START_WAKEUP  6
#define STANDBY_STOP_WAKEUP   7

/* use this to emit wakeup event */
void standby_emit_wakeup(uint8_t event);
void standby_emit_wakeup_isr(uint8_t event, BaseType_t *pxHigherPriorityTaskWoken);
void task_standby_wakeup(void *parg);

#endif /* __STANDBY_WAKEUP_H */

