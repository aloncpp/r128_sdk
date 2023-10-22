#ifndef __TFM_SUNXI_DEBUG_NSC_FUNCTION_H__
#define __TFM_SUNXI_DEBUG_NSC_FUNCTION_H__

void tfm_sunxi_hexdump(const uint32_t *addr, uint32_t num);

void tfm_sunxi_sau_info_printf(void);

uint32_t tfm_sunxi_readl(uint32_t reg);
void tfm_sunxi_writel(uint32_t reg, uint32_t value);

int tfm_sunxi_efuse_read(char *key_name, unsigned char *key_data, size_t key_bit_len);

#if 0
void tfm_sunxi_spc_test_begin(void);
void tfm_sunxi_spc_test_end(void);
#endif

#endif /* __TFM_SUNXI_DEBUG_NSC_FUNCTION_H__ */
