#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <reent.h>

#include <pthread.h>
#include <console.h>
#include "sunxi_amp.h"

#include "pm_debug.h"
#include "pm_rpcfunc.h"

int rpc_pm_wakelocks_getcnt_dsp(int stash)
{
	void *args[1] = {0};
	args[0] = (void *)(intptr_t)stash;
	pm_stub_trace1(stash);
	return func_stub(RPCCALL_PMOFDSP(_rpc_pm_wakelocks_getcnt_dsp), 1, ARRAY_SIZE(args), args);
}

int rpc_pm_msgtodsp_trigger_notify(int mode, int event)
{
	void *args[2] = {0};
	args[0] = (void *)(intptr_t)mode;
	args[1] = (void *)(intptr_t)event;
	pm_stub_trace2(mode, event);
	return func_stub(RPCCALL_PMOFDSP(_rpc_pm_msgtodsp_trigger_notify), 1, ARRAY_SIZE(args), args);
}

int rpc_pm_msgtodsp_trigger_suspend(int mode)
{
	void *args[1] = {0};
	args[0] = (void *)(intptr_t)mode;
	pm_stub_trace1(mode);
	return func_stub(RPCCALL_PMOFDSP(_rpc_pm_msgtodsp_trigger_suspend), 1, ARRAY_SIZE(args), args);
}

int rpc_pm_msgtodsp_check_subsys_assert(int mode)
{
	void *args[1] = {0};
	args[0] = (void *)(intptr_t)mode;
	pm_stub_trace1(mode);
	return func_stub(RPCCALL_PMOFDSP(_rpc_pm_msgtodsp_check_subsys_assert), 1, ARRAY_SIZE(args), args);
}

int rpc_pm_msgtodsp_check_wakesrc_num(int type)
{
	void *args[1] = {0};
	args[0] = (void *)(intptr_t)type;
	pm_stub_trace1(type);
	return func_stub(RPCCALL_PMOFDSP(_rpc_pm_msgtodsp_check_wakesrc_num), 1, ARRAY_SIZE(args), args);
}
