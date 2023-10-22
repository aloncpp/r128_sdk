#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <tinatest.h>

extern int cmd_memtest(int argc, char **argv);

int tt_ddr(int argc, char **argv)
{
	int ret = 0;

	char *ddr_argv[] = {
		"memtester",
		"-t",
		"1000",
	};

	int ddr_argc = sizeof(ddr_argv) / sizeof(ddr_argv[0]);

	ret = cmd_memtest(ddr_argc, ddr_argv);

	if (ret)
	{
		printf("ddr test failed!");
		return -1;
	}

	return 0;
}
testcase_init(tt_ddr, ddr, ddr for tinatest);
