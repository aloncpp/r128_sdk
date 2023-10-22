#ifndef _SYS_STATFS_H
#define _SYS_STATFS_H

#include <sys/types.h>

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

extern int statfs (const char *__file, struct statfs *__buf)
	__THROW __nonnull ((1, 2));

#endif
