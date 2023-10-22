#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <console.h>

#include "rpcconsole.h"
#include "sunxi_amp.h"
#include <hal_cache.h>

static int cmd_rpcconsole(int argc, char **argv)
{
    int ret = -1;
    int i;

    if (argc <= 2)
    {
        printf("Usage: rpccli [arm|dsp|rv] commandname [arg0 ...] \n");
        return -1;
    }
    if (argc > SUNXI_AMP_SH_MAX_CMD_ARGS)
    {
        printf("maximum number of arg:%d\n", SUNXI_AMP_SH_MAX_CMD_ARGS);
        return -1;
    }

    struct rpcconsole_arg_t *rpc_arg = amp_align_malloc(sizeof(struct rpcconsole_arg_t));
    if (rpc_arg == NULL)
    {
        printf("Alloc memory failed!\n");
        return -1;
    }
    memset(rpc_arg, 0, sizeof(struct rpcconsole_arg_t));

    char *rpc_args_cmd = amp_align_malloc(SH_MAX_CMD_LEN);
    if (rpc_args_cmd == NULL)
    {
        printf("Alloc memory failed!\n");
        amp_align_free(rpc_arg);
        return -1;
    }
    memset(rpc_args_cmd, 0, SH_MAX_CMD_LEN);

    rpc_arg->argv_cmd = rpc_args_cmd;
    rpc_arg->argc = argc - 2;

    for (i = 2; i < argc; i++)
    {
        rpc_arg->argv_32[i - 2] = (uint32_t)(unsigned long)rpc_args_cmd;
        rpc_arg->argv_64[i - 2] = (uint32_t)(unsigned long)rpc_args_cmd;
        memcpy(rpc_args_cmd, argv[i], strlen(argv[i]));
        rpc_args_cmd += ALIGN_UP((strlen(argv[i]) + 1), 4);
    }

    void *args[1] = {0};
    args[0] = rpc_arg;

    if (!strcmp("arm", argv[1]))
    {
        rpc_arg->flag = SEND_TO_32BIT;
        hal_dcache_clean((unsigned long)rpc_arg, sizeof(*rpc_arg));
        hal_dcache_clean((unsigned long)rpc_arg->argv_cmd, SH_MAX_CMD_LEN);

        ret = func_stub(RPCCALL_ARM_CONSOLE(exe_cmd), 1, 1, (void *)&args);
    }
    else if (!strcmp("dsp", argv[1]))
    {
        rpc_arg->flag = SEND_TO_32BIT;
        hal_dcache_clean((unsigned long)rpc_arg, sizeof(*rpc_arg));
        hal_dcache_clean((unsigned long)rpc_arg->argv_cmd, SH_MAX_CMD_LEN);

        ret = func_stub(RPCCALL_DSP_CONSOLE(exe_cmd), 1, 1, (void *)&args);
    }
    else if (!strcmp("rv", argv[1]))
    {
        rpc_arg->flag = SEND_TO_64BIT;
        hal_dcache_clean((unsigned long)rpc_arg, sizeof(*rpc_arg));
        hal_dcache_clean((unsigned long)rpc_arg->argv_cmd, SH_MAX_CMD_LEN);

        ret = func_stub(RPCCALL_RV_CONSOLE(exe_cmd), 1, 1, (void *)&args);
    }

    amp_align_free(rpc_arg->argv_cmd);
    amp_align_free(rpc_arg);

    return ret;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_rpcconsole, rpccli, exe);
