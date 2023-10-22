/******************************************************************************
 *
 *  Copyright (C) 2014 Google, Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include "save_log_by_file.h"

static int default_fd = -1;
static int fd_flag = O_RDWR /*| O_SYNC*/ | O_CREAT;

#define DEFAULT_PATH "data/btsnoop.cfa"
#define AUTHORITY 0777

#define	_SAVE_LOG_WARN			1
#define	_SAVE_LOG_ERROR			1
#define	_SAVE_LOG_INFO			1

#define SAVE_LOG_PRINTF(flags, fmt, arg...)		\
        do {							\
            if (flags)					\
                printf(fmt, ##arg); 	\
        } while (0)

#define SAVE_LOG_WARN(fmt, arg...)	\
        SAVE_LOG_PRINTF(_SAVE_LOG_WARN, "[SAVE LOG WRN] %s():%d "fmt, __func__, __LINE__, ##arg)

#define SAVE_LOG_ERROR(fmt, arg...)	\
        SAVE_LOG_PRINTF(_SAVE_LOG_ERROR, "[SAVE LOG ERR] %s():%d "fmt, __func__, __LINE__, ##arg)

#define SAVE_LOG_INFO(fmt, arg...)	\
        SAVE_LOG_PRINTF(_SAVE_LOG_INFO, "[SAVE LOG INF]  "fmt, ##arg)

// Internal functions
static int file_save_log_init(const char *file_path)
{
    int fd;
    if (file_path == NULL)
        file_path = DEFAULT_PATH;

    unlink(file_path);

    fd = open(file_path, fd_flag, AUTHORITY);
    if (fd < 0) {
        SAVE_LOG_ERROR("file create failed %d\n", fd);
        return fd;
    }
    SAVE_LOG_INFO("file create %d\n", fd);

    default_fd = fd;

    return fd;

}

static int file_save_log_deinit(int fd)
{
    if (fd == -1)
        fd = default_fd;

    default_fd = -1;

    return close(fd);
}

// Module lifecycle functions
int file_save_log_start_up(const char *path)
{
    int ret = file_save_log_init(path);
    if (ret < 0) {
        return ret;
    }

    return 0;
}

int file_save_log_shut_down(void)
{
    return file_save_log_deinit(-1);
}

// Interface function
int file_save_log_write(const void *p_data, int len)
{
    int fd = default_fd;

    if (fd < 0) {
        SAVE_LOG_ERROR("file_save_log module has not been initialized!!!\n");
        return -1;
    }

    int32_t total = 0;
    int ret;

    while (len > 0) {
        ret = write(fd, (uint8_t *)p_data + total, len);

        if (ret < 0) {
            SAVE_LOG_ERROR("error(%d) writing to file.\n", ret);
            return total;
        }
        total += ret;
        len -= ret;
    }

    if (fd_flag & O_SYNC) // O_SYNC not effect right now with this fs.
        fsync(default_fd);

    return total;
}

void file_save_log_flush(void)
{
    if (default_fd != -1)
        fsync(default_fd);
}

static const struct save_log_iface save_log_if =
{
    .write = file_save_log_write,
    .shut_down = file_save_log_shut_down,
};

const struct save_log_iface *file_save_log_iface_create(const char *path, uint8_t o_sync)
{
    if (o_sync)
        fd_flag |= O_SYNC;
    else
        fd_flag &= ~O_SYNC;

    if (file_save_log_start_up(path) == 0)
        return &save_log_if;
    return NULL;
}