#ifndef XTENSA_WBP_H
#define XTENSA_WBP_H

#ifdef __cplusplus
extern "C" {
#endif

void xtensa_uninstall_hw_breakpoint(int i);
void xtensa_uninstall_hw_watchpoint(int i);
int xtensa_install_hw_watchpoint(enum gdb_bptype type, int i, unsigned long addr);
int xtensa_install_hw_breakpoint(int i, unsigned long addr);
int monitor_mode_enabled(void);
int enable_monitor_mode(void);
int get_num_brp_resources(void);
int get_num_wrp_resources(void);

#ifdef __cplusplus
}
#endif

#endif
