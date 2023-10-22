#include <stdint.h>
#include <tinatest.h>
#include <stdio.h>

static int tinatest_funclist_test1(int argc, char *argv[])
{
	int i;
	printf("%s...\n", __func__);
	for (i = 0; i < argc; i++)
		printf("%s ", argv[i]);
	printf("\n");
	return 0;
}

static int tinatest_funclist_test2(int argc, char *argv[])
{
	int i;
	printf("%s...\n", __func__);
	for (i = 0; i < argc; i++)
		printf("%s ", argv[i]);
	printf("\n");
	return 0;
}

tt_funclist_t tt_funclist[] = {
	{"tinatest funclist test1", tinatest_funclist_test1},
	TT_FUNCLIST_LABEL(tinatest_funclist_test2),
};

testcase_init_with_funclist(tt_funclist, funclist, testcase funclist test);
