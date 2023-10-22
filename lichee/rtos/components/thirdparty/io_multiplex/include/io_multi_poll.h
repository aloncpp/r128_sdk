/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 */

#ifndef VFS_POLL_H__
#define VFS_POLL_H__

#include <rt_config.h>
#include <finsh.h>
#include <vfs.h>
#include <hal_uart.h>

#ifdef RT_USING_POSIX
#include <sys/time.h> /* for struct timeval */
#include <vfs.h>

typedef unsigned int nfds_t;

struct pollfd
{
    int fd;
    short events;
    short revents;
};

#define POLLMASK_DEFAULT (POLLIN | POLLOUT | POLLRDNORM | POLLWRNORM)
int poll(struct pollfd *fds, nfds_t nfds, int timeout);
#endif /* RT_USING_POSIX */


#endif /* VFS_POLL_H__ */
