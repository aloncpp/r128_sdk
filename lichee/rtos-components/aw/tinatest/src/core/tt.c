#define pr_fmt(fmt) "tinatest: " fmt

#include <stdint.h>
#include <errno.h>
#include <awlog.h>
#include <console.h>
#include <string.h>
#include <tinatest.h>
#include <pthread.h>
#include "task.h"

static void tt_show(char *buf, uint32_t len)
{
    struct tt_struct *tt;

    for_each_testcases(tt) {
        if (buf && len) {
            int l;

            l = snprintf(buf, len, "    %s : %s\n", tt->name, tt->desc);
            //buf += l;
            len -= l;
            continue;
        }
        printf("    %s : %s\n", tt->name, tt->desc);
    }
}

static int tt_help(void)
{
    printf("tt: Tina Test for Tina SDK\n");
    printf("Usage:\n");
    printf("\ttt -p : print all testcase\n");
    printf("\ttt :  run all testcase\n");
    printf("\ttt <testcase> <arg1> <arg2> ... : run <testcase>, argument is : arg1, arg2...\n");
    return 0;
}

pthread_mutex_t tt_task_mutex = FREERTOS_POSIX_MUTEX_INITIALIZER;
static void *wait_connect_and_run_testcase(void *arg){
	pthread_mutex_lock(&tt_task_mutex);
	(void *)arg;
	volatile int *tt_exit = malloc(sizeof(int));
	if(!tt_exit){
		ERROR("malloc failed!\n");
		goto exit1;
	}

	*tt_exit = 0;
    struct tt_data_t *data = tt_conn_init(tt_exit);
    if (!data) {
	    ERROR("tt connections init failed!\n");
	    goto exit2;
    }

    int ret = tt_run_testcase(data);
    if (ret < 0) {
	    ERROR("task run testcase failed!\n");
	    goto exit2;
    }

exit2:
	*tt_exit = 1;//free in used thread
	tt_conn_exit(data);
exit1:
	pthread_mutex_unlock(&tt_task_mutex);
	return NULL;
}

static inline int tt_create_server_thread(void){
	pthread_t tid;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	struct sched_param sched;
	sched.sched_priority = 5;
	pthread_attr_setschedparam(&attr, &sched);
	pthread_attr_setstacksize(&attr, 32768);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	int ret = pthread_create(&tid, &attr, wait_connect_and_run_testcase, NULL);
	if(ret){
		return -1;
	}
	pthread_setname_np(tid, "tinatest");
	return 0;
}

int tinatest(int argc, char *argv[])
{
    int ret = -1;

    //tt -p
    if (argc > 1 && !strcmp(argv[1], "-p")) {
        tt_show(NULL, 0);
        return 0;
    }

    ret = tt_testcase_list_init(argc-1, argv?&argv[1]:NULL);
    if (ret < 0) {
	    ERROR("init testcase list failed\n");
	    goto err;
    }

	ret = tt_create_server_thread();
    if (ret < 0) {
	    ERROR("create server thread failed\n");
	    goto err;
    }

    return 0;
err:
    tt_help();
    return -1;

}
FINSH_FUNCTION_EXPORT_CMD(tinatest, tt, Tina Test Platfrom);
/*
callstack:

tinatest
	tt_testcase_list_init
		tt_paser_one_testcase
		tt_read_testcase_config
		(tt_default)
		tt_save_testcase_config
	tt_create_server_thread -> wait_connect_and_run_testcase
	(return)

wait_connect_and_run_testcase
	tt_conn_init
		interact_init
			interact_actor_init
		outlog_init
			dragonmat_module_init
				socket_init
					socket_listen
					xTaskCreate -> heart_socket
					socket_accept
	tt_run_testcase
		outlog_call_before_all
		create_tt_task -> tt_task_thread
		outlog_call_after_all
	tt_conn_exit
		interact_exit
			//interact_actor_exit
		outlog_exit
			dragonmat_module_exit
				socket_exit
	(return)

tt_task_thread
	outlog_call_before_one
	(testcase)
	outlog_call_after_one_end

*/
