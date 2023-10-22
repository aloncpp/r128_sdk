#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <statfs.h>
#include <dirent.h>

#include "sunxi_amp.h"
#include <hal_cache.h>

#include "pm_debug.h"
#include "pm_rpcfunc.h"

static int _rpc_pm_wakelocks_getcnt_riscv(int stash)
{
	pm_ser_trace1(stash);
	return rpc_pm_wakelocks_getcnt_riscv(!!stash);
}

static int _rpc_pm_msgtorv_trigger_notify(int mode, int event)
{
	pm_ser_trace2(mode, event);
	return rpc_pm_msgtorv_trigger_notify(mode, event);
}

static int _rpc_pm_msgtorv_trigger_suspend(int mode)
{
	pm_ser_trace1(mode);
	return rpc_pm_msgtorv_trigger_suspend(mode);
}

static int _rpc_pm_msgtorv_check_subsys_assert(int mode)
{
	pm_ser_trace1(mode);
	return rpc_pm_msgtorv_check_subsys_assert(mode);
}

static int _rpc_pm_msgtorv_check_wakesrc_num(int type)
{
	pm_ser_trace1(type);
	return rpc_pm_msgtorv_check_wakesrc_num(type);
}

sunxi_amp_func_table pmofrv_table[] = {
    {.func = (void *)&_rpc_pm_wakelocks_getcnt_riscv, .args_num = 1, .return_type = RET_POINTER},
    {.func = (void *)&_rpc_pm_msgtorv_trigger_notify, .args_num = 2, .return_type = RET_POINTER},
    {.func = (void *)&_rpc_pm_msgtorv_trigger_suspend, .args_num = 1, .return_type = RET_POINTER},
    {.func = (void *)&_rpc_pm_msgtorv_check_subsys_assert, .args_num = 1, .return_type = RET_POINTER},
    {.func = (void *)&_rpc_pm_msgtorv_check_wakesrc_num, .args_num = 1, .return_type = RET_POINTER},
};

