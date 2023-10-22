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
// working directory ports from rt-thread project

#define pr_fmt(fmt) "vfs: " fmt
#include <FreeRTOS.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/unistd.h>
#include <sys/param.h>
#include <dirent.h>
#include <stdbool.h>
#include <semphr.h>
#include <awlog.h>
#include <portmacro.h>
#include <errno.h>
#include <vfs.h>
#include <sys/reent.h>
#include <sys/vfs.h>

#include <sunxi_hal_common.h>

#define STDIO_FD_OFF 3
#define VFS_MAX_COUNT   8   /* max number of VFS entries (registered filesystems) */
#define LEN_PATH_PREFIX_IGNORED SIZE_MAX /* special length value for VFS which is never recognised by open() */
#define FD_TABLE_ENTRY_UNUSED   (fd_table_t) { .permanent = false, .vfs_index = -1, .local_fd = -1 }
//#define _errno_r(ptr) (((struct _reent *)(ptr))->_errno)
#define _errno_r(ptr) (errno)

typedef uint8_t local_fd_t;

typedef int8_t vfs_index_t;
_Static_assert((1 << (sizeof(vfs_index_t)*8)) >= VFS_MAX_COUNT, "VFS index type too small");
_Static_assert(((vfs_index_t) -1) < 0, "vfs_index_t must be a signed type");

typedef struct {
    bool permanent;
    vfs_index_t vfs_index;
    local_fd_t local_fd;
} fd_table_t;

typedef struct vfs_entry_ {
    vfs_t vfs;          // contains pointers to VFS functions
    char path_prefix[VFS_PATH_MAX]; // path prefix mapped to this VFS
    size_t path_prefix_len; // micro-optimization to avoid doing extra strlen
    void *ctx;              // optional pointer which can be passed to VFS
    int offset;             // index of this structure in s_vfs array
} vfs_entry_t;

struct rootdir {
    union {
        DIR dir;
        char padding[64];
    };
    struct dirent e;
} g_rdir __attribute__((aligned(64)));

static vfs_entry_t *s_vfs[VFS_MAX_COUNT] = { 0 };
static size_t s_vfs_count = 0;

static fd_table_t s_fd_table[MAX_FDS] = { [0 ... MAX_FDS-1] = FD_TABLE_ENTRY_UNUSED };
static SemaphoreHandle_t s_fd_table_lock = NULL;
static SemaphoreHandle_t s_workdir_lock = NULL;

static int vfs_tb_lock(void)
{
    if (s_fd_table_lock == NULL) {
        s_fd_table_lock = xSemaphoreCreateMutex();
        if (s_fd_table_lock == NULL)
            return -ENOMEM;
    }

    if (xSemaphoreTake(s_fd_table_lock, portMAX_DELAY) == pdTRUE)
        return 0;
    return -EBUSY;
}

static int vfs_tb_unlock(void)
{
    if (s_fd_table_lock != NULL) {
        if (xSemaphoreGive(s_fd_table_lock) == pdTRUE)
            return 0;
        else
            return -EBUSY;
    }
    return -EINVAL;
}

static int vfs_register_common(const char *base_path, size_t len,
        const vfs_t *vfs, void *ctx, int *vfs_index)
{
    size_t index;
    vfs_entry_t *entry;

    if (len != LEN_PATH_PREFIX_IGNORED) {
        if ((len != 0 && len < 2) || (len > VFS_PATH_MAX)) {
            return -EINVAL;
        }
        if ((len > 0 && base_path[0] != '/') || base_path[len - 1] == '/') {
            return -EINVAL;
        }
    }

    entry = (vfs_entry_t*) malloc(sizeof(vfs_entry_t));
    if (entry == NULL)
        return -ENOMEM;

    for (index = 0; index < s_vfs_count; ++index) {
        if (s_vfs[index] == NULL)
            break;
    }
    if (index == s_vfs_count) {
        if (s_vfs_count >= VFS_MAX_COUNT) {
            free(entry);
            return -ENOMEM;
        }
        ++s_vfs_count;
    }
    s_vfs[index] = entry;

    if (len != LEN_PATH_PREFIX_IGNORED)
        strcpy(entry->path_prefix, base_path);
    else
        bzero(entry->path_prefix, sizeof(entry->path_prefix));
    memcpy(&entry->vfs, vfs, sizeof(vfs_t));
    entry->path_prefix_len = len;
    entry->ctx = ctx;
    entry->offset = index;

    if (vfs_index)
        *vfs_index = index;

    return 0;
}

int vfs_register(const char *base_path, const vfs_t *vfs, void *ctx)
{
    return vfs_register_common(base_path, strlen(base_path), vfs, ctx, NULL);
}

int vfs_register_fd_range(const vfs_t *vfs, void *ctx, int min_fd, int max_fd)
{
    int index = -1;
    int ret;

    if (min_fd < 0 || max_fd < 0 || min_fd > MAX_FDS || max_fd > MAX_FDS || min_fd > max_fd) {
        pr_debug("Invalid arguments: vfs_register_fd_range(0x%x, 0x%x, %d, %d)",
                (int) vfs, (int) ctx, min_fd, max_fd);
        return -EINVAL;
    }

    ret = vfs_register_common("", LEN_PATH_PREFIX_IGNORED, vfs, ctx, &index);

    if (ret == 0) {
        vfs_tb_lock();
        for (int i = min_fd; i < max_fd; ++i) {
            if (s_fd_table[i].vfs_index != -1) {
                free(s_vfs[i]);
                s_vfs[i] = NULL;
                for (int j = min_fd; j < i; ++j) {
                    if (s_fd_table[j].vfs_index == index) {
                        s_fd_table[j] = FD_TABLE_ENTRY_UNUSED;
                    }
                }
                vfs_tb_unlock();
                pr_debug("vfs_register_fd_range cannot set fd %d (used by other VFS)", i);
                return -EINVAL;
            }
            s_fd_table[i].permanent = true;
            s_fd_table[i].vfs_index = index;
            s_fd_table[i].local_fd = i;
        }
        vfs_tb_unlock();
    }

    pr_debug("vfs_register_fd_range is successful for range <%d; %d) and VFS ID %d", min_fd, max_fd, index);

    return ret;
}

int vfs_register_with_id(const vfs_t *vfs, void *ctx, vfs_id_t *vfs_id)
{
    if (vfs_id == NULL) {
        return -EINVAL;
    }

    *vfs_id = -1;
    return vfs_register_common("", LEN_PATH_PREFIX_IGNORED, vfs, ctx, vfs_id);
}

int vfs_unregister(const char *base_path)
{
    const size_t base_path_len = strlen(base_path);

    for (size_t i = 0; i < s_vfs_count; ++i) {
        vfs_entry_t *vfs = s_vfs[i];
        if (vfs == NULL) {
            continue;
        }
        if (base_path_len == vfs->path_prefix_len &&
                memcmp(base_path, vfs->path_prefix, vfs->path_prefix_len) == 0) {
            free(vfs);
            s_vfs[i] = NULL;

            vfs_tb_lock();
            for (int j = 0; j < MAX_FDS; ++j) {
                if (s_fd_table[j].vfs_index == i) {
                    s_fd_table[j] = FD_TABLE_ENTRY_UNUSED;
                }
            }
            vfs_tb_unlock();

            return 0;
        }
    }
    return -EINVAL;
}

int vfs_register_fd(vfs_id_t vfs_id, int *fd)
{
    int ret = -ENOMEM;

    if (vfs_id < 0 || vfs_id >= s_vfs_count || fd == NULL) {
        pr_debug("Invalid arguments for vfs_register_fd(%d, 0x%x)", vfs_id, (int) fd);
        return -EINVAL;
    }

    vfs_tb_lock();
    for (int i = 0; i < MAX_FDS; ++i) {
        if (s_fd_table[i].vfs_index == -1) {
            s_fd_table[i].permanent = true;
            s_fd_table[i].vfs_index = vfs_id;
            s_fd_table[i].local_fd = i;
            *fd = i;
            ret = 0;
            break;
        }
    }
    vfs_tb_unlock();

    pr_debug("vfs_register_fd(%d, 0x%x) finished with %d", vfs_id, (int) fd, ret);

    return ret;
}

int vfs_unregister_fd(vfs_id_t vfs_id, int fd)
{
    int ret = -EINVAL;

    if (vfs_id < 0 || vfs_id >= s_vfs_count || fd < 0 || fd >= MAX_FDS) {
        pr_debug("Invalid arguments for vfs_unregister_fd(%d, %d)", vfs_id, fd);
        return ret;
    }

    vfs_tb_lock();
    fd_table_t *item = s_fd_table + fd;
    if (item->permanent == true && item->vfs_index == vfs_id && item->local_fd == fd) {
        *item = FD_TABLE_ENTRY_UNUSED;
        ret = 0;
    }
    vfs_tb_unlock();

    pr_debug("vfs_unregister_fd(%d, %d) finished with %d", vfs_id, fd, ret);

    return ret;
}

static inline const vfs_entry_t *get_vfs_for_index(int index)
{
    if (index < 0 || index >= s_vfs_count) {
        return NULL;
    } else {
        return s_vfs[index];
    }
}

static inline bool fd_valid(int fd)
{
    return (fd < MAX_FDS) && (fd >= 0);
}

static const vfs_entry_t *get_vfs_for_fd(int fd)
{
    const vfs_entry_t *vfs = NULL;
    int index;

    fd -= STDIO_FD_OFF;

    if (fd_valid(fd)) {
        index = s_fd_table[fd].vfs_index; // single read -> no locking is required
        vfs = get_vfs_for_index(index);
    }
    return vfs;
}

static inline int get_local_fd(const vfs_entry_t *vfs, int fd)
{
    int local_fd = -1;

    fd -= STDIO_FD_OFF;

    if (vfs && fd_valid(fd))
        local_fd = s_fd_table[fd].local_fd;

    return local_fd;
}

static const char *translate_path(const vfs_entry_t *vfs, const char *src_path)
{
    assert(strncmp(src_path, vfs->path_prefix, vfs->path_prefix_len) == 0);
    if (strlen(src_path) == vfs->path_prefix_len) {
        // special case when src_path matches the path prefix exactly
        return "/";
    }
    return src_path + vfs->path_prefix_len;
}

static const vfs_entry_t *get_vfs_for_path(const char *path)
{
    const vfs_entry_t *best_match = NULL;
    ssize_t best_match_prefix_len = -1;
    size_t len = strlen(path);
    size_t i;

    for (i = 0; i < s_vfs_count; ++i) {
        const vfs_entry_t *vfs = s_vfs[i];

        if (!vfs || vfs->path_prefix_len == LEN_PATH_PREFIX_IGNORED) {
            continue;
        }
        // match path prefix
        if (len < vfs->path_prefix_len ||
            memcmp(path, vfs->path_prefix, vfs->path_prefix_len) != 0) {
            continue;
        }
        // this is the default VFS and we don't have a better match yet.
        if (vfs->path_prefix_len == 0 && !best_match) {
            best_match = vfs;
            continue;
        }
        // if path is not equal to the prefix, expect to see a path separator
        // i.e. don't match "/data" prefix for "/data1/foo.txt" path
        if (len > vfs->path_prefix_len &&
                path[vfs->path_prefix_len] != '/') {
            continue;
        }
        // Out of all matching path prefixes, select the longest one;
        // i.e. if "/dev" and "/dev/uart" both match, for "/dev/uart/1" path,
        // choose "/dev/uart",
        // This causes all s_vfs_count VFS entries to be scanned when opening
        // a file by name. This can be optimized by introducing a table for
        // FS search order, sorted so that longer prefixes are checked first.
        if (best_match_prefix_len < (ssize_t) vfs->path_prefix_len) {
            best_match_prefix_len = (ssize_t) vfs->path_prefix_len;
            best_match = vfs;
        }
    }
    return best_match;
}

/*
  *Using huge multi-line macros is never nice, but in this case
  *the only alternative is to repeat this chunk of code (with different function names)
  *for each syscall being implemented. Given that this define is contained within a single
  *file, this looks like a good tradeoff.
 *
  *First we check if syscall is implemented by VFS (corresponding member is not NULL),
  *then call the right flavor of the method (e.g. open or open_p) depending on
  *VFS_FLAG_CONTEXT_PTR flag. If VFS_FLAG_CONTEXT_PTR is set, context is passed
  *in as first argument and _p variant is used for the call.
  *It is enough to check just one of them for NULL, as both variants are part of a union.
 */
#define CHECK_AND_CALL(ret, r, pvfs, func, ...) \
    if (pvfs->vfs.func == NULL) { \
        if (r) \
            _errno_r(((struct _reent *)r)) = ENOSYS; \
        return -1; \
    } \
    if (pvfs->vfs.flags & VFS_FLAG_CONTEXT_PTR) { \
        ret = (*pvfs->vfs.func ## _p)(pvfs->ctx, __VA_ARGS__); \
    } else { \
        ret = (*pvfs->vfs.func)(__VA_ARGS__);\
    }


#define CHECK_AND_CALLV(r, pvfs, func, ...) \
    if (pvfs->vfs.func == NULL) { \
        if (r) \
            _errno_r(((struct _reent *)r)) = ENOSYS; \
        return; \
    } \
    if (pvfs->vfs.flags & VFS_FLAG_CONTEXT_PTR) { \
        (*pvfs->vfs.func ## _p)(pvfs->ctx, __VA_ARGS__); \
    } else { \
        (*pvfs->vfs.func)(__VA_ARGS__);\
    }

#define CHECK_AND_CALLP(ret, r, pvfs, func, ...) \
    if (pvfs->vfs.func == NULL) { \
        if (r != NULL) \
            _errno_r(((struct _reent *)r)) = ENOSYS; \
        return NULL; \
    } \
    if (pvfs->vfs.flags & VFS_FLAG_CONTEXT_PTR) { \
        ret = (*pvfs->vfs.func ## _p)(pvfs->ctx, __VA_ARGS__); \
    } else { \
        ret = (*pvfs->vfs.func)(__VA_ARGS__);\
    }

#ifdef CONFIG_USING_WORKDIR

#define WORK_DIR_PATH_MAX 256
char working_directory[WORK_DIR_PATH_MAX] = {"/"};

static int vfs_workdir_lock(void)
{
    if (s_workdir_lock == NULL) {
        s_workdir_lock = xSemaphoreCreateMutex();
        if (s_workdir_lock == NULL)
            return -ENOMEM;
    }

    if (xSemaphoreTake(s_workdir_lock, portMAX_DELAY) == pdTRUE)
        return 0;
    return -EBUSY;
}

static int vfs_workdir_unlock(void)
{
    if (s_workdir_lock != NULL) {
        if (xSemaphoreGive(s_workdir_lock) == pdTRUE)
            return 0;
        else
            return -EBUSY;
    }
    return -EINVAL;
}
#endif

char *getcwd(char *buf, size_t size)
{
#ifdef CONFIG_USING_WORKDIR
    vfs_workdir_lock();
    strncpy(buf, working_directory, MIN(sizeof(working_directory), size));
    vfs_workdir_unlock();
#endif
    return buf;
}

void vfs_workdir_put_path(char *path)
{
#ifdef CONFIG_USING_WORKDIR
    free(path);
#endif
}

char *vfs_workdir_get_path(const char *directory, const char *filename)
{
#ifdef CONFIG_USING_WORKDIR
    char *fullpath;
    char *dst0, *dst, *src;

    if (filename == NULL)
        return NULL;

    if (directory == NULL)
        directory = &working_directory[0];

    if (filename[0] != '/') /* it's a absolute path, use it directly */
    {
        fullpath = (char *)malloc(strlen(directory) + strlen(filename) + 2);

        if (fullpath == NULL)
            return NULL;

        /* join path and file name */
        snprintf(fullpath, strlen(directory) + strlen(filename) + 2,
                    "%s/%s", directory, filename);
    }
    else
    {
        fullpath = strdup(filename); /* copy string */

        if (fullpath == NULL)
            return NULL;
    }

    src = fullpath;
    dst = fullpath;

    dst0 = dst;
    while (1)
    {
        char c = *src;

        if (c == '.')
        {
            if (!src[1]) src ++; /* '.' and ends */
            else if (src[1] == '/')
            {
                /* './' case */
                src += 2;

                while ((*src == '/') && (*src != '\0'))
                    src ++;
                continue;
            }
            else if (src[1] == '.')
            {
                if (!src[2])
                {
                    /* '..' and ends case */
                    src += 2;
                    goto up_one;
                }
                else if (src[2] == '/')
                {
                    /* '../' case */
                    src += 3;

                    while ((*src == '/') && (*src != '\0'))
                        src ++;
                    goto up_one;
                }
            }
        }

        /* copy up the next '/' and erase all '/' */
        while ((c = *src++) != '\0' && c != '/')
            *dst ++ = c;

        if (c == '/')
        {
            *dst ++ = '/';
            while (c == '/')
                c = *src++;

            src --;
        }
        else if (!c)
            break;

        continue;

up_one:
        dst --;
        if (dst < dst0)
        {
            free(fullpath);
            return NULL;
        }
        while (dst0 < dst && dst[-1] != '/')
            dst --;
    }

    *dst = '\0';

    /* remove '/' in the end of path if exist */
    dst --;
    if ((dst != fullpath) && (*dst == '/'))
        *dst = '\0';

    /* final check fullpath is not empty, for the special path of lwext "/.." */
    if ('\0' == fullpath[0])
    {
        fullpath[0] = '/';
        fullpath[1] = '\0';
    }

    return fullpath;
#else
    return (char *)filename;
#endif
}

int chdir(const char *path)
{
#ifdef CONFIG_USING_WORKDIR
	char *fullpath;
	DIR *d;

	if (path == NULL)
	{
		vfs_workdir_lock();
		printf("%s\n", working_directory);
		vfs_workdir_unlock();
		return 0;
	}

	if (strlen(path) > WORK_DIR_PATH_MAX)
	{
		return -1;
	}

	fullpath = vfs_workdir_get_path(NULL, path);
	if (fullpath == NULL)
	{
		return -1; /* build path failed */
	}

	vfs_workdir_lock();
	d = opendir(fullpath);
	if (d == NULL)
	{
		vfs_workdir_put_path(fullpath);
		vfs_workdir_unlock();

		return -1;
	}

	/* close directory stream */
	closedir(d);

	/* copy full path to working directory */
	strncpy(working_directory, fullpath, sizeof(working_directory) - 1);
	/* release normalize directory path name */
	vfs_workdir_put_path(fullpath);

	vfs_workdir_unlock();
#endif
	return 0;
}

int vfs_open(struct _reent *r, const char  *path, int flags, int mode)
{
    const vfs_entry_t *vfs;
    const char *path_within_vfs;
    int fd_within_vfs, ret, i;
    char *fullpath;

	fullpath = vfs_workdir_get_path(NULL, path);
    if (fullpath == NULL) {
        return -ENOMEM;
    }

    vfs = get_vfs_for_path(fullpath);
    if (vfs == NULL) {
        _errno_r(((struct _reent *)r)) = ENOENT;
        vfs_workdir_put_path(fullpath);
        return -1;
    }

    path_within_vfs = translate_path(vfs, fullpath);

    if (!strlen(path_within_vfs)) {
        _errno_r(((struct _reent *)r)) = ENOENT;
        vfs_workdir_put_path(fullpath);
        return -1;
    }

    CHECK_AND_CALL(fd_within_vfs, r, vfs, open, path_within_vfs, flags, mode);
    if (fd_within_vfs >= 0) {

        vfs_tb_lock();
        for (i = 0; i < MAX_FDS; ++i) {
            if (s_fd_table[i].vfs_index == -1) {
                s_fd_table[i].permanent = false;
                s_fd_table[i].vfs_index = vfs->offset;
                s_fd_table[i].local_fd = fd_within_vfs;
                vfs_tb_unlock();
                vfs_workdir_put_path(fullpath);
                return i + STDIO_FD_OFF;
            }
        }
        vfs_tb_unlock();

        CHECK_AND_CALL(ret, r, vfs, close, fd_within_vfs);
        (void) ret;
        _errno_r(((struct _reent *)r)) = ENOMEM;
        vfs_workdir_put_path(fullpath);
        return -1;
    }

    _errno_r(((struct _reent *)r)) = ENOENT;
    vfs_workdir_put_path(fullpath);
    return -1;
}

ssize_t vfs_write(struct _reent *r, int fd, const void  *data, size_t size)
{
    const vfs_entry_t *vfs = get_vfs_for_fd(fd);
    const int local_fd = get_local_fd(vfs, fd);
    ssize_t ret;

    if (vfs == NULL || local_fd < 0) {
        _errno_r(((struct _reent *)r)) = EBADF;
        return -1;
    }
    CHECK_AND_CALL(ret, r, vfs, write, local_fd, data, size);
    return ret;
}

off_t vfs_lseek(struct _reent *r, int fd, off_t size, int mode)
{
    off_t ret;
    const vfs_entry_t *vfs = get_vfs_for_fd(fd);
    const int local_fd = get_local_fd(vfs, fd);

    if (vfs == NULL || local_fd < 0) {
        _errno_r(((struct _reent *)r)) = EBADF;
        return -1;
    }

    CHECK_AND_CALL(ret, r, vfs, lseek, local_fd, size, mode);
    return ret;
}

ssize_t vfs_read(struct _reent *r, int fd, void  *dst, size_t size)
{
    const vfs_entry_t *vfs = get_vfs_for_fd(fd);
    const int local_fd = get_local_fd(vfs, fd);
    ssize_t ret;

    if (vfs == NULL || local_fd < 0) {
        _errno_r(((struct _reent *)r)) = EBADF;
        return -1;
    }

    CHECK_AND_CALL(ret, r, vfs, read, local_fd, dst, size);
    return ret;
}


int vfs_close(struct _reent *r, int fd)
{
    const vfs_entry_t *vfs = get_vfs_for_fd(fd);
    const int local_fd = get_local_fd(vfs, fd);
    int ret;

    if (vfs == NULL || local_fd < 0) {
        _errno_r(((struct _reent *)r)) = EBADF;
        return -1;
    }

    CHECK_AND_CALL(ret, r, vfs, close, local_fd);

    /* skip the fds for stdin/stdout/stderr */
    fd -= STDIO_FD_OFF;

    vfs_tb_lock();
    if (!s_fd_table[fd].permanent)
        s_fd_table[fd] = FD_TABLE_ENTRY_UNUSED;
    vfs_tb_unlock();
    return ret;
}

int vfs_fstat(struct _reent *r, int fd, struct stat  *st)
{
    int ret;
    const vfs_entry_t *vfs = get_vfs_for_fd(fd);
    const int local_fd = get_local_fd(vfs, fd);

    if (vfs == NULL || local_fd < 0) {
        _errno_r(((struct _reent *)r)) = EBADF;
        return -1;
    }

    CHECK_AND_CALL(ret, r, vfs, fstat, local_fd, st);
    return ret;
}

int vfs_stat(struct _reent *r, const char  *path, struct stat  *st)
{
    const char *path_within_vfs;
    const vfs_entry_t *vfs;
    int ret;
    char *fullpath;

    fullpath = vfs_workdir_get_path(NULL, path);
    if (fullpath == NULL) {
        return -ENOMEM;
    }

    if (strcmp(fullpath, "/") == 0) {
        st->st_size = 0;
        st->st_mode = S_IRWXU | S_IRWXG | S_IRWXO | S_IFDIR;
        vfs_workdir_put_path(fullpath);
        return 0;
    }

    vfs = get_vfs_for_path(fullpath);
    if (vfs == NULL) {
        _errno_r(((struct _reent *)r)) = EBADF;
        vfs_workdir_put_path(fullpath);
        return -1;
    }
    path_within_vfs = translate_path(vfs, fullpath);
    CHECK_AND_CALL(ret, r, vfs, stat, path_within_vfs, st);
    vfs_workdir_put_path(fullpath);
    return ret;
}

int vfs_link(struct _reent *r, const char *n1, const char *n2)
{
    const vfs_entry_t *vfs;
    const vfs_entry_t *vfs2;
    const char *path1_within_vfs;
    const char *path2_within_vfs;
    int ret;
    char *fulln1, *fulln2;

    fulln1 = vfs_workdir_get_path(NULL, n1);
    if (fulln1 == NULL) {
        return -ENOMEM;
    }

    fulln2 = vfs_workdir_get_path(NULL, n2);
    if (fulln2 == NULL) {
        vfs_workdir_put_path(fulln1);
        return -ENOMEM;
    }

    vfs = get_vfs_for_path(fulln1);
    if (vfs == NULL) {
        _errno_r(((struct _reent *)r)) = ENOENT;
        vfs_workdir_put_path(fulln1);
        vfs_workdir_put_path(fulln2);
        return -1;
    }

    vfs2 = get_vfs_for_path(fulln2);
    if (vfs != vfs2) {
        _errno_r(((struct _reent *)r)) = ENOENT;
        vfs_workdir_put_path(fulln1);
        vfs_workdir_put_path(fulln2);
        return -1;
    }

    path1_within_vfs = translate_path(vfs, fulln1);
    path2_within_vfs = translate_path(vfs, fulln2);

    CHECK_AND_CALL(ret, r, vfs, link, path1_within_vfs, path2_within_vfs);

	vfs_workdir_put_path(fulln1);
    vfs_workdir_put_path(fulln2);

    return ret;
}

int vfs_unlink(struct _reent *r, const char *path)
{
    const vfs_entry_t *vfs;
    const char *path_within_vfs;
    int ret;
    char *fullpath;

    fullpath = vfs_workdir_get_path(NULL, path);
    if (fullpath == NULL) {
        return -ENOMEM;
    }

    vfs = get_vfs_for_path(fullpath);
    if (vfs == NULL) {
        _errno_r(((struct _reent *)r)) = ENOENT;
        vfs_workdir_put_path(fullpath);
        return -1;
    }

    path_within_vfs = translate_path(vfs, fullpath);
    CHECK_AND_CALL(ret, r, vfs, unlink, path_within_vfs);
    vfs_workdir_put_path(fullpath);
    return ret;
}

int vfs_rename(struct _reent *r, const char *src, const char *dst)
{
    const vfs_entry_t *vfs;
    const vfs_entry_t *vfs_dst;
    const char *src_within_vfs;
    const char *dst_within_vfs;
    int ret;
	char *fullsrc, *fulldst;

    fullsrc = vfs_workdir_get_path(NULL, src);
    if (fullsrc == NULL) {
        return -ENOMEM;
    }

    fulldst = vfs_workdir_get_path(NULL, dst);
    if (fulldst == NULL) {
        free(fullsrc);
        return -ENOMEM;
    }

    vfs = get_vfs_for_path(fullsrc);
    if (vfs == NULL) {
        _errno_r(((struct _reent *)r)) = ENOENT;
        vfs_workdir_put_path(fullsrc);
        vfs_workdir_put_path(fulldst);
        return -1;
    }

    vfs_dst = get_vfs_for_path(fulldst);
    if (vfs != vfs_dst) {
        _errno_r(((struct _reent *)r)) = EXDEV;
        vfs_workdir_put_path(fullsrc);
        vfs_workdir_put_path(fulldst);
        return -1;
    }

    src_within_vfs = translate_path(vfs, fullsrc);
    dst_within_vfs = translate_path(vfs, fulldst);
    CHECK_AND_CALL(ret, r, vfs, rename, src_within_vfs, dst_within_vfs);
    vfs_workdir_put_path(fullsrc);
    vfs_workdir_put_path(fulldst);

    return ret;
}

static DIR *opendir_root(void)
{
    struct rootdir *rdir = &g_rdir;

    memset(rdir, 0, sizeof(*rdir));
    return (DIR *)rdir;
}

DIR *opendir(const char *name)
{
    const vfs_entry_t *vfs;
    const char *path_within_vfs;
    DIR *ret;
    char *fullname;

    fullname = vfs_workdir_get_path(NULL, name);
    if (fullname == NULL) {
        return NULL;
    }

    if (strcmp(name, "/") == 0)
    {
        vfs_workdir_put_path(fullname);
        return opendir_root();
    }

    vfs = get_vfs_for_path(fullname);
    if (vfs == NULL) {
        errno = ENOENT;
        vfs_workdir_put_path(fullname);
        return NULL;
    }

    path_within_vfs = translate_path(vfs, fullname);
    CHECK_AND_CALLP(ret, NULL, vfs, opendir, path_within_vfs);
    if (ret != NULL)
        ret->dd_vfs_idx = vfs->offset;

    vfs_workdir_put_path(fullname);
    return ret;
}

static struct dirent *readdir_root(DIR *pdir)
{
    struct rootdir *rdir = (struct rootdir *)pdir;
    vfs_entry_t *v;

    v = s_vfs[rdir->dir.dd_vfs_idx];
    if (!v)
        return NULL;
    rdir->dir.dd_vfs_idx++;

    memset(&rdir->e, 0, sizeof(rdir->e));
    rdir->e.d_ino = 0;
    rdir->e.d_type = DT_DIR;
    strncpy(rdir->e.d_name, v->path_prefix + 1, 255);
    return &rdir->e;
}

struct dirent *readdir(DIR *pdir)
{
    const vfs_entry_t *vfs;
    struct dirent *ret;

    if (pdir == (DIR *)&g_rdir)
        return readdir_root(pdir);

    vfs = get_vfs_for_index(pdir->dd_vfs_idx);
    if (vfs == NULL) {
        errno = EBADF;
        return NULL;
    }

    CHECK_AND_CALLP(ret, NULL, vfs, readdir, pdir);
    return ret;
}

int readdir_r(DIR *pdir, struct dirent *entry, struct dirent **out_dirent)
{
    const vfs_entry_t *vfs;
    int ret;

    vfs = get_vfs_for_index(pdir->dd_vfs_idx);
    if (vfs == NULL) {
        errno = EBADF;
        return -1;
    }
    CHECK_AND_CALL(ret, NULL, vfs, readdir_r, pdir, entry, out_dirent);
    return ret;
}

long telldir(DIR *pdir)
{
    long ret;
    const vfs_entry_t *vfs;

    vfs = get_vfs_for_index(pdir->dd_vfs_idx);
    if (vfs == NULL) {
        errno = EBADF;
        return -1;
    }
    CHECK_AND_CALL(ret, NULL, vfs, telldir, pdir);
    return ret;
}

void seekdir(DIR *pdir, long loc)
{
    const vfs_entry_t *vfs = get_vfs_for_index(pdir->dd_vfs_idx);

    if (vfs == NULL) {
        errno = EBADF;
        return;
    }
    CHECK_AND_CALLV(NULL, vfs, seekdir, pdir, loc);
}

void rewinddir(DIR *pdir)
{
    seekdir(pdir, 0);
}

static int closedir_root(DIR *pdir)
{
    struct rootdir *rdir = &g_rdir;

    memset(rdir, 0, sizeof(*rdir));
    return 0;
}

int closedir(DIR *pdir)
{
    const vfs_entry_t *vfs;
    int ret;

    if (pdir == (DIR *)&g_rdir)
        return closedir_root(pdir);

    vfs = get_vfs_for_index(pdir->dd_vfs_idx);
    if (vfs == NULL) {
        errno = EBADF;
        return -1;
    }
    CHECK_AND_CALL(ret, NULL, vfs, closedir, pdir);
    return ret;
}

int mkdir(const char *name, mode_t mode)
{
    const vfs_entry_t *vfs;
    const char *path_within_vfs;
    int ret;
    char *fullname;

    fullname = vfs_workdir_get_path(NULL, name);
    if (fullname == NULL) {
        return -ENOMEM;
    }

    vfs = get_vfs_for_path(fullname);
    if (vfs == NULL) {
        errno = ENOENT;
        vfs_workdir_put_path(fullname);
        return -1;
    }

    path_within_vfs = translate_path(vfs, fullname);
    CHECK_AND_CALL(ret, NULL, vfs, mkdir, path_within_vfs, mode);

    vfs_workdir_put_path(fullname);
    return ret;
}

int rmdir(const char *name)
{
    const vfs_entry_t *vfs;
    const char *path_within_vfs;
    int ret;
    char *fullname;

    fullname = vfs_workdir_get_path(NULL, name);
    if (fullname == NULL) {
        return -ENOMEM;
    }

    vfs = get_vfs_for_path(fullname);
    if (vfs == NULL) {
        errno = ENOENT;
        vfs_workdir_put_path(fullname);
        return -1;
    }

    path_within_vfs = translate_path(vfs, fullname);
    CHECK_AND_CALL(ret, NULL, vfs, rmdir, path_within_vfs);
    vfs_workdir_put_path(fullname);
    return ret;
}

int fcntl(int fd, int cmd, ...)
{
    int ret;
    va_list args;
    const vfs_entry_t *vfs = get_vfs_for_fd(fd);
    const int local_fd = get_local_fd(vfs, fd);

    if (vfs == NULL || local_fd < 0) {
        errno = EBADF;
        return -1;
    }

    va_start(args, cmd);
    CHECK_AND_CALL(ret, NULL, vfs, fcntl, local_fd, cmd, args);
    va_end(args);
    return ret;
}

int ioctl(int fd, int cmd, ...)
{
    int ret;
    va_list args;
    const vfs_entry_t *vfs = get_vfs_for_fd(fd);
    const int local_fd = get_local_fd(vfs, fd);

    if (vfs == NULL || local_fd < 0) {
        errno = EBADF;
        return -1;
    }

    va_start(args, cmd);
    CHECK_AND_CALL(ret, NULL, vfs, ioctl, local_fd, cmd, args);
    va_end(args);
    return ret;
}

int fsync(int fd)
{
    int ret;
    const vfs_entry_t *vfs = get_vfs_for_fd(fd);
    const int local_fd = get_local_fd(vfs, fd);

    if (vfs == NULL || local_fd < 0) {
        errno = EBADF;
        return -1;
    }

    CHECK_AND_CALL(ret, NULL, vfs, fsync, local_fd);
    return ret;
}

vfs_t* vfs_fd_get(int fd, int *outfd)
{
    const vfs_entry_t *vfs = get_vfs_for_fd(fd);
    const int local_fd = get_local_fd(vfs, fd);
    *outfd = local_fd;
    if (vfs) {
        return (vfs_t *)(&vfs->vfs);
    }
    return NULL;
}

int access(const char *path, int amode)
{
    int ret;
    const vfs_entry_t *vfs;
    const char *path_within_vfs;
    char *fullpath;

    fullpath = vfs_workdir_get_path(NULL, path);
    if (fullpath == NULL) {
        return -ENOMEM;
    }

    vfs = get_vfs_for_path(fullpath);
    if (vfs == NULL) {
        errno = ENOENT;
        vfs_workdir_put_path(fullpath);
        return -1;
    }

    path_within_vfs = translate_path(vfs, fullpath);
    CHECK_AND_CALL(ret, NULL, vfs, access, path_within_vfs, amode);

    vfs_workdir_put_path(fullpath);
    return ret;
}

int truncate(const char *path, off_t length)
{
    int ret;
    const vfs_entry_t *vfs;
    const char *path_within_vfs;
    char *fullpath;

    fullpath = vfs_workdir_get_path(NULL, path);
    if (fullpath == NULL) {
        return -ENOMEM;
    }

    vfs = get_vfs_for_path(fullpath);
    if (vfs == NULL) {
        vfs_workdir_put_path(fullpath);
        errno = ENOENT;
        return -1;
    }

    path_within_vfs = translate_path(vfs, fullpath);
    CHECK_AND_CALL(ret, NULL, vfs, truncate, path_within_vfs, length);

    vfs_workdir_put_path(fullpath);

    return ret;
}

int statfs(const char *path, struct statfs *buf)
{
    int ret;
    const vfs_entry_t *vfs;
    char *fullpath;

    fullpath = vfs_workdir_get_path(NULL, path);
    if (fullpath == NULL) {
        return -ENOMEM;
    }

    vfs = get_vfs_for_path(fullpath);
    if (vfs == NULL) {
        vfs_workdir_put_path(fullpath);
        errno = ENOENT;
        return -1;
    }

    CHECK_AND_CALL(ret, NULL, vfs, statfs, fullpath, buf);

    vfs_workdir_put_path(fullpath);
    return ret;
}

int fstatfs(int fd, struct statfs *buf)
{
    int ret;
    const vfs_entry_t *vfs = get_vfs_for_fd(fd);
    const int local_fd = get_local_fd(vfs, fd);

    if (vfs == NULL || local_fd < 0) {
        _errno_r(((struct _reent *)r)) = EBADF;
        return -1;
    }

    CHECK_AND_CALL(ret, NULL, vfs, fstatfs, fd, buf);
    return ret;
}

int flock(int fd, int operation)
{
    /*  TODO: support flock function */
    return 0;
}
