#include <stdio.h>
#include "console.h"
#include <cli_console.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int cmd_xip_test(int argc, const char **argv)
{
    printf("%s, %d\n", __func__, __LINE__);
	debug_flashc2();
    printf("%s, %d\n", __func__, __LINE__);
    HAL_Xip_Init(0, 0);
    printf("%s, %d\n", __func__, __LINE__);
    return 0;
}
FINSH_FUNCTION_EXPORT_ALIAS(cmd_xip_test, xip, ktime);

