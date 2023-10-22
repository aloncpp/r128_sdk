#ifndef __ASR_INPUT_H__
#define __ASR_INPUT_H__

typedef int (*asr_data_send_t)(void *priv, void *data, int size);

struct asr_input_config_t {
	uint32_t rate;
	uint8_t ch;
	uint8_t bit;

	asr_data_send_t data_send;
	void *priv;
};

void *asr_input_create(const struct asr_input_config_t *config);
void asr_input_destroy(void *_hdl);
int asr_input_start(void *_hdl);
int asr_input_stop(void *_hdl, int force);
int asr_input_is_stop(void *_hdl);

#endif /* __ASR_INPUT_H__ */