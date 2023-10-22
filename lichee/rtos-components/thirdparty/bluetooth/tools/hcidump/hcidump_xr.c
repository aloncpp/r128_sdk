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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "hcidump_xr/hcidump_xr.h"
#include "parser/parser.h"


#define HCIDUMP_VERSION    "2.5"
#define SNAP_LEN           HCI_MAX_FRAME_SIZE

/* Default options */
static int mode = PARSE;
static uint8_t hcidump_start = 0;

typedef enum {
    kCommandPacket = 1,
    kAclPacket = 2,
    kScoPacket = 3,
    kEventPacket = 4
} packet_type_t;

#if 0
#include "fs/fatfs/ff.h"
#include "common/framework/fs_ctrl.h"
#endif

int process_hci_data(int mode, uint8_t *data, uint32_t len)
{
    struct frame frm;

    if (!hcidump_start)
        return -1;

    frm.data = data;
    frm.data_len = len;

    gettimeofday(&frm.ts, NULL);            //use hcidump local receive time
    frm.in = *(frm.data);
    frm.data++;
    //printf("xrdump[process_hci_data]: frm.in = 0x%x \n", frm.in);

    frm.ptr = frm.data;
    frm.len = frm.data_len;

    switch (mode) {
    case PARSE:
        /* Parse and print */
        parse(&frm);
        break;
    case WRITE:
        /* Add save log to file function here */
        //btsnoop_write(frm.ptr, frm.len);
        break;
    default:
        break;
    }

    return 0;
}

int hcidump_process(uint8_t type, const uint8_t *packet, uint8_t is_received)
{
    struct frame frm;

    if (!hcidump_start)
        return -1;

    uint32_t length = 0;

    switch (type) {
    case kCommandPacket:
        length = packet[2] + 4;
        break;
    case kAclPacket:
        length = (packet[3] << 8) + packet[2] + 5;
        break;
    case kScoPacket:
        length = packet[2] + 4;
        break;
    case kEventPacket:
        length = packet[1] + 3;
        break;
    default:
        printf("trc_dump_buffer: unknown type = 0x%x \n", type);
        return -1;
    }

    uint8_t *tmp = calloc(1, length + 1);
    tmp++;
    memcpy(tmp, packet, length);

    tmp--;
    *tmp = type;

    frm.data = tmp;
    frm.data_len = length + 1;

    gettimeofday(&frm.ts, NULL);            //use hcidump local receive time
    frm.in = is_received;
    //frm.data++;
    //printf("xrdump[process_hci_data]: frm.in = 0x%x \n", frm.in);

    frm.ptr = frm.data;
    frm.len = frm.data_len;

    switch (mode) {
    case PARSE:
        /* Parse and print */
        parse(&frm);
        break;
    case WRITE:
        /* Add save log to file function here */
        //btsnoop_write(frm.ptr, frm.len);
        break;
    default:
        break;
    }

    free(tmp);

    return 0;
}

void hcidump_start_up(int set_mode)
{
    printf("[hcidump_start_up] version %s \n", HCIDUMP_VERSION);

    unsigned long flags = 0;
    unsigned long filter = 0;

    mode = set_mode;

    /* Default settings */
    if (!filter)
        filter = ~0L;

    flags |= DUMP_BTSNOOP;
    flags |= DUMP_VERBOSE;
    init_parser(flags, filter, 0, 0xff, -1, -1);

    if (WRITE == set_mode) {
#if 0
        if (fs_ctrl_mount(FS_MNT_DEV_TYPE_SDCARD, 0) != 0) {
            printf("mount fail\n");
            return -1;
        } else {
            printf("mount success\n");
        }

        FIL fp;
        uint32_t bw;

        int ret = f_open(&fp, "hcilog/bt_test.txt", FA_WRITE | FA_OPEN_ALWAYS);
        if(ret != FR_OK) {
            printf("open failed: %d", ret);
            return;
        }

        printf("[hcidump_start_up]open ok: %d\n", ret);

        ret = f_write(&fp, "test...", 7, &bw);
        if(ret != FR_OK || 7 != bw) {
            printf("write failed: %d \n", ret);
            return;
        }

        f_close(&fp);
#endif

    }

    hcidump_start = 1;

}

void hcidump_shut_down(void)
{
    hcidump_start = 0;
}

