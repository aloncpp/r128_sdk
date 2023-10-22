#ifndef _BTMG_A2DP_SOURCE_H_
#define _BTMG_A2DP_SOURCE_H_

#ifdef __cplusplus
extern "C" {
#endif

btmg_err bt_a2dp_source_init(void);
btmg_err bt_a2dp_source_deinit(void);
btmg_err bt_a2dp_source_connect(const char *addr);
btmg_err bt_a2dp_source_disconnect(const char *addr);
btmg_err bt_a2dp_source_set_audio_param(uint8_t channels, uint32_t sampling);
btmg_err bt_a2dp_source_send_data(uint8_t *data, int len);
btmg_err bt_a2dp_source_play_start(void);
btmg_err bt_a2dp_source_play_stop(bool drop);
bool bt_a2dp_source_is_ready(void);

#ifdef __cplusplus
}
#endif

#endif /* _BTMG_A2DP_SOURCE_H_ */
