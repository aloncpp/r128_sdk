#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <platform.h>
#include <aw_io.h>
#include <console.h>
#include <stub.h>

#include <FreeRTOS.h>
#include <task.h>

#include <stdio.h>
#include <xtensa/tie/xt_hifi2.h>
#include <xtensa/config/core.h>
#include <xtensa/tie/xt_timer.h>

/* use example
void test_fun(void);
STUB_FUNCTION((const unsigned int)&test_fun, test_fun,test_fun_);
void test_fun(void)
{
    int i = 0;
    i++;
    //i = STUB_GET_ADDR(test_fun_);
    STUB_MSG_UPDATE(test_fun_);

}*/

int stub_refresh_show(int argc, char **argv)
{
	struct stub_msg_t* index;
	printf("*****************stub test*****************\n");
	printf("fun name\t\tfun addr\t\tcnt\n");
	for (index = (struct stub_msg_t *)&_stubTab_start; (uint32_t)index < (uint32_t)&_stubTab_end; index++)
	{
		printf("[%s]\t0x%x\t%d\n",
			index->func_name,
			index->addr,
			index->cnt);
		index->cnt = 0;
	}
	return 0;
}

FINSH_FUNCTION_EXPORT_CMD(stub_refresh_show, stub_cmd_test, stub test);
