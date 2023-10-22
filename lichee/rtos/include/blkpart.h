#ifndef __BLKPART_H__
#define __BLKPART_H__

#include <sys/types.h>
#include <stdint.h>
#include <devfs.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SECTOR_SHIFT 9
#define SECTOR_SIZE (1 << SECTOR_SHIFT)

/*
 * flag: PARTFLAG_FORCE_RW
 *     in normal case, when do blkpart read/write, we don't need to care about
 *     align and erase, blkpart handle them automatically, like read-merge-erase-write.
 *     in special case, read write speed is more important, we can set PARTFLAG_FORCE_RW
 *     and have to handle align and erase.
 *     after set, no nor cache and auto erase.
 *     for example, to update rtos part in nor flash:
 *	   1. config flag PARTFLAG_FORCE_RW for part rtos
 *	      [function: blkpart_config_force_rw]
 *	   2. erase where we want to write, or erase whole part rtos
 *	      [function: blkpart_erase]
 *	   3. write to part rtos in multiple times(don't need to erase again, but should not write different data to same address)
 *	      [open /dev/rtos and write]
 *	   4. reboot or remove flag PARTFLAG_FORCE_RW for part rtos
 *	      [function: blkpart_config_force_rw]
 *
 *
 * flag: PARTFLAG_SKIP_ERASE_BEFORE_WRITE
 *     in normal case, devfs blkpart write will do erase first automatically.
 *     in special case, read write speed is more important, we can set PARTFLAG_SKIP_ERASE_BEFORE_WRITE
 *     and have to handle erase.
 *     after set, nor cache still work but no auto erase.
 *     for example, to update rtos part in nor flash:
 *	   1. config flag PARTFLAG_SKIP_ERASE _BEFORE_WRITE for part rtos
 *	      [function: blkpart_config_skip_erase_before_write]
 *	   2. erase where we want to write, or erase whole part rtos
 *	      [function: blkpart_erase]
 *	   3. write to part rtos in multiple times(don't need to erase again, but should not write different data to same address)
 *	      [open /dev/rtos and write]
 *	   4. reboot or remove flag PARTFLAG_SKIP_ERASE_BEFORE_WRITE for part rtos
 *	      [function: blkpart_config_skip_erase_before_write]
 */
#define PARTFLAG_FORCE_RW  (1 << 0)
#define PARTFLAG_SKIP_ERASE_BEFORE_WRITE (1 << 1)

struct part {
    /* public */
#define BLKPART_OFF_APPEND UINT32_MAX
    uint64_t off;
#define BLKPART_SIZ_FULL UINT32_MAX
    uint64_t bytes;
#define MAX_BLKNAME_LEN 16
    char name[MAX_BLKNAME_LEN];      /* name: UDISK */

    /* private */
    char devname[MAX_BLKNAME_LEN];   /* name: nor0p1 */
    struct blkpart *blk;
#ifdef CONFIG_COMPONENTS_AW_DEVFS
    struct devfs_node node;
#endif
    uint32_t n_part;
    uint32_t flag;
    void *private;
};

struct blkpart {
    /* public */
    const char *name;
    uint64_t total_bytes;
    uint32_t blk_bytes;
    uint32_t page_bytes;
    int (*erase)(unsigned int, unsigned int);
    int (*program)(unsigned int, char *, unsigned int);
    int (*read)(unsigned int, char *, unsigned int);
    int (*sync)(void);
    int (*noncache_erase)(unsigned int, unsigned int);
    int (*noncache_program)(unsigned int, char *, unsigned int);
    int (*noncache_read)(unsigned int, char *, unsigned int);

    /* if no any partition, the follow can be NULL */
    struct part *parts;
    uint32_t n_parts;

    /* private */
    int blk_cnt;
    struct part root;
    struct blkpart *next;
    uint32_t n_blk;
};

int add_blkpart(struct blkpart *blk);
void del_blkpart(struct blkpart *blk);

#define PARTINDEX_THE_LAST UINT32_MAX
struct blkpart *get_blkpart_by_name(const char *name);
struct part *get_part_by_index(const char *blk_name, uint32_t index);
struct part *get_part_by_name(const char *name);
ssize_t blkpart_devfs_read(struct devfs_node *node, uint32_t offset,
        uint32_t size, void *data);
ssize_t blkpart_devfs_write(struct devfs_node *node, uint32_t offset,
        uint32_t size, const void *data);
ssize_t blkpart_erase_write(struct part *, uint32_t, uint32_t, const void *);
int blkpart_erase(struct part *, uint32_t, uint32_t);
int blkpart_sync(struct part *);
int blkpart_config_force_rw(struct part *, uint32_t);
int blkpart_config_skip_erase_before_write(struct part *, uint32_t);


#ifdef __cplusplus
}
#endif

#endif
