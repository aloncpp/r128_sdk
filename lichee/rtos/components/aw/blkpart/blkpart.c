#define pr_fmt(fmt) "blkpart: " fmt

#include <stdint.h>
#include <errno.h>
#include <awlog.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <blkpart.h>
#include <console.h>

#include <sunxi_hal_common.h>

#ifdef CONFIG_COMPONENTS_AW_DEVFS
#include <devfs.h>
#endif

static struct blkpart *blk_head = NULL;

static int do_write_without_erase(struct blkpart *blk, uint32_t addr, char *buf)
{
    return blk->program(addr, buf, blk->page_bytes);
}

static int do_noncache_write(struct blkpart *blk, uint32_t addr, char *buf, uint32_t len)
{
    return blk->noncache_program(addr, buf, len);
}

static int do_erase_write_blk(struct blkpart *blk, uint32_t addr, char *buf)
{
    int ret;

    if (!(addr % blk->blk_bytes)) {
        ret = blk->erase(addr, blk->blk_bytes);
        if (ret)
            return ret;
    }

    return blk->program(addr, buf, blk->page_bytes);
}

int blkpart_sync(struct part *part)
{
    struct blkpart *blk = part->blk;

    if (blk->sync)
        return blk->sync();
    return 0;
}

int blkpart_config_force_rw(struct part *part, uint32_t force_rw)
{

    if (force_rw)
        part->flag |= PARTFLAG_FORCE_RW;
    else
        part->flag &= ~PARTFLAG_FORCE_RW;

    return 0;
}

int blkpart_config_skip_erase_before_write(struct part *part, uint32_t skip_erase_before_write)
{
    if (skip_erase_before_write)
        part->flag |= PARTFLAG_SKIP_ERASE_BEFORE_WRITE;
    else
        part->flag &= ~PARTFLAG_SKIP_ERASE_BEFORE_WRITE;

    return 0;

}

int blkpart_erase(struct part *part, uint32_t offset, uint32_t size)
{
    struct blkpart *blk = part->blk;

    if (size == 0)
        return 0;

    if (!blk->erase)
        return -ENOSYS;

    if (offset + size > part->bytes)
        pr_warn("erase %s(%s) over limit: offset %u + size %u over %u\n",
                part->name, part->devname, offset, size, part->bytes);

    size = MIN(part->bytes - offset, size);
    pr_debug("erase %s(%s) off 0x%x\n", part->name, part->devname, offset);
    if (part->flag & PARTFLAG_FORCE_RW)
        return blk->noncache_erase(offset + part->off, size);
    else
        return blk->erase(offset + part->off, size);
}

static ssize_t blkpart_write_do(struct part *part, uint32_t offset,
        uint32_t size, const void *data, bool erase_before_write)
{
    if (size == 0)
        return 0;

    ssize_t ret, sz = 0;
    struct blkpart *blk = part->blk;
    char *page_buf = NULL;
    int (*pwrite)(struct blkpart *blk, uint32_t addr, char *buf);

    if (offset >= part->bytes) {
        pr_warn("write offset %u over part size %u\n", offset, part->bytes);
        return 0;
    }

    if (offset + size > part->bytes)
        pr_warn("write %s(%s) over limit: offset %u + size %u over %u\n",
                part->name, part->devname, offset, size, part->bytes);

    size = MIN(part->bytes - offset, size);
    pr_debug("write %s(%s) off 0x%x size %u (erase %d)\n", part->name,
            part->devname, offset, size, erase_before_write);
    offset += part->off;

    if (offset % blk->page_bytes || size % blk->page_bytes) {
        page_buf = malloc(blk->page_bytes);
        if (!page_buf)
            return -ENOMEM;
    }

    if (erase_before_write)
        pwrite = do_erase_write_blk;
    else
        pwrite = do_write_without_erase;

    /**
     * Step 1:
     * write the beginning data that not align to block size
     */
    if (offset % blk->page_bytes) {
        uint32_t addr, poff, len;

        if (part->flag & PARTFLAG_FORCE_RW) {
            len = MIN(blk->page_bytes - (offset % blk->page_bytes), size);
            ret = do_noncache_write(blk, offset, (char *)data, len);
            if (ret)
                goto err;
        } else {
            addr = ALIGN_DOWN(offset, blk->page_bytes);
            poff = offset - addr;
            len = MIN(blk->page_bytes - poff, size);

            pr_debug("offset %u not align %u, fix them before align write\n",
                    offset, blk->blk_bytes);
            pr_debug("step1: read page data from addr 0x%x\n", addr);
            ret = blk->read(addr, page_buf, blk->page_bytes);
            if (ret)
                goto err;

            /* addr must less or equal to address */
            pr_debug("step2: copy buf data to page data with page offset 0x%x and len %u\n",
                    poff, len);
            memcpy(page_buf + poff, data, len);

            pr_debug("step3: flush the fixed page data\n");
            ret = pwrite(blk , addr, page_buf);
            if (ret)
                goto err;
        }

        offset += len;
        data += len;
        sz += len;
        size -= len;
    }

    /**
     * Step 2:
     * write data that align to block size
     */
    while (size >= blk->page_bytes) {
        if (part->flag & PARTFLAG_FORCE_RW)
            ret = do_noncache_write(blk, offset, (char *)data, blk->page_bytes);
        else
            ret = pwrite(blk, offset, (char *)data);

        if (ret)
            goto err;

        offset += blk->page_bytes;
        data += blk->page_bytes;
        sz += blk->page_bytes;
        size -= blk->page_bytes;
    }

    /**
     * Step 3:
     * write the last data that not align to block size
     */
    if (size) {
        pr_debug("last size %u not align %u, write them\n", size, blk->blk_bytes);

        if (part->flag & PARTFLAG_FORCE_RW) {
            ret = do_noncache_write(blk, offset, (char *)data, size);
        } else {
            pr_debug("step1: read page data from addr 0x%x\n", offset);
            ret = blk->read(offset, page_buf, blk->page_bytes);
            if (ret)
                goto err;

            pr_debug("step2: copy buf to page data with page with len %u\n", size);
            memcpy(page_buf, data, size);

            pr_debug("step3: flush the fixed page data\n");
            ret = pwrite(blk, offset, page_buf);
        }
        if (ret)
            goto err;

        sz += size;
    }

#ifdef DEBUG
    pr_debug("write data:\n");
    hexdump(data - sz, sz);
#endif
    ret = 0;
    goto out;

err:
    pr_err("write failed - %d\n", (int)ret);
out:
    if (page_buf)
        free(page_buf);
    return ret ? ret : sz;
}

#ifdef CONFIG_COMPONENTS_AW_DEVFS
ssize_t blkpart_devfs_write(struct devfs_node *node, uint32_t offset,
        uint32_t size, const void *data)
{
    bool erase_before_write = true;

    if(size == 0)
        return 0;

    struct part *part = (struct part *)node->private;

    if (part->flag & PARTFLAG_FORCE_RW)
        erase_before_write = false;

    return blkpart_write_do(part, offset, size, data, erase_before_write);
}
#endif

ssize_t blkpart_write(struct part *part, uint32_t offset,
        uint32_t size, const void *data)
{
    if (size == 0)
        return 0;

    return blkpart_write_do(part, offset, size, data, false);
}

ssize_t blkpart_read(struct part *part, uint32_t offset, uint32_t size,
        void *data)
{
    if (size == 0)
        return 0;

    ssize_t ret, sz = 0;
    struct blkpart *blk = part->blk;
    char *page_buf = NULL;

    if (offset >= part->bytes) {
        pr_warn("read offset %u over part size %u\n", offset, part->bytes);
        return 0;
    }

    if (offset + size > part->bytes)
        pr_warn("read %s(%s) over limit: offset %u + size %u over %u\n",
                part->name, part->devname, offset, size, part->bytes);

    size = MIN(part->bytes - offset, size);
    pr_debug("read %s(%s) off 0x%x size %u\n", part->name, part->devname,
            offset, size);
    offset += part->off;

    if (offset % blk->page_bytes || size % blk->page_bytes) {
        page_buf = malloc(blk->page_bytes);
        if (!page_buf)
            return -ENOMEM;
    }

    /**
     * Step 1:
     * read the beginning data that not align to block size
     */
    if (offset % blk->page_bytes) {
        uint32_t addr, poff, len;

        addr = ALIGN_DOWN(offset, blk->page_bytes);
        poff = offset - addr;
        len = MIN(blk->page_bytes - poff, size);

        pr_debug("offset %u not align %u, fix them before align read\n",
                offset, blk->blk_bytes);
        pr_debug("step1: read page data from addr 0x%x\n", addr);

        if (part->flag & PARTFLAG_FORCE_RW)
            ret = blk->noncache_read(addr, page_buf, blk->page_bytes);
        else
            ret = blk->read(addr, page_buf, blk->page_bytes);

        if (ret)
            goto err;

        pr_debug("step2: copy page data to buf with page offset 0x%x and len %u\n",
                poff, len);
        memcpy(data, page_buf + poff, len);

        offset += len;
        data += len;
        sz += len;
        size -= len;
    }

    /**
     * Step 2:
     * read data that align to block size
     */
    while (size >= blk->page_bytes) {
        if (part->flag & PARTFLAG_FORCE_RW)
            ret = blk->noncache_read(offset, data, blk->page_bytes);
        else
            ret = blk->read(offset, data, blk->page_bytes);

        if (ret)
            goto err;

        offset += blk->page_bytes;
        data += blk->page_bytes;
        sz += blk->page_bytes;
        size -= blk->page_bytes;
    }

    /**
     * Step 3:
     * read the last data that not align to block size
     */
    if (size) {
        pr_debug("last size %u not align %u, read them\n", size, blk->blk_bytes);

        pr_debug("step1: read page data from addr 0x%x\n", offset);
        if (part->flag & PARTFLAG_FORCE_RW)
            ret = blk->noncache_read(offset, page_buf, blk->page_bytes);
        else
            ret = blk->read(offset, page_buf, blk->page_bytes);
        if (ret)
            goto err;

        pr_debug("step2: copy page data to buf with page with len %u\n", size);
        memcpy(data, page_buf, size);
        sz += size;
    }

#ifdef DEBUG
    pr_debug("read data:\n");
    hexdump(data - sz, sz);
#endif
    ret = 0;
    goto out;

err:
    pr_err("read failed - %d\n", (int)ret);
out:
    if (page_buf)
        free(page_buf);
    return ret ? ret : sz;
}

#ifdef CONFIG_COMPONENTS_AW_DEVFS
ssize_t blkpart_devfs_read(struct devfs_node *node, uint32_t offset,
        uint32_t size, void *data)
{
    if (size == 0)
        return 0;

    struct part *part = (struct part *)node->private;

    return blkpart_read(part, offset, size, data);
}
#endif

static int blkpart_init_part(struct blkpart *blk, struct part *part,
        uint32_t offset, uint32_t n_part)
{
    if (part->off == (uint32_t)BLKPART_OFF_APPEND)
        part->off = offset;
    if (part->bytes == (uint32_t)BLKPART_SIZ_FULL)
        part->bytes = blk->total_bytes - part->off;
    else
        part->bytes = ALIGN_DOWN(part->bytes, blk->page_bytes);
    part->n_part = n_part;
    part->blk = blk;
    if (n_part)
        snprintf(part->devname, 16, "%s0p%d", blk->name, n_part);
    else
        snprintf(part->devname, 16, "%s0", blk->name);

#ifdef CONFIG_COMPONENTS_AW_DEVFS
    memset(&part->node, 0, sizeof(struct devfs_node));
    part->node.name = part->devname;
    part->node.alias = part->name;
    part->node.size = part->bytes;
    part->node.write = blkpart_devfs_write;
    part->node.read = blkpart_devfs_read;
    part->node.private = part;
    part->flag = 0;

    return devfs_add_node(&part->node);
#else
    return 0;
#endif
}

void blkpart_del_list(struct blkpart *blk)
{
    struct blkpart *pblk, *pre;

    if (!blk_head)
        return;

    pblk = pre = blk_head;
    for (pblk = blk_head; pblk; pre = pblk, pblk= pblk->next) {
        if (pblk == blk) {
            if (pblk == blk_head)
                blk_head = NULL;
            else
                pre->next = pblk->next;
            break;
        }
    }
}

void blkpart_add_list(struct blkpart *blk)
{
    struct blkpart *pblk, *pre;

    blk->next = NULL;

    if (!blk_head) {
        blk_head = blk;
        return;
    }

    pblk = pre = blk_head;
    while (pblk) {
        pre = pblk;
        pblk = pblk->next;
    }
    pre->next = blk;
}

void del_blkpart(struct blkpart *blk)
{
    int i;

    if (!blk)
        return;

    for (i = 0; i < blk->n_parts; i++) {
        struct part *part = &blk->parts[i];

        if (!part)
            continue;
#ifdef CONFIG_COMPONENTS_AW_DEVFS
        devfs_del_node(&part->node);
#endif
    }

    blkpart_del_list(blk);
}

int add_blkpart(struct blkpart *blk)
{
    int i, ret = -EINVAL;
    uint32_t off = 0;

    if (!blk->name || !blk->total_bytes || !blk->blk_bytes || !blk->page_bytes)
        goto err;

    if (blk->n_parts && !blk->parts)
        goto err;

    blk->blk_bytes = ALIGN_DOWN(blk->blk_bytes, blk->page_bytes);
    blk->total_bytes = ALIGN_DOWN(blk->total_bytes, blk->blk_bytes);
    blk->blk_cnt = blk->total_bytes / blk->total_bytes;

    for (i = 0; i < blk->n_parts; i++) {
        struct part *part = &blk->parts[i];
	if (i  && !part->off) {
            ret = -EINVAL;
            goto err;
	}

        if (!part || !part->bytes) {
            ret = -EINVAL;
            goto err;
        }

        ret = blkpart_init_part(blk, part, off, i + 1);
        if (ret)
            goto del_part;
#ifdef CONFIG_RESERVE_IMAGE_PART
        /* not need to calculate offset value if it is reserve part */
        if (strncmp(part->name, "reserve", 7))
#endif
        {
            off += part->bytes;
        }
    }

    blk->root.bytes = blk->total_bytes;
    snprintf(blk->root.name, 16, "%s", blk->name);
    ret = blkpart_init_part(blk, &blk->root, 0, 0);
    if (ret)
        goto del_part;

    blkpart_add_list(blk);
    return 0;

del_part:
    del_blkpart(blk);
err:
    pr_err("add blkpart %s failed\n", blk->name);
    return ret;
}

struct part *get_part_by_index(const char *blk_name, uint32_t index)
{
    struct blkpart *blk = blk_head;

    for (blk = blk_head; blk; blk = blk->next) {
        if (!strcmp(blk_name, blk->name)) {
            if (index == 0)
                return &blk->root;
            else if (index == PARTINDEX_THE_LAST)
                return &blk->parts[blk->n_parts - 1];
            else if (blk->n_parts >= index)
                return &blk->parts[index - 1];
            else
                return NULL;
        }
    }
    return NULL;
}

static int part_info_main(int argc, char **argv)
{
    int i;

    for (i = 0; i < PARTINDEX_THE_LAST; i++) {
        struct part *part = get_part_by_index("nor", i);

        if (!part)
            break;

        pr_info("%-16s(%-8s):\tbytes 0x%-8x\toff 0x%-8x\tflag:0x%-8x\n", part->name, part->devname,
                part->bytes, part->off, part->flag);
    }
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(part_info_main, part_info, dump nor partitions);

struct part *get_part_by_name(const char *name)
{
    struct blkpart *blk;

    if (!strncmp(name, "/dev/", sizeof("/dev/") - 1))
        name += sizeof("/dev/") - 1;

    for (blk = blk_head; blk; blk = blk->next) {
        int i;

        for (i = 0; i < blk->n_parts; i++) {
            struct part *part = &blk->parts[i];

            if (!strcmp(part->name, name))
                return part;

            if (!strcmp(part->devname, name))
                return part;
        }
    }
    return NULL;
}

int part_erase(const char *partition)
{
    int i, ret;
    struct part* part;

    part = get_part_by_name(partition);
    if (!part) {
        pr_err("get partition %s fail\n", partition);
        return -1;
    }

    ret = blkpart_erase(part, 0, part->bytes);
    if (ret) {
        pr_err("erase partition %s fail\n", partition);
        return -1;
    }

    pr_info("erase partition %s done\n", partition);

    return 0;
}

static int part_erase_main(int argc, char **argv)
{
    int i, ret;
    struct part* part;

    if (argc != 2) {
        pr_err("usage: part_erase <part name>\n");
        return -1;
    }

    part = get_part_by_name(argv[1]);
    if (!part) {
        pr_err("get partition %s fail\n", argv[1]);
        return -1;
    }

    ret = blkpart_erase(part, 0, part->bytes);
    if (ret) {
        pr_err("erase partition %s fail\n", argv[1]);
        return -1;
    }

    pr_info("erase partition %s done\n", argv[1]);

    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(part_erase_main, part_erase, erase nor partition);

static int part_config_main(int argc, char **argv)
{
    int i, ret;
    uint32_t skip_erase_before_write;
    uint32_t force_rw;
    struct part* part;

    if (argc < 3) {
        pr_err("usage: part_config <part name> <config> <other args>\n");
        pr_err("support config item: force_rw, skip_erase_before_write\n");
        return -1;
    }

    part = get_part_by_name(argv[1]);
    if (!part) {
        pr_err("get partition %s fail\n", argv[1]);
        return -1;
    }

    if (strcmp(argv[2], "force_rw") == 0) {
        if (argc != 4) {
            pr_err("usage: part_config <part name> force_rw <0 or 1>\n");
            return -1;
        }

        if (strcmp(argv[3], "0") == 0) {
            force_rw = 0;
        } else if (strcmp(argv[3], "1") == 0) {
            force_rw = 1;
        } else {
            pr_err("not support %s, only 0 or 1\n", argv[3]);
            return -1;
        }

        pr_info("config force_rw %d for part %s\n", force_rw, argv[1]);
        ret = blkpart_config_force_rw(part, force_rw);
    } else if (strcmp(argv[2], "skip_erase_before_write") == 0) {
        if (argc != 4) {
            pr_err("usage: part_config <part name> skip_erase_before_write <0 or 1>\n");
            return -1;
        }

        if (strcmp(argv[3], "0") == 0) {
            skip_erase_before_write = 0;
        } else if (strcmp(argv[3], "1") == 0) {
            skip_erase_before_write = 1;
        } else {
            pr_err("not support %s, only 0 or 1\n", argv[3]);
            return -1;
        }

        pr_info("config skip_erase_before_write %d for part %s\n", skip_erase_before_write, argv[1]);
        ret = blkpart_config_skip_erase_before_write(part, skip_erase_before_write);
    } else {
        pr_err("not support config %s\n", argv[2]);
        return -1;
    }

    return ret;
}
FINSH_FUNCTION_EXPORT_CMD(part_config_main, part_config, config nor partition);
