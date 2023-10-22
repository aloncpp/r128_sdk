/*
 * Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of XRADIO TECHNOLOGY CO., LTD. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SYS_MONITOR_DEBUG_H_
#define _SYS_MONITOR_DEBUG_H_

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SYS_MONITOR_DBG_ON      0
#define SYS_MONITOR_WRN_ON      0
#define SYS_MONITOR_ERR_ON      1
#define SYS_MONITOR_ABORT_ON    0

#define SYS_MONITOR_SYSLOG      printf
#define SYS_MONITOR_ABORT()     do { } while (0)

#define SYS_MONITOR_LOG(flags, fmt, arg...) \
	do {                                    \
		if (flags)                          \
			SYS_MONITOR_SYSLOG(fmt, ##arg); \
	} while (0)

#define SYS_MONITOR_DBG(fmt, arg...) \
	SYS_MONITOR_LOG(SYS_MONITOR_DBG_ON, "[sys_mon] "fmt, ##arg)

#define SYS_MONITOR_WRN(fmt, arg...) \
	SYS_MONITOR_LOG(SYS_MONITOR_WRN_ON, "[sys_mon WRN] "fmt, ##arg)

#define SYS_MONITOR_ERR(fmt, arg...)                                      \
	do {                                                                  \
		SYS_MONITOR_LOG(SYS_MONITOR_ERR_ON, "[sys_mon ERR] %s():%d, "fmt, \
		                __func__, __LINE__, ##arg);                       \
		if (SYS_MONITOR_ABORT_ON)                                         \
			SYS_MONITOR_ABORT();                                          \
	} while (0)

#ifdef __cplusplus
}
#endif

#endif /* _SYS_MONITOR_DEBUG_H_ */
