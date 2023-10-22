#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <sunxi_amp.h>

#define SEND_TO_32BIT (1)
#define SEND_TO_64BIT (2)

struct rpcconsole_arg_t
{
    uint32_t argc;
    uint32_t flag;
    uint64_t argv_64[SUNXI_AMP_SH_MAX_CMD_ARGS];
    uint32_t argv_32[SUNXI_AMP_SH_MAX_CMD_ARGS];
    union
    {
        char *argv_cmd;
        uint64_t argv_cmd_data;
    };
};
