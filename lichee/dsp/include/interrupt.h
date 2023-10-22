#ifndef __INTERRUPT_H
#define __INTERRUPT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sunxi_hal_common.h>

/*
 * R_INTC interrupt source
 */
#define RINTC_IRQ_MASK			0xffff0000

typedef int (*interrupt_handler_t)(void *);
s32 irq_request(u32 irq_no, interrupt_handler_t hdle, void *arg);
s32 irq_free(u32 irq_no);
s32 irq_enable(u32 irq_no);
s32 irq_disable(u32 irq_no);


#ifdef __cplusplus
}
#endif

#endif /* __INTERRUPT_H */
