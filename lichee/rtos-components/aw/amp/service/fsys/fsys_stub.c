#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <reent.h>

#include "sunxi_amp.h"

#include <hal_cache.h>

#ifndef _MAYBE_STATIC
#include <statfs.h>
#include <dirent.h>

#else
struct dirent
{
    int d_ino;          /*!< file number */
    uint8_t d_type;     /*!< not defined in POSIX, but present in BSD and Linux */
#define DT_UNKNOWN  0
#define DT_REG      1
#define DT_DIR      2
    char d_name[256];   /*!< zero-terminated file name */
};

typedef __uint64_t __fsblkcnt_t;

struct statfs
{
    uint32_t f_type;
    uint32_t f_bsize;
    __fsblkcnt_t f_blocks;
    __fsblkcnt_t f_bfree;
    __fsblkcnt_t f_bavail;
    __fsfilcnt_t f_files;
    __fsfilcnt_t f_ffree;
};

typedef struct
{
    uint16_t dd_vfs_idx; /*!< VFS index, not to be used by applications */
    uint16_t dd_rsv;     /*!< field reserved for future extension */
    /* remaining fields are defined by VFS implementation */
} DIR;
#endif


MAYBE_STATIC int open(const char *name, int flag, ...)
{
    void *args[3] = {0};
    int ret = -1;
    int nameSize = strlen(name) + 1;
    char *align_name = amp_align_malloc(nameSize);
    if (!align_name)
    {
        return -1;
    }
    memset(align_name, 0, nameSize);
    memcpy(align_name, name, nameSize);
    args[0] = align_name;
    args[1] = (void *)(unsigned long)nameSize;
    args[2] = (void *)(unsigned long)flag;
    hal_dcache_clean((unsigned long)align_name, nameSize);
    ret = func_stub(RPCCALL_FSYS(open), 1, ARRAY_SIZE(args), args);
    amp_align_free(align_name);
    return ret;
}

MAYBE_STATIC int close(int fd)
{
    void *args[1] = {0};
    args[0] = (void *)(unsigned long)fd;
    return func_stub(RPCCALL_FSYS(close), 1, ARRAY_SIZE(args), args);
}

MAYBE_STATIC ssize_t read(int fd, void *buffer, size_t size)
{
    void *args[3] = {0};
    ssize_t ret = -1;
    args[0] = (void *)(unsigned long)fd;
    args[1] = buffer;
    args[2] = (void *)(unsigned long)size;
    if (fd < 3)
    {
        return _read_r(_REENT, fd, buffer, size);
    }
    hal_dcache_invalidate((unsigned long)buffer, size);
    ret = func_stub(RPCCALL_FSYS(read), 1, ARRAY_SIZE(args), args);
    hal_dcache_invalidate((unsigned long)buffer, size);
    return ret;
}

MAYBE_STATIC ssize_t write(int fd, void *buffer, size_t size)
{
    void *args[3] = {0};
    args[0] = (void *)(unsigned long)fd;
    args[1] = buffer;
    args[2] = (void *)(unsigned long)size;
    if (fd < 3)
    {
        return _write_r(_REENT, fd, buffer, size);
    }
    hal_dcache_clean((unsigned long)buffer, size);
    return func_stub(RPCCALL_FSYS(write), 1, ARRAY_SIZE(args), args);
}

MAYBE_STATIC off_t lseek(int fd, off_t offset, int whence)
{
    void *args[3] = {0};
    args[0] = (void *)(unsigned long)fd;
    args[1] = (void *)(unsigned long)offset;
    args[2] = (void *)(unsigned long)whence;
    return func_stub(RPCCALL_FSYS(lseek), 1, ARRAY_SIZE(args), args);
}

#ifndef _MAYBE_STATIC
MAYBE_STATIC int fstat(int fd, struct stat *st)
{
    void *args[2] = {0};
    int ret = -1;
    struct stat *st_align = amp_align_malloc(sizeof(*st));
    if (!st_align)
    {
        return -1;
    }
    memset(st_align, 0, sizeof(*st));
    args[0] = (void *)(unsigned long)fd;
    args[1] = st_align;
    hal_dcache_clean((unsigned long)st_align, sizeof(struct stat));
    ret = func_stub(RPCCALL_FSYS(fstat), 1, ARRAY_SIZE(args), args);
    hal_dcache_invalidate((unsigned long)st_align, sizeof(struct stat));
    memcpy(st, st_align, sizeof(struct stat));
    amp_align_free(st_align);
    return ret;
}

MAYBE_STATIC int stat(const char *path, struct stat *st)
{
    void *args[3] = {0};
    int ret = -1;
    int pathSize = strlen(path) + 1;
    char *align_path = amp_align_malloc(pathSize);
    if (!align_path)
    {
        return -1;
    }
    memset(align_path, 0, pathSize);
    memcpy(align_path, path, pathSize);
    struct stat *st_align = amp_align_malloc(sizeof(*st));
    if (!st_align)
    {
        amp_align_free(align_path);
        return -1;
    }
    memset(st_align, 0, sizeof(*st));

    args[0] = align_path;
    args[1] = (void *)(unsigned long)pathSize;
    args[2] = st_align;

    hal_dcache_clean((unsigned long)align_path, pathSize);
    hal_dcache_clean((unsigned long)st_align, sizeof(struct stat));

    ret = func_stub(RPCCALL_FSYS(stat), 1, ARRAY_SIZE(args), args);

    hal_dcache_invalidate((unsigned long)st_align, sizeof(struct stat));
    memcpy(st, st_align, sizeof(struct stat));
    amp_align_free(st_align);
    amp_align_free(align_path);
    return ret;
}
#endif

MAYBE_STATIC int unlink(const char *path)
{
    void *args[2] = {0};
    int ret = -1;
    int pathSize = strlen(path) + 1;
    char *align_path = amp_align_malloc(pathSize);
    if (!align_path)
    {
        return -1;
    }
    memset(align_path, 0, pathSize);
    memcpy(align_path, path, pathSize);
    args[0] = align_path;
    args[1] = (void *)(unsigned long)pathSize;
    hal_dcache_clean((unsigned long)align_path, pathSize);
    ret = func_stub(RPCCALL_FSYS(unlink), 1, ARRAY_SIZE(args), args);
    amp_align_free(align_path);
    return ret;
}

#ifndef _MAYBE_STATIC
MAYBE_STATIC int rename(const char *old, const char *new)
{
    void *args[4] = {0};
    int ret = -1;
    int oldSize = strlen(old) + 1;
    int newSize = strlen(new) + 1;
    char *align_old = amp_align_malloc(oldSize);
    if (!align_old)
    {
        return -1;
    }
    char *align_new = amp_align_malloc(newSize);
    if (!align_new)
    {
        amp_align_free(align_old);
        return -1;
    }
    memset(align_new, 0, newSize);
    memcpy(align_new, new, newSize);
    memset(align_old, 0, oldSize);
    memcpy(align_old, old, oldSize);

    args[0] = align_old;
    args[1] = (void *)(unsigned long)oldSize;
    args[2] = align_new;
    args[3] = (void *)(unsigned long)newSize;
    hal_dcache_clean((unsigned long)align_old, oldSize);
    hal_dcache_clean((unsigned long)align_new, newSize);
    ret = func_stub(RPCCALL_FSYS(rename), 1, ARRAY_SIZE(args), args);
    amp_align_free(align_old);
    amp_align_free(align_new);
    return ret;
}
#endif

MAYBE_STATIC DIR *opendir(const char *path)
{
    DIR *ret = NULL;
    void *args[2] = {0};
    int pathSize = strlen(path) + 1;
    char *align_path = amp_align_malloc(pathSize);
    if (!align_path)
    {
        return NULL;
    }
    memset(align_path, 0, pathSize);
    memcpy(align_path, path, pathSize);
    args[0] = align_path;
    args[1] = (void *)(unsigned long)pathSize;
    hal_dcache_clean((unsigned long)align_path, pathSize);
    ret = (void *)func_stub(RPCCALL_FSYS(opendir), 1, ARRAY_SIZE(args), args);
    if (ret)
    {
        hal_dcache_invalidate((unsigned long)ret, sizeof(*ret));
    }
    amp_align_free(align_path);
    return ret;
}

MAYBE_STATIC struct dirent *readdir(DIR *pdir)
{
    struct dirent *ent = NULL;
    void *args[1] = {0};
    args[0] = pdir;
    hal_dcache_clean((unsigned long)pdir, sizeof(*pdir));
    ent = (void *)func_stub(RPCCALL_FSYS(readdir), 1, ARRAY_SIZE(args), args);
    if (ent)
    {
        hal_dcache_invalidate((unsigned long)ent, sizeof(*ent));
    }
    return ent;
}

MAYBE_STATIC int closedir(DIR *pdir)
{
    void *args[1] = {0};
    args[0] = pdir;
    hal_dcache_clean((unsigned long)pdir, sizeof(*pdir));
    return func_stub(RPCCALL_FSYS(closedir), 1, ARRAY_SIZE(args), args);
}

#ifndef _MAYBE_STATIC
MAYBE_STATIC int mkdir(const char *name, mode_t mode)
{
    void *args[3] = {0};
    int ret = -1;
    int nameSize = strlen(name) + 1;
    char *align_name = amp_align_malloc(nameSize);
    if (!align_name)
    {
        return -1;
    }
    memset(align_name, 0, nameSize);
    memcpy(align_name, name, nameSize);
    args[0] = align_name;
    args[1] = (void *)(unsigned long)nameSize;
    args[2] = (void *)(unsigned long)mode;
    hal_dcache_clean((unsigned long)align_name, nameSize);
    ret = func_stub(RPCCALL_FSYS(mkdir), 1, ARRAY_SIZE(args), args);
    amp_align_free(align_name);
    return ret;
}
#endif

MAYBE_STATIC int rmdir(const char *name)
{
    void *args[2] = {0};
    int ret = -1;
    int nameSize = strlen(name) + 1;
    char *align_name = amp_align_malloc(nameSize);
    if (!align_name)
    {
        return -1;
    }
    memset(align_name, 0, nameSize);
    memcpy(align_name, name, nameSize);
    args[0] = align_name;
    args[1] = (void *)(unsigned long)nameSize;
    hal_dcache_clean((unsigned long)align_name, nameSize);
    ret = func_stub(RPCCALL_FSYS(rmdir), 1, ARRAY_SIZE(args), args);
    amp_align_free(align_name);
    return ret;
}

MAYBE_STATIC int access(const char *name, int amode)
{
    void *args[3] = {0};
    int ret = -1;
    int nameSize = strlen(name) + 1;
    char *align_name = amp_align_malloc(nameSize);
    if (!align_name)
    {
        return -1;
    }
    memset(align_name, 0, nameSize);
    memcpy(align_name, name, nameSize);
    args[0] = align_name;
    args[1] = (void *)(unsigned long)nameSize;
    args[2] = (void *)(unsigned long)amode;
    hal_dcache_clean((unsigned long)align_name, nameSize);
    ret = func_stub(RPCCALL_FSYS(access), 1, ARRAY_SIZE(args), args);
    amp_align_free(align_name);
    return ret;
}

MAYBE_STATIC int truncate(const char *name, off_t length)
{
    void *args[3] = {0};
    int ret = -1;
    int nameSize = strlen(name) + 1;
    char *align_name = amp_align_malloc(nameSize);
    if (!align_name)
    {
        return -1;
    }
    memset(align_name, 0, nameSize);
    memcpy(align_name, name, nameSize);
    args[0] = align_name;
    args[1] = (void *)(unsigned long)nameSize;
    args[2] = (void *)(unsigned long)length;
    hal_dcache_clean((unsigned long)align_name, nameSize);
    ret = func_stub(RPCCALL_FSYS(truncate), 1, ARRAY_SIZE(args), args);
    amp_align_free(align_name);
    return ret;
}

MAYBE_STATIC int statfs(const char *path, struct statfs *buf)
{
    void *args[3] = {0};
    int ret = -1;
    int pathSize = strlen(path) + 1;
    char *align_path = amp_align_malloc(pathSize);
    if (!align_path)
    {
        return -1;
    }
    char *align_statfs = amp_align_malloc(sizeof(struct statfs));
    if (!align_statfs)
    {
        amp_align_free(align_statfs);
        return -1;
    }
    memset(align_path, 0, pathSize);
    memcpy(align_path, path, pathSize);
    memset(align_statfs, 0, sizeof(struct statfs));

    args[0] = align_path;
    args[1] = (void *)(unsigned long)pathSize;
    args[2] = align_statfs;

    hal_dcache_clean((unsigned long)align_path, pathSize);
    hal_dcache_clean((unsigned long)align_statfs, sizeof(struct statfs));
    ret = func_stub(RPCCALL_FSYS(statfs), 1, ARRAY_SIZE(args), args);

    hal_dcache_invalidate((unsigned long)align_statfs, sizeof(struct statfs));
    memcpy(buf, align_statfs, sizeof(struct statfs));

    amp_align_free(align_path);
    amp_align_free(align_statfs);
    return ret;
}

MAYBE_STATIC int fstatfs(int fd, struct statfs *buf)
{
    void *args[2] = {0};
    int ret = -1;
    char *align_statfs = amp_align_malloc(sizeof(struct statfs));
    if (!align_statfs)
    {
        return -1;
    }
    args[0] = (void *)(unsigned long)fd;
    args[1] = align_statfs;

    hal_dcache_clean((unsigned long)align_statfs, sizeof(struct statfs));
    ret = func_stub(RPCCALL_FSYS(fstatfs), 1, ARRAY_SIZE(args), args);

    hal_dcache_invalidate((unsigned long)align_statfs, sizeof(struct statfs));
    memcpy(buf, align_statfs, sizeof(struct statfs));

    amp_align_free(align_statfs);
    return ret;
}

MAYBE_STATIC int fsync(int fd)
{
    void *args[1] = {0};
    args[0] = (void *)(unsigned long)fd;

    return func_stub(RPCCALL_FSYS(fsync), 1, ARRAY_SIZE(args), args);
}
