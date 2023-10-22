#include <console.h>
#include <cli_console.h>

extern cli_console cli_pseudo_console;

int cmd_exit(int argc, char **argv)
{
    cli_console_set_exit_flag(cli_task_get_console(cli_current_task()));
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_exit, exit, Console Exit Command);
