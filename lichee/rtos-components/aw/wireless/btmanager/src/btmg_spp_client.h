#ifndef _BTMG_SPP_CLIENT_H_
#define _BTMG_SPP_CLIENT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "bt_manager.h"

btmg_err bt_sppc_init(void);
btmg_err bt_sppc_deinit(void);
btmg_err bt_sppc_connect(const char *dst);
btmg_err bt_sppc_disconnect(const char *dst);
btmg_err bt_sppc_write(char *data, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif /* _BTMG_SPP_CLIENT_H_ */
