#ifndef __OS_NET_UTILS_H__
#define __OS_NET_UTILS_H__
#include <stdio.h>
#include <stdlib.h>

typedef enum {
    OS_NET_STATUS_OK = 0,
    OS_NET_STATUS_FAILED,
    OS_NET_STATUS_NOT_READY,
    OS_NET_STATUS_BUSY,
    OS_NET_STATUS_PARAM_INVALID,
    OS_NET_STATUS_NOMEM,
} os_net_status_t;

#define OS_NET_PRINTF printf

#define OS_NET_OS_LOG(fmt, arg...)                                                                 \
    do {                                                                                           \
        OS_NET_PRINTF(fmt "\n", ##arg);                                                            \
    } while (0)

#define OS_NET_INFO(fmt, arg...) OS_NET_OS_LOG("[ACT I][%s,%d]" fmt, __func__, __LINE__, ##arg)
#define OS_NET_DEBUG(fmt, arg...) OS_NET_OS_LOG("[ACT D][%s,%d]" fmt, __func__, __LINE__, ##arg)
#define OS_NET_ERROR(fmt, arg...) OS_NET_OS_LOG("[ACT E][%s,%d]" fmt, __func__, __LINE__, ##arg)
#define OS_NET_WARN(fmt, arg...) OS_NET_OS_LOG("[ACT W][%s,%d]" fmt, __func__, __LINE__, ##arg)
#define OS_NET_DUMP(fmt, arg...) //OS_NET_OS_LOG("[ACT DUMP][%s,%d]"fmt,__func__,__LINE__,##arg)
#define OS_NET_EXCESSIVE(fmt, arg...) //OS_NET_OS_LOG("[ACT EX][%s,%d]"fmt,__func__,__LINE__,##arg)

#define OS_NET_TASK_POST_BLOCKING (0xffffffffU)
#define OS_NET_TASK_POST_NO_BLOCKING (0)
#define OS_NET_WAIT_FOREVER (0xffffffffU)
#endif
