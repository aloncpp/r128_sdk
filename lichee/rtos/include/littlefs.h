#ifndef __LITTLEFS_H_
#define __LITTLEFS_H_

#include <stdbool.h>

int littlefs_mount(const char *dev, const char *mnt, bool format);
int littlefs_umount(const char *mnt);

#endif
