#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <FreeRTOS.h>
#include <semphr.h>
#include <queue.h>
#include <portmacro.h>
#include <blkpart.h>
#include <vfs.h>

#ifdef CONFIG_COMPONENT_LITTLEFS_2_2_1
#include "littlefs-2.2.1/lfs.h"
#endif

#define MAX_OPENED_FILES CONFIG_COMPONENT_LITTLEFS_MAX_OPEN_FILES

struct lfs_vfs_entry {
    char *path;
    void *entry;
};

struct lfs_ctx {
    lfs_t *lfs;
    struct lfs_config *cfg;
    struct part *part;
    const char *mnt;

    SemaphoreHandle_t lock;
    struct lfs_ctx *next;
    struct lfs_vfs_entry files[MAX_OPENED_FILES];
};

int lfs_register_vfs(struct lfs_ctx *ctx);
int lfs_unregister_vfs(struct lfs_ctx *ctx);
int lfs_lock(struct lfs_ctx *ctx);
int lfs_unlock(struct lfs_ctx *ctx);
