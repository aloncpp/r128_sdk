#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <tinatest.h>

#define tt_printf(fmt, arg...) printf("\e[0m" "[%s] " fmt "\e[0m\n", tt_name, ##arg)

int tt_demo(int argc, char **argv)
{
    int i, opt;
    char reply[10];
    int ret = -1;
	char tt_name[TT_NAME_LEN_MAX+1];
	int sleep_ms = 500;
	tname(tt_name);
	tt_printf("=======TINATEST FOR %s=========", tt_name);

    tt_printf("It's in demo for tinatest\n");
    tt_printf("Calling: ");
    for (i = 0; i < argc; i++)
	    tt_printf("%s ", argv[i]);
    tt_printf("\n");

	if( (2==argc) && argv && argv[1]){
		char *err = NULL;
		unsigned long tmp = strtoul(argv[1], &err, 0);
		if (*err == 0){
			sleep_ms = tmp;
			tt_printf("set sleep_ms:%d", sleep_ms);
		}
	}

    ttips("ttips test: print tips to user\n");

    ret = ttrue("true test: user select yes/no?\n");
    if (ret < 0) {
	    tt_printf("enter no\n");
	    return -1;
    }

    ret = task("task test: user enter string", reply, 10);
    if (ret < 0) {
	    tt_printf("task err\n");
	    return -1;
    }
    tt_printf("user entry reply %s\n", reply);

	usleep(sleep_ms*1000);
	tt_printf("======TINATEST FOR %s OK=======", tt_name);
    return 0;
err:
	tt_printf("=====TINATEST FOR %s FAIL======", tt_name);
	return -1;
}
testcase_init(tt_demo, demo, demo for tinatest);
