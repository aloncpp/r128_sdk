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

  .syntax unified
  .cpu cortex-m33
  .fpu softvfp
  .thumb

/* import depends function, must place at sram space */
  .global wakeup_check_callback
  .global standby_CleanDCache

  .equ M33_SYST_CSR,         (0xe000e000 + 0x010)
  .equ M33_SCR,              (0xe000e000 + 0xd10)
  .equ M33_ICSR,             (0xe000e000 + 0xd04)
  .equ GPRCM_CPUA_BOOT_FLAG, (0x40050000 + 0x1c0)
  .equ GPRCM_CPUA_BOOT_ADDR, (0x40050000 + 0x1c4)
  .equ GPRCM_CPUA_BOOT_ARG,  (0x40050000 + 0x1c8)
  .equ GPRCM_GPDATA1,        (0x40050000 + 0x204)

  .section .data
m33_regs:
.rept 64
.word 0
.endr

  .thumb_func
  .section .text
  .type	_fw_cpu_sleep, %function
  .global _fw_cpu_sleep

  /*------------------ void _fw_cpu_sleep(int nouse) ------------------------*/
_fw_cpu_sleep:
  .fnstart
  .cantunwind

  PUSH {R0-R12, LR}

  /* bl standby_CleanDCache // it was previously disabled */

  DSB
  ISB
  NOP

  MRS R0, PRIMASK
  LDR R1, =0x1
  ISB
  MSR PRIMASK, R1
  ISB

  MRS R1, FAULTMASK
  LDR R2, =0x0
  ISB
  MSR FAULTMASK, R2

  ISB
  WFI

  MSR FAULTMASK, R1
  MSR PRIMASK, R0
  ISB

  POP {R0-R12, PC}

  .fnend
  .size   _fw_cpu_sleep, .-_fw_cpu_sleep


  /*------------------ void _fw_cpu_standby(int nouse) ------------------------*/
  .thumb_func
  .section .text
  .type   _fw_cpu_standby, %function
  .global _fw_cpu_standby

_fw_cpu_standby:
  .fnstart
  .cantunwind

  PUSH {R0-R12, LR}
  ISB

  /*
   * save MSP,PSP,PRIMASK,FAULTMASK,BASEPRI,CONTROL to space is pointed by
   * GPRCM_CPUA_BOOT_ARG.
   */
  LDR R0, =m33_regs
  LDR R1, =GPRCM_CPUA_BOOT_ARG
  ISB
  STR R0, [R1]

  LDR R0, =GPRCM_CPUA_BOOT_ARG
  ISB
  LDR R1, [R0]

  MRS R0, MSP
  ISB
  STR R0, [R1]

  MRS R0, PSP
  ISB
  STR R0, [R1, #4]

  MRS R0, PRIMASK
  STR R0, [R1, #12]

  MRS R0, FAULTMASK
  STR R0, [R1, #16]

  MRS R0, BASEPRI
  STR R0, [R1, #20]

  MRS R0, CONTROL
  STR R0, [R1, #24]

  DSB
  ISB
  NOP

  /* bl standby_CleanDCache // it was previously disabled */

  /* set deepsleep mode */
  LDR R0, =0x14
  LDR R1, =M33_SCR
  ISB
  STR R0, [R1]

  /* set bootflag */
/*
  LDR R0, =0x429b0001
  LDR R1, =GPRCM_CPUA_BOOT_FLAG
  ISB
  STR R0, [R1]
*/

  /* set resume address in thumb state */
  LDR R0, =__resume_standby
  ORR.W R0, R0, #1  @format thumb instruction
  LDR R1, =GPRCM_CPUA_BOOT_ADDR
  ISB
  STR R0, [R1]

  DSB
  ISB
  NOP

  /* the WFE instruction will cause two kinds of CPU actions:
   * 1. EVNET_REGISTER = 1, WFE will clear the EVENT_REGISTER and the
   *	 CPU executes the next instruction.
   * 2. EVENT_REGISTER = 0, WFE will make the CPU go to SLEEP state.
   */
  /* first time executing WFE instruction, there are some different
   * situations as follows:
   * 1. if there are interrupts pending and be cleared already,
   *	 the WFE will only clear the CPU EVENT_REGISTER.
   * 2. if there are new interrupts pending after ar400_deepsleep_lock
   *	 operation, the WFE will only clear the CPU EVENT_REGISTER.
   * 3. if the SEV/NMI/DEBUG events coming before now, WFE will only
   *	 clear the CPU EVENT_REGISTER.
   * 4. if there are no SEV/NMI/DEBUG events before and no interrupts
   *	 pending too, WFE wil make the CPU go to the SLEEP state.
   */

  WFE

  /* read the NVIC SET_PENDING_REGISTER to check whether there are
   *  any new pending interrupts after ar400_deepsleep_lock operation
   *  which make the first WFE executing failed.
   * 1. If ther are some new pending interrupts, jump to the RESUME_ENTRY
   *	 and abandon the next WFE execution.
   * 2. If there is no new pending interrupts, we execute WFE instruction
   *	 twice to ensure the CPU goes to SLEEP state successfully.
   */
  BL wakeup_check_callback
  CMP R0, #0
  BNE __resume_standby

  DSB
  ISB
  NOP

  WFE

__resume_standby:
  DSB
  ISB

  /* remove bootaddr */

  /* remove bootflag */
/*
  LDR R0, =0x429b0000
  LDR R1, =GPRCM_CPUA_BOOT_FLAG
  ISB
  STR R0, [R1]
*/

  /* set normal mode */
  MOV R0, 0
  LDR R1, =M33_SCR
  ISB
  STR R0, [R1]

  /* restore cpu contex */
  LDR R0, =GPRCM_CPUA_BOOT_ARG
  ISB
  LDR R1, [R0]

  LDR R0, [R1]
  MSR MSP, R0
  ISB

  LDR R0, [R1,#4]
  MSR PSP, R0
  ISB

  LDR R0, [R1, #12]
  MSR PRIMASK, R0

  LDR R0, [R1, #16]
  MSR FAULTMASK, R0

  LDR R0, [R1, #20]
  MSR BASEPRI, R0

  LDR R0, [R1, #24]
  MSR CONTROL, R0
  ISB
  NOP

  POP {R0-R12, PC}
  NOP

  .fnend
  .size   _fw_cpu_standby, .-_fw_cpu_standby

