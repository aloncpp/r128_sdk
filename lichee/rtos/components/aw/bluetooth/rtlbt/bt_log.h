/*
* btmanager - bt_log.h
*
* Copyright (c) 2018 Allwinner Technology. All Rights Reserved.
*
* Author         laumy    liumingyuan@allwinnertech.com
* verision       0.01
* Date           2018.3.26
*
* History:
*    1. date
*    2. Author
*    3. modification
*/

#ifndef __BT_LOG_H
#define __BT_LOG_H

#include <stdio.h>

#if __cplusplus
extern "C" {
#endif

#define BTMG_DEBUG(fmt,...) \
    printf("aw-bt[%s:%d] "fmt"\n", __FILE__, __LINE__, ##__VA_ARGS__)

#define BTMG_INFO(fmt,...) \
    printf("aw-bt[%s:%d] "fmt"\n", __FILE__, __LINE__, ##__VA_ARGS__)

#define BTMG_WARNG(fmt,...) \
    printf("aw-bt[%s:%d] "fmt"\n", __FILE__, __LINE__, ##__VA_ARGS__)

#define BTMG_ERROR(fmt,...) \
    printf("aw-bt[%s:%d] "fmt"\n", __FILE__, __LINE__, ##__VA_ARGS__)

#if __cplusplus
};  // extern "C"
#endif

#endif
