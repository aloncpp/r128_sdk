#include "hardware_wbp.h"
#include "gdb_stub.h"
#include <stdio.h>
#include <FreeRTOSConfig.h>
static int kgdb_arch_init_flag[configNR_CPUS] = {0};

int create_hw_break_watch(unsigned int hw_break, unsigned int hw_watch);

int arm_arch_set_hw_watchpoint(enum gdb_bptype type, int i, unsigned long addr)
{
    return arm_install_hw_watchpoint(type, i, addr);
}

int arm_arch_remove_hw_watchpoint(enum gdb_bptype type, int i, unsigned long addr)
{
    arm_uninstall_hw_watchpoint(i);
    return 0;
}

int arm_arch_set_hw_breakpoint(int i, unsigned long addr)
{
    return arm_install_hw_breakpoint(i, addr);
}

int arm_arch_remove_hw_breakpoint(int i, unsigned long addr)
{
    arm_uninstall_hw_breakpoint(i);
    return 0;
}

struct gdb_arch arch_gdb_ops =
{
    .set_hw_watchpoint = arm_arch_set_hw_watchpoint,
    .remove_hw_watchpoint = arm_arch_remove_hw_watchpoint,
    .set_hw_breakpoint = arm_arch_set_hw_breakpoint,
    .remove_hw_breakpoint = arm_arch_remove_hw_breakpoint,
};

int kgdb_arch_init(void)
{
    int processor_id = cur_cpu_id();
    int ret = -1;

    unsigned int brp = 0;
    unsigned int wrp = 0;

    if (kgdb_arch_init_flag[processor_id] > 0)
    {
        return 0;
    }

    if (!monitor_mode_enabled())
    {
        ret = enable_monitor_mode();
        if (ret)
        {
            printf("cpu%d: enter monitor mode failed!\n", processor_id);
            return -1;
        }
    }

    brp = get_num_brp_resources();
    wrp = get_num_wrp_resources();

    if (create_hw_break_watch(brp, wrp))
    {
        return -1;
    }

    kgdb_arch_init_flag[processor_id] = 1;
    return ret;
}
