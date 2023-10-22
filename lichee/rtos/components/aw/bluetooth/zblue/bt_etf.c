#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
// #include <sys/param.h>
// #include <sys/ioctl.h>
// #include <sys/socket.h>
// #include <signal.h>
// #include "btetf.h"

#include "console.h"
#include "hal/sunxi_hal_watchdog.h"
#include "kernel/os/os.h"



int cmd_btetf_exec(int argc, char *argv[]);


FINSH_FUNCTION_EXPORT_CMD(cmd_btetf_exec, btetf, bt_etf_Test_Command);
