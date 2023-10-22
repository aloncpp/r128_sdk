#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>

#include <blkpart.h>

#include <sdmmc/hal_sdhost.h>
#include <sdmmc/_mmc.h>
#include <sdmmc/card.h>
#include <sdmmc/sys/sys_debug.h>

#include <awlog.h>

typedef enum BLOCK_DEVICE_CMD_T
{
    BLOCK_DEVICE_CMD_ERASE_ALL = 0x00,
    BLOCK_DEVICE_CMD_ERASE_SECTOR,
    BLOCK_DEVICE_CMD_GET_TOTAL_SIZE,
    BLOCK_DEVICE_CMD_GET_PAGE_SIZE,
    BLOCK_DEVICE_CMD_GET_BLOCK_SIZE,
    BLOCK_DEVICE_CMD_GETGEOME,
    BLOCK_DEVICE_CMD_NUM,
} BLOCK_DEVICE_CMD;

typedef enum DEVICE_PART_CMD_T
{
    DEVICE_PART_CMD_ERASE_ALL = 0x00,
    DEVICE_PART_CMD_ERASE_SECTOR,
    DEVICE_PART_CMD_GET_TOTAL_SIZE,
    DEVICE_PART_CMD_GET_BLOCK_SIZE,
    DEVICE_PART_CMD_NUM,
} DEVICE_PART_CMD;

struct device_blk_geometry
{
	uint32_t sector_count;                           /* *< count of sectors */
	uint32_t bytes_per_sector;                       /* *< number of bytes per sector */
	uint32_t block_size;                             /* *< number of bytes to erase one block */
};

#ifdef CONFIG_SUPPORT_SDMMC_CACHE
#include "sdmmc_cache.h"
#endif

#ifndef CONFIG_EMMC_SUPPORT
#define DETECT_BY_GPIO
#endif

#ifdef CONFIG_SDC_DMA_BUF_SIZE
#define SDXC_MAX_TRANS_LEN              (CONFIG_SDC_DMA_BUF_SIZE * 1024)
#endif

#ifndef ALIGN_DOWN
#define ALIGN_DOWN(size, align)      ((size) & ~((align) - 1))
#endif

#ifndef MIN
#define MIN(a, b) (a > b ? b : a)
#endif

void card_detect(uint32_t present);

int32_t mmc_test_init(uint32_t host_id, SDC_InitTypeDef *sdc_param, uint32_t scan);
struct mmc_card *mmc_card_open(uint8_t card_id);
int32_t mmc_card_close(uint8_t card_id);
int32_t mmc_block_read(struct mmc_card *card, uint8_t *buf, uint64_t sblk, uint32_t nblk);
int32_t mmc_block_write(struct mmc_card *card, const uint8_t *buf, uint64_t sblk, uint32_t nblk);

int sunxi_sdmmc_init(struct devfs_node * dev)
{
    int ret = -1;
    int host_id = 0;

    SDC_InitTypeDef sdc_param = {0};

    sdc_param.debug_mask = (ROM_INF_MASK | \
                            ROM_WRN_MASK | ROM_ERR_MASK | ROM_ANY_MASK);

#ifndef DETECT_BY_GPIO
    sdc_param.cd_mode = CARD_ALWAYS_PRESENT;
#else
    sdc_param.cd_mode = CARD_DETECT_BY_GPIO_IRQ;
#endif

    sdc_param.cd_cb = &card_detect;
    sdc_param.dma_use = 1;
    dev->private = (void *)host_id;

    if (mmc_test_init(host_id, &sdc_param, 1))
    {
        printf("init sdmmc failed!\n");
        return ret;
    }

    return 0;
}

ssize_t sunxi_sdmmc_read(struct devfs_node * dev, uint32_t pos, uint32_t size, void *buffer)
{
    int err = -1;
    ssize_t ret, sz = 0;
    struct device_blk_geometry geometry;
    unsigned long long total_bytes;
    char *sector_buf = NULL;
    uint8_t *data = buffer;

    struct mmc_card *card = mmc_card_open((int)dev->private);
    if (card == NULL)
    {
        printf("mmc open fail\n");
        return -EIO;
    }

    if (size == 0)
    {
        return 0;
    }

    memset(&geometry, 0, sizeof(struct device_blk_geometry));
    err = dev->ioctl(dev, BLOCK_DEVICE_CMD_GETGEOME, &geometry);
    if (err)
    {
        return -EIO;
    }
    total_bytes = geometry.bytes_per_sector * geometry.sector_count;

    if (pos >= total_bytes)
    {
        printf("read offset %u over part size %llu\n", pos, total_bytes);
        return 0;
    }

    if (pos + size > total_bytes)
    {
        printf("over limit: offset %u + size %u over %llu\n",
               pos, size, total_bytes);
    }

    size = MIN(total_bytes - pos, size);
    pr_debug("off 0x%lx size %lu\n", pos, size);

    if (pos % geometry.bytes_per_sector || size % geometry.bytes_per_sector)
    {
        sector_buf = malloc(geometry.bytes_per_sector);
        if (!sector_buf)
        {
            return -ENOMEM;
        }
        memset(sector_buf, 0, geometry.bytes_per_sector);
    }

    /**
     * Step 1:
     * read the beginning data that not align to block size
     */
    if (pos % geometry.bytes_per_sector)
    {
        uint32_t addr, poff, len;

        addr = ALIGN_DOWN(pos, geometry.bytes_per_sector);
        poff = pos - addr;
        len = MIN(geometry.bytes_per_sector - poff, size);

        pr_debug("offset %lu not align %u, fix them before align read\n",
                 pos, geometry.bytes_per_sector);
        pr_debug("step1: read page data from addr 0x%x\n", addr);
        ret = mmc_block_read(card, sector_buf, addr / 512, 1);
        if (ret != 0)
        {
            goto err;
        }

        pr_debug("step2: copy page data to buf with page offset 0x%x and len %u\n",
                 poff, len);
        memcpy(data, sector_buf + poff, len);

        pos += len;
        buffer += len;
        sz += len;
        size -= len;
    }

    /**
     * Step 2:
     * read data that align to block size
     */
    while (size >= geometry.bytes_per_sector)
    {
        if (size < SDXC_MAX_TRANS_LEN)
        {
            ret = mmc_block_read(card, data, pos / 512, size / geometry.bytes_per_sector);
            if (ret)
            {
                goto err;
            }
            pos += geometry.bytes_per_sector * (size / geometry.bytes_per_sector);
            data += geometry.bytes_per_sector * (size / geometry.bytes_per_sector);
            sz += geometry.bytes_per_sector * (size / geometry.bytes_per_sector);
            size -= geometry.bytes_per_sector * (size / geometry.bytes_per_sector);
        }
        else
        {
            while (size > SDXC_MAX_TRANS_LEN)
            {
                ret = mmc_block_read(card, data, pos / 512, SDXC_MAX_TRANS_LEN / 512);
                if (ret)
                {
                    goto err;
                }
                size -= SDXC_MAX_TRANS_LEN;
                data += SDXC_MAX_TRANS_LEN;
                pos += SDXC_MAX_TRANS_LEN;
                sz += SDXC_MAX_TRANS_LEN;
            }
        }
    }

    /**
     * Step 3:
     * read the last data that not align to block size
     */
    if (size)
    {
        pr_debug("last size %u not align %u, read them\n", size, geometry.bytes_per_sector);

        pr_debug("step1: read page data from addr 0x%lx\n", pos);
        ret = mmc_block_read(card, sector_buf, pos / 512, 1);
        if (ret != 0)
        {
            goto err;
        }

        pr_debug("step2: copy page data to buf with page with len %u\n", size);
        memcpy(data, sector_buf, size);
        sz += size;
    }

    ret = 0;
    goto out;

err:
    pr_err("read failed - %d\n", (int)ret);
out:
    if (sector_buf)
    {
        free(sector_buf);
    }
    mmc_card_close((int)dev->private);

    return ret ? ret : sz;
}

int sunxi_sdmmc_open(struct devfs_node * dev)
{
    return 1;
}

int sunxi_sdmmc_close(struct devfs_node * dev)
{
    return 1;
}

size_t _sunxi_sdmmc_write(size_t pos, const void *buffer, size_t size)
{
    size_t ret;
    struct mmc_card *card = mmc_card_open(0);
    ret = mmc_block_write(card, buffer, pos / 512, 1);
    mmc_card_close(0);
    return ret;
}

size_t _sunxi_sdmmc_read(size_t pos, const void *buffer, size_t size)
{
    size_t ret;
    struct mmc_card *card = mmc_card_open(0);
    ret = mmc_block_read(card, (uint8_t *)buffer, pos / 512, size / 512);
    mmc_card_close(0);
    return ret;
}

ssize_t sunxi_sdmmc_write(struct devfs_node * dev, uint32_t pos, uint32_t size, const void *buffer)
{
    int err = -1;
    ssize_t ret, sz = 0;
    struct device_blk_geometry geometry;
    unsigned long long total_bytes;
    char *sector_buf = NULL;
    uint8_t *data = (uint8_t *)buffer;

    struct mmc_card *card = mmc_card_open((int)dev->private);
    if (card == NULL)
    {
        printf("mmc open fail\n");
        return -EIO;
    }

    if (size == 0)
    {
        return 0;
    }

    memset(&geometry, 0, sizeof(struct device_blk_geometry));
    err = dev->ioctl(dev, BLOCK_DEVICE_CMD_GETGEOME, &geometry);
    if (err)
    {
        return -EIO;
    }
    total_bytes = ((uint64_t)geometry.bytes_per_sector) * geometry.sector_count;

    if (pos >= total_bytes)
    {
        printf("read offset %u over part size %llu\n", pos, total_bytes);
        return 0;
    }

    if (pos + size > total_bytes)
    {
        printf("over limit: offset %u + size %u over %llu\n",
               pos, size, total_bytes);
    }

    size = MIN(total_bytes - pos, size);
    pr_debug("off 0x%lx size %lu\n", pos, size);

    if (pos % geometry.bytes_per_sector || size % geometry.bytes_per_sector)
    {
        sector_buf = malloc(geometry.bytes_per_sector);
        if (!sector_buf)
        {
            return -ENOMEM;
        }
        memset(sector_buf, 0, geometry.bytes_per_sector);
    }

    /**
     * Step 1:
     * read the beginning data that not align to block size
     */
    if (pos % geometry.bytes_per_sector)
    {
        uint32_t addr, poff, len;

        addr = ALIGN_DOWN(pos, geometry.bytes_per_sector);
        poff = pos - addr;
        len = MIN(geometry.bytes_per_sector - poff, size);

        pr_debug("offset %lu not align %u, fix them before align read\n",
                 pos, geometry.bytes_per_sector);
        pr_debug("step1: read page data from addr 0x%x\n", addr);
        ret = mmc_block_write(card, sector_buf, addr / 512, 1);
        if (ret != 0)
        {
            goto err;
        }

        pr_debug("step2: copy page data to buf with page offset 0x%x and len %u\n",
                 poff, len);
        memcpy(data, sector_buf + poff, len);

        pos += len;
        buffer += len;
        sz += len;
        size -= len;
    }

    /**
     * Step 2:
     * read data that align to block size
     */
    while (size >= geometry.bytes_per_sector)
    {
        if (size < SDXC_MAX_TRANS_LEN)
        {
            ret = mmc_block_write(card, data, pos / 512, size / geometry.bytes_per_sector);
            if (ret)
            {
                goto err;
            }
            pos += geometry.bytes_per_sector * (size / geometry.bytes_per_sector);
            data += geometry.bytes_per_sector * (size / geometry.bytes_per_sector);
            sz += geometry.bytes_per_sector * (size / geometry.bytes_per_sector);
            size -= geometry.bytes_per_sector * (size / geometry.bytes_per_sector);
        }
        else
        {
            while (size > SDXC_MAX_TRANS_LEN)
            {
                ret = mmc_block_write(card, data, pos / 512, SDXC_MAX_TRANS_LEN / 512);
                if (ret)
                {
                    goto err;
                }
                size -= SDXC_MAX_TRANS_LEN;
                data += SDXC_MAX_TRANS_LEN;
                pos += SDXC_MAX_TRANS_LEN;
                sz += SDXC_MAX_TRANS_LEN;
            }
        }
    }

    /**
     * Step 3:
     * read the last data that not align to block size
     */
    if (size)
    {
        pr_debug("last size %u not align %u, read them\n", size, geometry.bytes_per_sector);

        pr_debug("step1: read page data from addr 0x%llx\n", pos);
        ret = mmc_block_write(card, sector_buf, pos / 512, 1);
        if (ret != 0)
        {
            goto err;
        }

        pr_debug("step2: copy page data to buf with page with len %u\n", size);
        memcpy(data, sector_buf, size);
        sz += size;
    }

    ret = 0;
    goto out;

err:
    pr_err("read failed - %d\n", (int)ret);
out:
    if (sector_buf)
    {
        free(sector_buf);
    }
    mmc_card_close((int)dev->private);

    return ret ? ret : sz;
}

int sunxi_sdmmc_control(struct devfs_node * dev, int cmd, void *args)
{
    int ret = 0;
    struct device_blk_geometry *geometry;
    int flag = 0;

    if (!dev)
    {
        return -EINVAL;
    }

    struct mmc_card *card = mmc_card_open((int)dev->private);
    if (!card)
    {
        return ret;
    }

    switch (cmd)
    {
        case BLOCK_DEVICE_CMD_ERASE_ALL:
            break;
        case BLOCK_DEVICE_CMD_ERASE_SECTOR:
            break;
        case BLOCK_DEVICE_CMD_GET_TOTAL_SIZE:
            *(uint64_t *)args = card->csd.capacity;
            *(uint64_t *)args <<= 9;
            ret = 0;
            break;
        case BLOCK_DEVICE_CMD_GET_PAGE_SIZE:
            *(uint32_t *)args = 512;
            ret = 0;
            break;
        case BLOCK_DEVICE_CMD_GET_BLOCK_SIZE:
            *(uint32_t *)args = 512;
            ret = 0;
            break;
        case BLOCK_DEVICE_CMD_GETGEOME:
            geometry = (struct device_blk_geometry *)args;
            memset(geometry, 0, sizeof(struct device_blk_geometry));
            geometry->block_size = 512;
            geometry->bytes_per_sector = 512;
            geometry->sector_count = ((uint64_t)(card->csd.capacity << 9)) / geometry->bytes_per_sector;
            ret = 0;
            break;
        default:
            break;
    }

    mmc_card_close((int)dev->private);
    return ret;
}

int sunxi_driver_sdmmc_init(void)
{
    int ret = -1;
#ifdef CONFIG_COMPONENTS_AW_DEVFS
    struct devfs_node *device;

    device = malloc(sizeof(struct devfs_node));
    if (!device)
    {
        return ret;
    }

    device->name = "mmcblk";
    device->alias = "card";
    device->size = 1;
    device->open = sunxi_sdmmc_open;
    device->close = sunxi_sdmmc_close;
    device->read = sunxi_sdmmc_read;
    device->write = sunxi_sdmmc_write;
    device->ioctl = sunxi_sdmmc_control;

    ret = devfs_add_node(device);
    if (ret != 0)
    {
        free(device);
        return ret;
    }
    int sdmmc_blkpart_init(const char *name);
    ret = sdmmc_blkpart_init(device->name);
#endif

    return ret;
}

int sunxi_driver_sdmmc_deinit(void)
{
#ifdef CONFIG_SUPPORT_SDMMC_CACHE
    void shutdown_block_cache(void);
    shutdown_block_cache();
#endif
    return 0;
}
