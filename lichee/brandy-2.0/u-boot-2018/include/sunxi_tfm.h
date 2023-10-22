// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2016 Google, Inc
 * Written by Simon Glass <sjg@chromium.org>
 */

#ifndef __SUNXI_TFM_H__
#define __SUNXI_TFM_H__

extern int tfm_sunxi_efuse_write(char *key_name, unsigned char *key_data, unsigned int key_bit_len);

extern int tfm_sunxi_aes_encrypt_with_hardware_ssk(uint8_t *dst_addr, uint8_t *src_addr, int len);
extern int tfm_sunxi_aes_decrypt_with_hardware_ssk(uint8_t *dst_addr, uint8_t *src_addr, int len);
extern int tfm_sunxi_keybox_store(void *in_buf);
extern int tfm_sunxi_check_hash(const char *name, uint8_t *hash);

extern void tfm_sunxi_flashenc_set_nonce(uint8_t *nonce);
extern void tfm_sunxi_flashenc_config(uint8_t id, uint32_t saddr, uint32_t eaddr, uint32_t *key);
extern void tfm_sunxi_flashenc_set_region(uint8_t id, uint32_t saddr, uint32_t eaddr);
extern void tfm_sunxi_flashenc_set_key(uint8_t id, uint32_t *key);
extern void tfm_sunxi_flashenc_set_ssk_key(uint8_t id);
extern void tfm_sunxi_flashenc_enable(uint8_t id);
extern void tfm_sunxi_flashenc_disable(uint8_t id);

int tfm_ssk_encrypt(char *out_buf, char *in_buf, int len, int *out_len);
int tfm_ssk_decrypt(char *out_buf, char *in_buf, int len);
int tfm_keybox_store(const char *name, char *in_buf, int len);
int tfm_check_hash(const char *name, uint8_t *hash);

#endif /* __TFM_SUNXI_NSC_FUNCTION_H__ */
