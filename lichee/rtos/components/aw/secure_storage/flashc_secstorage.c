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
#include <blkpart.h>
#include <part_efi.h>
#include <sunxi_hal_common.h>
#include <hal_flashc_enc.h>

extern int nor_read(unsigned int addr, char *buf, unsigned int size);

static int nor_get_gpt(char *buf, int len)
{
    if (len < GPT_TABLE_SIZE) {
        printf("buf too small for gpt\n");
        return -1;
    }

    return nor_read(GPT_ADDRESS, buf, GPT_TABLE_SIZE);
}

static int get_secret_part_info(uint32_t *start_addr, uint32_t *size)
{
    int ret, index;
    char *gpt_buf;
    struct nor_flash *nor;
    struct gpt_part *gpt_part;
    struct part *part;
    int start_sector;
    int sectors;

#ifdef CONFIG_FLASHC_CPU_XFER_ONLY
    gpt_buf = malloc(GPT_TABLE_SIZE);
#else
    gpt_buf = hal_malloc_coherent(GPT_TABLE_SIZE);
#endif
    if (!gpt_buf) {
        ret = -1;
        goto err;
    }
    memset(gpt_buf, 0, GPT_TABLE_SIZE);

    ret = nor_get_gpt(gpt_buf, GPT_TABLE_SIZE);
    if (ret) {
        printf("get gpt from nor flash failed - %d\n", ret);
        ret = -1;
        goto err;
    }
    ret = get_part_info_by_name(gpt_buf, "secret", &start_sector, &sectors);
    if (ret < 0) {
        printf("get secret part failed - %d\n", ret);
	goto err;
    }
err:
#ifdef CONFIG_FLASHC_CPU_XFER_ONLY
    free(gpt_buf);
#else
    hal_free_coherent(gpt_buf);
#endif
    //printf("start_sector: 0x%0x sectors: 0x%0x\n", start_sector, sectors);
    *start_addr = start_sector * 512;
    *size = sectors * 512;
    return 0;
}

#ifdef CONFIG_HAL_TEST_FLASHC_ENC
Flashc_Enc_Cfg ss_enc_set;
#endif
int secret_storage_init(uint32_t *aes_key)
{
	int ret;
	uint32_t start_addr, size;
#ifndef CONFIG_HAL_TEST_FLASHC_ENC
	Flashc_Enc_Cfg ss_enc_set;
#endif
	Flashc_Enc_Cfg *enc_cfg = get_flashc_enc_cfg();
	printf_enc_config(enc_cfg);
	ss_enc_set.ch = hal_flashc_enc_alloc_ch();
	if (ss_enc_set.ch < 0) {
		printf("err: alloc channel failed.\n");
		return -1;
	}

	ret = get_secret_part_info(&start_addr, &size);
	ss_enc_set.start_addr = start_addr;
	ss_enc_set.end_addr   = start_addr + size;
	ss_enc_set.key_0 = aes_key[0];
	ss_enc_set.key_1 = aes_key[1];
	ss_enc_set.key_2 = aes_key[2];
	ss_enc_set.key_3 = aes_key[3];
	ss_enc_set.enable = 1;
	hal_flashc_set_enc(&ss_enc_set);
	printf_enc_config(enc_cfg);
    return 0;
}
