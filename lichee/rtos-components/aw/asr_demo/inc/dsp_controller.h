#ifndef __DSP_CONTROLLER_H__
#define __DSP_CONTROLLER_H__

typedef int (*audio_data_send_t)(void *priv, void *data, int size);
typedef int (*audio_raw_data_send_t)(void *priv, int vad_flag, void *data, int size);
typedef int (*word_id_send_t)(void *priv, int word_id, int cnt);

struct dsp_controller_config_t {
    audio_data_send_t audio_data_send;
    audio_raw_data_send_t audio_raw_data_send;
    word_id_send_t word_id_send;
    int vad_bos_buffer_len;
    void *priv;
    unsigned int timeout_ms;
};

void *dsp_controller_create(struct dsp_controller_config_t *config);
void dsp_controller_destroy(void *_hdl);
int dsp_controller_start(void *_hdl);
int dsp_controller_stop(void *_hdl);
int dsp_controller_get_vad_flag(void *_hdl);
int dsp_controller_get_eos(void *_hdl);
int dsp_controller_enable_vad(void *_hdl);
int dsp_controller_disable_vad(void *_hdl);
int dsp_controller_get_enable_vad(void *_hdl);

#endif /* __DSP_CONTROLLER_H__ */