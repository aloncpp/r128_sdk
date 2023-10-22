#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <reent.h>

#include "sunxi_amp.h"
#include <pthread.h>
#include <console.h>

#include "pm_debug.h"
#include "pm_rpcfunc.h"

int rpc_pm_wakelocks_getcnt_riscv(int stash)
{
	void *args[1] = {0};
	args[0] = (void *)stash;
	pm_stub_trace1(stash);
	return func_stub(RPCCALL_PMOFRV(_rpc_pm_wakelocks_getcnt_riscv), 1, ARRAY_SIZE(args), args);
}


int rpc_pm_msgtorv_trigger_notify(int mode, int event)
{
	void *args[2] = {0};
	args[0] = (void *)mode;
	args[1] = (void *)event;
	pm_stub_trace2(mode, event);
	return func_stub(RPCCALL_PMOFRV(_rpc_pm_msgtorv_trigger_notify), 1, ARRAY_SIZE(args), args);
}

int rpc_pm_msgtorv_trigger_suspend(int mode)
{
	void *args[1] = {0};
	args[0] = (void *)mode;
	pm_stub_trace1(mode);
	return func_stub(RPCCALL_PMOFRV(_rpc_pm_msgtorv_trigger_suspend), 1, ARRAY_SIZE(args), args);
}

int rpc_pm_msgtorv_check_subsys_assert(int mode)
{
	void *args[1] = {0};
	args[0] = (void *)mode;
	pm_stub_trace1(mode);
	return func_stub(RPCCALL_PMOFRV(_rpc_pm_msgtorv_check_subsys_assert), 1, ARRAY_SIZE(args), args);
}

int rpc_pm_msgtorv_check_wakesrc_num(int type)
{
	void *args[1] = {0};
	args[0] = (void *)type;
	pm_stub_trace1(type);
	return func_stub(RPCCALL_PMOFRV(_rpc_pm_msgtorv_check_wakesrc_num), 1, ARRAY_SIZE(args), args);
}
