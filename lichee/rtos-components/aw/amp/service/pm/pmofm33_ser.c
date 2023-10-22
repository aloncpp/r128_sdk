#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "sunxi_amp.h"
#include <hal_cache.h>

#include "pm_debug.h"
#include "pm_rpcfunc.h"


// server: M33
static int _rpc_pm_set_wakesrc(int wakesrc_id, int core, int status)
{
	pm_ser_trace3(wakesrc_id, core, status);
	return rpc_pm_set_wakesrc(wakesrc_id, core, status);
}

static int _rpc_pm_trigger_suspend(int mode)
{
	pm_ser_trace1(mode);
	return rpc_pm_trigger_suspend(mode);
}

static int _rpc_pm_report_subsys_action(int subsys_id, int action)
{
	pm_ser_trace2(subsys_id, action);

	return rpc_pm_report_subsys_action(subsys_id, action);
}

int _rpc_pm_subsys_soft_wakeup(int affinity, int irq, int action)
{
	pm_ser_trace3(affinity, irq, action);
	return rpc_pm_subsys_soft_wakeup(affinity, irq, action);
}

sunxi_amp_func_table pmofm33_table[] = {
    {.func = (void *)&_rpc_pm_set_wakesrc, .args_num = 3, .return_type = RET_POINTER},
    {.func = (void *)&_rpc_pm_trigger_suspend, .args_num = 1, .return_type = RET_POINTER},
    {.func = (void *)&_rpc_pm_report_subsys_action, .args_num = 2, .return_type = RET_POINTER},
    {.func = (void *)&_rpc_pm_subsys_soft_wakeup, .args_num = 3, .return_type = RET_POINTER},
};

