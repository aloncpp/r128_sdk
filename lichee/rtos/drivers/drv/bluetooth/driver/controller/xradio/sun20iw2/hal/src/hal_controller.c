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
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "errno.h"
#include "kernel/os/os.h"
#include "hal_controller.h"
#include "hal_hci.h"
#include "xrbtc.h"

enum {
    VIRTUAL_HCI_ENABLED     =   1 << 0,
};

static uint32_t driver_flags;
#define MAC_LEN 6
#define SET_MAC_COMMAND_LEN 11

#define SDD_FILE_PATH    "/data/sys_sdd_40M.bin"
#define SDD_MAX_SIZE     (2048)

static int load_sdd_file()
{
	int fd = -1, ret = 0;
	uint32_t size, rd_count;
	struct stat file_st;
	uint8_t *file_data = NULL;

	if (stat(SDD_FILE_PATH, &file_st) < 0) {
		printf("Can not access config file %s\n",SDD_FILE_PATH);
		return -ENOENT;
	}

	size = file_st.st_size;
	if (size <= 0 || size > SDD_MAX_SIZE) {
		printf("invalid sdd data size: %ld\n",size);
		return -EINVAL;
	}

	fd = open(SDD_FILE_PATH, O_RDONLY);
	if (fd <= 0) {
		printf("open %s failed\n",SDD_FILE_PATH);
		return -ENOENT;
	}

	file_data = (unsigned char *)malloc(size);
	if (!file_data) {
		printf("can not malloc %d-byte buffer\n", size);
		return -ENOSPC;
	}

	rd_count = read(fd, file_data, size);
	if (rd_count != size) {
		printf("read config file error, size: %ld rd_count:%ld\n",size,rd_count);
		free(file_data);
		close(fd);
		return -EIO;
	}
	close(fd);
	fd = -1;

	ret = xrbtc_sdd_init(size);
	if (ret) {
		printf("xrbtc_sdd_init failed(%d)\n", ret);
		free(file_data);
		return -EIO;
	}

	ret = xrbtc_sdd_write(file_data, size);
	if (ret != size) {
		printf("xrbtc_sdd_write failed(%d)\n", ret);
		free(file_data);
		return -EIO;
	}

	free(file_data);

	return 0;
}

int hal_controller_init()
{
    int ret = 0;

    if (driver_flags & VIRTUAL_HCI_ENABLED)
            return -EALREADY;

    ret = load_sdd_file();
    if (ret) {
        printf("load_sdd_file failed\n");
    } else {
        printf("%s %d load_sdd_file success!\n",__func__,__LINE__);
    }

    ret = xrbtc_init();
    xrbtc_enable();

    if (ret) {
        printf("blec failed(%d)\n", ret);
        return -ENODEV;
    }

    driver_flags |= VIRTUAL_HCI_ENABLED;

    return 0;
}

int hal_controller_deinit()
{
    driver_flags &= ~VIRTUAL_HCI_ENABLED;

    xrbtc_disable();
    xrbtc_deinit();

    return 0;
}

uint32_t hal_controller_ready(void)
{
    return driver_flags;
}

void hal_controller_set_mac(uint8_t *mac)
{
    int i = 0;
    uint8_t write_mac[SET_MAC_COMMAND_LEN] = {0x01, 0x32, 0xfc, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    for (i = 0; i < MAC_LEN; i++) {
        write_mac[SET_MAC_COMMAND_LEN - 2 - i] = mac[i];
    }
    hal_hci_write(write_mac, sizeof(write_mac));
}

