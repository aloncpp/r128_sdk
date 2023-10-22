#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gdb_stub.h"
#include <spinlock.h>

#include <barrier.h>

extern int printf(const char *fmt, ...);

volatile struct gdb_cmd_arg *gdb_cmd = NULL;

static int gdb_hw_break_max[configNR_CPUS];
static int gdb_hw_watch_max[configNR_CPUS];

static struct gdb_bkpt *gdb_hw_break[configNR_CPUS] = {NULL};
static struct gdb_bkpt *gdb_hw_watch[configNR_CPUS] = {NULL};

static int init_flag[configNR_CPUS] = {0};

extern int kgdb_arch_init(void);

struct bp_type_state_match
{
    int type;
    char *str;
};

static struct bp_type_state_match bp_type_match [] =
{
    {BP_BREAKPOINT, "breakpoint"},
    {BP_HARDWARE_BREAKPOINT, "hardware breakpoint"},
    {BP_WRITE_WATCHPOINT, "write watchpoint"},
    {BP_READ_WATCHPOINT, "read watchpoint"},
    {BP_ACCESS_WATCHPOINT, "access watchpoint"},
    {BP_POKE_BREAKPOINT, "poke breakpoint"},
};

static struct bp_type_state_match bp_state_match[] =
{
    {BP_UNDEFINED, "undefined"},
    {BP_REMOVED, "removed"},
    {BP_SET, "set"},
    {BP_ACTIVE, "active"},
};

__attribute__((weak)) int cur_cpu_id(void)
{
    return 0;
}

int create_hw_break_watch(unsigned int hw_break, unsigned int hw_watch)
{
    int i;
    int processor_id = cur_cpu_id();
    if (hw_watch)
    {
        gdb_hw_watch[processor_id] = calloc(hw_watch, sizeof(struct gdb_bkpt));
        if (gdb_hw_watch[processor_id])
        {
            gdb_hw_watch_max[processor_id] = hw_watch;
            struct gdb_bkpt *bkpt = gdb_hw_watch[processor_id];
            for (i = 0; i < hw_watch; i++)
            {
                bkpt->state = BP_UNDEFINED;
                bkpt++;
            }
        }
    }

    if (hw_break)
    {
        gdb_hw_break[processor_id] = calloc(hw_break, sizeof(struct gdb_bkpt));
        if (gdb_hw_break[processor_id])
        {
            gdb_hw_break_max[processor_id] = hw_break;
            struct gdb_bkpt *bkpt = gdb_hw_break[processor_id];
            for (i = 0; i < hw_break; i++)
            {
                bkpt->state = BP_UNDEFINED;
                bkpt++;
            }
        }
    }

    return 0;
}

int destory_hw_break_watch(void)
{
    int processor_id = cur_cpu_id();
    if (gdb_hw_watch[processor_id])
    {
        free(gdb_hw_watch[processor_id]);
        gdb_hw_watch[processor_id] = NULL;
    }

    if (gdb_hw_break[processor_id])
    {
        free(gdb_hw_break[processor_id]);
        gdb_hw_break[processor_id] = NULL;
    }

    return 0;
}

static int gdb_arch_set_hw_watchpoint(enum gdb_bptype type, unsigned int no, unsigned long addr)
{
    if (arch_gdb_ops.set_hw_watchpoint)
    {
        return arch_gdb_ops.set_hw_watchpoint(type, no, addr);
    }
    return -1;
}

static int gdb_arch_remove_hw_watchpoint(enum gdb_bptype type, unsigned int no, unsigned long addr)
{
    if (arch_gdb_ops.remove_hw_watchpoint)
    {
        return arch_gdb_ops.remove_hw_watchpoint(type, no, addr);
    }
    return -1;
}

static int gdb_arch_set_hw_breakpoint(unsigned int no, unsigned long addr)
{
    if (arch_gdb_ops.set_hw_breakpoint)
    {
        return arch_gdb_ops.set_hw_breakpoint(no, addr);
    }
    return -1;
}

static int gdb_arch_remove_hw_breakpoint(unsigned int no, unsigned long addr)
{
    if (arch_gdb_ops.remove_hw_breakpoint)
    {
        return arch_gdb_ops.remove_hw_breakpoint(no, addr);
    }
    return -1;
}

/*
 * HW watchpoint management:
 */
int gdb_set_hw_watch(unsigned long addr, enum gdb_bptype type)
{
    int processor_id = cur_cpu_id();
    int i;
    int ret = -1;
    int breakno = -1;

    if (init_flag[processor_id] == 0)
    {
        if (debug_watchpoint_init())
        {
            return -1;
        }
        init_flag[processor_id] = 1;
    }

    for (i = 0; i < gdb_hw_watch_max[processor_id]; i++)
    {
        if ((gdb_hw_watch[processor_id][i].state == BP_ACTIVE) &&
            (gdb_hw_watch[processor_id][i].bpt_addr == addr))
        {
            return 0;
        }
    }
    for (i = 0; i < gdb_hw_watch_max[processor_id]; i++)
    {
        if (gdb_hw_watch[processor_id][i].state == BP_REMOVED)
        {
            breakno = i;
            break;
        }
    }

    if (breakno == -1)
    {
        for (i = 0; i < gdb_hw_watch_max[processor_id]; i++)
        {
            if (gdb_hw_watch[processor_id][i].state == BP_UNDEFINED)
            {
                breakno = i;
                break;
            }
        }
    }

    if (breakno == -1)
    {
        return -1;
    }

    ret = gdb_arch_set_hw_watchpoint(type, breakno, addr);
    if (ret)
    {
        printf("set hw watchpoint 0x%lx failed!\n", addr);
        return -1;
    }
    gdb_hw_watch[processor_id][breakno].state = BP_ACTIVE;
    gdb_hw_watch[processor_id][breakno].type = type;
    gdb_hw_watch[processor_id][breakno].bpt_addr = addr;

    return ret;
}

int gdb_remove_hw_watch(unsigned long addr)
{
    int processor_id = cur_cpu_id();
    int i;

    for (i = 0; i < gdb_hw_watch_max[processor_id]; i++)
    {
        if ((gdb_hw_watch[processor_id][i].state == BP_ACTIVE) &&
            (gdb_hw_watch[processor_id][i].bpt_addr == addr))
        {
            if (gdb_arch_remove_hw_watchpoint(gdb_hw_watch[processor_id][i].type, i, addr))
            {
                printf("remove hw watchpoint 0x%lx failed!\n", addr);
                return -1;
            }
            gdb_hw_watch[processor_id][i].state = BP_REMOVED;
            return 0;
        }
    }
    return -1;
}

int gdb_isremoved_hw_watch(unsigned long addr)
{
    int processor_id = cur_cpu_id();
    int i;

    for (i = 0; i < gdb_hw_watch_max[processor_id]; i++)
    {
        if ((gdb_hw_watch[processor_id][i].state == BP_REMOVED) &&
            (gdb_hw_watch[processor_id][i].bpt_addr == addr))
        {
            return 1;
        }
    }
    return 0;
}

/*
 * HW breakpoint management:
 */
int gdb_set_hw_break(unsigned long addr)
{
    int processor_id = cur_cpu_id();
    int i;
    int breakno = -1;
    int ret = -1;

    if (init_flag[processor_id] == 0)
    {
        if (debug_watchpoint_init())
        {
            return -1;
        }
        init_flag[processor_id] = 1;
    }

    for (i = 0; i < gdb_hw_break_max[processor_id]; i++)
    {
        if ((gdb_hw_break[processor_id][i].state == BP_SET) &&
            (gdb_hw_break[processor_id][i].bpt_addr == addr))
        {
            return 0;
        }
    }
    for (i = 0; i < gdb_hw_break_max[processor_id]; i++)
    {
        if (gdb_hw_break[processor_id][i].state == BP_REMOVED)
        {
            breakno = i;
            break;
        }
    }

    if (breakno == -1)
    {
        for (i = 0; i < gdb_hw_break_max[processor_id]; i++)
        {
            if (gdb_hw_break[processor_id][i].state == BP_UNDEFINED)
            {
                breakno = i;
                break;
            }
        }
    }

    if (breakno == -1)
    {
        return -1;
    }

    ret = gdb_arch_set_hw_breakpoint(breakno, addr);
    if (ret)
    {
        printf("set hw breakpoint 0x%lx failed!\n", addr);
        return -1;
    }
    gdb_hw_break[processor_id][breakno].state = BP_SET;
    gdb_hw_break[processor_id][breakno].type = BP_HARDWARE_BREAKPOINT;
    gdb_hw_break[processor_id][breakno].bpt_addr = addr;

    return ret;
}

int gdb_remove_hw_break(unsigned long addr)
{
    int processor_id = cur_cpu_id();
    int i;

    for (i = 0; i < gdb_hw_break_max[processor_id]; i++)
    {
        if ((gdb_hw_break[processor_id][i].state == BP_SET) &&
            (gdb_hw_break[processor_id][i].bpt_addr == addr))
        {
            if (gdb_arch_remove_hw_breakpoint(i, addr))
            {
                printf("remove hw breakpoint 0x%lx failed!\n", addr);
                return -1;
            }
            gdb_hw_break[processor_id][i].state = BP_REMOVED;
            return 0;
        }
    }
    return -1;
}

int gdb_isremoved_hw_break(unsigned long addr)
{
    int processor_id = cur_cpu_id();
    int i;

    for (i = 0; i < gdb_hw_break_max[processor_id]; i++)
    {
        if ((gdb_hw_break[processor_id][i].state == BP_REMOVED) &&
            (gdb_hw_break[processor_id][i].bpt_addr == addr))
        {
            return 1;
        }
    }
    return 0;
}

int remove_all_break_watch_points(void)
{
    int processor_id = cur_cpu_id();
    unsigned long addr;
    int error = 0;
    int i;

    for (i = 0; i < gdb_hw_watch_max[processor_id]; i++)
    {
        if (gdb_hw_watch[processor_id][i].state != BP_ACTIVE)
        {
            goto hw_watch_setundefined;
        }
        addr = gdb_hw_watch[processor_id][i].bpt_addr;
        error = gdb_arch_remove_hw_watchpoint(gdb_hw_watch[processor_id][i].type, i, addr);
        if (error)
        {
            printf("GDB: breakpoint remove failed: %lx\n", addr);
        }
hw_watch_setundefined:
        gdb_hw_watch[processor_id][i].state = BP_UNDEFINED;
    }

    for (i = 0; i < gdb_hw_break_max[processor_id]; i++)
    {
        if (gdb_hw_break[processor_id][i].state != BP_ACTIVE)
        {
            goto hw_break_setundefined;
        }
        addr = gdb_hw_break[processor_id][i].bpt_addr;
        error = gdb_arch_remove_hw_breakpoint(i, addr);
        if (error)
        {
            printf("GDB: breakpoint remove failed: %lx\n", addr);
        }
hw_break_setundefined:
        gdb_hw_break[processor_id][i].state = BP_UNDEFINED;
    }

    return 0;
}

static char *get_bp_type_str(enum gdb_bptype type)
{
    if (type < sizeof(bp_type_match) / sizeof(bp_type_match[0]))
    {
        return bp_type_match[type].str;
    }
    return NULL;
}

static char *get_bp_state_str(enum gdb_bpstate state)
{
    if (state < sizeof(bp_state_match) / sizeof(bp_state_match[0]))
    {
        return bp_state_match[state].str;
    }
    return NULL;
}

void debug_dump_all_breaks_info(void)
{
    int i, cpui;
    for (cpui = 0; cpui < configNR_CPUS; cpui++)
    {
        printf("cpu%d:\n", cpui);
        if (init_flag[cpui] == 0)
        {
            printf("watchpoint debug not init!\n");
            return;
        }

        printf("watchpoint num = %d:\n", gdb_hw_watch_max[cpui]);
        printf("Id    Addr    State      Type\n");

        for (i = 0; i < gdb_hw_watch_max[cpui]; i++)
        {
            printf("%d  0x%lx  %s      %s\n", i,
                   gdb_hw_watch[cpui][i].bpt_addr,
                   get_bp_state_str(gdb_hw_watch[cpui][i].state),
                   get_bp_type_str(gdb_hw_watch[cpui][i].type));
        }

        printf("\nbreakpoint num = %d:\n", gdb_hw_break_max[cpui]);
        printf("Id    Addr    State      Type\n");

        for (i = 0; i < gdb_hw_break_max[cpui]; i++)
        {
            printf("%d  0x%lx  %s      %s\n", i,
                   gdb_hw_break[cpui][i].bpt_addr,
                   get_bp_state_str(gdb_hw_break[cpui][i].state),
                   get_bp_type_str(gdb_hw_break[cpui][i].type));
        }
    }
}

int debug_watchpoint_init(void)
{
    int processor_id = cur_cpu_id();
    if (!kgdb_arch_init())
    {
        init_flag[processor_id] = 1;
        return 0;
    }
    return -1;
}

#define ALL_CORE_MASK   ( (1<<configNR_CPUS)-1 )
#define IS_SET_CORE(flag, core) ( ( (flag) & (1UL<<(core)) ) ? 1 : 0 )
#define SET_CORE(flag, core)    ( (flag) |= (1UL<<(core)) )
#define CLR_CORE(flag, core)    ( (flag) &= ~(1UL<<(core)) )

void gdbcmd_ipi_hander(void);
void freert_cpus_unlock(uint32_t cpu_sr);
uint32_t freert_cpus_lock(void);

static inline int gdb_call_ipi(int core_mask, volatile struct gdb_cmd_arg *cmd)
{
    return 0;
}

int debug_watchpoint_init_on_core(int core_mask)
{
    volatile struct gdb_cmd_arg cmd;
    memset((void *)&cmd, 0, sizeof(struct gdb_cmd_arg));
    cmd.cmd = GDBCMD_INIT;
    return gdb_call_ipi(core_mask, &cmd);
}

int remove_all_break_watch_points_on_core(int core_mask)
{
    volatile struct gdb_cmd_arg cmd;
    memset((void *)&cmd, 0, sizeof(struct gdb_cmd_arg));
    cmd.cmd = GDBCMD_RMALL_BREAK_WATCH;
    return gdb_call_ipi(core_mask, &cmd);
}

int gdb_isremoved_hw_break_on_core(int core_mask, unsigned long addr)
{
    volatile struct gdb_cmd_arg cmd;
    memset((void *)&cmd, 0, sizeof(struct gdb_cmd_arg));
    cmd.cmd = GDBCMD_IS_RM_HW_BREAK;
    cmd.addr = addr;
    return gdb_call_ipi(core_mask, &cmd);
}

int gdb_remove_hw_break_on_core(int core_mask, unsigned long addr)
{
    volatile struct gdb_cmd_arg cmd;
    memset((void *)&cmd, 0, sizeof(struct gdb_cmd_arg));
    cmd.cmd = GDBCMD_RM_HW_BREAK;
    cmd.addr = addr;
    return gdb_call_ipi(core_mask, &cmd);
}

int gdb_set_hw_break_on_core(int core_mask, unsigned long addr)
{
    volatile struct gdb_cmd_arg cmd;
    memset((void *)&cmd, 0, sizeof(struct gdb_cmd_arg));
    cmd.cmd = GDBCMD_SET_HW_BREAK;
    cmd.addr = addr;
    return gdb_call_ipi(core_mask, &cmd);
}

int gdb_isremoved_hw_watch_on_core(int core_mask, unsigned long addr)
{
    volatile struct gdb_cmd_arg cmd;
    memset((void *)&cmd, 0, sizeof(struct gdb_cmd_arg));
    cmd.cmd = GDBCMD_IS_RM_HW_WATCH;
    cmd.addr = addr;
    return gdb_call_ipi(core_mask, &cmd);
}

int gdb_remove_hw_watch_on_core(int core_mask, unsigned long addr)
{
    volatile struct gdb_cmd_arg cmd;
    memset((void *)&cmd, 0, sizeof(struct gdb_cmd_arg));
    cmd.cmd = GDBCMD_RM_HW_WATCH;
    cmd.addr = addr;
    return gdb_call_ipi(core_mask, &cmd);
}

int gdb_set_hw_watch_on_core(int core_mask, unsigned long addr, enum gdb_bptype type)
{
    volatile struct gdb_cmd_arg cmd;
    memset((void *)&cmd, 0, sizeof(struct gdb_cmd_arg));
    cmd.cmd = GDBCMD_SET_HW_WATCH;
    cmd.addr = addr;
    cmd.bp = type;
    return gdb_call_ipi(core_mask, &cmd);
}

void gdbcmd_ipi_hander(void)
{
    int processor_id = cur_cpu_id();
    volatile struct gdb_cmd_arg *cmd = gdb_cmd;
    if (!cmd)
    {
        printf("gdb: cpu%d\n", processor_id);
        return;
    }

    switch (cmd->cmd)
    {
        case GDBCMD_NOP:
            break;
        case GDBCMD_INIT:
            cmd->reply[processor_id] = debug_watchpoint_init();
            break;
        case GDBCMD_RMALL_BREAK_WATCH:
            cmd->reply[processor_id] = remove_all_break_watch_points();
            break;

        case GDBCMD_IS_RM_HW_BREAK:
            cmd->reply[processor_id] = gdb_isremoved_hw_break(cmd->addr);
            break;
        case GDBCMD_RM_HW_BREAK:
            cmd->reply[processor_id] = gdb_remove_hw_break(cmd->addr);
            break;
        case GDBCMD_SET_HW_BREAK:
            cmd->reply[processor_id] = gdb_set_hw_break(cmd->addr);
            break;

        case GDBCMD_IS_RM_HW_WATCH:
            cmd->reply[processor_id] = gdb_isremoved_hw_watch(cmd->addr);
            break;
        case GDBCMD_RM_HW_WATCH:
            cmd->reply[processor_id] = gdb_remove_hw_watch(cmd->addr);
            break;
        case GDBCMD_SET_HW_WATCH:
            cmd->reply[processor_id] = gdb_set_hw_watch(cmd->addr, cmd->bp);
            break;
        default:/*SMP_DBG("unknown gdb cmd\n");*/
            break;
    };

    cmd->status[processor_id] = 1;
    isb();
    dsb();
}

int debug_watchpoint_init_all_core(void)
{
    return debug_watchpoint_init_on_core(ALL_CORE_MASK);
}

int remove_all_break_watch_points_all_core(int core_mask)
{
    return remove_all_break_watch_points_on_core(ALL_CORE_MASK);
}

int gdb_isremoved_hw_break_all_core(unsigned long addr)
{
    return gdb_isremoved_hw_break_on_core(ALL_CORE_MASK, addr);
}

int gdb_remove_hw_break_all_core(unsigned long addr)
{
    return gdb_remove_hw_break_on_core(ALL_CORE_MASK, addr);
}

int gdb_set_hw_break_all_core(unsigned long addr)
{
    return gdb_set_hw_break_on_core(ALL_CORE_MASK, addr);
}

int gdb_isremoved_hw_watch_all_core(unsigned long addr)
{
    return gdb_isremoved_hw_watch_on_core(ALL_CORE_MASK, addr);
}

int gdb_remove_hw_watch_all_core(unsigned long addr)
{
    return gdb_remove_hw_watch_on_core(ALL_CORE_MASK, addr);
}

int gdb_set_hw_watch_all_core(unsigned long addr, enum gdb_bptype type)
{
    return gdb_set_hw_watch_on_core(ALL_CORE_MASK, addr, type);
}

