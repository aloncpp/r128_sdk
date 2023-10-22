#ifndef __AG_WIFI_LOG_H__
#define __AG_WIFI_LOG_H__

#include <stdio.h>
#define WFLOGE(fmt, args...)	\
    printf("[ERROR][%s] line:%d " fmt "\n", __func__, __LINE__, ##args)

#define WFLOGI(fmt, args...)	\
    printf("[INFO][%s] line:%d " fmt "\n", __func__, __LINE__, ##args)

#if 0
#define WFLOGD(fmt, args...)	\
    printf("[DEBUG][%s] line:%d " fmt "\n", __func__, __LINE__, ##args)
#else
#define WFLOGD(fmt, args...)
#endif
#endif
