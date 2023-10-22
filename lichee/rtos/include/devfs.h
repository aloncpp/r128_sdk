#ifndef __DEVFS_H__
#define __DEVFS_H__

#include <stdint.h>
#include <stdio.h>

#ifdef CONFIG_COMPONENT_IO_MULTIPLEX
#include <poll.h>
#endif
struct devfs_file {
    struct devfs_node *node;
    uint32_t off;
    void *private;
};

struct devfs_node {
    const char *name;
    const char *alias;
#define NODE_SIZE_INFINITE UINT32_MAX
    uint64_t size;

    ssize_t (*write)(struct devfs_node *node, uint32_t addr, uint32_t size, const void *data);
    ssize_t (*read)(struct devfs_node *node, uint32_t addr, uint32_t size, void *data);
    /* device can do some initializion at this call back function */
    int (*open)(struct devfs_node *node);
    /* device can do some de-initializion at this call back function */
    int (*close)(struct devfs_node *node);
    /* if device has some other feature, make it come true here */
    int (*ioctl)(struct devfs_node *node, int cmd, void *arg);
    /* device can flush cache if it has at this call back function */
    int (*fsync)(struct devfs_node *node);
	struct devfs_node *next;
    void *private;

#ifdef CONFIG_COMPONENT_IO_MULTIPLEX
    vfs_t *fops;
    struct rt_wqueue wait_queue;
#endif
};

int devfs_add_node(struct devfs_node *new);
void devfs_del_node(struct devfs_node *node);
int devfs_mount(const char *mnt_point);

typedef uint8_t BYTE;
void ff_diskio_register_usb_msc(BYTE pdrv, struct devfs_node* card);
struct devfs_node * devfs_get_node(char *name);
//int vfs_fat_register(const char* base_path, const char* fat_drive, size_t max_files, FATFS** out_fs);
int vfs_fat_unregister_path(const char* base_path);
BYTE ff_usb_msc_diskio_get_pdrv_node(const struct devfs_node* node);
#endif

