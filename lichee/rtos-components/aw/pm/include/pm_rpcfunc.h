
#ifndef _PM_RPCFUNC_H_
#define _PM_RPCFUNC_H_

#include <stdio.h>
#include "pm_base.h"
#include "pm_notify.h"
#include "pm_debug.h"

#undef  PM_DEBUG_MODULE
#define PM_DEBUG_MODULE  PM_DEBUG_AMPRPC

/*
 * When AMP-RPC is used, stubs and services function both are required.
 * So, these macros have to drop,
 * otherwise there will be a warning
 */

//#ifdef CONFIG_AMP_PMOFM33_SERVICE
int rpc_pm_set_wakesrc(int wakesrc_id, int core, int status);
int rpc_pm_trigger_suspend(int mode);
int rpc_pm_report_subsys_action(int subsys_id, int action);
int rpc_pm_subsys_soft_wakeup(int affinity, int irq, int action);
//#endif

//#ifdef CONFIG_AMP_PMOFRV_SERVICE
int rpc_pm_wakelocks_getcnt_riscv(int stash);
int rpc_pm_msgtorv_trigger_notify(int mode, int event);
int rpc_pm_msgtorv_trigger_suspend(int mode);
int rpc_pm_msgtorv_check_subsys_assert(int mode);
int rpc_pm_msgtorv_check_wakesrc_num(int type);
//#endif

//#ifdef CONFIG_AMP_PMOFDSP_SERVICE
int rpc_pm_wakelocks_getcnt_dsp(int stash);
int rpc_pm_msgtodsp_trigger_notify(int mode, int event);
int rpc_pm_msgtodsp_trigger_suspend(int mode);
int rpc_pm_msgtodsp_check_subsys_assert(int mode);
int rpc_pm_msgtodsp_check_wakesrc_num(int type);
//#endif

//#ifdef CONFIG_AMP_PMOFM33_STUB
int pm_subsys_soft_wakeup(int affinity, int irq, int action);
int pm_set_wakesrc(int wakesrc_id, int core, int status);
int pm_trigger_suspend(suspend_mode_t mode);
int pm_report_subsys_action(int subsys_id, int action);
//#endif

//#ifdef CONFIG_AMP_PMOFRV_STUB
uint32_t pm_wakelocks_getcnt_riscv(int stash);
int pm_msgtorv_trigger_notify(int mode, int event);
int pm_msgtorv_trigger_suspend(int mode);
int pm_msgtorv_check_subsys_assert(int mode);
int pm_msgtorv_check_wakesrc_num(int type);
//#endif

//#ifdef CONFIG_AMP_PMOFDSP_STUB
uint32_t pm_wakelocks_getcnt_dsp(int stash);
int pm_msgtodsp_trigger_notify(int mode, int event);
int pm_msgtodsp_trigger_suspend(int mode);
int pm_msgtodsp_check_subsys_assert(int mode);
int pm_msgtodsp_check_wakesrc_num(int type);
//#endif

#endif

