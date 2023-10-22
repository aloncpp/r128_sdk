#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <port_misc.h>
#include <mmu_cache.h>
#include <hal_uart.h>

#ifdef CONFIG_DEBUG_BACKTRACE
#include <backtrace.h>
#endif

extern int check_freeze_flag(void);

#define UND_MODE 0x1
#define ABT_MODE 0x2
#define HYPE_MODE 0x3

typedef struct
{
    uint32_t fcsid;                     // 0x00
    uint32_t abt_iFAR;                  // 0x04
    uint32_t abt_dFAR;                  // 0x08
    uint32_t abt_iFSR;                  // 0x0c
    uint32_t abt_dFSR;                  // 0x10
    uint32_t abt_lr;                    // 0x14
    uint32_t ttbr0;                     // 0x18
    uint32_t ttbr1;                     // 0x1c
    uint32_t sctl;                      // 0x20
    uint32_t scr;                       // 0x24
    uint32_t orig_cpsr;                 // 0x28
}ctrl_regs_t;

typedef struct
{
    ctrl_regs_t control;                //0x00-0x24
    uint32_t lr;                        // 0x28
    uint32_t sp;                        // 0x2c
    uint32_t r0;                        // 0x30
    uint32_t r1;                        // 0x34
    uint32_t r2;                        // 0x38
    uint32_t r3;                        // 0x3c
    uint32_t r4;                        // 0x40
    uint32_t r5;                        // 0x44
    uint32_t r6;                        // 0x48
    uint32_t r7;                        // 0x4c
    uint32_t r8;                        // 0x50
    uint32_t r9;                        // 0x54
    uint32_t r10;                       // 0x58
    uint32_t r11;                       // 0x5c
    uint32_t r12;                       // 0x60
} excep_status_t;

#define CONSOLEBUF_SIZE 1024

static char log_buf[CONSOLEBUF_SIZE];

int printk(const char *fmt, ...)
{
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

#if !defined(CONFIG_DISABLE_ALL_UART_LOG)
    while(length--)
    {
        hal_uart_send(CONFIG_CLI_UART_PORT, &log_buf[i++], 1);
    }
#endif

    va_end(args);

    return length;
}

__attribute__((weak)) void dump_stack(void * frame)
{
    printk("not support dump stack!\r\n");
}

void freeze_other_core(void)
{
#ifdef CONFIG_FREEZE_CORE
    while(1) {
        int i;
        if (check_freeze_flag()) {
           break;
        }

        for(i = 0; i < configNR_CPUS; i++) {
            if (i != cur_cpu_id()) {
                send_ipi_interrupt(IPI_FREEZE, (1 << i), 0, 1);
            }
        }
    }
#endif
}

static void dump_memory(unsigned long *buf, uint32_t len)
{
    int i;

    printk("\r\ndump stack memory:");

    for (i = 0; i < len; i ++)
    {
        if ((i % 4) == 0)
        {
            printk("\r\n%p: ", buf + i);
        }
        if (!xport_is_valid_address(buf + i, buf + i))
            break;
        printk("0x%08lx ", buf[i]);
    }
    printk("\r\n\r");

    return;
}

void xport_V7a_Dump_Excep_Status(excep_status_t * regs, uint32_t err_mode)
{
    TaskHandle_t task = NULL;

    if (uGetInterruptNest() == 0)
    {
        task = xTaskGetCurrentTaskHandle();
        if (task)
        {
            printk("task: %s\r\n", pcTaskGetName(task));
        }
    }
    else
    {
        printk("crash in irq context!\r\n")    ;
    }

    if(regs)
    {
        uint32_t error_pc = 0;
        ctrl_regs_t *control;	

        control = &regs->control;
        if(err_mode == UND_MODE)
        {
	        error_pc = control->abt_lr - 4;
        }
        else if(err_mode == ABT_MODE)
        {
	        error_pc = control->abt_lr - 8;
        }

        printk("cpsr: 0x%08x.\r\n", control->orig_cpsr);

        printk("\r\ngprs:\r\n");
        printk("r00:0x%08x r01:0x%08x r02:0x%08x r03:0x%08x\r\n",
            regs->r0, regs->r1, regs->r2, regs->r3);
        printk("r04:0x%08x r05:0x%08x r06:0x%08x r07:0x%08x\r\n",
            regs->r4, regs->r5, regs->r6, regs->r7);
        printk("r08:0x%08x r09:0x%08x r10:0x%08x r11:0x%08x\r\n",
            regs->r8, regs->r9, regs->r10, regs->r11);
        printk("r12:0x%08x  sp:0x%08x  lr:0x%08x  pc:0x%08x\r\n",
            regs->r12, regs->sp, regs->lr, error_pc);

        printk("\r\ncp15:\r\n");
        printk("fst_fsid:0x%08x\r\n" \
           "abt_ifar:0x%08x\r\n" \
           "abt_dfar:0x%08x\r\n" \
           "abt_ifsr:0x%08x\r\n" \
           "abt_dfsr:0x%08x\r\n" \
           "abt_lr  :0x%08x\r\n" \
           "abt_sp  :0x%08x\r\n" \
           "sctl    :0x%08x\r\n" \
           "actl    :0x%08x\r\n" \
           "ttbr0   :0x%08x\r\n" \
           "ttbr1   :0x%08x\r\n",\
           control->fcsid, control->abt_iFAR, control->abt_dFAR, control->abt_iFSR, \
           control->abt_dFSR, control->abt_lr, (uint32_t)regs, control->sctl,\
           control->scr, control->ttbr0, control->ttbr1);

#ifdef CONFIG_DEBUG_BACKTRACE
        printk("\r\n----backtrace information----\r\n");
        backtrace_exception((print_function)printk, control->orig_cpsr, regs->sp, control->abt_lr, regs->lr);
        printk("-----------------------------\r\n");
#endif
        dump_memory((unsigned long*)regs->sp, 128);
    }

#ifdef CONFIG_PANIC_CLI
    panic_goto_cli();
#endif
}

void xPort_V7a_Und_Trap_Entry(excep_status_t *regs)
{
    freeze_other_core();

    printk("\r\n");
    printk("================================================================\r\n");
    printk("                  undefined instructions trap                   \r\n");
    printk("================================================================\r\n");

    xport_V7a_Dump_Excep_Status(regs, UND_MODE);
    while(1);
}

void xPort_V7a_Iabt_Trap_Entry(excep_status_t *regs)
{
    int8_t  i = 1;

    freeze_other_core();

    uint32_t victim = regs->control.abt_lr - 4;
    #define readmem(addr)     	(*(const volatile uint32_t *)addr)
    extern int cur_cpu_id(void);
 
    /*makesure in code range and check whether it was the 'bkpt' instructions.*/
    if ((readmem(victim) & 0xfff00070) == 0xe1200070)
    {   
        static uint32_t loop = 0;
 
        /* A proper intrval for print */                                                                                                                                                       
        if ((loop ++ & 0x007fffff) == 0)
        {
            printk("cpu%d: softbreak, connect the debugger.sp: 0x%p, lr: 0x%08x, abt_lr: 0x%08x...\r\n", \
                  cur_cpu_id(), regs, regs->lr, regs->control.abt_lr);

#ifdef CONFIG_DEBUG_BACKTRACE
            printk("\r\n----backtrace information----\r\n");
            backtrace_exception((print_function)printk, regs->control.orig_cpsr, regs->sp, regs->control.abt_lr, regs->lr);
            printk("-----------------------------\r\n");
#endif
            dump_memory((unsigned long*)regs->sp, 128);
        }
        regs->control.abt_lr = victim;
 
        /*resume to connect the DS5*/
        return;
    }
 
    printk("\r\n");
    printk("================================================================\r\n");
    printk("                      memory access abort(IF)                   \r\n");
    printk("================================================================\r\n");

    xport_V7a_Dump_Excep_Status(regs, ABT_MODE);
    while(1);
}

void xPort_V7a_Dabt_Trap_Entry(excep_status_t *regs)
{
    freeze_other_core();

    printk("\r\n");
    printk("================================================================\r\n");
    printk("                      memory access abort(MA)                   \r\n");
    printk("================================================================\r\n");

    xport_V7a_Dump_Excep_Status(regs, ABT_MODE);
    while(1);
}

void xPort_V7a_Hype_Trap_Entry (excep_status_t *regs)
{
    freeze_other_core();

    printk("\r\n");
    printk("================================================================\r\n");
    printk("                         Hype Trap abort                        \r\n");
    printk("================================================================\r\n");

    xport_V7a_Dump_Excep_Status(regs, HYPE_MODE);
    while(1);
}

void xPort_V7a_FIQ_Trap_Entry(excep_status_t *regs)
{
    freeze_other_core();

    printk("\r\n");
    printk("================================================================\r\n");
    printk("                              FIQ Error                         \r\n");
    printk("================================================================\r\n");

    while(1);
}

