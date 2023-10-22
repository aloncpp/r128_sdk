#ifndef __BT_SBC_H__
#define __BT_SBC_H__

enum SBCMsgQue{
	MSG_SBC_STREAM = 0,
	MSG_SBC_EXIT,
};

uint8_t aw_decode_set_config(uint8_t sampling,uint8_t channel_mode,uint8_t sbc_encode_hdr);
uint8_t aw_a2dp_stream_push(uint8_t *data,uint32_t len);
#endif
