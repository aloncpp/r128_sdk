#ifndef __ASR_RWS_CLIENT_H__
#define __ASR_RWS_CLIENT_H__

#define FRAME_SIZE          (320 * 8)       // 320*n 1<n<=10
#define DELAY_PERIOD        ((10 - 2) * 3)
#define DELAY_PERIOD_MIN    (10)

typedef int (*asr_rws_get_audio_data_fun_t)(void *, void *, int);
typedef int (*asr_rws_result_cb_t)(void *, const char *, int);

struct asr_rws_client_config_t {
    asr_rws_get_audio_data_fun_t get_audio_data_fun;
    asr_rws_result_cb_t result_cb;
    void *priv;
    int timeout_ms;
    int id;

    const char *scheme;
    const char *host;
    const char *path;
    const int port;
    const char *cert;
};

void *asr_rws_client_create(void);
int asr_rws_client_config(void *_hdl, const struct asr_rws_client_config_t *config);
int asr_rws_get_id(void *_hdl);
int asr_rws_client_start(void *hdl);
int asr_rws_client_wait_stop(void *_hdl, int timeout_ms);
int asr_rws_client_stop(void *_hdl, int force);
void asr_rws_client_destroy(void *_hdl);

#endif /* __ASR_RWS_CLIENT__ */