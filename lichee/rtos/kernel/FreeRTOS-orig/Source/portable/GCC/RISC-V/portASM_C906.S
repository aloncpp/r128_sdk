/*
 * FreeRTOS Kernel V10.4.3
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
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
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 * 1 tab == 4 spaces!
 */

/*
 * The FreeRTOS kernel's RISC-V port is split between the the code that is
 * common across all currently supported RISC-V chips (implementations of the
 * RISC-V ISA), and code which tailors the port to a specific RISC-V chip:
 *
 * + The code that is common to all RISC-V chips is implemented in
 *   FreeRTOS\Source\portable\GCC\RISC-V-RV32\portASM.S.  There is only one
 *   portASM.S file because the same file is used no matter which RISC-V chip is
 *   in use.
 *
 * + The code that tailors the kernel's RISC-V port to a specific RISC-V
 *   chip is implemented in freertos_risc_v_chip_specific_extensions.h.  There
 *   is one freertos_risc_v_chip_specific_extensions.h that can be used with any
 *   RISC-V chip that both includes a standard CLINT and does not add to the
 *   base set of RISC-V registers.  There are additional
 *   freertos_risc_v_chip_specific_extensions.h files for RISC-V implementations
 *   that do not include a standard CLINT or do add to the base set of RISC-V
 *   registers.
 *
 * CARE MUST BE TAKEN TO INCLDUE THE CORRECT
 * freertos_risc_v_chip_specific_extensions.h HEADER FILE FOR THE CHIP
 * IN USE.  To include the correct freertos_risc_v_chip_specific_extensions.h
 * header file ensure the path to the correct header file is in the assembler's
 * include path.
 *
 * This freertos_risc_v_chip_specific_extensions.h is for use on RISC-V chips
 * that include a standard CLINT and do not add to the base set of RISC-V
 * registers.
 *
 */
#include <csr.h>
#include <excep.h>

#define portasmHAS_SIFIVE_CLINT 1
#define portasmHAS_MTIME 1
#define portasmADDITIONAL_CONTEXT_SIZE 0 /* Must be even number on 32-bit cores. */
#define portasmHANDLE_INTERRUPT C906_Default_IRQHandler

#ifdef configTASK_RETURN_ADDRESS
	#define portTASK_RETURN_ADDRESS	configTASK_RETURN_ADDRESS
#else
	#define portTASK_RETURN_ADDRESS	prvTaskExitError
#endif

.macro portasmSAVE_ADDITIONAL_REGISTERS
	/* No additional registers to save, so this macro does nothing. */
	.endm

.macro portasmRESTORE_ADDITIONAL_REGISTERS
	/* No additional registers to restore, so this macro does nothing. */
	.endm


#if __riscv_xlen == 64
	#define portWORD_SIZE 8
	#define store_x sd
	#define load_x ld
#elif __riscv_xlen == 32
	#define store_x sw
	#define load_x lw
	#define portWORD_SIZE 4
#else
	#error Assembler did not define __riscv_xlen
#endif

/* Check the freertos_risc_v_chip_specific_extensions.h and/or command line
definitions. */
#if defined( portasmHAS_CLINT ) && defined( portasmHAS_MTIME )
	#error The portasmHAS_CLINT constant has been deprecated.  Please replace it with portasmHAS_MTIME.  portasmHAS_CLINT and portasmHAS_MTIME cannot both be defined at once.  See https://www.FreeRTOS.org/Using-FreeRTOS-on-RISC-V.html
#endif

#ifdef portasmHAS_CLINT
	#warning The portasmHAS_CLINT constant has been deprecated.  Please replace it with portasmHAS_MTIME and portasmHAS_SIFIVE_CLINT.  For now portasmHAS_MTIME and portasmHAS_SIFIVE_CLINT are derived from portasmHAS_CLINT.  See https://www.FreeRTOS.org/Using-FreeRTOS-on-RISC-V.html
	#define portasmHAS_MTIME portasmHAS_CLINT
	#define portasmHAS_SIFIVE_CLINT portasmHAS_CLINT
#endif

#ifndef portasmHAS_MTIME
	#error freertos_risc_v_chip_specific_extensions.h must define portasmHAS_MTIME to either 1 (MTIME clock present) or 0 (MTIME clock not present).  See https://www.FreeRTOS.org/Using-FreeRTOS-on-RISC-V.html
#endif

#ifndef portasmHANDLE_INTERRUPT
	#error portasmHANDLE_INTERRUPT must be defined to the function to be called to handle external/peripheral interrupts.  portasmHANDLE_INTERRUPT can be defined on the assembler command line or in the appropriate freertos_risc_v_chip_specific_extensions.h header file.  https://www.FreeRTOS.org/Using-FreeRTOS-on-RISC-V.html
#endif

#ifndef portasmHAS_SIFIVE_CLINT
	#define portasmHAS_SIFIVE_CLINT 0
#endif

/* Only the standard core registers are stored by default.  Any additional
registers must be saved by the portasmSAVE_ADDITIONAL_REGISTERS and
portasmRESTORE_ADDITIONAL_REGISTERS macros - which can be defined in a chip
specific version of freertos_risc_v_chip_specific_extensions.h.  See the notes
at the top of this file. */

#if (defined CONFIG_ARCH_RISCV_FPU && defined CONFIG_SAVE_C906_FPU_CONTEXT_IN_TCB) || defined CONFIG_ARCH_RISCV_VECTOR

.pushsection ".bss"
.balign 8
s_rtos_tcb_when_trap:
.space __riscv_xlen / 8
.popsection

#endif

#ifdef CONFIG_ARCH_RISCV_FPU

#ifndef CONFIG_SAVE_C906_FPU_CONTEXT_IN_TCB

#define portCONTEXT_SIZE ( 68 * portWORD_SIZE )

#else

#define portCONTEXT_SIZE ( 34 * portWORD_SIZE )

.global g_fpu_ctx_member_offset

.pushsection ".bss"

#ifdef CONFIG_RECORD_C906_FPU_CTX_STATISTICS
.balign 4
.global g_skip_fpu_ctx_save_cnt
g_skip_fpu_ctx_save_cnt:
.space 4

.global g_skip_fpu_off_ctx_restore_cnt
g_skip_fpu_off_ctx_restore_cnt:
.space 4

.global g_skip_fpu_init_ctx_restore_cnt
g_skip_fpu_init_ctx_restore_cnt:
.space 4

.global g_skip_fpu_clean_ctx_restore_cnt
g_skip_fpu_clean_ctx_restore_cnt:
.space 4
#endif

.popsection

#endif

#else
#define portCONTEXT_SIZE ( 34 * portWORD_SIZE )
#endif

#ifdef CONFIG_ARCH_RISCV_VECTOR

.global g_vector_ctx_member_offset

#endif

.global xPortStartFirstTask
.global freertos_risc_v_trap_handler
.global pxPortInitialiseStack
.extern pxCurrentTCB
.extern ulPortTrapHandler
.extern vTaskSwitchContext
.extern xTaskIncrementTick
.extern Timer_IRQHandler
.extern pullMachineTimerCompareRegister
.extern pullNextTime
.extern uxTimerIncrementsForOneTick /* size_t type so 32-bit on 32-bit core and 64-bits on 64-bit core. */
.extern xISRStackTop
.extern portasmHANDLE_INTERRUPT
.extern enter_interrupt_handler
.extern exit_interrupt_handler
.extern prvTaskExitError

/*-----------------------------------------------------------*/
.align 8
.func
freertos_risc_v_trap_handler:
	addi sp, sp, -portCONTEXT_SIZE
	store_x x1, 1 * portWORD_SIZE( sp )
	store_x x5, 2 * portWORD_SIZE( sp )
	store_x x6, 3 * portWORD_SIZE( sp )
	store_x x7, 4 * portWORD_SIZE( sp )
	store_x x8, 5 * portWORD_SIZE( sp )
	store_x x9, 6 * portWORD_SIZE( sp )
	store_x x10, 7 * portWORD_SIZE( sp )
	store_x x11, 8 * portWORD_SIZE( sp )
	store_x x12, 9 * portWORD_SIZE( sp )
	store_x x13, 10 * portWORD_SIZE( sp )
	store_x x14, 11 * portWORD_SIZE( sp )
	store_x x15, 12 * portWORD_SIZE( sp )
	store_x x16, 13 * portWORD_SIZE( sp )
	store_x x17, 14 * portWORD_SIZE( sp )
	store_x x18, 15 * portWORD_SIZE( sp )
	store_x x19, 16 * portWORD_SIZE( sp )
	store_x x20, 17 * portWORD_SIZE( sp )
	store_x x21, 18 * portWORD_SIZE( sp )
	store_x x22, 19 * portWORD_SIZE( sp )
	store_x x23, 20 * portWORD_SIZE( sp )
	store_x x24, 21 * portWORD_SIZE( sp )
	store_x x25, 22 * portWORD_SIZE( sp )
	store_x x26, 23 * portWORD_SIZE( sp )
	store_x x27, 24 * portWORD_SIZE( sp )
	store_x x28, 25 * portWORD_SIZE( sp )
	store_x x29, 26 * portWORD_SIZE( sp )
	store_x x30, 27 * portWORD_SIZE( sp )
	store_x x31, 28 * portWORD_SIZE( sp )
	store_x x3,  31 * portWORD_SIZE( sp )
	store_x x4,  32 * portWORD_SIZE( sp )

	mv a0, sp
	addi a0, a0, portCONTEXT_SIZE
	store_x a0,  30 * portWORD_SIZE( sp )

	csrr t0, mscratch
	store_x t0, 33 * portWORD_SIZE( sp )

	csrr t0, mstatus					/* Required for MPIE bit. */
	store_x t0, 29 * portWORD_SIZE( sp )

	portasmSAVE_ADDITIONAL_REGISTERS	/* Defined in freertos_risc_v_chip_specific_extensions.h to save any registers unique to the RISC-V implementation. */

#ifdef CONFIG_ARCH_RISCV_FPU

#ifndef CONFIG_SAVE_C906_FPU_CONTEXT_IN_TCB

	addi a0, sp, 34 * portWORD_SIZE
	fsd f0,  FPU_CTX_F0_F0(a0)
	fsd f1,  FPU_CTX_F1_F0(a0)
	fsd f2,  FPU_CTX_F2_F0(a0)
	fsd f3,  FPU_CTX_F3_F0(a0)
	fsd f4,  FPU_CTX_F4_F0(a0)
	fsd f5,  FPU_CTX_F5_F0(a0)
	fsd f6,  FPU_CTX_F6_F0(a0)
	fsd f7,  FPU_CTX_F7_F0(a0)
	fsd f8,  FPU_CTX_F8_F0(a0)
	fsd f9,  FPU_CTX_F9_F0(a0)
	fsd f10, FPU_CTX_F10_F0(a0)
	fsd f11, FPU_CTX_F11_F0(a0)
	fsd f12, FPU_CTX_F12_F0(a0)
	fsd f13, FPU_CTX_F13_F0(a0)
	fsd f14, FPU_CTX_F14_F0(a0)
	fsd f15, FPU_CTX_F15_F0(a0)
	fsd f16, FPU_CTX_F16_F0(a0)
	fsd f17, FPU_CTX_F17_F0(a0)
	fsd f18, FPU_CTX_F18_F0(a0)
	fsd f19, FPU_CTX_F19_F0(a0)
	fsd f20, FPU_CTX_F20_F0(a0)
	fsd f21, FPU_CTX_F21_F0(a0)
	fsd f22, FPU_CTX_F22_F0(a0)
	fsd f23, FPU_CTX_F23_F0(a0)
	fsd f24, FPU_CTX_F24_F0(a0)
	fsd f25, FPU_CTX_F25_F0(a0)
	fsd f26, FPU_CTX_F26_F0(a0)
	fsd f27, FPU_CTX_F27_F0(a0)
	fsd f28, FPU_CTX_F28_F0(a0)
	fsd f29, FPU_CTX_F29_F0(a0)
	fsd f30, FPU_CTX_F30_F0(a0)
	fsd f31, FPU_CTX_F31_F0(a0)
	frcsr t0
	sw t0, FPU_CTX_FCSR_F0(a0)

#else

	/* save the rtos TCB when enter trap handler */
	load_x s0, pxCurrentTCB
	la t0, s_rtos_tcb_when_trap
	store_x s0, 0(t0)

	csrr t0, mstatus
	li s1, SR_FS
	and t0, t0, s1

#ifdef CONFIG_RECORD_C906_FPU_CTX_STATISTICS
	beq t0, s1, fpu_context_save

	lw t0, g_skip_fpu_ctx_save_cnt
	add t0, t0, 1
	sw t0, g_skip_fpu_ctx_save_cnt, t1
	j skip_fpu_context_save

fpu_context_save:
#else
	bne t0, s1, skip_fpu_context_save /* FPU is not dirty state, don't need to save FPU context */
#endif

	/* get the offset of arch_fpu_context_t member */
	lw t0, g_fpu_ctx_member_offset
	add a0, s0, t0

	frcsr t0
	fsd f0,  FPU_CTX_F0_F0(a0)
	fsd f1,  FPU_CTX_F1_F0(a0)
	fsd f2,  FPU_CTX_F2_F0(a0)
	fsd f3,  FPU_CTX_F3_F0(a0)
	fsd f4,  FPU_CTX_F4_F0(a0)
	fsd f5,  FPU_CTX_F5_F0(a0)
	fsd f6,  FPU_CTX_F6_F0(a0)
	fsd f7,  FPU_CTX_F7_F0(a0)
	fsd f8,  FPU_CTX_F8_F0(a0)
	fsd f9,  FPU_CTX_F9_F0(a0)
	fsd f10, FPU_CTX_F10_F0(a0)
	fsd f11, FPU_CTX_F11_F0(a0)
	fsd f12, FPU_CTX_F12_F0(a0)
	fsd f13, FPU_CTX_F13_F0(a0)
	fsd f14, FPU_CTX_F14_F0(a0)
	fsd f15, FPU_CTX_F15_F0(a0)
	fsd f16, FPU_CTX_F16_F0(a0)
	fsd f17, FPU_CTX_F17_F0(a0)
	fsd f18, FPU_CTX_F18_F0(a0)
	fsd f19, FPU_CTX_F19_F0(a0)
	fsd f20, FPU_CTX_F20_F0(a0)
	fsd f21, FPU_CTX_F21_F0(a0)
	fsd f22, FPU_CTX_F22_F0(a0)
	fsd f23, FPU_CTX_F23_F0(a0)
	fsd f24, FPU_CTX_F24_F0(a0)
	fsd f25, FPU_CTX_F25_F0(a0)
	fsd f26, FPU_CTX_F26_F0(a0)
	fsd f27, FPU_CTX_F27_F0(a0)
	fsd f28, FPU_CTX_F28_F0(a0)
	fsd f29, FPU_CTX_F29_F0(a0)
	fsd f30, FPU_CTX_F30_F0(a0)
	fsd f31, FPU_CTX_F31_F0(a0)
	sw t0,  FPU_CTX_FCSR_F0(a0)

	/* set FS filed(in mstatus reg) to clean state */
	csrc mstatus, s1
	li s2, SR_FS_CLEAN
	csrs mstatus, t0

	/* set FS filed(in thread stack) to clean state */
	load_x  a0, 29 * portWORD_SIZE(sp)
	not t1, s1
	and a0, a0, t1
	or a0, a0, s2
	store_x a0, 29 * portWORD_SIZE(sp)

skip_fpu_context_save:
#endif

#endif

#ifdef CONFIG_ARCH_RISCV_VECTOR
	/* Save TCB when enter trap handler */
	load_x s0, pxCurrentTCB
	la t0, s_rtos_tcb_when_trap
	store_x s0, 0(t0)

	csrr t0, mstatus
	li s1, SR_VS
	and t0, t0, s1
	/* If mstatus.vs = clean or init, skip save VECTOR context */
	bne t0, s1, skip_vector_context_save

	/* Get the offset of arch_vector_context_t member */
	lw t0, g_vector_ctx_member_offset
	add a0, s0, t0

vstate_csr_save:
	csrr   t0, vstart
	sd t0,  VECTOR_CTX_VSTART_V0(a0)
	csrr   t0,  vl
	sd t0,  VECTOR_CTX_Vl_V0(a0)
	csrr   t0,  vtype
	sd t0,  VECTOR_CTX_VTYPE_V0(a0)

vstate_vx_save:
#	vsetvli        t4, x0, e8, m8, ta, ma
	vsetvli        t4, x0, e8, m8
	vse.v          v0, (a0)
	add            a0, a0, t4
	vse.v          v8, (a0)
	/* Force only use v0 - v15 */
#	add            a0, a0, t4
#	vse.v          v16, (a0)
#	add            a0, a0, t4
#	vse.v          v24, (a0)

	/* set mstatus.VS filed clean state */
	csrc mstatus, s1
	li t0, SR_VS_CLEAN
	csrs mstatus, t0

skip_vector_context_save:
#endif /* CONFIG_ARCH_RISCV_VECTOR */


	load_x  t0, pxCurrentTCB			/* Load pxCurrentTCB. */
	store_x  sp, 0( t0 )				/* Write sp to first TCB member. */

	csrr a0, mcause
	csrr a1, mepc

test_if_asynchronous:
	srli a2, a0, __riscv_xlen - 1		/* MSB of mcause is 1 if handing an asynchronous interrupt - shift to LSB to clear other bits. */
	beq a2, x0, handle_synchronous		/* Branch past interrupt handing if not asynchronous. */
	store_x a1, 0( sp )					/* Asynch so save unmodified exception return address. */

handle_asynchronous:

#if( portasmHAS_MTIME != 0 )

	test_if_mtimer:						/* If there is a CLINT then the mtimer is used to generate the tick interrupt. */

		addi t0, x0, 1

		slli t0, t0, __riscv_xlen - 1   /* LSB is already set, shift into MSB.  Shift 31 on 32-bit or 63 on 64-bit cores. */
		addi t1, t0, 7					/* 0x8000[]0007 == machine timer interrupt. */
		bne a0, t1, test_if_external_interrupt

		load_x t0, pullMachineTimerCompareRegister  /* Load address of compare register into t0. */
		load_x t1, pullNextTime  		/* Load the address of ullNextTime into t1. */

		#if( __riscv_xlen == 32 )

			/* Update the 64-bit mtimer compare match value in two 32-bit writes. */
			li t4, -1
			lw t2, 0(t1)				/* Load the low word of ullNextTime into t2. */
			lw t3, 4(t1)				/* Load the high word of ullNextTime into t3. */
			sw t4, 0(t0)				/* Low word no smaller than old value to start with - will be overwritten below. */
			sw t3, 4(t0)				/* Store high word of ullNextTime into compare register.  No smaller than new value. */
			sw t2, 0(t0)				/* Store low word of ullNextTime into compare register. */
			lw t0, uxTimerIncrementsForOneTick	/* Load the value of ullTimerIncrementForOneTick into t0 (could this be optimized by storing in an array next to pullNextTime?). */
			add t4, t0, t2				/* Add the low word of ullNextTime to the timer increments for one tick (assumes timer increment for one tick fits in 32-bits). */
			sltu t5, t4, t2				/* See if the sum of low words overflowed (what about the zero case?). */
			add t6, t3, t5				/* Add overflow to high word of ullNextTime. */
			sw t4, 0(t1)				/* Store new low word of ullNextTime. */
			sw t6, 4(t1)				/* Store new high word of ullNextTime. */

		#endif /* __riscv_xlen == 32 */

		#if( __riscv_xlen == 64 )

			/* Update the 64-bit mtimer compare match value. */
			ld t2, 0(t1)			 	/* Load ullNextTime into t2. */
			sd t2, 0(t0)				/* Store ullNextTime into compare register. */
			ld t0, uxTimerIncrementsForOneTick  /* Load the value of ullTimerIncrementForOneTick into t0 (could this be optimized by storing in an array next to pullNextTime?). */
			add t4, t0, t2				/* Add ullNextTime to the timer increments for one tick. */
			sd t4, 0(t1)				/* Store ullNextTime. */

		#endif /* __riscv_xlen == 64 */

		load_x sp, xISRStackTop			/* Switch to ISR stack before function call. */
		call enter_interrupt_handler
		call riscv_timer_interrupt
		call exit_interrupt_handler
		call xTaskIncrementTick
		beqz a0, processed_source		/* Don't switch context if incrementing tick didn't unblock a task. */
		call vTaskSwitchContext
		j processed_source

	test_if_external_interrupt:			/* If there is a CLINT and the mtimer interrupt is not pending then check to see if an external interrupt is pending. */
		addi t1, t1, 4					/* 0x80000007 + 4 = 0x8000000b == Machine external interrupt. */
		bne a0, t1, as_yet_unhandled	/* Something as yet unhandled. */

#endif /* portasmHAS_MTIME */

	load_x sp, xISRStackTop				/* Switch to ISR stack before function call. */
	call portasmHANDLE_INTERRUPT			/* Jump to the interrupt handler if there is no CLINT or if there is a CLINT and it has been determined that an external interrupt is pending. */
	j processed_source

handle_synchronous:
	addi a1, a1, 4						/* Synchronous so updated exception return address to the instruction after the instruction that generated the exeption. */
	store_x a1, 0( sp )					/* Save updated exception return address. */

test_if_environment_call:
	li t0, 11 							/* 11 == environment call. */
	bne a0, t0, is_exception			/* Not an M environment call, so some other exception. */
	load_x sp, xISRStackTop				/* Switch to ISR stack before function call. */
	call vTaskSwitchContext
	j processed_source

is_exception:
	csrr t0, mcause						/* For viewing in the debugger only. */
	csrr t1, mepc						/* For viewing in the debugger only */
	csrr t2, mtval
	mv   a0, t0
	mv   a1, t1
	mv   a2, t2
	mv   a3, sp
	call enter_interrupt_handler
	call riscv_cpu_handle_exception
	call exit_interrupt_handler
	j is_exception						/* No other exceptions handled yet. */

as_yet_unhandled:
	csrr t0, mcause						/* For viewing in the debugger only. */
	j as_yet_unhandled

processed_source:
	load_x  t1, pxCurrentTCB			/* Load pxCurrentTCB. */
	load_x  sp, 0( t1 )				 	/* Read sp from first TCB member. */

	/* Load mret with the address of the next instruction in the task to run next. */
	load_x t0, 0( sp )
	csrw mepc, t0

	portasmRESTORE_ADDITIONAL_REGISTERS	/* Defined in freertos_risc_v_chip_specific_extensions.h to restore any registers unique to the RISC-V implementation. */
#ifdef CONFIG_ARCH_RISCV_FPU

#ifndef CONFIG_SAVE_C906_FPU_CONTEXT_IN_TCB

	addi a0, sp, 34 * portWORD_SIZE
	lw s0, FPU_CTX_FCSR_F0(a0)
	li t1, SR_FS
	csrs mstatus, t1
	fld f0, FPU_CTX_F0_F0(a0)
	fld f1, FPU_CTX_F1_F0(a0)
	fld f2, FPU_CTX_F2_F0(a0)
	fld f3, FPU_CTX_F3_F0(a0)
	fld f4, FPU_CTX_F4_F0(a0)
	fld f5, FPU_CTX_F5_F0(a0)
	fld f6, FPU_CTX_F6_F0(a0)
	fld f7, FPU_CTX_F7_F0(a0)
	fld f8, FPU_CTX_F8_F0(a0)
	fld f9, FPU_CTX_F9_F0(a0)
	fld f10,FPU_CTX_F10_F0(a0)
	fld f11,FPU_CTX_F11_F0(a0)
	fld f12,FPU_CTX_F12_F0(a0)
	fld f13,FPU_CTX_F13_F0(a0)
	fld f14,FPU_CTX_F14_F0(a0)
	fld f15,FPU_CTX_F15_F0(a0)
	fld f16,FPU_CTX_F16_F0(a0)
	fld f17,FPU_CTX_F17_F0(a0)
	fld f18,FPU_CTX_F18_F0(a0)
	fld f19,FPU_CTX_F19_F0(a0)
	fld f20,FPU_CTX_F20_F0(a0)
	fld f21,FPU_CTX_F21_F0(a0)
	fld f22,FPU_CTX_F22_F0(a0)
	fld f23,FPU_CTX_F23_F0(a0)
	fld f24,FPU_CTX_F24_F0(a0)
	fld f25,FPU_CTX_F25_F0(a0)
	fld f26,FPU_CTX_F26_F0(a0)
	fld f27,FPU_CTX_F27_F0(a0)
	fld f28,FPU_CTX_F28_F0(a0)
	fld f29,FPU_CTX_F29_F0(a0)
	fld f30,FPU_CTX_F30_F0(a0)
	fld f31,FPU_CTX_F31_F0(a0)
	fscsr s0

#else

	/* get the saved mstatus reg value in thread stack and extract the FS field */
	load_x  s0, 29 * portWORD_SIZE(sp)
	li s1, SR_FS
	and s2, s0, s1

	/* confirm whether the thread need to use FPU */
	li t0, SR_FS_OFF
#ifdef CONFIG_RECORD_C906_FPU_CTX_STATISTICS
	bne s2, t0, fpu_is_not_off_state

	lw t0, g_skip_fpu_off_ctx_restore_cnt
	add t0, t0, 1
	sw t0, g_skip_fpu_off_ctx_restore_cnt, t1
	j skip_fpu_context_restore

fpu_is_not_off_state:
#else
	beq s2, t0, skip_fpu_context_restore /* the value of FS field is OFF, does't need to restore FPU context */
#endif

	/* confirm whether the thread has used FPU previously */
	li t0, SR_FS_INITIAL
#ifdef CONFIG_RECORD_C906_FPU_CTX_STATISTICS
	bne s2, t0, fpu_is_not_init_state

	lw t0, g_skip_fpu_init_ctx_restore_cnt
	add t0, t0, 1
	sw t0, g_skip_fpu_init_ctx_restore_cnt, t1
	j skip_fpu_context_restore

fpu_is_not_init_state:
#else
	beq s2, t0, skip_fpu_context_restore /* the value of FS field is initial, does't need to restore FPU context */
#endif

	load_x s3, pxCurrentTCB
	li s4, SR_FS_CLEAN

	/* handle abnormal situation. the value of FS field should not be dirte value in here!
	 * maybe the TCB of this thread has been modified accidentally.
	 */
	li s5, SR_FS_DIRTY
	beq s2, s5, fpu_context_restore /* the value of FS field is dirty, force to restore the FPU context in this situation. */

	/* confirm whether there is a thread switch operation after execute vTaskSwitchContext function */
	load_x t0, s_rtos_tcb_when_trap
	bne s3, t0, fpu_context_restore /* thread switch has been happened, need to restore FPU context*/

	/* the value of FS field(in thread stack) is clean and no thread switch, confirm whether the current FPU is clean state */
	csrr t0, mstatus
	and t1, t0, s1
#ifdef CONFIG_RECORD_C906_FPU_CTX_STATISTICS
	bne t1, s4, fpu_is_not_clean_state

	lw t0, g_skip_fpu_clean_ctx_restore_cnt
	add t0, t0, 1
	sw t0, g_skip_fpu_clean_ctx_restore_cnt, t1
	j skip_fpu_context_restore

fpu_is_not_clean_state:
#else
	beq t1, s4, skip_fpu_context_restore /* FPU is not used(clean state) since enter trap handler, does't need to restore FPU context */
#endif

fpu_context_restore:
	/* get the offset of arch_fpu_context_t member */
	lw t0, g_fpu_ctx_member_offset
	add a0, s3, t0

	lw  t0, FPU_CTX_FCSR_F0(a0)
	csrs mstatus, s1
	fld f0, FPU_CTX_F0_F0(a0)
	fld f1, FPU_CTX_F1_F0(a0)
	fld f2, FPU_CTX_F2_F0(a0)
	fld f3, FPU_CTX_F3_F0(a0)
	fld f4, FPU_CTX_F4_F0(a0)
	fld f5, FPU_CTX_F5_F0(a0)
	fld f6, FPU_CTX_F6_F0(a0)
	fld f7, FPU_CTX_F7_F0(a0)
	fld f8, FPU_CTX_F8_F0(a0)
	fld f9, FPU_CTX_F9_F0(a0)
	fld f10,FPU_CTX_F10_F0(a0)
	fld f11,FPU_CTX_F11_F0(a0)
	fld f12,FPU_CTX_F12_F0(a0)
	fld f13,FPU_CTX_F13_F0(a0)
	fld f14,FPU_CTX_F14_F0(a0)
	fld f15,FPU_CTX_F15_F0(a0)
	fld f16,FPU_CTX_F16_F0(a0)
	fld f17,FPU_CTX_F17_F0(a0)
	fld f18,FPU_CTX_F18_F0(a0)
	fld f19,FPU_CTX_F19_F0(a0)
	fld f20,FPU_CTX_F20_F0(a0)
	fld f21,FPU_CTX_F21_F0(a0)
	fld f22,FPU_CTX_F22_F0(a0)
	fld f23,FPU_CTX_F23_F0(a0)
	fld f24,FPU_CTX_F24_F0(a0)
	fld f25,FPU_CTX_F25_F0(a0)
	fld f26,FPU_CTX_F26_F0(a0)
	fld f27,FPU_CTX_F27_F0(a0)
	fld f28,FPU_CTX_F28_F0(a0)
	fld f29,FPU_CTX_F29_F0(a0)
	fld f30,FPU_CTX_F30_F0(a0)
	fld f31,FPU_CTX_F31_F0(a0)
	fscsr t0

	bne s2, s5, skip_fpu_state_update
	/* set FS filed(in thread stack) to clean state when the value of FS field is dirty. */
	not t1, s1
	and s0, s0, t1
	or s0, s0, s4
	store_x s0, 29 * portWORD_SIZE(sp)

skip_fpu_state_update:

skip_fpu_context_restore:
	/* no special operation here, the FS field of mstatus register will be correctly restored later */
#endif

#endif

#ifdef CONFIG_ARCH_RISCV_VECTOR
	/* Get mstatus.vs saved state from stack */
	load_x  s0, 29 * portWORD_SIZE(sp)
	li s1, SR_VS
	and s2, s0, s1

	/* set mstatus.vs = clean (stack) */
	not t0, s1
	and t1, s0, t0
	li t2, SR_VS_CLEAN
	or t3, s0, t2
	store_x t3, 29 * portWORD_SIZE(sp)

	/* if current task == pre task, skip restore vector context */
	load_x s3, pxCurrentTCB
	load_x t0, s_rtos_tcb_when_trap
	beq s3, t0, skip_vector_context_restore

	/* Confirm whether vector context needs to be restore */
	li t0, SR_VS_OFF
	beq s2, t0, skip_vector_context_restore /* if mstatus.VS = OFF, skip restore vector context */
	li t0, SR_VS_INITIAL
	beq s2, t0, skip_vector_context_restore /* if mstatus.VS = initial, skip restore vector context */
	li t0, SR_FS_CLEAN
	beq s2, t0, skip_vector_context_restore /* if mstatus.VS = clean, skip restore vector context */

vector_context_restore: #SR_FS_DIRTY
	/* get the offset of arch_fpu_context_t member */
	load_x t0, g_vector_ctx_member_offset
	add a0, s3, t0
	mv a1, a0

vstate_vx_restore:
	vsetvli        t4, x0, e8, m8
	vle.v          v0, (a0)
	add            a0, a0, t4
	vle.v          v8, (a0)
	/* Force only use v0 - v15 */
#	add            a0, a0, t4
#	vle.v          v16, (a0)
#	add            a0, a0, t4
#	vle.v          v24, (a0)

vstate_csr_restore:
	load_x t0, VECTOR_CTX_VTYPE_V0(a1)
	vsetvl  t4, x0, t0
	load_x t0, VECTOR_CTX_VSTART_V0(a1)
	csrw    vstart, t0

skip_vector_context_restore:
#endif

	/* Load mstatus with the interrupt enable bits used by the task. */
	load_x  t0, 29 * portWORD_SIZE( sp )
	csrw mstatus, t0						/* Required for MPIE bit. */

	load_x  x1, 1 * portWORD_SIZE( sp )
	load_x  x5, 2 * portWORD_SIZE( sp )		/* t0 */
	load_x  x6, 3 * portWORD_SIZE( sp )		/* t1 */
	load_x  x7, 4 * portWORD_SIZE( sp )		/* t2 */
	load_x  x8, 5 * portWORD_SIZE( sp )		/* s0/fp */
	load_x  x9, 6 * portWORD_SIZE( sp )		/* s1 */
	load_x  x10, 7 * portWORD_SIZE( sp )	/* a0 */
	load_x  x11, 8 * portWORD_SIZE( sp )	/* a1 */
	load_x  x12, 9 * portWORD_SIZE( sp )	/* a2 */
	load_x  x13, 10 * portWORD_SIZE( sp )	/* a3 */
	load_x  x14, 11 * portWORD_SIZE( sp )	/* a4 */
	load_x  x15, 12 * portWORD_SIZE( sp )	/* a5 */
	load_x  x16, 13 * portWORD_SIZE( sp )	/* a6 */
	load_x  x17, 14 * portWORD_SIZE( sp )	/* a7 */
	load_x  x18, 15 * portWORD_SIZE( sp )	/* s2 */
	load_x  x19, 16 * portWORD_SIZE( sp )	/* s3 */
	load_x  x20, 17 * portWORD_SIZE( sp )	/* s4 */
	load_x  x21, 18 * portWORD_SIZE( sp )	/* s5 */
	load_x  x22, 19 * portWORD_SIZE( sp )	/* s6 */
	load_x  x23, 20 * portWORD_SIZE( sp )	/* s7 */
	load_x  x24, 21 * portWORD_SIZE( sp )	/* s8 */
	load_x  x25, 22 * portWORD_SIZE( sp )	/* s9 */
	load_x  x26, 23 * portWORD_SIZE( sp )	/* s10 */
	load_x  x27, 24 * portWORD_SIZE( sp )	/* s11 */
	load_x  x28, 25 * portWORD_SIZE( sp )	/* t3 */
	load_x  x29, 26 * portWORD_SIZE( sp )	/* t4 */
	load_x  x30, 27 * portWORD_SIZE( sp )	/* t5 */
	load_x  x31, 28 * portWORD_SIZE( sp )	/* t6 */
	addi sp, sp, portCONTEXT_SIZE

	mret
	.endfunc
/*-----------------------------------------------------------*/

.align 8
.func
xPortStartFirstTask:

#if( portasmHAS_SIFIVE_CLINT != 0 )
	/* If there is a clint then interrupts can branch directly to the FreeRTOS
	trap handler.  Otherwise the interrupt controller will need to be configured
	outside of this file. */
	la t0, freertos_risc_v_trap_handler
	csrw mtvec, t0
#endif /* portasmHAS_CLILNT */

	load_x  sp, pxCurrentTCB			/* Load pxCurrentTCB. */
	load_x  sp, 0( sp )				 	/* Read sp from first TCB member. */

	load_x  x1, 0( sp ) /* Note for starting the scheduler the exception return address is used as the function return address. */

	portasmRESTORE_ADDITIONAL_REGISTERS	/* Defined in freertos_risc_v_chip_specific_extensions.h to restore any registers unique to the RISC-V implementation. */

	load_x  x6, 3 * portWORD_SIZE( sp )		/* t1 */
	load_x  x7, 4 * portWORD_SIZE( sp )		/* t2 */
	load_x  x8, 5 * portWORD_SIZE( sp )		/* s0/fp */
	load_x  x9, 6 * portWORD_SIZE( sp )		/* s1 */
	load_x  x10, 7 * portWORD_SIZE( sp )	/* a0 */
	load_x  x11, 8 * portWORD_SIZE( sp )	/* a1 */
	load_x  x12, 9 * portWORD_SIZE( sp )	/* a2 */
	load_x  x13, 10 * portWORD_SIZE( sp )	/* a3 */
	load_x  x14, 11 * portWORD_SIZE( sp )	/* a4 */
	load_x  x15, 12 * portWORD_SIZE( sp )	/* a5 */
	load_x  x16, 13 * portWORD_SIZE( sp )	/* a6 */
	load_x  x17, 14 * portWORD_SIZE( sp )	/* a7 */
	load_x  x18, 15 * portWORD_SIZE( sp )	/* s2 */
	load_x  x19, 16 * portWORD_SIZE( sp )	/* s3 */
	load_x  x20, 17 * portWORD_SIZE( sp )	/* s4 */
	load_x  x21, 18 * portWORD_SIZE( sp )	/* s5 */
	load_x  x22, 19 * portWORD_SIZE( sp )	/* s6 */
	load_x  x23, 20 * portWORD_SIZE( sp )	/* s7 */
	load_x  x24, 21 * portWORD_SIZE( sp )	/* s8 */
	load_x  x25, 22 * portWORD_SIZE( sp )	/* s9 */
	load_x  x26, 23 * portWORD_SIZE( sp )	/* s10 */
	load_x  x27, 24 * portWORD_SIZE( sp )	/* s11 */
	load_x  x28, 25 * portWORD_SIZE( sp )	/* t3 */
	load_x  x29, 26 * portWORD_SIZE( sp )	/* t4 */
	load_x  x30, 27 * portWORD_SIZE( sp )	/* t5 */
	load_x  x31, 28 * portWORD_SIZE( sp )	/* t6 */

	load_x  x5, 29 * portWORD_SIZE( sp )	/* Initial mstatus into x5 (t0) */
	addi x5, x5, 0x08						/* Set MIE bit so the first task starts with interrupts enabled - required as returns with ret not eret. */
	csrrw  x0, mstatus, x5					/* Interrupts enabled from here! */
	load_x  x5, 2 * portWORD_SIZE( sp )		/* Initial x5 (t0) value. */

	addi	sp, sp, portCONTEXT_SIZE
	ret
	.endfunc
/*-----------------------------------------------------------*/

/*
 * Unlike other ports pxPortInitialiseStack() is written in assembly code as it
 * needs access to the portasmADDITIONAL_CONTEXT_SIZE constant.  The prototype
 * for the function is as per the other ports:
 * StackType_t *pxPortInitialiseStack( StackType_t *pxTopOfStack, TaskFunction_t pxCode, void *pvParameters );
 *
 * As per the standard RISC-V ABI pxTopcOfStack is passed in in a0, pxCode in
 * a1, and pvParameters in a2.  The new top of stack is passed out in a0.
 *
 * RISC-V maps registers to ABI names as follows (X1 to X31 integer registers
 * for the 'I' profile, X1 to X15 for the 'E' profile, currently I assumed).
 *
 * Register		ABI Name	Description						Saver
 * x0			zero		Hard-wired zero					-
 * x1			ra			Return address					Caller
 * x2			sp			Stack pointer					Callee
 * x3			gp			Global pointer					-
 * x4			tp			Thread pointer					-
 * x5-7			t0-2		Temporaries						Caller
 * x8			s0/fp		Saved register/Frame pointer	Callee
 * x9			s1			Saved register					Callee
 * x10-11		a0-1		Function Arguments/return values Caller
 * x12-17		a2-7		Function arguments				Caller
 * x18-27		s2-11		Saved registers					Callee
 * x28-31		t3-6		Temporaries						Caller
 *
 * The RISC-V context is saved t FreeRTOS tasks in the following stack frame,
 * where the global and thread pointers are currently assumed to be constant so
 * are not saved:
 *
 * mstatus
 * x31
 * x30
 * x29
 * x28
 * x27
 * x26
 * x25
 * x24
 * x23
 * x22
 * x21
 * x20
 * x19
 * x18
 * x17
 * x16
 * x15
 * x14
 * x13
 * x12
 * x11
 * pvParameters
 * x9
 * x8
 * x7
 * x6
 * x5
 * portTASK_RETURN_ADDRESS
 * [chip specific registers go here]
 * pxCode
 */
.align 8
.func
pxPortInitialiseStack:

#ifdef CONFIG_ARCH_RISCV_FPU
	addi a0, a0, -(34 * portWORD_SIZE)
	/* set fpu rounding mode field to 0(Round to Nearest, ties to Even) when create new thread,
	 * otherwise it's possible that the rounding mode filed is invalid and
	 * cause the illegal instruction exception when restore the fcsr.
	 */
	store_x x0, FPU_CTX_FCSR_F0(a0)
#endif
	addi a0, a0, -portWORD_SIZE
	addi a0, a0, -portWORD_SIZE
	addi a0, a0, -portWORD_SIZE
	addi a0, a0, -portWORD_SIZE
	csrr t0, mstatus					/* Obtain current mstatus value. */
	andi t0, t0, ~0x8					/* Ensure interrupts are disabled when the stack is restored within an ISR.  Required when a task is created after the schedulre has been started, otherwise interrupts would be disabled anyway. */
	addi t1, x0, 0x188					/* Generate the value 0x1880, which are the MPIE and MPP bits to set in mstatus. */
	slli t1, t1, 4
	or t0, t0, t1						/* Set MPIE and MPP bits in mstatus value. */

#ifdef CONFIG_SAVE_C906_FPU_CONTEXT_IN_TCB
	/* set FS filed(in thread stack) to initial state for skip save/restore FPU context operation */
	li t1, SR_FS
	li t2, SR_FS_INITIAL
	not t3, t1
	and t0, t0, t3
	or t0, t0, t2
#endif

	addi a0, a0, -portWORD_SIZE
	store_x t0, 0(a0)					/* mstatus onto the stack. */
	addi a0, a0, -(22 * portWORD_SIZE)	/* Space for registers x11-x31. */
	store_x a2, 0(a0)					/* Task parameters (pvParameters parameter) goes into register X10/a0 on the stack. */
	addi a0, a0, -(6 * portWORD_SIZE)	/* Space for registers x5-x9. */
	la   t0, portTASK_RETURN_ADDRESS
	store_x t0, 0(a0)					/* Return address onto the stack, could be portTASK_RETURN_ADDRESS */
	addi t0, x0, portasmADDITIONAL_CONTEXT_SIZE /* The number of chip specific additional registers. */
chip_specific_stack_frame:				/* First add any chip specific registers to the stack frame being created. */
	beq t0, x0, 1f						/* No more chip specific registers to save. */
	addi a0, a0, -portWORD_SIZE			/* Make space for chip specific register. */
	store_x x0, 0(a0)					/* Give the chip specific register an initial value of zero. */
	addi t0, t0, -1						/* Decrement the count of chip specific registers remaining. */
	j chip_specific_stack_frame			/* Until no more chip specific registers. */
1:
	addi a0, a0, -portWORD_SIZE
	store_x a1, 0(a0)					/* mret value (pxCode parameter) onto the stack. */
	ret
	.endfunc
/*-----------------------------------------------------------*/

