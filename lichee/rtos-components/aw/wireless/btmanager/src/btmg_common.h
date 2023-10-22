#ifndef _BTMG_COMMON_H_
#define _BTMG_COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "xr_bt_defs.h"
#include "bt_manager.h"
#include "btmg_dev_list.h"

typedef enum {
    CB_MAIN = 0,
    CB_MINOR,
    CB_MAX,
} btmg_cb_id_t;

uint64_t btmg_interval_time(void *tag, uint64_t expect_time_ms);
int str2bda(const char *strmac, xr_bd_addr_t bda);
void bda2str(xr_bd_addr_t bda, const char *bda_str);
bool btmg_disconnect_dev_list(dev_list_t *dev_list);

extern btmg_callback_t *btmg_cb_p[CB_MAX];
extern btmg_profile_info_t *bt_pro_info;
extern dev_list_t *connected_devices;
extern char bda_str[18];

#ifdef __cplusplus
}
#endif

#endif /* _BTMG_COMMON_H_ */
