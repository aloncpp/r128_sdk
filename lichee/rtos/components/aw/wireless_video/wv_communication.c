#include "wv_communication.h"
#include <lwip/sockets.h>
#include "wv_log.h"
#include <stdlib.h>
#include <hal_time.h>

#define TCP_SERVER_PORT 2022
int client_sockfd = 0;
int tcp_connected = 0;

static video_data *testdata;

static TaskHandle_t tcp_server_thread;
static TaskHandle_t tcp_send_test_thread;

int send_package(int fd, void *pkt, int size)
{
    void* buf = pkt;
    int count = size;

    while (count > 0)
    {
        int len = send(fd, buf, count, 0);
        if (len == -1)
        {
            close(fd);
            return -1;
        }
        else if (len == 0)
        {
            continue;
        }
        buf += len;
        count -= len;
    }

    return size;
}

static void tcp_server_func(void *args)
{
	int ret;
	int server_sockfd;
    int keepAlive = 1;
    int keepIdle = 5;
    int keepInterval = 1;
    int keepCount = 3;

start:
	LOG_D("tcp_server start");

	if ((server_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		LOG_E("socket error");
		return ;
	}

	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(TCP_SERVER_PORT);

	int option = 1;
	ret = setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&option, sizeof(option));
	if (ret < 0) {
		LOG_E("failed to setsockopt sock_fd");
		goto fail1;
	}

    if (bind(server_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        LOG_E("bind error");
        goto fail1;
    }

    if (listen(server_sockfd, 2) == -1) {
        LOG_E("listen error");
        goto fail1;
    };

	LOG_D("Waiting for incoming connections...");

    struct sockaddr_in client_addr;
    unsigned int client_addr_len = sizeof(struct sockaddr_in);

    if ((client_sockfd = accept(server_sockfd,(struct sockaddr *)&client_addr,&client_addr_len)) == -1) {
        LOG_E("accept error");
        goto fail1;
    }

    LOG_D("TCP connetced: [%s:%d]", inet_ntoa(client_addr.sin_addr), client_addr.sin_port);

	tcp_connected = 1;

    if (setsockopt(client_sockfd, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(keepAlive))) {
        LOG_E("Error setsockopt(SO_KEEPALIVE) failed");
		goto fail2;
    }
    if (setsockopt(client_sockfd, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(keepIdle))) {
         LOG_E("Error setsockopt(TCP_KEEPIDLE) failed");
         goto fail2;
    }
    if (setsockopt(client_sockfd, IPPROTO_TCP, TCP_KEEPINTVL, (void *)&keepInterval, sizeof(keepInterval))) {
        LOG_E("Error setsockopt(TCP_KEEPINTVL) failed");
        goto fail2;
    }
    if (setsockopt(client_sockfd, IPPROTO_TCP, TCP_KEEPCNT, (void *)&keepCount, sizeof(keepCount))) {
        LOG_E("Error setsockopt(TCP_KEEPCNT) failed");
        goto fail2;
    }

	while(1) {
		int pkt_len;
		char recv_buf[1024];
		pkt_len = recv(client_sockfd, recv_buf ,sizeof(recv_buf), 0);
		recv_buf[pkt_len] = '\0';

		if (pkt_len < 0) {
			LOG_D("Recv fail, tcp may disconnect!");
			break;
		}

		if (pkt_len == 0) {
			/* 连接被远端关闭 */
			LOG_E("TCP Disconnect!");
			break;
		}

		LOG_D("TCP RECEIVER:[%d bytes]:", pkt_len);
		LOG_D("%s", recv_buf);
	}

    /* 关闭套接字 */
    close(client_sockfd);
    close(server_sockfd);
	tcp_connected =  0 ;
	goto start;

fail2:
	close(client_sockfd);

fail1:
	close(server_sockfd);
	tcp_connected =  0 ;
    return;
}

int tcp_server_start(void)
{
	xTaskCreate(tcp_server_func, "wv_tcps_thread",
				4096, NULL, 3, &tcp_server_thread);

	return 0;
}
