/*
 * (C) Copyright 2018-2020
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * wangwei <wangwei@allwinnertech.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <sunxi_image_header.h>

struct sunxi_image_verify_pattern_st {
	uint32_t size;
	uint32_t interval;
	int32_t cnt;
};

extern int sunxi_verify_rotpk_hash(void *input_hash_buf, int len);
extern int sunxi_verify_os(ulong os_load_addr, const char *cert_name);
extern int sunxi_verify_partion(struct sunxi_image_verify_pattern_st *pattern, const char *part_name, const char *cert_name, int full);
extern int sunxi_verify_preserve_toc1(void *toc1_head_buf);
extern int sunxi_verify_get_rotpk_hash(void *hash_buf);

#ifdef CONFIG_SUNXI_RTOS
#include <rtos_image.h>
extern int sunxi_verify_rtos(struct rtos_img_hdr *rtos_hdr);
#endif
extern int verify_image_by_vbmeta(const char *image_name,
				  const uint8_t *image_data, size_t image_len,
				  const uint8_t *vb_data, size_t vb_len,
				  const char *pubkey_in_toc1);

#ifdef CONFIG_SUNXI_VERIFY_DSP
extern int sunxi_verify_dsp(ulong img_addr, u32 img_len, u32 dsp_id);
#endif

int sunxi_verify_mips(void *buff, uint len, void *cert, unsigned cert_len);

#ifdef CONFIG_SUNXI_IMAGE_HEADER
int sunxi_image_verify(ulong src, const char *cert_name);
#endif

int sunxi_image_part_verify(sunxi_image_header_t *ih, uint8_t *payload, sunxi_tlv_header_t *tlv);
