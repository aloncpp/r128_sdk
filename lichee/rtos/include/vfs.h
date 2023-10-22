// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
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

#ifndef __VFS_H__
#define __VFS_H__

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <unistd.h>
#include <FreeRTOS.h>
#include <semphr.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/reent.h>
#include <dirent.h>
#include <string.h>
#include <sys/vfs.h>

#include <hal_poll.h>

#ifdef __cplusplus
extern "C" {
#endif

//#ifndef _SYS_TYPES_FD_SET
//#error "VFS should be used with FD_SETSIZE and FD_SET from sys/types.h"
//#endif

/**
 * Maximum number of (global) file descriptors.
 */
#define MAX_FDS         FD_SETSIZE /* for compatibility with fd_set */

/**
 * Maximum length of path prefix (not including zero terminator)
 */
#define VFS_PATH_MAX 15

/**
 * Default value of flags member in vfs_t structure.
 */
#define VFS_FLAG_DEFAULT        0

/**
 * Flag which indicates that FS needs extra context pointer in syscalls.
 */
#define VFS_FLAG_CONTEXT_PTR    1

/*
 * @brief VFS identificator used for vfs_register_with_id()
 */
typedef int vfs_id_t;

/**
 * @brief VFS definition structure
 *
 * This structure should be filled with pointers to corresponding
 * FS driver functions.
 *
 * VFS component will translate all FDs so that the filesystem implementation
 * sees them starting at zero. The caller sees a global FD which is prefixed
 * with an pre-filesystem-implementation.
 *
 * Some FS implementations expect some state (e.g. pointer to some structure)
 * to be passed in as a first argument. For these implementations,
 * populate the members of this structure which have _p suffix, set
 * flags member to VFS_FLAG_CONTEXT_PTR and provide the context pointer
 * to vfs_register function.
 * If the implementation doesn't use this extra argument, populate the
 * members without _p suffix and set flags member to VFS_FLAG_DEFAULT.
 *
 * If the FS driver doesn't provide some of the functions, set corresponding
 * members to NULL.
 */
typedef struct
{
    int flags;      /*!< VFS_FLAG_CONTEXT_PTR or VFS_FLAG_DEFAULT */
    union {
        ssize_t (*write_p)(void* p, int fd, const void * data, size_t size);
        ssize_t (*write)(int fd, const void * data, size_t size);
    };
    union {
        off_t (*lseek_p)(void* p, int fd, off_t size, int mode);
        off_t (*lseek)(int fd, off_t size, int mode);
    };
    union {
        ssize_t (*read_p)(void* ctx, int fd, void * dst, size_t size);
        ssize_t (*read)(int fd, void * dst, size_t size);
    };
    union {
        int (*open_p)(void* ctx, const char * path, int flags, int mode);
        int (*open)(const char * path, int flags, int mode);
    };
    union {
        int (*close_p)(void* ctx, int fd);
        int (*close)(int fd);
    };
    union {
        int (*poll_p)(void* ctx, int fd, struct hal_pollreq *req);
        int (*poll)(int fd, struct hal_pollreq *req);
    };
    union {
        int (*fstat_p)(void* ctx, int fd, struct stat * st);
        int (*fstat)(int fd, struct stat * st);
    };
    union {
        int (*stat_p)(void* ctx, const char * path, struct stat * st);
        int (*stat)(const char * path, struct stat * st);
    };
    union {
        int (*link_p)(void* ctx, const char* n1, const char* n2);
        int (*link)(const char* n1, const char* n2);
    };
    union {
        int (*unlink_p)(void* ctx, const char *path);
        int (*unlink)(const char *path);
    };
    union {
        int (*rename_p)(void* ctx, const char *src, const char *dst);
        int (*rename)(const char *src, const char *dst);
    };
    union {
        DIR* (*opendir_p)(void* ctx, const char* name);
        DIR* (*opendir)(const char* name);
    };
    union {
        struct dirent* (*readdir_p)(void* ctx, DIR* pdir);
        struct dirent* (*readdir)(DIR* pdir);
    };
    union {
        int (*readdir_r_p)(void* ctx, DIR* pdir, struct dirent* entry, struct dirent** out_dirent);
        int (*readdir_r)(DIR* pdir, struct dirent* entry, struct dirent** out_dirent);
    };
    union {
        long (*telldir_p)(void* ctx, DIR* pdir);
        long (*telldir)(DIR* pdir);
    };
    union {
        void (*seekdir_p)(void* ctx, DIR* pdir, long offset);
        void (*seekdir)(DIR* pdir, long offset);
    };
    union {
        int (*closedir_p)(void* ctx, DIR* pdir);
        int (*closedir)(DIR* pdir);
    };
    union {
        int (*mkdir_p)(void* ctx, const char* name, mode_t mode);
        int (*mkdir)(const char* name, mode_t mode);
    };
    union {
        int (*rmdir_p)(void* ctx, const char* name);
        int (*rmdir)(const char* name);
    };
    union {
        int (*fcntl_p)(void* ctx, int fd, int cmd, va_list args);
        int (*fcntl)(int fd, int cmd, va_list args);
    };
    union {
        int (*ioctl_p)(void* ctx, int fd, int cmd, va_list args);
        int (*ioctl)(int fd, int cmd, va_list args);
    };
    union {
        int (*fsync_p)(void* ctx, int fd);
        int (*fsync)(int fd);
    };
    union {
        int (*access_p)(void* ctx, const char *path, int amode);
        int (*access)(const char *path, int amode);
    };
    union {
        int (*truncate_p)(void* ctx, const char *path, off_t length);
        int (*truncate)(const char *path, off_t length);
    };
    union {
        int (*statfs_p)(void* ctx, const char *path, struct statfs *buf);
        int (*statfs)(const char *path, struct statfs *buf);
    };
    union {
        int (*fstatfs_p)(void* ctx, int fd, struct statfs *buf);
        int (*fstatfs)(int fd, struct statfs *buf);
    };
} vfs_t;


/**
 * Register a virtual filesystem for given path prefix.
 *
 * @param base_path  file path prefix associated with the filesystem.
 *                   Must be a zero-terminated C string, up to VFS_PATH_MAX
 *                   characters long, and at least 2 characters long.
 *                   Name must start with a "/" and must not end with "/".
 *                   For example, "/data" or "/dev/spi" are valid.
 *                   These VFSes would then be called to handle file paths such as
 *                   "/data/myfile.txt" or "/dev/spi/0".
 * @param vfs  Pointer to vfs_t, a structure which maps syscalls to
 *             the filesystem driver functions. VFS component doesn't
 *             assume ownership of this pointer.
 * @param ctx  If vfs->flags has VFS_FLAG_CONTEXT_PTR set, a pointer
 *             which should be passed to VFS functions. Otherwise, NULL.
 *
 * @return  OK if successful, ERR_NO_MEM if too many VFSes are
 *          registered.
 */
int vfs_register(const char* base_path, const vfs_t* vfs, void* ctx);


/**
 * Special case function for registering a VFS that uses a method other than
 * open() to open new file descriptors from the interval <min_fd; max_fd).
 *
 * This is a special-purpose function intended for registering LWIP sockets to VFS.
 *
 * @param vfs Pointer to vfs_t. Meaning is the same as for vfs_register().
 * @param ctx Pointer to context structure. Meaning is the same as for vfs_register().
 * @param min_fd The smallest file descriptor this VFS will use.
 * @param max_fd Upper boundary for file descriptors this VFS will use (the biggest file descriptor plus one).
 *
 * @return  OK if successful, ERR_NO_MEM if too many VFSes are
 *          registered, ERR_INVALID_ARG if the file descriptor boundaries
 *          are incorrect.
 */
int vfs_register_fd_range(const vfs_t *vfs, void *ctx, int min_fd, int max_fd);

/**
 * Special case function for registering a VFS that uses a method other than
 * open() to open new file descriptors. In comparison with
 * vfs_register_fd_range, this function doesn't pre-registers an interval
 * of file descriptors. File descriptors can be registered later, by using
 * vfs_register_fd.
 *
 * @param vfs Pointer to vfs_t. Meaning is the same as for vfs_register().
 * @param ctx Pointer to context structure. Meaning is the same as for vfs_register().
 * @param vfs_id Here will be written the VFS ID which can be passed to
 *               vfs_register_fd for registering file descriptors.
 *
 * @return  OK if successful, ERR_NO_MEM if too many VFSes are
 *          registered, ERR_INVALID_ARG if the file descriptor boundaries
 *          are incorrect.
 */
int vfs_register_with_id(const vfs_t *vfs, void *ctx, vfs_id_t *vfs_id);

/**
 * Unregister a virtual filesystem for given path prefix
 *
 * @param base_path  file prefix previously used in vfs_register call
 * @return OK if successful, ERR_INVALID_STATE if VFS for given prefix
 *         hasn't been registered
 */
int vfs_unregister(const char* base_path);

/**
 * Special function for registering another file descriptor for a VFS registered
 * by vfs_register_with_id.
 *
 * @param vfs_id VFS identificator returned by vfs_register_with_id.
 * @param fd The registered file descriptor will be written to this address.
 *
 * @return  OK if the registration is successful,
 *          ERR_NO_MEM if too many file descriptors are registered,
 *          ERR_INVALID_ARG if the arguments are incorrect.
 */
int vfs_register_fd(vfs_id_t vfs_id, int *fd);

/**
 * Special function for unregistering a file descriptor belonging to a VFS
 * registered by vfs_register_with_id.
 *
 * @param vfs_id VFS identificator returned by vfs_register_with_id.
 * @param fd File descriptor which should be unregistered.
 *
 * @return  OK if the registration is successful,
 *          ERR_INVALID_ARG if the arguments are incorrect.
 */
int vfs_unregister_fd(vfs_id_t vfs_id, int fd);

/**
 * These functions are to be used in newlib syscall table. They will be called by
 * newlib when it needs to use any of the syscalls.
 */
/**@{*/
ssize_t vfs_write(struct _reent *r, int fd, const void * data, size_t size);
off_t vfs_lseek(struct _reent *r, int fd, off_t size, int mode);
ssize_t vfs_read(struct _reent *r, int fd, void * dst, size_t size);
int vfs_open(struct _reent *r, const char * path, int flags, int mode);
int vfs_close(struct _reent *r, int fd);
int vfs_fstat(struct _reent *r, int fd, struct stat * st);
int vfs_stat(struct _reent *r, const char * path, struct stat * st);
int vfs_link(struct _reent *r, const char* n1, const char* n2);
int vfs_unlink(struct _reent *r, const char *path);
int vfs_rename(struct _reent *r, const char *src, const char *dst);
vfs_t *vfs_fd_get(int fd, int *outfd);
/**@}*/

#ifdef __cplusplus
} // extern "C"
#endif

#endif //__VFS_H__
