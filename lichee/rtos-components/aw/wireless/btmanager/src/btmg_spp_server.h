#ifndef _BTMG_SPP_SERVER_H_
#define _BTMG_SPP_SERVER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "bt_manager.h"

btmg_err bt_spps_init(void);
btmg_err bt_spps_deinit(void);
btmg_err bt_spps_start(int scn);
btmg_err bt_spps_stop(void);
btmg_err bt_spps_write(char *data, uint32_t len);
btmg_err bt_spps_disconnect(const char *dst);

#ifdef __cplusplus
}
#endif

#endif /* _BTMG_SPP_SERVER_H_ */
