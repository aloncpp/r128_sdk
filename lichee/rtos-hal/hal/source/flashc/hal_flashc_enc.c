/*
 * Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of XRADIO TECHNOLOGY CO., LTD. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hal_log.h>
#include "porting.h"
#include "hal_flash.h"
#include "hal_flashctrl.h"
#include <hal_flashc_enc.h>
#include "hal_xip.h"
#include "sunxi_hal_efuse.h"

//#define ENC_DEBUG
#if defined(ENC_DEBUG)
#define ENC_DBG(fmt, arg...) printf(fmt, ##arg)
#define ENC_WAR(fmt, arg...) hal_log_warn("%s()%d - "fmt, __func__, __LINE__, ##arg)
#else
#define ENC_DBG(fmt, arg...) do{} while(0)
#define ENC_WAR(fmt, arg...) do{} while(0)
#endif
#define ENC_ERR(fmt, arg...) hal_log_err("%s()%d - "fmt, __func__, __LINE__, ##arg)
#define MAX_ENC_CH 6
static uint8_t  Start_Enc_Ch = 0;
static uint32_t Flash_Max_Addr = 0;
//extern void HAL_PRCM_SetFlashCryptoNonce(uint8_t *nonce);
static Flashc_Enc_Cfg *g_Enc_Cfg = NULL;

void printf_enc_config(const Flashc_Enc_Cfg *enc_cfg)
{
	uint8_t i;

	ENC_DBG("==================================\n");
	for (i = 0; i < MAX_ENC_CH; i++) {
		ENC_DBG("ch:     0x%x\n", enc_cfg->ch);
		ENC_DBG("used:      0x%x\n", enc_cfg->used);
		ENC_DBG("enable:    0x%x\n", enc_cfg->enable);
		ENC_DBG("start_addr:0x%x\n", enc_cfg->start_addr);
		ENC_DBG("end_addr:  0x%x\n", enc_cfg->end_addr);
		ENC_DBG("key_0:     0x%x\n", enc_cfg->key_0);
		ENC_DBG("key_1:     0x%x\n", enc_cfg->key_1);
		ENC_DBG("key_2:     0x%x\n", enc_cfg->key_2);
		ENC_DBG("key_3:     0x%x\n", enc_cfg->key_3);
		ENC_DBG("\n");
		enc_cfg++;
	}
	ENC_DBG("==================================\n");
	ENC_DBG("\n");
}

Flashc_Enc_Cfg *get_flashc_enc_cfg(void)
{
	return g_Enc_Cfg;
}

int hal_flashc_enc_set_addr(uint32_t start_addr, uint32_t end_addr, uint8_t ch)
{
	if (ch > MAX_ENC_CH - 1)
		return -1;
#if defined (CONFIG_COMPONENTS_TFM) || defined (CONFIG_COMPONENTS_AMP_TFM)
	extern void tfm_sunxi_flashenc_set_region(uint8_t id, uint32_t saddr, uint32_t eaddr);
	tfm_sunxi_flashenc_set_region(ch, start_addr, end_addr);
#else
	hal_writel(start_addr, (uintptr_t)&(FLASHC_ENC_CFG->ENC_0_RNG_ST) + ch * 0x20);
	hal_writel(end_addr, (uintptr_t)&(FLASHC_ENC_CFG->ENC_0_RNG_ED) + ch * 0x20);
#endif
	return 0;
}

int hal_flashc_enc_set_key(const uint32_t *aes_key, uint8_t ch)
{
	if (ch > MAX_ENC_CH - 1)
		return -1;

#if defined (CONFIG_COMPONENTS_TFM) || defined (CONFIG_COMPONENTS_AMP_TFM)
	#ifdef CONFIG_SECURE_STORAGE_SSK_KEY
		extern void tfm_sunxi_flashenc_set_ssk_key(uint8_t id);
		tfm_sunxi_flashenc_set_ssk_key(ch);
	#else
		extern void tfm_sunxi_flashenc_set_key(uint8_t id, uint32_t *key);
		tfm_sunxi_flashenc_set_key(ch, (uint32_t *)aes_key);
	#endif
#else
	#ifdef CONFIG_SECURE_STORAGE_SSK_KEY
		uint32_t ssk_key[4] = {0};

		hal_efuse_read("ssk", (unsigned char *)ssk_key, 128);
		for(int i = 0; i < 4; i++){
			hal_writel(ssk_key[i], (uintptr_t)FLASHC_ENC_BASE + 0x20 * ch + 0x4 * i + 0x10);
		}
	#else
		hal_writel(aes_key[0], (uintptr_t)&(FLASHC_ENC_CFG->ENC_0_KEY_RG_0) + ch * 0x20);
		hal_writel(aes_key[1], (uintptr_t)&(FLASHC_ENC_CFG->ENC_0_KEY_RG_1) + ch * 0x20);
		hal_writel(aes_key[2], (uintptr_t)&(FLASHC_ENC_CFG->ENC_0_KEY_RG_2) + ch * 0x20);
		hal_writel(aes_key[3], (uintptr_t)&(FLASHC_ENC_CFG->ENC_0_KEY_RG_3) + ch * 0x20);
	#endif
#endif
}

int hal_flashc_enc_enable_ch(uint8_t ch)
{
	if (ch > MAX_ENC_CH - 1)
		return -1;

#if defined (CONFIG_COMPONENTS_TFM) || defined (CONFIG_COMPONENTS_AMP_TFM)
	extern void tfm_sunxi_flashenc_enable(uint8_t id);
	tfm_sunxi_flashenc_enable(ch);
#else
	hal_writel(1, (uintptr_t)&(FLASHC_ENC_CFG->ENC_0_ENABLE) + ch * 0x20);
#endif
	return 0;
}

int hal_flashc_enc_disable_ch(uint8_t ch)
{
	if (ch > MAX_ENC_CH - 1)
		return -1;

#if defined (CONFIG_COMPONENTS_TFM) || defined (CONFIG_COMPONENTS_AMP_TFM)
	extern void tfm_sunxi_flashenc_disable(uint8_t id);
	tfm_sunxi_flashenc_disable(ch);
#else
	hal_writel(0, (uintptr_t)&(FLASHC_ENC_CFG->ENC_0_ENABLE) + ch * 0x20);
#endif

	return 0;
}

int hal_flashc_enc_alloc_ch(void)
{
	int i;
	Flashc_Enc_Cfg *enc_cfg = get_flashc_enc_cfg();

	if (enc_cfg == NULL) {
		ENC_ERR("err: invalid enc_cfg.\n");
		return -1;
	}
	enc_cfg += Start_Enc_Ch;
	for (i = Start_Enc_Ch; i< MAX_ENC_CH; i++) {
		if (!enc_cfg->used)
			break;
		enc_cfg++;
	}

	if (i == MAX_ENC_CH) {
		ENC_ERR("err: alloc channel failed.\n");
		i = -1;
	}
	return i;
}

int hal_flashc_enc_free_ch(const Flashc_Enc_Cfg *enc_set)
{
	int i;
	Flashc_Enc_Cfg *enc_cfg = get_flashc_enc_cfg();

	if (enc_cfg == NULL) {
		ENC_ERR("err: invalid enc_cfg.\n");
		return -1;
	}

	if (enc_set->ch > MAX_ENC_CH - 1) {
		ENC_ERR("err: invalid channel.\n");
		return -1;
	}

	enc_cfg += enc_set->ch;
	enc_cfg->enable = 0;
	enc_cfg->used = 0;
	hal_flashc_enc_disable_ch(enc_cfg->ch);

#if defined(CONFIG_XIP) && defined(FLASH_XIP_OPT_READ)
	enc_cfg = get_flashc_enc_cfg();
	enc_cfg += enc_set->ch + 1;
	enc_cfg->enable = 0;
	enc_cfg->used = 0;
	hal_flashc_enc_disable_ch(enc_cfg->ch);
#endif
    return 0;
}

int hal_flashc_enc_enable(const Flashc_Enc_Cfg *enc_set)
{
	Flashc_Enc_Cfg *enc_cfg = get_flashc_enc_cfg();

	if (enc_cfg == NULL) {
		ENC_ERR("err: invalid enc_cfg.\n");
		return -1;
	}

	enc_cfg = get_flashc_enc_cfg();
	enc_cfg += enc_set->ch;

	enc_cfg->enable = 1;
	hal_flashc_enc_enable_ch(enc_cfg->ch);
#if defined(CONFIG_XIP) && defined(FLASH_XIP_OPT_READ)
	enc_cfg = get_flashc_enc_cfg();
	enc_cfg += enc_set->ch + 1;

	enc_cfg->enable = 1;
	hal_flashc_enc_enable_ch(enc_cfg->ch);
#endif
	return 0;
}

int hal_flashc_enc_disable(const Flashc_Enc_Cfg *enc_set)
{
	Flashc_Enc_Cfg *enc_cfg = get_flashc_enc_cfg();

	if (enc_cfg == NULL) {
		ENC_ERR("err: invalid enc_cfg.\n");
		return -1;
	}

	enc_cfg = get_flashc_enc_cfg();
	enc_cfg += enc_set->ch;

	enc_cfg->enable = 0;
	hal_flashc_enc_disable_ch(enc_cfg->ch);
#if defined(CONFIG_XIP) && defined(FLASH_XIP_OPT_READ)
	enc_cfg = get_flashc_enc_cfg();
	enc_cfg += enc_set->ch + 1;

	enc_cfg->enable = 0;
	hal_flashc_enc_disable_ch(enc_cfg->ch);
#endif
	return 0;
}

int hal_flashc_set_enc(const Flashc_Enc_Cfg *enc_set)
{
	uint8_t i;
	Flashc_Enc_Cfg *enc_cfg = get_flashc_enc_cfg();

	if (enc_cfg == NULL) {
		ENC_ERR("err: invalid enc_cfg.\n");
		return -1;
	}

	if (enc_set->ch > MAX_ENC_CH - 1) {
		ENC_ERR("err: invalid channel.\n");
		return -1;
	}

	if (enc_cfg[enc_set->ch].used) {
		ENC_ERR("err: channel have been using.\n");
		return -1;
	}

	if (enc_set->start_addr >= enc_set->end_addr || enc_cfg->end_addr >= Flash_Max_Addr) {
		ENC_ERR("err: invalid addr, out of range.\n");
		return -1;
	}

	for (i = 0; i< MAX_ENC_CH; i++) {
		if (enc_cfg->used) {
			if ((enc_set->start_addr > enc_cfg->start_addr && enc_set->start_addr < enc_cfg->end_addr) ||
				(enc_set->end_addr > enc_cfg->start_addr && enc_set->end_addr < enc_cfg->end_addr)) {

				ENC_ERR("err: invalid addr.\n");
				return -1;
			}
		}
		enc_cfg++;
	}

	enc_cfg = get_flashc_enc_cfg();
	enc_cfg += enc_set->ch;

	enc_cfg->start_addr = enc_set->start_addr;
	enc_cfg->end_addr = enc_set->end_addr;
	enc_cfg->ch = enc_set->ch;
	enc_cfg->enable = enc_set->enable;
	enc_cfg->used = 1;
	enc_cfg->key_0 = enc_set->key_0;
	enc_cfg->key_1 = enc_set->key_1;
	enc_cfg->key_2 = enc_set->key_2;
	enc_cfg->key_3 = enc_set->key_3;
	hal_flashc_enc_set_addr(enc_cfg->start_addr, enc_cfg->end_addr, enc_cfg->ch);
	hal_flashc_enc_set_key(&enc_cfg->key_0, enc_cfg->ch);
	hal_flashc_enc_enable_ch(enc_cfg->ch);

#if defined(CONFIG_XIP) && defined(FLASH_XIP_OPT_READ)
	enc_cfg = get_flashc_enc_cfg();
	enc_cfg += enc_set->ch + 1;

	enc_cfg->start_addr = enc_set->start_addr + FLASH_XIP_START_ADDR;
	enc_cfg->end_addr = enc_set->end_addr + FLASH_XIP_START_ADDR;
	enc_cfg->ch = enc_set->ch + 1;
	enc_cfg->enable = enc_set->enable;
	enc_cfg->used = 1;
	enc_cfg->key_0 = enc_set->key_0;
	enc_cfg->key_1 = enc_set->key_1;
	enc_cfg->key_2 = enc_set->key_2;
	enc_cfg->key_3 = enc_set->key_3;
	hal_flashc_enc_set_addr(enc_cfg->start_addr, enc_cfg->end_addr, enc_cfg->ch);
	hal_flashc_enc_set_key(&enc_cfg->key_0, enc_cfg->ch);
	hal_flashc_enc_enable_ch(enc_cfg->ch);
#endif
	return 0;
}

HAL_Status HAL_Flashc_Enc_Init(Flashc_Enc_Cfg *enc_cfg)
{
	uint8_t i, j, k=0;
	Flashc_Enc_Cfg *tmp_enc_cfg = enc_cfg, *cmp_enc_cfg;

	for (i = 0; i < MAX_ENC_CH ; i++) {
		enc_cfg->ch = i;
		if (enc_cfg->used) {
			#if 1
			cmp_enc_cfg = tmp_enc_cfg;
			if (enc_cfg->start_addr >= enc_cfg->end_addr || enc_cfg->end_addr >= Flash_Max_Addr) {
				ENC_ERR("err: ch:%d,addr out of range.\n", enc_cfg->ch);
				continue;
			}

			for (j = 0; j < k; j++) {
				if ((enc_cfg->start_addr > cmp_enc_cfg->start_addr && enc_cfg->start_addr < cmp_enc_cfg->end_addr) ||
					(enc_cfg->end_addr > cmp_enc_cfg->start_addr && enc_cfg->end_addr < cmp_enc_cfg->end_addr)) {
					ENC_ERR("err: invalid addr.\n");
					break;
				}
				cmp_enc_cfg++;
			}

			if (j == k) {
				hal_flashc_enc_set_addr(enc_cfg->start_addr, enc_cfg->end_addr, enc_cfg->ch);
				hal_flashc_enc_set_key(&enc_cfg->key_0, enc_cfg->ch);
				hal_flashc_enc_enable_ch(enc_cfg->ch);
				k++;
			}
			#else
			ENC_DBG("ch:0x%x\n", enc_cfg->ch);
			hal_flashc_enc_set_addr(enc_cfg->start_addr, enc_cfg->end_addr, enc_cfg->ch);
			hal_flashc_enc_set_key(&enc_cfg->key_0, enc_cfg->ch);
			hal_flashc_enc_disable_ch(enc_cfg->ch);
			#endif

		}
		enc_cfg++;
	}
}

int hal_flashc_enc_init(uint32_t max_addr, uint8_t start_ch)
{
	uint8_t i;
	Flashc_Enc_Cfg xip_enc_cfg[2];

	Flash_Max_Addr = max_addr;
	Start_Enc_Ch = start_ch;
	ENC_DBG("flashc_enc_init...\n");

	g_Enc_Cfg = malloc(sizeof(Flashc_Enc_Cfg)*MAX_ENC_CH);
	if (g_Enc_Cfg == NULL) {
		ENC_ERR("err: malloc failed.\n");
		return -1;
	}
	memset(g_Enc_Cfg, 0, sizeof(Flashc_Enc_Cfg)*MAX_ENC_CH);

	HAL_Flashc_Enc_Init(g_Enc_Cfg);
	//HAL_PRCM_SetFlashCryptoNonce(nonce_key);
	printf_enc_config(g_Enc_Cfg);

	ENC_DBG("flashc_enc_init end.\n");
	return 0;
}

//api test
int user_enc_api_test(void)
{
	Flashc_Enc_Cfg enc_set;
	Flashc_Enc_Cfg *enc_cfg = get_flashc_enc_cfg();
	uint32_t aes_key[4] = {0x12345677, 0x12345677, 0x12345677, 0x12345677};
	ENC_DBG("user api test...\n");
	printf_enc_config(enc_cfg);
	enc_set.ch = hal_flashc_enc_alloc_ch();
	if (enc_set.ch < 0) {
		ENC_ERR("err: alloc channel failed.\n");
		return -1;
	}

	enc_set.start_addr = 0x800000;
	enc_set.end_addr   = 0x900000;
	enc_set.key_0 = 0x12345678;
	enc_set.key_1 = 0x12345678;
	enc_set.key_2 = 0x12345678;
	enc_set.key_3 = 0x12345678;
	enc_set.enable = 1;
	hal_flashc_set_enc(&enc_set);

	printf_enc_config(enc_cfg);

	hal_flashc_enc_set_addr(0xA00000, 0xB00000, 0);
	hal_flashc_enc_set_key(aes_key, 0);
	hal_flashc_enc_enable_ch(0);

	ENC_DBG("user api test end.\n");
	return 0;
}

