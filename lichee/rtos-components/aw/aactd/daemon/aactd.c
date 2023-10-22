#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <hal_sem.h>

#include "lwip/inet.h"
#include "lwip/sockets.h"
#include "libc/errno.h"

#include "console.h"
#include "FreeRTOS.h"

#include "aactd/common.h"
#include "aactd/communicate.h"
#include "local.h"

#define PORT_DEFAULT 5005

#define INET_ADDR_STR_LEN INET_ADDRSTRLEN

static int exit_flag = 0;
static int delay = 0;
static int port = 0;

static void help_msg(void)
{
    printf("\n");
    printf("USAGE:\n");
    printf("\taactd [OPTIONS]\n");
    printf("OPTIONS:\n");
    printf("\t-b,--buffer-length LENGTH\t: max length of commnunicate buffer "
            "(default: %d bytes)\n", COM_BUF_LEN_MAX_DEFAULT);
    printf("\t-p,--port PORT\t: binding port (default: %d)\n", PORT_DEFAULT);
    printf("\t-v,--verbose LEVEL\t: print verbose message (level: 0~2)\n");
}

static void aactd_server(void * param)
{

    int ret = 0;
    int listen_fd, connect_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len;
    char inet_addr_str[INET_ADDR_STR_LEN];
	int timeout = delay * 1000; //ms
	hal_sem_t sem =  (hal_sem_t)param;

    uint8_t *remote_com_buf = NULL;
    unsigned int remote_com_actual_len;
    struct aactd_com remote_com = {
        .data = NULL,
        .checksum = 0,
    };
    uint8_t checksum_cal;

	remote_com_buf = malloc(com_buf_len_max);
	if (!remote_com_buf) {
		aactd_error("No memory\n");
		ret = -ENOMEM;
		goto release_local_process;
	}

	memset(&remote_com, 0, sizeof(struct aactd_com));

	remote_com.data = remote_com_buf + sizeof(struct aactd_com_header);

	listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_fd < 0) {
		aactd_error("Failed to create socket (%d)\n", errno);
		ret = -1;
		goto free_remote_com_buf;
	}

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(port);

	ret = bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
	if (ret < 0) {
		aactd_error("Failed to bind address %s and port %d to socket (fd: %d) (%d)\n",
				inet_ntop(AF_INET, &server_addr.sin_addr, inet_addr_str, INET_ADDR_STR_LEN),
				ntohs(server_addr.sin_port), listen_fd, errno);
		goto close_listen_fd;
	}
	AACTD_DEBUG(0, "Bind address %s and port %d to socket (fd: %d)\n",
			inet_ntop(AF_INET, &server_addr.sin_addr, inet_addr_str, INET_ADDR_STR_LEN),
			ntohs(server_addr.sin_port), listen_fd);

	ret = listen(listen_fd, LISTEN_BACKLOG);
	if (ret < 0) {
		aactd_error("Failed to listen socket (fd: %d) (%d)\n", listen_fd, errno);
		goto close_listen_fd;
	}

	/* Set the recv timeout to set the accept timeout */
	if (setsockopt(listen_fd, SOL_SOCKET, SO_RCVTIMEO,
					(char *)&timeout, sizeof(timeout)) < 0) {
		aactd_error("set socket option err %d\n", errno);
		goto close_listen_fd;
	}

	memset(&client_addr, 0, sizeof(client_addr));
	client_addr_len = sizeof(client_addr);
	while (1) {

		if (exit_flag == 1) {
			exit_flag = 0;
			break;
		}

		ssize_t read_bytes = 0;
		connect_fd = accept(listen_fd,
				(struct sockaddr *)&client_addr, &client_addr_len);
		if (connect_fd < 0) {
			if (errno == EAGAIN)
				AACTD_DEBUG(2, "accept connection timeout: %d s (fd: %d)(errno :%d) try again\n",
					delay, listen_fd, errno);
			else
				aactd_error("Failed to accept connection from socket (fd: %d) (%d)\n",
					listen_fd, errno);
			continue;
		}

		aactd_info("TCP connection from %s, port %d established\n",
				inet_ntop(AF_INET, &client_addr.sin_addr, inet_addr_str, INET_ADDR_STR_LEN),
				ntohs(client_addr.sin_port));

		while (1) {
			/* Read header */
			read_bytes = aactd_readn(connect_fd, remote_com_buf,
					sizeof(struct aactd_com_header));
			if (read_bytes < 0) {
				aactd_error("Read error\n");
				break;
			} else if (read_bytes == 0) {
				aactd_info("TCP connection with client (%s:%d) closed\n",
					inet_ntop(AF_INET, &client_addr.sin_addr, inet_addr_str, INET_ADDR_STR_LEN),
					ntohs(client_addr.sin_port));
				break;
			} else if ((size_t)read_bytes < sizeof(struct aactd_com_header)) {
				aactd_error("Failed to read header, try again\n");
				continue;
			}

			AACTD_DEBUG(0, "Read data (%zd bytes)\n", read_bytes);

			aactd_com_buf_to_header(remote_com_buf, &remote_com.header);

			if (remote_com.header.flag != AACTD_COM_HEADER_FLAG && remote_com.header.flag != AACTD_COM_HEADER_FLAG_V2) {
				aactd_error("Incorrect header flag: 0x%x\n", remote_com.header.flag);
				continue;
			}

			if (verbose_level >= 2) {
				aactd_print_original_buf(remote_com_buf, read_bytes, 4);
			}

			remote_com_actual_len =
				sizeof(struct aactd_com_header) + remote_com.header.data_len + 1;

			if (com_buf_len_max < remote_com_actual_len) {
				aactd_error("remote_com_buf is not large enough to receive data\n");
				continue;
			}

			/* Read data & checksum */
			read_bytes = aactd_readn(connect_fd, remote_com.data,
					remote_com.header.data_len + 1);
			if (read_bytes < 0) {
				aactd_error("Read error\n");
				break;
			} else if (read_bytes == 0) {
				aactd_info("TCP connection with client (%s:%d) closed\n",
					inet_ntop(AF_INET, &client_addr.sin_addr, inet_addr_str, INET_ADDR_STR_LEN),
					ntohs(client_addr.sin_port));
				break;
			} else if (read_bytes < remote_com.header.data_len + 1) {
				aactd_error("Failed to read data and checksum, try again\n");
				continue;
			}
			AACTD_DEBUG(0, "Read data (%zd bytes)\n", read_bytes);
			if (verbose_level >= 2) {
				if (remote_com.header.flag == AACTD_COM_HEADER_FLAG)
					aactd_print_original_buf(remote_com_buf, read_bytes, 4);
				else if (remote_com.header.flag == AACTD_COM_HEADER_FLAG_V2)
					aactd_print_original_buf(remote_com_buf, read_bytes + sizeof(struct aactd_com_header), 8);
			}

			/* Verify checksum */
			remote_com.checksum = *(remote_com.data + remote_com.header.data_len);
			checksum_cal = aactd_calculate_checksum(remote_com_buf,
					remote_com_actual_len - 1);
			if (remote_com.checksum != checksum_cal) {
				aactd_error("Checksum error (got: 0x%x, calculated: 0x%x), "
						"discard and try again\n", remote_com.checksum, checksum_cal);
				continue;
			}

			AACTD_DEBUG(1, "Print remote_com:\n");
			if (verbose_level >= 1) {
				aactd_com_print_content(&remote_com);
			}

			switch(remote_com.header.command) {
			case CMD_READ:
				// TODO: implement CMD_READ
				// read_com_from_local(&remote_com);
				// update com_buf from remote_com
				// aactd_writen(connect_fd, com_buf);
				break;
			case CMD_WRITE:
				write_com_to_local(&remote_com);
				break;
			default:
				aactd_error("Unknown command\n");
				break;
			}

		}
		close(connect_fd);
	}

close_listen_fd:
	close(listen_fd);
free_remote_com_buf:
	free(remote_com_buf);
release_local_process:
	local_process_release();
out:

	AACTD_DEBUG(1, "exit aactd server\n");

	hal_sem_post(sem);

	vTaskDelete(NULL);

}

int cmd_aactd_server(int argc, char *argv[])
{
    int ret = 0;

	hal_sem_t sem = NULL;

    const struct option opts[] = {
        { "help", no_argument, NULL, 'h' },
        { "buffer-length", required_argument, NULL, 'b' },
        { "port", required_argument, NULL, 'p' },
        { "verbose", required_argument, NULL, 'v' },
        { "delay", required_argument, NULL, 'd' },
    };

    int opt;

	verbose_level = -1;
	delay = 1;
	port = PORT_DEFAULT;

    while ((opt = getopt_long(argc, argv, "hb:p:v:d:", opts, NULL)) != -1) {
        switch (opt) {
        case 'h':
            help_msg();
            ret = 0;
            goto out;
        case 'b':
            com_buf_len_max = atoi(optarg);
            break;
        case 'p':
            port = atoi(optarg);
            break;
        case 'v':
            verbose_level = atoi(optarg);
            break;
        case 'd':
	        delay = atoi(optarg);
	        break;
        default:
            aactd_error("Invalid option\n");
            help_msg();
            ret = -1;
            goto out;
        }
    }

    ret = local_process_init();
    if (ret < 0) {
        aactd_error("Failed to init local process\n");
        goto out;
    }

	sem = hal_sem_create(0);
	if (sem == NULL) {
		aactd_error("sem init err.\n");
		goto out;
	}

	ret = xTaskCreate(aactd_server, (signed portCHAR *) "aactd_server", 4096, sem, 0, NULL);
	if (ret != pdPASS) {
		aactd_error("Error creating task, status was %d\n", ret);
		goto out;
	}

	while(1) {
		char cRxed = 0;

        cRxed = getchar();
        if(cRxed == 'q' || cRxed == 3) {
			exit_flag = 1;
			aactd_info("Ready to exit aactd, please wait %d s...\n", delay);
			break;
		}
	}

	ret = hal_sem_wait(sem);
	if (ret != 0) {
		aactd_error("Error hal_sem_wait %d\n", ret);
		goto out;
	}


out:
	if (sem) {
		hal_sem_delete(sem);
		sem = NULL;
	}
	aactd_info("exit cmd aactd \n");

	return ret;

}
FINSH_FUNCTION_EXPORT_CMD(cmd_aactd_server, aactd, Console aactd server command);

