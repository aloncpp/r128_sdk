/*
 * Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of XRADIO TECHNOLOGY CO., LTD. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _XR_SYS_INTERRUPT_H_
#define _XR_SYS_INTERRUPT_H_

#include "compiler.h"

#if defined(__CC_ARM)
/* ARM Compiler */

/*
 * CPU interrupt mask handling.
 */
#define IRQMASK_REG_NAME_R primask
#define IRQMASK_REG_NAME_W primask

/*
 * Save the current interrupt enable state & disable IRQs
 */
static __always_inline unsigned long arch_irq_save(void)
{
	unsigned long flags;

	__asm {
		mrs	flags, IRQMASK_REG_NAME_R
		cpsid	i
	}
	return flags;
}

/*
 * restore saved IRQ state
 */
static __always_inline void arch_irq_restore(unsigned long flags)
{
	__asm { msr	IRQMASK_REG_NAME_W, flags }
}

/*
 * Enable IRQs
 */
#define arch_irq_enable()	__enable_irq()

/*
 * Disable IRQs
 */
#define arch_irq_disable()	__disable_irq()

/*
 * Enable FIQs
 */
#define arch_fiq_enable()	__enable_fiq()

/*
 * Disable FIQs
 */
#define arch_fiq_disable()	__disable_fiq()

#elif defined(__GNUC__)
/* GNU Compiler */

#ifdef CONFIG_OS_RTTHREAD

/*
 * Save the current interrupt enable state & disable IRQs
 */
#define arch_irq_save(void)	rt_hw_interrupt_disable()

/*
 * restore saved IRQ state
 */
#define arch_irq_restore(flags)	rt_hw_interrupt_enable(flags)

/*
 * Enable IRQs
 */
#define arch_irq_enable()	rt_hw_interrupt_enable(__irq_level)

/*
 * Disable IRQs
 */
#define arch_irq_disable()	rt_base_t __irq_level = rt_hw_interrupt_disable()

#elif defined(CONFIG_OS_YUNOS) /* CONFIG_OS_YUNOS */

void save_and_cli(void);
void restore_flags(void);
/*
 * Save the current interrupt enable state & disable IRQs
 */
#define arch_irq_save(void)	\
({	\
	save_and_cli();	\
	0;	\
})

/*
 * restore saved IRQ state
 */
#define arch_irq_restore(flags)	\
({				\
	restore_flags();	\
	flags;	\
})



/*
 * Enable IRQs
 */
#define arch_irq_enable()	restore_flags()

/*
 * Disable IRQs
 */
#define arch_irq_disable()	save_and_cli()

#elif defined(CONFIG_OS_NUTTX)

extern void irqsave_and_cli(void);
extern void irqrsto_and_ena(void);
#define arch_irq_save(void)    \
({\
	irqsave_and_cli(); \
    0;\
})

#define arch_irq_restore(flags)	\
({				\
	irqrsto_and_ena();	\
	flags;	\
})

#define arch_irq_enable()   do { asm volatile ("cpsie i"); } while (0)
#define arch_irq_disable()  do { asm volatile ("cpsid i"); } while (0)

#elif defined(CONFIG_OS_TINA)

#if (defined(CONFIG_ARCH_ARM_CORTEX_M33) || defined(CONFIG_ARCH_RISCV_RV64))
#include "hal_interrupt.h"

#define arch_irq_save       hal_interrupt_disable_irqsave
#define arch_irq_restore    hal_interrupt_enable_irqrestore
#define arch_irq_enable     hal_interrupt_enable
#define arch_irq_disable    hal_interrupt_disable
#endif

#elif defined(CONFIG_OS_XRADIO)
/*
 * CPU interrupt mask handling.
 */
#define IRQMASK_REG_NAME_R "primask"
#define IRQMASK_REG_NAME_W "primask"

/*
 * Save the current interrupt enable state & disable IRQs
 */
static __always_inline unsigned long arch_irq_save(void)
{
	unsigned long flags;

	__asm volatile(
		"mrs	%0, " IRQMASK_REG_NAME_R "\n"
		"cpsid	i"
		: "=r" (flags) : : "memory", "cc");
	return flags;
}

/*
 * restore saved IRQ state
 */
static __always_inline void arch_irq_restore(unsigned long flags)
{
	__asm volatile(
		"msr	" IRQMASK_REG_NAME_W ", %0"
		:
		: "r" (flags)
		: "memory", "cc");
}

/*
 * Save the current interrupt enable state.
 */
static __always_inline unsigned long arch_irq_get_flags(void)
{
	unsigned long flags;

	__asm volatile(
		"mrs	%0, " IRQMASK_REG_NAME_R "\n"
		: "=r" (flags) : : "memory", "cc");
	return flags;

}

/*
 * Enable IRQs
 */
#define arch_irq_enable()	__asm volatile("cpsie i" : : : "memory", "cc")

/*
 * Disable IRQs
 */
#define arch_irq_disable()	__asm volatile("cpsid i" : : : "memory", "cc")

/*
 * Enable FIQs
 */
#define arch_fiq_enable()	__asm volatile("cpsie f" : : : "memory", "cc")

/*
 * Disable FIQs
 */
#define arch_fiq_disable()	__asm volatile("cpsid f" : : : "memory", "cc")

#endif /* CONFIG_OS_RTTHREAD */

#else
#error "Compiler not supported."
#endif

#define xr_irq_save arch_irq_save
#define xr_irq_restore arch_irq_restore
#define arch_local_save_flags arch_irq_get_flags
#define xr_irq_enable arch_irq_enable
#define xr_irq_disable arch_irq_disable

#endif /* _SYS_INTERRUPT_H_ */
