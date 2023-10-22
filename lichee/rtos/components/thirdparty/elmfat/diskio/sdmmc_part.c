#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <blkpart.h>
#include <part_efi.h>
#include <sdmmc/hal_sdhost.h>
#include <sdmmc/card.h>
#include <sdmmc/_mmc.h>

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

#define pr_err printf
#define pr_debug printf

#ifndef CONFIG_DRIVERS_SDMMC_FS_PATH
#define CONFIG_DRIVERS_SDMMC_FS_PATH "/sdmmc"
#endif

extern size_t _sunxi_sdmmc_write(size_t pos, const void *buffer, size_t size);
extern size_t _sunxi_sdmmc_read(size_t pos, const void *buffer, size_t size);

static struct blkpart sdmmcblk;

static int register_part(struct part *part)
{
    int ret = -1;
    part->node.size = part->bytes;
    part->node.name = part->name;
    part->node.alias = part->devname;
    part->node.write = blkpart_devfs_write;
    part->node.read = blkpart_devfs_read;
    part->node.private = part;
    part->flag = PARTFLAG_FORCE_RW;

    return devfs_add_node(&part->node);
}

static int unregister_part(struct devfs_node *dev)
{
    devfs_del_node(dev);
    return 0;
}

static int register_blk_device(struct devfs_node *dev)
{
    int ret = -1, index = 0;
    struct part *part;
    unsigned int offset = 0;
    int i = 0;

    memset(&sdmmcblk, 0, sizeof(struct blkpart));

    dev->ioctl(dev, BLOCK_DEVICE_CMD_GET_PAGE_SIZE, &sdmmcblk.page_bytes);
    dev->ioctl(dev, BLOCK_DEVICE_CMD_GET_TOTAL_SIZE, &sdmmcblk.total_bytes);
    dev->ioctl(dev, BLOCK_DEVICE_CMD_GET_BLOCK_SIZE, &sdmmcblk.blk_bytes);
    dev->size = sdmmcblk.total_bytes;

    sdmmcblk.n_parts = 1;
    sdmmcblk.parts = malloc(sizeof(struct part) * sdmmcblk.n_parts);
    if (sdmmcblk.parts == NULL)
    {
        pr_err("allocate part array failed.\n");
        goto err;
    }
    memset(sdmmcblk.parts, 0, sizeof(struct part) * sdmmcblk.n_parts);

    sdmmcblk.name = "sdmmc";
    sdmmcblk.noncache_program = _sunxi_sdmmc_write;
    sdmmcblk.read = _sunxi_sdmmc_read;
    sdmmcblk.noncache_read = _sunxi_sdmmc_read;

    part = &sdmmcblk.parts[index++];
    part->blk = &sdmmcblk;
    part->off = 0;
    part->bytes = dev->size;
    snprintf(part->name, MAX_BLKNAME_LEN, "%s", "SDMMC-DISK");

    register_part(part);
    blkpart_add_list(&sdmmcblk);

    ret = 0;

err:
    return ret;
}

static int register_emmc_blk_device(struct devfs_node *dev)
{
    int i = 0;
    int ret = -1, index = 0;
    struct part *part;
    unsigned int offset = 0;
    unsigned char *gpt_buf = NULL;
    struct mmc_card *card = NULL;
    struct gpt_part *gpt_part;
    int nparts;

    memset(&sdmmcblk, 0, sizeof(struct blkpart));

    dev->ioctl(dev, BLOCK_DEVICE_CMD_GET_PAGE_SIZE, &sdmmcblk.page_bytes);
    dev->ioctl(dev, BLOCK_DEVICE_CMD_GET_TOTAL_SIZE, &sdmmcblk.total_bytes);
    dev->ioctl(dev, BLOCK_DEVICE_CMD_GET_BLOCK_SIZE, &sdmmcblk.blk_bytes);
    dev->size = sdmmcblk.total_bytes;

    card = mmc_card_open((int)dev->private);
    if (!card)
    {
        ret = -1;
        goto err;
    }

#define EMMC_GPT_SIZE (16 * 1024)
#define EMMC_GPT_ADDR (0)
    gpt_buf = hal_malloc(EMMC_GPT_SIZE);
    if (!gpt_buf)
    {
        ret = -1;
        goto err;
    }
    memset(gpt_buf, 0, EMMC_GPT_SIZE);
    ret = mmc_block_read(card, gpt_buf, EMMC_GPT_ADDR, EMMC_GPT_SIZE >> 9);
    if (ret < 0)
    {
        ret = -1;
        goto err;
    }
    nparts = gpt_part_cnt(gpt_buf);
    if (nparts <= 0)
    {
        ret = -1;
        goto err;
    }

    sdmmcblk.n_parts = nparts;
    sdmmcblk.parts = malloc(sizeof(struct part) * sdmmcblk.n_parts);
    if (sdmmcblk.parts == NULL)
    {
        pr_err("allocate part array failed.\n");
        ret = -1;
        goto err;
    }
    memset(sdmmcblk.parts, 0, sizeof(struct part) * sdmmcblk.n_parts);

    sdmmcblk.name = "emmc";
    sdmmcblk.noncache_program = _sunxi_sdmmc_write;
    sdmmcblk.read = _sunxi_sdmmc_read;
    sdmmcblk.noncache_read = _sunxi_sdmmc_read;

    foreach_gpt_part(gpt_buf, gpt_part)
    {
        uint32_t start_sectors = 0;
        uint32_t num_sectors = 0;
        part = &sdmmcblk.parts[index];
        part->blk = &sdmmcblk;
        get_part_info_by_name(gpt_buf, gpt_part->name, &start_sectors, &num_sectors);
        part->bytes = num_sectors;
        part->bytes <<= SECTOR_SHIFT;
        part->off = start_sectors;
        part->off <<= SECTOR_SHIFT;
        snprintf(part->name, MAX_BLKNAME_LEN, "%s", gpt_part->name);
        snprintf(part->devname, MAX_BLKNAME_LEN, "emmc0p%d", index);
        index++;
    }

    ret = add_blkpart(&sdmmcblk);
    if (ret)
    {
        goto free_parts;
    }

    if (card) {
        mmc_card_close((int)dev->private);
    }
    hal_free(gpt_buf);
    return 0;

free_parts:
    hal_free(sdmmcblk.parts);
err:
    if (card) {
        mmc_card_close((int)dev->private);
    }
    hal_free(gpt_buf);
    return ret;
}


static int unregister_blk_device(struct devfs_node *dev)
{
    int ret = -1;

    struct devfs_node *part_dev = devfs_get_node("SDMMC-DISK");
    if (!part_dev)
    {
        return ret;
    }

    ret = unregister_part(part_dev);
    if (ret != 0)
    {
        return ret;
    }
    blkpart_del_list(&sdmmcblk);
    free(sdmmcblk.parts);
    return 0;
}

static int unregister_emmc_blk_device(struct devfs_node *dev)
{
    del_blkpart(&sdmmcblk);
    free(sdmmcblk.parts);
    return 0;
}

int sdmmc_blkpart_init(const char *sdmmc_device_name)
{
    int ret = -1;
    struct devfs_node *dev = devfs_get_node(sdmmc_device_name);
    if (!dev)
    {
        pr_err("the sdmmc device %s is not exist!\n", sdmmc_device_name);
        return ret;
    }

    return sunxi_sdmmc_init(dev);
}

void mount_sdmmc_filesystem(int card_id)
{
    int ret = -1;
    struct devfs_node *dev = devfs_get_node("mmcblk");
    struct mmc_card *card = NULL;
    char *device_name = NULL;

    if (!dev) {
        pr_err("the sdmmc device [sdmmc] is not exist!\n");
        return;
    }

    card = mmc_card_open(card_id);
    if (!card) {
        return;
    }

    if (card->type == CT_MMC) {
        ret = register_emmc_blk_device(dev);
        device_name = "UDISK";
    }
    else {
        ret = register_blk_device(dev);
        device_name = "SDMMC-DISK";
    }

    mmc_card_close(card_id);

    if (ret == 0)
    {
        ret = elmfat_mount(device_name, CONFIG_DRIVERS_SDMMC_FS_PATH);
        if (ret != 0)
        {
            if (card->type == CT_MMC) {
                unregister_emmc_blk_device(dev);
            } else {
                unregister_blk_device(dev);
            }
        }
    }
    return;
}

void unmount_sdmmc_filesystem(int card_id)
{
    int ret = -1;
    char *device_name;
    struct mmc_card *card = NULL;

    card = mmc_card_open(card_id);
    if (!card) {
        return;
    }

    if (card->type == CT_MMC) {
        device_name = "UDISK";
    } else {
        device_name = "SDMMC-DISK";
    }

    mmc_card_close(card_id);

    ret = elmfat_unmount(CONFIG_DRIVERS_SDMMC_FS_PATH, devfs_get_node(device_name));
    if (ret == 0)
    {
        if (card->type == CT_MMC) {
            unregister_emmc_blk_device(devfs_get_node("mmcblk"));
        } else {
            unregister_blk_device(devfs_get_node("mmcblk"));
        }
    }
    return;
}
