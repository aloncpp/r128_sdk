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

#include <string.h>

#include "kernel/os/os.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include "lwip/ip.h"

#include "smart_config.h"

#define g_debuglevel  ERROR

static int ack_successful(struct netif *nif, uint32_t random_num)
{
	uint8_t num[1];
	int tmp = 1;
	int ret = -1;
	int i;
	struct sockaddr_in addr;
	int socketfd;

	if (nif == NULL)
		return -1;

	num[0] = (uint8_t)random_num;

	if (!netif_is_up(nif) || ip_addr_isany(&nif->ip_addr) || !netif_is_link_up(nif))
		return -1;

	socketfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (socketfd < 0) {
		SC_DBG(ERROR, "%s() create socket fail\n", __func__);
		return -1;
	}

	memset(&addr, 0, sizeof(addr));

	addr.sin_port = htons(SC_ACK_UDP_PORT);
	addr.sin_family = AF_INET;
	if (inet_aton("239.0.0.1", &addr.sin_addr) < 0) {
		SC_DBG(ERROR, "%s() inet_aton error\n", __func__);
		goto out;
	}

	ret = setsockopt(socketfd, SOL_SOCKET, SO_BROADCAST,
	                 &tmp, sizeof(int));
	if (ret) {
		SC_DBG(ERROR, "%s() setsockopt error\n", __func__);
		goto out;
	}

	for (i = 0; i < 300; i ++) {
		ret = sendto(socketfd, num, 1, 0, (struct sockaddr *)&addr,
		                  sizeof(addr));
		if (ret == -1) {
			SC_DBG(ERROR, "%s() udp send error, %d\n", __func__, errno);
			goto out;
		}
		XR_OS_MSleep(2);
	}

	ret = 0;

out:
	closesocket(socketfd);

	return ret;
}

smartconfig_ack_status smart_config_ack_start(smartconfig_priv_t *priv, uint32_t random_num,
                           uint32_t timeout_ms)
{
	smartconfig_ack_status ret = SC_ACK_FAIL;
	uint32_t end_time;

	SC_DBG(INFO, "ack start\n");
	XR_OS_ThreadSuspendScheduler();
	priv->ack_run |= SC_TASK_RUN;
	XR_OS_ThreadResumeScheduler();
	end_time = XR_OS_JiffiesToMSecs(XR_OS_GetJiffies()) + timeout_ms;

	while (!(priv->ack_run & SC_TASK_STOP) &&
	       XR_OS_TimeBefore(XR_OS_JiffiesToMSecs(XR_OS_GetJiffies()), end_time)) {
		if (!ack_successful(priv->nif, random_num)) {
			ret = SC_ACK_SUCCESS;
			break;
		}

		XR_OS_MSleep(100);
	}
	if (ret != SC_ACK_SUCCESS) {
		if (priv->ack_run & SC_TASK_STOP) {
			ret = SC_ACK_STOP;
		} else if (XR_OS_TimeAfterEqual(XR_OS_JiffiesToMSecs(XR_OS_GetJiffies()), end_time)) {
			ret = SC_ACK_TIMEOUT;
		}
	}

	XR_OS_ThreadSuspendScheduler();
	priv->ack_run = 0;
	XR_OS_ThreadResumeScheduler();

	SC_DBG(INFO, "ack end\n");

	return ret;
}

int smart_config_ack_stop(smartconfig_priv_t *priv)
{
	XR_OS_ThreadSuspendScheduler();
	priv->ack_run |= SC_TASK_STOP;
	XR_OS_ThreadResumeScheduler();

	while (priv->ack_run & SC_TASK_RUN) {
		XR_OS_MSleep(10);
	}

	//todo: delete it
	XR_OS_ThreadSuspendScheduler();
	priv->ack_run = 0;
	XR_OS_ThreadResumeScheduler();

	return 0;
}
