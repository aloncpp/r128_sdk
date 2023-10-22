#include <stdlib.h>
#include <stdio.h>
#include <backtrace.h>
#include <console.h>

int cmd_panic(int argc, char **argv)
{
    __asm__ volatile("ill\n" :::"memory");
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_panic, panic, Execption Test Command);

int cmd_backtrace(int argc, char **argv)
{
    if (argc < 2)
    {
        backtrace(NULL, NULL, 0, 0, printf);
    }
    else
    {
        backtrace(argv[1], NULL, 0, 0, printf);
    }
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_backtrace, backtrace, Backtrace Command);
