/**
 * Copyright (C) 2018 Alibaba.inc, All rights reserved.
 *
 * @file:    agcts.h
 * @brief:
 * @author:  tanyan.lk@alibaba-inc.com
 * @date:    2019/7/10
 * @version: 1.0
 */
#ifndef __AGCTS_H__
#define __AGCTS_H__
// cpp head files
//#include <cstddef>
//#include <cstdint>
//#include <cstdio>
//#include <cstring>

/* FreeRTOS+POSIX includes. */

#include <list>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <vector>
#include <algorithm>

extern "C" {
// c head files
#include <assert.h>
#include <ctype.h>
//#include <execinfo.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <math.h>
#include <pthread.h>
//#include <semaphore.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

//#include "syslog.h"

// other libs
//#include "librws.h"
//#include <sntp.h>
}

#define _AGCTS_SUCCESS                  0
#define _AGCTS_FAIL                     -1

//#define _AGCTS_DEBUG

#ifdef _AGCTS_DEBUG
#define _AGCTS_LOGD(...)                printf(__VA_ARGS__)
#else
#define _AGCTS_LOGD(...)
#endif
#define _AGCTS_LOGI(...)                printf(__VA_ARGS__)

#define _AGCTS_TEST_BEGIN               _AGCTS_LOGI("######## _AGCTS_TEST: %s :BEGIN ########\n", __func__);
#define _AGCTS_TEST_END                 _AGCTS_LOGI("######## _AGCTS_TEST: %s :END ########\n", __func__);
//#define _AGCTS_API_SUPPORTED(...)       _AGCTS_LOGI("AliGenie Compatibility Test: %s supported\n", ##__VA_ARGS__);
//#define _AGCTS_API_NOT_SUPPORTED(...)   _AGCTS_LOGI("AliGenie Compatibility Test: %s not supported\n", ##__VA_ARGS__);

typedef struct {
    char *name;
    int (*func)(void);
} AGCTS_TEST_CASE;

typedef struct {
    AGCTS_TEST_CASE *head;
    AGCTS_TEST_CASE *next;
} AGCTS_TEST_SUITE;
//AGCTS_TEST_CASE *agcts_posix_test_cases;
//AGCTS_TEST_CASE *agcts_cpp_test_cases;

void agcts_run_test(AGCTS_TEST_CASE *test_case);
void agcts_posix_test();
void agcts_cpp_test();
void agcts_librws_test();
void agcts_audio_test();

#endif
