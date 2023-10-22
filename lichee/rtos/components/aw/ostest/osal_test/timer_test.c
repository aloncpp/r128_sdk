#include <stdlib.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <hal_cmd.h>
#include <hal_timer.h>

static osal_timer_t timer;
static void timeout(void *param)
{
    printf("param:%s\n", (char *)param);
}

int cmd_osal_timer_test(int argc, char **argv)
{
    timer = osal_timer_create("timer1", timeout, "hello", 10, OSAL_TIMER_FLAG_PERIODIC); 
    if (timer) {
        osal_timer_start(timer);
    }
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_osal_timer_test, osal_timer_create, osal timer test);

int cmd_osal_timer_delete(int argc, char **argv)
{
    if (timer) {
        osal_timer_stop(timer);
        osal_timer_delete(timer);
    }
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_osal_timer_delete, osal_timer_delete, osal timer test);

int cmd_osal_timer_control(int argc, char **argv)
{
    unsigned long value = 2000;
    if (timer) {
        osal_timer_control(timer, OSAL_TIMER_CTRL_SET_PERIODIC, &value);
    }
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_osal_timer_control, osal_timer_control, osal timer test);
