#include <stdlib.h>
#include "rv_interrupt.h"
#include "irqdesc.h"
#include "irq_internal.h"
#include "irqflags.h"
#include "excep.h"
#include "irqs.h"
#include <csr.h>

#include <hal_debug.h>

void plic_handle_irq(irq_regs_t *);

/* test hardware interrupt enable bit */
unsigned long arch_irq_is_disable(void)
{
    return arch_irqs_disabled_flags(arch_local_save_flags());
}
/*
 * Generic no controller implementation
 */
extern struct irq_chip plic_chip;

struct irq_desc irq_desc[NR_IRQS]  =
{
    [0 ... NR_IRQS - 1] = {
        .action         = NULL,
    }
};

void handle_level_irq(unsigned int irq, struct irq_desc *desc)
{
    struct irq_chip *chip = &plic_chip;
    struct irqaction *action;

    for_each_action_of_desc(desc, action)
    {
        hal_irqreturn_t res;
        res = action->handler(action->dev_id);
        action->irq_nums ++;

        switch (res)
        {
            case HAL_IRQ_OK:
            /* Fall through - to add to randomness */
            case HAL_IRQ_ERR:
                break;
            default:
                break;
        }
    }

    chip->irq_eoi(irq);
}

void irq_init(void)
{
}

struct irq_desc *irq_to_desc(unsigned int irq)
{
    return (irq < NR_IRQS) ? irq_desc + irq : NULL;
}

static int __setup_irq(unsigned int irq, struct irq_desc *desc, struct irqaction *new)
{
    struct irqaction *old, **old_ptr;
    unsigned long flags;
    int ret, nested;

    if (!desc)
    {
        printf("no desc for this irq %d.", irq);
        return -1;
    }

    flags = arch_local_irq_save();
    old_ptr = &desc->action;
    old = *old_ptr;
    if (old)
    {
        /* add new interrupt at end of irq queue */
        do
        {
            old_ptr = &old->next;
            old = *old_ptr;
        } while (old);
    }

    *old_ptr = new;

    arch_local_irq_restore(flags);

    return 0;
}

int irq_request(uint32_t irq, hal_irq_handler_t handler,
                void *data)
{
    struct irqaction *action;
    struct irq_desc *desc;
    int retval;

    if (irq == IRQ_NOTCONNECTED)
    {
        printf("irq invalided!");
        return -1;
    }

    desc = irq_to_desc(irq);
    if (!desc)
    {
        printf("no desc for this irq %d.!", irq);
        return -1;
    }

    if (!handler)
    {
        printf("handler parameter is null");
        return -1;
    }

    action = calloc(sizeof(struct irqaction), 1);
    if (!action)
    {
        printf("no free buffer.");
        return -1;
    }

    action->handler = handler;
    action->dev_id = data;
    action->irq_nums = 0;

    retval = __setup_irq(irq, desc, action);
    if (retval)
    {
        free(action);
        action = NULL;
    }
    else
    {
        retval = irq;
    }

    return retval;
}

int generic_handle_irq(unsigned int irq)
{
    struct irq_desc *desc = irq_to_desc(irq);
    if (!desc)
    {
        printf("fatal error, desc is null for irq %d.", irq);
        hal_sys_abort();
        return -1;
    }
    generic_handle_irq_desc(irq, desc);
    return 0;
}

static struct irqaction *__free_irq(unsigned int irq, struct irq_desc *desc, void *dev_id)
{
    struct irqaction *action, **action_ptr;
    unsigned long flags;

    flags = arch_local_irq_save();

    action_ptr = &desc->action;
    for (; ;)
    {
        action = *action_ptr;

        if (!action)
        {
            printf("Trying to free already-free IRQ %d", irq);
            arch_local_irq_restore(flags);
            return NULL;
        }
        break;
    }

    /* Found it - now remove it from the list of entries: */
    *action_ptr = action->next;

    /* If this was the last handler, shut down the IRQ line: */
    if (!desc->action)
    {
        /* Only shutdown. Deactivate after synchronize_hardirq() */
        void hal_disable_irq(int32_t irq);
        hal_disable_irq(irq);
    }

    arch_local_irq_restore(flags);

    return action;
}

int irq_free(unsigned int irq, void *dev_id)
{
    struct irq_desc *desc = irq_to_desc(irq);
    struct irqaction *action;

    if (!desc)
    {
        printf("no desc for irq %d.", irq);
        return -1;
    }

    action = __free_irq(irq, desc, dev_id);

    if (!action)
    {
        printf("find no desc for irq %d.", irq);
        return -1;
    }

    free(action);
    return 0;
}

void remove_irq(unsigned int irq, struct irqaction *act)
{
    struct irq_desc *desc = irq_to_desc(irq);

    if (desc)
    {
        __free_irq(irq, desc, act->dev_id);
    }
}

int setup_irq(unsigned int irq, struct irqaction *act)
{
    int retval;
    struct irq_desc *desc = irq_to_desc(irq);

    if (!desc || !act)
    {
        printf("cant find irq descriptor.");
        return -1;
    }

    retval = __setup_irq(irq, desc, act);

    return retval;
}

void show_irqs(void)
{
    int i;
    struct irq_desc *desc = NULL;
    struct irqaction *action = NULL;;

    printf(" irqno    handler        active.\n");
    printf(" -------------------------------\n");
    for (i = 0; i < NR_IRQS; i ++)
    {
        int line = 0;
        desc = irq_to_desc(i);
        if (!desc || desc->action == NULL)
        {
            continue;
        }

        printf("%6d", i);
        for_each_action_of_desc(desc, action)
        {
            printf("      0x%lx     0x%08lx\n", (unsigned long)action->handler, action->irq_nums);
        }
    }
    printf(" -------------------------------\n");
}

void handle_arch_irq(irq_regs_t *regs)
{
    unsigned long mip = csr_read(CSR_MIP);

    if (!(mip & (MIE_MTIE | MIE_MEIE)))
    {
        printf("sip status error.");
        hal_sys_abort();
    }

    plic_handle_irq(regs);

    return;
}

int arch_request_irq(int32_t irq, hal_irq_handler_t handler,
                     void *data)
{
    return irq_request(irq, handler, data);
}

void arch_free_irq(int32_t irq)
{
    irq_free(irq, NULL);
}
