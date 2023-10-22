#ifndef _BTMG_HFP_AG_H_
#define _BTMG_HFP_AG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "bt_manager.h"

btmg_err bt_hfp_ag_init(void);
btmg_err bt_hfp_ag_deinit(void);
btmg_err bt_hfp_ag_connect(const char *addr);
btmg_err bt_hfp_ag_disconnect(const char *addr);
btmg_err bt_hfp_ag_connect_audio(const char *addr);
btmg_err bt_hfp_ag_disconnect_audio(const char *addr);
btmg_err bt_hfp_ag_spk_vol_update(const char *addr, int volume);

#ifdef __cplusplus
}
#endif

#endif /* _BTMG_HFP_AG_H_ */
