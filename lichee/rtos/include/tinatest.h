#ifndef __TINATEST_H
#define __TINATEST_H

#include <unistd.h>
#include <stdlib.h>
#include <aw_common.h>
#include <aw_list.h>

#define SECTION(x)  __attribute__((section(x)))
#define RT_USED     __attribute__((used))

#define for_each_testcases(tt)        \
    for (tt = &_tt_begin; (unsigned long)tt < (unsigned long)&_tt_end; tt++)

//#define TT_DEBUG
/* ERROR */
#define ERROR(fmt, arg...) \
	do { \
		fprintf(stdout, "\033[31m" "[ERROR]: <%s:%d>: " fmt "\033[0m" "\r", \
				__func__, __LINE__, ##arg); \
		fflush(stdout);\
	}while(0)


/* DEBUG */
#ifdef TT_DEBUG
#define DEBUG(fmt, arg...) \
	do { \
		fprintf(stdout, "\033[32m" "[DEBUG]: <%s:%d>: " fmt "\033[0m", \
				__func__, __LINE__, ##arg); \
		fflush(stdout); \
	}while(0)
#else
#define DEBUG(fmt, arg...)
#endif

/* INFO */
#define INFO(fmt, arg...) \
	do { \
		fprintf(stdout, "\033[0m" "[tinatest]: " fmt "\033[0m", \
				##arg); \
		fflush(stdout);\
	}while(0)

typedef int (*testcase_func)(int argc, char *argv[]);

struct tt_struct {
    const char *name;
    const char *desc;
    testcase_func func;
    int argc;
    char **argv;
	int run_times;
	int run_alone;
	volatile int run_times_cur;
    int result;
	int tid;
    struct list_head node;
};
extern struct tt_struct _tt_begin, _tt_end;

/*
 * tinatest_init: register a testcase to tinatest
 *
 * @func: the callback function in type testcase_func
 * @cmd: the comnand string of testcase
 * @desc: the descriptive string for this testcase
 */
#define testcase_init(func, cmd, desc)                                  \
    const char __tsym_##cmd##_name[] SECTION(".rodata") = #cmd;         \
    const char __tsym_##cmd##_desc[] SECTION(".rodata") = #desc;        \
    static struct tt_struct __ttcall_##cmd RT_USED SECTION("ttcall") =  \
    {                                                                   \
        __tsym_##cmd##_name,                                            \
        __tsym_##cmd##_desc,                                            \
        (testcase_func)&func,                                           \
    }

typedef struct {
    char *func_desc;
    testcase_func func;
} tt_funclist_t;

#define TT_FUNCLIST_LABEL(func) 	{ #func, func }

/*
 * tinatest_init_with_funclist: register a testcase with function list
 *
 * @funclist: testcase function list array
 * @cmd: the comnand string of testcase
 * @desc: the descriptive string for this testcase
 */
#define testcase_init_with_funclist(funclist, cmd, desc) \
int tt_##cmd##_main(int argc, char **argv) \
{ \
	int c =0, i = 0; \
	optind = 0; \
	while ((c = getopt(argc, argv, "lr:")) != -1) { \
		switch (c) { \
		case 'r': \
			i = atoi(optarg); \
			if (i < 0 || (i + 1) > ARRAY_SIZE(funclist)) { \
				printf("invalid index\n"); \
				return -1; \
			} \
			if (funclist[i].func) { \
				argc = argc - optind + 1; \
				argv = argv + optind - 1; \
				optind = 0; \
				return (*funclist[i].func)(argc, argv); \
			} \
			printf("no function...\n"); \
			return -1; \
		case 'l': \
			printf("%5s%40s\n", "index", "func_desc"); \
			for (i = 0; i < ARRAY_SIZE(funclist); i++) \
				printf("%5d%40s\n", i, funclist[i].func_desc); \
			return 0; \
		} \
	} \
	printf("Usage: tt testcase [opion]\n"); \
	printf("option:\n"); \
	printf("-l:          list testcase all func.\n"); \
	printf("-r [index]:  run index func.\n"); \
	return 0; \
} \
testcase_init(tt_##cmd##_main, cmd, desc)

/*
 * api for testcase
 */
int tname_to_thread_name(char *thread_name,const char *tname);
int tname(char *name);
int task(const char *ask, char *reply, int len);
int ttips(const char *tips);
int ttrue(const char *tips);

#define TINATEST_PATH		"/data/tinatest"
#define TINATEST_OK_NAME	"tinatest_ok"
#define TINATEST_INI_NAME	"tinatest.ini"

#define TINATEST_OK_FILE	TINATEST_PATH "/" TINATEST_OK_NAME
#define TINATEST_INI_FILE	TINATEST_PATH "/" TINATEST_INI_NAME

#define TT_NAME_LEN_MAX		(configMAX_TASK_NAME_LEN)
#define TT_ARGV_LEN_MAX		(31)

#endif
