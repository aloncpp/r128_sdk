#include <hal_cmd.h>
#include "multi_console_internal.h"

int cmd_exit(int argc, char **argv)
{
    cli_console_set_exit(get_current_console());
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_exit, exit, Console Exit Command);
