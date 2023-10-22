// Copyright 2015-2019 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <xtensa_context.h>
#include <console.h>
#include <hal_uart.h>
#include <backtrace.h>

bool check_ptr_is_valid(const void *p);
uint32_t cpu_process_stack_pc(unsigned long pc);

#ifdef CONFIG_DEBUG_BACKTRACE_EXCCAUSE_STR
static char *exccause_str[] = {
	"Ill_Inst",
	"Syscall",
	"Inst_Fetch_Err",
	"Load_Store_Err",
	"Level1_Interrupt",
	"Alloca",
	"DivideByZero",
	"PC_Value_Err, CSR_Parity_Err",
	"Privileged_Err",
	"Load_Store_Align_Err",
	"External_Reg_Privilege_Err",
	"Exclusive_Err",
	"InstrPIFData_Err",
	"LoadStorePIFData_Err",
	"InstrPIFAddr_Err",
	"LoadStorePIFAddr_Err",
	"InstTLBMiss",
	"InstTLBMultHit",
	"InstFetchPrivilege",
	"RESERVED",
	"InstFetchProhibited",
	"RESERVED",
	"RESERVED",
	"RESERVED",
	"LoadStoreTLBMiss",
	"LoadStoreTLBMultHit",
	"LoadStorePrivilege",
	"RESERVED",
	"LoadProhibited",
	"StoreProhibited",
	"RESERVED",
	"RESERVED",
};
#endif

#ifdef CONFIG_LINUX_DEBUG
#include <components/aw/linux_debug/debug_common.h>

void pr_registers(const void *f, int core)
{
    int x, y;

    XtExcFrame *frame = (XtExcFrame *) f;
    int *regs = (int *)frame;

    const char *sdesc[] =
    {
        "PC      ", "PS      ", "A0      ", "A1      ", "A2      ", "A3      ", "A4      ", "A5      ",
        "A6      ", "A7      ", "A8      ", "A9      ", "A10     ", "A11     ", "A12     ", "A13     ",
        "A14     ", "A15     ", "SAR     ", "EXCCAUSE", "EXCVADDR", "LBEG    ", "LEND    ", "LCOUNT  "
    };

    log_save("Core %d register dump:\n",core);
    for (x = 0; x < 24; x += 4)
    {
        for (y = 0; y < 4; y++)
        {
            if (sdesc[x + y][0] != 0)
            {
                log_save("%s: 0x%x\n", sdesc[x + y], regs[x + y + 1]);
            }
        }
    }
    log_save("\r\n");
}


void show_exception_information_for_core(const void *f, int core)
{
    log_save("\r\n");
    log_save("-----------------------------------------\r\n");
    log_save("-----------------------------------------\r\n");
    pr_registers(f, core);
    log_save("-----xt-addr2line-----\r\n");
    arch_backtrace_exception(log_save, (void*)f);
    log_save("----------------------\r\n");
    while (1);
}

#else

#define CONSOLEBUF_SIZE 512
#if !defined(CONFIG_DISABLE_ALL_UART_LOG)
static char log_buf[CONSOLEBUF_SIZE];
#endif

void panic_print_char(const char c)
{
#if !defined(CONFIG_DISABLE_ALL_UART_LOG)
#ifndef BACKTRACE_SIM
    hal_uart_put_char(CONSOLE_UART, c);
#else
    /* for sim */
    putchar(c);
    fflush(stdout);
#endif
#endif
}

void panic_print_hex(int h)
{
    int x;
    int c;
    for (x = 0; x < 8; x++)
    {
        c = (h >> 28) & 0xf;
        if (c < 10)
        {
            panic_print_char('0' + c);
        }
        else
        {
            panic_print_char('a' + c - 10);
        }
        h <<= 4;
    }
}

void panic_print_dec(int d)
{
    int n1, n2;
    n1 = d % 10;
    n2 = d / 10;
    if (n2 == 0)
    {
        panic_print_char(' ');
    }
    else
    {
        panic_print_char(n2 + '0');
    }
    panic_print_char(n1 + '0');
}

void panic_print_str(const char *str)
{
    int i;
    for (i = 0; str[i] != 0; i++)
    {
        panic_print_char(str[i]);
    }
}

int printk(const char *fmt, ...)
{
#if !defined(CONFIG_DISABLE_ALL_UART_LOG)
    va_list args;
    size_t length;
    int i = 0;

    memset(&log_buf, 0, CONSOLEBUF_SIZE);

    va_start(args, fmt);

    length = vsnprintf(log_buf, sizeof(log_buf) - 1, fmt, args);
    if (length > CONSOLEBUF_SIZE - 1)
    {
        length = CONSOLEBUF_SIZE - 1;
    }

    while (length--)
    {
#ifndef BACKTRACE_SIM
        hal_uart_put_char(CONSOLE_UART, log_buf[i++]);
#else
        putchar(log_buf[i++]);
        fflush(stdout);
#endif
    }

    va_end(args);
#endif

    return 0;
}

void panic_print_registers(const void *f, int core, int isdebug)
{
    int x, y;

    XtExcFrame *frame = (XtExcFrame *) f;
    int *regs = (int *)frame;
    int cause = frame->exccause;

    const char *sdesc[] =
    {
        "PC      ", "PS      ", "A0      ", "A1      ", "A2      ", "A3      ", "A4      ", "A5      ",
        "A6      ", "A7      ", "A8      ", "A9      ", "A10     ", "A11     ", "A12     ", "A13     ",
        "A14     ", "A15     ", "SAR     ", "EXCCAUSE", "EXCVADDR", "LBEG    ", "LEND    ", "LCOUNT  "
    };

    panic_print_str("-----------------------------------------\r\n");
    if (!isdebug) {
#ifdef CONFIG_DEBUG_BACKTRACE_EXCCAUSE_STR
        if (cause < sizeof(exccause_str)/sizeof(exccause_str[0])) {
            panic_print_str("            ");
            panic_print_str(exccause_str[cause]);
            panic_print_str("            \r\n");
        } else {
            panic_print_str("            RESERVED            \r\n");
        }
#endif
    } else {
        panic_print_str("            Debug            \r\n");
    }
    panic_print_str("-----------------------------------------\r\n");

    panic_print_str("Core ");
    panic_print_dec(core);
    panic_print_str(" register dump:");

    for (x = 0; x < 24; x += 4)
    {
        panic_print_str("\r\n");
        for (y = 0; y < 4; y++)
        {
            if (sdesc[x + y][0] != 0)
            {
                panic_print_str(sdesc[x + y]);
                panic_print_str(": 0x");
                panic_print_hex(regs[x + y + 1]);
                panic_print_str("  ");
            }
        }
    }
    panic_print_str("\r\n");
}

static void show_stack_data(int *buf, int len)
{
    int i;
    /*  align down to 4 bytes */
    buf = (int *)((unsigned int)(buf) & ~(4 - 1));

    if ((check_ptr_is_valid(buf) != 1) ||
            (check_ptr_is_valid(buf + len) != 1)) {
        panic_print_str("\r\n\r\n");
        return;
    }

    for (i = 0; i < len; i++)
    {
        if (i % 4 == 0) {
            panic_print_str("\r\n0x");
            panic_print_hex((unsigned int)(buf + i));
            panic_print_str(": ");
        }
        panic_print_str("0x");
        panic_print_hex(buf[i]);
        panic_print_str("  ");
    }
}

static void _show_exception_information_for_core(const void *f, int core, int isdebug)
{
    unsigned int EPC = 0;
    unsigned int DEPC = 0;
    unsigned int debugcause = 0;
    unsigned int cpenable = 0;
    unsigned int EPC_level[5] = {0};
    unsigned int debug_data_reg = 0;

    XtExcFrame *frame = (XtExcFrame *) f;
    int *sp = (int *)(frame->a1);

    panic_print_str("\r\n");
    panic_print_registers(f, core, isdebug);
    panic_print_str("\r\n");

    __asm__ volatile("rsr %0, 192\n" : "=r"(DEPC));
    __asm__ volatile("rsr %0, 177\n" : "=r"(EPC));
    __asm__ volatile("rsr %0, CPENABLE\n" : "=r"(cpenable));
    __asm__ volatile("rsr %0, DEBUGCAUSE\n" : "=r"(debugcause));

    __asm__ volatile("rsr %0, 177\n" : "=r"(EPC_level[0]));
    __asm__ volatile("rsr %0, 178\n" : "=r"(EPC_level[1]));
    __asm__ volatile("rsr %0, 179\n" : "=r"(EPC_level[2]));
    __asm__ volatile("rsr %0, 180\n" : "=r"(EPC_level[3]));
    __asm__ volatile("rsr %0, 181\n" : "=r"(EPC_level[4]));
    __asm__ volatile("rsr %0, 104\n" : "=r"(debug_data_reg));

    panic_print_str("Other Core Register:\r\n");
    panic_print_str("EPC     : 0x");
    panic_print_hex(EPC);
    panic_print_str("  DEPC    : 0x");
    panic_print_hex(DEPC);
    panic_print_str("  CPENABLE: 0x");
    panic_print_hex(cpenable);
    panic_print_str("  DEBUGCAU: 0x");
    panic_print_hex(debugcause);
    panic_print_str("\r\n");

    panic_print_str("EPC1    : 0x");
    panic_print_hex(EPC_level[0]);
    panic_print_str("  EPC2    : 0x");
    panic_print_hex(EPC_level[1]);
    panic_print_str("  EPC3    : 0x");
    panic_print_hex(EPC_level[2]);
    panic_print_str("  EPC4    : 0x");
    panic_print_hex(EPC_level[3]);
    panic_print_str("\r\n");

    panic_print_str("EPC5    : 0x");
    panic_print_hex(EPC_level[4]);
    panic_print_str("  DDR     : 0x");
    panic_print_hex(debug_data_reg);
    panic_print_str("\r\n");
    panic_print_str("\r\n");

    panic_print_str("------------\r\n");
    arch_backtrace_exception(printk, (void*)f);
    panic_print_str("------------\r\n");

    panic_print_str("-------------------------------------\r\n");
    panic_print_str("\r\nStack Data:");
    show_stack_data(sp - 128, 512);
    panic_print_str("\r\n");

    panic_print_str("\r\nA[0] Data:");
    show_stack_data((int *)cpu_process_stack_pc(frame->a0) - 16, 32);
    panic_print_str("\r\n");

    panic_print_str("\r\nA[2] Data:");
    show_stack_data((int *)cpu_process_stack_pc(frame->a2) - 16, 32);
    panic_print_str("\r\n");

    panic_print_str("\r\nA[3] Data:");
    show_stack_data((int *)cpu_process_stack_pc(frame->a3) - 16, 32);
    panic_print_str("\r\n");

    panic_print_str("\r\nA[4] Data:");
    show_stack_data((int *)cpu_process_stack_pc(frame->a4) - 16, 32);
    panic_print_str("\r\n");

    panic_print_str("\r\nA[5] Data:");
    show_stack_data((int *)cpu_process_stack_pc(frame->a5) - 16, 32);
    panic_print_str("\r\n");

    panic_print_str("\r\nA[6] Data:");
    show_stack_data((int *)cpu_process_stack_pc(frame->a6) - 16, 32);
    panic_print_str("\r\n");

    panic_print_str("\r\nA[7] Data:");
    show_stack_data((int *)cpu_process_stack_pc(frame->a7) - 16, 32);
    panic_print_str("\r\n");

    panic_print_str("\r\nA[8] Data:");
    show_stack_data((int *)cpu_process_stack_pc(frame->a8) - 16, 32);
    panic_print_str("\r\n");

    panic_print_str("\r\nA[9] Data:");
    show_stack_data((int *)cpu_process_stack_pc(frame->a9) - 16, 32);
    panic_print_str("\r\n");

    panic_print_str("\r\nA[10] Data:");
    show_stack_data((int *)cpu_process_stack_pc(frame->a10) - 16, 32);
    panic_print_str("\r\n");

    panic_print_str("\r\nA[11] Data:");
    show_stack_data((int *)cpu_process_stack_pc(frame->a11) - 16, 32);
    panic_print_str("\r\n");

    panic_print_str("\r\nA[12] Data:");
    show_stack_data((int *)cpu_process_stack_pc(frame->a12) - 16, 32);
    panic_print_str("\r\n");

    panic_print_str("\r\nA[13] Data:");
    show_stack_data((int *)cpu_process_stack_pc(frame->a13) - 16, 32);
    panic_print_str("\r\n");

    panic_print_str("\r\nA[14] Data:");
    show_stack_data((int *)cpu_process_stack_pc(frame->a14) - 16, 32);
    panic_print_str("\r\n");

    panic_print_str("\r\nA[15] Data:");
    show_stack_data((int *)cpu_process_stack_pc(frame->a15) - 16, 32);
    panic_print_str("\r\n");

    panic_print_str("\r\nPC Data:");
    show_stack_data((int *)cpu_process_stack_pc(frame->pc) - 16, 32);
    panic_print_str("\r\n");

    panic_print_str("-------------------------------------\r\n");
    while (1);
}

void show_exception_information_for_core(const void *f, int core)
{
    _show_exception_information_for_core(f, core, 0);
}

void aw_debug_panic(const void *f, int core)
{
    (void) core;
    _show_exception_information_for_core(f, 0, 0);
}

#endif

