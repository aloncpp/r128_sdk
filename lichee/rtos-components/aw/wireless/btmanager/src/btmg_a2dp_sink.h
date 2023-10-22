#ifndef _BTMG_A2DP_SINK_H_
#define _BTMG_A2DP_SINK_H_

#ifdef __cplusplus
extern "C" {
#endif

btmg_err bt_a2dp_sink_init(void);
btmg_err bt_a2dp_sink_deinit(void);
btmg_err bt_a2dp_sink_connect(const char *addr);
btmg_err bt_a2dp_sink_disconnect(const char *addr);

#ifdef __cplusplus
}
#endif

#endif /* _BTMG_A2DP_SINK_H_ */
