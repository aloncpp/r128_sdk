#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <tinatest.h>

extern int rwcheck_main(int argc, char **argv);

int tt_throughout(int argc, char **argv)
{
	int ret = 0;

	char *throughout_argv[] = {
		"rwcheck",
		"-d",
		"/data",
		"-s",
		"128k",
		"-t",
		"1000000",
		"-j",
		"2",
		"-p",
		"95"
	};

	int throughout_argc = sizeof(throughout_argv) / sizeof(throughout_argv[0]);

	ret = rwcheck_main(throughout_argc, throughout_argv);

	if (ret)
	{
		printf("rw-throughout test failed!");
		return -1;
	}

	return 0;
}
testcase_init(tt_throughout, rw_throughput, throughput for tinatest);
