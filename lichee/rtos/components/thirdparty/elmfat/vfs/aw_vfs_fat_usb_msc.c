#include <stdlib.h>
#include <string.h>
#include "awlog.h"
#include "vfs.h"
#include <devfs.h>

#include <diskio_impl.h>

#include <ff.h>

extern int vfs_fat_register(const char* base_path, const char* fat_drive, size_t max_files, FATFS** out_fs);
int usb_msc_elmfat_mount(const char *dev, const char *base_path)
{
    FATFS* fs = NULL;
    int err;
	int pdrv = 0;
	struct devfs_node *node = devfs_get_node((void *)dev);

    ff_diskio_register_usb_msc(pdrv, node);
    pr_debug(TAG, "using pdrv=%i", pdrv);
    char drv[3] = {(char)('0' + pdrv), ':', 0};

    // connect FATFS to VFS
    err = vfs_fat_register(base_path, drv, 16, &fs);
    if (err == -1) {
        // it's okay, already registered with VFS
    } else if (err != 0) {
        pr_debug(TAG, "esp_vfs_fat_register failed 0x(%x)", err);
        goto fail;
    }

    // Try to mount partition
    FRESULT res = f_mount(fs, drv, 1);
    if (res != FR_OK) {
        err = -1;
        pr_err("failed to mount node (%d)", res);
        if (!((res == FR_NO_FILESYSTEM || res == FR_INT_ERR))) {
            goto fail;
        }
    }
    printf("mount usb mass storage successull!!\n");
    return 0;

fail:
    if (fs) {
        f_mount(NULL, drv, 0);
    }
	vfs_fat_unregister_path(base_path);
	ff_diskio_unregister(pdrv);
    return err;
}

static void local_node_remove(void)
{
}

static int unmount_node_core(const char *base_path, struct devfs_node *node)
{
    BYTE pdrv = ff_usb_msc_diskio_get_pdrv_node(node);
    if (pdrv == 0xff) {
        return -1;
    }

    // unmount
    char drv[3] = {(char)('0' + pdrv), ':', 0};
    f_mount(0, drv, 0);
    // release SD driver
    ff_diskio_unregister(pdrv);

    /*
     * When connect peripherals, Disk driver will allocate and release memory.
     * Request memory : DiskProbe -> UsbBlkDevAllocInit
     * Release memory : DiskRemove -> UsbBlkDevFree
     * So, it is totally unnecessary to free node.
     * free(node);
     */

    int err = vfs_fat_unregister_path(base_path);
    return err;
}

int usb_msc_elmfat_unmount(const char *base_path, struct devfs_node *node)
{
    int err = unmount_node_core(base_path, node);
    return err;
}
