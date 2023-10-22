#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <console.h>

#include "rpcconsole.h"
#include "sunxi_amp.h"
#include <barrier.h>

#include <hal_cache.h>

static int _execute_cmd(struct rpcconsole_arg_t *rpc_arg)
{
    char *command_name;
    struct finsh_syscall *call;
    int ret = -1;

    hal_dcache_invalidate((unsigned long)rpc_arg, (unsigned long)sizeof(*rpc_arg));
#ifndef CONFIG_ARCH_DSP
    dsb();
    isb();
#endif
    hal_dcache_invalidate((unsigned long)rpc_arg->argv_cmd, SH_MAX_CMD_LEN);

    if (rpc_arg->flag == SEND_TO_32BIT) {
        command_name = (char *)(unsigned long)(rpc_arg->argv_32[0]);
    } else {
        command_name = (char *)(unsigned long)(rpc_arg->argv_64[0]);
    }

    call = finsh_syscall_lookup(command_name);
    if (call == NULL || call->func == NULL)
    {
        printf("The command(%s) no exist !\n", command_name);
        return -1;
    }

    if (rpc_arg->flag == SEND_TO_32BIT) {
        ret = call->func(rpc_arg->argc, (char **)(unsigned long)rpc_arg->argv_32);
    } else {
        ret = call->func(rpc_arg->argc, (char **)(unsigned long)rpc_arg->argv_64);
    }
    hal_dcache_invalidate((unsigned long)rpc_arg, (unsigned long)sizeof(*rpc_arg));
    hal_dcache_invalidate((unsigned long)rpc_arg->argv_cmd, SH_MAX_CMD_LEN);
    return ret;
}

sunxi_amp_func_table console_table[] =
{
    {.func = (void *)&_execute_cmd, .args_num = 1, .return_type = RET_POINTER},
};
