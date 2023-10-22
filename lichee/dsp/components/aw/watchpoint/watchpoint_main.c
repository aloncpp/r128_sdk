#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <console.h>

#include "aw_breakpoint.h"

static void show_help(void)
{
    debug_dump_all_breaks_info();
    printf("Usage: watchpoint (write | read | access | remove) addr [core mask]\n");
}

int cmd_watchpoint(int argc, char *argv[])
{
    unsigned long addr = 0;
    char *err = NULL;
    int core_mask;

    if (argc < 3)
    {
        show_help();
        return -1;
    }

    addr = strtoul(argv[2], &err, 0);
    if (*err != 0)
    {
        printf("addr error\n");
        return -1;
    }

    if (argc == 4)
    {
        core_mask = strtoul(argv[3], &err, 0);
        goto mutil_core;
    }

    if (!strcmp(argv[1], "write"))
    {
        return gdb_set_hw_watch(addr, BP_WRITE_WATCHPOINT);
    }
    else if (!strcmp(argv[1], "read"))
    {
        return gdb_set_hw_watch(addr, BP_READ_WATCHPOINT);
    }
    else if (!strcmp(argv[1], "access"))
    {
        return gdb_set_hw_watch(addr, BP_ACCESS_WATCHPOINT);
    }
    else if (!strcmp(argv[1], "remove"))
    {
        return gdb_remove_hw_watch(addr);
    }
    else
    {
        show_help();
    }
    return -1;

mutil_core:
    if (!strcmp(argv[1], "write"))
    {
        return gdb_set_hw_watch_on_core(core_mask, addr, BP_WRITE_WATCHPOINT);
    }
    else if (!strcmp(argv[1], "read"))
    {
        return gdb_set_hw_watch_on_core(core_mask, addr, BP_READ_WATCHPOINT);
    }
    else if (!strcmp(argv[1], "access"))
    {
        return gdb_set_hw_watch_on_core(core_mask, addr, BP_ACCESS_WATCHPOINT);
    }
    else if (!strcmp(argv[1], "remove"))
    {
        return gdb_remove_hw_watch_on_core(core_mask, addr);
    }
    else if (!strcmp(argv[1], "break"))
    {
        return gdb_set_hw_break_on_core(core_mask, addr);
    }
    else if (!strcmp(argv[1], "rmbreak"))
    {
        return gdb_remove_hw_break_on_core(core_mask, addr);
    }
    else
    {
        show_help();
    }
    return -1;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_watchpoint, watchpoint, Watchpoint Command);
