#define pr_fmt(fmt) "littlefs: " fmt

#include <awlog.h>
#include "internal.h"

#include <sunxi_hal_common.h>

#ifdef CONFIG_COMPONENTS_AMP

#define MAX_L1_CACHELINE_SIZE 64

struct lfs_vfs_dir {
    union {
        DIR dir;
        char dir_data[64];
    }__attribute__((aligned(MAX_L1_CACHELINE_SIZE)));
    lfs_dir_t lfs_dir;
    union {
        struct dirent e;
        char e_data[64];
    }__attribute__((aligned(MAX_L1_CACHELINE_SIZE)));
};
#else
struct lfs_vfs_dir {
    DIR dir;
    lfs_dir_t lfs_dir;
    struct dirent e;
};
#endif

static int32_t lfs_convert_mode(int32_t flags)
{
    int32_t mode, res = 0;

    mode = flags & O_ACCMODE;
    if (mode == O_RDONLY)
        res |= LFS_O_RDONLY;
    else if (mode == O_WRONLY)
        res |= LFS_O_WRONLY;
    else if (mode == O_RDWR)
        res |= LFS_O_RDWR;

    if (flags & O_CREAT)
        res |= LFS_O_CREAT;
    if (flags & O_EXCL)
        res |= LFS_O_EXCL;
    if (flags & O_TRUNC)
        res |= LFS_O_TRUNC;
    if (flags & O_APPEND)
        res |= LFS_O_CREAT | LFS_O_APPEND;
    return res;
}

static int lfs_convert_return(int lfs_ret)
{
    switch(lfs_ret) {
        case LFS_ERR_OK: return 0;
        case LFS_ERR_IO: return -EIO;
        case LFS_ERR_CORRUPT: return -EIO;
        case LFS_ERR_NOENT: return -ENOENT;
        case LFS_ERR_EXIST: return -EEXIST;
        case LFS_ERR_NOTDIR: return -ENOTDIR;
        case LFS_ERR_ISDIR: return -EISDIR;
        case LFS_ERR_NOTEMPTY: return -ENOTEMPTY;
        case LFS_ERR_BADF: return -EBADF;
        case LFS_ERR_FBIG: return -EFBIG;
        case LFS_ERR_INVAL: return -EINVAL;
        case LFS_ERR_NOSPC: return -ENOSPC;
        case LFS_ERR_NOMEM: return -ENOMEM;
        case LFS_ERR_NOATTR: return -ENODATA;
        case LFS_ERR_NAMETOOLONG: return -ENAMETOOLONG;
        default: return lfs_ret;
    }
}

static void *lfs_get_entry(struct lfs_ctx *ctx, int index)
{
    return ctx->files[index].entry;
}

static char *lfs_get_path(struct lfs_ctx *ctx, int index)
{
    return ctx->files[index].path;
}

static void lfs_add_file(struct lfs_ctx *ctx, int index, void *entry,
        const char *path)
{
    ctx->files[index].entry = entry;
    ctx->files[index].path = strdup(path);
}

static void lfs_del_file(struct lfs_ctx *ctx, int index)
{
    ctx->files[index].entry = NULL;
    free(ctx->files[index].path);
    ctx->files[index].path = NULL;
}

static int lfs_vfs_open(void *_ctx, const char *path, int flags, int mode)
{
    struct lfs_ctx *ctx = _ctx;
    lfs_file_t *file;
    int ret = -ENOMEM, index;

    if (!path || !strlen(path))
        return -EINVAL;

    lfs_lock(ctx);

    for (index = 0; index < MAX_OPENED_FILES; index++) {
        if (!lfs_get_entry(ctx, index))
            break;
    }
    if (index == MAX_OPENED_FILES) {
        pr_err("open too much files, limit %d\n", MAX_OPENED_FILES);
        goto err_unlock;
    }

    file = (lfs_file_t *)malloc(sizeof(*file));
    if (!file)
        goto err_unlock;

    ret = lfs_file_open(ctx->lfs, file, path, lfs_convert_mode(flags));
    if (ret < 0) {
        ret = lfs_convert_return(ret);
        goto err_free;
    }

    lfs_add_file(ctx, index, file, path);
    lfs_unlock(ctx);
    return index;

err_free:
    free(file);
err_unlock:
    lfs_unlock(ctx);
    return ret;
}

static ssize_t lfs_vfs_write(void *_ctx, int fd, const void *buf, size_t size)
{
    ssize_t ret;
    struct lfs_ctx *ctx = _ctx;
    lfs_file_t *file = lfs_get_entry(ctx, fd);

    lfs_lock(ctx);
    ret = lfs_file_write(ctx->lfs, file, buf, size);
    lfs_unlock(ctx);

    if (ret < 0)
        ret = lfs_convert_return(ret);
    return ret;
}

static ssize_t lfs_vfs_read(void *_ctx, int fd, void *buf, size_t size)
{
    ssize_t ret;
    struct lfs_ctx *ctx = _ctx;
    lfs_file_t *file = lfs_get_entry(ctx, fd);

    lfs_lock(ctx);
    ret = lfs_file_read(ctx->lfs, file, buf, size);
    lfs_unlock(ctx);

    if (ret < 0)
        ret = lfs_convert_return(ret);
    return ret;
}

static int lfs_vfs_close(void *_ctx, int fd)
{
    ssize_t ret;
    struct lfs_ctx *ctx = _ctx;
    lfs_file_t *file = lfs_get_entry(ctx, fd);

    lfs_lock(ctx);
    ret = lfs_file_close(ctx->lfs, file);
    if (!ret) {
        lfs_del_file(ctx, fd);
        free(file);
    }
    lfs_unlock(ctx);

    if (ret < 0)
        ret = lfs_convert_return(ret);
    return ret;
}

static off_t lfs_vfs_lseek(void *_ctx, int fd, off_t off, int whence)
{
    ssize_t ret;
    struct lfs_ctx *ctx = _ctx;
    lfs_file_t *file = lfs_get_entry(ctx, fd);

    lfs_lock(ctx);
    ret = lfs_file_seek(ctx->lfs, file, off, whence);
    lfs_unlock(ctx);

    if (ret < 0)
        ret = lfs_convert_return(ret);
    return ret;
}

static int lfs_vfs_stat(void *_ctx, const char *path, struct stat *st)
{
    ssize_t ret;
    struct lfs_ctx *ctx = _ctx;
    struct lfs_info s = {0};

    lfs_lock(ctx);
    ret = lfs_stat(ctx->lfs, path, &s);
    lfs_unlock(ctx);

    if (ret < 0) {
        ret = lfs_convert_return(ret);
    } else {
        st->st_size = s.size;
        st->st_mode = S_IRWXU | S_IRWXG | S_IRWXO |
            ((s.type == LFS_TYPE_DIR ? S_IFDIR : S_IFREG));
    }
    return ret;
}

static int lfs_vfs_fstat(void *ctx, int fd, struct stat *st)
{
    return lfs_vfs_stat(ctx, lfs_get_path(ctx, fd), st);
}


static int lfs_vfs_link(void *ctx, const char *n1, const char *n2)
{
    return -ENOSYS;
}

static int lfs_vfs_unlink(void *_ctx, const char *path)
{
    ssize_t ret;
    struct lfs_ctx *ctx = _ctx;

    lfs_lock(ctx);
    ret = lfs_remove(ctx->lfs, path);
    lfs_unlock(ctx);

    if (ret < 0)
        ret = lfs_convert_return(ret);
    return ret;
}

static int lfs_vfs_rename(void *_ctx, const char *src, const char *dst)
{
    ssize_t ret;
    struct lfs_ctx *ctx = _ctx;

    lfs_lock(ctx);
    ret = lfs_rename(ctx->lfs, src, dst);
    lfs_unlock(ctx);

    if (ret < 0)
        ret = lfs_convert_return(ret);
    return ret;
}

static DIR *lfs_vfs_opendir(void *_ctx, const char *path)
{
    ssize_t ret, index;
    struct lfs_ctx *ctx = _ctx;
    struct lfs_vfs_dir *dir;

    if (!path || !strlen(path))
        return NULL;

    lfs_lock(ctx);

    for (index = 0; index < MAX_OPENED_FILES; index++) {
        if (!lfs_get_entry(ctx, index))
            break;
    }
    if (index == MAX_OPENED_FILES) {
        pr_err("open too much files, limit %d\n", MAX_OPENED_FILES);
        goto err_unlock;
    }

#ifdef CONFIG_COMPONENTS_AMP
    dir = amp_align_malloc(sizeof(*dir));
#else
    dir = malloc(sizeof(*dir));
#endif
    if (!dir)
        goto err_unlock;

    ret = lfs_dir_open(ctx->lfs, &dir->lfs_dir, path);
    if (ret < 0) {
        ret = lfs_convert_return(ret);
        goto err_free;
    }

    lfs_unlock(ctx);

    if (ret < 0)
        ret = lfs_convert_return(ret);
    return (DIR *)dir;

err_free:
#ifdef CONFIG_COMPONENTS_AMP
    amp_align_free(dir);
#else
    free(dir);
#endif
err_unlock:
    lfs_unlock(ctx);
    return NULL;
}

static int lfs_vfs_closedir(void *_ctx, DIR *pdir)
{
    ssize_t ret;
    struct lfs_ctx *ctx = _ctx;
    struct lfs_vfs_dir *dir = (struct lfs_vfs_dir *)pdir;

    lfs_lock(ctx);
    ret = lfs_dir_close(ctx->lfs, &dir->lfs_dir);
    if (!ret)
#ifdef CONFIG_COMPONENTS_AMP
        amp_align_free(dir);
#else
        free(dir);
#endif
    lfs_unlock(ctx);

    if (ret < 0)
        ret = lfs_convert_return(ret);
    return ret;
}

static struct dirent *lfs_vfs_readdir(void *_ctx, DIR *pdir)
{
    ssize_t ret;
    struct lfs_ctx *ctx = _ctx;
    struct lfs_vfs_dir *dir = (struct lfs_vfs_dir *)pdir;
    struct lfs_info info;

    lfs_lock(ctx);
    ret = lfs_dir_read(ctx->lfs, &dir->lfs_dir, &info);
    lfs_unlock(ctx);

    if (ret < 0)
        return NULL;

    if (info.name[0] == 0)
        return NULL;

    dir->e.d_ino = 0;
    dir->e.d_type = info.type;
    strncpy(dir->e.d_name, info.name, LFS_NAME_MAX);
    dir->e.d_name[LFS_NAME_MAX - 1] = '\0';
    return &dir->e;
}

static int lfs_vfs_readdir_r(void *_ctx, DIR *pdir, struct dirent *entry,
        struct dirent **out_dirent)
{
    return -ENOSYS;
}

static void lfs_vfs_seekdir(void *_ctx, DIR *pdir, long offset)
{
    struct lfs_ctx *ctx = _ctx;
    struct lfs_vfs_dir *dir = (struct lfs_vfs_dir *)pdir;

    lfs_lock(ctx);
    lfs_dir_seek(ctx->lfs, &dir->lfs_dir, (lfs_off_t)offset);
    lfs_unlock(ctx);
}

static long lfs_vfs_telldir(void *_ctx, DIR *pdir)
{
    long ret;
    struct lfs_ctx *ctx = _ctx;
    struct lfs_vfs_dir *dir = (struct lfs_vfs_dir *)pdir;

    lfs_lock(ctx);
    ret = lfs_dir_tell(ctx->lfs, &dir->lfs_dir);
    lfs_unlock(ctx);

    if (ret < 0)
        ret = lfs_convert_return(ret);
    return ret;
}

static int lfs_vfs_mkdir(void *_ctx, const char *name, mode_t mode)
{
    long ret;
    struct lfs_ctx *ctx = _ctx;

    lfs_lock(ctx);
    ret = lfs_mkdir(ctx->lfs, name);
    lfs_unlock(ctx);

    if (ret < 0)
        ret = lfs_convert_return(ret);
    return ret;
}

static int lfs_vfs_rmdir(void *_ctx, const char *name)
{
    int ret;
    struct lfs_ctx *ctx = _ctx;

    lfs_lock(ctx);
    ret = lfs_remove(ctx->lfs, name);
    lfs_unlock(ctx);

    if (ret < 0)
        ret = lfs_convert_return(ret);
    return ret;
}

static int lfs_vfs_statfs(void *_ctx, const char *path, struct statfs *sfs)
{
    int ret;
    struct lfs_ctx *ctx = _ctx;

    lfs_lock(ctx);
    ret = lfs_fs_size(ctx->lfs);
    lfs_unlock(ctx);

    if (ret < 0) {
        ret = lfs_convert_return(ret);
    } else {
        memset(sfs, 0, sizeof(*sfs));
        sfs->f_type = 0xd3fc;
        sfs->f_bsize = ctx->cfg->block_size;
        sfs->f_blocks = ctx->cfg->block_count;
        sfs->f_bfree = sfs->f_bavail = sfs->f_blocks - ret;
        sfs->f_files = 1024;
        ret = 0;
    }
    return ret;
}

static int lfs_vfs_fstatfs(void *ctx, int fd, struct statfs *sfs)
{
    return lfs_vfs_statfs(ctx, lfs_get_path(ctx, fd), sfs);
}

static int lfs_vfs_access(void *_ctx, const char *path, int amode)
{
    struct stat st;
    int ret;

    ret = lfs_vfs_stat(_ctx, path, &st);
    if (ret < 0)
        return ret;

    switch (amode) {
    case F_OK: return 0;
    case R_OK: return st.st_mode & S_IRUSR ? 0 : 1;
    case W_OK: return st.st_mode & S_IWUSR ? 0 : 1;
    case X_OK: return st.st_mode & S_IXUSR ? 0 : 1;
    }
}

static int lfs_vfs_fsync(void *_ctx, int fd)
{
    int ret;
    struct lfs_ctx *ctx = _ctx;
    lfs_file_t *file = lfs_get_entry(ctx, fd);

    lfs_lock(ctx);
    ret = lfs_file_sync(ctx->lfs, file);
    lfs_unlock(ctx);

    if (ret < 0)
        ret = lfs_convert_return(ret);
    return ret;
}

int lfs_register_vfs(struct lfs_ctx *ctx)
{
    const vfs_t vfs = {
        .flags = VFS_FLAG_CONTEXT_PTR,
        .open_p = lfs_vfs_open,
        .write_p = &lfs_vfs_write,
        .read_p = &lfs_vfs_read,
        .close_p = &lfs_vfs_close,
        .lseek_p = &lfs_vfs_lseek,
        .fstat_p = &lfs_vfs_fstat,
        .stat_p = &lfs_vfs_stat,
        .link_p = &lfs_vfs_link,
        .unlink_p = &lfs_vfs_unlink,
        .rename_p = &lfs_vfs_rename,
        .opendir_p = &lfs_vfs_opendir,
        .closedir_p = &lfs_vfs_closedir,
        .readdir_p = &lfs_vfs_readdir,
        .readdir_r_p = &lfs_vfs_readdir_r,
        .seekdir_p = &lfs_vfs_seekdir,
        .telldir_p = &lfs_vfs_telldir,
        .mkdir_p = &lfs_vfs_mkdir,
        .rmdir_p = &lfs_vfs_rmdir,
        .statfs_p = &lfs_vfs_statfs,
        .fstatfs_p = &lfs_vfs_fstatfs,
        .access_p = &lfs_vfs_access,
	.fsync_p = &lfs_vfs_fsync,
    };

    return vfs_register(ctx->mnt, &vfs, ctx);
}

int lfs_unregister_vfs(struct lfs_ctx *ctx)
{
    return vfs_unregister(ctx->mnt);
}
