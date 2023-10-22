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
#include <stdio.h>
#include <stdint.h>
#include <strings.h>
#include <interrupt.h>
#include <irqs.h>
#include <platform.h>
#include <memory.h>
#include <core_cm33.h>
#include <io.h>
#include <sunxi_hal_common.h>

#ifdef CONFIG_DEBUG_BACKTRACE
#include <backtrace.h>
#endif

#ifdef CONFIG_COMPONENTS_BOOT_REASON
#include <boot_reason.h>
#endif

#define DUMP_BUF_LEN        (1024 / 4)

#define  CPU_REG_NVIC_ICSR      ((uint32_t)readl(0xE000ED04))     /* Int Ctrl State  Reg.         */
#define  CPU_REG_NVIC_SHCSR     ((uint32_t)readl(0xE000ED24))     /* Hard  Fault Status Reg.      */
#define  CPU_REG_NVIC_HFSR      ((uint32_t)readl(0xE000ED2C))     /* Hard  Fault Status Reg.      */
#define  CPU_REG_NVIC_DFSR      ((uint32_t)readl(0xE000ED30))     /* Debug Fault Status Reg.      */
#define  CPU_REG_NVIC_MMFAR     ((uint32_t)readl(0xE000ED34))     /* Mem Manage Addr Reg.         */
#define  CPU_REG_NVIC_BFAR      ((uint32_t)readl(0xE000ED38))     /* Bus Fault  Addr Reg.         */
#define  CPU_REG_FPU_FPSCR      ((uint32_t)readl(0xE000ED38))     /* FP status and control Reg.   */

#define CHIP_TEXT_START (0x00000000)    /* include ROM APP and XIP */
#define SRAM_TEXT_START (0x04000000)    /*  only include APP vector */
#define CHIP_TEXT_END   (0x24000000)    /* 0x01400000:16MB XIP End, 0x01C00000:8MB PSRAM End */
#define CHIP_STACK_END  (0x040C0000)    /* 512K SRAM */

/* defined for other modules to trace exception */
volatile int exceptin_step;
volatile int exceptin_dump_addr;
volatile int exceptin_dump_len;

typedef void (*panic_cli_handler) (void);

static panic_cli_handler __panic_cli;

#ifdef CONFIG_EXCEPTION_CAUSE_STR
static char *ufsr_err_str[] = {
    "unknown",
    "undefine instruction",
    "invalid state",
    "invalid pc",
    "no coprocessor",
    "stack overflow",
    "unknown",
    "unknown",
    "unknown",
    "unaligned",
    "div by zero",
};

static char *dfsr_err_str[] = {
    "unknown",
    "halt",
    "bkpt",
    "dwttrap",
    "vcatch",
    "external",
};
#endif

void exception_register_panic_cli(void *cli)
{
	__panic_cli = cli;
}

/*
 * GET CURRENT EXCETOPM
 *
 * Description: get the source number of current exception, include all exceptions.
 * Arguments  : none.
 * Returns    : the source number of current exception.
 * Note       :
 */
static uint32_t intc_get_current_exception(void)
{
	return (CPU_REG_NVIC_ICSR & 0x1ff);
}

static void exception_hex_dump(const uint32_t *addr, uint32_t num)
{
	uint32_t i;

	for (i = 0; i < num; i++) {
		if ((i & 0x03) == 0x0)
			printf("\n[%p]: ", addr);
		printf("0x%08x ", *addr++);
	}
	printf("\n");
}

/*
 * EXCEPTION ENTRY
 *
 * Description:  the entry of CPU exception, mainly for CPU nmi, hard fault,
 * memmanger fault, bus fault, usage fault, SVCall, debugmonitor exception.
 *
 * register list in stack:
 * ---------LOW ADDR--------
 * R4            <-pstack
 * R5            <-pstack+4*1
 * R6            <-pstack+4*2
 * R7            <-pstack+4*3
 * R8            <-pstack+4*4
 * R9            <-pstack+4*5
 * R10           <-pstack+4*6
 * R11           <-pstack+4*7
 * R0            <-PSP
 * R1            <-PSP+4*1
 * R2            <-PSP+4*2
 * R3            <-PSP+4*3
 * R12           <-PSP+4*4
 * R14(LR)       <-PSP+4*5
 * R15(PC)       <-PSP+4*6
 * xPSR          <-PSP+4*7
 * --------HIGH ADDR--------
 * Arguments  :  pstack:the pointer of stack, PSP before the exception happen.
 * Returns    :  OK if process CPU exception succeeded, others if failed.
 */
int32_t exception_entry(uint32_t *pstack, uint32_t *msp, uint32_t *psp)
{
	uint32_t i;
	uint32_t exceptionno = intc_get_current_exception();
	const uint32_t *addr = pstack + 16;
	uint32_t lr, pc, xpsr;
	uint32_t len;
	uint32_t exec_pc = 0, exec_lr = 0, exec_sp = 0;
#ifdef CONFIG_CPU_CM33F
	uint32_t msplim = __get_MSPLIM();
	uint32_t psplim = __get_PSPLIM();
#endif

	printf("\nexception:%u happen!!\n", exceptionno);
	printf("appos pstack:0x%x msp:0x%x psp:0x%x\n",
	       (uint32_t)pstack, (uint32_t)msp, (uint32_t)psp);
#ifdef CONFIG_CPU_CM33F
	printf("msplim:0x%x psplim:0x%x\n", msplim, psplim);
#endif

	switch (exceptionno) {
	case 2:
		printf("NMI happen\n");
		break;
	case 3:
		printf("hard  fault       happen, HFSR:0x%x\n", CPU_REG_NVIC_HFSR);
		printf("memm  fault maybe happen, MFSR:0x%x, MMFAR:0x%x\n",
		       readb(0xE000ED28), CPU_REG_NVIC_MMFAR);
		printf("bus   fault maybe happen, BFSR:0x%x, BFAR:0x%x\n",
		       readb(0xE000ED29), CPU_REG_NVIC_BFAR);
#ifdef CONFIG_EXCEPTION_CAUSE_STR
		printf("usage fault maybe happen, UFSR:0x%x, cause:%s\n", readw(0xE000ED2A), ufsr_err_str[ffs(readw(0xE000ED2A))]);
#else
		printf("usage fault maybe happen, UFSR:0x%x\n", readw(0xE000ED2A));
#endif
		break;
	case 4:
		printf("memm fault happen, MFSR:0x%x, MMFAR:0x%x\n",
		       readb(0xE000ED28), CPU_REG_NVIC_MMFAR);
		break;
	case 5:
		printf("bus fault happen, BFSR:0x%x, BFAR:0x%x\n",
		       readb(0xE000ED29), CPU_REG_NVIC_BFAR);
		break;
	case 6:
#ifdef CONFIG_EXCEPTION_CAUSE_STR
		printf("usage fault happen, UFSR:0x%x, cause:%s\n", readw(0xE000ED2A), ufsr_err_str[ffs(readw(0xE000ED2A))]);
#else
		printf("usage fault happen, UFSR:0x%x\n", readw(0xE000ED2A));
#endif
		break;
	case 11:
		printf("SVCall fault happen\n");
		break;
	case 12:
#ifdef CONFIG_EXCEPTION_CAUSE_STR
		printf("Debug Monitor happen, DFSR:0x%x, cause:%s\n", CPU_REG_NVIC_DFSR, dfsr_err_str[ffs(CPU_REG_NVIC_DFSR)]);
#else
		printf("Debug Monitor happen, DFSR:0x%x\n", CPU_REG_NVIC_DFSR);
#endif
		break;
	default:
		/* invalid exception nr */
		printf("invalid exception nr\n");
		break;
	}

	/* print R0-R3 */
	printf("CPU registers:\n");
	for (i = 0; i <= 3; i++) {
		printf("R%02u:[%p]: 0x%08x\n", i, (pstack + (8 + i)), *(pstack + (8 + i)));
	}

	/* print R4-R11 */
	for (i = 0; i <= 7; i++) {
		printf("R%02u:[%p]: 0x%08x\n", i + 4, (pstack + i), *(pstack + i));
	}

	/* print R12, R14(LR), R15(PC), xPSR */
	printf("R12:[%p]: 0x%08x\n", (pstack + 12), *(pstack + 12));
	exec_sp = (uint32_t)addr;
	printf("R13(SP):[%p]: %p\n", addr, addr);
	lr = *(pstack + 13);
	exec_lr = lr;
	printf("R14(LR):[%p]: 0x%08x\n", (pstack + 13), lr);
	pc = *(pstack + 14);
	exec_pc = pc;
	printf("R15(PC):[%p]: 0x%08x\n", (pstack + 14), pc);
	xpsr = *(pstack + 15);
	printf("xPSR:[%p]: 0x%08x\n", (pstack + 15), xpsr);
	printf("SHCSR:0x%08x step:%x\n", CPU_REG_NVIC_SHCSR, exceptin_step);
#if ((__FPU_PRESENT == 1) && (__FPU_USED == 1))
	printf("FPSCR:0x%08x\n", CPU_REG_FPU_FPSCR);
#endif

#ifdef CONFIG_DEBUG_BACKTRACE
	printf("\r\n----backtrace information----\r\n");
	backtrace_exception((print_function)printf, xpsr, exec_sp, exec_pc, exec_lr);
	printf("-----------------------------\r\n");
#endif

	printf("\nstack info:");
	if ((((uint32_t)addr) + DUMP_BUF_LEN*4) <= (uint32_t)CHIP_STACK_END)
		len = DUMP_BUF_LEN;
	else
		len = (((uint32_t)CHIP_STACK_END) - ((uint32_t)addr))/4;
	len = MIN(len, DUMP_BUF_LEN*2);
	exception_hex_dump(addr, len);

	printf("\n[LR]:0x%x", lr);
	lr &= ~0x07;
	if (lr >= (uint32_t)CHIP_TEXT_START && lr < (uint32_t)CHIP_TEXT_END) {
		lr -= DUMP_BUF_LEN * 2;
		exception_hex_dump((const uint32_t *)lr, DUMP_BUF_LEN);
	}

	printf("\n[PC]:0x%x", pc);
	pc &= ~0x07;
	if (pc >= (uint32_t)CHIP_TEXT_START && pc < (uint32_t)CHIP_TEXT_END) {
		pc -= DUMP_BUF_LEN * 2;
		exception_hex_dump((const uint32_t *)pc, DUMP_BUF_LEN);
	}

	if (lr > (uint32_t)SRAM_TEXT_START && pc > (uint32_t)SRAM_TEXT_START) {
		printf("\n[text start]:0x%x", (uint32_t)SRAM_TEXT_START);
		exception_hex_dump((const uint32_t *)SRAM_TEXT_START, DUMP_BUF_LEN/2);
	}

	printf("\n[NVIC Pending]:0x%x\n", (uint32_t)NVIC->ISPR);
	for (i = 0; i < ARRAY_SIZE(NVIC->ISPR); i += 4) {
		printf("[%p]:0x%08x, 0x%08x, 0x%08x, 0x%08x\n", &NVIC->ISPR[i],
		       NVIC->ISPR[i], NVIC->ISPR[i + 1],
		       NVIC->ISPR[i + 2], NVIC->ISPR[i + 3]);
	}
	printf("\n[NVIC Activing]:0x%x\n", (uint32_t)NVIC->IABR);
	for (i = 0; i < ARRAY_SIZE(NVIC->IABR); i += 4) {
		printf("[%p]:0x%08x, 0x%08x, 0x%08x, 0x%08x\n", &NVIC->IABR[i],
		       NVIC->IABR[i], NVIC->IABR[i + 1],
		       NVIC->IABR[i + 2], NVIC->IABR[i + 3]);
	}

#if 0
	if (exceptin_dump_len)
		print_hex_dump(NULL, 1, 32, 1, (const void *)exceptin_dump_addr, exceptin_dump_len, 1);
#endif

extern int cmd_free(int argc, char ** argv);
#ifdef CONFIG_COMMAND_FREE
	 cmd_free(0, NULL);
#endif

#ifdef CONFIG_COMPONENTS_BOOT_REASON
	app_write_boot_reason_when_panic();
#endif

	if (__panic_cli)
		__panic_cli();

	/* if happen fault, print important information and drop-dead halt */
	while (1);

	return 0;
}
