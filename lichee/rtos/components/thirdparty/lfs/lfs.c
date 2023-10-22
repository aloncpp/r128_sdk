#define pr_fmt(fmt) "littlefs: " fmt

#include <awlog.h>
#include "internal.h"

#ifdef CONFIG_DRIVERS_SPINOR
#include <sunxi_hal_spinor.h>
#endif

static struct lfs_ctx *ctx_list;

static int lfs_phy_sync(const struct lfs_config *c)
{
    int ret = 0;
    struct lfs_ctx *ctx = c->context;
    struct blkpart *blk = ctx->part->blk;

    if (blk->sync)
        ret = blk->sync();
    return ret;
}

static int32_t lfs_phy_erase(const struct lfs_config *c, lfs_block_t block)
{
    int ret = -ENOSYS;
    struct lfs_ctx *ctx = c->context;
    struct blkpart *blk = ctx->part->blk;
    unsigned int addr;

    addr = ctx->part->off + blk->blk_bytes * block;
    if (blk->erase)
        ret = blk->erase(addr, blk->blk_bytes);
    return ret;
}

static int32_t lfs_phy_read(const struct lfs_config *c, lfs_block_t block,
                             lfs_off_t off, void *dst, lfs_size_t size)
{
    int ret = -ENOSYS;
    struct lfs_ctx *ctx = c->context;
    struct blkpart *blk = ctx->part->blk;
    unsigned int addr;

    addr = ctx->part->off + blk->blk_bytes * block + off;
    if (blk->read)
        ret = blk->read(addr, (void *)dst, size);
    return ret;
}

static int32_t lfs_phy_write(const struct lfs_config *c, lfs_block_t block,
                              lfs_off_t off, const void *dst, lfs_size_t size)
{
    int ret = -ENOSYS;
    struct lfs_ctx *ctx = c->context;
    struct blkpart *blk = ctx->part->blk;
    unsigned int addr;

    addr = ctx->part->off + blk->blk_bytes * block + off;
    if (blk->program)
        ret = blk->program(addr, (void *)dst, size);
    return ret;
}

static int lfs_init_lock(struct lfs_ctx *ctx)
{
    ctx->lock = xSemaphoreCreateMutex();
    if (ctx->lock == NULL) {
        pr_err("init mutex failed\n");
        return -ENOMEM;
    }
    return 0;
}

static int lfs_exit_lock(struct lfs_ctx *ctx)
{
    vSemaphoreDelete(ctx->lock);
    ctx->lock = NULL;
    return 0;
}

int lfs_lock(struct lfs_ctx *ctx)
{
    if (!ctx->lock)
        return -EINVAL;

    if (xSemaphoreTake(ctx->lock, portMAX_DELAY) == pdTRUE)
        return 0;

    return -EBUSY;
}

int lfs_unlock(struct lfs_ctx *ctx)
{
    if (!ctx->lock)
        return -EINVAL;

    if (xSemaphoreGive(ctx->lock) == pdTRUE)
        return 0;

    return -EBUSY;
}

static int lfs_init(struct lfs_ctx *ctx, struct part *part, const char *mnt)
{
    int ret;
    lfs_t *lfs;
    struct lfs_config *cfg;

    lfs = malloc(sizeof(*lfs));
    if (!lfs) {
        pr_err("alloc for lfs failed\n");
        return -ENOMEM;
    }

    cfg = malloc(sizeof(*cfg));
    if (!cfg) {
        pr_err("alloc for cfg failed\n");
        ret = -ENOMEM;
        goto free_lfs;
    }

    ret = lfs_init_lock(ctx);
    if (ret)
        goto free_cfg;

    memset(lfs, 0, sizeof(*lfs));
    memset(cfg, 0, sizeof(*cfg));
    cfg->context = ctx;
    cfg->read = lfs_phy_read;
    cfg->prog = lfs_phy_write;
    cfg->erase = lfs_phy_erase;
    cfg->sync = lfs_phy_sync;
    cfg->read_size = part->blk->page_bytes;
    cfg->prog_size = part->blk->page_bytes;
    cfg->block_size = part->blk->blk_bytes;
    cfg->block_count = part->bytes / part->blk->blk_bytes;
    cfg->cache_size = part->blk->page_bytes;
    cfg->lookahead_size = 256;
    /*
     * block_cycles is not erase count, but the boundary for lfs to
     * relocate the metadata to other block. It's used for wear balance.
     * It's is suggested in the range 100-1000, the larger, the less
     * wear cost and the bettter performance.
     */
    cfg->block_cycles = 500;

    ctx->mnt = mnt;
    ctx->part = part;
    ctx->cfg = cfg;
    ctx->lfs = lfs;
    return 0;
free_cfg:
    free(cfg);
free_lfs:
    free(lfs);
    return ret;
}

static int lfs_exit(struct lfs_ctx *ctx)
{
    lfs_exit_lock(ctx);
    free(ctx->cfg);
    ctx->cfg = NULL;
    free(ctx->lfs);
    ctx->lfs = NULL;
    return 0;
}

static void lfs_add_ctx(struct lfs_ctx *ctx)
{
    if (!ctx_list) {
        ctx_list = ctx;
    }  else {
        struct lfs_ctx *pre = ctx_list;

        while (pre->next)
            pre = pre->next;
        pre->next = ctx;
        ctx->next = NULL;
    }
}

static void lfs_del_ctx(struct lfs_ctx *ctx)
{
    struct lfs_ctx *pre, *next;

    if (!ctx_list)
        return;

    pre = next = ctx_list;

    while (next) {
        if (next == ctx) {
            if (pre == next)
                ctx_list = next->next;
            else
                pre->next = next->next;
        }
        pre = next;
        next = next->next;
    }
}

static struct lfs_ctx *get_ctx_by_mnt(const char *mnt)
{
    struct lfs_ctx *ctx = ctx_list;

    while (ctx) {
        if (!strcmp(ctx->mnt, mnt))
            break;
        ctx = ctx->next;
    }
    return ctx;
}

int littlefs_mount(const char *dev, const char *mnt, bool format)
{
    int ret;
    struct part *part;
    struct lfs_ctx *ctx;

    if (strncmp(dev, "/dev/", sizeof("/dev/") - 1)) {
        pr_err("path of device must start with '/dev/': %s\n", dev);
        return -EINVAL;
    }

    part = get_part_by_name(dev + sizeof("/dev/") - 1);
    if (!part) {
        pr_err("not found device %s\n", dev);
        return -ENODEV;
    }

    ctx = malloc(sizeof(*ctx));
    if (!ctx) {
        pr_err("alloc for lfs context failed\n");
        return -ENOMEM;
    }
    memset(ctx, 0, sizeof(*ctx));

    ret = lfs_init(ctx, part, mnt);
    if (ret)
        goto err_out;

    ret = lfs_mount(ctx->lfs, ctx->cfg);
    if (ret) {
        if (!format)
            goto err_exit;

        pr_info("mount %s to %s failed, try to format\n", dev, mnt);
        ret = lfs_format(ctx->lfs, ctx->cfg);
        if (ret) {
            pr_err("format lfs failed %d\n", ret);
            goto err_exit;
        }

        ret = lfs_mount(ctx->lfs, ctx->cfg);
        if (ret) {
            pr_err("re-mount after formating failed %d\n", ret);
            goto err_exit;
        }
    }

    ret = lfs_register_vfs(ctx);
    if (ret)
        goto err_exit;

    lfs_add_ctx(ctx);
    pr_info("mount %s on %s successfully\n", dev, mnt);
    return 0;

err_exit:
    lfs_exit(ctx);
err_out:
    pr_err("mount %s on %s with type littlefs failed %d\n", dev, mnt, ret);
    free(ctx);
    return ret;
}

int littlefs_umount(const char *mnt)
{
    struct lfs_ctx *ctx;

    ctx = get_ctx_by_mnt(mnt);
    if (!ctx) {
        pr_err("no device mount on %s\n", mnt);
        return -EINVAL;
    }

    lfs_del_ctx(ctx);
    lfs_unregister_vfs(ctx);
    lfs_exit(ctx);
    free(ctx);
    return 0;
}
