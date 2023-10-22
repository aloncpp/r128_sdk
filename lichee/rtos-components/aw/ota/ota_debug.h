#ifndef _OTA_DEBUG_H_
#define _OTA_DEBUG_H_

#include <stdio.h>
#include "sys/xr_util.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OTA_DBG_ON		1
#define OTA_WRN_ON		1
#define OTA_ERR_ON		1
#define OTA_ABORT_ON		0

#define OTA_SYSLOG		printf
#define OTA_ABORT()		sys_abort()

#define OTA_LOG(flags, fmt, arg...)	\
	do {							\
		if (flags) 					\
			OTA_SYSLOG(fmt, ##arg);	\
	} while (0)

#define OTA_DBG(fmt, arg...)	OTA_LOG(OTA_DBG_ON, "[OTA] "fmt, ##arg)
#define OTA_WRN(fmt, arg...)	OTA_LOG(OTA_WRN_ON, "[OTA WRN] "fmt, ##arg)
#define OTA_ERR(fmt, arg...)							\
	do {												\
		OTA_LOG(OTA_ERR_ON, "[OTA ERR] %s():%d, "fmt,	\
				__func__, __LINE__, ##arg);				\
	    if (OTA_ABORT_ON)								\
			OTA_ABORT();								\
	} while (0)

#ifdef __cplusplus
}
#endif

#endif /* _OTA_DEBUG_H_ */
