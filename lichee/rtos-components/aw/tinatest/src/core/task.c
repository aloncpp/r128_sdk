#include <stdio.h>
#include <string.h>
#include <fcntl.h>

#include "aw_list.h"
#include "aw_types.h"
#include "tinatest.h"
#include "../outlog/outlog.h"

#include <pthread.h>
#include <semaphore.h>

#define TT_DEFAULT_RUN_TIMES	1
#define TT_DEFAULT_RUN_ALONE	1
#define TT_TESTCASE_ARGC_MAX	7

static struct list_head TASK_LIST;
static int tt_task_info(struct tt_struct *current_tt);
static struct tt_struct * tt_lookup(const char* name)
{
	struct tt_struct *tt;

	for_each_testcases(tt) {
		if (strcmp(tt->name, name) == 0)
			return tt;
	}
	return NULL;
}

static inline void tt_default_para(struct tt_struct *tt){
	tt->argc = 0;
	tt->argv = NULL;
	tt->run_times = TT_DEFAULT_RUN_TIMES;
	tt->run_alone = TT_DEFAULT_RUN_ALONE;
}

enum tt_config_t
{
	unknown_type = -1,
	testcase_name=1,
	display_name,
	run_alone,
	run_times
};

/* 
 * input:
 * fd: file descriptor
 * size: limit of dst space
 * output:
 * dst: string of one line
 * return: >=0(success, lenth of one line)/-1(failed)
 */
static int tt_read_one_line(int fd, char *dst, int size){
	int ret = -1;
	int len = 0;
	while(1){
		char ch;
		ret = read(fd, &ch, 1);
		if(ret!=1){
			if(len!=0){
				ret = len;
				goto exit;
			}
			else{
				ret = -1;
				goto exit;
			}
		}
		if(ch==';'){
			while(1){
				ret = read(fd, &ch, 1);
				if(ret!=1 || ch=='\n'){
					ret = 0;
					goto exit;
				}
			}
		}
		if(ch=='\n'){
			dst[len] = 0;
			ret = len;
			goto exit;
		}
		if(ch<0x80 && ch!='\r'){
			dst[len] = ch;
			len++;
		}
	}
exit:
	return ret;
}

/* 
 * input:
 * line: string of one line, such as: <[testcase]> or <key = "val">
 * output:
 * key: string of key
 * val: string of key
 * return: 0(success)/-1(failed)
 */
static int tt_parser_one_line(const char *line, char *key, char *val){
	int ret = -1;
	if(!line)
		return 0;
	
	if(line[0]=='['){
		//[testcase]
		line++;
		const char *end = strchr(line, ']');
		if(!end)
			goto exit;
		memcpy(key, "testcase", 9);
		unsigned int len = (unsigned int)(end - line);
		if(0==len)
			goto exit;
		memcpy(val, line, len);
		val[len] = 0;
	}
	else{
		//null or <key = "val">
		//get key
		const char *end = strchr(line, ' ');
		if(!end)
			goto exit;
		unsigned int len = (unsigned int)(end - line);
		if(0==len)
			goto exit;
		memcpy(key, line, len);
		key[len] = 0;
		//get val
		line = strchr(line, '\"');
		if(!line)
			goto exit;
		line++;
		end = strchr(line, '\"');
		if(!end)
			goto exit;
		len = (unsigned int)(end - line);
		if(0==len)
			goto exit;
		memcpy(val, line, len);
		val[len] = 0;
	}
	ret = 0;
exit:
	return ret;
}

static int tt_add_argv(struct tt_struct *tt, char *argv){
	if(tt->argc==0 && !tt->argv){
		tt->argv = malloc( sizeof(tt->argv[0]) * (TT_TESTCASE_ARGC_MAX+1) );
		if(!tt->argv){
			ERROR("malloc failed!\n");
			goto err;
		}
	}
	else if(tt->argc==0 && tt->argv){
		ERROR("should not be here!\n");
		goto err;
	}

	if(tt->argc >= TT_TESTCASE_ARGC_MAX)
		goto err;

	char *buf = malloc(TT_ARGV_LEN_MAX+1);
	if(!buf){
		ERROR("malloc failed!\n");
		goto err;
	}
	int len = strlen(argv);
	if(len > TT_ARGV_LEN_MAX)
		len = TT_ARGV_LEN_MAX;
	memcpy(buf, argv, len);
	buf[len] = 0;
	tt->argv[tt->argc] = buf;
	tt->argc++;

	return 0;
err:
	return -1;
}

static int tt_read_testcase_config(void){
	int ret = -1;
	struct tt_struct *tt;

	if(access(TINATEST_PATH, F_OK)!=0){
		//TINATEST_PATH not exist, mkdir
		mkdir(TINATEST_PATH, 0777);
		goto exit;
	}

	if(access(TINATEST_INI_FILE, F_OK | R_OK )!=0){
		//TINATEST_INI_FILE not exist or can not be read
		goto exit;
	}

	int fd = open(TINATEST_INI_FILE, O_RDONLY);
	if(fd<0){
		ERROR("open %s failed!\n", TINATEST_INI_FILE);
		goto exit;
	}
	while(1){
		char buf[128];
		char key[64];
		char val[64];
		memset(buf, 0, 128);
		memset(key, 0, 64);
		memset(val, 0, 64);

		int len = tt_read_one_line(fd, buf, 128);
		if(len<0){
			DEBUG("tt_read_one_line EOF!\n");
			ret = 0;
			break;
		}
		else if(len==0){
			DEBUG("tt_read_one_line empty!\n");
			continue;
		}

		if(tt_parser_one_line(buf, key, val)<0){
			printf("tt_parser_one_line failed!\n");
			break;
		}

		DEBUG("key:%s, val:%s\n", key, val);
		if(!strcmp(key, "testcase")){
			//[testcase name]
			tt = tt_lookup(val);
			if(tt==NULL){
				ERROR("can not find testcase name:%s\n", val);
				break;
			}
			if (tt && tt->func) {
				tt_default_para(tt);
				list_add_tail(&tt->node, &TASK_LIST);
			}
		}
		else if(!strcmp(key, "display_name")){
			//display_name
			if(strcmp(tt->name, val))
				ERROR("diaplay_name(%s) mismatchs?\n", val);
		}
		else if(!strcmp(key, "run_alone")){
			//run_alone
			tt->run_alone = atoi(val);
		}
		else if(!strcmp(key, "run_times")){
			//run_times
			tt->run_times = atoi(val);
		}
		else if(!strcmp(key, "argv")){
			//argv
			tt_add_argv(tt, val);
		}
		else{
			ERROR("unknown key:%s, val:%s\n", key, val);
			break;
		}
	}
	close(fd);

exit:
	return ret;
}

static int tt_save_testcase_config(void){
	int fd = -1;
	char buf[256];
	struct tt_struct *tt;

	fd = open(TINATEST_INI_FILE, O_RDWR | O_CREAT | O_TRUNC, 0666);
	if (fd < 0) {
		ERROR("%s create error\n", TINATEST_INI_NAME);
		return -1;
	}

	list_for_each_entry(tt, &TASK_LIST, node) {
		tt_task_info(tt);
		snprintf(buf, sizeof(buf), "[%s]\n", tt->name);
		write(fd, buf, strlen(buf));
		snprintf(buf, sizeof(buf), "display_name = \"%s\"\n", tt->name);
		write(fd, buf, strlen(buf));
		if(tt->run_times != TT_DEFAULT_RUN_TIMES){
			snprintf(buf, sizeof(buf), "run_times = \"%d\"\n", tt->run_times);
			write(fd, buf, strlen(buf));
		}
		if(tt->run_alone != TT_DEFAULT_RUN_ALONE){
			snprintf(buf, sizeof(buf), "run_alone = \"%d\"\n", tt->run_alone);
			write(fd, buf, strlen(buf));
		}
		for(int i=0;i<tt->argc;i++){
			snprintf(buf, sizeof(buf), "argv = \"%s\"\n", tt->argv[i]);
			write(fd, buf, strlen(buf));
		}
	}
	close(fd);

	return 0;
}

/* 
 * parser testcase arg
 * argc: number of argv
 * argv: argv1 argv2 argv3
 * return: 0(success)/-1(failed)
 */
static int tt_paser_one_testcase(struct tt_struct *tt, int argc, char *argv[]){
	if(!tt->argv && argc>0){
		//need free?
		tt->argv = malloc( sizeof(argv[0]) * (TT_TESTCASE_ARGC_MAX+1) );
		if(!tt->argv){
			ERROR("malloc failed!\n");
			goto err;
		}
	}
	for(int i=0; i<argc; i++){
		tt->argv[tt->argc] = argv[tt->argc];
		tt->argc++;
		if(tt->argc>=TT_TESTCASE_ARGC_MAX)
			break;
	}
	return 0;
err:
	return -1;
}

int tt_testcase_list_init(int argc, char *argv[]){
	int i;
	int ret = -1;
	struct tt_struct *tt;

	if(argc<0){
		ERROR("tt_testcase_list_init argc(%d) err!\n", argc);
		goto exit;
	}

	INIT_LIST_HEAD(&TASK_LIST);

	//add one testcase to list
	if(argc>0 && argv && argv[0]){
		tt = tt_lookup(argv[0]);
		if(!tt){
			ERROR("can not find testcase: %s!\n", argv[0]);
			goto exit;
		}
		tt_default_para(tt);
		ret = tt_paser_one_testcase(tt, argc-1, &argv[1]);
		if(!ret)
			list_add_tail(&tt->node, &TASK_LIST);

		goto exit;
	}

	//read testcase config from file
	ret = tt_read_testcase_config();
	if(!ret){
		INFO("read testcase config success!\n");

#if 1
		goto save_config;
#else
		goto exit;
#endif
	}

	//add all testcase to list
	for_each_testcases(tt) {
		tt_default_para(tt);
		list_add_tail(&tt->node, &TASK_LIST);
	}

save_config:
	//save testcase config to file
	ret = tt_save_testcase_config();
	if(ret){
		ERROR("save testcase config failed!\n");
		goto exit;
	}
exit:
	return ret;
}

struct tt_data_t{
	struct outlog_data_t *outlog_data;
	struct interact_data_t *interact_data;
};

struct tt_data_t *tt_conn_init(volatile int *exit)
{
	int ret = -1;
	struct tt_data_t *data= malloc(sizeof(struct tt_data_t));
	if(!data){
		ERROR("malloc failed!\n");
		goto err1;
	}

	data->interact_data = interact_init();
	if (!data->interact_data){
		ERROR("interact_init err\n");
		goto err2;
	}

	data->outlog_data = outlog_init(exit);
	if(!data->outlog_data){
		ERROR("outlog_init err\n");
		goto err3;
	}

	return data;
err3:
	interact_exit(data->interact_data);
err2:
	free(data);
err1:
	return NULL;
}

int tt_conn_exit(struct tt_data_t *data)
{
	if(data){
		interact_exit(data->interact_data);
		outlog_exit(data->outlog_data);
		free(data);
	}
	return 0;
}

struct tt_thread_res_t{
	struct tt_struct *current_tt;
	pthread_t tid;
	sem_t *sem_prev;
	sem_t sem;
};

static int tt_task_info(struct tt_struct *current_tt){
	DEBUG("\n");
	DEBUG("==============================\n");
	DEBUG("this: %p\n", current_tt);
	DEBUG("name: %s\n", current_tt->name);
	DEBUG("desc: %s\n", current_tt->desc);
	DEBUG("func: %p\n", current_tt->func);
	DEBUG("argc: %d\n", current_tt->argc);
	for(int i=0;i<current_tt->argc;i++){
		DEBUG("argv[%d]: %s\n", i, current_tt->argv[i]);
	}
	DEBUG("run_times: %d\n", current_tt->run_times);
	DEBUG("run_times_cur: %d\n", current_tt->run_times_cur);
	DEBUG("run_alone: %d\n", current_tt->run_alone);
	DEBUG("result: %d\n", current_tt->result);
	DEBUG("node: %p\n", current_tt->node);
	DEBUG("==============================\n\n");
	return 0;
}

static void *tt_task_thread(void* arg){
	struct tt_thread_res_t *res = (struct tt_thread_res_t *)arg;
	struct tt_struct *current_tt = res->current_tt;

	tt_task_info(current_tt);

	outlog_call_before_one(current_tt);
	for(current_tt->run_times_cur=0;current_tt->run_times_cur<current_tt->run_times;current_tt->run_times_cur++){
		if(current_tt->run_times!=1){
			INFO("%s: %d/%d times\n", current_tt->name, (current_tt->run_times_cur+1), current_tt->run_times);
		}
		current_tt->result = current_tt->func(current_tt->argc, current_tt->argv);
		if (current_tt->result < 0) {
			ERROR("testcase run err : %s\n", current_tt->name);
			break;
		}
	}

	tt_task_info(current_tt);

	outlog_call_after_one_end(current_tt);//mark

	sem_wait(&res->sem);
	sem_post(res->sem_prev);
	sem_destroy(&res->sem);
	free(res);

	return NULL;
}

static sem_t *create_tt_task(struct tt_struct *current_tt, sem_t *sem, int *result){
	
	struct tt_thread_res_t *res = malloc(sizeof(*res));
	if(!res){
		return NULL;
	}
	memset(res, 0, sizeof(*res));
	res->current_tt = current_tt;
	res->sem_prev = sem;
	sem_init(&res->sem, 0, 0);

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	struct sched_param sched;
	sched.sched_priority = 3;
	pthread_attr_setschedparam(&attr, &sched);
	pthread_attr_setstacksize(&attr, 32768);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	int ret = pthread_create(&res->tid, &attr, tt_task_thread, res);
	if(ret){
		ERROR("create tt_task: %s fail\n", current_tt->name);
		current_tt->result = -1;
		sem_destroy(&res->sem);
		free(res);
		return NULL;
	}
	char name[configMAX_TASK_NAME_LEN+1];
	
	tname_to_thread_name(name, current_tt->name);
	pthread_setname_np(res->tid, name);

	return &res->sem;
}

static inline int tt_wait_task_finish(sem_t *sem_head, sem_t **sem_tail){
	sem_post(*sem_tail);
	sem_wait(sem_head);
	*sem_tail = sem_head;
	return 0;
}

int tt_run_testcase(struct tt_data_t *data)
{
	int ret = 0;
	struct tt_struct *current_tt;
	
	sem_t sem_head;
	sem_t *sem_tail = &sem_head;
	sem_init(&sem_head, 0, 0);
	outlog_call_before_all(&TASK_LIST);

	list_for_each_entry(current_tt, &TASK_LIST, node) {
		if(current_tt->run_alone){
			tt_wait_task_finish(&sem_head, &sem_tail);
		}
		sem_t *sem_tail_prev = sem_tail;
		sem_tail = create_tt_task(current_tt, sem_tail_prev, &ret);
		usleep(100*1000);
		if(sem_tail==NULL){
			sem_tail = sem_tail_prev;
			continue;
		}
		if(current_tt->run_alone){
			tt_wait_task_finish(&sem_head, &sem_tail);
		}
	}

	tt_wait_task_finish(&sem_head, &sem_tail);

	list_for_each_entry(current_tt, &TASK_LIST, node) {
		if(current_tt->result<0){
			ret = -1;
			break;
		}
	}
	outlog_call_after_all(&TASK_LIST);

	sem_destroy(&sem_head);

	return ret;
}

static const char *cur_task_name(void){
	TaskHandle_t handle = xTaskGetCurrentTaskHandle();
	if(!handle)
		return "none";
	return pcTaskGetName(handle);
}

#if 0
int tname_to_thread_name(char *thread_name,const char *tname){
	if(!thread_name || !tname)
		return -1;
	memcpy(thread_name, "tt-", 3);
	strcpy(thread_name+3, tname);
	return 0;
}
int tname(char *name){
	if(!name)
		return -1;
	const char *thread_name = cur_task_name();
	if(!thread_name)
		return -1;
	strcpy(name, thread_name+3);
	return 0;
}
#else
int tname_to_thread_name(char *thread_name,const char *tname){
	if(!thread_name || !tname)
		return -1;
	strcpy(thread_name, tname);
	return 0;
}
int tname(char *name){
	if(!name)
		return -1;
	const char *thread_name = cur_task_name();
	if(!thread_name)
		return -1;
	strcpy(name, thread_name);
	return 0;
}
#endif
