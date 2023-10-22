#ifndef __ARCH_TIMER_H
#define __ARCH_TIMER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <interrupt.h>

int32_t hal_arch_timer_init(int32_t num, interrupt_handler_t func, void *arg);
int32_t hal_arch_timer_deinit(int32_t num);
int32_t hal_arch_timer_start(int32_t num, uint32_t interval);
int32_t hal_arch_timer_stop(int32_t num);
int32_t hal_arch_timer_update(int32_t num, uint32_t interval);


#ifdef __cplusplus
}
#endif

#endif /* __ARCH_TIMER_H */
