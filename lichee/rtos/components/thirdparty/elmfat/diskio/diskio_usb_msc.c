// Copyright 2015-2017 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "diskio_impl.h"
#include "ffconf.h"
#include "ff.h"
#include "awlog.h"
#include <devfs.h>
#include <compiler.h>

static struct devfs_node* s_usb_msc_luns[FF_VOLUMES] = { NULL };

static const char* TAG = "diskio_usb_msc";

DSTATUS ff_usb_msc_initialize (BYTE pdrv)
{
    return 0;
}

DSTATUS ff_usb_msc_status (BYTE pdrv)
{
    return 0;
}

extern unsigned int DiskRead(struct devfs_node *node, uint32_t blk, uint32_t n, const void *pBuffer);
extern unsigned int DiskWrite(struct devfs_node *node, uint32_t blk, uint32_t n, const void *pBuffer);

DRESULT ff_usb_msc_read (BYTE pdrv, BYTE* buff, DWORD sector, UINT count)
{
    struct devfs_node* node = s_usb_msc_luns[pdrv];
    int err = DiskRead(node, (uint32_t)sector, (uint32_t)count, buff);
    if (unlikely(err != count)) {
        pr_err(TAG, "usb_msc_read_blocks failed (%d)", err);
        return -1;
    }

    return 0;
}

DRESULT ff_usb_msc_write (BYTE pdrv, const BYTE* buff, DWORD sector, UINT count)
{
    struct devfs_node* node = s_usb_msc_luns[pdrv];
    int err = DiskWrite(node, (uint32_t)sector, count, buff);
    if (unlikely(err != count)) {
        pr_err(TAG, "usb_msc_write_blocks failed (%d)", err);
        return RES_ERROR;
    }

    return RES_OK;
}

DRESULT ff_usb_msc_ioctl (BYTE pdrv, BYTE cmd, void* buff)
{
	DRESULT ret = RES_ERROR;
    struct devfs_node* node = s_usb_msc_luns[pdrv];
    switch(cmd) {
        case CTRL_SYNC:
            ret = RES_OK;
            break;
        case GET_SECTOR_COUNT:
            *((DWORD*) buff) = node->size / 512;
            ret = RES_OK;
            break;
        case GET_SECTOR_SIZE:
            *((WORD*) buff) = 512;
            ret = RES_OK;
            break;
        case GET_BLOCK_SIZE:
            ret = RES_ERROR;
            break;
        default:
            break;
    }
    return ret;
}

void ff_diskio_register_usb_msc(BYTE pdrv, struct devfs_node* card)
{
    static const ff_diskio_impl_t usb_msc_impl = {
        .init = &ff_usb_msc_initialize,
        .status = &ff_usb_msc_status,
        .read = &ff_usb_msc_read,
        .write = &ff_usb_msc_write,
        .ioctl = &ff_usb_msc_ioctl
    };
    s_usb_msc_luns[pdrv] = card;
    ff_diskio_register(pdrv, &usb_msc_impl);
}

BYTE ff_usb_msc_diskio_get_pdrv_node(const struct devfs_node* node)
{
    for (int i = 0; i < FF_VOLUMES; i++) {
        if (node == s_usb_msc_luns[i]) {
            return i;
        }
    }
    return 0xff;
}
