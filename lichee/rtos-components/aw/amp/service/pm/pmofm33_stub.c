#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <reent.h>

#include "sunxi_amp.h"
#include <pthread.h>
#include <console.h>

#include "pm_debug.h"
#include "pm_rpcfunc.h"

int rpc_pm_set_wakesrc(int wakesrc_id, int core, int status)
{
	void *args[3] = {0};
	args[0] = (void *)(intptr_t)wakesrc_id;
	args[1] = (void *)(intptr_t)core;
	args[2] = (void *)(intptr_t)status;
	pm_stub_trace3(wakesrc_id, core, status);
	return func_stub(RPCCALL_PMOFM33(_rpc_pm_set_wakesrc), 1, ARRAY_SIZE(args), args);
}

int rpc_pm_trigger_suspend(int mode)
{
	void *args[1] = {0};
	args[0] = (void *)(intptr_t)mode;
	pm_stub_trace1(mode);
	return func_stub(RPCCALL_PMOFM33(_rpc_pm_trigger_suspend), 1, ARRAY_SIZE(args), args);
}

int rpc_pm_report_subsys_action(int subsys_id, int action)
{
	void *args[2] = {0};
	args[0] = (void *)(intptr_t)subsys_id;
	args[1] = (void *)(intptr_t)action;
	pm_stub_trace2(subsys_id, action);
	return func_stub(RPCCALL_PMOFM33(_rpc_pm_report_subsys_action), 1, ARRAY_SIZE(args), args);
}

int rpc_pm_subsys_soft_wakeup(int affinity, int irq, int action)
{
	void *args[3] = {0};
	args[0] = (void *)(intptr_t)affinity;
	args[1] = (void *)(intptr_t)irq;
	args[2] = (void *)(intptr_t)action;
	pm_stub_trace3(affinity, irq, action);
	return func_stub(RPCCALL_PMOFM33(_rpc_pm_subsys_soft_wakeup), 1, ARRAY_SIZE(args), args);
}
