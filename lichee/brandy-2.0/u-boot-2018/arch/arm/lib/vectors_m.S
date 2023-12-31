/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2015
 * Kamil Lulko, <kamil.lulko@gmail.com>
 */

#include <config.h>
#ifdef CONFIG_CPU_V7M
#include <asm/armv7m.h>
#endif
#ifdef CONFIG_CPU_V8M
#include <asm/armv8m.h>
#endif
#include <linux/linkage.h>

.type __hard_fault_entry, %function
__hard_fault_entry:
	mov	r0, sp	@ pass auto-saved registers as argument
	b	do_hard_fault

.type __mm_fault_entry, %function
__mm_fault_entry:
	mov	r0, sp	@ pass auto-saved registers as argument
	b	do_mm_fault

.type __bus_fault_entry, %function
__bus_fault_entry:
	mov	r0, sp	@ pass auto-saved registers as argument
	b	do_bus_fault

.type __usage_fault_entry, %function
__usage_fault_entry:
	mov	r0, sp	@ pass auto-saved registers as argument
	b	do_usage_fault

.type __invalid_entry, %function
__invalid_entry:
	mov	r0, sp	@ pass auto-saved registers as argument
	b	do_invalid_entry

.global irq_vector
   .section  .vectors
   .type irq_vector, %object
   .size irq_vector, .-irq_vector
ENTRY(_start)
irq_vector:
	.long	CONFIG_SYS_INIT_SP_ADDR		@ 0 - Reset stack pointer
	.long	reset				@ 1 - Reset
	.long	__invalid_entry			@ 2 - NMI
	.long	__hard_fault_entry		@ 3 - HardFault
	.long	__mm_fault_entry		@ 4 - MemManage
	.long	__bus_fault_entry		@ 5 - BusFault
	.long	__usage_fault_entry		@ 6 - UsageFault
	.long	__invalid_entry			@ 7 - Reserved
	.long	__invalid_entry			@ 8 - Reserved
	.long	__invalid_entry			@ 9 - Reserved
	.long	__invalid_entry			@ 10 - Reserved
	.long	__invalid_entry			@ 11 - SVCall
	.long	__invalid_entry			@ 12 - Debug Monitor
	.long	__invalid_entry			@ 13 - Reserved
	.long	__invalid_entry			@ 14 - PendSV
	.long	__invalid_entry			@ 15 - SysTick
	.rept	255 - 16
	.long	__invalid_entry			@ 16..255 - External Interrupts
	.endr
