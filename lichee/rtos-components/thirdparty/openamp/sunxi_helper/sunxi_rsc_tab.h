#ifndef OPENAMP_SUNXI_RSC_TAB_H_
#define OPENAMP_SUNXI_RSC_TAB_H_

#include <hal_osal.h>
#include <stddef.h>
#include <openamp/open_amp.h>

void resource_table_init(int rsc_id, void **table_ptr, int *length);

struct fw_rsc_carveout *
resource_table_get_rvdev_shm_entry(void *rsc_table, int rvdev_id);

struct fw_rsc_vdev *resource_table_get_vdev(int index);

#ifdef CONFIG_ARCH_SUN20IW3
#include "sunxi/sun20iw3/rsc_tab.h"
#endif

#endif
