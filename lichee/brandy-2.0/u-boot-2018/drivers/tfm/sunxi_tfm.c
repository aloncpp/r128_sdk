// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2016 Google, Inc
 * Written by Simon Glass <sjg@chromium.org>
 */

#include <common.h>
#include <stdlib.h>
#include <linux/kernel.h>
#include <sunxi_tfm.h>
#include <securestorage.h>

int tfm_ssk_encrypt(char *out_buf, char *in_buf, int len, int *out_len)
{
	int ret = 0;
	int align_len = 0;

	align_len = ALIGN(len, 64);
	*out_len = align_len;
	flush_cache((unsigned long)in_buf, align_len);
	flush_cache((unsigned long)out_buf, align_len);
	ret = tfm_sunxi_aes_encrypt_with_hardware_ssk((uint8_t *)out_buf, (uint8_t *)in_buf, len);

	invalidate_dcache_range((unsigned long)out_buf, align_len);

	if (ret < 0) {
		printf("tfm encrypt with ssk failed with: %d", ret);

		return -1;
	}

	return 0;
}

int tfm_ssk_decrypt(char *out_buf, char *in_buf, int len)
{

	int ret = 0;
	int align_len = 0;

	align_len = ALIGN(len, 32);
	flush_cache((unsigned long)in_buf, align_len);
	flush_cache((unsigned long)out_buf, align_len);
	ret = tfm_sunxi_aes_decrypt_with_hardware_ssk((uint8_t *)out_buf, (uint8_t *)in_buf, len);

	invalidate_dcache_range((unsigned long)out_buf, align_len);

	if (ret < 0) {
		printf("tfm decrypt with ssk failed with: %d", ret);

		return -1;
	}

	return 0;
}
int tfm_keybox_store(const char *name, char *in_buf, int len)
{
	int ret = 0;
	sunxi_secure_storage_info_t *key_box = NULL;

	key_box = (sunxi_secure_storage_info_t *)malloc(sizeof(sunxi_secure_storage_info_t));
	if (key_box == NULL) {
		printf("key_box malloc fail \n");
		return -1;
	}
	memset(key_box, 0, sizeof(sunxi_secure_storage_info_t));
	memcpy(key_box, in_buf, len);

#if 0 /*debug info*/
	printf("len=%d\n", len);
	printf("name=%s\n", key_box->name);
	printf("len=%d\n",  key_box->len);
	printf("encrypt=%d\n", key_box->encrypted);
	printf("write_protect=%d\n", key_box->write_protect);
	printf("******************\n");
#endif

	if (strcmp(name, key_box->name)) {
		pr_err("name of key %s not match, key data corrupted\n", name);
		free(key_box);
		return -1;
	}
	flush_cache((unsigned long)key_box, sizeof(sunxi_secure_storage_info_t));
	//ret = tfm_sunxi_keybox_store(key_box);

	if (ret < 0) {
		printf("tfm keybox store failed with: %d", ret);
		free(key_box);
		return ret;
	}

	free(key_box);
	return 0;
}

int tfm_check_hash(const char *name, uint8_t *hash)
{
	int ret = 0;

	flush_cache((unsigned long)name, 32);
	flush_cache((unsigned long)hash, 32);
	ret = tfm_sunxi_check_hash(name, hash);

	return ret;
}

