//SPDX-License-Identifier:	GPL-2.0+
/*
 ************************************************************************************************************************
 *                                                      eNand
 *                                           Nand flash driver scan module
 *
 *                             Copyright(C), 2008-2009, SoftWinners Microelectronic Co., Ltd.
 *                                                  All Rights Reserved
 *
 * File Name : version.c
 *
 * Author :
 *
 * Version : v0.1
 *
 * Date : 2013-11-20
 *
 * Description :
 *
 * Others : None at present.
 *
 *
 *
 ************************************************************************************************************************
 */
#define _NVER_C_

#include "nand_physic_inc.h"

#define NAND_VERSION_0 0x03
#define NAND_VERSION_1 0x01

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       : 0:ok  -1:fail
 *Note         :
 *****************************************************************************/
int nand_code_info(void)
{
	NAND_Print_Version();

	return 0;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
 *****************************************************************************/
__u32 nand_get_nand_version(void)
{
	__u32 nand_version;

	nand_version = 0;
	nand_version |= 0xff;
	nand_version |= 0x00 << 8;
	nand_version |= NAND_VERSION_0 << 16;
	nand_version |= NAND_VERSION_1 << 24;

	return nand_version;
}

int nand_get_version(__u8 *nand_version)
{
	__u32 version;
	version = nand_get_nand_version();

	nand_version[0] = (u8)version;
	nand_version[1] = (u8)(version >> 8);
	nand_version[2] = (u8)(version >> 16);
	nand_version[3] = (u8)(version >> 24);

	return version;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
 *****************************************************************************/
void show_nctri(struct _nand_controller_info *nctri)
{
	int i;

	if (nctri == NULL) {
		PHY_DBG(" ====invalid nctri=====\n");
		return;
	}

	PHY_DBG("==============show_nctri======================\n");
	PHY_DBG("channel_id:            %d\n", nctri->channel_id);
	PHY_DBG("type:                  %d\n", nctri->type);
	PHY_DBG("chip_cnt:              %d\n", nctri->chip_cnt);
	PHY_DBG("chip_connect_info:     %d\n", nctri->chip_connect_info);
	PHY_DBG("rb_connect_info:       %d\n", nctri->rb_connect_info);

	PHY_DBG("ce: ");
	for (i = 0; i < nctri->chip_cnt; i++)
		PHY_DBG("%d ", nctri->ce[i]);
	PHY_DBG("\nrb: ");
	for (i = 0; i < nctri->chip_cnt; i++)
		PHY_DBG("%d ", nctri->rb[i]);
	PHY_DBG("\n");

	PHY_DBG("dma_type:              %d\n", nctri->dma_type);
	PHY_DBG("dma_addr:              %d\n", nctri->dma_addr);
	PHY_DBG("write_wait_rb_before:  %d\n",
			nctri->write_wait_rb_before_cmd_io);
	PHY_DBG("write_wait_rb_mode:    %d\n", nctri->write_wait_rb_mode);
	PHY_DBG("rb_ready_flag:         %d\n", nctri->rb_ready_flag);
	PHY_DBG("dma_ready_flag:        %d\n", nctri->dma_ready_flag);

	PHY_DBG("timing ctl: ");
	for (i = 0; i < nctri->chip_cnt; i++)
		PHY_DBG("0x%x ", nctri->ddr_timing_ctl[i]);
	PHY_DBG("\n");

	ndfc_print_reg(nctri);

	PHY_DBG("==============show_nctri end======================\n");

	return;
}
/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
 *****************************************************************************/
void show_nci(struct _nand_chip_info *nci)
{
	int i;

	if (nci == NULL) {
		PHY_DBG(" invalid nci\n");
		return;
	}

	PHY_DBG("==============show_nci======================\n");
	PHY_DBG("id: ");
	for (i = 0; i < 8; i++)
		PHY_DBG("%02x ", nci->id[i]);
	PHY_DBG("\n");
	PHY_DBG("chip_no:                   %d\n", nci->chip_no);
	PHY_DBG("nctri_chip_no:             %d\n", nci->nctri_chip_no);
	PHY_DBG("driver_no:                 %d\n", nci->driver_no);
	PHY_DBG("blk_cnt_per_chip:          %d\n", nci->blk_cnt_per_chip);
	PHY_DBG("sector_cnt_per_page:       %d\n", nci->sector_cnt_per_page);
	PHY_DBG("page_cnt_per_blk:          %d\n", nci->page_cnt_per_blk);
	PHY_DBG("page_offset_for_next_blk:  %d\n",
			nci->page_offset_for_next_blk);
	PHY_DBG("randomizer:                %d\n", nci->randomizer);
	PHY_DBG("read_retry:                %d\n", nci->read_retry);
	PHY_DBG("interface_type:            %d\n", nci->interface_type);
	PHY_DBG("timing_mode:               %d\n", nci->timing_mode);
	PHY_DBG("support_change_onfi_timing_mode:  %d\n",
			nci->support_change_onfi_timing_mode);
	PHY_DBG("support_ddr2_specific_cfg:        %d\n",
			nci->support_ddr2_specific_cfg);
	PHY_DBG("support_io_driver_strength:       %d\n",
			nci->support_io_driver_strength);
	PHY_DBG("support_vendor_specific_cfg:      %d\n",
			nci->support_vendor_specific_cfg);
	PHY_DBG("support_onfi_sync_reset:   %d\n",
			nci->support_onfi_sync_reset);
	PHY_DBG("support_toggle_only:       %d\n", nci->support_toggle_only);
	PHY_DBG("frequency:                 %d\n", nci->frequency);
	PHY_DBG("ecc_mode:                  %d\n", nci->ecc_mode);
	PHY_DBG("max_erase_times:           %d\n", nci->max_erase_times);
	PHY_DBG("page_addr_bytes:           %d\n", nci->page_addr_bytes);
	PHY_DBG("sdata_bytes_per_page:      %d\n", nci->sdata_bytes_per_page);
	PHY_DBG("nsci chip_no:              0x%08x\n", nci->nsci->chip_no);
	PHY_DBG("nctri channel_id:          0x%08x\n", nci->nctri->channel_id);
	PHY_DBG("multi_plane_read_cmd0:     0x%08x\n",
			nci->opt_phy_op_par->multi_plane_read_cmd[0]);
	PHY_DBG("multi_plane_read_cmd1:     0x%08x\n",
			nci->opt_phy_op_par->multi_plane_read_cmd[1]);
	PHY_DBG("multi_plane_write_cmd0:    0x%08x\n",
			nci->opt_phy_op_par->multi_plane_write_cmd[0]);
	PHY_DBG("multi_plane_write_cmd1:    0x%08x\n",
			nci->opt_phy_op_par->multi_plane_write_cmd[1]);
	PHY_DBG("bad_block_flag_position:   0x%08x\n",
			nci->opt_phy_op_par->bad_block_flag_position);
	PHY_DBG("multi_plane_block_offset:  0x%08x\n",
			nci->opt_phy_op_par->multi_plane_block_offset);
	PHY_DBG("id_number:                 0x%08x\n", nci->npi->id_number);

	return;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
 *****************************************************************************/
void show_nsci(struct _nand_super_chip_info *nsci)
{
	if (nsci == NULL) {
		PHY_DBG(" invalid nsci\n");
		return;
	}
	PHY_DBG("=================show_nsci===================\n");
	PHY_DBG("chip_no:                       %d\n", nsci->chip_no);
	PHY_DBG("blk_cnt_per_super_chip:        %d\n",
			nsci->blk_cnt_per_super_chip);
	PHY_DBG("sector_cnt_per_super_page:     %d\n",
			nsci->sector_cnt_per_super_page);
	PHY_DBG("page_cnt_per_super_blk:        %d\n",
			nsci->page_cnt_per_super_blk);
	PHY_DBG("page_offset_for_next_super_blk:%d\n",
			nsci->page_offset_for_next_super_blk);
	PHY_DBG("spare_bytes:                   %d\n", nsci->spare_bytes);
	PHY_DBG("two_plane:                     %d\n", nsci->two_plane);
	PHY_DBG("channel_num:                   %d\n", nsci->channel_num);
	PHY_DBG("vertical_interleave:           %d\n",
			nsci->vertical_interleave);
	PHY_DBG("dual_channel:                  %d\n", nsci->dual_channel);
	PHY_DBG("driver_no:                     %d\n", nsci->driver_no);
	PHY_DBG("nci_first:                     %d %d\n",
			nsci->nci_first->chip_no, nsci->nci_first->nctri_chip_no);
	if (nsci->vertical_interleave != 0) {
		PHY_DBG("v_intl_nci_1:              %d %d\n",
				nsci->v_intl_nci_1->chip_no,
				nsci->v_intl_nci_1->nctri_chip_no);
		PHY_DBG("v_intl_nci_2:              %d %d\n",
				nsci->v_intl_nci_2->chip_no,
				nsci->v_intl_nci_2->nctri_chip_no);
	}
	if (nsci->dual_channel != 0) {
		PHY_DBG("d_channel_nci_1:              %d %d\n",
				nsci->d_channel_nci_1->chip_no,
				nsci->d_channel_nci_1->nctri_chip_no);
		PHY_DBG("d_channel_nci_2:              %d %d\n",
				nsci->d_channel_nci_2->chip_no,
				nsci->d_channel_nci_2->nctri_chip_no);
	}
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
 *****************************************************************************/
void show_nsi(void)
{
	struct _nand_storage_info *nsi = g_nsi;
	PHY_DBG("================show_nsi====================\n");
	PHY_DBG("chip_cnt:      %d\n", nsi->chip_cnt);
	PHY_DBG("block_nums:    %d\n", nsi->block_nums);
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
 *****************************************************************************/
void show_nssi(void)
{
	struct _nand_super_storage_info *nssi = g_nssi;
	struct _nand_super_chip_info *nsci;

	PHY_DBG("================show_nssi====================\n");
	PHY_DBG("super_chip_cnt:      %d\n", nssi->super_chip_cnt);
	PHY_DBG("super_block_nums:    %d\n", nssi->super_block_nums);
	PHY_DBG("support_two_plane:   %d\n", nssi->support_two_plane);
	PHY_DBG("support_v_interleave:%d\n", nssi->support_v_interleave);
	PHY_DBG("support_dual_channel:%d\n", nssi->support_dual_channel);

	nsci = nssi->nsci;
	while (nsci != NULL) {
		show_nsci(nsci);
		nsci = nsci->nssi_next;
	}
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
 *****************************************************************************/
void show_static_info(void)
{
	struct _nand_controller_info *nctri;
	struct _nand_chip_info *nci;

	nctri = g_nctri;
	while (nctri != NULL) {
		show_nctri(nctri);
		nci = nctri->nci;
		while (nci != NULL) {
			show_nci(nci);
			nci = nci->nctri_next;
		}
		nctri = nctri->next;
	}
	show_nsi();
	show_nssi();
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
 *****************************************************************************/
void show_spare(int flag)
{
	unsigned char spare[64];
	int i, j, ret;
	unsigned char *mbuf;

	mbuf = nand_get_temp_buf(64 * 1024);

	for (i = NAND_BOOT0_BLK_START; i < NAND_BOOT0_BLK_CNT; i++) {
		;
	}

	PHY_ERR("==========================uboot========================================\n");
	for (i = NAND_UBOOT_BLK_START; i < NAND_UBOOT_BLK_CNT; i++) {
		for (j = 0; j < g_nsi->nci->page_cnt_per_blk; j++) {
			ret = nand_physic_read_page(
					0, i, j, g_nsi->nci->sector_cnt_per_page, mbuf,
					spare);
			if ((ret != 0) && (ret != ECC_LIMIT)) {
				PHY_ERR("ecc error chip0 block :%d page :0x%x \n",
						i, j);
			}
			PHY_ERR("block:%d page:%d spare:0x%2x  0x%2x%2x%2x%2x   0x%2x%2x  0x%2x%2x%2x%2x\n",
					i, j, spare[0], spare[1], spare[2], spare[3],
					spare[4], spare[5], spare[6], spare[7],
					spare[8], spare[9], spare[10]);
		}
	}

	PHY_ERR("==========================all page 0========================================\n");
	for (i = 8; i < g_nssi->nsci->blk_cnt_per_super_chip; i++) {
		ret = nand_physic_read_super_page(0, i, 0, 0, NULL, spare);
		if ((ret != 0) && (ret != ECC_LIMIT)) {
			PHY_ERR("ecc error chip0 block :%d page :0 \n", i);
		}
		PHY_ERR("block:%d page:0 spare:0x%2x    0x%2x 0x%2x 0x%2x 0x%2x   0x%2x%2x  0x%2x%2x%2x%2x\n",
				i, spare[0], spare[1], spare[2], spare[3], spare[4],
				spare[5], spare[6], spare[7], spare[8], spare[9],
				spare[10]);
	}

	if (flag == 0) {
		nand_free_temp_buf(mbuf, 64 * 1024);
		return;
	}

	PHY_ERR("==========================all page========================================\n");
	for (i = 8; i < g_nssi->nsci->blk_cnt_per_super_chip; i++) {
		if (nand_physic_super_bad_block_check(0, i) != 0) {
			continue;
		}

		nand_physic_read_super_page(0, i, 0, 0, NULL, spare);

		PHY_ERR("block:%d page:0 spare:0x%2x   0x%2x 0x%2x 0x%2x 0x%2x  0x%x%x  0x%x%x%x%x\n",
				i, spare[0], spare[1], spare[2], spare[3], spare[4],
				spare[5], spare[6], spare[7], spare[8], spare[9],
				spare[10]);

		if ((spare[0] == 0xff) && (spare[1] == 0xff) &&
				(spare[2] == 0xff) && (spare[3] == 0xff) &&
				(spare[4] == 0xff) && (spare[5] == 0xff)) {
			continue;
		}

		for (j = 1; j < g_nssi->nsci->page_cnt_per_super_blk; j++) {
			ret = nand_physic_read_super_page(0, i, j, 0, NULL,
					spare);
			if ((ret != 0) && (ret != ECC_LIMIT)) {
				PHY_ERR("ecc error chip0 block :%d page :0x%x \n",
						i, j);
			}
			PHY_ERR("block:%d page:%d spare:0x%2x   0x%2x 0x%2x 0x%2x 0x%2x  0x%x%x  0x%x%x%x%x\n",
					i, j, spare[0], spare[1], spare[2], spare[3],
					spare[4], spare[5], spare[6], spare[7],
					spare[8], spare[9], spare[10]);
		}
	}

	nand_free_temp_buf(mbuf, 64 * 1024);
	return;
}
/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
 *****************************************************************************/
void nand_show_data(uchar *buf, u32 len)
{
	u32 i;
	PHY_ERR("show data: 0x%x %d\n ", buf, len);
	for (i = 0; i < len; i++) {
		PHY_ERR("%2x ", buf[i]);
		if (((i + 1) % 32) == 0) {
			PHY_ERR("\n");
		}
	}
	return;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
 *****************************************************************************/

extern uint32 get_special_data_from_oob(uchar *buf);
extern uint16 get_erase_count_from_oob(uchar *buf);
extern uint32 get_block_used_count_from_oob(uchar *buf);

void show_dict_page(unsigned int chip, unsigned int block, unsigned int page,
		u32 start, u32 len)
{
	unsigned char spare[64];
	int i;
	unsigned char *mbuf;

	uint32 special_data, block_used_count;
	uint16 erase_count;
	u32 start_byte;
	u32 len_byte;

	mbuf = nand_get_temp_buf(32 * 1024);


	nand_physic_read_super_page(chip, block, page,
			g_nssi->nsci->sector_cnt_per_super_page,
			mbuf, spare);

	special_data     = get_special_data_from_oob(spare);
	block_used_count = get_block_used_count_from_oob(spare);
	erase_count      = get_erase_count_from_oob(spare);

	PHY_ERR("block:%d page:%d special_data:%x,erase_count:%x,block_used_count:%x\n",
			block, page, special_data, erase_count, block_used_count);

	start_byte = start << 9;
	len_byte   = len << 9;

	for (i = start_byte; i < (start_byte + len_byte); i++) {
		PHY_ERR("|%2x", mbuf[i]);
		if (((i + 1) % 32) == 0) {
			PHY_ERR("\n");
		}
	}

	nand_free_temp_buf(mbuf, 32 * 1024);
	return;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
 *****************************************************************************/
void show_logic_page_all_history(unsigned int logic_page)
{
	unsigned char spare[64];
	int i, j, ret;
	uint32 special_data;


	for (i = 8; i < g_nssi->nsci->blk_cnt_per_super_chip; i++) {
		if (nand_physic_super_bad_block_check(0, i) != 0) {
			continue;
		}

		nand_physic_read_super_page(0, i, 0, 0, NULL, spare);
		if ((spare[0] == 0xff) && (spare[1] == 0xff) &&
				(spare[2] == 0xff) && (spare[3] == 0xff) &&
				(spare[4] == 0xff) && (spare[5] == 0xff)) {
			continue;
		}
		if ((spare[0] == 0xff) && (spare[1] == 0x55) &&
				(spare[2] == 0x55) && (spare[3] == 0x55) &&
				(spare[4] == 0x55)) {
			continue;
		}

		for (j = 0; j < g_nssi->nsci->page_cnt_per_super_blk; j++) {
			ret = nand_physic_read_super_page(0, i, j, 0, NULL,
					spare);
			if ((ret != 0) && (ret != ECC_LIMIT)) {
				PHY_ERR("ecc error chip0 block :%d page :0x%x \n",
						i, j);
			}
			special_data = get_special_data_from_oob(spare);
			special_data &= 0x0fffffff;
			if (special_data == logic_page) {
				show_dict_page(0, i, j, 8, 10);
			}
		}
	}

	return;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
 *****************************************************************************/
int nand_super_page_test(unsigned int chip, unsigned int block,
		unsigned int page, unsigned char *mbuf)
{
	unsigned char spare[64];
	int i, ret;

	PHY_ERR("nand super page_test chip:0x%x block:0x%x page:0x%x \n", chip,
			block, page);

	ret = nand_physic_erase_super_block(chip, block);
	if (ret != 0) {
		return ret;
	}

	for (i = 0; i < g_nssi->nsci->sector_cnt_per_super_page << 7; i++) {
		*((unsigned int *)(mbuf + (i << 2))) = i;
	}

	spare[0] = 0xff;
	for (i = 1; i < 16; i++) {
		spare[i] = 0xa0 + i;
	}

	nand_physic_write_super_page(chip, block, page,
			g_nssi->nsci->sector_cnt_per_super_page,
			mbuf, spare);

	MEMSET(mbuf, 0, g_nssi->nsci->sector_cnt_per_super_page << 9);
	MEMSET(spare, 0, 64);

	nand_physic_read_super_page(chip, block, page,
			g_nssi->nsci->sector_cnt_per_super_page,
			mbuf, spare);

	for (i = 0; i < g_nssi->nsci->sector_cnt_per_super_page << 7; i++) {
		if (*((unsigned int *)(mbuf + (i << 2))) != i) {
			PHY_ERR("data error chip:0x%x block:0x%x page:0x%x \n",
					chip, block, page);
			ret = 2;
			break;
		}
	}

	for (i = 1; i < 16; i++) {
		if ((spare[i] != 0xa0 + i) || (spare[0] != 0xff)) {
			PHY_ERR("spare error chip:0x%x block:0x%x page:0x%x \n",
					chip, block, page);
			ret = 2;
			break;
		}
	}

	nand_physic_erase_super_block(chip, block);

	return ret;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
 *****************************************************************************/
int nand_phy_page_test(unsigned int chip, unsigned int block, unsigned int page,
		unsigned char *mbuf)
{
	unsigned char spare[64];
	int i, ret = 0;
	int page_size;

	page_size = g_nsi->nci->sector_cnt_per_page << 9;

	PHY_ERR("nand phy page_test chip:0x%x block:0x%x page:0x%x \n", chip,
			block, page);

	nand_physic_erase_block(chip, block);

	for (i = 0; i < (page_size >> 2); i++) {
		*((unsigned int *)(mbuf + (i << 2))) = i;
	}

	spare[0] = 0xff;
	for (i = 1; i < 16; i++) {
		spare[i] = 0xa0 + i;
	}

	nand_physic_write_page(chip, block, page, page_size >> 9, mbuf, spare);

	MEMSET(mbuf, 0, page_size);
	MEMSET(spare, 0, 64);

	nand_physic_read_page(chip, block, page, page_size >> 9, mbuf, spare);

	for (i = 0; i < (page_size >> 2); i++) {
		if (*((unsigned int *)(mbuf + (i << 2))) != i) {
			PHY_ERR("data error chip:0x%x block:0x%x page:0x%x \n",
					chip, block, page);
			ret = 2;
			break;
		}
	}

	for (i = 1; i < 16; i++) {
		if ((spare[i] != 0xa0 + i) || (spare[0] != 0xff)) {
			PHY_ERR("spare error chip:0x%x block:0x%x page:0x%x \n",
					chip, block, page);
			ret = 2;
			break;
		}
	}
	nand_physic_erase_block(chip, block);

	return ret;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
 *****************************************************************************/
int nand_phy_block_test(unsigned int chip, unsigned int block,
		unsigned char *mbuf, u32 test_bad, u8 dat)
{
	unsigned char spare[64];
	int i, j, ret;
	int page_size;
	unsigned char *mbuf_t;

	page_size = g_nsi->nci->sector_cnt_per_page << 9;

	mbuf_t = mbuf;

	PHY_ERR("nand phy page_test !chip:0x%x block:0x%x\n", chip, block);

	if (test_bad == 0) {
		if (nand_physic_bad_block_check(chip, block) != 0) {
			PHY_ERR("nand phy page_test bad block\n");
			return 0;
		}
	}

	nand_physic_erase_block(chip, block);

	for (i = 0; i < page_size; i++) {
		mbuf_t[i] = dat;
	}

	spare[0] = 0xff;
	for (i = 1; i < 16; i++) {
		spare[i] = i;
	}

	for (j = 0; j < g_nsi->nci->page_cnt_per_blk; j++) {
		nand_physic_write_page(chip, block, j,
				g_nsi->nci->sector_cnt_per_page, mbuf,
				spare);
	}

	for (j = 0; j < g_nsi->nci->page_cnt_per_blk; j++) {
		MEMSET(mbuf, 0, page_size);
		MEMSET(spare, 0, 64);
		ret = nand_physic_read_page(chip, block, j,
				g_nsi->nci->sector_cnt_per_page,
				mbuf, spare);
		if (ret == ERR_ECC) {
			ret = 2;
			goto block_test_end;
		}

		if (ret == 0) {
			ret = 0;
			goto block_test_end;
		}

		mbuf_t = mbuf;
		for (i = 0; i < page_size; i++) {
			if (mbuf_t[i] != dat) {
				PHY_ERR("data error chip:0x%x block:0x%x page:0x%x offset:0x%x data:0x%x\n",
						chip, block, j, i, mbuf_t[i]);
				ret = 2;
				goto block_test_end;
			}
		}

		for (i = 1; i < 16; i++) {
			if ((spare[i] != i) || (spare[0] != 0xff)) {
				PHY_ERR("spare error chip:0x%x block:0x%x spare:0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x \n",
						chip, block, spare[0], spare[1],
						spare[2], spare[3], spare[4], spare[5],
						spare[6], spare[7]);
				ret = 2;
				goto block_test_end;
			}
		}
		ret = 0;
	}

block_test_end:
	nand_physic_erase_block(chip, block);

	return ret;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
 *****************************************************************************/
int nand_phy_block_erase_test(unsigned int chip, unsigned int block)
{
	unsigned char spare[64];
	int i, j, ret;
	int page_size;

	unsigned char *mbuf;

	page_size = g_nsi->nci->sector_cnt_per_page << 9;

	mbuf = nand_get_temp_buf(page_size);
	if (mbuf == NULL) {
		return 1;
	}

	PHY_ERR("nand phy block erase test !chip:0x%x block:0x%x\n", chip,
			block);
	nand_physic_erase_block(chip, block);

	for (j = 0; j < g_nsi->nci->page_cnt_per_blk; j++) {
		MEMSET(mbuf, 0, page_size);
		MEMSET(spare, 0, 64);
		nand_physic_temp1 = 1;
		nand_physic_read_page(chip, block, j,
				g_nsi->nci->sector_cnt_per_page, mbuf,
				spare);
		nand_physic_temp1 = 0;
		if ((mbuf[0] != 0xff) && (mbuf[1] != 0xff) &&
				(mbuf[2] != 0xff) && (mbuf[3] != 0xff)) {
			PHY_ERR("data error chip:0x%x block:0x%x page:0x%x data:%x %x %x %x\n",
					chip, block, j, mbuf[0], mbuf[1], mbuf[2],
					mbuf[3]);
			ret = 2;
			goto test_ff_end;
		}
		for (i = 1; i < 16; i++) {
			if ((spare[i] != 0xff) || (spare[0] != 0xff)) {
				PHY_ERR("spare error chip:0x%x block:0x%x spare:0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x \n",
						chip, block, spare[0], spare[1],
						spare[2], spare[3], spare[4], spare[5],
						spare[6], spare[7]);
				ret = 2;
				goto test_ff_end;
			}
		}
	}

test_ff_end:
	nand_physic_erase_block(chip, block);

	nand_free_temp_buf(mbuf, page_size);
	return ret;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
 *****************************************************************************/
void nand_phy_erase_all(void)
{
	unsigned int i, j;

	for (i = 0; i < g_nsi->chip_cnt; i++) {
		for (j = 0; j < g_nsi->nci->blk_cnt_per_chip; j++) {
			PHY_ERR("erase chip: %d block: %d\n", i, j);
			nand_physic_erase_block(i, j);
			nand_wait_all_rb_ready();
		}
	}
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
 *****************************************************************************/
void nand_phy_check_all(u8 dat)
{
	unsigned char *mbuf;
	unsigned int i, j;
	int ret;

	int page_size = g_nsi->nci->sector_cnt_per_page << 9;

	PHY_ERR("nand phy check all start\n");

	mbuf = nand_get_temp_buf(page_size);

	for (i = 0; i < g_nsi->chip_cnt; i++) {
		for (j = 0; j < g_nsi->nci->blk_cnt_per_chip; j++) {
			ret = nand_phy_block_test(i, j, mbuf, 0, dat);
			if (ret == 0) {
				continue;
			}
			PHY_ERR("mark chip: %d block: %d\n", i, j);
			nand_physic_bad_block_mark(i, j);
		}
	}
	nand_free_temp_buf(mbuf, page_size);

	PHY_ERR("nand phy check all end\n");
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
 *****************************************************************************/
int nand_phy_read_block_test(unsigned int chip, unsigned int start_block,
		unsigned int end_block)
{
	unsigned char spare[64];
	int i, j, ret = 0;
	int page_size;

	unsigned char *mbuf;

	page_size = g_nsi->nci->sector_cnt_per_page << 9;

	mbuf = nand_get_temp_buf(page_size);
	if (mbuf == NULL) {
		return 1;
	}

	for (j = start_block; j < end_block; j++) {
		PHY_ERR("nand_phy_read_block_test !chip:0x%x block:0x%x\n",
				chip, j);

		for (i = 0; i < g_nsi->nci->page_cnt_per_blk; i++) {
			ret = nand_physic_read_page(
					chip, j, i, g_nsi->nci->sector_cnt_per_page,
					mbuf, spare);
			if (ret < 0) {
				PHY_ERR("block error! chip:%d block:%d page:%d ret:%d\n",
						chip, j, i, ret);
				break;
			}
		}
	}

	nand_free_temp_buf(mbuf, page_size);
	return ret;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
 *****************************************************************************/
int nand_update_end(void)
{
	/*int ret;*/
	unsigned char *mbuf;
	int page_size;

	page_size = g_nsi->nci->sector_cnt_per_page << 9;
	mbuf      = nand_get_temp_buf(page_size);

	nand_phy_block_test(0, 0, mbuf, 0, 0xaa);

	nand_phy_block_test(0, 2, mbuf, 0, 0xaa);

	nand_free_temp_buf(mbuf, page_size);
	PHY_ERR("nand update end !\n");

	return 0;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
 *****************************************************************************/
void nand_special_test(void)
{
	return;
}
