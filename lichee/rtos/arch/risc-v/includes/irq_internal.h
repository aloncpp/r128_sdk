#ifndef _RTOS_IRQ_H
#define _RTOS_IRQ_H

#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

struct irq_desc;
typedef void (*irq_flow_handler_t)(struct irq_desc *desc);

struct irq_data
{
    struct irq_chip     *chip;
};

struct irq_chip
{
    const char  *name;
    void (*irq_mask)(unsigned int irq);
    void (*irq_mask_ack)(unsigned int irq);
    void (*irq_unmask)(unsigned int irq);
    void (*irq_eoi)(unsigned int irq);

    unsigned long   flags;
};

enum
{
    IRQCHIP_SET_TYPE_MASKED     = (1 <<  0),
    IRQCHIP_EOI_IF_HANDLED      = (1 <<  1),
    IRQCHIP_MASK_ON_SUSPEND     = (1 <<  2),
    IRQCHIP_ONOFFLINE_ENABLED   = (1 <<  3),
    IRQCHIP_SKIP_SET_WAKE       = (1 <<  4),
    IRQCHIP_ONESHOT_SAFE        = (1 <<  5),
    IRQCHIP_EOI_THREADED        = (1 <<  6),
    IRQCHIP_SUPPORTS_LEVEL_MSI  = (1 <<  7),
    IRQCHIP_SUPPORTS_NMI        = (1 <<  8),
};

#include "irqdesc.h"

#endif /* _RTOS_IRQ_H */
