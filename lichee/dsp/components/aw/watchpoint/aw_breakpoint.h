#ifndef AW_BREAKPOINT_H
#define AW_BREAKPOINT_H

#include <FreeRTOSConfig.h>

#define BREAK_INSTR_SIZE 4

#ifndef configNR_CPUS
#define configNR_CPUS 1
#endif

enum gdb_bptype
{
    BP_BREAKPOINT = 0,
    BP_HARDWARE_BREAKPOINT,
    BP_WRITE_WATCHPOINT,
    BP_READ_WATCHPOINT,
    BP_ACCESS_WATCHPOINT,
    BP_POKE_BREAKPOINT,
};

enum gdb_bpstate
{
    BP_UNDEFINED = 0,
    BP_REMOVED,
    BP_SET,
    BP_ACTIVE
};

struct gdb_bkpt
{
    unsigned long       bpt_addr;
    unsigned char       saved_instr[BREAK_INSTR_SIZE];
    enum gdb_bptype type;
    enum gdb_bpstate    state;
    unsigned long       length;
};

struct gdb_arch
{
    unsigned char gdb_bpt_instr[BREAK_INSTR_SIZE];
    unsigned long flags;

    int (*set_sw_breakpoint)(unsigned long addr, char *saved_instr);
    int (*remove_sw_breakpoint)(unsigned long addr, char *bundle);
    int (*set_hw_breakpoint)(int, unsigned long);
    int (*remove_hw_breakpoint)(int, unsigned long);
    int (*set_hw_watchpoint)(enum gdb_bptype, int, unsigned long);
    int (*remove_hw_watchpoint)(enum gdb_bptype, int, unsigned long);
};

enum gdb_cmdtype
{
    GDBCMD_NOP = 0,
    GDBCMD_INIT,
    GDBCMD_RMALL_BREAK_WATCH,

    GDBCMD_IS_RM_HW_BREAK,
    GDBCMD_RM_HW_BREAK,
    GDBCMD_SET_HW_BREAK,

    GDBCMD_IS_RM_HW_WATCH,
    GDBCMD_RM_HW_WATCH,
    GDBCMD_SET_HW_WATCH,
};

struct gdb_cmd_arg
{
    volatile int status[configNR_CPUS];
    int reply[configNR_CPUS];
    enum gdb_cmdtype cmd;
    unsigned long addr;
    enum gdb_bptype bp;
    int core;
};

extern struct gdb_arch arch_gdb_ops;

void debug_dump_all_breaks_info(void);

// current cpu only
int remove_all_break_watch_points(void);
int debug_watchpoint_init(void);

int gdb_set_hw_break(unsigned long addr);
int gdb_remove_hw_break(unsigned long addr);
int gdb_isremoved_hw_break(unsigned long addr);

int gdb_set_hw_watch(unsigned long addr, enum gdb_bptype type);
int gdb_remove_hw_watch(unsigned long addr);
int gdb_isremoved_hw_watch(unsigned long addr);

// specify cpu
int debug_watchpoint_init_on_core(int core_mask);
int remove_all_break_watch_points_on_core(int core_mask);

int gdb_isremoved_hw_break_on_core(int core_mask, unsigned long addr);
int gdb_remove_hw_break_on_core(int core_mask, unsigned long addr);
int gdb_set_hw_break_on_core(int core_mask, unsigned long addr);

int gdb_isremoved_hw_watch_on_core(int core_mask, unsigned long addr);
int gdb_remove_hw_watch_on_core(int core_mask, unsigned long addr);
int gdb_set_hw_watch_on_core(int core_mask, unsigned long addr, enum gdb_bptype type);

// all cpu
int debug_watchpoint_init_all_core(void);
int remove_all_break_watch_points_all_core(int core_mask);

int gdb_isremoved_hw_break_all_core(unsigned long addr);
int gdb_remove_hw_break_all_core(unsigned long addr);
int gdb_set_hw_break_all_core(unsigned long addr);

int gdb_isremoved_hw_watch_all_core(unsigned long addr);
int gdb_remove_hw_watch_all_core(unsigned long addr);
int gdb_set_hw_watch_all_core(unsigned long addr, enum gdb_bptype type);

#endif  /*AW_BREAKPOINT_H*/
