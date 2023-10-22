/*
 * (C) Copyright 2013-2016
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/ce.h>
#include <memalign.h>
#include "ss_op.h"

#define ALG_SHA256 (0x13)
#define ALG_SHA512 (0x15)
#define ALG_RSA (0x20)
#define ALG_MD5 (0x10)
#define ALG_TRANG (0x1C)

/*check if task_queue size is cache_line align*/
#define STATIC_CHECK(condition) extern u8 checkFailAt##__LINE__[-!(condition)];
STATIC_CHECK(sizeof(task_queue) == ALIGN(sizeof(task_queue), CACHE_LINE_SIZE))

static u32 __aw_endian4(u32 data)
{
	u32 d1, d2, d3, d4;
	d1 = (data & 0xff) << 24;
	d2 = (data & 0xff00) << 8;
	d3 = (data & 0xff0000) >> 8;
	d4 = (data & 0xff000000) >> 24;

	return (d1 | d2 | d3 | d4);
}

static u32 __sha_padding(u32 data_size, u8 *text, u32 hash_mode)
{
	u32 i;
	u32 k, q;
	u32 size;
	u32 padding_buf[16];
	u8 *ptext;

	k = data_size / 64;
	q = data_size % 64;

	ptext = (u8 *)padding_buf;
	memset(padding_buf, 0, 16 * sizeof(u32));
	if (q == 0) {
		padding_buf[0] = 0x00000080;

		if (hash_mode) {
			padding_buf[14] = data_size >> 29;
			padding_buf[15] = data_size << 3;
			padding_buf[14] = __aw_endian4(padding_buf[14]);
			padding_buf[15] = __aw_endian4(padding_buf[15]);
		} else {
			padding_buf[14] = data_size << 3;
			padding_buf[15] = data_size >> 29;
		}

		for (i = 0; i < 64; i++) {
			text[k * 64 + i] = ptext[i];
		}
		size = (k + 1) * 64;
	} else if (q < 56) {
		for (i = 0; i < q; i++) {
			ptext[i] = text[k * 64 + i];
		}
		ptext[q] = 0x80;

		if (hash_mode) {
			padding_buf[14] = data_size >> 29;
			padding_buf[15] = data_size << 3;
			padding_buf[14] = __aw_endian4(padding_buf[14]);
			padding_buf[15] = __aw_endian4(padding_buf[15]);
		} else {
			padding_buf[14] = data_size << 3;
			padding_buf[15] = data_size >> 29;
		}

		for (i = 0; i < 64; i++) {
			text[k * 64 + i] = ptext[i];
		}
		size = (k + 1) * 64;
	} else {
		for (i = 0; i < q; i++) {
			ptext[i] = text[k * 64 + i];
		}
		ptext[q] = 0x80;
		for (i = 0; i < 64; i++) {
			text[k * 64 + i] = ptext[i];
		}

		/*send last 512-bits text to SHA1/MD5*/
		for (i = 0; i < 16; i++) {
			padding_buf[i] = 0x0;
		}

		if (hash_mode) {
			padding_buf[14] = data_size >> 29;
			padding_buf[15] = data_size << 3;
			padding_buf[14] = __aw_endian4(padding_buf[14]);
			padding_buf[15] = __aw_endian4(padding_buf[15]);
		} else {
			padding_buf[14] = data_size << 3;
			padding_buf[15] = data_size >> 29;
		}

		for (i = 0; i < 64; i++) {
			text[(k + 1) * 64 + i] = ptext[i];
		}
		size = (k + 2) * 64;
	}

	return size;
}

static void __rsa_padding(u8 *dst_buf, u8 *src_buf, u32 data_len, u32 group_len)
{
	int i = 0;

	memset(dst_buf, 0, group_len);
	for (i = group_len - data_len; i < group_len; i++) {
		dst_buf[i] = src_buf[group_len - 1 - i];
	}
}

void sunxi_ss_open(void)
{
	ss_open();
}

void sunxi_ss_close(void)
{
	ss_close();
}

int sunxi_md5_calc(u8 *dst_addr, u32 dst_len, u8 *src_addr, u32 src_len)
{
	u32 word_len = 0, src_align_len = 0;
	u32 total_bit_len			    = 0;
	task_queue task0 __aligned(CACHE_LINE_SIZE) = { 0 };
	/* sha256  2word, sha512 4word*/
	ALLOC_CACHE_ALIGN_BUFFER(u32, total_package_len,
				 CACHE_LINE_SIZE / sizeof(u32));
	ALLOC_CACHE_ALIGN_BUFFER(u8, p_sign, CACHE_LINE_SIZE);

	memset(p_sign, 0, sizeof(p_sign));

	if (ss_get_ver() < 2) {
		/* CE1.0 */
		src_align_len = __sha_padding(src_len, (u8 *)src_addr, 1);
		word_len      = src_align_len >> 2;

		task0.task_id    = 0;
		task0.common_ctl = (ALG_MD5) | (1U << 31);
		task0.data_len   = word_len;

	} else {
		/* CE2.0 */
		src_align_len = ALIGN(src_len, 4);
		word_len      = src_align_len >> 2;

		total_bit_len	= src_len << 3;
		total_package_len[0] = total_bit_len;
		total_package_len[1] = 0;

		task0.task_id	= 0;
		task0.common_ctl     = (ALG_MD5) | (1 << 15) | (1U << 31);
		task0.key_descriptor = (u32)total_package_len;
		task0.data_len       = total_bit_len;
	}

	task0.source[0].addr	= (uint)src_addr;
	task0.source[0].length      = word_len;
	task0.destination[0].addr   = (uint)p_sign;
	task0.destination[0].length = dst_len >> 2;
	task0.next_descriptor       = 0;

	flush_cache((u32)&task0, sizeof(task0));
	flush_cache((u32)p_sign, CACHE_LINE_SIZE);
	flush_cache((u32)src_addr, src_align_len);
	flush_cache((u32)total_package_len, CACHE_LINE_SIZE);

	ss_set_drq((u32)&task0);
	ss_irq_enable(task0.task_id);
	ss_ctrl_start(ALG_MD5);
	ss_wait_finish(task0.task_id);
	ss_pending_clear(task0.task_id);
	ss_ctrl_stop();
	ss_irq_disable(task0.task_id);
	if (ss_check_err(0)) {
		printf("SS %s fail 0x%x\n", __func__, ss_check_err(0));
	}

	invalidate_dcache_range((ulong)p_sign, (ulong)p_sign + CACHE_LINE_SIZE);
	/*copy data*/
	memcpy(dst_addr, p_sign, dst_len);
	return 0;
}

int sunxi_sha_calc(u8 *dst_addr, u32 dst_len, u8 *src_addr, u32 src_len)
{
	u32 total_bit_len			    = 0;
	task_queue_other task0 __aligned(CACHE_LINE_SIZE) = { 0 };
	/* sha256  2word, sha512 4word*/
	ALLOC_CACHE_ALIGN_BUFFER(u8, p_sign, CACHE_LINE_SIZE);
	u32 align_shift = ss_get_addr_align();
	u32 md_size = 32;
	memset(p_sign, 0, sizeof(p_sign));

	/* CE2.1 */

	total_bit_len	= src_len * 8;
	task0.ctrl = (CHANNEL_0 << CHN) | (0x1 << LPKG) |
		     (0x0 << DLAV) | (0x1 << IE);
	task0.cmd		    = (SUNXI_SHA256 << 0);
	task0.data_toal_len_addr    = total_bit_len;
	task0.hmac_prng_key_addr    = 0;
	task0.iv_addr		    = 0;

	task0.source[0].addr	= ((u32)src_addr >> align_shift);
	task0.source[0].length		= ((src_len & 0x3) == 0?src_len>>2:(src_len>>2)+1)<<2;
	task0.destination[0].addr	= ((u32)p_sign >> align_shift);
	task0.destination[0].length = md_size;

	task0.next_descriptor_addr  = 0;

	flush_cache(((u32)&task0), ALIGN(sizeof(task0), CACHE_LINE_SIZE));
	flush_cache((u32)p_sign, CACHE_LINE_SIZE);
	flush_cache(((u32)src_addr), ALIGN(src_len, CACHE_LINE_SIZE));

	ss_set_drq(((u32)&task0) >> align_shift);
	ss_irq_enable(CHANNEL_0);
	ss_ctrl_start(HASH_RBG_TRPE);
	ss_wait_finish(CHANNEL_0);
	ss_pending_clear(CHANNEL_0);
	ss_ctrl_stop();
	ss_irq_disable(CHANNEL_0);
	ss_set_drq(0);
	if (ss_check_err(CHANNEL_0)) {
		printf("SS %s fail 0x%x\n", __func__,
			ss_check_err(CHANNEL_0));
		return -1;
	}

	invalidate_dcache_range((ulong)p_sign, ((ulong)p_sign) + CACHE_LINE_SIZE);
	/*copy data*/
	memcpy(dst_addr, p_sign, md_size);
	return 0;
}

int sunxi_hash_test(u8 *dst_addr, u32 dst_len, u8 *src_addr, u32 src_len, u32 sha_type)
{
	u32 total_bit_len			    = 0;
	task_queue_other task0 __aligned(CACHE_LINE_SIZE) = { 0 };
	/* sha256  2word, sha512 4word*/
	ALLOC_CACHE_ALIGN_BUFFER(u8, p_sign, CACHE_LINE_SIZE);
	u32 align_shift = ss_get_addr_align();
	u32 md_size = 32;
	memset(p_sign, 0, sizeof(p_sign));

	/* CE2.1 */

	total_bit_len	= src_len * 8;
	task0.ctrl = (CHANNEL_0 << CHN) | (0x1 << LPKG) |
		     (0x0 << DLAV) | (0x1 << IE);
	task0.cmd		    = (sha_type << 0);
	task0.data_toal_len_addr    = total_bit_len;
	task0.hmac_prng_key_addr    = 0;
	task0.iv_addr		    = 0;

	task0.source[0].addr	= ((u32)src_addr >> align_shift);
	task0.source[0].length		= ((src_len & 0x3) == 0?src_len>>2:(src_len>>2)+1)<<2;
	task0.destination[0].addr	= ((u32)p_sign >> align_shift);
	task0.destination[0].length = md_size;

	task0.next_descriptor_addr  = 0;

	flush_cache(((u32)&task0), ALIGN(sizeof(task0), CACHE_LINE_SIZE));
	flush_cache((u32)p_sign, CACHE_LINE_SIZE);
	flush_cache(((u32)src_addr), ALIGN(src_len, CACHE_LINE_SIZE));

	ss_set_drq(((u32)&task0) >> align_shift);
	ss_irq_enable(CHANNEL_0);
	ss_ctrl_start(HASH_RBG_TRPE);
	ss_wait_finish(CHANNEL_0);
	ss_pending_clear(CHANNEL_0);
	ss_ctrl_stop();
	ss_irq_disable(CHANNEL_0);
	ss_set_drq(0);
	if (ss_check_err(CHANNEL_0)) {
		printf("SS %s fail 0x%x\n", __func__,
			ss_check_err(CHANNEL_0));
		return -1;
	}

	invalidate_dcache_range((ulong)p_sign, ((ulong)p_sign) + CACHE_LINE_SIZE);
	/*copy data*/
	memcpy(dst_addr, p_sign, md_size);
	return 0;
}

s32 sunxi_sm2_test(struct sunxi_sm2_ctx_t *sm2_ctx)
{
	task_queue task0 __aligned(CACHE_LINE_SIZE) = { 0 };
	u32 sm2_size_byte = sm2_ctx->sm2_size >> 3;
	u32 sm2_size_word = sm2_ctx->sm2_size >> 5;
	u32 mode = sm2_ctx->mode;
	u32 align_shift = ss_get_addr_align();
	int i;

	memset((void *)&task0, 0x00, sizeof(task0));

	task0.task_id	     = CHANNEL_0;
	task0.common_ctl     = (ALG_SM2 | (1U << 31));
	task0.asymmetric_ctl = (sm2_size_word) | (mode << 16);

	task0.source[0].addr = ((ulong)sm2_ctx->k >> align_shift);
	task0.source[0].length = (sm2_ctx->k_len >> 2);
	task0.source[1].addr = ((ulong)sm2_ctx->p >> align_shift);
	task0.source[1].length = sm2_size_word;
	task0.source[2].addr = ((ulong)sm2_ctx->a >> align_shift);
	task0.source[2].length = sm2_size_word;
	task0.source[3].addr = ((ulong)sm2_ctx->gx >> align_shift);
	task0.source[3].length = sm2_size_word;
	task0.source[4].addr = ((ulong)sm2_ctx->gy >> align_shift);
	task0.source[4].length = sm2_size_word;
	task0.source[5].addr = ((ulong)sm2_ctx->n >> align_shift);
	task0.source[5].length = sm2_size_word;
	task0.source[6].addr = ((ulong)sm2_ctx->px >> align_shift);
	task0.source[6].length = sm2_size_word;
	task0.source[7].addr = ((ulong)sm2_ctx->py >> align_shift);
	task0.source[7].length = sm2_size_word;

	for (i = 0; i < 8; i++) {
		task0.data_len += task0.source[i].length;
	}
	pr_err("data_len = %d\n", task0.data_len);

	task0.destination[0].addr = ((ulong)sm2_ctx->cx >> align_shift);
	task0.destination[0].length = sm2_size_word;
	task0.destination[1].addr = ((ulong)sm2_ctx->cy >> align_shift);
	task0.destination[1].length = sm2_size_word;
	task0.destination[2].addr = ((ulong)sm2_ctx->kx >> align_shift);
	task0.destination[2].length = sm2_size_word;
	task0.destination[3].addr = ((ulong)sm2_ctx->ky >> align_shift);
	task0.destination[3].length = sm2_size_word;
	task0.destination[4].addr = ((ulong)sm2_ctx->out >> align_shift);
	task0.destination[4].length = sm2_size_word;

	flush_cache((ulong)&task0, sizeof(task0));
	flush_cache((ulong)sm2_ctx->p, sm2_size_byte);
	flush_cache((ulong)sm2_ctx->a, sm2_size_byte);
	flush_cache((ulong)sm2_ctx->gx, sm2_size_byte);
	flush_cache((ulong)sm2_ctx->gy, sm2_size_byte);
	flush_cache((ulong)sm2_ctx->n, sm2_size_byte);
	flush_cache((ulong)sm2_ctx->px, sm2_size_byte);
	flush_cache((ulong)sm2_ctx->py, sm2_size_byte);


	ss_set_drq((u32)&task0);
	ss_irq_enable(task0.task_id);
	ss_ctrl_start(ASYM_TRPE);
	ss_wait_finish(task0.task_id);
	ss_pending_clear(task0.task_id);
	ss_ctrl_stop();
	ss_irq_disable(task0.task_id);
	if (ss_check_err(task0.task_id)) {
		printf("SS %s fail 0x%x\n", __func__,
		       ss_check_err(task0.task_id));
		return -1;
	}

	invalidate_dcache_range((ulong)sm2_ctx->cx,
				(ulong)sm2_ctx->cx + sm2_size_byte);
	invalidate_dcache_range((ulong)sm2_ctx->cy,
				(ulong)sm2_ctx->cy + sm2_size_byte);
	invalidate_dcache_range((ulong)sm2_ctx->kx,
				(ulong)sm2_ctx->kx + sm2_size_byte);
	invalidate_dcache_range((ulong)sm2_ctx->ky,
				(ulong)sm2_ctx->ky + sm2_size_byte);
	invalidate_dcache_range((ulong)sm2_ctx->out,
				(ulong)sm2_ctx->out + sm2_size_byte);

	return 0;
}

s32 sunxi_normal_rsa(u8 *n_addr, u32 n_len, u8 *e_addr, u32 e_len, u8 *dst_addr,
		     u32 dst_len, u8 *src_addr, u32 src_len)
{
	const u32 TEMP_BUFF_LEN = ((2048 >> 3) + CACHE_LINE_SIZE);

	u32 mod_bit_size			    = 2048;
	u32 mod_size_len_inbytes		    = mod_bit_size / 8;
	u32 data_word_len			    = mod_size_len_inbytes / 4;
	u32 align_shift				    = ss_get_addr_align();
	task_queue task0 __aligned(CACHE_LINE_SIZE) = { 0 };
	ALLOC_CACHE_ALIGN_BUFFER(u8, p_n, TEMP_BUFF_LEN);
	ALLOC_CACHE_ALIGN_BUFFER(u8, p_e, TEMP_BUFF_LEN);
	ALLOC_CACHE_ALIGN_BUFFER(u8, p_src, TEMP_BUFF_LEN);
	ALLOC_CACHE_ALIGN_BUFFER(u8, p_dst, TEMP_BUFF_LEN);

	memset(p_src, 0, mod_size_len_inbytes);
	memcpy(p_src, src_addr, src_len);
	memset(p_n, 0, mod_size_len_inbytes);
	memcpy(p_n, n_addr, n_len);
	memset(p_e, 0, mod_size_len_inbytes);
	memcpy(p_e, e_addr, e_len);
	/*CE2.1*/
	task0.task_id	  = CHANNEL_0;
	task0.common_ctl       = (ALG_RSA | (1U << 31));
	task0.symmetric_ctl    = 0;
	task0.asymmetric_ctl   = (2048 >> 5);
	task0.source[0].addr   = ((uint)p_e >> align_shift);
	task0.source[0].length = data_word_len;
	task0.source[1].addr   = ((uint)p_n >> align_shift);
	task0.source[1].length = data_word_len;
	task0.source[2].addr   = ((uint)p_src >> align_shift);
	task0.source[2].length = data_word_len;
	task0.data_len += task0.source[0].length;
	task0.data_len += task0.source[1].length;
	task0.data_len += task0.source[2].length;
	task0.data_len *= 4;

	task0.destination[0].addr   = ((uint)p_dst >> align_shift);
	task0.destination[0].length = data_word_len;
	task0.next_descriptor       = 0;

	flush_cache((u32)&task0, sizeof(task0));
	flush_cache((u32)p_n, mod_size_len_inbytes);
	flush_cache((u32)p_e, mod_size_len_inbytes);
	flush_cache((u32)p_src, mod_size_len_inbytes);
	flush_cache((u32)p_dst, mod_size_len_inbytes);

	ss_set_drq(((u32)&task0) >> align_shift);
	ss_irq_enable(task0.task_id);
	ss_ctrl_start(ASYM_TRPE);
	ss_wait_finish(task0.task_id);
	ss_pending_clear(task0.task_id);
	ss_ctrl_stop();
	ss_irq_disable(task0.task_id);
	if (ss_check_err(task0.task_id)) {
		printf("SS %s fail 0x%x\n", __func__,
		       ss_check_err(task0.task_id));
		return -1;
	}

	invalidate_dcache_range((ulong)p_dst,
				(ulong)p_dst + mod_size_len_inbytes);
	memcpy(dst_addr, p_dst, mod_size_len_inbytes);

	return 0;
}

s32 sunxi_rsa_calc(u8 *n_addr, u32 n_len, u8 *e_addr, u32 e_len, u8 *dst_addr,
		   u32 dst_len, u8 *src_addr, u32 src_len)
{
	const u32 TEMP_BUFF_LEN = ((2048 >> 3) + CACHE_LINE_SIZE);

	u32 mod_bit_size	 = 2048;
	u32 mod_size_len_inbytes = mod_bit_size / 8;
	u32 data_word_len	= mod_size_len_inbytes / 4;
	u32 align_shift = ss_get_addr_align();
	task_queue task0 __aligned(CACHE_LINE_SIZE) = { 0 };
	ALLOC_CACHE_ALIGN_BUFFER(u8, p_n, TEMP_BUFF_LEN);
	ALLOC_CACHE_ALIGN_BUFFER(u8, p_e, TEMP_BUFF_LEN);
	ALLOC_CACHE_ALIGN_BUFFER(u8, p_src, TEMP_BUFF_LEN);
	ALLOC_CACHE_ALIGN_BUFFER(u8, p_dst, TEMP_BUFF_LEN);

	__rsa_padding(p_src, src_addr, src_len, mod_size_len_inbytes);
	__rsa_padding(p_n, n_addr, n_len, mod_size_len_inbytes);
	memset(p_e, 0, mod_size_len_inbytes);
	memcpy(p_e, e_addr, e_len);
	/*CE2.1*/
	task0.task_id	= CHANNEL_0;
	task0.common_ctl     = (ALG_RSA | (1U << 31));
	task0.symmetric_ctl  = 0;
	task0.asymmetric_ctl = (2048 >> 5);
	task0.source[0].addr   = ((uint)p_e >> align_shift);
	task0.source[0].length = data_word_len;
	task0.source[1].addr   = ((uint)p_n >> align_shift);
	task0.source[1].length = data_word_len;
	task0.source[2].addr   = ((uint)p_src >> align_shift);
	task0.source[2].length = data_word_len;
	task0.data_len += task0.source[0].length;
	task0.data_len += task0.source[1].length;
	task0.data_len += task0.source[2].length;
	task0.data_len *= 4;

	task0.destination[0].addr   = ((uint)p_dst >> align_shift);
	task0.destination[0].length = data_word_len;
	task0.next_descriptor       = 0;

	flush_cache((u32)&task0, sizeof(task0));
	flush_cache((u32)p_n, mod_size_len_inbytes);
	flush_cache((u32)p_e, mod_size_len_inbytes);
	flush_cache((u32)p_src, mod_size_len_inbytes);
	flush_cache((u32)p_dst, mod_size_len_inbytes);

	ss_set_drq(((u32)&task0) >> align_shift);
	ss_irq_enable(task0.task_id);
	ss_ctrl_start(ASYM_TRPE);
	ss_wait_finish(task0.task_id);
	ss_pending_clear(task0.task_id);
	ss_ctrl_stop();
	ss_irq_disable(task0.task_id);
	if (ss_check_err(task0.task_id)) {
		printf("SS %s fail 0x%x\n", __func__,
			ss_check_err(task0.task_id));
		return -1;
	}

	invalidate_dcache_range((ulong)p_dst,
				(ulong)p_dst + mod_size_len_inbytes);
	__rsa_padding(dst_addr, p_dst, mod_bit_size / 64, mod_bit_size / 64);

	return 0;
}

int sunxi_aes_with_hardware(uint8_t *dst_addr, uint8_t *src_addr, int len,
			    uint8_t *key, uint32_t key_len,
			    uint32_t symmetric_ctl, uint32_t c_ctl)
{
	uint8_t *src_align;
	uint8_t *dst_align;
	uint8_t __aligned(64) iv[16]   = { 0 };
	task_queue task0 __aligned(64) = { 0 };
	uint32_t cts_size, destination_len;
	uint32_t align_shift = ss_get_addr_align();
	ALLOC_CACHE_ALIGN_BUFFER(uint8_t, key_map, 64);
	src_align = src_addr;
	dst_align = dst_addr;

	memset(key_map, 0, 64);
	if (key) {
		memcpy(key_map, key, key_len);
	}
	memset((void *)&task0, 0x00, sizeof(task_queue));

	cts_size	= ALIGN(len, 4) / 4;
	destination_len = cts_size;

	task0.task_id		    = 0;
	task0.common_ctl	    = (1U << 31) | c_ctl;
	task0.symmetric_ctl	 = symmetric_ctl;
	task0.key_descriptor	= (((uint32_t)(key_map)) >> align_shift);
	task0.iv_descriptor	 = ((uint32_t)iv >> align_shift);
	task0.data_len		    = len;
	task0.source[0].addr	= (((uint32_t)(src_align)) >> align_shift);
	task0.source[0].length      = cts_size;
	task0.destination[0].addr   = (((uint32_t)(dst_align)) >> align_shift);
	task0.destination[0].length = destination_len;
	task0.next_descriptor       = 0;

	//flush&clean cache
	flush_cache((u32)iv, ALIGN(sizeof(iv), CACHE_LINE_SIZE));
	flush_cache((u32)&task0, sizeof(task_queue));
	flush_cache((u32)key_map, ALIGN(sizeof(key_map), CACHE_LINE_SIZE));
	flush_cache((u32)src_align, ALIGN(len, CACHE_LINE_SIZE));
	flush_cache((u32)dst_align, ALIGN(len, CACHE_LINE_SIZE));

	ss_set_drq(((u32)&task0) >> align_shift);
	ss_irq_enable(CHANNEL_0);
	ss_ctrl_start(SYMM_TRPE);
	ss_wait_finish(CHANNEL_0);
	ss_pending_clear(CHANNEL_0);
	ss_ctrl_stop();
	ss_irq_disable(CHANNEL_0);
	ss_set_drq(0);
	if (ss_check_err(CHANNEL_0)) {
		printf("SS %s fail 0x%x\n", __func__, ss_check_err(CHANNEL_0));
		return -1;
	}

	invalidate_dcache_range((ulong)dst_align,
				((ulong)dst_align) + CACHE_LINE_SIZE);
	memcpy(dst_addr, dst_align, len);

	return 0;
}

int sunxi_trng_gen(u8 *rng_buf, u32 rng_byte)
{
	task_queue_other task0 __aligned(CACHE_LINE_SIZE) = { 0 };
	/* sha256  2word, sha512 4word*/
	ALLOC_CACHE_ALIGN_BUFFER(u8, p_sign, CACHE_LINE_SIZE);
	u32 align_shift = ss_get_addr_align();
	u32 md_size     = 32;
	memset(p_sign, 0, sizeof(p_sign));

	/* CE2.1 */

	task0.ctrl = (CHANNEL_0 << CHN) | (0x1 << LPKG) | (0x0 << DLAV) |
		     (0x1 << IE);
	task0.cmd		 = (0x2 << 8) | (0x3 << 0);
	task0.data_toal_len_addr = 0;
	task0.hmac_prng_key_addr = 0;
	task0.iv_addr		 = 0;

	task0.source[0].addr	= 0;
	task0.source[0].length      = 0;
	task0.destination[0].addr   = ((u32)p_sign >> align_shift);
	task0.destination[0].length = rng_byte;

	task0.next_descriptor_addr = 0;

	flush_cache(((u32)&task0), ALIGN(sizeof(task0), CACHE_LINE_SIZE));
	flush_cache((u32)p_sign, CACHE_LINE_SIZE);

	ss_set_drq(((u32)&task0) >> align_shift);
	ss_irq_enable(CHANNEL_0);
	ss_ctrl_start(HASH_RBG_TRPE);
	ss_wait_finish(CHANNEL_0);
	ss_pending_clear(CHANNEL_0);
	ss_ctrl_stop();
	ss_irq_disable(CHANNEL_0);
	ss_set_drq(0);
	if (ss_check_err(CHANNEL_0)) {
		printf("SS %s fail 0x%x\n", __func__, ss_check_err(CHANNEL_0));
		return -1;
	}

	invalidate_dcache_range((ulong)p_sign,
				((ulong)p_sign) + CACHE_LINE_SIZE);
	/*copy data*/
	memcpy(rng_buf, p_sign, md_size);
	return 0;
}

int sunxi_create_rssk(u8 *rssk_buf, u32 rssk_byte)
{
	return sunxi_trng_gen(rssk_buf, rssk_byte);
}
#if defined(SHA256_MULTISTEP_PACKAGE) || defined(SHA512_MULTISTEP_PACKAGE)
/**************************************************************************
*function():
*	sunxi_hash_init(): used for the first package data;
*	sunxi_hash_final(): used for the last package data;
*	sunxi_hash_update(): used for other package data;
*
* Note: these functions just used for CE2.0 in hash_alg
*
**************************************************************************/
int sunxi_sha_process(u8 *dst_addr, u32 dst_len, u8 *src_addr, u32 src_len,
		      int iv_mode, int last_flag, u32 total_len)
{
	u32 word_len				    = 0;
	u32 src_align_len			    = 0;
	u32 total_bit_len			    = 0;
	uint iv_addr				    = 0;
	u32 cur_bit_len				    = 0;
	int alg_hash				    = ALG_SHA256;
	task_queue task0 __aligned(CACHE_LINE_SIZE) = { 0 };
	/* sha256  2word, sha512 4word*/
	ALLOC_CACHE_ALIGN_BUFFER(u32, total_package_len,
				 CACHE_LINE_SIZE / sizeof(u32));
	ALLOC_CACHE_ALIGN_BUFFER(u8, p_sign, CACHE_LINE_SIZE);

	memset(p_sign, 0, sizeof(p_sign));

#ifdef SHA512_MULTISTEP_PACKAGE
	alg_hash = ALG_SHA512;
#endif

	if (iv_mode == 1) {
		iv_addr		    = (uint)dst_addr;
		task0.iv_descriptor = iv_addr;
		flush_cache((u32)iv_addr, dst_len);
	}

	/* CE2.0 */
	src_align_len = ALIGN(src_len, 4);

	if ((last_flag == 0) && (alg_hash == ALG_SHA256)) {
		cur_bit_len = (ALIGN(src_len, 64)) << 3;
	} else if ((last_flag == 0) && (alg_hash == ALG_SHA512)) {
		cur_bit_len = (ALIGN(src_len, 128)) << 3;
	} else {
		cur_bit_len = src_len << 3;
	}
	word_len	     = src_align_len >> 2;
	total_bit_len	= total_len << 3;
	total_package_len[0] = total_bit_len;
	total_package_len[1] = 0;

	task0.task_id = 0;
	task0.common_ctl =
		(alg_hash) | (last_flag << 15) | (iv_mode << 16) | (1U << 31);
	task0.key_descriptor = (u32)total_package_len; /* total_len in bits */
	task0.data_len       = cur_bit_len; /* cur_data_len in bits */

	task0.source[0].addr	= (uint)src_addr;
	task0.source[0].length      = word_len; /* cur_data_len in words */
	task0.destination[0].addr   = (uint)p_sign;
	task0.destination[0].length = dst_len >> 2;
	task0.next_descriptor       = 0;

	flush_cache((u32)&task0, sizeof(task0));
	flush_cache((u32)p_sign, CACHE_LINE_SIZE);
	flush_cache((u32)src_addr, src_align_len);
	flush_cache((u32)total_package_len, CACHE_LINE_SIZE);

	ss_set_drq((u32)&task0);
	ss_irq_enable(task0.task_id);
	ss_ctrl_start(alg_hash);
	ss_wait_finish(task0.task_id);
	ss_pending_clear(task0.task_id);
	ss_ctrl_stop();
	ss_irq_disable(task0.task_id);
	if (ss_check_err(0)) {
		printf("SS %s fail 0x%x\n", __func__, ss_check_err(0));
		return -1;
	}

	if (alg_hash == ALG_SHA512) {
		invalidate_dcache_range((ulong)p_sign,
					(ulong)p_sign + CACHE_LINE_SIZE * 2);
	} else {
		invalidate_dcache_range((ulong)p_sign,
					(ulong)p_sign + CACHE_LINE_SIZE);
	}
	/* copy data */
	memcpy(dst_addr, p_sign, dst_len);
	return 0;
}

int sunxi_hash_init(u8 *dst_addr, u8 *src_addr, u32 src_len, u32 total_len)
{
	u32 dst_len = 32;
	int ret     = 0;

#ifdef SHA512_MULTISTEP_PACKAGE
	dst_len = 64;
#endif
	ret = sunxi_sha_process(dst_addr, dst_len, src_addr, src_len, 0, 0,
				total_len);
	if (ret) {
		printf("sunxi hash init failed!\n");
		return -1;
	}
	return 0;
}

int sunxi_hash_update(u8 *dst_addr, u8 *src_addr, u32 src_len, u32 total_len)
{
	u32 dst_len = 32;
	int ret     = 0;

#ifdef SHA512_MULTISTEP_PACKAGE
	dst_len = 64;
#endif
	ret = sunxi_sha_process(dst_addr, dst_len, src_addr, src_len, 1, 0,
				total_len);
	if (ret) {
		printf("sunxi hash update failed!\n");
		return -1;
	}
	return 0;
}

int sunxi_hash_final(u8 *dst_addr, u8 *src_addr, u32 src_len, u32 total_len)
{
	u32 dst_len = 32;
	int ret     = 0;

#ifdef SHA512_MULTISTEP_PACKAGE
	dst_len = 64;
#endif
	ret = sunxi_sha_process(dst_addr, dst_len, src_addr, src_len, 1, 1,
				total_len);
	if (ret) {
		printf("sunxi hash final failed!\n");
		return -1;
	}
	return 0;
}
#endif

#ifdef SUNXI_HASH_TEST
int do_sha256_test(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	u8 hash[32] = { 0 };
	u32 x1      = simple_strtol(argv[1], NULL, 16);
	u32 x2      = simple_strtol(argv[2], NULL, 16);
	if (argc < 3) {
		return -1;
	}
	printf("src = 0x%x, len = 0x%x\n", x1, x2);

	tick_printf("sha256 test start 0\n");
	sunxi_ss_open();
	sunxi_sha_calc(hash, 32, (u8 *)x1, x2);
	tick_printf("sha256 test end\n");
	sunxi_dump(hash, 32);

	return 0;
}

U_BOOT_CMD(sha256_test, 3, 0, do_sha256_test,
	   "do a sha256 test, arg1: src address, arg2: len(hex)", "NULL");
#endif
