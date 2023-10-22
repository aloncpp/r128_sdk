#ifndef _ASM_RISCV_IRQFLAGS_H
#define _ASM_RISCV_IRQFLAGS_H

#include "csr.h"

/* read interrupt enabled status */
static inline unsigned long arch_local_save_flags(void)
{
    return csr_read(CSR_MSTATUS);
}

/* test flags */
static inline int arch_irqs_disabled_flags(unsigned long flags)
{
    return !(flags & SR_MIE);
}

/* test hardware interrupt enable bit */
static inline int arch_irqs_disabled(void)
{
    return arch_irqs_disabled_flags(arch_local_save_flags());
}

/* unconditionally enable interrupts */
static inline void arch_local_irq_enable(void)
{
    csr_set(CSR_MSTATUS, SR_MIE);
}

/* unconditionally disable interrupts */
static inline void arch_local_irq_disable(void)
{
    csr_clear(CSR_MSTATUS, SR_MIE);
}

/* get status and disable interrupts */
static inline unsigned long arch_local_irq_save(void)
{
    return csr_read_clear(CSR_MSTATUS, SR_MIE);
}

/* set interrupt enabled status */
static inline void arch_local_irq_restore(unsigned long flags)
{
    csr_set(CSR_MSTATUS, flags & SR_MIE);
}

static inline void wait_for_interrupt(void)
{
    __asm__ __volatile__("wfi \n:::memory");
}

#endif /* _ASM_RISCV_IRQFLAGS_H */
