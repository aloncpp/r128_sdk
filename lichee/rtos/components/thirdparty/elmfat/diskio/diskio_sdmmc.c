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
#include <blkpart.h>
#include <compiler.h>
#include <sdmmc/sdmmc.h>
#include <sdmmc/card.h>
#include <sdmmc/_mmc.h>

static struct devfs_node *s_cards[FF_VOLUMES] = { NULL };

static const char *TAG = "diskio_sdmmc";

DSTATUS ff_sdmmc_initialize(BYTE pdrv)
{
    return 0;
}

DSTATUS ff_sdmmc_status(BYTE pdrv)
{
    return 0;
}

DRESULT ff_sdmmc_read(BYTE pdrv, BYTE *buff, DWORD sector, UINT count)
{
    struct devfs_node *node = s_cards[pdrv];
    struct mmc_card *card = mmc_card_open(0);
    struct part *part = (struct part *)node->private;
    hal_assert(card);
    int err = mmc_block_read(card, buff, (uint64_t)sector + (part->off >> 9), (uint32_t)count);
    mmc_card_close(0);
    if (unlikely(err != 0))
    {
        pr_err(TAG, "sdmmc_read_blocks failed (%d)", err);
        return -1;
    }

    return 0;
}

DRESULT ff_sdmmc_write(BYTE pdrv, const BYTE *buff, DWORD sector, UINT count)
{
    struct devfs_node *node = s_cards[pdrv];
    struct mmc_card *card = mmc_card_open(0);
    struct part *part = (struct part *)node->private;
    hal_assert(card);
    int err = mmc_block_write(card, buff, (uint64_t)sector + (part->off >> 9), count);
    mmc_card_close(0);
    if (unlikely(err != 0))
    {
        pr_err(TAG, "sdmmc_write_blocks failed (%d)", err);
        return RES_ERROR;
    }

    return RES_OK;
}

DRESULT ff_sdmmc_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    DRESULT ret = RES_ERROR;
    struct devfs_node *node = s_cards[pdrv];
    struct mmc_card *card = mmc_card_open(0);
    struct part *part = (struct part *)node->private;
    hal_assert(node);
    switch (cmd)
    {
        case CTRL_SYNC:
            ret = RES_OK;
            break;
        case GET_SECTOR_COUNT:
            *((DWORD *) buff) = part->bytes >> 9;
            ret = RES_OK;
            break;
        case GET_SECTOR_SIZE:
            *((WORD *) buff) = 512;
            ret = RES_OK;
            break;
        case GET_BLOCK_SIZE:
            *((DWORD *) buff) = part->bytes >> 12;
            break;
        default:
            break;
    }
    mmc_card_close(0);
    return ret;
}

void ff_diskio_register_sdmmc(BYTE pdrv, struct devfs_node *card)
{
    static const ff_diskio_impl_t sdmmc_impl =
    {
        .init = &ff_sdmmc_initialize,
        .status = &ff_sdmmc_status,
        .read = &ff_sdmmc_read,
        .write = &ff_sdmmc_write,
        .ioctl = &ff_sdmmc_ioctl
    };
    s_cards[pdrv] = card;
    ff_diskio_register(pdrv, &sdmmc_impl);
}

BYTE ff_diskio_get_pdrv_node(const struct devfs_node *node)
{
    for (int i = 0; i < FF_VOLUMES; i++)
    {
        if (node == s_cards[i])
        {
            return i;
        }
    }
    return 0xff;
}
