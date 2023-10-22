#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <statfs.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>

#include "sunxi_amp.h"
#include <hal_cache.h>

extern int truncate (const char *__file, __off_t __length);
extern int fstatfs (int __fildes, struct statfs *__buf);

static int _open(const char *name, int nameSize, int flag)
{
    int ret;
    hal_dcache_invalidate((unsigned long)name, nameSize);
    ret = open(name, flag);
    hal_dcache_invalidate((unsigned long)name, nameSize);
    return ret;
}

static int _close(int fd)
{
    return close(fd);
}

static ssize_t _read(int fd, void *buffer, size_t size)
{
    ssize_t ret = -1;
    ret = read(fd, buffer, size);
    hal_dcache_clean((unsigned long)buffer, size);
    return ret;
}

static ssize_t _write(int fd, void *buffer, size_t size)
{
    ssize_t ret = -1;
    hal_dcache_invalidate((unsigned long)buffer, size);
    ret = write(fd, buffer, size);
    hal_dcache_clean((unsigned long)buffer, size);
    return ret;
}

static off_t _lseek(int fd, off_t offset, int whence)
{
    return lseek(fd, offset, whence);
}

static int _stat(const char *path, int pathSize, struct stat *st)
{
    int ret = -1;
    hal_dcache_invalidate((unsigned long)st, sizeof(*st));
    hal_dcache_invalidate((unsigned long)path, pathSize);
    ret = stat(path, st);
    hal_dcache_clean((unsigned long)st, sizeof(*st));
    return ret;
}

static int _fstat(int fd, struct stat *st)
{
    int ret = -1;
    hal_dcache_invalidate((unsigned long)st, sizeof(*st));
    ret = fstat(fd, st);
    hal_dcache_clean((unsigned long)st, sizeof(*st));
    return ret;
}

static int _unlink(const char *path, int pathSize)
{
    int ret = -1;
    hal_dcache_invalidate((unsigned long)path, pathSize);
    ret = unlink(path);
    hal_dcache_invalidate((unsigned long)path, pathSize);
    return ret;
}

static int _rename(const char *old, int oldSize, const char *new, int newSize)
{
    int ret = -1;
    hal_dcache_invalidate((unsigned long)new, newSize);
    hal_dcache_invalidate((unsigned long)old, oldSize);
    ret = rename(old, new);
    hal_dcache_invalidate((unsigned long)old, oldSize);
    hal_dcache_invalidate((unsigned long)new, newSize);
    return ret;
}

static DIR *_opendir(const char *path, int pathSize)
{
    DIR *ret = NULL;
    hal_dcache_invalidate((unsigned long)path, pathSize);
    ret = opendir(path);
    if (ret)
        hal_dcache_clean((unsigned long)ret, sizeof(*ret));
    hal_dcache_invalidate((unsigned long)path, pathSize);
    return ret;
}

static struct dirent *_readdir(DIR *pdir)
{
    struct dirent *ret = NULL;
    ret = readdir(pdir);
    if (ret)
        hal_dcache_clean((unsigned long)ret, sizeof(*ret));
    return ret;
}

static int _closedir(DIR *pdir)
{
    return closedir(pdir);
}

static int _mkdir(const char *name, int nameSize, mode_t mode)
{
    int ret;
    hal_dcache_invalidate((unsigned long)name, nameSize);
    ret = mkdir(name, mode);
    hal_dcache_invalidate((unsigned long)name, nameSize);
    return ret;
}

static int _rmdir(const char *name, int nameSize)
{
    int ret;
    hal_dcache_invalidate((unsigned long)name, nameSize);
    ret = rmdir(name);
    hal_dcache_invalidate((unsigned long)name, nameSize);
    return ret;
}

static int _access(const char *name, int nameSize, int amode)
{
    int ret;
    hal_dcache_invalidate((unsigned long)name, nameSize);
    ret = access(name, amode);
    hal_dcache_invalidate((unsigned long)name, nameSize);
    return ret;
}

static int _truncate(const char *name, int nameSize, off_t length)
{
    int ret;
    hal_dcache_invalidate((unsigned long)name, nameSize);
    ret = truncate(name, length);
    hal_dcache_invalidate((unsigned long)name, nameSize);
    return ret;
}

static int _statfs(const char *name, int nameSize, struct statfs *buf)
{
    int ret = -1;
    hal_dcache_invalidate((unsigned long)name, nameSize);
    ret = statfs(name, buf);
    hal_dcache_clean((unsigned long)buf, sizeof(*buf));
    return ret;
}

static int _fstatfs(int fd, struct statfs *buf)
{
    int ret = -1;
    ret = fstatfs(fd, buf);
    hal_dcache_clean((unsigned long)buf, sizeof(*buf));
    return ret;
}

static int _fsync(int fd)
{
    return fsync(fd);
}

sunxi_amp_func_table fsys_table[] =
{
    {.func = (void *)&_open, .args_num = 3, .return_type = RET_POINTER},
    {.func = (void *)&_close, .args_num = 1, .return_type = RET_POINTER},
    {.func = (void *)&_read, .args_num = 3, .return_type = RET_POINTER},
    {.func = (void *)&_write, .args_num = 3, .return_type = RET_POINTER},
    {.func = (void *)&_lseek, .args_num = 3, .return_type = RET_POINTER},
    {.func = (void *)&_fstat, .args_num = 2, .return_type = RET_POINTER},
    {.func = (void *)&_stat, .args_num = 3, .return_type = RET_POINTER},
    {.func = (void *)&_unlink, .args_num = 2, .return_type = RET_POINTER},
    {.func = (void *)&_rename, .args_num = 4, .return_type = RET_POINTER},
    {.func = (void *)&_opendir, .args_num = 2, .return_type = RET_POINTER},
    {.func = (void *)&_readdir, .args_num = 1, .return_type = RET_POINTER},
    {.func = (void *)&_closedir, .args_num = 1, .return_type = RET_POINTER},
    {.func = (void *)&_mkdir, .args_num = 3, .return_type = RET_POINTER},
    {.func = (void *)&_rmdir, .args_num = 2, .return_type = RET_POINTER},
    {.func = (void *)&_access, .args_num = 3, .return_type = RET_POINTER},
    {.func = (void *)&_truncate, .args_num = 3, .return_type = RET_POINTER},
    {.func = (void *)&_statfs, .args_num = 3, .return_type = RET_POINTER},
    {.func = (void *)&_fstatfs, .args_num = 2, .return_type = RET_POINTER},
    {.func = (void *)&_fsync, .args_num = 1, .return_type = RET_POINTER},
};
