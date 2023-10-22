#ifndef _BTMG_AUDIO_H_
#define _BTMG_AUDIO_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sys_ctrl/sys_ctrl.h"

typedef enum {
    BT_AUDIO_EVENT_A2DP_SNK_START = 0,
    BT_AUDIO_EVENT_A2DP_SNK_STOP,
    BT_AUDIO_EVENT_HFP_START,
    BT_AUDIO_EVENT_HFP_STOP,
} bt_app_audio_events;

typedef enum {
    BT_AUDIO_TYPE_NONE = 0,
    BT_AUDIO_TYPE_A2DP_SNK,
    BT_AUDIO_TYPE_A2DP_SRC,
    BT_AUDIO_TYPE_HFP
} bt_audio_type_t;

typedef enum {
    IO_TYPE_RB = 1, /* I/O through ringbuffer */
    IO_TYPE_CB,     /* I/O through callback */
} io_type_t;

int bt_audio_init(bt_audio_type_t type, uint16_t cache_time);
int bt_audio_deinit(bt_audio_type_t type);
int bt_audio_write(uint8_t *data, uint32_t len, uint32_t timeout, bt_audio_type_t type);
int bt_audio_read(uint8_t *data, uint32_t len, uint32_t timeout, bt_audio_type_t type);
int bt_audio_write_unblock(uint8_t *data, uint32_t len, bt_audio_type_t type);
int bt_audio_read_unblock(uint8_t *data, uint32_t len, bt_audio_type_t type);
int bt_audio_reset(bt_audio_type_t type);
int bt_audio_get_cache(bt_audio_type_t type);
void bt_audio_event_handle(event_msg *msg);
void bt_audio_pcm_config(uint32_t samplerate, uint8_t channels, bt_audio_type_t type);

#ifdef __cplusplus
}
#endif

#endif /* _BTMG_AUDIO_H_ */
