/*
 * (C) Copyright 2018 allwinnertech  <wangwei@allwinnertech.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <securestorage.h>
#include <sunxi_board.h>
#include <asm/arch/ce.h>
#include <sunxi_board.h>
#include <asm/arch/timer.h>

int do_sha256_test(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	u8 hash[32] = { 0 };
	phys_addr_t x1      = simple_strtol(argv[1], NULL, 16);
	u32 x2      = simple_strtol(argv[2], NULL, 16);
	if (argc < 3) {
		return CMD_RET_USAGE;
	}
	if ((x1 & (CACHE_LINE_SIZE - 1)) != 0) {
		pr_err("start addr 0x%x not aligned with cache line size 0x%x\n",
		       x1, CACHE_LINE_SIZE);
		return CMD_RET_FAILURE;
	}
	if ((x2 & (CACHE_LINE_SIZE - 1)) != 0) {
		pr_err("len 0x%x not aligned with cache line size 0x%x\n", x2,
		       CACHE_LINE_SIZE);
		return CMD_RET_FAILURE;
	}
	tick_printf("src = 0x%x, len = 0x%x\n", x1, x2);

	tick_printf("sha256 test start 0\n");
	sunxi_ss_open();
	sunxi_sha_calc(hash, 32, (u8 *)(IOMEM_ADDR(x1)), x2);
	tick_printf("sha256 test end\n");
	tick_printf("src data\n");
	sunxi_dump((u8 *)(IOMEM_ADDR(x1)), x2);
	tick_printf("hash\n");
	sunxi_dump(hash, 32);

	return CMD_RET_SUCCESS;
}

s32 sunxi_normal_rsa(u8 *n_addr, u32 n_len, u8 *e_addr, u32 e_len, u8 *dst_addr,
		     u32 dst_len, u8 *src_addr, u32 src_len);
int do_rsa_test(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	/* 2048 bit */
	u8 rsa[256]  = { 0 };
	phys_addr_t n_addr   = simple_strtol(argv[1], NULL, 16);
	u32 n_len    = simple_strtol(argv[2], NULL, 16);
	phys_addr_t e_addr   = simple_strtol(argv[3], NULL, 16);
	u32 e_len    = simple_strtol(argv[4], NULL, 16);
	phys_addr_t src_addr = simple_strtol(argv[5], NULL, 16);
	u32 src_len  = simple_strtol(argv[6], NULL, 16);

	if ((n_len > 256) || (e_len > 256) || (src_len > 256)) {
		pr_err("len invalid, n:0x%x e:0x%x src:0x%x\n", n_len, e_len,
		       src_len);
	}
	tick_printf("rsa test start 0\n");
	sunxi_ss_open();
	sunxi_normal_rsa((u8 *)IOMEM_ADDR(n_addr), n_len, (u8 *)IOMEM_ADDR(e_addr), e_len, rsa, 256,
			 (u8 *)IOMEM_ADDR(src_addr), src_len);
	tick_printf("rsa test end\n");
	tick_printf("key n\n");
	sunxi_dump((u8 *)IOMEM_ADDR(n_addr), n_len);
	tick_printf("key e\n");
	sunxi_dump((u8 *)IOMEM_ADDR(e_addr), e_len);
	tick_printf("src data\n");
	sunxi_dump((u8 *)IOMEM_ADDR(src_addr), src_len);
	tick_printf("rsa\n");
	sunxi_dump(rsa, 256);
	return CMD_RET_SUCCESS;
}

int sunxi_trng_gen(u8 *rng_buf, u32 rng_byte);
int do_rng_test(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	phys_addr_t x1 = simple_strtol(argv[1], NULL, 16);
	u32 x2 = simple_strtol(argv[2], NULL, 16);

	if (x2 != 32) {
		pr_err("only 32 bytes len supported only\n");
		return CMD_RET_FAILURE;
	}
	tick_printf("rng test start 0\n");
	sunxi_ss_open();
	sunxi_trng_gen((u8 *)IOMEM_ADDR(x1), x2);
	tick_printf("rng test end\n");
	tick_printf("rng\n");
	sunxi_dump((u8 *)IOMEM_ADDR(x1), x2);

	return CMD_RET_SUCCESS;
}

#define SUNXI_AES_CBC_USERKEY_CFG                                              \
	(_SUNXI_AES_CFG | (SS_AES_KEY_128BIT << 0) | (SS_AES_MODE_CBC << 8) |  \
	 (SS_KEY_SELECT_INPUT << 20))
#define SSK_NAME	"ssk"
#define	HUK_NAME	"huk"
#define RSSK_NAME	"rssk"
int sunxi_aes_with_hardware(uint8_t *dst_addr, uint8_t *src_addr, int len,
			    uint8_t *key, uint32_t key_len,
			    uint32_t symmetric_ctl, uint8_t dir);
int do_aes_test(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	phys_addr_t src_addr = simple_strtol(argv[1], NULL, 16);
	u32 src_len  = simple_strtol(argv[2], NULL, 16);
	phys_addr_t enc_addr = simple_strtol(argv[3], NULL, 16);
	phys_addr_t dec_addr = simple_strtol(argv[4], NULL, 16);
	phys_addr_t key_addr = 0;
	u32 key_len = 32;
	u32 s_ctl, byte_algin = 0;
	s32 ret;

	if ((strcmp(argv[5], SSK_NAME) == 0) &&
				(strlen(argv[5]) == strlen(SSK_NAME))) {
		s_ctl = (_SUNXI_AES_CFG | (SS_AES_KEY_256BIT << 0) |
				(SS_AES_MODE_CBC << 8) | (SS_KEY_SELECT_SSK<< 20));
		key_len = 32;
	} else if ((strcmp(argv[5], HUK_NAME) == 0) &&
				(strlen(argv[5]) == strlen(HUK_NAME))) {
		s_ctl = (_SUNXI_AES_CFG | (SS_AES_KEY_192BIT << 0) |
				(SS_AES_MODE_CBC << 8) | (SS_KEY_SELECT_HUK<< 20));
		key_len = 24;
	} else if ((strcmp(argv[5], RSSK_NAME) == 0) &&
				(strlen(argv[5]) == strlen(RSSK_NAME))) {
		s_ctl = (_SUNXI_AES_CFG | (SS_AES_KEY_128BIT << 0) |
				(SS_AES_MODE_CBC << 8) | (SS_KEY_SELECT_RSSK<< 20));
		key_len = 16;
	} else {
		key_addr = simple_strtol(argv[5], NULL, 16);
		s_ctl = SUNXI_AES_CBC_USERKEY_CFG;
	}

	if (argc == 7) {
		byte_algin = simple_strtol(argv[6], NULL, 16);
	}

	if (!byte_algin) {
		if ((src_addr & (CACHE_LINE_SIZE - 1)) != 0) {
			pr_err("start addr 0x%x not aligned with cache line size 0x%x\n",
					src_addr, CACHE_LINE_SIZE);
			return CMD_RET_FAILURE;
		}

		if ((src_len & (CACHE_LINE_SIZE - 1)) != 0) {
			pr_err("len 0x%x not aligned with cache line size 0x%x\n",
					src_len, CACHE_LINE_SIZE);
			return CMD_RET_FAILURE;
		}

		if ((enc_addr & (CACHE_LINE_SIZE - 1)) != 0) {
			pr_err("enc addr 0x%x not aligned with cache line size 0x%x\n",
					enc_addr, CACHE_LINE_SIZE);
			return CMD_RET_FAILURE;
		}

		if ((dec_addr & (CACHE_LINE_SIZE - 1)) != 0) {
			pr_err("dec addr 0x%x not aligned with cache line size 0x%x\n",
					dec_addr, CACHE_LINE_SIZE);
			return CMD_RET_FAILURE;
		}
	}

	tick_printf("aes test start 0\n");
	sunxi_ss_open();
	sunxi_aes_with_hardware((u8 *)IOMEM_ADDR(enc_addr), (u8 *)IOMEM_ADDR(src_addr), src_len,
				(u8 *)IOMEM_ADDR(key_addr), key_len, s_ctl,
				SS_DIR_ENCRYPT);
	sunxi_aes_with_hardware((u8 *)IOMEM_ADDR(dec_addr), (u8 *)IOMEM_ADDR(enc_addr), src_len,
				(u8 *)IOMEM_ADDR(key_addr), key_len, s_ctl,
				SS_DIR_DECRYPT);

	ret = memcmp((u8 *)IOMEM_ADDR(src_addr), (u8 *)IOMEM_ADDR(dec_addr), src_len);
	if (ret != 0) {
		tick_printf("aes test fail\n");
		tick_printf("raw\n");
		sunxi_dump((u8 *)IOMEM_ADDR(src_addr), src_len);
		tick_printf("enc\n");
		sunxi_dump((u8 *)IOMEM_ADDR(enc_addr), src_len);
		tick_printf("dec\n");
		sunxi_dump((u8 *)IOMEM_ADDR(dec_addr), src_len);
	} else {
		tick_printf("aes test sucess\n");
	}

	return CMD_RET_SUCCESS;
}

#define AES_128_ECB_USERKEY_CFG                                              \
	(_SUNXI_AES_CFG | (SS_AES_KEY_128BIT << 0) | (SS_AES_MODE_ECB << 8) |  \
	 (SS_KEY_SELECT_INPUT << 20))

#define AES_256_ECB_USERKEY_CFG                                              \
	(_SUNXI_AES_CFG | (SS_AES_KEY_256BIT << 0) | (SS_AES_MODE_ECB << 8) |  \
	 (SS_KEY_SELECT_INPUT << 20))


int do_aes_perf_test(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	phys_addr_t src_addr = simple_strtol(argv[1], NULL, 16);
	u32 src_len  = simple_strtol(argv[2], NULL, 16);
	phys_addr_t enc_addr = simple_strtol(argv[3], NULL, 16);
	phys_addr_t dec_addr = simple_strtol(argv[4], NULL, 16);
	phys_addr_t key_addr = 0;
	u32 key_bit = 128;
	u32 key_len = 16;
	u32 s_ctl = AES_128_ECB_USERKEY_CFG;
	int i, ret;
	u64 start, end, tmp;

	key_addr = simple_strtol(argv[5], NULL, 16);
	key_bit = simple_strtol(argv[6], NULL, 10);
	if (key_bit == 128) {
		s_ctl = AES_128_ECB_USERKEY_CFG;
		key_len = key_bit >> 3;
	} else if (key_bit == 256) {
		s_ctl = AES_256_ECB_USERKEY_CFG;
		key_len = key_bit >> 3;
	}
	pr_err("key_len = %d\n", key_len);

	if ((src_addr & (CACHE_LINE_SIZE - 1)) != 0) {
		pr_err("start addr 0x%x not aligned with cache line size 0x%x\n",
		       src_addr, CACHE_LINE_SIZE);
		return CMD_RET_FAILURE;
	}

	if ((src_len & (CACHE_LINE_SIZE - 1)) != 0) {
		pr_err("len 0x%x not aligned with cache line size 0x%x\n",
		       src_len, CACHE_LINE_SIZE);
		return CMD_RET_FAILURE;
	}

	if ((enc_addr & (CACHE_LINE_SIZE - 1)) != 0) {
		pr_err("enc addr 0x%x not aligned with cache line size 0x%x\n",
		       enc_addr, CACHE_LINE_SIZE);
		return CMD_RET_FAILURE;
	}
	if ((dec_addr & (CACHE_LINE_SIZE - 1)) != 0) {
		pr_err("dec addr 0x%x not aligned with cache line size 0x%x\n",
		       dec_addr, CACHE_LINE_SIZE);
		return CMD_RET_FAILURE;
	}

	sunxi_ss_open();
	tick_printf("test 100 times start\n");
	start = read_timer();
	for (i = 0; i < 50; i++) {
		sunxi_aes_with_hardware((u8 *)IOMEM_ADDR(enc_addr),
				(u8 *)IOMEM_ADDR(src_addr), src_len,
				(u8 *)IOMEM_ADDR(key_addr), key_len, s_ctl, SS_DIR_ENCRYPT);
		sunxi_aes_with_hardware((u8 *)IOMEM_ADDR(dec_addr),
				(u8 *)IOMEM_ADDR(enc_addr), src_len,
				(u8 *)IOMEM_ADDR(key_addr), key_len, s_ctl,
				SS_DIR_DECRYPT);
	}

	end = read_timer();
	tmp = end - start;
	pr_err("100 times need counter:%lld\n", tmp);

	ret = memcmp((u8 *)IOMEM_ADDR(src_addr), (u8 *)IOMEM_ADDR(dec_addr), src_len);
	if (ret != 0) {
		tick_printf("aes test fail\n");
		tick_printf("raw\n");
		sunxi_dump((u8 *)IOMEM_ADDR(src_addr), src_len);
		tick_printf("enc\n");
		sunxi_dump((u8 *)IOMEM_ADDR(enc_addr), src_len);
		tick_printf("dec\n");
		sunxi_dump((u8 *)IOMEM_ADDR(dec_addr), src_len);
	} else {
		tick_printf("aes test sucess\n");
	}

	return CMD_RET_SUCCESS;
}

static cmd_tbl_t cmd_ce_test[] = {
	U_BOOT_CMD_MKENT(sha_256, 5, 0, do_sha256_test, "", ""),
	U_BOOT_CMD_MKENT(rsa, 10, 0, do_rsa_test, "", ""),
	U_BOOT_CMD_MKENT(rng, 5, 0, do_rng_test, "", ""),
	U_BOOT_CMD_MKENT(aes, 10, 0, do_aes_test, "", ""),
	U_BOOT_CMD_MKENT(aes_perf_test, 10, 0, do_aes_perf_test, "", ""),
};

int do_sunxi_ce_test(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	cmd_tbl_t *cp;

	cp = find_cmd_tbl(argv[1], cmd_ce_test, ARRAY_SIZE(cmd_ce_test));
	/* Drop the sunxi_ce_test command */
	argc--;
	argv++;

	if (cp)
		return cp->cmd(cmdtp, flag, argc, argv);
	else {
		pr_err("unknown sub command\n");
		return CMD_RET_USAGE;
	}
}

U_BOOT_CMD(sunxi_ce_test, CONFIG_SYS_MAXARGS, 0, do_sunxi_ce_test,
	   "sunxi ce test run", "NULL");
