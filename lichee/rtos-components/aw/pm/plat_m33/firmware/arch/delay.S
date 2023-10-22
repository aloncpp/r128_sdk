/*
 * linux/arch/arm/lib/lib1funcs.S: Optimized ARM division routines
 *
 * Author: Nicolas Pitre <nico@fluxnic.net>
 *   - contributed to gcc-3.4 on Sep 30, 2003
 *   - adapted for the Linux kernel on Oct 2, 2003
 */


  .syntax unified
  .cpu cortex-m33
  .fpu softvfp
  .thumb

  .equ LOOP_COEFFICIENT, (17)  /*1008M*/

  .text
  .globl standby_delay
standby_delay:
	push    {r0-r3}
	ldr     r1, =LOOP_COEFFICIENT
	mov     r3, r0
standby_delay_loop2:
	subs    r0, r0, #1
	bhi     standby_delay_loop2
	mov     r0, r3
	subs    r1, r1, #1
	bhi     standby_delay_loop2

	pop     {r0-r3}
	mov     pc, lr

  .text
  .globl standby_delay_cycle
standby_delay_cycle:
standby_delay_cycle_loop3:
	subs    r0, r0, #1
	bhi     standby_delay_cycle_loop3
	mov     pc, lr

