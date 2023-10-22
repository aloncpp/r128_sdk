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
#include <stdlib.h>
#include <getopt.h>
#include <stdio.h>

#include "lwip/sockets.h"
#include "lwip/netif.h"

#include "iperf.h"
#include "iperf_debug.h"
#include "libc/errno.h"


#define iperf_thread_exit(thread)   XR_OS_ThreadDelete(thread)
#define iperf_errno                 XR_OS_GetErrno()
#define iperf_msleep(msec)          XR_OS_MSleep(msec)

#define IPERF_PORT                  5001

#define IPERF_BUF_SIZE              (1500)
#define IPERF_UDP_SEND_DATA_LEN     (1470)	// UDP: 1470 + 8  + 20 = 1498
#define IPERF_TCP_SEND_DATA_LEN     (1460)	// TCP: 1460 + 20 + 20 = 1500

#ifdef CONFIG_ARCH_RISCV_RV64
#define IPERF_THREAD_STACK_SIZE     (3 * 1024)
#else
#define IPERF_THREAD_STACK_SIZE     (2 * 1024)
#endif

iperf_arg *g_iperf_arg_handle[IPERF_ARG_HANDLE_MAX] = {NULL};

// in 1ms
#define IPERF_TIME_PER_SEC         1000
#define IPERF_TIME()               XR_OS_GetTicks()
#define IPERF_SEC_2_INTERVAL(sec)  ((sec) * IPERF_TIME_PER_SEC)

#define IPERF_SELECT_TIMEOUT       100

typedef struct {
	uint32_t run_time;
	uint32_t run_beg_tm;
	uint32_t run_end_tm;
	uint32_t beg_tm;
	uint32_t end_tm;
	uint32_t cur_tm;
#if IPERF_OPT_BANDWIDTH
	uint32_t send_tm;
	uint32_t run_tm;
#endif
	uint32_t data_cnt;
	uint64_t data_total_cnt;
}iperf_state;

static void iperf_speed_log(iperf_arg *arg, uint64_t bytes, uint32_t time,
                            int8_t is_end)
{
	uint64_t speed;
	uint32_t integer_part, decimal_part;
	char str[16];

	if (arg->flags & IPERF_FLAG_FORMAT) {
		/* KBytes/sec */
		speed = bytes * IPERF_TIME_PER_SEC * 100 / 1024 / time;
	} else {
		/* Mbits/sec */
		speed = bytes * 8 * IPERF_TIME_PER_SEC * 100 / 1000 / 1000 / time;
	}

	integer_part = speed / 100;
	decimal_part = speed % 100;

	if (integer_part >= 100) {
		snprintf(str, sizeof(str), "%u", integer_part);
	} else if (integer_part >= 10) {
		snprintf(str, sizeof(str), "%u.%u", integer_part, decimal_part / 10);
	} else {
		snprintf(str, sizeof(str), "%u.%02u", integer_part, decimal_part);
	}

	IPERF_LOG(1, "[%d] %s%s %s\n", arg->handle, is_end ?  "TEST END: " : "",
	          str, (arg->flags & IPERF_FLAG_FORMAT) ? "KB/s" : "Mb/s");
}


static __inline void iperf_udp_seq_set(uint32_t *p, uint32_t seq)
{
	*p = htonl(seq);
}

static int iperf_sock_create(int sock_type, short local_port, int do_bind)
{
	int local_sock;
	struct sockaddr_in local_addr;
	int ret, tmp;

	local_sock = socket(AF_INET, sock_type, 0);
	if (local_sock < 0) {
		IPERF_ERR("socket() return %d\n", local_sock);
		return local_sock;
	}

	if (!do_bind)
		return local_sock;

	tmp = 1;
	ret = setsockopt(local_sock, SOL_SOCKET, SO_REUSEADDR, &tmp, sizeof(int));
	if (ret != 0) {
		IPERF_ERR("setsockopt(SO_REUSEADDR) failed, err %d\n", iperf_errno);
		closesocket(local_sock);
		return -1;
	}

	if (sock_type == SOCK_STREAM) {
		tmp = 1;
		ret = setsockopt(local_sock, SOL_SOCKET, SO_KEEPALIVE, &tmp, sizeof(int));
		if (ret != 0) {
			IPERF_ERR("setsockopt(SO_KEEPALIVE) failed, err %d\n", iperf_errno);
			closesocket(local_sock);
			return -1;
		}
	}

	/* bind socket to port */
	memset(&local_addr, 0, sizeof(struct sockaddr_in));
	local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	local_addr.sin_port = htons(local_port);
	local_addr.sin_family = AF_INET;
	ret = bind(local_sock, (struct sockaddr *)&local_addr, sizeof(struct sockaddr_in));
	if (ret < 0) {
		IPERF_ERR("Failed to bind socket %d, err %d\n", local_sock, iperf_errno);
		closesocket(local_sock);
		return -1;
	}
	return local_sock;
}

static int iperf_set_sock_opt(int sock, iperf_arg *idata)
{
	int ret;

#if IPERF_OPT_TOS
    /* set IP TOS (type-of-service) field */
    if (idata->tos > 0) {
		int tos = idata->tos;
		ret = setsockopt(sock, IPPROTO_IP, IP_TOS, (void *)&tos, sizeof(tos));
		if (ret != 0) {
			IPERF_ERR("setsockopt(IP_TOS) failed, err %d\n", iperf_errno);
			return -1;
		}
    }
#endif
	return 0;
}

static uint8_t *iperf_buf_new(uint32_t size)
{
	uint32_t i;
	uint8_t *data_buf = malloc(size);
	if (data_buf == NULL) {
		IPERF_ERR("malloc() failed!\n");
	} else {
		for (i = 0; i < size; ++i) {
			data_buf[i] = '0' + i % 10;
		}
	}
	return data_buf;
}

static __inline int iperf_loop_init(iperf_state *state, iperf_arg *idata)
{
	state->run_time = IPERF_SEC_2_INTERVAL(idata->run_time);
	state->run_beg_tm = IPERF_TIME();
	state->run_end_tm = state->run_beg_tm + state->run_time;
	state->beg_tm = state->run_beg_tm;
	state->end_tm = state->beg_tm + IPERF_SEC_2_INTERVAL(idata->interval);
	state->data_cnt = 0;
	state->data_total_cnt = 0;
	return 0;
}

static __inline void iperf_calc_speed(iperf_state *state, iperf_arg *idata, int32_t data_len)
{
	state->data_cnt += data_len;
	state->data_total_cnt += data_len;
	state->cur_tm = IPERF_TIME();
	if (state->cur_tm > state->end_tm) {
		iperf_speed_log(idata, state->data_cnt, state->cur_tm - state->beg_tm, 0);
		state->data_cnt = 0;
		state->beg_tm = IPERF_TIME();
		state->end_tm = state->beg_tm + IPERF_SEC_2_INTERVAL(idata->interval);
	}
	if (idata->mode_time) {
		if (state->run_time && state->cur_tm > state->run_end_tm) {
			idata->flags |= IPERF_FLAG_STOP;
		}
	} else {
		if (idata->amount > data_len) {
			idata->amount -= data_len;
		} else {
			idata->amount = 0;
			idata->flags |= IPERF_FLAG_STOP;
		}
	}
	if (idata->flags & IPERF_FLAG_STOP) {
		iperf_speed_log(idata, state->data_total_cnt, state->cur_tm - state->run_beg_tm, 1);
	}
}

static __inline void iperf_calc_speed_fin(iperf_state *state, iperf_arg *idata, int32_t data_len)
{
	state->cur_tm = IPERF_TIME();
	state->data_total_cnt += data_len;
	iperf_speed_log(idata, state->data_total_cnt, state->cur_tm - state->run_beg_tm, 1);
}

#if IPERF_OPT_BANDWIDTH
static __inline void iperf_adjust_bandwidth(iperf_state *state, iperf_arg *idata)
{
	if (idata->bandwidth != 0) {
		state->run_tm = IPERF_TIME() - state->run_beg_tm;
		state->send_tm = state->data_total_cnt * 8 * 1000 / idata->bandwidth;
		if (state->send_tm > state->run_tm) {
			iperf_msleep(state->send_tm - state->run_tm);
		}
	}
}
#endif

/* -------------------------------------------------------------------
 * Send a datagram on the socket. The datagram's contents should signify
 * a FIN to the application. Keep re-transmitting until an
 * acknowledgement datagram is received.
 * ------------------------------------------------------------------- */

void write_UDP_FIN(int local_sock,
						struct sockaddr *remote_addr,
						uint8_t *data_buf)
{
	int rc;
	fd_set readSet;
	struct timeval timeout;
	struct UDP_datagram* mBuf_UDP = (struct UDP_datagram*) data_buf;

	int count = 0;
	int packetid;
	while (count < 10) {
		 count++;

		 // write data
		 sendto(local_sock, data_buf, IPERF_UDP_SEND_DATA_LEN, 0,
		                  remote_addr, sizeof(*remote_addr));
		// decrement the packet count
		packetid = ntohl(mBuf_UDP->id);
		packetid--;
		mBuf_UDP->id = htonl(packetid);

		// wait until the socket is readable, or our timeout expires
		FD_ZERO(&readSet);
		FD_SET(local_sock, &readSet);
		timeout.tv_sec  = 0;
		timeout.tv_usec = 250000; // quarter second, 250 ms

		rc = select(local_sock + 1, &readSet, NULL, NULL, &timeout);
		if (rc == 0) {
		 // select timed out
		 continue;
		} else {
			// socket ready to read
			rc = recvfrom(local_sock, data_buf, IPERF_BUF_SIZE, 0, NULL, 0);
			if (rc < 0)
				break;
			return;
		}
	}

	IPERF_WARN("did not receive ack of last datagram after %d tries.\n", count);
}

/* -------------------------------------------------------------------
 * Send an AckFIN (a datagram acknowledging a FIN) on the socket,
 * then select on the socket for some time. If additional datagrams
 * come in, probably our AckFIN was lost and they are re-transmitted
 * termination datagrams, so re-transmit our AckFIN.
 * ------------------------------------------------------------------- */

void write_UDP_AckFIN(int local_sock,
							struct sockaddr *remote_addr,
							uint8_t *data_buf) {
	int rc;
	fd_set readSet;
	FD_ZERO(&readSet);
	struct timeval timeout;
	int count = 0;

	while ( count < 10 ) {
	    count++;

	    // write data
		sendto(local_sock, data_buf, IPERF_UDP_SEND_DATA_LEN, 0,
		                  remote_addr, sizeof(*remote_addr));

	    // wait until the socket is readable, or our timeout expires
	    FD_SET(local_sock, &readSet);
	    timeout.tv_sec  = 1;
	    timeout.tv_usec = 0;

	    rc = select(local_sock + 1, &readSet, NULL, NULL, &timeout);
	    if ( rc == 0 ) {
	        // select timed out
	        return;
	    } else {
	        // socket ready to read
	        rc = recvfrom(local_sock, data_buf, IPERF_BUF_SIZE, 0, NULL, 0);
	        if ( rc <= 0 ) {
	            // Connection closed or errored
	            // Stop using it.
	            return;
	        }
	    }
	}

	IPERF_WARN("ack of last datagram failed after %d tries.\n", count);
}

void iperf_udp_send_task(void *arg)
{
	int local_sock = -1;
	struct sockaddr_in remote_addr;
	iperf_arg *idata = (iperf_arg *)arg;
	iperf_state istate;
	uint32_t port = idata->port;
	uint8_t *data_buf = NULL;
	int32_t data_len;
	int packetID = 0;
	struct UDP_datagram* mBuf_UDP;

	local_sock = iperf_sock_create(SOCK_DGRAM, 0, 0);
	if (local_sock < 0) {
		IPERF_ERR("socket() return %d\n", local_sock);
		goto socket_error;
	}
	iperf_set_sock_opt(local_sock, idata);

	data_buf = iperf_buf_new(IPERF_BUF_SIZE);
	if (data_buf == NULL) {
		IPERF_ERR("malloc() failed!\n");
		goto socket_error;
	}

	memset(&remote_addr, 0, sizeof(struct sockaddr_in));
	inet_aton(idata->remote_ip, &remote_addr.sin_addr);
	remote_addr.sin_port = htons(port);
	remote_addr.sin_family = AF_INET;

	IPERF_DBG("iperf: UDP send to %s:%d\n", idata->remote_ip, port);

	mBuf_UDP = (struct UDP_datagram*) data_buf;
	mBuf_UDP->id = htonl(packetID);
	iperf_loop_init(&istate, idata);

	while (!(idata->flags & IPERF_FLAG_STOP)) {
#if IPERF_OPT_BANDWIDTH
		iperf_adjust_bandwidth(&istate, idata);
#endif
		data_len = sendto(local_sock, data_buf, IPERF_UDP_SEND_DATA_LEN, 0,
		                  (struct sockaddr *)&remote_addr, sizeof(remote_addr));
		if (data_len > 0) {
			packetID++;
			mBuf_UDP->id = htonl(packetID);
		} else {
			data_len = 0;
		}
		iperf_calc_speed(&istate, idata, data_len);
	}

	mBuf_UDP->id = htonl(-packetID);
	write_UDP_FIN(local_sock, (struct sockaddr *)&remote_addr, data_buf);

socket_error:
	if (data_buf)
		free(data_buf);
	if (local_sock >= 0)
		closesocket(local_sock);

	iperf_handle_free(idata->handle);
	IPERF_DBG("%s() [%d] exit!\n", __func__, idata->handle);
	iperf_thread_exit(NULL);
}

void iperf_udp_recv_task(void *arg)
{
	int local_sock = -1;
	struct sockaddr_in remote_addr;
	socklen_t addr_len = sizeof(remote_addr);
	iperf_arg *idata = (iperf_arg *)arg;
	iperf_state istate;
	uint32_t port = idata->port;
	uint8_t *data_buf = NULL;
	int32_t data_len;
	int timeout = IPERF_SELECT_TIMEOUT; //ms
	int packetID = 0;
	struct UDP_datagram* mBuf_UDP;
	uint8_t wait = 1;

	if (port == 0) {
		port = IPERF_PORT;
	}
	local_sock = iperf_sock_create(SOCK_DGRAM, port, 1);
	if (local_sock < 0) {
		IPERF_ERR("socket() return %d\n", local_sock);
		goto socket_error;
	}
	iperf_set_sock_opt(local_sock, idata);

	data_buf = iperf_buf_new(IPERF_BUF_SIZE);
	if (data_buf == NULL) {
		IPERF_ERR("malloc() failed!\n");
		goto socket_error;
	}

	IPERF_DBG("iperf: UDP recv at port %d\n", port);

#if IPERF_OPT_NUM
	idata->mode_time = 1;
#endif

	if (setsockopt(local_sock, SOL_SOCKET, SO_RCVTIMEO,
					(char *)&timeout, sizeof(timeout)) < 0) {
		IPERF_ERR("set socket option err %d\n", iperf_errno);
		goto socket_error;
	}
	mBuf_UDP = (struct UDP_datagram*)data_buf;
	iperf_loop_init(&istate, idata);

	while (!(idata->flags & IPERF_FLAG_STOP)) {
		data_len = recvfrom(local_sock, data_buf, IPERF_BUF_SIZE, 0,
							(struct sockaddr *)&remote_addr, &addr_len);
		if (data_len > 0) {
			if (wait) {
				wait = 0;
				iperf_loop_init(&istate, idata); // reinitialization time
				IPERF_DBG("iperf udp_recv_task reinit time and reclocking\n");
			}
		} else {
			data_len = 0;
		}

		packetID = ntohl(mBuf_UDP->id);
		if (packetID < 0) {
			iperf_calc_speed_fin(&istate, idata, data_len);
			IPERF_DBG("iperf udp_recv_task receive a FIN datagram\n");
			write_UDP_AckFIN(local_sock, (struct sockaddr *)&remote_addr,
								data_buf);
			break;
		}
		iperf_calc_speed(&istate, idata, data_len);
	}

socket_error:
	if (data_buf)
		free(data_buf);
	if (local_sock >= 0)
		closesocket(local_sock);

	iperf_handle_free(idata->handle);
	IPERF_DBG("%s() [%d] exit!\n", __func__, idata->handle);
	iperf_thread_exit(NULL);
}

void iperf_tcp_send_task(void *arg)
{
	int local_sock = -1;
	struct sockaddr_in remote_addr;
	iperf_arg *idata = (iperf_arg *)arg;
	iperf_state istate;
	uint32_t port = idata->port;
	uint8_t *data_buf = NULL;
	int32_t data_len;
	int ret;

	fd_set fdw;
	struct timeval tv;

//	OS_MSleep(1); // enable task create function return
	local_sock = iperf_sock_create(SOCK_STREAM, 0, 0);
	if (local_sock < 0) {
		IPERF_ERR("socket() return %d\n", local_sock);
		goto socket_error;
	}
	iperf_set_sock_opt(local_sock, idata);

	data_buf = iperf_buf_new(IPERF_BUF_SIZE);
	if (data_buf == NULL) {
		IPERF_ERR("malloc() failed!\n");
		goto socket_error;
	}

	uint32_t i = 1;
	ret = ioctlsocket(local_sock, FIONBIO, &i);
	if (ret < 0) {
		IPERF_ERR("ioctlsocket() return %d\n", ret);
		goto socket_error;
	}

	memset(&remote_addr, 0, sizeof(struct sockaddr_in));
	inet_aton(idata->remote_ip, &remote_addr.sin_addr);
	remote_addr.sin_port = htons(port);
	remote_addr.sin_family = AF_INET;

	ret = connect(local_sock, (struct sockaddr *)&remote_addr, sizeof(remote_addr));
	if (ret != 0) {
		if (errno != EINPROGRESS) {
			IPERF_ERR("connect() return %d\n", ret);
			goto socket_error;
		} else {
			while (!(idata->flags & IPERF_FLAG_STOP)) {

				FD_ZERO(&fdw);
				FD_SET(local_sock, &fdw);
				tv.tv_sec = 0;
				tv.tv_usec = 100000; //100ms

				ret = select(local_sock + 1, NULL, &fdw, NULL, &tv);
				if (ret > 0) {
					if (FD_ISSET(local_sock, &fdw)) {
						break;
					}
				} else if (ret < 0) {
					IPERF_ERR("socket select err! errno:%d\n", errno);
					goto socket_error;
				} else if (ret == 0) {
					continue;
				}
			}
			if (idata->flags & IPERF_FLAG_STOP)
				goto socket_error;
		}
	}

	IPERF_DBG("iperf: TCP send to %s:%d\n", idata->remote_ip, port);

	iperf_loop_init(&istate, idata);

	while (!(idata->flags & IPERF_FLAG_STOP)) {
		FD_ZERO(&fdw);
		FD_SET(local_sock, &fdw);
		tv.tv_sec = 0;
		tv.tv_usec = 100000; //100ms

		ret = select(local_sock + 1, NULL, &fdw, NULL, &tv);
		if (ret > 0) {
			if (FD_ISSET(local_sock, &fdw)) {
				data_len = send(local_sock, data_buf, IPERF_TCP_SEND_DATA_LEN, 0);
				if (data_len <= 0) {
					IPERF_WARN("send return %d, err %d\n", data_len, iperf_errno);
					break;
				}
				iperf_calc_speed(&istate, idata, data_len);
			}
		} else if (ret < 0) {
			IPERF_ERR("socket select err! errno:%d\n", errno);
			goto socket_error;
		} else if (ret == 0) {
			continue;
		}
	}

socket_error:
	if (data_buf)
		free(data_buf);
	if (local_sock >= 0)
		closesocket(local_sock);

	iperf_handle_free(idata->handle);
	IPERF_DBG("%s() [%d] exit!\n", __func__, idata->handle);
	iperf_thread_exit(NULL);
}

void iperf_tcp_recv_task(void *arg)
{
	int local_sock = -1;
	int remote_sock = -1;
	struct sockaddr_in remote_addr;
	iperf_arg *idata = (iperf_arg *)arg;
	iperf_state istate;
	uint32_t port = idata->port;
	uint8_t *data_buf = NULL;
	int32_t data_len;
	int ret;
	int timeout = IPERF_SELECT_TIMEOUT; //ms

	local_sock = iperf_sock_create(SOCK_STREAM, port, 1);
	if (local_sock < 0) {
		IPERF_ERR("socket() return %d\n", local_sock);
		goto socket_error;
	}
	iperf_set_sock_opt(local_sock, idata);

	data_buf = iperf_buf_new(IPERF_BUF_SIZE);
	if (data_buf == NULL) {
		IPERF_ERR("malloc() failed!\n");
		goto socket_error;
	}

	IPERF_DBG("iperf: TCP listen at port %d\n", port);

	ret = listen(local_sock, 5);
	if (ret < 0) {
		IPERF_ERR("Failed to listen socket %d, err %d\n", local_sock, iperf_errno);
		goto socket_error;
	}

	ret = sizeof(struct sockaddr_in);

	/* Set the recv timeout to set the accept timeout */
	if (setsockopt(local_sock, SOL_SOCKET, SO_RCVTIMEO,
					(char *)&timeout, sizeof(timeout)) < 0) {
		IPERF_ERR("set socket option err %d\n", iperf_errno);
		goto socket_error;
	}

	while (!(idata->flags & IPERF_FLAG_STOP)) {
		remote_sock = accept(local_sock, (struct sockaddr *)&remote_addr,
							(socklen_t *)&ret);
		if (remote_sock >= 0) {
			iperf_set_sock_opt(remote_sock, idata);
			break;
		}
	}

	if (!(idata->flags & IPERF_FLAG_STOP)) {
		IPERF_DBG("iperf: client from %s:%d\n", inet_ntoa(remote_addr.sin_addr),
					ntohs(remote_addr.sin_port));
	}

#if IPERF_OPT_NUM
	idata->mode_time = 1;
#endif

	iperf_loop_init(&istate, idata);

	while (!(idata->flags & IPERF_FLAG_STOP)) {
		data_len = recv(remote_sock, data_buf, IPERF_BUF_SIZE, 0);
		if (data_len <= 0) {
			iperf_calc_speed_fin(&istate, idata, 0); /* data len set to '0' when calc speed */
			IPERF_WARN("recv return %d, err %d\n", data_len, iperf_errno);
			break;
		}
		iperf_calc_speed(&istate, idata, data_len);
	}

socket_error:
	if (remote_sock)
		closesocket(remote_sock);
	if (data_buf)
		free(data_buf);
	if (local_sock >= 0)
		closesocket(local_sock);

	iperf_handle_free(idata->handle);
	IPERF_DBG("%s() [%d] exit!\n", __func__, idata->handle);
	iperf_thread_exit(NULL);
}

static const TaskFunction_t iperf_thread_entry[IPERF_MODE_NUM] = {
	iperf_udp_send_task,
	iperf_udp_recv_task,
	iperf_tcp_send_task,
	iperf_tcp_recv_task,
}; /* index by enum IPERF_MODE */

int iperf_handle_start(struct netif *nif, int handle)
{
	if (nif == NULL || handle < 0 || handle >= IPERF_ARG_HANDLE_MAX)
		return -1;

	if (XR_OS_ThreadIsValid(&(g_iperf_arg_handle[handle]->iperf_thread))) {
		IPERF_ERR("iperf task is running\n");
		return -1;
	}

	g_iperf_arg_handle[handle]->flags &= ~IPERF_FLAG_STOP; /* clean stop flag */
	if (XR_OS_ThreadCreate(&(g_iperf_arg_handle[handle]->iperf_thread),
	                       "iperf",
	                       iperf_thread_entry[g_iperf_arg_handle[handle]->mode],
	                       (void *)g_iperf_arg_handle[handle],
	                       XR_OS_THREAD_PRIO_APP,
	                       IPERF_THREAD_STACK_SIZE) != XR_OS_OK) {
		IPERF_ERR("iperf task create failed\n");
		return -1;
	}
	return 0;
}

int iperf_handle_stop(int handle)
{
	if ((handle < 0) || (handle >= IPERF_ARG_HANDLE_MAX))
		return -1;

	if (g_iperf_arg_handle[handle] != NULL) {
		if (XR_OS_ThreadIsValid(&(g_iperf_arg_handle[handle]->iperf_thread))) {
			IPERF_TRC("(%s:%d) stop handler:%d, mode:%d, port:%d\n", __func__, __LINE__,
			          handle, g_iperf_arg_handle[handle]->mode, g_iperf_arg_handle[handle]->port);
			g_iperf_arg_handle[handle]->flags |= IPERF_FLAG_STOP;
		} else {
			IPERF_ERR("iperf task %d not exist.\n", handle);
			return -1;
		}
	}

	return 0;
}

int iperf_start(struct netif *nif, int handle)
{
	return iperf_handle_start(nif, handle);
}

int iperf_stop(char* arg)
{
	int i;
	int handle = 0;

	if (strcmp("a", arg) == 0) {
		for (i = 0; i < IPERF_ARG_HANDLE_MAX; i++) {
			if (g_iperf_arg_handle[i] != NULL) {
				IPERF_TRC("handle:%d stop\n", i);
				iperf_handle_stop(i);
			}
		}
		return 0;
	}

	handle = atoi(arg);
	if (handle < 0 || handle >= IPERF_ARG_HANDLE_MAX ||
		(handle == 0 && strcmp("0", arg))) {
		IPERF_DBG("handle '%s' input err\n", arg);
		return -1;
	}
	return iperf_handle_stop(handle);
}

int iperf_handle_new(iperf_arg* arg)
{
	int i = 0, pos = 0, find = 0;
	if (arg == NULL)
		return -1;

	for (i = 0; i < IPERF_ARG_HANDLE_MAX; i++) {
		if (g_iperf_arg_handle[i] == NULL) {
			if (!find) {
				find = 1;
				pos = i;
				if ((arg->mode & IPERF_MODE_TCP_SEND) ||
					(arg->mode & IPERF_MODE_UDP_SEND))
					break;
			}
		} else {
			if ((arg->mode & IPERF_MODE_TCP_RECV) ||
				(arg->mode & IPERF_MODE_UDP_RECV)) {
				if(g_iperf_arg_handle[i]->port == arg->port) {
					IPERF_DBG("the handle:%d, mode:%d, port %d has been used\n",
					          i, g_iperf_arg_handle[i]->mode, arg->port);
					return -1;
				}
			}
		}
	}

	if (find) {
		g_iperf_arg_handle[pos] = malloc(sizeof(iperf_arg));
		if (g_iperf_arg_handle[pos] == NULL) {
			IPERF_ERR("iperf malloc faild\n");
			return -1;
		}
		memcpy(g_iperf_arg_handle[pos], arg, sizeof(iperf_arg));
		g_iperf_arg_handle[pos]->handle = pos;
		IPERF_TRC("(%s:%d)new handle:%d, mode:%d, port:%d\n", __func__, __LINE__,
		          pos, g_iperf_arg_handle[pos]->mode, g_iperf_arg_handle[pos]->port);
		return pos;
	}

	IPERF_DBG("cannot new more handle, buf is full\n");
	return -1;
}

int iperf_handle_free(int handle)
{
	if (handle < 0 || handle >= IPERF_ARG_HANDLE_MAX)
		return -1;

	IPERF_TRC("(%s:%d)try free handle:%d\n", __func__, __LINE__, handle);

	if (g_iperf_arg_handle[handle] != NULL) {
		IPERF_TRC("(%s:%d) free handler:%d, mode:%d, port:%d\n", __func__, __LINE__, handle,
		          g_iperf_arg_handle[handle]->mode, g_iperf_arg_handle[handle]->port);
		free(g_iperf_arg_handle[handle]);
		g_iperf_arg_handle[handle] = NULL;
	}
	return 0;
}

static const char *iperf_mode_str[IPERF_MODE_NUM] = {
	"udp-send",
	"udp-recv",
	"tcp-send",
	"tcp-recv",
};

int iperf_show_list(void)
{
	int i = 0;
	int run_num = 0;

	for (i = 0; i < IPERF_ARG_HANDLE_MAX; i++) {
		if (g_iperf_arg_handle[i] != NULL) {
			run_num++;

			IPERF_LOG(1, "\nhandle    = %d\n", i);
			IPERF_LOG(1, "mode      = %s\n",
						iperf_mode_str[g_iperf_arg_handle[i]->mode]);
			IPERF_LOG(1, "remote ip = %s\n", g_iperf_arg_handle[i]->remote_ip);
			IPERF_LOG(1, "port      = %u\n", g_iperf_arg_handle[i]->port);
			IPERF_LOG(1, "run time  = %u\n", g_iperf_arg_handle[i]->run_time);
			IPERF_LOG(1, "interval  = %u\n\n", g_iperf_arg_handle[i]->interval);
		}
	}
	if (!run_num)
		return -1;
	return 0;
}

int iperf_parse_argv(int argc, char *argv[])
{
	iperf_arg iperf_arg_t;
	uint32_t port;
	int opt = 0;
	char *short_opts = "LusQ:c:f:p:t:i:b:n:S:";
	memset(&iperf_arg_t, 0, sizeof(iperf_arg_t));
#if IPERF_OPT_BANDWIDTH
	iperf_arg_t.bandwidth = 1000 * 1000; /* default to 1Mbits/sec */
#endif
#if IPERF_OPT_NUM
	iperf_arg_t.mode_time = 1;
#endif

	optind = 0; /* reset the index */
	//opterr = 0; /* close the "invalid option" warning */
	while ((opt = getopt(argc, argv, short_opts)) != -1) {
		//IPERF_DBG("opt=%c\n", opt);
		switch (opt) {
			case 'f':
				if (strcmp("m", optarg) == 0 )
					iperf_arg_t.flags &= ~IPERF_FLAG_FORMAT;
				else if (strcmp("K", optarg) == 0)
					iperf_arg_t.flags |= IPERF_FLAG_FORMAT;
				else {
					IPERF_ERR("invalid format arg '%s'\n", optarg);
					return -1;
				}
				break;
			case 'p':
				port = (uint32_t)atoi(optarg);
				if (port > 65535 || port == 0) {
					IPERF_ERR("invalid port arg '%s'\n", optarg);
					return -1;
				} else {
					iperf_arg_t.port = port;
					iperf_arg_t.flags |= IPERF_FLAG_PORT;
				}
				break;
			case 'u':
				iperf_arg_t.flags |= IPERF_FLAG_UDP;
				break;
			case 's':
				iperf_arg_t.flags |= IPERF_FLAG_SERVER;
				break;
			case 'c':
				if (inet_addr(optarg) == INADDR_NONE) {
					IPERF_ERR("invalid ip arg '%s'\n", optarg);
					return -1;
				} else {
					snprintf(iperf_arg_t.remote_ip, IPERF_ADDR_STRLEN_MAX, optarg);
					iperf_arg_t.flags |= IPERF_FLAG_CLINET;
				}
				break;
			case 'i':
				iperf_arg_t.interval = (uint32_t)atoi(optarg);
				break;
			case 't':
				iperf_arg_t.run_time = (uint32_t)atoi(optarg);
				break;
			case 'Q':
				iperf_stop(optarg);
				return -2;
			case 'L':
				if(iperf_show_list())
					IPERF_DBG("no task running\n");
				return -2;
#if IPERF_OPT_BANDWIDTH
			case 'b': {
				float value;
				char suffix = '\0';
				sscanf(optarg, "%f%c", &value, &suffix);
				if (suffix == 'm' || suffix == 'M') {
					value *= 1000 * 1000;
				} else if (suffix == 'k' || suffix == 'K') {
					value *= 1000;
				}
				iperf_arg_t.bandwidth = value;
				break;
			}
#endif
#if IPERF_OPT_NUM
			case 'n': {
				uint32_t value;
				char suffix = '\0';
				sscanf(optarg, "%u%c", &value, &suffix);
				iperf_arg_t.mode_time = 0;
				iperf_arg_t.amount = value;
				if (suffix == 'm' || suffix == 'M') {
					iperf_arg_t.amount *= 1024 * 1024;
				} else if (suffix == 'k' || suffix == 'K') {
					iperf_arg_t.amount *= 1024;
				}
				break;
			}
#endif
#if IPERF_OPT_TOS
			case 'S':
				iperf_arg_t.tos = (uint16_t)strtol(optarg, NULL, 0);
				break;
#endif
			default :
				return -1;
		}
	}

	/* set the default interval time 1 second */
	if (!iperf_arg_t.interval)
		iperf_arg_t.interval = 1;

	/* Cannot be client and server simultaneously */
	uint32_t flag = IPERF_FLAG_SERVER | IPERF_FLAG_CLINET;
	if ((iperf_arg_t.flags & flag) == 0 || (~iperf_arg_t.flags & flag) == 0) {
		IPERF_ERR("invalid iperf cmd\n");
		return -1;
	}

	if (iperf_arg_t.port == 0)
		iperf_arg_t.port = IPERF_PORT;

	if ((iperf_arg_t.flags & IPERF_FLAG_UDP) == IPERF_FLAG_UDP) {
		if ((iperf_arg_t.flags & IPERF_FLAG_SERVER) == IPERF_FLAG_SERVER)
			iperf_arg_t.mode = IPERF_MODE_UDP_RECV;
		else
			iperf_arg_t.mode = IPERF_MODE_UDP_SEND;
	} else {
		if ((iperf_arg_t.flags & IPERF_FLAG_SERVER) == IPERF_FLAG_SERVER)
			iperf_arg_t.mode = IPERF_MODE_TCP_RECV;
		else
			iperf_arg_t.mode = IPERF_MODE_TCP_SEND;
	}

	int handle = iperf_handle_new(&iperf_arg_t);
	return handle;
}
