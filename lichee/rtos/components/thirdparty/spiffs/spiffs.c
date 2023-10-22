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

// start from esp: https://github.com/espressif/esp-idf

#define pr_fmt(fmt) "SPIFFS: " fmt
#define assert(...)

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/lock.h>
#include <vfs.h>
#include <awlog.h>
#include <blkpart.h>
#include <errno.h>
#include <spiffs.h>

#include "spiffs_api.h"
#include "spiffs/src/spiffs.h"
#include "spiffs/src/spiffs_nucleus.h"

#define SPIFFS_SUPER_MAGIC 0xd3fc

#ifdef CONFIG_SPIFFS_USE_MTIME
_Static_assert(CONFIG_SPIFFS_META_LENGTH >= sizeof(time_t),
        "SPIFFS_META_LENGTH size should be >= sizeof(time_t)");
#endif //CONFIG_SPIFFS_USE_MTIME

/**
 * @brief Configuration structure for vfs_spiffs_register
 */
typedef struct {
    /* File path prefix associated with the filesystem. */
    const char *base_path;
    /* block node name from devfs, if NULL, the last partiiton will be used */
    const char *blk_name;
    /*!< Maximum files that could be open at the same time. */
    size_t max_files;
    /*!< If true, it will format the file system if it fails to mount. */
    bool format_if_mount_failed;
} vfs_spiffs_conf_t;

/**
 * @brief SPIFFS DIR structure
 */
typedef struct {
    DIR dir;            /*!< VFS DIR struct */
    spiffs_DIR d;       /*!< SPIFFS DIR struct */
    struct dirent e;    /*!< Last open dirent */
    long offset;        /*!< Offset of the current dirent */
    char path[SPIFFS_OBJ_NAME_LEN]; /*!< Requested directory name */
} vfs_spiffs_dir_t;

static int vfs_spiffs_open(void* ctx, const char * path, int flags, int mode);
static ssize_t vfs_spiffs_write(void* ctx, int fd, const void * data, size_t size);
static ssize_t vfs_spiffs_read(void* ctx, int fd, void * dst, size_t size);
static int vfs_spiffs_close(void* ctx, int fd);
static off_t vfs_spiffs_lseek(void* ctx, int fd, off_t offset, int mode);
static int vfs_spiffs_fstat(void* ctx, int fd, struct stat * st);
static int vfs_spiffs_stat(void* ctx, const char * path, struct stat * st);
static int vfs_spiffs_unlink(void* ctx, const char *path);
static int vfs_spiffs_link(void* ctx, const char* n1, const char* n2);
static int vfs_spiffs_rename(void* ctx, const char *src, const char *dst);
static DIR* vfs_spiffs_opendir(void* ctx, const char* name);
static int vfs_spiffs_closedir(void* ctx, DIR* pdir);
static struct dirent* vfs_spiffs_readdir(void* ctx, DIR* pdir);
static int vfs_spiffs_readdir_r(void* ctx, DIR* pdir,
                                struct dirent* entry, struct dirent** out_dirent);
static long vfs_spiffs_telldir(void* ctx, DIR* pdir);
static void vfs_spiffs_seekdir(void* ctx, DIR* pdir, long offset);
static int vfs_spiffs_mkdir(void* ctx, const char* name, mode_t mode);
static int vfs_spiffs_rmdir(void* ctx, const char* name);
static int vfs_spiffs_statfs(void *ctx, const char *path, struct statfs *buf);
static int vfs_spiffs_fstatfs(void *ctx, int fd, struct statfs *buf);
static int vfs_spiffs_access(void *cxt, const char *path, int amode);
static void vfs_spiffs_update_mtime(spiffs *fs, spiffs_file f);
static time_t vfs_spiffs_get_mtime(const spiffs_stat* s);

static spiffs_t * _efs[CONFIG_SPIFFS_MAX_PARTITIONS];

static void spiffs_free(spiffs_t ** efs)
{
    spiffs_t * e = *efs;
    if (*efs == NULL) {
        return;
    }
    *efs = NULL;

    if (e->fs) {
        SPIFFS_unmount(e->fs);
        free(e->fs);
    }
    vSemaphoreDelete(e->lock);
    free(e->fds);
    free(e->cache);
    free(e->work);
    free(e);
}

static int spiffs_by_mountpoint(const char *mountpoint, int *index){
    int i;
    spiffs_t *p;

    if (!mountpoint)
        return -EINVAL;

    for (i = 0; i < CONFIG_SPIFFS_MAX_PARTITIONS; i++) {
        p = _efs[i];
        if (p) {
            if (!strncmp(mountpoint, p->base_path, VFS_PATH_MAX)) {
                *index = i;
                return 0;
            }
        }
    }
    return -ENODEV;
}

static int spiffs_by_label(const char *label, int *index){
    int i;
    spiffs_t *p;

    if (!label)
        return -EINVAL;

    for (i = 0; i < CONFIG_SPIFFS_MAX_PARTITIONS; i++) {
        p = _efs[i];
        if (p) {
            if (!label && !p->by_label) {
                *index = i;
                return 0;
            }
            if (label && p->by_label && !strncmp(label, p->part->name, 16)) {
                *index = i;
                return 0;
            }
            if (label && p->by_label && !strncmp(label, p->part->devname, 16)) {
                *index = i;
                return 0;
            }
        }
    }
    return -ENODEV;
}

static int spiffs_get_empty(int * index){
    int i;
    for (i = 0; i < CONFIG_SPIFFS_MAX_PARTITIONS; i++) {
        if (_efs[i] == NULL) {
            *index = i;
            return 0;
        }
    }
    return -ENODEV;
}

static struct part *spiffs_get_part(const vfs_spiffs_conf_t *conf)
{
    if (conf->blk_name)
        return get_part_by_name(conf->blk_name);
    return get_part_by_index("nor", PARTINDEX_THE_LAST);
}

static int spiffs_init(const vfs_spiffs_conf_t *conf)
{
    int index;
    struct part *part;
    struct blkpart *blkpart;
    uint32_t log_page_size = CONFIG_SPIFFS_PAGE_SIZE;

    //find if such partition is already mounted
    if (spiffs_by_label(conf->blk_name, &index) == 0)
        return -EINVAL;

    if (spiffs_get_empty(&index) != 0) {
        pr_err("max mounted partitions reached\n");
        return -EINVAL;
    }

    part = spiffs_get_part(conf);
    if (!part) {
        pr_err("not found block partition %s\n", conf->blk_name);
        return -ENODEV;
    }
    blkpart = part->blk;
    pr_debug("find blk %s part %s(%s)\n", blkpart->name, part->name, part->devname);

    if (log_page_size % blkpart->page_bytes != 0) {
        pr_err("SPIFFS_PAGE_SIZE is not multiple of flash chip page size (%d)\n",
                blkpart->page_bytes);
        return -EINVAL;
    }

    spiffs_t *efs = malloc(sizeof(spiffs_t));
    if (efs == NULL) {
        pr_err("esp_spiffs could not be malloced\n");
        return -ENOMEM;
    }
    memset(efs, 0, sizeof(spiffs_t));

    efs->cfg.hal_erase_f       = (spiffs_erase)spiffs_api_erase;
    efs->cfg.hal_read_f        = (spiffs_read)spiffs_api_read;
    efs->cfg.hal_write_f       = (spiffs_write)spiffs_api_write;
    efs->cfg.log_block_size    = blkpart->blk_bytes;
    efs->cfg.log_page_size     = log_page_size;
    efs->cfg.phys_addr         = 0;
    efs->cfg.phys_erase_block  = blkpart->blk_bytes;
    efs->cfg.phys_size         = part->bytes;

    efs->by_label = conf->blk_name != NULL;

    efs->lock = xSemaphoreCreateMutex();
    if (efs->lock == NULL) {
        pr_err("mutex lock could not be created\n");
        spiffs_free(&efs);
        return -ENOMEM;
    }

    efs->fds_sz = conf->max_files * sizeof(spiffs_fd);
    efs->fds = malloc(efs->fds_sz);
    if (efs->fds == NULL) {
        pr_err("fd buffer could not be malloced\n");
        spiffs_free(&efs);
        return -ENOMEM;
    }
    memset(efs->fds, 0, efs->fds_sz);

#if SPIFFS_CACHE
    efs->cache_sz = sizeof(spiffs_cache) + conf->max_files * (sizeof(spiffs_cache_page)
                          + efs->cfg.log_page_size);
    efs->cache = malloc(efs->cache_sz);
    if (efs->cache == NULL) {
        pr_err("cache buffer could not be malloced\n");
        spiffs_free(&efs);
        return -ENOMEM;
    }
    memset(efs->cache, 0, efs->cache_sz);
#endif

    const uint32_t work_sz = efs->cfg.log_page_size * 2;
    efs->work = malloc(work_sz);
    if (efs->work == NULL) {
        pr_err("work buffer could not be malloced\n");
        spiffs_free(&efs);
        return -ENOMEM;
    }
    memset(efs->work, 0, work_sz);

    efs->fs = malloc(sizeof(spiffs));
    if (efs->fs == NULL) {
        pr_err("spiffs could not be malloced\n");
        spiffs_free(&efs);
        return -ENOMEM;
    }
    memset(efs->fs, 0, sizeof(spiffs));

    efs->fs->user_data = (void *)efs;
    efs->part = part;

    s32_t res = SPIFFS_mount(efs->fs, &efs->cfg, efs->work, efs->fds,
            efs->fds_sz, efs->cache, efs->cache_sz,
        (spiffs_check_callback)spiffs_api_check);

    if (conf->format_if_mount_failed && res != SPIFFS_OK) {
        pr_warn("mount failed, %i. formatting...\n", SPIFFS_errno(efs->fs));
        SPIFFS_clearerr(efs->fs);
        res = SPIFFS_format(efs->fs);
        if (res != SPIFFS_OK) {
            pr_err("format failed, %i\n", SPIFFS_errno(efs->fs));
            SPIFFS_clearerr(efs->fs);
            spiffs_free(&efs);
            return -EINVAL;
        }
        res = SPIFFS_mount(efs->fs, &efs->cfg, efs->work, efs->fds, efs->fds_sz,
                            efs->cache, efs->cache_sz,
                (spiffs_check_callback)spiffs_api_check);
    }
    if (res != SPIFFS_OK) {
        pr_err("mount failed, %i\n", SPIFFS_errno(efs->fs));
        SPIFFS_clearerr(efs->fs);
        spiffs_free(&efs);
        return -EINVAL;
    }
    _efs[index] = efs;
    return 0;
}

bool spiffs_mounted(const char *blkpart)
{
    int index;
    if (spiffs_by_label(blkpart, &index) != 0) {
        return false;
    }
    return (SPIFFS_mounted(_efs[index]->fs));
}

int spiffs_info(const char *blkpart, size_t *total_bytes, size_t *used_bytes)
{
    int index;
    if (spiffs_by_label(blkpart, &index) != 0) {
        return -EINVAL;
    }
    SPIFFS_info(_efs[index]->fs, (u32_t *)total_bytes, (u32_t *)used_bytes);
    return 0;
}

int spiffs_format(const char *blkdev)
{
    bool partition_was_mounted = false;
    int index;
    /* If the partition is not mounted, need to create SPIFFS structures
     * and mount the partition, unmount, format, delete SPIFFS structures.
     * See SPIFFS wiki for the reason why.
     */
    int err = spiffs_by_label(blkdev, &index);
    if (err != 0) {
        vfs_spiffs_conf_t conf = {
                .format_if_mount_failed = true,
                .blk_name = blkdev,
                .max_files = 1
        };
        err = spiffs_init(&conf);
        if (err != 0) {
            return err;
        }
        err = spiffs_by_label(blkdev, &index);
        assert(err == 0 && "failed to get index of the partition just mounted");
    } else if (SPIFFS_mounted(_efs[index]->fs)) {
        partition_was_mounted = true;
    }

    SPIFFS_unmount(_efs[index]->fs);

    s32_t res = SPIFFS_format(_efs[index]->fs);
    if (res != SPIFFS_OK) {
        pr_err("format failed, %i\n", SPIFFS_errno(_efs[index]->fs));
        SPIFFS_clearerr(_efs[index]->fs);
        /* If the partition was previously mounted, but format failed, don't
         * try to mount the partition back (it will probably fail). On the
         * other hand, if it was not mounted, need to clean up.
         */
        if (!partition_was_mounted)
            spiffs_free(&_efs[index]);
        return res;
    }

    if (partition_was_mounted) {
        res = SPIFFS_mount(_efs[index]->fs, &_efs[index]->cfg, _efs[index]->work,
                            _efs[index]->fds, _efs[index]->fds_sz, _efs[index]->cache,
                            _efs[index]->cache_sz,
                (spiffs_check_callback)spiffs_api_check);
        if (res != SPIFFS_OK) {
            pr_err("mount failed, %i\n", SPIFFS_errno(_efs[index]->fs));
            SPIFFS_clearerr(_efs[index]->fs);
            return res;
        }
    } else {
        spiffs_free(&_efs[index]);
    }
    return 0;
}

static int vfs_spiffs_register(const vfs_spiffs_conf_t * conf)
{
    int index, err;
#ifdef CONFIG_COMPONENT_VFS
    const vfs_t vfs = {
        .flags = VFS_FLAG_CONTEXT_PTR,
        .write_p = &vfs_spiffs_write,
        .lseek_p = &vfs_spiffs_lseek,
        .read_p = &vfs_spiffs_read,
        .open_p = &vfs_spiffs_open,
        .close_p = &vfs_spiffs_close,
        .fstat_p = &vfs_spiffs_fstat,
        .stat_p = &vfs_spiffs_stat,
        .link_p = &vfs_spiffs_link,
        .unlink_p = &vfs_spiffs_unlink,
        .rename_p = &vfs_spiffs_rename,
        .opendir_p = &vfs_spiffs_opendir,
        .closedir_p = &vfs_spiffs_closedir,
        .readdir_p = &vfs_spiffs_readdir,
        .readdir_r_p = &vfs_spiffs_readdir_r,
        .seekdir_p = &vfs_spiffs_seekdir,
        .telldir_p = &vfs_spiffs_telldir,
        .mkdir_p = &vfs_spiffs_mkdir,
        .rmdir_p = &vfs_spiffs_rmdir,
        .statfs_p = &vfs_spiffs_statfs,
        .fstatfs_p = &vfs_spiffs_fstatfs,
        .access_p = &vfs_spiffs_access,
    };
#endif

    assert(conf->base_path);

    //find if such partition is already mounted
    if (spiffs_by_label(conf->blk_name, &index) == 0)
        return 0;

    err = spiffs_init(conf);
    if (err)
        goto err;

    // check if such partition mount ok
    if (spiffs_by_label(conf->blk_name, &index) != 0) {
        pr_err("blk part %s did not mounted\n", conf->blk_name);
        err = -EINVAL;
        goto err;
    }

    strlcat(_efs[index]->base_path, conf->base_path, VFS_PATH_MAX + 1);
#ifdef CONFIG_COMPONENT_VFS
    err = vfs_register(conf->base_path, &vfs, _efs[index]);
    if (err) {
        spiffs_free(&_efs[index]);
        goto err;
    }
#endif

    pr_info("mount spiffs from %s to %s ok\n", conf->blk_name, conf->base_path);
    return 0;
err:
    pr_err("mount spiffs from %s to %s failed\n", conf->blk_name, conf->base_path);
    return err;
}

static int vfs_spiffs_unregister(const char *blkpart)
{
    int index;
#ifdef CONFIG_COMPONENT_VFS
    int err;
#endif
    if (spiffs_by_label(blkpart, &index) != 0)
        return -EINVAL;

#ifdef CONFIG_COMPONENT_VFS
    err = vfs_unregister(_efs[index]->base_path);
    if (err != 0)
        return err;
#endif
    spiffs_free(&_efs[index]);
    return 0;
}

static int spiffs_res_to_errno(s32_t fr)
{
    switch(fr) {
    case SPIFFS_OK :
        return 0;
    case SPIFFS_ERR_NOT_MOUNTED :
        return ENODEV;
    case SPIFFS_ERR_NOT_A_FS :
        return ENODEV;
    case SPIFFS_ERR_FULL :
        return ENOSPC;
    case SPIFFS_ERR_BAD_DESCRIPTOR :
        return EBADF;
    case SPIFFS_ERR_MOUNTED :
        return EEXIST;
    case SPIFFS_ERR_FILE_EXISTS :
        return EEXIST;
    case SPIFFS_ERR_NOT_FOUND :
        return ENOENT;
    case SPIFFS_ERR_NOT_A_FILE :
        return ENOENT;
    case SPIFFS_ERR_DELETED :
        return ENOENT;
    case SPIFFS_ERR_FILE_DELETED :
        return ENOENT;
    case SPIFFS_ERR_NAME_TOO_LONG :
        return ENAMETOOLONG;
    case SPIFFS_ERR_RO_NOT_IMPL :
        return EROFS;
    case SPIFFS_ERR_RO_ABORTED_OPERATION :
        return EROFS;
    default :
        return EIO;
    }
    return ENOTSUP;
}

static int spiffs_mode_conv(int m)
{
    int res = 0;
    int acc_mode = m & O_ACCMODE;

    if (acc_mode == O_RDONLY)
        res |= SPIFFS_O_RDONLY;
    else if (acc_mode == O_WRONLY)
        res |= SPIFFS_O_WRONLY;
    else if (acc_mode == O_RDWR)
        res |= SPIFFS_O_RDWR;

    if (m & O_CREAT)
        res |= SPIFFS_O_CREAT;
    if (m & O_EXCL)
        res |= SPIFFS_O_EXCL;
    if (m & O_TRUNC)
        res |= SPIFFS_O_TRUNC;
    if (m & O_APPEND)
        res |= SPIFFS_O_CREAT | SPIFFS_O_APPEND;
    return res;
}

static int vfs_spiffs_open(void *ctx, const char *path, int flags, int mode)
{
    assert(path);
    spiffs_t *efs = (spiffs_t *)ctx;
    int spiffs_flags = spiffs_mode_conv(flags);
    int fd;

    if (!strlen(path))
        return -1;

    fd = SPIFFS_open(efs->fs, path, spiffs_flags, mode);
    if (fd < 0) {
        errno = spiffs_res_to_errno(SPIFFS_errno(efs->fs));
        SPIFFS_clearerr(efs->fs);
        return -1;
    }
    if (!(spiffs_flags & SPIFFS_RDONLY)) {
        vfs_spiffs_update_mtime(efs->fs, fd);
    }
    return fd;
}

static ssize_t vfs_spiffs_write(void* ctx, int fd, const void * data, size_t size)
{
    spiffs_t * efs = (spiffs_t *)ctx;
    ssize_t res = SPIFFS_write(efs->fs, fd, (void *)data, size);
    if (res < 0) {
        errno = spiffs_res_to_errno(SPIFFS_errno(efs->fs));
        SPIFFS_clearerr(efs->fs);
        return -1;
    }
    return res;
}

static ssize_t vfs_spiffs_read(void* ctx, int fd, void * dst, size_t size)
{
    spiffs_t * efs = (spiffs_t *)ctx;
    ssize_t res = SPIFFS_read(efs->fs, fd, dst, size);
    if (res < 0) {
        errno = spiffs_res_to_errno(SPIFFS_errno(efs->fs));
        SPIFFS_clearerr(efs->fs);
        return -1;
    }
    return res;
}

static int vfs_spiffs_close(void* ctx, int fd)
{
    spiffs_t * efs = (spiffs_t *)ctx;
    int res = SPIFFS_close(efs->fs, fd);
    if (res < 0) {
        errno = spiffs_res_to_errno(SPIFFS_errno(efs->fs));
        SPIFFS_clearerr(efs->fs);
        return -1;
    }
    return res;
}

static off_t vfs_spiffs_lseek(void* ctx, int fd, off_t offset, int mode)
{
    spiffs_t * efs = (spiffs_t *)ctx;
    off_t res = SPIFFS_lseek(efs->fs, fd, offset, mode);
    if (res < 0) {
        errno = spiffs_res_to_errno(SPIFFS_errno(efs->fs));
        SPIFFS_clearerr(efs->fs);
        return -1;
    }
    return res;
}

static int vfs_spiffs_fstat(void* ctx, int fd, struct stat * st)
{
    assert(st);
    spiffs_stat s;
    spiffs_t * efs = (spiffs_t *)ctx;
    off_t res = SPIFFS_fstat(efs->fs, fd, &s);
    if (res < 0) {
        errno = spiffs_res_to_errno(SPIFFS_errno(efs->fs));
        SPIFFS_clearerr(efs->fs);
        return -1;
    }
    st->st_size = s.size;
    st->st_mode = S_IRWXU | S_IRWXG | S_IRWXO | S_IFREG;
    st->st_mtime = vfs_spiffs_get_mtime(&s);
    st->st_atime = 0;
    st->st_ctime = 0;
    return res;
}

static int vfs_spiffs_access(void *cxt, const char *path, int amode)
{
    struct stat s;
    int ret;

    ret = vfs_spiffs_stat(cxt, path, &s);
    if (ret < 0)
        return ret;

    switch (amode) {
    case R_OK: return s.st_mode & S_IRUSR ? 0 : 1;
    case W_OK: return s.st_mode & S_IWUSR ? 0 : 1;
    case X_OK: return s.st_mode & S_IXUSR ? 0 : 1;
    }
    return -EINVAL;
}

static int vfs_spiffs_fstatfs(void *ctx, int fd, struct statfs *buf)
{
    int ret;
    u32_t total_bytes, used_bytes;
    spiffs_t * efs = (spiffs_t *)ctx;

    ret = SPIFFS_info(efs->fs, &total_bytes, &used_bytes);
    if (ret) {
        pr_err("get spiffs info from fd %s failed\n", fd);
        return ret;
    }

    memset(buf, 0, sizeof(*buf));
    buf->f_type = SPIFFS_SUPER_MAGIC;
    buf->f_bsize = CONFIG_SPIFFS_PAGE_SIZE;
    buf->f_blocks = total_bytes / buf->f_bsize;
    buf->f_bfree = (total_bytes - used_bytes) / buf->f_bsize;
    buf->f_bavail = buf->f_bfree;
    buf->f_files = 1024;
    return 0;
}

static int vfs_spiffs_statfs(void *ctx, const char *path, struct statfs *buf)
{
    int ret, index;
    u32_t total_bytes, used_bytes;

    ret = spiffs_by_mountpoint(path, &index);
    if (ret) {
        pr_err("%s is not invalid mountpoint\n", path);
        return -EINVAL;
    }
    ret = SPIFFS_info(_efs[index]->fs, &total_bytes, &used_bytes);
    if (ret) {
        pr_err("get spiffs info from %s failed\n", path);
        return ret;
    }

    memset(buf, 0, sizeof(*buf));
    buf->f_type = SPIFFS_SUPER_MAGIC;
    buf->f_bsize = CONFIG_SPIFFS_PAGE_SIZE;
    buf->f_blocks = total_bytes / buf->f_bsize;
    buf->f_bfree = (total_bytes - used_bytes) / buf->f_bsize;
    buf->f_bavail = buf->f_bfree;
    buf->f_files = 1024;
    return 0;
}

static int vfs_spiffs_stat(void *ctx, const char *path, struct stat *st)
{
    assert(path);
    assert(st);
    spiffs_stat s;
    spiffs_t * efs = (spiffs_t *)ctx;
    off_t res;

    if (!strcmp(path, "/")) {
        st->st_size = 0;
        st->st_mode = S_IRWXU | S_IRWXG | S_IRWXO;
        st->st_mode |= S_IFDIR;
        st->st_mtime = 0;
        st->st_atime = 0;
        st->st_ctime = 0;
        return 0;
    }

    res = SPIFFS_stat(efs->fs, path, &s);
    if (res < 0) {
        errno = spiffs_res_to_errno(SPIFFS_errno(efs->fs));
        SPIFFS_clearerr(efs->fs);
        return -1;
    }

    st->st_size = s.size;
    st->st_mode = S_IRWXU | S_IRWXG | S_IRWXO;
    st->st_mode |= (s.type == SPIFFS_TYPE_DIR) ? S_IFDIR : S_IFREG;
    st->st_mtime = vfs_spiffs_get_mtime(&s);
    st->st_atime = 0;
    st->st_ctime = 0;
    return res;
}

static int vfs_spiffs_rename(void *ctx, const char *src, const char *dst)
{
    assert(src);
    assert(dst);
    spiffs_t *efs = (spiffs_t *)ctx;
    int res;

    res = SPIFFS_rename(efs->fs, src, dst);
    if (res < 0) {
        errno = spiffs_res_to_errno(SPIFFS_errno(efs->fs));
        SPIFFS_clearerr(efs->fs);
        return -1;
    }
    return res;
}

static int vfs_spiffs_unlink(void* ctx, const char *path)
{
    assert(path);
    spiffs_t * efs = (spiffs_t *)ctx;
    int res = SPIFFS_remove(efs->fs, path);
    if (res < 0) {
        errno = spiffs_res_to_errno(SPIFFS_errno(efs->fs));
        SPIFFS_clearerr(efs->fs);
        return -1;
    }
    return res;
}

static DIR *vfs_spiffs_opendir(void *ctx, const char *name)
{
    assert(name);
    spiffs_t *efs = (spiffs_t *)ctx;
    vfs_spiffs_dir_t *dir = calloc(1, sizeof(vfs_spiffs_dir_t));
    if (!dir) {
        errno = ENOMEM;
        return NULL;
    }
    if (!SPIFFS_opendir(efs->fs, name, &dir->d)) {
        free(dir);
        errno = spiffs_res_to_errno(SPIFFS_errno(efs->fs));
        SPIFFS_clearerr(efs->fs);
        return NULL;
    }
    dir->offset = 0;
    strlcpy(dir->path, name, SPIFFS_OBJ_NAME_LEN);
    return (DIR *)dir;
}

static int vfs_spiffs_closedir(void *ctx, DIR *pdir)
{
    assert(pdir);
    spiffs_t * efs = (spiffs_t *)ctx;
    vfs_spiffs_dir_t * dir = (vfs_spiffs_dir_t *)pdir;
    int res = SPIFFS_closedir(&dir->d);
    free(dir);
    if (res < 0) {
        errno = spiffs_res_to_errno(SPIFFS_errno(efs->fs));
        SPIFFS_clearerr(efs->fs);
        return -1;
    }
    return res;
}

static struct dirent *vfs_spiffs_readdir(void *ctx, DIR *pdir)
{
    assert(pdir);
    vfs_spiffs_dir_t * dir = (vfs_spiffs_dir_t *)pdir;
    struct dirent* out_dirent;
    int err = vfs_spiffs_readdir_r(ctx, pdir, &dir->e, &out_dirent);
    if (err != 0) {
        errno = err;
        return NULL;
    }
    return out_dirent;
}

static int vfs_spiffs_readdir_r(void* ctx, DIR* pdir, struct dirent* entry,
                                struct dirent** out_dirent)
{
    assert(pdir);
    spiffs_t * efs = (spiffs_t *)ctx;
    vfs_spiffs_dir_t * dir = (vfs_spiffs_dir_t *)pdir;
    struct spiffs_dirent out;
    size_t plen;
    char * item_name;

    do {
        if (SPIFFS_readdir(&dir->d, &out) == 0) {
            errno = spiffs_res_to_errno(SPIFFS_errno(efs->fs));
            SPIFFS_clearerr(efs->fs);
            if (!errno)
                *out_dirent = NULL;
            return errno;
        }
        item_name = (char *)out.name;
        plen = strlen(dir->path);
        if (plen <= 1)
            break;
        if (!strncasecmp(dir->path, (const char*)out.name, plen)) {
            if (out.name[plen] == '/' && out.name[plen + 1])
                break;
        }
    } while (true);

    if (plen > 1)
        item_name += plen + 1;
    else if (item_name[0] == '/')
        item_name++;
    entry->d_ino = 0;
    entry->d_type = out.type;
    snprintf(entry->d_name, SPIFFS_OBJ_NAME_LEN, "%s", item_name);
    dir->offset++;
    *out_dirent = entry;
    return 0;
}

static long vfs_spiffs_telldir(void* ctx, DIR* pdir)
{
    assert(pdir);
    vfs_spiffs_dir_t * dir = (vfs_spiffs_dir_t *)pdir;
    return dir->offset;
}

static void vfs_spiffs_seekdir(void* ctx, DIR* pdir, long offset)
{
    assert(pdir);
    spiffs_t * efs = (spiffs_t *)ctx;
    vfs_spiffs_dir_t * dir = (vfs_spiffs_dir_t *)pdir;
    struct spiffs_dirent tmp;
    if (offset < dir->offset) {
        //rewind dir
        SPIFFS_closedir(&dir->d);
        if (!SPIFFS_opendir(efs->fs, NULL, &dir->d)) {
            errno = spiffs_res_to_errno(SPIFFS_errno(efs->fs));
            SPIFFS_clearerr(efs->fs);
            return;
        }
        dir->offset = 0;
    }
    while (dir->offset < offset) {
        if (SPIFFS_readdir(&dir->d, &tmp) == 0) {
            errno = spiffs_res_to_errno(SPIFFS_errno(efs->fs));
            SPIFFS_clearerr(efs->fs);
            return;
        }
        size_t plen = strlen(dir->path);
        if (plen > 1) {
            if (strncasecmp(dir->path, (const char *)tmp.name, plen) || tmp.name[plen] != '/' || !tmp.name[plen+1]) {
                continue;
            }
        }
        dir->offset++;
    }
}

static int vfs_spiffs_mkdir(void* ctx, const char* name, mode_t mode)
{
    errno = ENOTSUP;
    return -1;
}

static int vfs_spiffs_rmdir(void* ctx, const char* name)
{
    errno = ENOTSUP;
    return -1;
}

static int vfs_spiffs_link(void* ctx, const char* n1, const char* n2)
{
    errno = ENOTSUP;
    return -1;
}

static void vfs_spiffs_update_mtime(spiffs *fs, spiffs_file fd)
{
#ifdef CONFIG_SPIFFS_USE_MTIME
    time_t t = time(NULL);
    spiffs_stat s;
    int ret = SPIFFS_OK;
    if (CONFIG_SPIFFS_META_LENGTH > sizeof(t)) {
        ret = SPIFFS_fstat(fs, fd, &s);
    }
    if (ret == SPIFFS_OK) {
        memcpy(s.meta, &t, sizeof(t));
        ret = SPIFFS_fupdate_meta(fs, fd, s.meta);
    }
    if (ret != SPIFFS_OK) {
        pr_warn("Failed to update mtime (%d)", ret);
    }
#endif //CONFIG_SPIFFS_USE_MTIME
}

static time_t vfs_spiffs_get_mtime(const spiffs_stat* s)
{
    time_t t = 0;
#ifdef CONFIG_SPIFFS_USE_MTIME
    memcpy(&t, s->meta, sizeof(t));
#endif
    return t;
}

int spiffs_mount(const char *source, const char *target, bool format)
{
    vfs_spiffs_conf_t spiffs_conf = {
        .base_path = target,
        .blk_name = source,
        .max_files = 1024,
        .format_if_mount_failed = format,
    };

    if (!strncmp(spiffs_conf.blk_name, "/dev/", sizeof("/dev/") - 1))
        spiffs_conf.blk_name += sizeof("/dev/") - 1;

    return vfs_spiffs_register(&spiffs_conf);
}

int spiffs_umount(const char *target)
{
    int ret, index;

    ret = spiffs_by_mountpoint(target, &index);
    if (ret) {
        pr_err("%s is not invalid mountpoint\n", target);
        return -EINVAL;
    }

    return vfs_spiffs_unregister(_efs[index]->part->name);
}
