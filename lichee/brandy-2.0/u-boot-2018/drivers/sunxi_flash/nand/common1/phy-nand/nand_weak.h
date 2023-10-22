/*
 * SPDX-License-Identifier: GPL-2.0
 * nand_weak.c
 *
 * Copyright (C) 2019 Allwinner.
 * 2019.9.19 cuizhikui<cuizhikui@allwinnertech.com>
 */

#ifndef __NAND_WEAK_H__
#define __NAND_WEAK_H__

unsigned int __attribute__((weak)) spinand_nftl_get_super_page_size(int type);
unsigned int __attribute__((weak)) spinand_nftl_get_super_block_size(int type);
unsigned int __attribute__((weak)) spinand_nftl_get_single_page_size(int type);
unsigned int __attribute__((weak)) spinand_nftl_get_single_block_size(int type);
unsigned int __attribute__((weak)) spinand_nftl_get_chip_size(int type);
unsigned int __attribute__((weak)) spinand_nftl_get_die_size(int type);
unsigned int __attribute__((weak)) spinand_nftl_get_die_cnt(void);
unsigned int __attribute__((weak)) spinand_nftl_get_chip_cnt(void);
unsigned int __attribute__((weak)) spinand_nftl_get_max_erase_times(void);
unsigned int __attribute__((weak)) spinand_nftl_get_multi_plane_flag(void);
unsigned int __attribute__((weak)) spinand_nftl_get_operation_opt(void);
void __attribute__((weak)) spinand_nftl_get_chip_id(unsigned char *id);



int __attribute__((weak)) spinand_nftl_read_super_page(unsigned short dienum,
						       unsigned short blocknum, unsigned short pagenum,
						       unsigned short sectorbitmap, void *rmbuf, void *rspare);

int __attribute__((weak)) spinand_nftl_write_super_page(unsigned short dienum,
							unsigned short blocknum, unsigned short pagenum,
							unsigned short sectorbitmap, void *wmbuf, void *wspare);
int __attribute__((weak)) spinand_nftl_erase_super_block(unsigned short dienum,
							 unsigned short blocknum);
int __attribute__((weak)) spinand_nftl_super_badblock_check(unsigned short dienum,
							    unsigned short blocknum);
int __attribute__((weak)) spinand_nftl_super_badblock_mark(unsigned short dienum,
							   unsigned short blocknum);

int __attribute__((weak)) spinand_nftl_read_single_page(unsigned short dienum,
							unsigned short blocknum, unsigned short pagenum,
							unsigned short sectorbitmap, void *rmbuf, void *rspare);
int __attribute__((weak)) spinand_nftl_write_single_page(unsigned short dienum,
							 unsigned short blocknum, unsigned short pagenum,
							 unsigned short sectorbitmap, void *wmbuf, void *wspare);
int __attribute__((weak)) spinand_nftl_erase_single_block(unsigned short dienum,
							  unsigned short blocknum);
int __attribute__((weak)) spinand_nftl_single_block_copy(unsigned int from_chip,
							 unsigned int from_block, unsigned int to_chip, unsigned int to_block);
int __attribute__((weak)) spinand_nftl_single_badblock_check(unsigned short chipnum,
							     unsigned short blocknum);
int __attribute__((weak)) spinand_nftl_single_badblock_mark(unsigned short chipnum,
							    unsigned short blocknum);
struct _nand_info *__attribute__((weak)) spinand_hardware_init(void);
int __attribute__((weak)) spinand_hardware_exit(void);
unsigned int __attribute__((weak)) spinand_lsb_page_cnt_per_block(void);
unsigned int __attribute__((weak)) spinand_get_page_num(unsigned int lsb_page_no);
int __attribute__((weak)) spinand_write_boot0_one(unsigned char *buf,
						  unsigned int len, unsigned int counter);
int __attribute__((weak)) spinand_read_boot0_one(unsigned char *buf,
						 unsigned int len, unsigned int counter);
int __attribute__((weak)) spinand_read_uboot_one(unsigned char *buf,
						 unsigned int len, unsigned int counter);
int __attribute__((weak)) spinand_write_uboot_one(unsigned char *buf,
						  unsigned int len, struct _boot_info *info_buf,
						  unsigned int info_len, unsigned int counter);
int __attribute__((weak)) spinand_get_param(void *nand_param);
int __attribute__((weak)) spinand_get_param_for_uboottail(void *nand_param);
int __attribute__((weak)) SPINAND_UpdatePhyArch(void);
int __attribute__((weak)) spinand_erase_chip(unsigned int chip,
					     unsigned int start_block, unsigned int end_block,
					     unsigned int force_flag);
void __attribute__((weak)) spinand_erase_special_block(void);
int __attribute__((weak)) spinand_uboot_erase_all_chip(UINT32 force_flag);
int __attribute__((weak)) spinand_dragonborad_test_one(unsigned char *buf,
						       unsigned char *oob, unsigned int blk_num);
int __attribute__((weak)) spinand_physic_info_get_one_copy(unsigned int start_block,
							   unsigned int pages_offset, unsigned int *block_per_copy,
							   unsigned int *buf);
int __attribute__((weak)) spinand_add_len_to_uboot_tail(unsigned int uboot_size);
struct _nand_info *__attribute__((weak)) RawNandHwInit(void);
int __attribute__((weak)) RawNandHwExit(void);
int __attribute__((weak)) RawNandHwSuperStandby(void);
int __attribute__((weak)) RawNandHwSuperResume(void);
int __attribute__((weak)) RawNandHwNormalStandby(void);
int __attribute__((weak)) RawNandHwNormalResume(void);
int __attribute__((weak)) RawNandHwShutDown(void);
int __attribute__((weak)) rawnand_physic_erase_block(unsigned int chip, unsigned int block);
int __attribute__((weak)) rawnand_physic_read_page(unsigned int chip, unsigned int block, unsigned int page, unsigned int bitmap, unsigned char *mbuf, unsigned char *sbuf);
int __attribute__((weak)) rawnand_physic_write_page(unsigned int chip, unsigned int block, unsigned int page, unsigned int bitmap, unsigned char *mbuf, unsigned char *sbuf);
int __attribute__((weak)) rawnand_physic_bad_block_check(unsigned int chip, unsigned int block);
int __attribute__((weak)) rawnand_physic_bad_block_mark(unsigned int chip, unsigned int block);
int __attribute__((weak)) rawnand_physic_block_copy(unsigned int chip_s, unsigned int block_s, unsigned int chip_d, unsigned int block_d, unsigned int copy_nums);
int __attribute__((weak)) rawnand_physic_read_super_page(unsigned int chip, unsigned int block, unsigned int page, unsigned int bitmap, unsigned char *mbuf, unsigned char *sbuf);
int __attribute__((weak)) rawnand_physic_write_super_page(unsigned int chip, unsigned int block, unsigned int page, unsigned int bitmap, unsigned char *mbuf, unsigned char *sbuf);
int __attribute__((weak)) rawnand_physic_erase_super_block(unsigned int chip, unsigned int block);
int __attribute__((weak)) rawnand_physic_super_bad_block_check(unsigned int chip, unsigned int block);
int __attribute__((weak)) rawnand_physic_super_bad_block_mark(unsigned int chip, unsigned int block);
__u32 __attribute__((weak)) rawnand_get_lsb_block_size(void);
__u32 __attribute__((weak)) rawnand_get_lsb_pages(void);
__u32 __attribute__((weak)) rawnand_get_phy_block_size(void);
__u32 __attribute__((weak)) rawnand_used_lsb_pages(void);
u32 __attribute__((weak)) rawnand_get_pageno(u32 lsb_page_no);
__u32 __attribute__((weak)) rawnand_get_page_cnt_per_block(void);
__u32 __attribute__((weak)) rawnand_get_pae_size(void);
__u32 __attribute__((weak)) rawnand_get_twoplane_flag(void);
int __attribute__((weak)) rawnand_write_boot0_one(unsigned char *buf, unsigned int len, unsigned int counter);
int __attribute__((weak)) rawnand_read_boot0_one(unsigned char *buf, unsigned int len, unsigned int counter);
int __attribute__((weak)) rawnand_read_uboot_one(unsigned char *buf, unsigned int len, unsigned int counter);
int __attribute__((weak)) rawnand_write_uboot_one(unsigned char *buf, unsigned int len, struct _boot_info *info_buf, unsigned int info_len, unsigned int counter);
int __attribute__((weak)) rawnand_get_param(void *nand_param);
int __attribute__((weak)) rawnand_get_param_for_uboottail(void *nand_param);
int __attribute__((weak)) RAWNAND_UpdatePhyArch(void);
int __attribute__((weak)) rawnand_is_blank(void);
int __attribute__((weak)) rawnand_erase_chip(unsigned int chip, unsigned int start_block, unsigned int end_block, unsigned int force_flag);
void __attribute__((weak)) rawnand_erase_special_block(void);
int __attribute__((weak)) rawnand_uboot_erase_all_chip(UINT32 force_flag);
int __attribute__((weak)) rawnand_dragonborad_test_one(unsigned char *buf, unsigned char *oob,
		unsigned int blk_num);
int __attribute__((weak)) rawnand_physic_info_get_one_copy(unsigned int start_block, unsigned int pages_offset, unsigned int *block_per_copy, unsigned int *buf);
int __attribute__((weak)) rawnand_add_len_to_uboot_tail(unsigned int uboot_size);
int __attribute__((weak)) nand_wait_all_rb_ready(void);
void __attribute__((weak)) do_nand_interrupt(u32 no);
int __attribute__((weak)) nand_wait_rb_mode(void);
int __attribute__((weak)) nand_wait_dma_mode(void);
#endif /*NAND_WEAK_H*/
