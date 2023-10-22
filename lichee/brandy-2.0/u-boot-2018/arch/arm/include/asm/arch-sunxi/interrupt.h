/*
 * Allwinnertech rtos interrupt header file.
 * Copyright (C) 2019  Allwinnertech Co., Ltd. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */


#ifndef __INTERRUPT_H
#define __INTERRUPT_H

#include "aw_types.h"

//#include "interrupt_gic.h"
#include "interrupt_nvic.h"
#include "irqs.h"

//#include "hal_interrupt.h"
/*
 * keep return value for compatibility,
 * driver shoule return one,
 * but os may don't care it
 */
enum hal_irqreturn {
    HAL_IRQ_OK      = (0 << 0),
    HAL_IRQ_ERR     = (1 << 0),
};
typedef enum hal_irqreturn hal_irqreturn_t;
typedef hal_irqreturn_t (*hal_irq_handler_t) (void *data);


//typedef void (*pIRQ_Handler)(void);
typedef int (*irq_handler_t)(void *);

irq_handler_t irq_get_handler(uint32_t irq_no);
s32 irq_request(u32 irq_no, irq_handler_t  hdle, void *data);
s32 irq_free(u32 irq_no);
s32 irq_enable(u32 irq_no);
s32 irq_disable(u32 irq_no);
void irq_init(void);


int32_t arch_request_irq(int32_t irq, hal_irq_handler_t handler, void *data);
void arch_free_irq(int32_t irq);
void arch_enable_irq(int32_t irq);
void arch_disable_irq(int32_t irq);
void arch_irq_set_prioritygrouping(int32_t group);
uint32_t arch_irq_get_prioritygrouping(void);
void arch_irq_set_priority(int32_t irq, uint32_t preemptpriority, uint32_t subpriority);
void arch_irq_get_priority(int32_t irq, uint32_t prioritygroup, uint32_t *p_preemptpriority, uint32_t *p_subpriority);
void arch_nvic_irq_set_priority(int32_t irq, uint32_t priority);
uint32_t arch_nvic_irq_get_priority(int32_t irq);
void arch_enable_all_irq(void);
void arch_disable_all_irq(void);
unsigned long arch_irq_is_disable(void);

#endif

