#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <irqflags.h>
#include <excep.h>
#include <csr.h>

#ifdef CONFIG_DEBUG_BACKTRACE
#include <backtrace.h>
#endif

#ifdef CONFIG_COMPONENTS_BOOT_REASON
#include <boot_reason.h>
#endif

#ifdef CONFIG_COMPONENTS_PSTORE
extern int pstore_printf(const char *fmt, ...);
extern int pstore_flush(void);
#define panic_printf(fmt, ...)    do { \
	printf(fmt, ##__VA_ARGS__); \
	pstore_printf(fmt, ##__VA_ARGS__); \
} while(0)
#else
#define panic_printf(fmt, ...)    do { \
	printf(fmt, ##__VA_ARGS__); \
} while(0)
#endif

void dump_register_memory(char *name, unsigned long addr, int len);

#if 0
void awos_arch_save_fpu_status(fpu_context_t *);
void awos_arch_restore_fpu_status(fpu_context_t *);
void panic_goto_cli(void);
#endif

typedef unsigned long (* exception_callback)(
    char *str,
    unsigned long mcause,
    unsigned long mepc,
    unsigned long mtval,
    irq_regs_t *regs);

typedef struct _exception_handle_table_entry
{
    int cause;
    exception_callback callback;
    char *error_str;
} exception_handle_table_entry;

exception_handle_table_entry exception_table[] =
{
    {
        .cause = EXC_INST_MISALIGNED,
        .error_str = "EXC_INST_MISALIGNED",
    },
    {
        .cause = EXC_INST_ACCESS,
        .error_str = "EXC_INST_ACCESS",
    },
    {
        .cause = EXC_INST_ILLEGAL,
        .error_str = "EXC_INST_ILLEGAL",
    },
    {
        .cause = EXC_BREAKPOINT,
        .error_str = "EXC_BREAKPOINT",
    },
    {
        .cause = EXC_LOAD_MISALIGN,
        .error_str = "EXC_LOAD_MISALIGN",
    },
    {
        .cause = EXC_LOAD_ACCESS,
        .error_str = "EXC_LOAD_ACCESS",
    },
    {
        .cause = EXC_STORE_MISALIGN,
        .error_str = "EXC_STORE_MISALIGN",
    },
    {
        .cause = EXC_STORE_ACCESS,
        .error_str = "EXC_STORE_ACCESS",
    },
    {
        .cause = EXC_INST_PAGE_FAULT,
        .error_str = "EXC_INST_PAGE_FAULT",
    },
    {
        .cause = EXC_LOAD_PAGE_FAULT,
        .error_str = "EXC_LOAD_PAGE_FAULT",
    },
    {
        .cause = EXC_STORE_PAGE_FAULT,
        .error_str = "EXC_STORE_PAGE_FAULT",
    },
    {
        .cause = EXC_SYSCALL_FRM_U,
        .error_str = "EXC_SYSCALL_FRM_U",
    },
    {
        .cause = EXC_SYSCALL_FRM_M,
        .error_str = "EXC_SYSCALL_FRM_M",
    },
};

#if 0
static int check_fpu_status_clean(void)
{
    unsigned long mstatus;

    mstatus = arch_local_save_flags();
    if ((mstatus & SR_FS) != SR_FS_CLEAN)
    {
        return 0;
    }
    return 1;
}

void fpu_save_inirq(unsigned long mstatus)
{
    rt_thread_t self = rt_thread_self();

    if (!melis_kernel_running)
    {
        return;
    }

    if (self == RT_NULL)
    {
        software_break();
    }

    if ((mstatus & SR_FS) == SR_FS_DIRTY)
    {
        awos_arch_save_fpu_status(&self->fdext_ctx);
    }

    if (!check_fpu_status_clean())
    {
        software_break();
    }
}

void fpu_restore_inirq(unsigned long mstatus)
{
    rt_thread_t self = rt_thread_self();

    if (!melis_kernel_running)
    {
        return;
    }

    if (self == RT_NULL)
    {
        software_break();
    }

    // interrupt handler vector operations is integrity.
    // and has finished, so need not save its context.
    if ((mstatus & SR_FS) == SR_FS_DIRTY)
    {
        awos_arch_restore_fpu_status(&self->fdext_ctx);
    }

    if (!check_fpu_status_clean())
    {
        software_break();
    }
}
#endif

static void show_register(irq_regs_t *regs, unsigned long mcause, unsigned long mepc, unsigned long mtval)
{
#if CONFIG_BITS_PER_LONG == 64
    panic_printf("\ngprs:\n");
    panic_printf(" x0:0x%08lx%08lx   ra:0x%08lx%08lx   sp:0x%08lx%08lx   gp:0x%08lx%08lx\n",
           (unsigned long)0, (unsigned long)0, regs->x1 >> 32, regs->x1 & 0xfffffffful, regs->x2 >> 32, regs->x2 & 0xfffffffful, regs->x3 >> 32, regs->x3 & 0xfffffffful);
    panic_printf(" tp:0x%08lx%08lx   t0:0x%08lx%08lx   t1:0x%08lx%08lx   t2:0x%08lx%08lx\n",
           regs->x4 >> 32, regs->x4 & 0xfffffffful, regs->x5 >> 32, regs->x5 & 0xfffffffful, regs->x6 >> 32, regs->x6 & 0xfffffffful, regs->x7 >> 32, regs->x7 & 0xfffffffful);
    panic_printf(" s0:0x%08lx%08lx   s1:0x%08lx%08lx   a0:0x%08lx%08lx   a1:0x%08lx%08lx\n",
           regs->x8 >> 32, regs->x8 & 0xfffffffful, regs->x9 >> 32, regs->x9 & 0xfffffffful, regs->x10 >> 32, regs->x10 & 0xfffffffful, regs->x11 >> 32, regs->x11 & 0xfffffffful);
    panic_printf(" a2:0x%08lx%08lx   a3:0x%08lx%08lx   a4:0x%08lx%08lx   a5:0x%08lx%08lx\n",
           regs->x12 >> 32, regs->x12 & 0xfffffffful, regs->x13 >> 32, regs->x13 & 0xfffffffful, regs->x14 >> 32, regs->x14 & 0xfffffffful, regs->x15 >> 32, regs->x15 & 0xfffffffful);
    panic_printf(" a6:0x%08lx%08lx   a7:0x%08lx%08lx   s2:0x%08lx%08lx   s3:0x%08lx%08lx\n",
           regs->x16 >> 32, regs->x16 & 0xfffffffful, regs->x17 >> 32, regs->x17 & 0xfffffffful, regs->x18 >> 32, regs->x18 & 0xfffffffful, regs->x19 >> 32, regs->x19 & 0xfffffffful);
    panic_printf(" s5:0x%08lx%08lx   s5:0x%08lx%08lx   s6:0x%08lx%08lx   s7:0x%08lx%08lx\n",
           regs->x20 >> 32, regs->x20 & 0xfffffffful, regs->x21 >> 32, regs->x21 & 0xfffffffful, regs->x22 >> 32, regs->x22 & 0xfffffffful, regs->x23 >> 32, regs->x23 & 0xfffffffful);
    panic_printf(" s8:0x%08lx%08lx   s9:0x%08lx%08lx  s10:0x%08lx%08lx  s11:0x%08lx%08lx\n",
           regs->x24 >> 32, regs->x24 & 0xfffffffful, regs->x25 >> 32, regs->x25 & 0xfffffffful, regs->x26 >> 32, regs->x26 & 0xfffffffful, regs->x27 >> 32, regs->x27 & 0xfffffffful);
    panic_printf(" t3:0x%08lx%08lx   t4:0x%08lx%08lx   t5:0x%08lx%08lx   t6:0x%08lx%08lx\n",
           regs->x28 >> 32, regs->x28 & 0xfffffffful, regs->x29 >> 32, regs->x29 & 0xfffffffful, regs->x30 >> 32, regs->x30 & 0xfffffffful, regs->x31 >> 32, regs->x31 & 0xfffffffful);

    panic_printf("\nother:\n");
    panic_printf("mepc    :0x%08lx%08lx\n" \
           "mcause  :0x%08lx%08lx\n" \
           "mtval   :0x%08lx%08lx\n" \
           "mstatus :0x%08lx%08lx\n" \
           "mscratch:0x%08lx%08lx\n" \
           , mepc >> 32, mepc & 0xfffffffful, mcause >> 32, mcause & 0xfffffffful,
           mtval >> 32, mtval & 0xfffffffful, regs->mstatus >> 32, regs->mstatus & 0xfffffffful,
           regs->mscratch >> 32, regs->mscratch & 0xfffffffful);
#else
    panic_printf("\ngprs:\n");
    panic_printf(" x0:0x%08x   ra:0x%08x   sp:0x%08x   gp:0x%08x\n", 0        , regs->x1 , regs->x2 , regs->x3 );
    panic_printf(" tp:0x%08x   t0:0x%08x   t1:0x%08x   t2:0x%08x\n", regs->x4 , regs->x5 , regs->x6 , regs->x7 );
    panic_printf(" s0:0x%08x   s1:0x%08x   a0:0x%08x   a1:0x%08x\n", regs->x8 , regs->x9 , regs->x10, regs->x11);
    panic_printf(" a2:0x%08x   a3:0x%08x   a4:0x%08x   a5:0x%08x\n", regs->x12, regs->x13, regs->x14, regs->x15);
    panic_printf(" a6:0x%08x   a7:0x%08x   s2:0x%08x   s3:0x%08x\n", regs->x16, regs->x17, regs->x18, regs->x19);
    panic_printf(" s5:0x%08x   s5:0x%08x   s6:0x%08x   s7:0x%08x\n", regs->x20, regs->x21, regs->x22, regs->x23);
    panic_printf(" s8:0x%08x   s9:0x%08x  s10:0x%08x  s11:0x%08x\n", regs->x24, regs->x25, regs->x26, regs->x27);
    panic_printf(" t3:0x%08x   t4:0x%08x   t5:0x%08x   t6:0x%08x\n", regs->x28, regs->x29, regs->x30, regs->x31);

    panic_printf("\nother:\n");
    panic_printf("mepc    :0x%08lx\n" \
           "mcause  :0x%08lx\n" \
           "mtval   :0x%08lx\n" \
           "mstatus :0x%08lx\n" \
           "mscratch:0x%08lx\n" \
           , mepc, mcause, mtval, regs->mstatus, regs->mscratch);
#endif
}

static unsigned long default_handle(char *str, unsigned long mcause, unsigned long mepc, unsigned long mtval, irq_regs_t *regs)
{
    panic_printf("=====================================================================================================\n");
    panic_printf("                                         %s              \n", str);
    panic_printf("=====================================================================================================\n");
    show_register(regs, mcause, mepc, mtval);

    panic_printf("\r\n");

#ifdef CONFIG_DEBUG_BACKTRACE
    panic_printf("-------backtrace-----------\n");
    backtrace_exception(printf, regs->mstatus, regs->x2, mepc, regs->x1);
#ifdef CONFIG_COMPONENTS_PSTORE
    backtrace_exception(pstore_printf, regs->mstatus, regs->x2, mepc, regs->x1);
#endif
    panic_printf("---------------------------\n");
#endif

#ifdef CONFIG_COMPONENTS_PSTORE
    pstore_flush();
#endif

#ifdef CONFIG_COMMAND_FREE
    int cmd_free(int argc, char ** argv);
    cmd_free(0, NULL);
#endif

    dump_register_memory("stack", regs->x2 - 128, 512);
    dump_register_memory("mepc", regs->mepc - 128, 512);
    dump_register_memory("x1", regs->x1 - 16, 32);
    dump_register_memory("x3", regs->x3 - 16, 32);
    dump_register_memory("x4", regs->x4 - 16, 32);
    dump_register_memory("x5", regs->x5 - 16, 32);
    dump_register_memory("x6", regs->x6 - 16, 32);
    dump_register_memory("x7", regs->x7 - 16, 32);
    dump_register_memory("x8", regs->x8 - 16, 32);
    dump_register_memory("x9", regs->x9 - 16, 32);
    dump_register_memory("x10", regs->x10 - 16, 32);
    dump_register_memory("x11", regs->x11 - 16, 32);
    dump_register_memory("x12", regs->x12 - 16, 32);
    dump_register_memory("x13", regs->x13 - 16, 32);
    dump_register_memory("x14", regs->x14 - 16, 32);
    dump_register_memory("x15", regs->x15 - 16, 32);
    dump_register_memory("x16", regs->x16 - 16, 32);
    dump_register_memory("x17", regs->x17 - 16, 32);
    dump_register_memory("x18", regs->x18 - 16, 32);
    dump_register_memory("x19", regs->x19 - 16, 32);
    dump_register_memory("x20", regs->x20 - 16, 32);
    dump_register_memory("x21", regs->x21 - 16, 32);
    dump_register_memory("x22", regs->x22 - 16, 32);
    dump_register_memory("x23", regs->x23 - 16, 32);
    dump_register_memory("x24", regs->x24 - 16, 32);
    dump_register_memory("x25", regs->x25 - 16, 32);
    dump_register_memory("x26", regs->x26 - 16, 32);
    dump_register_memory("x27", regs->x27 - 16, 32);
    dump_register_memory("x28", regs->x28 - 16, 32);
    dump_register_memory("x29", regs->x29 - 16, 32);
    dump_register_memory("x30", regs->x30 - 16, 32);
    dump_register_memory("x31", regs->x31 - 16, 32);

#ifdef CONFIG_COMPONENTS_BOOT_REASON
    app_write_boot_reason_when_panic();
#endif

#ifdef CONFIG_PANIC_CLI
    extern void panic_goto_cli(void);
    panic_goto_cli();
#endif

    while (1);
    return 0;
}

static unsigned long error_handle(unsigned long mcause, unsigned long mepc, unsigned long mtval, irq_regs_t *regs)
{
    int i;
    int cause = mcause & MCAUSE_IRQ_MASK;

    for (i = 0; i < sizeof(exception_table) / sizeof(exception_table[0]); i++)
    {
        if (exception_table[i].cause == cause && exception_table[i].error_str)
        {
            if (exception_table[i].callback)
            {
#ifdef CONFIG_COMPONENTS_BOOT_REASON
                unsigned long ret = 0;
                app_write_boot_reason_when_panic();
                ret = exception_table[i].callback(exception_table[i].error_str, mcause, mepc, mtval, regs);
                app_clear_boot_reason();
                return ret;
#else
                return exception_table[i].callback(exception_table[i].error_str, mcause, mepc, mtval, regs);
#endif
            }
            else
            {
                return default_handle(exception_table[i].error_str, mcause, mepc, mtval, regs);
            }
        }
    }
    return default_handle("UNKNOWN ERROR", mcause, mepc, mtval, regs);
}

unsigned long riscv_cpu_handle_exception(unsigned long mcause, unsigned long mepc, unsigned long mtval, irq_regs_t *regs)
{
    unsigned long ret = 0;

    if (mcause & SCAUSE_IRQ_FLAG)
    {
        return default_handle("UNKNOWN ERROR", mcause, mepc, mtval, regs);
    }

    switch (mcause & MCAUSE_IRQ_MASK)
    {
        case EXC_INST_MISALIGNED:
        case EXC_INST_ACCESS:
        case EXC_INST_ILLEGAL:
        case EXC_BREAKPOINT:
        case EXC_LOAD_MISALIGN:
        case EXC_LOAD_ACCESS:
        case EXC_STORE_MISALIGN:
        case EXC_STORE_ACCESS:
        case EXC_SYSCALL_FRM_U:
        case EXC_SYSCALL_FRM_M:
        case EXC_INST_PAGE_FAULT:
        case EXC_LOAD_PAGE_FAULT:
        case EXC_STORE_PAGE_FAULT:
        case EXC_SYSCALL_FRM_S:
            return error_handle(mcause, mepc, mtval, regs);
        default:
            return default_handle("UNKNOWN ERROR", mcause, mepc, mtval, regs);
    }

    return 0;
}

void trap_c(unsigned long *regs)
{
    panic_printf("\n");
    panic_printf("mepc   : %08lx\n", regs[31]);
    panic_printf("mstatus: %08lx\n", regs[32]);
    while (1);
}

