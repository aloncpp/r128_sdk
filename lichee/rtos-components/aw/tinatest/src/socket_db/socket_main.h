#ifndef __SOCKET_MAIN_H
#define __SOCKET_MAIN_H

/*head file for socket_main.c*/
typedef void (*connected_callback_t)(void *arg, int result);

struct socket_data_t *socket_init(int timeout_ms, volatile int *exit);
int socket_exit(struct socket_data_t *data);
int recv_from_socket(char *buf,int length);
int send_to_sockt(char *buf,int length);

#endif