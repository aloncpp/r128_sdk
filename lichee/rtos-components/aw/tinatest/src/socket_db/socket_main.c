#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "socket_main.h"

#include "tinatest.h"
#include "aw_types.h"
#include "init_entry.h"

#define MAIN_PORT  8887
#define HEART_PORT  8888
#define QUEUE   20
#define BUFFER_SIZE 1024

int conn_fd = -1;//tmp

int socket_listen(int port){
	int ret = -1;
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if(fd<0){
		ERROR("socket failed\n");
		return -1;
	}

	int optval = 1;
	ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	if(ret<0){
		ERROR("setsockopt failed\n");
		goto err;
	}

	struct sockaddr_in server_sockaddr;
	server_sockaddr.sin_family = AF_INET;
	server_sockaddr.sin_port = htons(port);
	server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	ret = bind(fd, (struct sockaddr *)&server_sockaddr, sizeof(server_sockaddr));
	if(ret<0){
		ERROR("bind socket failed\n");
		goto err;
	}

	ret = listen(fd, QUEUE);
	if(ret<0){
		ERROR("listen socket failed\n");
		goto err;
	}

	INFO("socket_listen(%d:%d) success!\n", fd, port);
	return fd;
err:
	close(fd);
	return ret;
}

static int socket_accept(int server_fd, int timeout_ms){
	if(timeout_ms==0)
		timeout_ms = 1;
	int exit = 0;
	int client_fd = -1;
	struct sockaddr_in client_addr;
	socklen_t length = sizeof(client_addr);
#if 1
	client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &length);
	if(client_fd<0){
		ERROR("accept(%d) failed\n", server_fd);
		goto err;
	}
#else
	struct timeval tv;
	fd_set fs;
	FD_ZERO(&fs);
	FD_SET(server_fd, &fs);
	while(1){
		if(timeout_ms>1000){
			tv.tv_sec = 1;
			tv.tv_usec = 0;
			timeout_ms -= 1000;
		}
		else if(timeout_ms>0){
			tv.tv_sec = 0;
			tv.tv_usec = timeout_ms * 1000;
			timeout_ms = 0;
		}
		else{
			ERROR("wait accept(%d) timeout\n", server_fd);
			goto err;
		}
		int ret = lwip_select(server_fd + 1, &fs, NULL, NULL, &tv);
		if(ret<0){
			if(EINTR==errno){
				DEBUG("wait accept(%d) again...\n", server_fd);
				continue;
			}
			else{
				ERROR("wait accept(%d) timeout\n", server_fd);
				goto err;
			}
		}
		else if(ret==0){
			DEBUG("wait accept(%d)...\n", server_fd);
			continue;
		}
		client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &length);
		if(client_fd<0){
			ERROR("accept(%d) failed\n", server_fd);
			goto err;
		}
		break;
	}
#endif
	INFO("socket_accept success! s:%d, c:%d\n", server_fd, client_fd);
	return client_fd;
err:
	return -1;
}

struct heartbeat_thread_data_t{
	volatile int *exit;
	volatile int *connected;
};

static void *heartbeat_thread(void *arg){
	struct heartbeat_thread_data_t *data = (struct heartbeat_thread_data_t *)arg;
	volatile int *exit = data->exit;
	volatile int *connected = data->connected;
	free(data);
	if(!exit || *exit)
		goto exit1;
	int heart_server_fd = socket_listen(HEART_PORT);
	if(heart_server_fd<0){
		ERROR("socket_listen heartbeat failed\n");
		goto exit1;
	}

	int heart_client_fd = socket_accept(heart_server_fd, 30000);//tmp
	if(heart_client_fd<0){
		ERROR("socket_accept heartbeat failed\n");
		goto exit2;
	}

	//keeping heartbeat
	int buffer[128];
	INFO("keeping heartbeat...\n");
	if(connected)
		*connected = 1;
	while(1){
		usleep(500*1000);
		if(!exit || *exit)
			break;
		memset(buffer,0,sizeof(buffer));

		int len = recv(heart_client_fd, buffer, sizeof(buffer),0);
		if(len<0 || (exit && *exit))
			break;
		DEBUG("heart: recv size:%d\n", len);

		len = send(heart_client_fd, buffer, len, 0);
		if(len<0 || (exit && *exit))
			break;
		DEBUG("heart: send size:%d\n", len);
	}
	DEBUG("stop heartbeat\n");
	close(heart_client_fd);
exit2:
	close(heart_server_fd);
exit1:
	free((void *)exit);
	INFO("stop heartbeat\n");
	return NULL;
}

int recv_from_socket(char *buf,int length)
{
	if(conn_fd<0){
		ERROR("conn_fd=%d!\n", conn_fd);
		usleep(1000*1000);
		return -1;
	}
	int len = recv(conn_fd, buf, length,0);//tmp

	return len;
}

int send_to_sockt(char *buf,int length)
{
	if(conn_fd<0){
		ERROR("conn_fd=%d!\n", conn_fd);
		usleep(1000*1000);
		return -1;
	}
	int len = send(conn_fd, buf, length, 0);//tmp

	return len;
}

struct socket_data_t{
	int server_fd;
	int client_fd;
};
//note: connect heartbeat port first
struct socket_data_t *socket_init(int timeout_ms, volatile int *exit){
	int ret = -1;
	struct socket_data_t *data = malloc(sizeof(struct socket_data_t));
	struct heartbeat_thread_data_t *tdata = malloc(sizeof(struct heartbeat_thread_data_t));
	volatile int *connected = malloc(sizeof(volatile int));
	if(!data || !tdata || !connected){
		goto err1;
	}
	*connected = 0;
	tdata->connected = connected;
	tdata->exit = exit;

	data->server_fd = socket_listen(MAIN_PORT);
	if (data->server_fd<0){
		ERROR("socket init fail\n");
		goto err2;
	}

	//create dragonmat heartbeat thread
	pthread_t tid;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	struct sched_param sched;
	sched.sched_priority = configTIMER_TASK_PRIORITY - 1;
	pthread_attr_setschedparam(&attr, &sched);
	pthread_attr_setstacksize(&attr, 32768);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	ret = pthread_create(&tid, &attr, heartbeat_thread, (void *)tdata);
	if(ret){
		ERROR("create dragonmat heartbeat thread failed\n");
		goto err3;
	}
	pthread_setname_np(tid, "dragonmat heart");
	tdata = NULL;//free by heartbeat_thread

	data->client_fd = socket_accept(data->server_fd, 30000);//tmp
	if(data->client_fd<0){
		ERROR("socket_accept failed\n");
		goto err4;
	}
	conn_fd = data->client_fd;//tmp
	while(!*connected)
		usleep(100*1000);
	DEBUG("socket_init success! s:%d, c:%d\n", data->server_fd, data->client_fd);
	return data;
err4:
	*exit = 1;
err3:
	close(data->server_fd);
err2:
	free(data);
err1:
	if(tdata)
		free(tdata);
	if(connected)
		free((void *)connected);
	return NULL;
}

int socket_exit(struct socket_data_t *data)
{
	if(data){
		close(data->client_fd);
		close(data->server_fd);
		free(data);
	}
	return 0;
}
