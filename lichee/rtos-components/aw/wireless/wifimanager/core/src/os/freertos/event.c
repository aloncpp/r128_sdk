#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <lwip/sockets.h>

#include "freertos_sta.h"
#include "event.h"
#include "wifi_log.h"
#include "scan.h"

void evt_socket_exit(event_handle_t *evt_handle)
{
	if (evt_handle->evt_socket_enable) {
		evt_socket_clear(evt_handle);

		if (evt_handle->evt_socket[0] >= 0) {
			close(evt_handle->evt_socket[0]);
			evt_handle->evt_socket[0] = -1;
		}

		if (evt_handle->evt_socket[1] >= 0) {
			close(evt_handle->evt_socket[1]);
			evt_handle->evt_socket[1] = -1;
		}
		evt_handle->evt_socket_enable = 0;
	}
}

int evt_socket_init(event_handle_t *evt_handle)
{
	int ret;

	if (evt_handle->evt_socket_enable)
		evt_socket_exit(evt_handle);

	ret = socketpair(AF_INET, SOCK_STREAM, 0, evt_handle->evt_socket);
	if (ret == -1) {
		WMG_ERROR("scan socketpair init error\n");
		return ret;
	}
	evt_handle->evt_socket_enable = 1;

	return 0;
}

int evt_socket_clear(event_handle_t *evt_handle)
{
	int ret = -1;
	char c;
	int flags;

	if (!evt_handle->evt_socket_enable) {
		WMG_ERROR("event socket is closed\n");
		return ret;
	}

	/* clear scan.sockets data before sending scan command*/
	if (flags = fcntl(evt_handle->evt_socket[1], F_GETFL, 0) < 0){
		WMG_ERROR("fcntl getfl error\n");
		return -1;
	}

	flags |= O_NONBLOCK;
	if (fcntl(evt_handle->evt_socket[1], F_SETFL, flags) < 0) {
		WMG_ERROR("fcntl setfl error\n");
		return -1;
	}

	while ((ret = TEMP_FAILURE_RETRY(read(evt_handle->evt_socket[1], &c, 1))) > 0) {
		WMG_EXCESSIVE("clear data %d\n", c);
	}

	return 0;
}

int evt_send(event_handle_t *evt_handle, int event)
{
	int ret = -1;
	char data;

	if (!evt_handle->evt_socket_enable) {
		WMG_ERROR("event socket is closed\n");
		return ret;
	}

	data = (char)event;
	ret = TEMP_FAILURE_RETRY(write(evt_handle->evt_socket[0], &data, 1));

	return ret;
}

int evt_read(event_handle_t *evt_handle, int *event)
{
	int ret = -1;
	struct pollfd rfds;
	char c;

	if (!evt_handle->evt_socket_enable) {
		WMG_ERROR("event socket is closed\n");
		return ret;
	}

	memset(&rfds, 0, sizeof(struct pollfd));
	rfds.fd = evt_handle->evt_socket[1];
	rfds.events |= POLLIN;

	/*wait  event.*/
	ret = TEMP_FAILURE_RETRY(poll(&rfds, 1, 70000));
	if (ret < 0) {
		WMG_ERROR("poll = %d\n", ret);
		return ret;
	}

	if(ret == 0) {
		WMG_ERROR("poll time out!\n");
		return ret;
	}

	if (rfds.revents & POLLIN) {
		ret = TEMP_FAILURE_RETRY(read(evt_handle->evt_socket[1], &c, 1));
		*event = (int)c;
		WMG_EXCESSIVE("Excessive: read event %d\n", c);
	}

	return ret;
}
