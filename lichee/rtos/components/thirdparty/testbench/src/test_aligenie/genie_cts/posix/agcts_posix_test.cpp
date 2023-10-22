/**
 * Copyright (C) 2018 Alibaba.inc, All rights reserved.
 *
 * @file:    agcts_posix_test.cpp
 * @brief:
 * @author:  tanyan.lk@alibaba-inc.com
 * @date:    2019/7/10
 * @version: 1.0
 */

#include "agcts.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////
// checkpoint: make sure POSIX API supported
using namespace std;

int _agcts_posix_string()
{
    int ret = _AGCTS_FAIL;

    char str2[]="ab,cd,ef";
    char *ptr;
    char *p;
    _AGCTS_LOGD("before strtok:  str2=%s\n", str2);
    _AGCTS_LOGD("begin:\n");
    ptr = strtok_r(str2, ",", &p);
    while(ptr != NULL){
        _AGCTS_LOGD("str2=%s\n",str2);
        _AGCTS_LOGD("ptr=%s\n",ptr);
        ptr = strtok_r(NULL, ",", &p);
    }

    ret = _AGCTS_SUCCESS;

    return ret;
}

char * _make_message(const char *fmt, ...)
{
    int size = 1024;
    char *p;
    va_list ap;

    if ((p = (char *)malloc(size * sizeof(char))) == NULL)
        return NULL;

    while (1)
    {
        int n;
        char *tmp;

        va_start(ap, fmt);
        n = vsnprintf(p, size, fmt, ap);
        va_end(ap);

        if (n > -1 && n < size)
            return p;

        size *= 2;
        tmp = (char *)realloc(p, size * sizeof(char));
        if (tmp == NULL) {
            free(p);
            return NULL;
        }
        return tmp;
    }
}

int _agcts_posix_memory_io()
{
    int ret = _AGCTS_FAIL;
    // memory, stdio
    char *str = _make_message("%d,%d,%d,%d", 5, 6, 7, 8);
    if (str != NULL) {
        _AGCTS_LOGD("%s\n",str);

        if (strncmp(str, "5,6,7,8", strlen(str)) == 0)
            ret = _AGCTS_SUCCESS;

        free(str);
    }

    return ret;
}

int _agcts_posix_time()
{
    int ret = _AGCTS_FAIL;
    // time
    struct timeval tv;
    struct tm timeinfo;
    //char buffer[64];
    gettimeofday(&tv, NULL);
    localtime_r(&tv.tv_sec, &timeinfo);
    _AGCTS_LOGD("get_now_time_str  sec:%ld  timeinfo hour:%d\n", tv.tv_sec, timeinfo.tm_hour);
    //strftime(buffer, 64, "%a, %d %b %Y %H:%M:%S GMT", &timeinfo);
    //_AGCTS_LOGD("buffer:%s\n", buffer);

    ret = _AGCTS_SUCCESS;

    return ret;
}

int _agcts_posix_unistd()
{
    int ret = _AGCTS_FAIL;
    // unistd
    sleep(1);
    usleep(10);

    ret = _AGCTS_SUCCESS;

    return ret;
}

int _agcts_posix_stdlib()
{
    int ret = _AGCTS_FAIL;
    // stdlib
    srand(time(NULL));
    int irand = rand() % 10;
    _AGCTS_LOGD("irand=%d\n", irand);

    ret = _AGCTS_SUCCESS;

    return ret;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// checkpoint: make sure POSIX pthread API supported
static int test_num = 0;
static int test_num_mutex = 0;
static int test_count = 10000;
static pthread_t test_thread0;
static pthread_t test_thread1;
static pthread_t test_thread2;
static mutex test_mutex;
static bool run_pthread = false;

void _agcts_thread_construct(pthread_t *test_thread, void *(*thread_loop)(void *), const char *name)
{
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    struct sched_param sched;
    sched.sched_priority = 4;
    pthread_attr_setschedparam(&attr,&sched);
    pthread_attr_setstacksize(&attr, 4096);

    pthread_create(test_thread, &attr, thread_loop, NULL);
    pthread_setname_np(*test_thread, name);
}

void * _agcts_thread_loop2(void *arg)
{
    while(run_pthread) {
        int i;
	    test_mutex.lock();
        for(i = 0; i < test_count; i++)
            test_num_mutex--;
	    test_mutex.unlock();

        _AGCTS_LOGD("test_num_mutex=%d\n", test_num_mutex);
        usleep(1000 * 100);
    }
    return NULL;
}

void * _agcts_thread_loop1(void *arg)
{
    while(run_pthread) {
        int i;
        for(i = 0; i < test_count; i++)
            test_num--;

        _AGCTS_LOGD("test_num=%d\n", test_num);
        usleep(1000 * 100);
    }
    return NULL;
}

void * _agcts_thread_loop0(void *arg)
{
    _agcts_thread_construct(&test_thread1, _agcts_thread_loop1, "test_thread1");
    _agcts_thread_construct(&test_thread2, _agcts_thread_loop2, "test_thread2");
    while(run_pthread) {
        int i;
        test_mutex.lock();
        for(i = 0; i < test_count; i++)
            test_num_mutex++;
        test_mutex.unlock();
        for(i = 0; i < test_count; i++)
            test_num++;

        _AGCTS_LOGD("test_num_mutex=%d\n", test_num_mutex);
        _AGCTS_LOGD("test_num=%d\n", test_num);
        usleep(1000 * 100);
    }

    pthread_join(test_thread2, NULL);
    pthread_join(test_thread1, NULL);
    return NULL;
}

int _agcts_posix_pthread()
{
    int ret = _AGCTS_FAIL;

    run_pthread = true;
    _agcts_thread_construct(&test_thread0, _agcts_thread_loop0, "test_thread0");
    sleep(1);
    run_pthread = false;
    pthread_join(test_thread0, NULL);

    if(test_num_mutex == 0)
        ret = _AGCTS_SUCCESS;

    return ret;
}

AGCTS_TEST_CASE agcts_posix_test_cases[] = {
    {(char *)"posix string API", _agcts_posix_string},
    {(char *)"posix memory, io API", _agcts_posix_memory_io},
    {(char *)"posix time API", _agcts_posix_time},
    {(char *)"posix unistd API", _agcts_posix_unistd},
    {(char *)"posix stdlib API", _agcts_posix_stdlib},
    {(char *)"posix pthread API, c++ mutex", _agcts_posix_pthread},
};

void agcts_posix_test()
{
    _AGCTS_TEST_BEGIN

    int i;
    for (i = 0; i < sizeof(agcts_posix_test_cases) / sizeof(AGCTS_TEST_CASE); i++) {
        agcts_run_test(&agcts_posix_test_cases[i]);
    }

    _AGCTS_TEST_END
}

