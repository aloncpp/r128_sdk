#ifndef _BTMG_AVRC_H_
#define _BTMG_AVRC_H_

#ifdef __cplusplus
extern "C" {
#endif

btmg_err bt_avrc_init(int a2dp_type);
btmg_err bt_avrc_deinit(int a2dp_type);
btmg_err bt_avrc_ct_send_passthrough_cmd(uint8_t key_code);
btmg_err bt_avrc_set_absolute_volume(uint32_t volume);
btmg_err bt_avrc_get_absolute_volume(uint32_t *value);

#ifdef __cplusplus
}
#endif

#endif /* _BTMG_AVRC_H_ */