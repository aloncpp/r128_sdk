#ifndef _INTERRUPT_H
#define _INTERRUPT_H

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>

#include <hal_interrupt.h>

#define NR_CPUS       1
#define BITS_PER_BYTE 8
#define __KERNEL_DIV_ROUND_UP(n, d)     (((n) + (d) - 1) / (d))
#define BITS_PER_TYPE(type)         (sizeof(type) * BITS_PER_BYTE)
#ifndef BITS_TO_LONGS
#define BITS_TO_LONGS(nr)           __KERNEL_DIV_ROUND_UP(nr, BITS_PER_TYPE(long))
#endif
#define DECLARE_BITMAP(name,bits) \
    unsigned long name[BITS_TO_LONGS(bits)]

#define IRQF_TRIGGER_NONE   0x00000000
#define IRQF_TRIGGER_RISING 0x00000001
#define IRQF_TRIGGER_FALLING    0x00000002
#define IRQF_TRIGGER_HIGH   0x00000004
#define IRQF_TRIGGER_LOW    0x00000008
#define IRQF_TRIGGER_MASK   (IRQF_TRIGGER_HIGH | IRQF_TRIGGER_LOW | \
                             IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING)
#define IRQF_TRIGGER_PROBE  0x00000010

#define IRQF_ONESHOT        0x00002000
#define IRQF_SHARED     0x00000080
#define IRQF_PROBE_SHARED   0x00000100
#define __IRQF_TIMER        0x00000200
#define IRQF_PERCPU     0x00000400
#define IRQF_NOBALANCING    0x00000800
#define IRQF_IRQPOLL        0x00001000
#define IRQF_NO_SUSPEND     0x00004000
#define IRQF_FORCE_RESUME   0x00008000
#define IRQF_NO_THREAD      0x00010000
#define IRQF_EARLY_RESUME   0x00020000
#define IRQF_COND_SUSPEND   0x00040000

#define IRQF_TIMER      (__IRQF_TIMER | IRQF_NO_SUSPEND | IRQF_NO_THREAD)

#define IRQ_RETVAL(x)   ((x) ? IRQ_HANDLED : IRQ_NONE)

struct irqaction
{
    hal_irq_handler_t   handler;
    void                *dev_id;
    struct irqaction    *next;
    unsigned long       irq_nums;
};

#define IRQ_NOTCONNECTED    (1U << 31)


int arch_request_irq(int32_t irq, hal_irq_handler_t handler,
                     void *data);
void arch_free_irq(int32_t irq);

/* implemented in armvxxx/ riscv/xx */
void arch_enable_irq(int32_t irq);
void arch_disable_irq(int32_t irq);

void arch_enable_all_irq(void);
void arch_disable_all_irq(void);

unsigned long xport_interrupt_disable(void);
void xport_interrupt_enable(unsigned long flag);
unsigned long arch_irq_is_disable(void);

#endif
