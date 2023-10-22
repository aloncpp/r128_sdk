#ifndef _BTMG_GAP_H_
#define _BTMG_GAP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "bt_manager.h"

btmg_err bt_gap_init(void);
btmg_err cmd_gap_deinit(void);
btmg_err bt_gap_scan_start();
btmg_err bt_gap_scan_stop();
btmg_err bt_gap_set_scan_mode(btmg_scan_mode_t mode);
btmg_err bt_gap_get_device_rssi(const char *addr);
btmg_err bt_gap_set_io_capability(btmg_io_capability_t io_cap);
btmg_err bt_gap_pincode_reply(char *pincode);
btmg_err bt_gap_ssp_passkey_reply(uint32_t passkey);
btmg_err bt_gap_ssp_passkey_confirm(uint32_t passkey);
btmg_err bt_gap_pairing_confirm(void);
btmg_err bt_gap_get_device_name(const char *addr);
btmg_err bt_gap_remove_bond_device(const char *addr);
int bt_gap_get_bond_device_num(void);
btmg_err bt_gap_get_bond_device_list(int device_num, btmg_paired_device_t *paired_list);

#ifdef __cplusplus
}
#endif

#endif /* _BTMG_GAP_H_ */
