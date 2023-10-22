#ifndef _WV_COMMUNICATION_H_
#define _WV_COMMUNICATION_H_

#ifdef __cplusplus
extern "C" {
#endif

enum data_format {
	JEPE_FORMAT = 0,
};

typedef struct _video_data_header
{
#if 0
	int format;
	unsigned int fps;
	unsigned int width;
	unsigned int height;
#endif
	unsigned int data_len;
} video_data_header;

typedef struct _video_data
{
	video_data_header header;
	char payload[0];
} video_data;

int tcp_server_start(void);
int send_package(int fd, void *pkt, int size);

extern int client_sockfd;
extern int tcp_connected;

#if __cplusplus
}; // extern "C"
#endif

#endif
