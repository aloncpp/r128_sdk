/* SPDX-License-Identifier: GPL-2.0 */

#define _NCFB8_C_

#include "rawnand_debug.h"
#include <linux/string.h>
#include "../nand_boot.h"
#include "../nand_physic_interface.h"
#include "rawnand_chip.h"
#include "controller/ndfc_ops.h"
#include "controller/ndfc_timings.h"
/*#include "rawnand.h"*/
#include "rawnand_base.h"
#include "rawnand_ids.h"
#include "rawnand_readretry.h"
#include "../version.h"
#include "rawnand_boot.h"
#include "../../nand_osal_uboot.h"
#include <private_toc.h>
#include <sunxi_nand_boot.h>
#include <sunxi_nand_errno.h>

/*int small_nand_seed; //small nand random seed flag*/
static __u32 SECURE_FLAG;

extern __u32 rawnand_get_page_cnt_per_block(void);
extern __u32 rawnand_get_pae_size(void);
extern int nand_verify_toc0(unsigned char *buffer, unsigned int len);
extern int NAND_IS_Secure_Chip(void);
extern int NAND_IS_Burn_Mode(void);

#if defined(CONFIG_SUNXI_SLCNAND_OFFLINE_BURN_SECURE_FIRMWARE)
/*
* For small capacity slc nand offline burner. block0~block3: store secure boot0
* block4~block7: store normal boot0. Secure bit is not efused.
* Normal boot0 will run after power on, TA efuses secure bit.
* Secure boot0(toc0) will run when power on again. From then on,
* we need destory normal boot0, or we`d better burn secure boot0.
*/
int rawnand_replace_boot0_with_toc0(void)
{
	uint8 *toc0_buf = NULL;
	uint8 sbuf[16]  = { 0 };
	uint8 *mbuf     = NULL;
	int b = 0, p = 0;
	int toc0_blk    = 0,
	    end_toc_blk = aw_nand_info.boot->uboot_start_block >> 1;
	int check       = (NAND_IS_Secure_Chip() && (!NAND_IS_Burn_Mode()));
	int ret		= 0;
	int got_toc0    = 0;
	int chip	= 0;
	unsigned int page_cnt_per_blk = rawnand_get_page_cnt_per_block();
	unsigned int pagesize = rawnand_get_pae_size();
	unsigned int blocksize = pagesize * page_cnt_per_blk;
	toc0_private_head_t *toc0 = NULL;

#if 0
	printf("%s secure_bit: %d mode: %d storage_type: %d, check: %d\n",
	       __func__, NAND_IS_Secure_Chip(), NAND_IS_Burn_Mode(),
	       get_boot_storage_type_ext(), check);
	printf("sector_cnt: %d, page_cnt: %d\n", SECTOR_CNT_OF_SINGLE_PAGE,
	       PAGE_CNT_OF_PHY_BLK);
	check = 1;
#endif
	if (pagesize == 0 || blocksize == 0) {
		printf("%s pagesize:%u blocksize:%u err\n", __func__,
				pagesize, blocksize);
		return -1;
	}

	if (check) {
		mbuf = (uint8 *)NAND_Malloc(pagesize);
		if (!mbuf) {
			printf("%s malloc mbuf fail\n", __func__);
			ret = -1;
			goto fail_out;
		}
		toc0_buf = (uint8 *)NAND_Malloc(blocksize);
		if (!toc0_buf) {
			printf("%s malloc toc0_buf fail\n", __func__);
			ret = -1;
			goto fail_out;
		}
		for (b = end_toc_blk; b < aw_nand_info.boot->uboot_start_block; b++) {
			/* toc0 header is in page 0 of each block*/
			ret = nand_physic_read_page(chip, b, 0, pagesize >> 9, mbuf, sbuf);
			/* toc0 name is "TOC0.GLH".
			 * now platform before T507 toc0 size is not over 1 block capacity*/
			toc0 = (toc0_private_head_t *)mbuf;

			if (0 == strncmp((const char *)toc0->name, TOC0_MAGIC, MAGIC_SIZE)) {
				printf("b %d is valid\n", b);
				continue;
			} else {
				printf("b %d is invalid and erase it first\n", b);
				nand_physic_erase_block(chip, b);
				/* find a good toc0 copy first */
				if (0 == got_toc0) {
					for (toc0_blk = 0; toc0_blk < end_toc_blk; toc0_blk++) {
						for (p = 0; p < page_cnt_per_blk; p++) {
							ret = nand_physic_read_page(chip, toc0_blk, p,
									pagesize >> 9, toc0_buf + p * pagesize, sbuf);
							if (ret == ERR_ECC) {
								printf("%s %d %d fail\n", __func__, toc0_blk, p);
								break;
							}
						}
						if (p == page_cnt_per_blk) {
							if (0 == nand_verify_toc0(toc0_buf, blocksize)) {
								printf("%s %d ok\n", __func__, toc0_blk);
								break;
							}
						}
					}
					if (toc0_blk == end_toc_blk)
						printf("%s can`t find good toc0 copy, %d\n", __func__, end_toc_blk);
					got_toc0 = 1;
				}
				for (p = 0; p < page_cnt_per_blk; p++) {
					ret = nand_physic_write_page(chip, b, p, pagesize >> 9,
							toc0_buf + p * pagesize, sbuf);
					if (ret) {
						printf("%s W b %d p %d fail\n", __func__, b, p);
						break;
					}
				}
			}
		}
	}
fail_out:
	if (mbuf)
		NAND_Free(mbuf, pagesize);
	if (toc0_buf)
		NAND_Free(toc0_buf, blocksize);
	return ret;
}
#endif

u32 _cal_sum(u32 *mem_base, u32 size)
{
	u32 count, sum;
	u32 *buf;

	count = size >> 2;
	sum = 0;
	buf = (__u32 *)mem_base;
	do {
		sum += *buf++;
		sum += *buf++;
		sum += *buf++;
		sum += *buf++;
	} while ((count -= 4) > (4 - 1));

	while (count-- > 0)
		sum += *buf++;

	return sum;
}

__s32 check_sum(__u32 *mem_base, __u32 size)
{
	__u32 *buf;
	__u32 count;
	__u32 src_sum;
	__u32 sum;
	boot_file_head_t *bfh;

	bfh = (boot_file_head_t *)mem_base;

	/* generate check sum,
	 * get check_sum field from the head of boot0
	 */
	src_sum = bfh->check_sum;
	/*replace the check_sum field of the boot0 head with STAMP_VALUE*/
	bfh->check_sum = STAMP_VALUE;

	/*unit, 4byte*/
	count = size >> 2;
	sum = 0;
	buf = (__u32 *)mem_base;
	do {
		sum += *buf++; // calculate check sum
		sum += *buf++;
		sum += *buf++;
		sum += *buf++;
	} while ((count -= 4) > (4 - 1));

	while (count-- > 0)
		sum += *buf++;

	/*restore the check_sum field of the boot0 head*/
	bfh->check_sum = src_sum;

	//msg("sum:0x%x - src_sum:0x%x\n", sum, src_sum);
	if (sum == src_sum)
		return 0; // ok
	else
		return 1; // err
}

int _generate_page_map_tab(__u32 nand_page_size, __u32 copy_cnt, __u32 page_cnt,
			   __u32 *page_addr, __u32 *page_map_tab_addr, __u32 *tab_size)
{
	s32 i, j;
	u32 max_page_cnt;
	u32 checksum = 0;
	u8 *magic = (u8 *)NDFC_PAGE_TAB_MAGIC;
	u32 *pdst = (u32 *)page_map_tab_addr;
	boot_file_head_t *bfh = (boot_file_head_t *)page_map_tab_addr;
	u32 page_tab_size = 0;
	u32 nand_page_cnt;
	u32 c, p;

	if (copy_cnt == 1) {
		if (nand_page_size != 1024) {
			RAWNAND_ERR("_cal_page_map_tab, wrong @nand_page_size, %u\n", nand_page_size);
			return -1;
		}

		max_page_cnt = (1024 - NDFC_PAGE_TAB_HEAD_SIZE) / 4;
		if (page_cnt > max_page_cnt) {
			RAWNAND_ERR("_cal_page_map_tab, wrong @page_cnt, %u\n", page_cnt);
			return -1;
		}

		// clear to 0x00
		for (i = 0; i < 1024 / 4; i++)
			*(pdst + i) = 0x0;

		// set page address
		for (j = 0, i = NDFC_PAGE_TAB_HEAD_SIZE / 4; j < page_cnt; i++, j++)
			*(pdst + i) = page_addr[j];

		// set page table information
		bfh->platform[0] = page_cnt; //entry_cnt
		bfh->platform[1] = 1;	//entry_cell_cnt
		bfh->platform[2] = 4;	//entry_cell_size, byte
		bfh->platform[3] = nand_page_size / 512;

		// set magic
		//msg("page map table magic: ");
		for (i = 0; i < sizeof(bfh->magic); i++) {
			bfh->magic[i] = *(magic + i);
			//msg("%c", bfh->magic[i]);
		}
		//msg("\n");

		// set stamp value
		bfh->check_sum = STAMP_VALUE;

		// cal checksum
		checksum = _cal_sum((u32 *)page_map_tab_addr, 1024);
		bfh->check_sum = checksum;

		// check
		if (check_sum((__u32 *)page_map_tab_addr, 1024)) {
			RAWNAND_ERR("_cal_page_map_tab, checksum error!\n");
			return -1;
		}

		*tab_size = 1024;

	} else {

		page_tab_size = NDFC_PAGE_TAB_HEAD_SIZE + copy_cnt * page_cnt * 4;
		if (page_tab_size % nand_page_size)
			nand_page_cnt = page_tab_size / nand_page_size + 1;
		else
			nand_page_cnt = page_tab_size / nand_page_size;
		page_tab_size = nand_page_cnt * nand_page_size;

		/* clear page table memory spare */
		for (i = 0; i < page_tab_size / 4; i++)
			*(pdst + i) = 0x0;

		/* set header */
		bfh->length = page_tab_size;
		bfh->platform[0] = page_cnt; //entry_cnt
		bfh->platform[1] = copy_cnt; //entry_cell_cnt
		bfh->platform[2] = 4;	//entry_cell_size, byte
		bfh->platform[3] = nand_page_size / 512;

		/* fill page address */
		for (p = 0; p < page_cnt; p++) {
			for (c = 0; c < copy_cnt; c++) {
				i = NDFC_PAGE_TAB_HEAD_SIZE / 4 + p * copy_cnt + c;
				j = c * page_cnt + p; //j = c*(page_cnt+4) + p;
				*(pdst + i) = page_addr[j];
			}
		}

		/* set magic */
		//msg("page map table magic: ");
		for (i = 0; i < sizeof(bfh->magic); i++) {
			bfh->magic[i] = *(magic + i);
			//msg("%c", bfh->magic[i]);
		}
		//msg("\n");

		/* set stamp value */
		bfh->check_sum = STAMP_VALUE;

		/* cal checksum */
		checksum = _cal_sum((u32 *)page_map_tab_addr, page_tab_size);
		bfh->check_sum = checksum;
		//msg("bfh->check_sum: 0x%x\n", bfh->check_sum);

		/* check */
		if (check_sum((__u32 *)page_map_tab_addr, page_tab_size)) {
			RAWNAND_ERR("_cal_page_map_tab, checksum error!\n");
			return -1;
		}

		*tab_size = page_tab_size;
	}

	return 0;
}

int _get_read_retry_table(struct nand_chip_info *nci, u8 *rr_tab)
{
	//	boot_file_head_t rr_tab_head={0};
	u32 checksum = 0;
	//    u8 *magic = NDFC_RR_TAB_MAGIC;
	u8 *dummy_magic = (u8 *)NDFC_DMY_TAB_MAGIC;
	u8 *pdst = (u8 *)rr_tab;
	boot_file_head_t *bfh = (boot_file_head_t *)rr_tab;
	__u32 table_size = sizeof(boot_file_head_t);
	int i;
	//get read retry cmd data

	/* clear page table memory spare */
	for (i = 0; i < table_size; i++)
		*(pdst + i) = 0x0;

	/* set header */
	bfh->length = table_size;
	bfh->platform[3] = nci->sector_cnt_per_page;
	RAWNAND_DBG("length: 0x%x\n",
		    bfh->length);

	/* set magic */
	RAWNAND_DBG("rr table magic: ");
	{
		for (i = 0; i < sizeof(bfh->magic); i++) {
			bfh->magic[i] = *(dummy_magic + i);
			RAWNAND_DBG("%c", bfh->magic[i]);
		}
		RAWNAND_DBG("\n");
	}
	/* set stamp value */
	bfh->check_sum = STAMP_VALUE;

	/* cal checksum */
	checksum = _cal_sum((u32 *)rr_tab, table_size);
	bfh->check_sum = checksum;
	RAWNAND_DBG("bfh->check_sum: 0x%x\n", bfh->check_sum);

	/* check */
	if (check_sum((u32 *)rr_tab, table_size)) {
		RAWNAND_ERR("_cal_rr_tab, checksum error!\n");
		return -1;
	}
	return 0;
}
int generic_write_boot0_one_pagetab_4k(unsigned char *buf, unsigned int len, unsigned int counter)
{
	__u32 i, k, j, count, block;
	__u8  oob_buf[64];
	__u32 tab_size, data_size_per_page;
	__u32 pages_per_block, copies_per_block;
	__u32 page_addr;
	__u32 *pos_data = NULL, *tab = NULL, *rr_tab = NULL;
	//	__u8 *data_FF_buf = NULL;
	int ret;
	struct nand_chip_info *nci;
	struct _nand_physic_op_par lnpo;
	/*__u8 *oob_buf = NAND_Malloc(64);*/

	nci = g_nctri->nci;

#if 0
	/*A50 FPGA LDPC encode have bug, must to use SDR interface By zzm*/
	if (nci->support_toggle_only) {
		ret = before_update_nctri_interface(nci->nctri);
		if (ret) {
			RAWNAND_ERR("before_update nctri nand interface fail!\n");
			return ret;
		}
	} else {
		ret = update_boot0_nctri_interface(nci->nctri);
		if (ret) {
			RAWNAND_ERR("update boot0 nctri nand interface fail!\n");
			return ret;
		}
	}
#endif

	/*For A50, LDPC*/
	/*nci->nctri->channel_sel = 1;*/
	/*For R100, BCH*/
	//nci->nctri->channel_sel = 0;
	ndfc_encode_select(nci->nctri);

	RAWNAND_DBG("burn_boot0_lsb_pagetab_4k mode!\n");

	pos_data = (__u32 *)NAND_Malloc(128 * 4 * BOOT0_MAX_COPY_CNT);
	if (!pos_data) {
		RAWNAND_ERR("burn_boot0_lsb_FF_mode, malloc for pos_data failed.\n");
		goto error;
	}

	tab = (__u32 *)NAND_Malloc(8 * 1024);
	if (!tab) {
		RAWNAND_ERR("burn_boot0_lsb_FF_mode, malloc for tab failed.\n");
		goto error;
	}

	rr_tab = (__u32 *)NAND_Malloc(8 * 1024);
	if (!rr_tab) {
		RAWNAND_ERR(" malloc for rr tab failed.\n");
		goto error;
	}

	for (i = 0; i < 64; i++)
		oob_buf[i] = 0xff;

	/* get nand driver version */
	nand_get_version(oob_buf);
	if ((oob_buf[0] != 0xff) || (oob_buf[1] != 0x00)) {
		RAWNAND_ERR("get flash driver version error!");
		goto error;
	}

	data_size_per_page = 4096;
	pages_per_block = nci->page_cnt_per_blk;
	copies_per_block = pages_per_block / NAND_BOOT0_PAGE_CNT_PER_COPY;

	count = 0;
	for (i = NAND_BOOT0_BLK_START; i < (NAND_BOOT0_BLK_START + aw_nand_info.boot->uboot_start_block); i++) {
		for (j = 0; j < copies_per_block; j++) {
			for (k = 8; k < NAND_BOOT0_PAGE_CNT_PER_COPY; k++) {
				page_addr = i * pages_per_block + j * NAND_BOOT0_PAGE_CNT_PER_COPY + k;
				if (nci->is_lsb_page((page_addr % pages_per_block))) {
					*((__u32 *)pos_data + count) = page_addr;
					count++;
					if (((count % (len / data_size_per_page)) == 0) && (count != 0))
						break;
				}
			}
		}
	}

	_generate_page_map_tab(data_size_per_page,
			       copies_per_block * aw_nand_info.boot->uboot_start_block,
			       len / data_size_per_page, pos_data, tab, &tab_size);

	// get read retry table
	_get_read_retry_table(nci, (u8 *)rr_tab);

	block = NAND_BOOT0_BLK_START + counter;

	RAWNAND_DBG("pagetab boot0 %x \n", block);

	lnpo.chip = 0;
	lnpo.block = block;
	lnpo.page = 0;
	nand_wait_all_rb_ready();

	ret = nci->nand_physic_erase_block(&lnpo);
	if (ret) {
		RAWNAND_ERR("Fail in erasing block %d!\n", lnpo.block);
		//return ret;
	}

	for (j = 0; j < copies_per_block; j++) {
		count = 0;
		for (k = 0; k < NAND_BOOT0_PAGE_CNT_PER_COPY; k++) {
			lnpo.chip = 0;
			lnpo.block = block;
			lnpo.page = j * NAND_BOOT0_PAGE_CNT_PER_COPY + k;
			lnpo.sdata = oob_buf;
			lnpo.slen = 64;

			if (nci->is_lsb_page(lnpo.page)) {
				if (k < 4)
					lnpo.mdata = (__u8 *)rr_tab;
				else if (k < 8) {
					lnpo.mdata = (__u8 *)tab;
				} else {
					lnpo.mdata = (__u8 *)(buf + count * data_size_per_page);
					count++;
				}

				nand_wait_all_rb_ready();
				if (nci->nand_write_boot0_page(nci, &lnpo) != 0) {
					RAWNAND_ERR("Warning. Fail in writing page %d in block %d.\n", lnpo.page, lnpo.block);
				}
				if (count == (len + data_size_per_page - 1) / data_size_per_page) {
					count = 0;
				}
			}
		}
	}
	//nci->nctri->channel_sel = 0;

	ndfc_encode_default(nci->nctri);
	ndfc_channel_select(nci->nctri, 0);

	if (pos_data)
		NAND_Free(pos_data, 128 * 4 * BOOT0_MAX_COPY_CNT);
	if (tab)
		NAND_Free(tab, 8 * 1024);
	if (rr_tab)
		NAND_Free(rr_tab, 8 * 1024);
	//	if(data_FF_buf)
	//		NAND_Free(data_FF_buf,18048);
	return 0;

error:
	if (pos_data)
		NAND_Free(pos_data, 128 * 4 * BOOT0_MAX_COPY_CNT);
	if (tab)
		NAND_Free(tab, 8 * 1024);
	if (rr_tab)
		NAND_Free(rr_tab, 8 * 1024);
	//	if(data_FF_buf)
	//		NAND_Free(data_FF_buf,18048);
	return -1;
}
int generic_write_boot0_one_1k_mode(unsigned char *buf, unsigned int len, unsigned int counter)
{
	__u32 i, j;
	__u8  oob_buf[64];
	__u32 pages_per_block = 0, blocks_per_copy = 0, start_block, count = 0;
	int ret;
	struct nand_chip_info *nci;
	struct _nand_physic_op_par lnpo;

	/*__u8 *oob_buf = NAND_Malloc(64);*/
	nci = g_nctri->nci;
	//nci->nctri->channel_sel = 0;
	ndfc_encode_default(nci->nctri);

	RAWNAND_DBG("burn_boot0_1k mode!\n");

	for (i = 0; i < 64; i++)
		oob_buf[i] = 0xff;

	/* get nand driver version */
	nand_get_version(oob_buf);
	if ((oob_buf[0] != 0xff) || (oob_buf[1] != 0x00)) {
		RAWNAND_ERR("get flash driver version error!");
		goto error;
	}

	pages_per_block = nci->page_cnt_per_blk;

	blocks_per_copy = NAND_BOOT0_PAGE_CNT_PER_COPY / pages_per_block;
	if (NAND_BOOT0_PAGE_CNT_PER_COPY % pages_per_block)
		blocks_per_copy++;

	start_block = blocks_per_copy * counter;
	if ((start_block + blocks_per_copy) > aw_nand_info.boot->uboot_start_block) {
		return 0;
	}

	RAWNAND_DBG("boot0 count %d!\n", counter);

	count = 0;
	for (i = start_block; i < (start_block + blocks_per_copy); i++) {
		lnpo.chip = 0;
		lnpo.block = i;
		lnpo.page = 0;
		nand_wait_all_rb_ready();

		ret = nci->nand_physic_erase_block(&lnpo);
		if (ret) {
			RAWNAND_ERR("Fail in erasing block %d!\n", lnpo.block);
			//return ret;
		}

		for (j = 0; j < pages_per_block; j++) {
			lnpo.page = j;
			lnpo.sdata = oob_buf;
			lnpo.slen = 64;
			nci->nctri->random_factor = count;
			nci->nctri->random_factor |= SMALL_CAPACITY_NAND;

			lnpo.mdata = (__u8 *)(buf + 1024 * count);

			nand_wait_all_rb_ready();
			if (nci->nand_write_boot0_page(nci, &lnpo) != 0) {
				RAWNAND_ERR("Warning. Fail in writing page %d in block %d.\n", lnpo.page, lnpo.block);
			}
			count++;
			if (count == len / 1024) {
				count = 0;
			}
		}
	}
	nci->nctri->random_factor = 0;

	ndfc_encode_default(nci->nctri);
	ndfc_channel_select(nci->nctri, 0);

	return 0;

error:

	return -1;
}

int generic_write_boot0_one(unsigned char *buf, unsigned int len, unsigned int counter)
{
	struct nand_chip_info *nci;
	__u32 pagesize = 0;
	__u32 pagesperblock = 0;

	nci = g_nctri->nci;
	pagesize = nci->sector_cnt_per_page << 9;
	pagesperblock = nci->page_cnt_per_blk;

	RAWNAND_DBG("m0 write boot0 one \n");

	if ((pagesize >= 8192) && (pagesperblock >= 128))
		return generic_write_boot0_one_pagetab_4k(buf, len, counter);
	else
		return generic_write_boot0_one_1k_mode(buf, len, counter);

	return -1;
}

s32 generic_read_boot0_page_cfg_mode(struct nand_chip_info *nci, struct _nand_physic_op_par *npo, struct boot_ndfc_cfg cfg)
{
	struct _nctri_cmd_seq *cmd_seq = &nci->nctri->nctri_cmd_seq;
	u32 row_addr = 0, col_addr = 0, blk_mask, config = 0, i;
	u32 def_spare[32];
	s32 ecc_sta = 0, ret = 0;
	uchar spare[64];
	//u32 ecc_mode_temp,page_size_temp,page_size_set;
	u32 ecc_mode_temp, bitmap = 0;
	u32 ecc_block_valid, ecc_block_lose;
	unsigned char *buff = NULL;

	/*For A50, Buffer Must 32 Bytes align*/
	buff = NAND_Malloc(cfg.page_size_kb << 10);
	memcpy(buff, npo->mdata, (cfg.page_size_kb << 10));

	ecc_block_lose = get_data_block_cnt_for_boot0_ecccode(nci, cfg.ecc_mode);
	ecc_block_valid = nci->sector_cnt_per_page / 2 - ecc_block_lose;
	bitmap = ((u32)1 << (ecc_block_valid - 1)) | (((u32)1 << (ecc_block_valid - 1)) - 1);
	blk_mask = (0x0000FFFF >> (16 - (cfg.page_size_kb << 10) / 2048));

	//wait nand ready before read
	nand_read_chip_status_ready(nci);

	ecc_mode_temp = ndfc_get_ecc_mode(nci->nctri);

	nand_enable_chip(nci);
	ndfc_clean_cmd_seq(cmd_seq);

	//set ecc and randomizer
	if (nci->nctri->channel_sel == 0) {
		/*BCH*/
		ndfc_channel_select(nci->nctri, 0);
		ndfc_set_ecc_mode(nci->nctri, cfg.ecc_mode);
		ndfc_enable_ecc(nci->nctri, 1, 1);

		if (nci->nctri->random_factor & SMALL_CAPACITY_NAND) {
			ndfc_set_rand_seed(nci->nctri,
					(nci->nctri->random_factor & RANDOM_VALID_BITS));
		} else {
			ndfc_set_rand_seed(nci->nctri, npo->page);
		}

		ndfc_enable_randomize(nci->nctri);
	} else {
		/*LDPC*/
		ndfc_channel_select(nci->nctri, 1);
		ndfc_enable_decode(nci->nctri);
		ndfc_enable_ldpc_ecc(nci->nctri, 1);

		ndfc_enable_randomize(nci->nctri);
		ndfc_set_new_rand_seed(nci->nctri, npo->page);
	}

	//command
	set_default_batch_read_cmd_seq(cmd_seq);

	if (cfg.sequence_mode == 1)
		cmd_seq->ecc_layout = cfg.sequence_mode;

	//address
	row_addr = get_row_addr(nci->page_offset_for_next_blk, npo->block, npo->page);
	if (nci->npi->operation_opt & NAND_WITH_TWO_ROW_ADR) {
		cmd_seq->nctri_cmd[0].cmd_acnt = 4;
		fill_cmd_addr(col_addr, 2, row_addr, 2, cmd_seq->nctri_cmd[0].cmd_addr);
	} else {
		cmd_seq->nctri_cmd[0].cmd_acnt = 5;
		fill_cmd_addr(col_addr, 2, row_addr, 3, cmd_seq->nctri_cmd[0].cmd_addr);
	}

	nci->nctri->random_addr_num = nci->random_addr_num;

	//data
	cmd_seq->nctri_cmd[0].cmd_trans_data_nand_bus = 1;
	if (npo->mdata != NULL) {
		cmd_seq->nctri_cmd[0].cmd_swap_data = 1;
		cmd_seq->nctri_cmd[0].cmd_swap_data_dma = 1;
	} else {
		//don't swap main data with host memory
		cmd_seq->nctri_cmd[0].cmd_swap_data = 0;
		cmd_seq->nctri_cmd[0].cmd_swap_data_dma = 0;
	}
	cmd_seq->nctri_cmd[0].cmd_direction = 0; //read
	cmd_seq->nctri_cmd[0].cmd_mdata_addr = buff;
	if (nci->nctri->channel_sel == 0) {
		cmd_seq->nctri_cmd[0].cmd_data_block_mask = bitmap;
		cmd_seq->nctri_cmd[0].cmd_mdata_len = ecc_block_valid << 10;
	} else {
		cmd_seq->nctri_cmd[0].cmd_data_block_mask = blk_mask;
		cmd_seq->nctri_cmd[0].cmd_mdata_len = cfg.page_size_kb << 10;
	}

	//	RAWNAND_DBG("before send cmd: %d 0x%x 0x%x\n", nci->randomizer, *(nci->nctri->nreg.reg_ecc_ctl), *(nci->nctri->nreg.reg_ecc_sta));
	memset(def_spare, 0x99, 128);
	ndfc_set_spare_data(nci->nctri, (u8 *)def_spare, nci->sdata_bytes_per_page);

	ndfc_set_user_data_len_cfg_4bytesper1k(nci->nctri, nci->sdata_bytes_per_page);
	ndfc_set_user_data_len(nci->nctri);

	if (nci->nctri->channel_sel == 1) {
		for (i = 0; i < (cfg.page_size_kb / 2) * 4; i += 4) {
			config |= NDFC_DATA_LEN_DATA_BLOCK << i;
		}
		*nci->nctri->nreg.reg_user_data_len_base = config;
	}

	ret = batch_cmd_io_send(nci->nctri, cmd_seq);
	if (ret) {
		RAWNAND_ERR("m8 read page start, batch cmd io send error!\n");
		nand_disable_chip(nci);
		return ret;
	}
	ret = _batch_cmd_io_wait(nci->nctri, cmd_seq);
	if (ret) {
		RAWNAND_ERR("m8 read page end, batch cmd io wait error!\n");
		goto ERROR;
	}

	//check ecc
	ecc_sta = ndfc_check_ecc(nci->nctri, cfg.page_size_kb);
	//get spare data
	ndfc_get_spare_data(nci->nctri, (u8 *)spare, npo->slen);

	if (npo->slen != 0) {
		memcpy(npo->sdata, spare, npo->slen);
	}
	//don't update ecc status and spare data for boot operation
	//ret = ndfc_update_ecc_sta_and_spare_data(npo, ecc_sta, spare);
	ret = ecc_sta;

ERROR:

	ndfc_set_ecc_mode(nci->nctri, ecc_mode_temp);
	//*(nci->nctri->nreg.reg_ctl) = ((*(nci->nctri->nreg.reg_ctl)) & (~NDFC_PAGE_SIZE)) | ((page_size_temp & 0xf) << 8);
	//	ndfc_set_page_size(nci->nctri,page_size_temp);
	//disable ecc mode & randomize
	ndfc_disable_ecc(nci->nctri);

	ndfc_disable_randomize(nci->nctri);

	nand_disable_chip(nci);

	memcpy(npo->mdata, buff, (cfg.page_size_kb << 10));
	NAND_Free(buff, cfg.page_size_kb << 10);

	return ret; //ecc status
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       : 0:ok  -1:fail
 *Note         :
 *****************************************************************************/
#define NDFC_VERSION_1 (1)
#define NDFC_VERSION_2 (2)
int generic_read_boot0_page(struct nand_chip_info *nci, struct _nand_physic_op_par *npo)
{
	struct boot_ndfc_cfg cfg;
	//	struct _nand_physic_op_par lnpo = *npo;


	if (nci->nctri->channel_sel == 0) //BCH
		cfg.page_size_kb = ((nci->sector_cnt_per_page) / 2) - 1;
	else //LDPC
		cfg.page_size_kb = ((nci->sector_cnt_per_page) / 2) - 2;


	cfg.ecc_mode = nci->nctri->max_ecc_level;
	cfg.sequence_mode = 1;

	return generic_read_boot0_page_cfg_mode(nci, npo, cfg);
}

s32 generic_write_boot0_page_cfg_mode(struct nand_chip_info *nci,
					   struct _nand_physic_op_par *npo, struct boot_ndfc_cfg cfg)
{
	uchar spare[64];
	struct _nctri_cmd_seq *cmd_seq = &nci->nctri->nctri_cmd_seq;
	u32 row_addr = 0, col_addr = 0, blk_mask, config = 0, i;
	s32 ret = 0;
	u32 ecc_mode_temp = 0;
	u32 ecc_block_valid, ecc_block_lose, bitmap = 0;
	unsigned char *buff = NULL;

	if (npo->mdata == NULL) {
		RAWNAND_ERR("m8 write page start, input parameter error!\n");
		return ERR_NO_87;
	}

	/*For A50, Buffer Must 32 Bytes align*/
	buff = NAND_Malloc(cfg.page_size_kb << 10);
	memcpy(buff, npo->mdata, (cfg.page_size_kb << 10));

	ecc_block_lose = get_data_block_cnt_for_boot0_ecccode(nci, cfg.ecc_mode);
	ecc_block_valid = nci->sector_cnt_per_page / 2 - ecc_block_lose;
	bitmap = ((u32)1 << (ecc_block_valid - 1)) | (((u32)1 << (ecc_block_valid - 1)) - 1);
	blk_mask = (0x0000FFFF >> (16 - (cfg.page_size_kb << 10) / 2048));

	//wait nand ready before write
	nand_read_chip_status_ready(nci);

	//get spare data
	memset(spare, 0xff, 64);
	if (npo->slen != 0) {
		memcpy(spare, npo->sdata, npo->slen);
	}

	nand_enable_chip(nci);
	ndfc_clean_cmd_seq(cmd_seq);

	ndfc_set_spare_data(nci->nctri, (u8 *)spare, nci->sdata_bytes_per_page);

	ecc_mode_temp = ndfc_get_ecc_mode(nci->nctri);

	//set ecc and randomizer
	if (nci->nctri->channel_sel == 0) {
		/*BCH*/
		ndfc_enable_randomize(nci->nctri);

		if (nci->nctri->random_factor & SMALL_CAPACITY_NAND) {
			ndfc_set_rand_seed(nci->nctri,
					(nci->nctri->random_factor & RANDOM_VALID_BITS));
		} else {
			ndfc_set_rand_seed(nci->nctri, npo->page);
		}

		ndfc_channel_select(nci->nctri, 0);
		ndfc_set_ecc_mode(nci->nctri, cfg.ecc_mode);
		ndfc_enable_ecc(nci->nctri, 1, 1);
	} else {
		/*LDPC*/
		ndfc_enable_randomize(nci->nctri);
		ndfc_set_new_rand_seed(nci->nctri, npo->page);

		ndfc_channel_select(nci->nctri, 1);
		ndfc_enable_encode(nci->nctri);
		ndfc_enable_ldpc_ecc(nci->nctri, 1);
	}

	nci->nctri->current_op_type = 1;
	nci->nctri->random_addr_num = nci->random_addr_num;
	nci->nctri->random_cmd2_send_flag = nci->random_cmd2_send_flag;
	if (nci->random_cmd2_send_flag) {
		nci->nctri->random_cmd2 = get_random_cmd2(npo);
	}

	//command
	set_default_batch_write_cmd_seq(cmd_seq, CMD_WRITE_PAGE_CMD1, CMD_WRITE_PAGE_CMD2);

	if (cfg.sequence_mode == 1) {
		cmd_seq->ecc_layout = cfg.sequence_mode;
	}

	//address
	row_addr = get_row_addr(nci->page_offset_for_next_blk, npo->block, npo->page);
	if (nci->npi->operation_opt & NAND_WITH_TWO_ROW_ADR) {
		cmd_seq->nctri_cmd[0].cmd_acnt = 4;
		fill_cmd_addr(col_addr, 2, row_addr, 2, cmd_seq->nctri_cmd[0].cmd_addr);
	} else {
		cmd_seq->nctri_cmd[0].cmd_acnt = 5;
		fill_cmd_addr(col_addr, 2, row_addr, 3, cmd_seq->nctri_cmd[0].cmd_addr);
	}

	//data
	cmd_seq->nctri_cmd[0].cmd_trans_data_nand_bus = 1;
	cmd_seq->nctri_cmd[0].cmd_swap_data = 1;
	cmd_seq->nctri_cmd[0].cmd_swap_data_dma = 1;
	cmd_seq->nctri_cmd[0].cmd_direction = 1; //write
	cmd_seq->nctri_cmd[0].cmd_mdata_addr = buff;
	if (nci->nctri->channel_sel == 0) {
		cmd_seq->nctri_cmd[0].cmd_data_block_mask = bitmap;
		cmd_seq->nctri_cmd[0].cmd_mdata_len = (ecc_block_valid << 10);
	} else {
		cmd_seq->nctri_cmd[0].cmd_data_block_mask = blk_mask;
		cmd_seq->nctri_cmd[0].cmd_mdata_len = cfg.page_size_kb << 10;
	}

	ndfc_set_user_data_len_cfg_4bytesper1k(nci->nctri, nci->sdata_bytes_per_page);
	ndfc_set_user_data_len(nci->nctri);

	if (nci->nctri->channel_sel == 1) {
		for (i = 0; i < (cfg.page_size_kb / 2) * 4; i += 4) {
			config |= NDFC_DATA_LEN_DATA_BLOCK << i;
		}
		*nci->nctri->nreg.reg_user_data_len_base = config;
	}

	ret = batch_cmd_io_send(nci->nctri, cmd_seq);
	if (ret) {
		RAWNAND_ERR("m8 read page start, batch cmd io send error!\n");

		nand_disable_chip(nci);
		return ret;
	}

	ret = _batch_cmd_io_wait(nci->nctri, cmd_seq);
	if (ret) {
		RAWNAND_ERR("m8 read page end,  batch cmd io wait error!\n");
	}

	ndfc_set_ecc_mode(nci->nctri, ecc_mode_temp);
	ndfc_disable_ecc(nci->nctri);

	ndfc_disable_randomize(nci->nctri);

	nci->nctri->current_op_type = 0;
	nci->nctri->random_cmd2_send_flag = 0;

	nand_disable_chip(nci);

	NAND_Free(buff, cfg.page_size_kb << 10);

	return ret;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       : 0:ok  -1:fail
 *Note         :
 *****************************************************************************/
int generic_write_boot0_page(struct nand_chip_info *nci, struct _nand_physic_op_par *npo)
{
	struct boot_ndfc_cfg cfg;

	if (nci->nctri->channel_sel == 0) //BCH
		cfg.page_size_kb = ((nci->sector_cnt_per_page) / 2) - 1;
	else //LDPC
		cfg.page_size_kb = ((nci->sector_cnt_per_page) / 2) - 2;
	cfg.ecc_mode = nci->nctri->max_ecc_level;
	cfg.sequence_mode = 1;
	return generic_write_boot0_page_cfg_mode(nci, npo, cfg);
}

int generic_read_boot0_one_pagetab_4k(unsigned char *buf, unsigned int len, unsigned int counter)
{

	__u32 j, k, m, err_flag, count, block;
	__u8 oob_buf[64];
	__u32 data_size_per_page;
	__u32 pages_per_block, copies_per_block;
	unsigned char *ptr;
	struct _nand_physic_op_par lnpo;
	struct nand_chip_info *nci;

	nci = g_nctri->nci;
	/*For A50, LDPC*/
	/*nci->nctri->channel_sel = 1;*/
	/*For R100, BCH*/
	//	nci->nctri->channel_sel = 0;
	ndfc_encode_select(nci->nctri);

	data_size_per_page = 4096;
	pages_per_block = nci->page_cnt_per_blk;
	copies_per_block = pages_per_block / NAND_BOOT0_PAGE_CNT_PER_COPY;

	block = NAND_BOOT0_BLK_START + counter;
	ptr = nand_get_temp_buf(data_size_per_page);

	RAWNAND_DBG("read blk %x \n", block);
	for (j = 0; j < copies_per_block; j++) {
		err_flag = 0;
		count = 0;
		for (k = 8; k < NAND_BOOT0_PAGE_CNT_PER_COPY; k++) {
			lnpo.chip = 0;
			lnpo.block = block;
			lnpo.page = j * NAND_BOOT0_PAGE_CNT_PER_COPY + k;
			lnpo.sdata = oob_buf;
			lnpo.slen = 64;

			for (m = 0; m < 64; m++)
				oob_buf[m] = 0x55;

			if (nci->is_lsb_page(lnpo.page)) {
				lnpo.mdata = ptr;

				nand_wait_all_rb_ready();
				SECURE_FLAG = 1;
				if (nci->nand_read_boot0_page(nci, &lnpo) < 0) {
					RAWNAND_ERR("Warning. Fail in read page %d in block %d.\n", lnpo.page, lnpo.block);
					err_flag = 1;
					break;
				}
				if ((oob_buf[0] != 0xff) || (oob_buf[1] != 0x00)) {
					RAWNAND_ERR("get flash driver version error!");
					err_flag = 1;
					break;
				}
				memcpy(buf + count * data_size_per_page, ptr, data_size_per_page);

				count++;
				if (count == (len / data_size_per_page))
					break;
			}
		}
		if (err_flag == 0)
			break;
	}
	//	nci->nctri->channel_sel = 0;
	ndfc_encode_default(nci->nctri);
	ndfc_channel_select(nci->nctri, 0);

	nand_wait_all_rb_ready();
	nand_free_temp_buf(ptr, data_size_per_page);

	if (err_flag == 1)
		return -1;

	return 0;
}

int generic_read_boot0_one_1k_mode(unsigned char *buf, unsigned int len, unsigned int counter)
{
	__u32 i, j;
	__u8 oob_buf[64];
	__u32 pages_per_block, blocks_per_copy, start_block, count;
	__u32 flag;
	unsigned char *ptr = NULL;
	struct nand_chip_info *nci;
	struct _nand_physic_op_par lnpo;

	nci = g_nctri->nci;

	RAWNAND_DBG("read_boot0_1k mode!\n");

	for (i = 0; i < 64; i++)
		oob_buf[i] = 0xff;

	/* get nand driver version */
	nand_get_version(oob_buf);
	if ((oob_buf[0] != 0xff) || (oob_buf[1] != 0x00)) {
		RAWNAND_ERR("get flash driver version error!");
		goto error;
	}

	pages_per_block = nci->page_cnt_per_blk;

	blocks_per_copy = NAND_BOOT0_PAGE_CNT_PER_COPY_4 / pages_per_block;
	if (NAND_BOOT0_PAGE_CNT_PER_COPY_4 % pages_per_block)
		blocks_per_copy++;

	start_block = blocks_per_copy * counter;
	if ((start_block + blocks_per_copy) > aw_nand_info.boot->uboot_start_block) {
		return 0;
	}

	RAWNAND_DBG("boot0 count %d!\n", counter);

	ptr = nand_get_temp_buf(1024);
	if (ptr == NULL)
		RAWNAND_ERR("%s %d ptr get buf fail\n", __func__, __LINE__);

	count = 0;
	flag = 0;
	for (i = start_block; i < (start_block + blocks_per_copy); i++) {
		lnpo.chip = 0;
		lnpo.block = i;
		lnpo.page = 0;
		nand_wait_all_rb_ready();

		for (j = 0; j < pages_per_block; j++) {
			lnpo.page = j;
			lnpo.sdata = oob_buf;
			lnpo.slen = 64;
			lnpo.mdata = ptr;
			nci->nctri->random_factor = count;
			nci->nctri->random_factor |= SMALL_CAPACITY_NAND;

			nand_wait_all_rb_ready();
			if (nci->nand_read_boot0_page(nci, &lnpo) < 0) {
				RAWNAND_ERR("Warning. Fail in read page %d in block %d.\n", lnpo.page, lnpo.block);
				goto error;
			}
			if ((oob_buf[0] != 0xff) || (oob_buf[1] != 0x00)) {
				RAWNAND_ERR("get flash driver version error!");
				goto error;
			}

			memcpy(buf + count * 1024, ptr, 1024);

			count++;
			if (count == (len / 1024)) {
				flag = 1;
				break;
			}
		}
		if (flag == 1)
			break;
	}
	nand_free_temp_buf(ptr, 1024);
	nci->nctri->random_factor = 0;

	return 0;

error:
	nand_free_temp_buf(ptr, 1024);
	return -1;
}

int generic_read_boot0_one(unsigned char *buf, unsigned int len, unsigned int counter)
{
	struct nand_chip_info *nci;
	__u32 pagesize = 0;
	__u32 pagesperblock = 0;

	nci = g_nctri->nci;
	pagesize = nci->sector_cnt_per_page << 9;
	pagesperblock = nci->page_cnt_per_blk;

	RAWNAND_DBG("m0 read boot0 one \n");
	if ((pagesize >= 8192) && (pagesperblock >= 128))
		return generic_read_boot0_one_pagetab_4k(buf, len, counter);
	else
		return generic_read_boot0_one_1k_mode(buf, len, counter);

	return -1;
}

int hynix16nm_4G_write_boot0_one(unsigned char *buf, unsigned int len, unsigned int counter)
{
	__u32 i, k, j, count, block;
	__u8 oob_buf[64];
	__u32 tab_size, data_size_per_page;
	__u32 pages_per_block, copies_per_block;
	__u32 page_addr;
	__u32 *pos_data = NULL, *tab = NULL, *rr_tab = NULL;
	__u8 *data_FF_buf = NULL;
	int ret;
	//	struct boot_ndfc_cfg cfg;
	struct nand_chip_info *nci;
	struct _nand_physic_op_par lnpo;

	nci = g_nctri->nci;

#if 1
	/*A50 FPGA LDPC encode have bug, must to use SDR interface By zzm*/
	if (nci->support_toggle_only) {
		ret = before_update_nctri_interface(nci->nctri);
		if (ret) {
			RAWNAND_ERR("before_update nctri nand interface fail!\n");
			return ret;
		}
	} else {
		ret = update_boot0_nctri_interface(nci->nctri);
		if (ret) {
			RAWNAND_ERR("update boot0 nctri nand interface fail!\n");
			return ret;
		}
	}
#endif

	/*For A50, LDPC*/
	/*nci->nctri->channel_sel = 1;*/
	/*For R100, BCH*/
	//nci->nctri->channel_sel = 0;
	ndfc_encode_select(nci->nctri);

	RAWNAND_DBG("burn_boot0_lsb_FF_pagetab secure mode!\n");

	pos_data = (__u32 *)NAND_Malloc(128 * 4 * BOOT0_MAX_COPY_CNT);
	if (!pos_data) {
		RAWNAND_ERR("burn_boot0_lsb_FF_mode, malloc for pos_data failed.\n");
		goto error;
	}

	tab = (__u32 *)NAND_Malloc(8 * 1024);
	if (!tab) {
		RAWNAND_ERR("burn_boot0_lsb_FF_mode, malloc for tab failed.\n");
		goto error;
	}

	rr_tab = (__u32 *)NAND_Malloc(8 * 1024);
	if (!rr_tab) {
		RAWNAND_ERR("burn_boot0_lsb_FF_mode, malloc for rr_tab failed.\n");
		goto error;
	}

	data_FF_buf = NAND_Malloc(20 * 1024);
	if (data_FF_buf == NULL) {
		RAWNAND_ERR("data_FF_buf malloc error!");
		goto error;
	}

	for (i = 0; i < (16384 + 1664); i++)
		*((__u8 *)data_FF_buf + i) = 0xFF;

	for (i = 0; i < 64; i++)
		oob_buf[i] = 0xff;

	/* get nand driver version */
	nand_get_version(oob_buf);
	if ((oob_buf[0] != 0xff) || (oob_buf[1] != 0x00)) {
		RAWNAND_ERR("get flash driver version error!");
		goto error;
	}

	data_size_per_page = 4096;
	pages_per_block = nci->page_cnt_per_blk;
	copies_per_block = pages_per_block / NAND_BOOT0_PAGE_CNT_PER_COPY;

	count = 0;
	for (i = NAND_BOOT0_BLK_START; i < (NAND_BOOT0_BLK_START + aw_nand_info.boot->uboot_start_block); i++) {
		for (j = 0; j < copies_per_block; j++) {
			for (k = 8; k < NAND_BOOT0_PAGE_CNT_PER_COPY; k++) {
				page_addr = i * pages_per_block + j * NAND_BOOT0_PAGE_CNT_PER_COPY + k;
				if (nci->is_lsb_page((page_addr % pages_per_block))) {
					*((__u32 *)pos_data + count) = page_addr;
					count++;
					if (((count % (len / data_size_per_page)) == 0) && (count != 0))
						break;
				}
			}
		}
	}

	_generate_page_map_tab(data_size_per_page, copies_per_block * aw_nand_info.boot->uboot_start_block, len / data_size_per_page, pos_data, tab, &tab_size);

	// get read retry table
	_get_read_retry_table(nci, (u8 *)rr_tab);

	block = NAND_BOOT0_BLK_START + counter;

	RAWNAND_DBG("pagetab boot0 %x \n", block);

	lnpo.chip = 0;
	lnpo.block = block;
	lnpo.page = 0;

	nand_wait_all_rb_ready();

	ret = nci->nand_physic_erase_block(&lnpo);
	if (ret) {
		RAWNAND_ERR("Fail in erasing block %d!\n", lnpo.block);
		//return ret;
	}

	for (j = 0; j < copies_per_block; j++) {
		count = 0;
		for (k = 0; k < NAND_BOOT0_PAGE_CNT_PER_COPY; k++) {
			lnpo.chip = 0;
			lnpo.block = block;
			lnpo.page = j * NAND_BOOT0_PAGE_CNT_PER_COPY + k;
			lnpo.sdata = oob_buf;
			lnpo.slen = 64;

			if (nci->is_lsb_page(lnpo.page)) {
				if (k < 4)
					lnpo.mdata = (__u8 *)rr_tab;
				else if (k < 8)
					lnpo.mdata = (__u8 *)tab;
				else {
					if (count < len / data_size_per_page)
						lnpo.mdata = (__u8 *)(buf + count * data_size_per_page);
					else
						lnpo.mdata = (__u8 *)buf;
					count++;
				}

				nand_wait_all_rb_ready();
				if (nci->nand_write_boot0_page(nci, &lnpo) != 0) {
					RAWNAND_ERR("Warning. Fail in writing page %d in block %d.\n", lnpo.page, lnpo.block);
				}
			} else {
				lnpo.mdata = (__u8 *)data_FF_buf;
				nand_wait_all_rb_ready();
				if (hynix16nm_write_page_FF(&lnpo, (nci->sector_cnt_per_page / 2)) != 0) {
					RAWNAND_ERR("Warning. Fail in writing page %d in block %d.\n", lnpo.page, lnpo.block);
				}
			}
		}
	}

	//nci->nctri->channel_sel = 0;

	ndfc_encode_default(nci->nctri);
	ndfc_channel_select(nci->nctri, 0);

	if (pos_data)
		NAND_Free(pos_data, 128 * 4 * BOOT0_MAX_COPY_CNT);
	if (tab)
		NAND_Free(tab, 8 * 1024);
	if (rr_tab)
		NAND_Free(rr_tab, 8 * 1024);
	if (data_FF_buf)
		NAND_Free(data_FF_buf, 18048);
	return 0;

error:
	if (pos_data)
		NAND_Free(pos_data, 128 * 4 * BOOT0_MAX_COPY_CNT);
	if (tab)
		NAND_Free(tab, 8 * 1024);
	if (rr_tab)
		NAND_Free(rr_tab, 8 * 1024);
	if (data_FF_buf)
		NAND_Free(data_FF_buf, 18048);
	return -1;
}

/**
 * nand flash controller different, write boot0 way is different
 */
/*
 *struct boot0_df_cfg_mode boot0_cfg_df = {
 *#if defined(CONFIG_ARCH_SUN8IW18)
 *    .generic_read_boot0_page_cfg_mode = generic_read_boot0_page_cfg_mode,
 *    .generic_write_boot0_page_cfg_mode = generic_write_boot0_page_cfg_mode,
 *#else [>CONFIG_ARCH_SUN50IW9<]
 *    .generic_read_boot0_page_cfg_mode = generic_read_boot0_page_cfg_mode_v1px,
 *    .generic_write_boot0_page_cfg_mode = generic_write_boot0_page_cfg_mode_v1px,
 *#endif
 *};
 */

int hynix20nm_write_boot0_one(unsigned char *buf, unsigned int len, unsigned int counter)
{
	__u32 i, k, j, count, block, chip_no;
	__u8 oob_buf[64];
	__u32 tab_size, data_size_per_page;
	__u32 pages_per_block, copies_per_block;
	__u32 page_addr;
	__u32 *pos_data = NULL, *tab = NULL, *rr_tab = NULL;
	//	__u8 *data_FF_buf=NULL;
	int ret;
	//	struct boot_ndfc_cfg cfg;
	struct nand_chip_info *nci;
	struct _nand_physic_op_par lnpo;

	nci = g_nctri->nci;

#if 0
	/*A50 FPGA LDPC encode have bug, must to use SDR interface By zzm*/
	if (nci->support_toggle_only) {
		ret = before_update_nctri_interface(nci->nctri);
		if (ret) {
			RAWNAND_ERR("before_update nctri nand interface fail!\n");
			return ret;
		}
	} else {
		ret = update_boot0_nctri_interface(nci->nctri);
		if (ret) {
			RAWNAND_ERR("update boot0 nctri nand interface fail!\n");
			return ret;
		}
	}
#endif

	/*For A50, LDPC*/
	/*nci->nctri->channel_sel = 1;*/
	/*For R100, BCH*/
	//nci->nctri->channel_sel = 0;
	ndfc_encode_select(nci->nctri);

	RAWNAND_DBG("burn_boot0_lsb_enable_pagetab mode!\n");

	pos_data = (__u32 *)NAND_Malloc(128 * 4 * BOOT0_MAX_COPY_CNT);
	if (!pos_data) {
		RAWNAND_ERR("burn_boot0_lsb_FF_mode, malloc for pos_data failed.\n");
		goto error;
	}

	tab = (__u32 *)NAND_Malloc(8 * 1024);
	if (!tab) {
		RAWNAND_ERR("burn_boot0_lsb_FF_mode, malloc for tab failed.\n");
		goto error;
	}

	rr_tab = (__u32 *)NAND_Malloc(8 * 1024);
	if (!rr_tab) {
		RAWNAND_ERR("burn_boot0_lsb_FF_mode, malloc for rr_tab failed.\n");
		goto error;
	}

#if 0
	data_FF_buf = NAND_Malloc(18048);
	if (data_FF_buf == NULL) {
		NAND_Malloc("data_FF_buf malloc error!");
		goto error;
	}

	for (i = 0; i < (16384 + 1664); i++)
		*((__u8 *)data_FF_buf + i) = 0xFF;
#endif

	for (i = 0; i < 64; i++)
		oob_buf[i] = 0xff;

	/* get nand driver version */
	nand_get_version(oob_buf);
	if ((oob_buf[0] != 0xff) || (oob_buf[1] != 0x00)) {
		RAWNAND_ERR("get flash driver version error!");
		goto error;
	}

	// lsb enable
	RAWNAND_DBG("lsb enalbe \n");
	RAWNAND_DBG("read retry mode: 0x%x\n", hynix20nm_read_retry_mode);

	chip_no = nci->nctri_chip_no;
	nci->nctri_chip_no = 0;
	hynix20nm_lsb_init(nci);
	hynix20nm_lsb_enable(nci);
	nci->nctri_chip_no = chip_no;

	data_size_per_page = 4096;
	pages_per_block = nci->page_cnt_per_blk;
	copies_per_block = pages_per_block / NAND_BOOT0_PAGE_CNT_PER_COPY;

	count = 0;
	for (i = NAND_BOOT0_BLK_START; i < (NAND_BOOT0_BLK_START + aw_nand_info.boot->uboot_start_block); i++) {
		for (j = 0; j < copies_per_block; j++) {
			for (k = 8; k < NAND_BOOT0_PAGE_CNT_PER_COPY; k++) {
				page_addr = i * pages_per_block + j * NAND_BOOT0_PAGE_CNT_PER_COPY + k;
				if (nci->is_lsb_page((page_addr % pages_per_block))) {
					*((__u32 *)pos_data + count) = page_addr;
					count++;
					if (((count % (len / data_size_per_page)) == 0) && (count != 0))
						break;
				}
			}
		}
	}

	_generate_page_map_tab(data_size_per_page, copies_per_block * aw_nand_info.boot->uboot_start_block, len / data_size_per_page, pos_data, tab, &tab_size);

	// get read retry table
	_get_read_retry_table(nci, (u8 *)rr_tab);

	block = NAND_BOOT0_BLK_START + counter;

	RAWNAND_DBG("pagetab boot0 %x \n", block);

	lnpo.chip = 0;
	lnpo.block = block;
	lnpo.page = 0;
	nand_wait_all_rb_ready();

	ret = nci->nand_physic_erase_block(&lnpo);
	if (ret) {
		RAWNAND_ERR("Fail in erasing block %d!\n", lnpo.block);
		//return ret;
	}

	for (j = 0; j < copies_per_block; j++) {
		count = 0;
		for (k = 0; k < NAND_BOOT0_PAGE_CNT_PER_COPY; k++) {
			lnpo.chip = 0;
			lnpo.block = block;
			lnpo.page = j * NAND_BOOT0_PAGE_CNT_PER_COPY + k;
			lnpo.sdata = oob_buf;
			lnpo.slen = 64;

			if (nci->is_lsb_page(lnpo.page)) {
				if (k < 4)
					lnpo.mdata = (__u8 *)rr_tab;
				else if (k < 8)
					lnpo.mdata = (__u8 *)tab;
				else {
					if (count < len / data_size_per_page)
						lnpo.mdata = (__u8 *)(buf + count * data_size_per_page);
					else
						lnpo.mdata = (__u8 *)buf;
					count++;
				}

				nand_wait_all_rb_ready();
				if (nci->nand_write_boot0_page(nci, &lnpo) != 0) {
					RAWNAND_ERR("Warning. Fail in writing page %d in block %d.\n", lnpo.page, lnpo.block);
				}
			}
		}
	}

	chip_no = nci->nctri_chip_no;
	nci->nctri_chip_no = 0;
	hynix20nm_lsb_disable(nci);
	hynix20nm_lsb_exit(nci);
	nci->nctri_chip_no = chip_no;

	RAWNAND_DBG("lsb disalbe \n");

	//	nci->nctri->channel_sel = 0;
	ndfc_encode_default(nci->nctri);
	ndfc_channel_select(nci->nctri, 0);

	if (pos_data)
		NAND_Free(pos_data, 128 * 4 * BOOT0_MAX_COPY_CNT);
	if (tab)
		NAND_Free(tab, 8 * 1024);
	if (rr_tab)
		NAND_Free(rr_tab, 8 * 1024);
	//	if(data_FF_buf)
	//		NAND_Free(data_FF_buf,18048);
	return 0;

error:
	chip_no = nci->nctri_chip_no;
	nci->nctri_chip_no = 0;
	hynix20nm_lsb_disable(nci);
	hynix20nm_lsb_exit(nci);
	nci->nctri_chip_no = chip_no;

	RAWNAND_DBG("lsb disalbe \n");

	if (pos_data)
		NAND_Free(pos_data, 128 * 4 * BOOT0_MAX_COPY_CNT);
	if (tab)
		NAND_Free(tab, 8 * 1024);
	if (rr_tab)
		NAND_Free(rr_tab, 8 * 1024);
	//	if(data_FF_buf)
	//		NAND_Free(data_FF_buf,18048);
	return -1;
}

int hynix26nm_write_boot0_one(unsigned char *buf, unsigned int len, unsigned int counter)
{
	__u32 i, k, j, count, block, chip_no;
	__u8 oob_buf[64];
	__u32 tab_size, data_size_per_page;
	__u32 pages_per_block, copies_per_block;
	__u32 page_addr;
	__u32 *pos_data = NULL, *tab = NULL, *rr_tab = NULL;
	//	__u8 *data_FF_buf=NULL;
	int ret;
	//	struct boot_ndfc_cfg cfg;
	struct nand_chip_info *nci;
	struct _nand_physic_op_par lnpo;

	nci = g_nctri->nci;

#if 0
	/*A50 FPGA LDPC encode have bug, must to use SDR interface By zzm*/
	if (nci->support_toggle_only) {
		ret = before_update_nctri_interface(nci->nctri);
		if (ret) {
			RAWNAND_ERR("before_update nctri nand interface fail!\n");
			return ret;
		}
	} else {
		ret = update_boot0_nctri_interface(nci->nctri);
		if (ret) {
			RAWNAND_ERR("update boot0 nctri nand interface fail!\n");
			return ret;
		}
	}
#endif

	/*For A50, LDPC*/
	/*nci->nctri->channel_sel = 1;*/
	/*For R100, BCH*/
	//nci->nctri->channel_sel = 0;
	ndfc_encode_select(nci->nctri);

	RAWNAND_DBG("burn_boot0_lsb_enable_pagetab mode!\n");

	pos_data = (__u32 *)NAND_Malloc(128 * 4 * BOOT0_MAX_COPY_CNT);
	if (!pos_data) {
		RAWNAND_ERR("burn_boot0_lsb_FF_mode, malloc for pos_data failed.\n");
		goto error;
	}

	tab = (__u32 *)NAND_Malloc(8 * 1024);
	if (!tab) {
		RAWNAND_ERR("burn_boot0_lsb_FF_mode, malloc for tab failed.\n");
		goto error;
	}

	rr_tab = (__u32 *)NAND_Malloc(8 * 1024);
	if (!rr_tab) {
		RAWNAND_ERR("burn_boot0_lsb_FF_mode, malloc for rr_tab failed.\n");
		goto error;
	}

#if 0
	data_FF_buf = NAND_Malloc(18048);
	if (data_FF_buf == NULL) {
		NAND_Malloc("data_FF_buf malloc error!");
		goto error;
	}

	for (i = 0; i < (16384 + 1664); i++)
		*((__u8 *)data_FF_buf + i) = 0xFF;
#endif

	for (i = 0; i < 64; i++)
		oob_buf[i] = 0xff;

	/* get nand driver version */
	nand_get_version(oob_buf);
	if ((oob_buf[0] != 0xff) || (oob_buf[1] != 0x00)) {
		RAWNAND_ERR("get flash driver version error!");
		goto error;
	}

	// lsb enable
	RAWNAND_DBG("lsb enalbe \n");
	RAWNAND_DBG("read retry mode: 0x%x\n", hynix20nm_read_retry_mode);

	chip_no = nci->nctri_chip_no;
	nci->nctri_chip_no = 0;
	hynix26nm_lsb_init(nci);
	hynix26nm_lsb_enable(nci);
	nci->nctri_chip_no = chip_no;

	data_size_per_page = 4096;
	pages_per_block = nci->page_cnt_per_blk;
	copies_per_block = pages_per_block / NAND_BOOT0_PAGE_CNT_PER_COPY;

	count = 0;
	for (i = NAND_BOOT0_BLK_START; i < (NAND_BOOT0_BLK_START + aw_nand_info.boot->uboot_start_block); i++) {
		for (j = 0; j < copies_per_block; j++) {
			for (k = 8; k < NAND_BOOT0_PAGE_CNT_PER_COPY; k++) {
				page_addr = i * pages_per_block + j * NAND_BOOT0_PAGE_CNT_PER_COPY + k;
				if (nci->is_lsb_page((page_addr % pages_per_block))) {
					*((__u32 *)pos_data + count) = page_addr;
					count++;
					if (((count % (len / data_size_per_page)) == 0) && (count != 0))
						break;
				}
			}
		}
	}

	_generate_page_map_tab(data_size_per_page, copies_per_block * aw_nand_info.boot->uboot_start_block, len / data_size_per_page, pos_data, tab, &tab_size);

	// get read retry table
	_get_read_retry_table(nci, (u8 *)rr_tab);

	block = NAND_BOOT0_BLK_START + counter;

	RAWNAND_DBG("pagetab boot0 %x \n", block);

	lnpo.chip = 0;
	lnpo.block = block;
	lnpo.page = 0;

	nand_wait_all_rb_ready();

	ret = nci->nand_physic_erase_block(&lnpo);
	if (ret) {
		RAWNAND_ERR("Fail in erasing block %d!\n", lnpo.block);
		//return ret;
	}

	for (j = 0; j < copies_per_block; j++) {
		count = 0;
		for (k = 0; k < NAND_BOOT0_PAGE_CNT_PER_COPY; k++) {
			lnpo.chip = 0;
			lnpo.block = block;
			lnpo.page = j * NAND_BOOT0_PAGE_CNT_PER_COPY + k;
			lnpo.sdata = oob_buf;
			lnpo.slen = 64;

			if (nci->is_lsb_page(lnpo.page)) {
				if (k < 4)
					lnpo.mdata = (__u8 *)rr_tab;
				else if (k < 8)
					lnpo.mdata = (__u8 *)tab;
				else {
					if (count < len / data_size_per_page)
						lnpo.mdata = (__u8 *)(buf + count * data_size_per_page);
					else
						lnpo.mdata = (__u8 *)buf;
					count++;
				}

				nand_wait_all_rb_ready();
				if (nci->nand_write_boot0_page(nci, &lnpo) != 0) {
					RAWNAND_ERR("Warning. Fail in writing page %d in block %d.\n", lnpo.page, lnpo.block);
				}
			}
		}
	}

	chip_no = nci->nctri_chip_no;
	nci->nctri_chip_no = 0;
	hynix26nm_lsb_disable(nci);
	hynix26nm_lsb_exit(nci);
	nci->nctri_chip_no = chip_no;

	RAWNAND_DBG("lsb disalbe \n");

	//nci->nctri->channel_sel = 0;
	ndfc_encode_default(nci->nctri);
	ndfc_channel_select(nci->nctri, 0);

	if (pos_data)
		NAND_Free(pos_data, 128 * 4 * BOOT0_MAX_COPY_CNT);
	if (tab)
		NAND_Free(tab, 8 * 1024);
	if (rr_tab)
		NAND_Free(rr_tab, 8 * 1024);
	//	if(data_FF_buf)
	//		NAND_Free(data_FF_buf,18048);
	return 0;

error:
	chip_no = nci->nctri_chip_no;
	nci->nctri_chip_no = 0;
	hynix26nm_lsb_disable(nci);
	hynix26nm_lsb_exit(nci);
	nci->nctri_chip_no = chip_no;

	RAWNAND_DBG("lsb disalbe \n");

	if (pos_data)
		NAND_Free(pos_data, 128 * 4 * BOOT0_MAX_COPY_CNT);
	if (tab)
		NAND_Free(tab, 8 * 1024);
	if (rr_tab)
		NAND_Free(rr_tab, 8 * 1024);
	//	if(data_FF_buf)
	//		NAND_Free(data_FF_buf,18048);
	return -1;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       : 0:ok  -1:fail
 *Note         :
 *****************************************************************************/
int rawnand_write_boot0_one(unsigned char *buf, unsigned int len, unsigned int counter)
{
	int ret;

	ret = g_nsi->nci->nand_write_boot0_one(buf, len, counter);

	return ret;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       : 0:ok  -1:fail
 *Note         :
 *****************************************************************************/
int rawnand_read_boot0_one(unsigned char *buf, unsigned int len, unsigned int counter)
{
	int ret;

	ret = g_nsi->nci->nand_read_boot0_one(buf, len, counter);

	return ret;
}
/**
 * different platform boot0 size is different ,so one copy size is different
 * */
/*
 *struct boot0_copy_df boot0_copy_df = {
 *#if defined(CONFIG_ARCH_SUN8IW18)
 *    .ndfc_blks_per_boot0_copy = ndfc_blks_per_boot0_copy_v2px,
 *#else
 *    .ndfc_blks_per_boot0_copy = ndfc_blks_per_boot0_copy_v1px,
 *#endif
 *};
 */

void selected_write_boot0_one(enum rq_write_boot0_type rq)
{
	switch (rq) {
	case NAND_WRITE_BOOT0_GENERIC:
		rawnand_boot0_ops.write_boot0_one = generic_write_boot0_one;
		break;
	case NAND_WRITE_BOOT0_HYNIX_16NM_4G:
		rawnand_boot0_ops.write_boot0_one = hynix16nm_4G_write_boot0_one;
		break;
	case NAND_WRITE_BOOT0_HYNIX_20NM:
		rawnand_boot0_ops.write_boot0_one = hynix20nm_write_boot0_one;
		break;
	case NAND_WRITE_BOOT0_HYNIX_26NM:
		rawnand_boot0_ops.write_boot0_one = hynix26nm_write_boot0_one;
		break;
	default:
		rawnand_boot0_ops.write_boot0_one = generic_write_boot0_one;
		break;
	};

}

write_boot0_one_t write_boot0_one[] = {
	generic_write_boot0_one,
	hynix16nm_4G_write_boot0_one,
	hynix20nm_write_boot0_one,
	hynix26nm_write_boot0_one,
};

struct rawnand_boot0_ops_t rawnand_boot0_ops = {
    .write_boot0_page = generic_write_boot0_page,
    .read_boot0_page = generic_read_boot0_page,
    .write_boot0_one = NULL,
    .read_boot0_one = generic_read_boot0_one,
};
