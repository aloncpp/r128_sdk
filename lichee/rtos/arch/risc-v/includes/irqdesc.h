#ifndef __IRQDESC_H
#define __IRQDESC_H

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "irq_internal.h"

struct irq_desc
{
    //struct irq_data     irq_data;
    struct irqaction    *action;    /* IRQ action list */
};

void handle_level_irq(unsigned int irq, struct irq_desc *desc);

static inline void generic_handle_irq_desc(unsigned int irq, struct irq_desc *desc)
{
    handle_level_irq(irq, desc);
}

int generic_handle_irq(unsigned int irq);

/* Test to see if a driver has successfully requested an irq */
static inline int irq_desc_has_action(struct irq_desc *desc)
{
    return desc->action != NULL;
}

#define for_each_action_of_desc(desc, act)                      \
    for (act = desc->action; act; act = act->next)

#endif
