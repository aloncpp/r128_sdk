#ifndef __TFM_SUNXI_NSC_FUNCTION_H__
#define __TFM_SUNXI_NSC_FUNCTION_H__

typedef void ( *Callback_t ) ( void );
uint32_t tfm_sunxi_nsc_func(Callback_t pxCallback);

int tfm_sunxi_efuse_write(char *key_name, unsigned char *key_data, unsigned int key_bit_len);

int tfm_sunxi_aes_with_hardware(crypto_aes_req_ctx_t *aes_ctx);
int tfm_sunxi_aes_encrypt_with_hardware_ssk(uint8_t *dst_addr, uint8_t *src_addr, int len);
int tfm_sunxi_keybox_store(void *in_buf);
int tfm_sunxi_check_hash(const char *name, uint8_t *hash);

void tfm_sunxi_flashenc_set_nonce(uint8_t *nonce);
void tfm_sunxi_flashenc_config(uint8_t id, uint32_t saddr, uint32_t eaddr, uint32_t *key);
void tfm_sunxi_flashenc_set_region(uint8_t id, uint32_t saddr, uint32_t eaddr);
void tfm_sunxi_flashenc_set_key(uint8_t id, uint32_t *key);
void tfm_sunxi_flashenc_set_ssk_key(uint8_t id);
void tfm_sunxi_flashenc_enable(uint8_t id);
void tfm_sunxi_flashenc_disable(uint8_t id);

#endif /* __TFM_SUNXI_NSC_FUNCTION_H__ */
