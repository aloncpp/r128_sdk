#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <csr.h>
#include <rv_io.h>

#include "irqflags.h"
#include "irq_internal.h"
#include "excep.h"
#include "irqs.h"

#include <hal_debug.h>

#ifdef CONFIG_COMPONENTS_PM
#include "pm_syscore.h"
#endif

#define NR_CPUS             1

#define C906_PLIC_PHY_ADDR              (0x50000000)
#define C906_PLIC_NR_EXT_IRQS           (NR_IRQS)
#define C906_NR_CPUS                    (NR_CPUS)

//M and S mode context.
#define C906_NR_CONTEXT                 (2)

#define MAX_DEVICES                     1024
#define MAX_CONTEXTS                    15872

/*
 *  Each interrupt source has a priority register associated with it.
 *  We always hardwire it to one in Linux.
 */
#define PRIORITY_BASE                   0
#define PRIORITY_PER_ID                 4

/*
 *  Each hart context has a vector of interrupt enable bits associated with it.
 *  There's one bit for each interrupt source.
 */
#define ENABLE_BASE                     0x2000
#define ENABLE_PER_HART                 0x00

/*
 *  Each hart context has a set of control registers associated with it.  Right
 *  now there's only two: a source priority threshold over which the hart will
 *  take an interrupt, and a register to claim interrupts.
 */
#define CONTEXT_BASE                    0x200000
#define CONTEXT_PER_HART                0x0000
#define CONTEXT_THRESHOLD               0x00
#define CONTEXT_CLAIM                   0x04

static void *c906_plic_regs = NULL;

struct plic_handler
{
    bool   present;
    void  *hart_base;
    void  *enable_base;
};

static inline void plic_toggle(struct plic_handler *handler, int hwirq, int enable);
struct plic_handler c906_plic_handlers[C906_NR_CPUS];

uint32_t plic_irq_rec[(C906_PLIC_NR_EXT_IRQS + 1 + 32) >> 5] = {0};

static inline void plic_irq_toggle(int hwirq, int enable)
{
    int cpu = 0;

    // set priority of interrupt, interrupt 0 is zero.
    writel(enable, c906_plic_regs + PRIORITY_BASE + hwirq * PRIORITY_PER_ID);
    struct plic_handler *handler = &c906_plic_handlers[cpu];

    if (handler->present)
    {
        plic_toggle(handler, hwirq, enable);
    }
}

static void plic_irq_mask(unsigned int irq)
{
    plic_irq_toggle(irq, 0);
    return;
}

static void plic_irq_unmask(unsigned int irq)
{
    plic_irq_toggle(irq, 1);
    return;
}

static void plic_irq_eoi(unsigned int irq)
{
    int cpu = 0;
    struct plic_handler *handler = &c906_plic_handlers[cpu];

    writel(irq, handler->hart_base + CONTEXT_CLAIM);
    return;
}

struct irq_chip plic_chip =
{
    .name           = "SiFive PLIC",
    .irq_mask       = plic_irq_mask,
    .irq_mask_ack   = NULL,
    .irq_unmask     = plic_irq_unmask,
    .irq_eoi        = plic_irq_eoi,
};

int irq_disable(unsigned int irq_no)
{
    plic_irq_toggle(irq_no, 0);

    if (plic_irq_rec[irq_no >> 5] & (1 << (irq_no % 32)))
	    plic_irq_rec[irq_no >> 5] &= ~(1 << (irq_no % 32));
    return 0;
}

int irq_enable(unsigned int irq_no)
{
    plic_irq_toggle(irq_no, 1);

    if (!(plic_irq_rec[irq_no >> 5] & (1 << (irq_no % 32))))
	    plic_irq_rec[irq_no >> 5] |= (1 << (irq_no % 32));
    return 0;
}

void arch_disable_irq(int32_t irq)
{
    irq_disable(irq);
}

void arch_enable_irq(int32_t irq)
{
    irq_enable(irq);
}

void xport_interrupt_enable(unsigned long flags)
{
    return arch_local_irq_restore(flags);
}

unsigned long xport_interrupt_disable(void)
{
    return arch_local_irq_save();
}

void arch_disable_all_irq(void)
{
    arch_local_irq_disable();
}

void arch_enable_all_irq(void)
{
    arch_local_irq_enable();
}

/*
 * Handling an interrupt is a two-step process: first you claim the interrupt
 * by reading the claim register, then you complete the interrupt by writing
 * that source ID back to the same claim register.  This automatically enables
 * and disables the interrupt, so there's nothing else to do.
 */
void plic_handle_irq(irq_regs_t *regs)
{
    int cpu = 0;
    unsigned int irq;

    struct plic_handler *handler = &c906_plic_handlers[cpu];
    void *claim = handler->hart_base + CONTEXT_CLAIM;

    if (c906_plic_regs == NULL || !handler->present)
    {
        printf("plic state not initialized.\n");
        hal_sys_abort();
        return;
    }

    csr_clear(mie, MIE_MEIE);

    while ((irq = readl(claim)))
    {
        // ID0 is diabled permantually from spec.
        if (irq == 0)
        {
            printf("irq no is zero.\n");
        }
        else
        {
            generic_handle_irq(irq);
        }
    }
    csr_set(mie, MIE_MEIE);
    return;
}


static inline void plic_toggle(struct plic_handler *handler, int hwirq, int enable)
{
    uint32_t  *reg = handler->enable_base + (hwirq / 32) * sizeof(uint32_t);
    uint32_t hwirq_mask = 1 << (hwirq % 32);

    if (enable)
    {
        writel(readl(reg) | hwirq_mask, reg);
    }
    else
    {
        writel(readl(reg) & ~hwirq_mask, reg);
    }
}

#ifdef CONFIG_COMPONENTS_PM
#define C906_PLIC_PRIO_BASE	(C906_PLIC_PHY_ADDR + 0x0004)
#define C906_PRIO_REG_NUM	(255)
#define C906_PLIC_CTRL_REG	(C906_PLIC_PHY_ADDR + 0x1ffffc)
/* reg 32 bit */
static uint32_t threshold_bak[C906_NR_CONTEXT];
static uint32_t prio_bak[C906_PRIO_REG_NUM];
static uint32_t ctrl_bak;
static int plic_suspend(void *data, suspend_mode_t mode)
{
	int ret = 0;
	int nr_context;
	int i;
	int cpu = 0;
	unsigned long hwirq;

	for (i = 0; i < C906_PRIO_REG_NUM; i++)
		prio_bak[i] = readl((void *)(intptr_t)(C906_PLIC_PRIO_BASE + 0x4 * i));

	/* save ctrl reg */
	ctrl_bak = readl((void *)(intptr_t)C906_PLIC_CTRL_REG);

	nr_context = C906_NR_CONTEXT;

	/* restore threshold for M/S mode */
	for (i = 0; i < nr_context; i ++) {
		struct plic_handler *handler;

		cpu = 0;

		/* skip contexts other than supervisor external interrupt */
		if (i == 0)
			continue;

		// we always use CPU0 M-mode target register.
		handler = &c906_plic_handlers[cpu];

		/* priority must be > threshold to trigger an interrupt */
		threshold_bak[i] = readl(handler->hart_base + CONTEXT_THRESHOLD);
	}

	return ret;
}

static void plic_resume(void *data, suspend_mode_t mode)
{
	int nr_irqs;
	int nr_context;
	int i;
	int cpu = 0;
	unsigned long hwirq;

	for (i = 0; i < C906_PRIO_REG_NUM; i++)
		writel(prio_bak[i], (void *)(intptr_t)(C906_PLIC_PRIO_BASE + 0x4 * i));

	/* save ctrl reg */
	writel(ctrl_bak, (void *)C906_PLIC_CTRL_REG);

	nr_context = C906_NR_CONTEXT;
	nr_irqs = C906_PLIC_NR_EXT_IRQS;

	/* restore threshold for M/S mode */
	for (i = 0; i < nr_context; i ++) {
		struct plic_handler *handler;

		cpu = 0;

		/* skip contexts other than supervisor external interrupt */
		if (i == 0)
			continue;

		// we always use CPU0 M-mode target register.
		handler = &c906_plic_handlers[cpu];

		/* priority must be > threshold to trigger an interrupt */
		writel(threshold_bak[i], handler->hart_base + CONTEXT_THRESHOLD);
		for (hwirq = 1; hwirq <= nr_irqs; hwirq++) {
			if(plic_irq_rec[hwirq >> 5] & (1 << (hwirq % 32)))
				plic_toggle(handler, hwirq, 1);
		}
	}
}

static struct syscore_ops plic_syscore_ops = {
	.name = "plic_syscore_ops",
	.suspend = plic_suspend,
	.resume = plic_resume,
};
#endif /* CONFIG_COMPONENTS_PM */

void plic_init(void)
{
    int nr_irqs;
    int nr_context;
    int i;
    unsigned long hwirq;
    int cpu = 0;

    if (c906_plic_regs)
    {
        printf("plic already initialized!\n");
        return;
    }

    // 1 CPU, but 2 target
    // cpuid           target
    //   0          CPU0 M-mode
    //   0          CPU0 S-mode
    nr_context = C906_NR_CONTEXT;

    c906_plic_regs = (void *)C906_PLIC_PHY_ADDR;
    if (!c906_plic_regs)
    {
        printf("fatal error, plic is reg space is null.\n");
        return;
    }

    nr_irqs = C906_PLIC_NR_EXT_IRQS;

    for (i = 0; i < nr_context; i ++)
    {
        struct plic_handler *handler;
        uint32_t threshold = 0;

        cpu = 0;

        /* skip contexts other than supervisor external interrupt */
        if (i == 0)
        {
            continue;
        }

        // we always use CPU0 M-mode target register.
        handler = &c906_plic_handlers[cpu];
        if (handler->present)
        {
            threshold  = 0xffffffff;
            goto done;
        }

        handler->present = true;
        handler->hart_base = c906_plic_regs + CONTEXT_BASE + i * CONTEXT_PER_HART;
        handler->enable_base = c906_plic_regs + ENABLE_BASE + i * ENABLE_PER_HART;

done:
        /* priority must be > threshold to trigger an interrupt */
        writel(threshold, handler->hart_base + CONTEXT_THRESHOLD);
        for (hwirq = 1; hwirq <= nr_irqs; hwirq++)
        {
            plic_toggle(handler, hwirq, 0);
        }
    }

#ifdef CONFIG_COMPONENTS_PM
	int ret;
	ret = pm_syscore_register(&plic_syscore_ops);
	if (ret)
		printf("WARNING: plic syscore ops registers failed\n");
#endif

    return;
}
