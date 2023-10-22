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
#ifdef CONFIG_CPU_CM33F
  .cpu cortex-m33
  .fpu softvfp
#elif (defined CONFIG_CPU_CM4F)
  .cpu cortex-m4
  .fpu softvfp
#else
  .cpu cortex-m3
#endif
  .thumb

.global g_pfnVectors
.global Default_Handler
/* start address for the initialization values of the .data section.
defined in linker script */
.word _sidata
/* start address for the .data section. defined in linker script */
.word _sdata
/* end address for the .data section. defined in linker script */
.word _edata
/* start address for the .bss section. defined in linker script */
.word _sbss
.word __psram_bss_start__
.word __hpsram_bss_start__
/* end address for the .bss section. defined in linker script */
.word _ebss
.word __psram_bss_end__
.word __hpsram_bss_end__
/* bottom of stack */
//.word _sstack

  .section .text.Reset_Handler
  .weak Reset_Handler
  .type Reset_Handler, %function

Reset_Handler:
  cpsid i
  cpsid f
  ldr   r0, =_estack
  mov   sp, r0          /* set stack pointer */

  ldr  r2, =_sbss
  b  LoopFillZerobss

  /* Zero fill the bss segment. */
FillZerobss:
  movs  r3, #0
  str  r3, [r2], #4
LoopFillZerobss:
  ldr  r3, = _ebss
  cmp  r2, r3
bcc  FillZerobss

#if (defined(CONFIG_LPSRAM))
  ldr  r2, =__psram_bss_start__
  b  LoopFillZerolpbss
FillZerolpbss:
  movs  r3, #0
  str  r3, [r2], #4
LoopFillZerolpbss:
  ldr  r3, = __psram_bss_end__
  cmp  r2, r3
bcc  FillZerolpbss
#endif

#if (defined(CONFIG_HPSRAM))
  ldr  r2, =__hpsram_bss_start__
  b  LoopFillZerohpbss
FillZerohpbss:
  movs  r3, #0
  str  r3, [r2], #4
LoopFillZerohpbss:
  ldr  r3, = __hpsram_bss_end__
  cmp  r2, r3
bcc  FillZerohpbss
#endif

#ifdef CONFIG_CPU_CM33F
  //ldr   r0, =_sstack  /* set stack bottom */
  //msr   msplim, r0
#endif
  bl   start_kernel

LoopForever:
  b LoopForever
.size Reset_Handler, .-Reset_Handler

/**
 * @brief  This is the code that gets called when the processor receives an
 *         unexpected interrupt.  This simply enters an infinite loop, preserving
 *         the system state for examination by a debugger.
 *
 * @param  None
 * @retval : None
*/
  .extern exception_entry

  .section .cpu_text,"ax",%progbits
  .thumb_func
  .type Default_Handler, %function

Default_Handler:
  CPSID F
  TST LR, #0x04
  ITE EQ
  MRSEQ R0, MSP
  MRSNE R0, PSP
  STMDB.W R0!, {R4-R11}
  MRS R1, MSP
  MRS R2, PSP
  BL exception_entry
.size Default_Handler, .-Default_Handler

/******************************************************************************
*
* The minimal vector table for a Cortex M3.  Note that the proper constructs
* must be placed on this to ensure that it ends up at physical address
* 0x0000.0000.
*
******************************************************************************/
  .section .isr_vector,"a",%progbits
  .type g_pfnVectors, %object
  .size g_pfnVectors, .-g_pfnVectors

g_pfnVectors:
  .word _estack
  .word Reset_Handler
  .word Default_Handler // NMI_Handler              // -14
  .word Default_Handler // HardFault_Handler
  .word Default_Handler // MemManage_Handler
  .word Default_Handler // BusFault_Handler
  .word Default_Handler // UsageFault_Handler
  .word Default_Handler //
  .word Default_Handler //
  .word Default_Handler //
  .word Default_Handler //
  .word SVC_Handler                                 // -5
  .word Default_Handler // DebugMon_Handler
  .word Default_Handler //
  .word PendSV_Handler
  .word SysTick_Handler                             // -1

  /* External Interrupts */
  .word Default_Handler // 0
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler // 10
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler // 20
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler // 30
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler // 40
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler // 50
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler // 60
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler // 70
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler // 80
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler // 90
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler // 100
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler // 110
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler // 120
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler
  .word Default_Handler // 127
