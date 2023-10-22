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

#ifndef _NET_SNTP_H_
#define _NET_SNTP_H_

#include <stdint.h>
#include "lwip/sockets.h"

#define SNTP_PORT                     123
#define SNTP_SUPPORT_MULTIPLE_SERVERS 1
#if SNTP_SUPPORT_MULTIPLE_SERVERS
#define SNTP_MAX_SERVERS              3
#endif
#define SNTP_RECV_TIMEOUT             3000 /* ms */
#define SNTP_RETRY_TIMEOUT            SNTP_RECV_TIMEOUT
#define SNTP_RETRY_TIMES              3

#define SNTP_SERVER_ADDRESS          "pool.ntp.org"

typedef struct {
	char *server_name;    /* remote server name, if this is not NULL, this will be preferred. */
	int recv_timeout;     /* the receive timeout from ntp server */
	uint8_t retry_times;  /* the retry times when receiver timeout */
} sntp_arg;

typedef struct {
        uint32_t sec;       /**< Seconds after the minute   - [0,59]  */
        uint32_t min;       /**< Minutes after the hour     - [0,59]  */
        uint32_t hour;      /**< Hours after the midnight   - [0,23]  */
        uint32_t day;       /**< Day of the month           - [1,31]  */
        uint32_t mon;       /**< Months                     - [1,12]  */
        uint32_t week;      /**< Days in a week             - [0,6]   */
        uint32_t year;      /**< Years                      - [0,127] */
} sntp_time;

int sntp_request(void *arg);
sntp_time *sntp_obtain_time(void);

int sntp_get_time(sntp_arg *arg, struct timeval *ntp_time);
#if SNTP_SUPPORT_MULTIPLE_SERVERS
int sntp_set_server(uint8_t idx, char *server_name);
#endif

#endif /* _NET_SNTP_H_ */

