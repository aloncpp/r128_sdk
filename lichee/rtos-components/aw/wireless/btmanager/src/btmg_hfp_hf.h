#ifndef _BTMG_HFP_HF_H_
#define _BTMG_HFP_HF_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "bt_manager.h"

btmg_err bt_hfp_hf_init(void);
btmg_err bt_hfp_hf_deinit(void);
btmg_err bt_hfp_hf_disconnect(const char *addr);
btmg_err bt_hfp_hf_disconnect_audio(const char *addr);
btmg_err bt_hfp_hf_start_voice_recognition(void);
btmg_err bt_hfp_hf_stop_voice_recognition(void);
btmg_err bt_hfp_hf_spk_vol_update(int volume);
btmg_err bt_hfp_hf_mic_vol_update(int volume);
btmg_err bt_hfp_hf_dial(const char *number);
btmg_err bt_hfp_hf_dial_memory(int location);
btmg_err bt_hfp_hf_send_chld_cmd(btmg_hf_chld_type_t chld, int idx);
btmg_err bt_hfp_hf_send_btrh_cmd(btmg_hf_btrh_cmd_t btrh);
btmg_err bt_hfp_hf_answer_call(void);
btmg_err bt_hfp_hf_reject_call(void);
btmg_err bt_hfp_hf_query_calls(void);
btmg_err bt_hfp_hf_query_operator(void);
btmg_err bt_hfp_hf_query_number(void);
btmg_err bt_hfp_hf_send_dtmf(char code);
btmg_err bt_hfp_hf_request_last_voice_tag_number(void);
btmg_err bt_hfp_hf_send_nrec(void);

#ifdef __cplusplus
}
#endif

#endif /* _BTMG_HFP_HF_H_ */
