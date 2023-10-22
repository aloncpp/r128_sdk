#define pr_fmt(fmt) "devfs: " fmt

#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <awlog.h>
#include <stdlib.h>
#include <vfs.h>
#include <devfs.h>

#include <sunxi_hal_common.h>

#define DEVFS_MAX_FDS (32)

#ifdef CONFIG_COMPONENTS_AMP

#define MAX_L1_CACHELINE_SIZE 64

struct devfs_dir {
    union {
        DIR dir;
        char dir_data[64];
    } __attribute__((aligned(MAX_L1_CACHELINE_SIZE)));
    struct devfs_node *node;
    int alias;
    union {
        struct dirent dirent;
        char e_data[64];
    } __attribute__((aligned(MAX_L1_CACHELINE_SIZE)));
};
#else
struct devfs_dir {
    DIR dir;
    struct devfs_node *node;
    int alias;
    struct dirent dirent;
};
#endif

static struct devfs {
    const char *mnt;

    SemaphoreHandle_t fds_tb_lock;
    struct devfs_file fds[DEVFS_MAX_FDS];
    unsigned int fds_cnt;

    struct devfs_node *node;
    vfs_t vfs;
} g_devfs = {0}, *devfs = &g_devfs;

static int devfs_fds_lock(TickType_t ticks)
{
    if (devfs->fds_tb_lock == NULL) {
        devfs->fds_tb_lock = xSemaphoreCreateMutex();
        if (devfs->fds_tb_lock == NULL)
            return -ENOMEM;
    }

    if (xSemaphoreTake(devfs->fds_tb_lock, ticks) == pdTRUE)
        return 0;

    pr_err("lock fd table timeout\n");
    return -EBUSY;
}

static int devfs_fds_unlock(void)
{
    if (devfs->fds_tb_lock != NULL) {
        if (xSemaphoreGive(devfs->fds_tb_lock) == pdTRUE)
            return 0;
        else
            return -EBUSY;
    }
    return -EINVAL;
}

static struct devfs_node *devfs_path_to_node(const char *path)
{
    struct devfs_node *n = devfs->node;

    if (*path == '/')
        path++;

    while (n) {
        pr_debug("find %s: %s(%s)\n", path, n->name, n->alias);
        if (!strcmp(path, n->name))
            return n;
        if (!strcmp(path, n->alias))
            return n;
        n = n->next;
    }
    return NULL;
}

struct devfs_file *devfs_fd_to_file(int fd)
{
    int ret;
    struct devfs_file *filp;

    ret = devfs_fds_lock(100);
    if (ret)
        return NULL;

    filp = &devfs->fds[fd];

    devfs_fds_unlock();

    if (filp->node)
        return filp;
    pr_err("fd %d not vailid\n", fd);
    return NULL;
}

static int devfs_vfs_open(const char *path, int flags, int mode)
{
    int ret, i;
    struct devfs_node *n;

    if (*path != '/') {
        pr_err("invalid path %s\n", path);
        ret = -EINVAL;
        goto err;
    }

    n = devfs_path_to_node(path);
    if (!n) {
        pr_warn("file %s not existed\n", path);
        ret = -ENOENT;
        goto err;
    }

    if (n->open) {
        ret = n->open(n);
        if (!ret)
            goto err;
    }

    devfs_fds_lock(100);
    for (i = 0; i < MAX_FDS; i++)
    {
        struct devfs_file *filp = &devfs->fds[i];

        if (!filp->node) {
            filp->node = n;
            filp->off = 0;
            devfs_fds_unlock();
            return i;
        }
    }
    devfs_fds_unlock();

    if (n->close)
        n->close(n);
    ret = -EINVAL;
err:
    pr_warn("open %s failed\n", path);
    return ret;
}

static ssize_t devfs_vfs_write(int fd, const void *data, size_t size)
{
    int ret;
    struct devfs_file *filp;
    struct devfs_node *n;
    ssize_t wlen;

    filp = devfs_fd_to_file(fd);
    if (!filp) {
        ret = -EBADF;
        goto err;
    }
    n = filp->node;

    if (!n->write) {
        ret = -ENOSYS;
        goto err;
    }

    size = MIN((uint32_t)size, n->size - filp->off);
    pr_debug("write %s(%s) off %u size %lu\n", n->name, n->alias, filp->off, size);
    wlen = n->write(n, filp->off, (uint32_t)size, data);
    if (wlen < 0) {
        ret = wlen;
        goto err;
    }

    filp->off += wlen;
    return wlen;
err:
    pr_err("write to fd %d with size %lu failed\n", fd, size);
    return ret;
}

static off_t devfs_vfs_lseek(int fd, off_t off, int whence)
{
    int ret;
    struct devfs_file *filp;

    filp = devfs_fd_to_file(fd);
    if (!filp) {
        ret = -EBADF;
        goto err;
    }

    switch (whence) {
    case SEEK_CUR: off = filp->off + off; break;
    case SEEK_END: off = filp->node->size + off; break;
    case SEEK_SET: break;
    }

    if (off > filp->node->size) {
        pr_warn("devfs not support lseek over device size\n");
        off = filp->node->size;
    }
    filp->off = off;
    return 0;
err:
    pr_err("lseek with fd %d off %ld whence %d failed\n", fd, off, whence);
    return ret;
}

static ssize_t devfs_vfs_read(int fd, void *data, size_t size)
{
    int ret;
    struct devfs_file *filp;
    struct devfs_node *n;
    ssize_t rlen;

    filp = devfs_fd_to_file(fd);
    if (!filp) {
        ret = -EBADF;
        goto err;
    }
    n = filp->node;

    if (!n->read) {
        ret = -ENOSYS;
        goto err;
    }

    size = MIN((uint32_t)size, n->size - filp->off);
    if (size == 0)
        return 0;

    pr_debug("read %s(%s) off %u size %lu\n", n->name, n->alias, filp->off, size);
    rlen = n->read(n, filp->off, (uint32_t)size, data);
    if (rlen < 0) {
        ret = rlen;
        goto err;
    }

    filp->off += rlen;
    return rlen;
err:
    pr_err("read from fd %d with size %lu failed\n", fd, size);
    return ret;
}

static int devfs_vfs_close(int fd)
{
    struct devfs_file *filp;

    filp = devfs_fd_to_file(fd);
    if (!filp)
        return -EBADF;

    /* we do not care what retrun of close */
    if (filp->node->close)
        filp->node->close(filp->node);

    devfs_fds_lock(100);
    filp->node = NULL;
    devfs_fds_unlock();

    return 0;
}

static int devfs_vfs_ioctl(int fd, int cmd, va_list args)
{
    struct devfs_file *filp;
    void *arg;

    arg = va_arg(args, void *);

    filp = devfs_fd_to_file(fd);
    if (!filp)
        return -EBADF;

    if (filp->node->ioctl)
        return filp->node->ioctl(filp->node, cmd, arg);
    return -ENOSYS;
}

static int devfs_vfs_fsync(int fd)
{
    struct devfs_file *filp;

    filp = devfs_fd_to_file(fd);
    if (!filp)
        return -EBADF;

    if (filp->node->fsync)
        return filp->node->fsync(filp->node);
    return -ENOSYS;
}

static DIR *devfs_vfs_opendir(const char *name)
{
    struct devfs_dir *dir;

    if (strlen(name) != 1 || *name != '/') {
        errno = ENOENT;
        return NULL;
    }

#ifdef CONFIG_COMPONENTS_AMP
    dir = amp_align_malloc(sizeof(struct devfs_dir));
#else
    dir = malloc(sizeof(struct devfs_dir));
#endif
    if (!dir) {
        errno = ENOMEM;
        return NULL;
    }
    memset(dir, 0, sizeof(struct devfs_dir));
    dir->node = devfs->node;
    return (DIR *)dir;
}

static int devfs_vfs_readdir_r(DIR *pdir, struct dirent *entry,
                                struct dirent **out_dirent)
{
    struct devfs_dir *dir = (struct devfs_dir *)pdir;

    if (!dir->node)
        goto end;

    entry->d_ino = 0;
    entry->d_type = 0;
    if (dir->alias) {
        while (dir->node && dir->node->alias[0] == '\0')
            dir->node = dir->node->next;
        if (!dir->node)
            goto end;
        snprintf(entry->d_name, 256, "%s", dir->node->alias);
    } else {
        snprintf(entry->d_name, 256, "%s", dir->node->name);
    }

    dir->node = dir->node->next;
    if (!dir->node && !dir->alias) {
        dir->node = devfs->node;
        dir->alias = 1;
    }

    *out_dirent = entry;
    return 0;
end:
    *out_dirent = NULL;
    return 0;
}

static struct dirent *devfs_vfs_readdir(DIR *pdir)
{
    struct devfs_dir *dir = (struct devfs_dir *)pdir;
    struct dirent *out_dirent = NULL;
    int err;

    err = devfs_vfs_readdir_r(pdir, &dir->dirent, &out_dirent);
    if (err != 0) {
        errno = err;
        return NULL;
    }
    return out_dirent;
}

static int devfs_vfs_closedir(DIR *pdir)
{
#ifdef CONFIG_COMPONENTS_AMP
    amp_align_free(pdir);
#else
    free(pdir);
#endif
    return 0;
}

static int devfs_vfs_stat(const char *path, struct stat *st)
{
    struct devfs_node *node;

    if (strcmp(path, "/") == 0) {
        st->st_size = 0;
        st->st_mode = S_IRWXU | S_IRWXG | S_IRWXO;
        st->st_mode |= S_IFDIR;
        st->st_mtime = st->st_atime = st->st_ctime = 0;
        return 0;
    }

    node = devfs_path_to_node(path);
    if (!node) {
        errno = ENOENT;
        return -1;
    }

    memset(st, 0, sizeof(*st));
    st->st_size = node->size;
    st->st_mode = S_IRWXU | S_IRWXG | S_IROTH;
    st->st_mode |= S_IFBLK;
    st->st_mtime = st->st_atime = st->st_ctime = 0;
    return 0;
}

static int devfs_vfs_statfs(const char *path, struct statfs *sfs)
{
    memset(sfs, 0, sizeof(*sfs));
    sfs->f_type = 0xdddd;
    sfs->f_bsize = sfs->f_blocks = 1024;
    sfs->f_bfree = sfs->f_bavail = 0;
    sfs->f_files = 1024;
    return 0;
}

static int devfs_vfs_poll(int fd, struct hal_pollreq *req)
{
    int mask = 0;

#ifdef CONFIG_COMPONENT_IO_MULTIPLEX
    struct devfs_file *filp;
    filp = devfs_fd_to_file(fd);
    mask = filp->node->fops->poll(fd, req);
#endif
    return mask;
}

#ifdef CONFIG_COMPONENTS_AW_DEVFS_NULL_DEV
static struct devfs_node null_dev = {0};
ssize_t null_dev_read(struct devfs_node *node, uint32_t addr, uint32_t size, void *data)
{
	return 0;
}

ssize_t null_dev_write(struct devfs_node *node, uint32_t addr, uint32_t size, const void *data)
{
	return size;
}
#endif

int devfs_mount(const char *mnt_point)
{
    int ret = -EINVAL;
    vfs_t *vfs;

#ifndef CONFIG_COMPONENT_VFS
    pr_err("devfs need vfs\n");
    return -ENOSYS;
#endif

    if (devfs->mnt) {
        if (strcmp(devfs->mnt, mnt_point)) {
            pr_err("already mount on %s\n", devfs->mnt);
            return -EBUSY;
        }
        return 0;
    }

    if (!mnt_point) {
        pr_err("invalid mount point %s\n", mnt_point);
        goto err;
    }

    vfs = &devfs->vfs;
    vfs->flags = VFS_FLAG_DEFAULT;
    vfs->write = devfs_vfs_write;
    vfs->lseek = devfs_vfs_lseek;
    vfs->read = devfs_vfs_read;
    vfs->open = devfs_vfs_open;
    vfs->close = devfs_vfs_close;
    vfs->ioctl = devfs_vfs_ioctl;
    vfs->fsync = devfs_vfs_fsync;
    vfs->opendir = devfs_vfs_opendir;
    vfs->readdir = devfs_vfs_readdir;
    vfs->readdir_r = devfs_vfs_readdir_r;
    vfs->closedir = devfs_vfs_closedir;
    vfs->stat = devfs_vfs_stat;
    vfs->statfs = devfs_vfs_statfs;
    vfs->poll = devfs_vfs_poll;
    ret = vfs_register(mnt_point, vfs, devfs);
    if (ret) {
        pr_err("register devfs to vfs failed - %d\n", ret);
        goto err;
    }

    devfs->mnt = mnt_point;
    pr_info("mount devfs to %s ok\n", mnt_point);

#ifdef CONFIG_COMPONENTS_AW_DEVFS_NULL_DEV
    null_dev.name = "null";
    null_dev.size = UINT32_MAX;
    null_dev.alias = "";
    null_dev.read = null_dev_read;
    null_dev.write = null_dev_write;
    devfs->node = &null_dev;
    null_dev.next = NULL;
#endif

    return 0;

err:
    pr_err("mount devfs to %s failed\n", mnt_point);
    return ret;
}

int devfs_add_node(struct devfs_node *new)
{
    int ret = -EINVAL;
    struct devfs_node *n, *pre;

    if (!new->name || !new->size)
        goto err;

    if (!devfs->node) {
        devfs->node = new;
        new->next = NULL;
        goto out;
    }

    for (n = devfs->node; n; pre = n, n = n->next) {
        if (!strcmp(n->name, new->name)) {
            pr_err("node %s already added\n", new->name);
            ret = -EEXIST;
            goto err;
        }
        if (!strcmp(n->alias, new->name)) {
            pr_err("node %s already added as alias\n", new->name);
            ret = -EEXIST;
            goto err;
        }
    }

    new->next = NULL;
    pre->next = new;
out:
    pr_debug("add node %s(%s) ok\n", new->name, new->alias);
    return 0;

err:
    pr_err("add node %s(%s) failed - %d\n", new->name, new->alias, ret);
    return ret;
}

void devfs_del_node(struct devfs_node *node)
{
    struct devfs_node *n, *pre;

    for (n = devfs->node; n; pre = n, n = n->next) {
        if (node == n) {
            if (n == devfs->node)
                devfs->node = NULL;
            else
                pre->next = n->next;
            break;
        }
    }
}



struct devfs_node* devfs_get_node(char *name)
{
    struct devfs_node *n, *pre;

    for (n = devfs->node; n; pre = n, n = n->next) {
        if (!strcmp(n->name, name) || !strcmp(n->alias, name)) {
            return n;
        }
    }
    return NULL;
}
