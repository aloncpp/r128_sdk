#ifndef __RAMFS_H__
#define __RAMFS_H__

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
#include <vfs.h>
#include <pthread.h>

#include "memheap.h"
#include <aw_list.h>

#define RAMFS_NAME_MAX  32
#define RAMFS_MAGIC     0x0A0A0A0A
#define RAMFS_FILE      0
#define RAMFS_DIR       1

#define DATA_SLICE_SIZE (4 * 1024)

#define MAX_OPENED_FILES 64

struct slist_node
{
    struct slist_node *next;        /* *< point to next node. */
};
typedef struct slist_node slist_t;  /* < Type for single list. */

/**
 * @brief initialize a single list
 *
 * @param l the single list to be initialized
 */
static inline void slist_init(slist_t *l)
{
    l->next = NULL;
}

static inline void slist_append(slist_t *l, slist_t *n)
{
    struct slist_node *node;

    node = l;
    while (node->next)
    {
        node = node->next;
    }

    /* append the node to the tail */
    node->next = n;
    n->next = NULL;
}

static inline void slist_insert(slist_t *l, slist_t *n)
{
    n->next = l->next;
    l->next = n;
}

static inline unsigned int slist_len(const slist_t *l)
{
    unsigned int len = 0;
    const slist_t *list = l->next;
    while (list != NULL)
    {
        list = list->next;
        len ++;
    }

    return len;
}

static inline slist_t *slist_remove(slist_t *l, slist_t *n)
{
    /* remove slist head */
    struct slist_node *node = l;
    while (node->next && node->next != n)
    {
        node = node->next;
    }

    /* remove node */
    if (node->next != (slist_t *)0)
    {
        node->next = node->next->next;
    }

    return l;
}

static inline slist_t *slist_first(slist_t *l)
{
    return l->next;
}

static inline slist_t *slist_tail(slist_t *l)
{
    while (l->next)
    {
        l = l->next;
    }

    return l;
}

static inline slist_t *slist_next(slist_t *n)
{
    return n->next;
}

static inline int slist_isempty(slist_t *l)
{
    return l->next == NULL;
}

/**
 * @brief get the struct for this single list node
 * @param node the entry point
 * @param type the type of structure
 * @param member the name of list in structure
 */
#define slist_entry(node, type, member) \
    container_of(node, type, member)

/**
 * slist_for_each - iterate over a single list
 * @pos:    the slist_t * to use as a loop cursor.
 * @head:   the head for your single list.
 */
#define slist_for_each(pos, head) \
    for (pos = (head)->next; pos != NULL; pos = pos->next)

/**
 * slist_for_each_entry  -   iterate over single list of given type
 * @pos:    the type * to use as a loop cursor.
 * @head:   the head for your single list.
 * @member: the name of the list_struct within the struct.
 */
#define slist_for_each_entry(pos, head, member) \
    for (pos = slist_entry((head)->next, typeof(*pos), member); \
         &pos->member != (NULL); \
         pos = slist_entry(pos->member.next, typeof(*pos), member))

/**
 * slist_first_entry - get the first element from a slist
 * @ptr:    the slist head to take the element from.
 * @type:   the type of the struct this is embedded in.
 * @member: the name of the slist_struct within the struct.
 *
 * Note, that slist is expected to be not empty.
 */
#define slist_first_entry(ptr, type, member) \
    slist_entry((ptr)->next, type, member)

/**
 * slist_tail_entry - get the tail element from a slist
 * @ptr:    the slist head to take the element from.
 * @type:   the type of the struct this is embedded in.
 * @member: the name of the slist_struct within the struct.
 *
 * Note, that slist is expected to be not empty.
 */
#define slist_tail_entry(ptr, type, member) \
    slist_entry(slist_tail(ptr), type, member)

typedef struct _data_slice
{
    struct slist_node list;
    uint8_t *data_node;
} data_slice;

struct ramfs_dirent
{
    struct list_head list;
    struct ramfs *fs;       /* file system ref */

    char name[RAMFS_NAME_MAX];  /* dirent name */
    data_slice data_slice_chain;

    size_t size;             /* file size */
    struct list_head dirent_list;
    struct ramfs_dirent *parent;
    int type;
};

struct ramfs_vfs_entry
{
    char *path;
    void *entry;
};

#ifdef CONFIG_COMPONENTS_AMP

#define MAX_L1_CACHELINE_SIZE 64

struct ramfs_dir
{
    union {
        DIR dir;
        char dir_data[64];
    } __attribute__((aligned(MAX_L1_CACHELINE_SIZE)));
    int index;
    union {
        struct dirent e;
        char e_data[64];
    } __attribute__((aligned(MAX_L1_CACHELINE_SIZE)));
};

#else
struct ramfs_dir
{
    DIR dir;
    int index;
    struct dirent e;
};
#endif

typedef struct _ramfs_file_t
{
    char *path;
    int flags;
    void *data;
    int pos;
    int size;
    void *entry;
} ramfs_file_t;

struct ramfs
{
    uint32_t magic;

    struct memheap memheap;
    struct ramfs_dirent root;
    struct ramfs_vfs_entry files[MAX_OPENED_FILES];
};

int ramfs_init(void);
struct ramfs *ramfs_create(uint8_t *pool, size_t size);

#endif

