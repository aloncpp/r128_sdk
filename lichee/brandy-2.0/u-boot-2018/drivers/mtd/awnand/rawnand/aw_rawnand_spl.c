/**
 * spdx-license-identifier: gpl-2.0+
 * aw_rawnand_spl.c
 *
 * (c) copyright 2020 - 2021
 * allwinner technology co., ltd. <www.allwinnertech.com>
 * cuizhikui <cuizhikui@allwinnertech.com>
 *
 */

#include <common.h>
#include <linux/mtd/aw-rawnand.h>
#include "aw_rawnand_nfc.h"
#include <private_toc.h>
#include <sprite_verify.h>

/*keep it the same with driver/sunxi_flash/nand/nand_bsp.h*/
typedef struct {
	__u32	ChannelCnt;
	__u32	ChipCnt;                            //the count of the total nand flash chips are currently connecting on the CE pin
	__u32	ChipConnectInfo;                    //chip connect information, bit == 1 means there is a chip connecting on the CE pin
	__u32	RbCnt;
	__u32	RbConnectInfo;						//the connect  information of the all rb  chips are connected
	__u32	RbConnectMode;						//the rb connect  mode
	__u32	BankCntPerChip;                     //the count of the banks in one nand chip, multiple banks can support Inter-Leave
	__u32	DieCntPerChip;                      //the count of the dies in one nand chip, block management is based on Die
	__u32	PlaneCntPerDie;                     //the count of planes in one die, multiple planes can support multi-plane operation
	__u32	SectorCntPerPage;                   //the count of sectors in one single physic page, one sector is 0.5k
	__u32	PageCntPerPhyBlk;                   //the count of physic pages in one physic block
	__u32	BlkCntPerDie;                       //the count of the physic blocks in one die, include valid block and invalid block
	__u32	OperationOpt;                       //the mask of the operation types which current nand flash can support support
	__u32	FrequencePar;                       //the parameter of the hardware access clock, based on 'MHz'
	__u32	EccMode;                            //the Ecc Mode for the nand flash chip, 0: bch-16, 1:bch-28, 2:bch_32
	__u8	NandChipId[8];                      //the nand chip id of current connecting nand chip
	__u32	ValidBlkRatio;                      //the ratio of the valid physical blocks, based on 1024
	__u32	good_block_ratio;					//good block ratio get from hwscan
	__u32	ReadRetryType;						//the read retry type
	__u32	DDRType;
	__u32	uboot_start_block;
	__u32	uboot_next_block;
	__u32	logic_start_block;
	__u32	nand_specialinfo_page;
	__u32	nand_specialinfo_offset;
	__u32	physic_block_reserved;
	/*special nand cmd for some nand in batch cmd, only for write*/
	__u32	random_cmd2_send_flag;
	/*random col addr num in batch cmd*/
	__u32	random_addr_num;
	/*real physic page size*/
	__u32	nand_real_page_size;
	__u32	Reserved[13];
} boot_nand_para_t;

#define NAND_VERSION_0 0x03
#define NAND_VERSION_1 0x01
struct aw_rawnand_boot0 {
	struct aw_nand_chip *chip;
/*
 *#define SLC_NAND	(0)
 *#define MLC_NAND	(1)
 */
	uint8_t nand_type;
	uint8_t ecc_mode;
	uint8_t reserve[2];
#define SLC_MDATA_IO (1024)
#define MLC_MDATA_IO (4096)
	/*mdata_len equal to (pagesize - ecc code), which >= MDATA_IO*/
	int mdata_len;
	uint8_t *mdata;
#define BOOT0_OOB_LEN	(8)
	uint8_t oob[BOOT0_OOB_LEN];
	int init_flag;
	/* according to boot0 sram size,
	 * minimum one page 1K boot0img*/
#define BOOT0_PAGE_CNT_PER_COPY	(128)
	int page_cnt_per_copy;
	int copys_per_blk;

	/*uboot start block*/
	int boundary;
	int writen_copy;
};

struct aw_rawnand_boot0 g_boot0;

static inline struct aw_rawnand_boot0 *get_rawnand_boot0(void)
{
	return &g_boot0;
}

int rawslcnand_write_boot0_page(struct mtd_info *mtd, struct aw_nand_chip *chip,
		uint8_t *mdata, int mlen, uint8_t *sdata, int slen, int page)
{
	struct aw_nand_host *host = awnand_chip_to_host(chip);

	int ret = 0;
	uint8_t status = 0;
	int row_cycles = chip->row_cycles;


	chip->operate_boot0 = 1;
	chip->boot0_ecc_mode = MAX_ECC_BCH_80;

	BATCH_REQ_WRITE_SEQ(req, page, row_cycles, mdata, mlen, sdata, slen);

	if (!chip->dev_ready_wait(mtd)) {
		awrawnand_err("dev is busy write boot0 page@%d fail\n", page);
		ret = -ETIMEDOUT;
		goto out;
	}

	ret = host->batch_op(chip, &req);
	if (ret == ECC_ERR) {
		awrawnand_err("%s write boot0 page@%d fail\n", __func__, 0);
		goto out;
	}

	if (!chip->dev_ready_wait(mtd)) {
		awrawnand_err("dev is busy write boot0 page@%d fail\n", page);
		ret = -ETIMEDOUT;
		goto out;
	}

	status = chip->dev_status(mtd);
	if (status & RAWNAND_STATUS_FAIL) {
		awrawnand_err("write boot0 page@%d fail\n", page);
		ret = -EIO;
	}

	chip->operate_boot0 = 0;
	chip->boot0_ecc_mode = 0;

out:
	return ret;
}

int rawslcnand_read_boot0_page(struct mtd_info *mtd, struct aw_nand_chip *chip,
		uint8_t *mdata, int mlen, uint8_t *sdata, int slen, int page)
{
	struct aw_nand_host *host = awnand_chip_to_host(chip);
	int row_cycles = chip->row_cycles;

	int ret = 0;

	chip->operate_boot0 = 1;
	chip->boot0_ecc_mode = MAX_ECC_BCH_80;

	BATCH_REQ_READ_SEQ(req, page, row_cycles, mdata, mlen, sdata, slen);

	if (!chip->dev_ready_wait(mtd)) {
		awrawnand_err("dev is busy read page@%d fail\n", page);
		ret = -EIO;
		goto out;
	}

	ret = host->batch_op(chip, &req);
	if (ret == ECC_ERR)
		awrawnand_err("read page@%d fail\n", 0);

	chip->operate_boot0 = 0;
	chip->boot0_ecc_mode = 0;

out:
	return ret;
}


static int rawnand_calc_page_valid_main_data_len(int pagesize, int boot0_ecc_mode, int ecc_mode)
{
	int boot0_ecc_bits = 0, normal_ecc_bits = 0;
	int increase_ecc_size = 0;
	int cnt = 1;
	int i = 0;
	/*ecc block size 1024Bytes*/
	int ecc_block_cnts = (pagesize >> 10);

	boot0_ecc_bits = ecc_bits_tbl[boot0_ecc_mode];
	normal_ecc_bits = ecc_bits_tbl[ecc_mode];
	increase_ecc_size = (14 * (boot0_ecc_bits - normal_ecc_bits) / 8);

	/* data_min_io is ecc block size,
	 * so the increased overhead(increase_ecc_size)
	 * is calculate in terms of the data_min_io*/
	for (i = (ecc_block_cnts - 1); i > 0; i--) {
		if ((increase_ecc_size * i) < (cnt << 10))
			break;
		cnt++;
	}

	return ((ecc_block_cnts - cnt) << 10);
}

/**
 * rawslcnand_write_boot0_one - write one or more copy boot0
 * @len: boot0 len
 * @buf: boot0 img
 * @count: range from 0 to n(rawnand_uboot_blknum(&n, NULL))
 */
static int rawslcnand_write_boot0_one(struct aw_rawnand_boot0 *boot0,
		unsigned int len, void *buf, int count)
{
	struct aw_nand_chip *chip = boot0->chip;
	struct mtd_info *mtd = awnand_chip_to_mtd(chip);
	int ret = 0;
	int start = 0;
	int blks_per_copy = 0;
	int blk = 0;
	int page = 0;
	int pages_per_blk = (1 << chip->pages_per_blk_shift);
	uint8_t *mdata = buf;
	int mlen = boot0->mdata_len;
	uint8_t *sdata = boot0->oob;
	int slen = BOOT0_OOB_LEN;

	int writen_len = 0;

	blks_per_copy = boot0->page_cnt_per_copy >> chip->pages_per_blk_shift;
	if (boot0->page_cnt_per_copy & chip->pages_per_blk_mask)
		blks_per_copy++;

	start = count * blks_per_copy;

	if ((start + blks_per_copy) > boot0->boundary)
		return 0;


	chip->select_chip(mtd, 0);
	for (blk = start; blk < start + blks_per_copy; blk++) {
		page = blk << chip->pages_per_blk_shift;
		ret = chip->erase(mtd, page);
		if (ret) {
			/*
			 *awrawnand_err("erase block-%d fail when download boot0 count-%d\n",
			 *                blk, count);
			 *awrawnand_err("skip boot0 count-%d\n", count);
			 */
			break;
		}

		for (; page < ((blk << chip->pages_per_blk_shift) + pages_per_blk);
				page++) {
			ret = chip->write_boot0_page(mtd, chip, mdata, mlen, sdata, slen, page);
			if (ret) {
				/*
				 *awrawnand_err("when write boot0 fail in page-%d\n\n", page);
				 *[>the info is no mean<]
				 *awrawnand_dbg("ecc mode-%d ecc limit-%d\n", ecc_bits_tbl[boot0->ecc_mode],
				 *                ecc_limit_tab[boot0->ecc_mode]);
				 */
				goto out;
			}
			writen_len += SLC_MDATA_IO;
			if (writen_len == len) {
				writen_len = 0;
				/*awrawnand_print("W boot0 cp-%d suc\n", boot0->writen_copy++);*/
			}
			mdata = buf + writen_len;
		}
	}
	chip->select_chip(mtd, -1);

out:
	return ret;
}

/**
 * rawslcnand_read_boot0_one - read a right boot0 in one boot0 area
 * @len: boot0 size(buf len)
 * @buf: boot0 buffer
 * @count: 0 from n(rawnand_uboot_blknum(&n, NULL))
 * */
static int rawslcnand_read_boot0_one(struct aw_rawnand_boot0 *boot0,
		unsigned int len, void *buf, int count)
{
	struct aw_nand_chip *chip = boot0->chip;
	struct mtd_info *mtd = awnand_chip_to_mtd(chip);
	int ret = 0;

	int start = 0;
	int blks_per_copy = 0;
	int blk = 0;
	int page = 0;
	int pages_per_blk = (1 << chip->pages_per_blk_shift);
	uint8_t *mdata = buf;
	int mlen = boot0->mdata_len;
	uint8_t *sdata = boot0->oob;
	int slen = BOOT0_OOB_LEN;

	int read_len = 0;

	blks_per_copy = boot0->page_cnt_per_copy >> chip->pages_per_blk_shift;
	if (boot0->page_cnt_per_copy & chip->pages_per_blk_mask)
		blks_per_copy++;

	start = count * blks_per_copy;

	if ((start + blks_per_copy) > boot0->boundary)
		return 0;


	chip->select_chip(mtd, 0);
	for (blk = start; blk < start + blks_per_copy; blk++) {
		page = blk << chip->pages_per_blk_shift;

		for (; page < ((blk << chip->pages_per_blk_shift) + pages_per_blk);
				page++) {
			ret = chip->read_boot0_page(mtd, chip, mdata, mlen, sdata, slen, page);
			if (ret) {
				awrawnand_err("when read boot0 fail in page-%d\n\n", page);
				/*the info is no mean*/
				awrawnand_dbg("ecc mode-%d ecc limit-%d\n",
						ecc_bits_tbl[boot0->ecc_mode],
						ecc_limit_tab[boot0->ecc_mode]);
				goto out;
			}
			read_len += SLC_MDATA_IO;
			if (read_len == len) {
				awrawnand_info("R boot0 suc\n");
				goto out;
			}
			mdata = buf + read_len;
		}
	}
	chip->select_chip(mtd, -1);

out:
	return ret;
}

/**
 * rawslcnand_mtd_download_boot0 - download boot0
 * @boot0:boot0 structure
 * @len: boot size
 * @buf: boot0 img
 * */
static int rawslcnand_mtd_download_boot0(struct aw_rawnand_boot0 *boot0,
		unsigned int len, void *buf)
{
	int ret = 0;
	int count = 0;
	unsigned int start = 0;
	unsigned int end = 0;
	rawnand_uboot_blknum(&end, NULL);

	for (count = start; count < end; count++) {
		ret &= rawslcnand_write_boot0_one(boot0, len, buf, count);
	}

	if (ret)
		awrawnand_err("download boot0 fail\n");

	return ret;
}

/**
 * rawslcnand_mtd_upload_boot0 - read boot0
 * @boot0: boot0 structure
 * @len: boot0 size
 * @buf: buffer to store boot0 img
 * */
static int rawslcnand_mtd_upload_boot0(struct aw_rawnand_boot0 *boot0,
		unsigned int len, void *buf)
{
	int ret = 0;
	int count = 0;
	unsigned int start = 0;
	unsigned int end = 0;
	rawnand_uboot_blknum(&end, NULL);

	for (count = start; count < end; count++) {
		ret = rawslcnand_read_boot0_one(boot0, len, buf, count);
		if (!ret)
			break;
	}

	if (count == end)
		awrawnand_err("upload boot0 fail\n");

	return ret;
}

/**
 * rawnand_mtd_download_boot0_init - init boot0 structure parameter
 * */
static inline void rawnand_mtd_download_boot0_init(void)
{
	struct aw_nand_chip *chip = get_rawnand();
	unsigned int uboot_start;

	rawnand_uboot_blknum(&uboot_start, NULL);

	if (!g_boot0.init_flag) {
		g_boot0.chip = chip;
		g_boot0.nand_type = SLC_NAND;
		g_boot0.page_cnt_per_copy = BOOT0_PAGE_CNT_PER_COPY;
		g_boot0.ecc_mode = MAX_ECC_BCH_80;
		g_boot0.mdata_len = rawnand_calc_page_valid_main_data_len(chip->pagesize,
				g_boot0.ecc_mode, chip->ecc_mode);
		memset(g_boot0.oob, 0xff, BOOT0_OOB_LEN);
		g_boot0.oob[0] = 0xff;
		g_boot0.oob[1] = 0x00;
		g_boot0.oob[2] = NAND_VERSION_0;
		g_boot0.oob[3] = NAND_VERSION_1;
		g_boot0.init_flag = 1;
		g_boot0.boundary = uboot_start;
		g_boot0.writen_copy = 0;
		awrawnand_info("boot0:nand_type@%s\n", g_boot0.nand_type ? "mlc" : "slc");
		awrawnand_info("boot0:ecc_mode@%d\n", g_boot0.ecc_mode);
		awrawnand_info("boot0:mdata_len@%d\n", g_boot0.mdata_len);
		awrawnand_info("boot0:doundary@%d\n", g_boot0.boundary);
	}
}

/**
 * rawnand_mtd_download_boot0_exit - destory process var
 * */
static inline void rawnand_mtd_download_boot0_exit(void)
{
	g_boot0.init_flag = 0;
	g_boot0.writen_copy = 0;
}


/**
 * fill boot0 header
 */
int rawnand_mtd_get_flash_info(void *data, unsigned int len)
{
	struct aw_nand_chip *chip = get_rawnand();
	boot_nand_para_t *boot_info = data;
	unsigned int uboot_start, uboot_end;

	rawnand_uboot_blknum(&uboot_start, &uboot_end);

	/* nand information */

	boot_info->ChipCnt = chip->chips;
	boot_info->DieCntPerChip = chip->dies;
	boot_info->SectorCntPerPage = (1 << (chip->pagesize_shift - 9));
	boot_info->PageCntPerPhyBlk = (1 << chip->pages_per_blk_shift);
	/*OperationOpt must be match to boot0 define*/
	if (chip->options & RAWNAND_NFC_RANDOM)
		boot_info->OperationOpt |= (1 << 7);
	if (chip->options & RAWNAND_TOGGLE_DDR_TO_SDR)
		boot_info->OperationOpt |= (1 << 27);
	if (chip->type == SLC_NAND)
		boot_info->OperationOpt &= ~(0xff << 12);
	else {
		boot_info->OperationOpt &= ~(0xff << 12);
		awrawnand_err("don't support nand type, default slc nand\n");
	}
	boot_info->FrequencePar = chip->clk_rate;
	boot_info->EccMode = chip->ecc_mode;
	memcpy(boot_info->NandChipId, chip->id, RAWNAND_MAX_ID_LEN);


	/* others */
	boot_info->uboot_start_block = uboot_start;
	boot_info->uboot_next_block = uboot_end;
	boot_info->logic_start_block = uboot_end + AW_RAWNAND_RESERVED_PHY_BLK_FOR_SECURE_STORAGE;
	boot_info->physic_block_reserved = 0;

	awrawnand_info("flash id@%02x %02x %02x %02x %02x %02x %02x %02x\n",
			boot_info->NandChipId[0], boot_info->NandChipId[1],
			boot_info->NandChipId[2], boot_info->NandChipId[3],
			boot_info->NandChipId[4], boot_info->NandChipId[5],
			boot_info->NandChipId[6], boot_info->NandChipId[7]);
	awrawnand_info("uboot_start_block@%u\n", boot_info->uboot_start_block);
	awrawnand_info("uboot_end_block@%u\n", boot_info->uboot_next_block);

	return 0;
}

static int rawnand_mtd_download_uboot_fill_remain_pages_in_block(struct aw_nand_chip *chip, int page)
{
	struct mtd_info *mtd = awnand_chip_to_mtd(chip);
	int ret = 0;
	int block_end_page = (((page >> chip->pages_per_blk_shift) + 1) << chip->pages_per_blk_shift);
	int p = 0;
	/*uboot spare require*/
	int slen = chip->pagesize > SZ_2K ? 16 : 8;
	uint8_t *mdata = kzalloc(chip->pagesize, GFP_KERNEL);
	if (mdata == NULL) {
		awrawnand_err("fill remain page: kzalloc mdata fail\n");
		return -ENOMEM;
	}
	memset(mdata, 0x55, chip->pagesize);
	uint8_t sdata[slen];
	memset(sdata, 0xA5, slen);
	/*bad block mark flag position*/
	sdata[0] = 0xff;

	for (p = page; p < block_end_page; p++) {
		ret = chip->write_page(mtd, chip, mdata, chip->pagesize,
				sdata, slen, p);
		if (ret) {
			awrawnand_err("fill remain data in block@%d page@%d fail chip page@%d\n",
					page >> chip->pages_per_blk_shift, page & chip->pages_per_blk_mask, page);
		}
	}

	kfree(mdata);
	return 0;
}

static int rawslcnand_mtd_upload_uboot(struct aw_nand_chip *chip,
		unsigned int len, void *buf)
{
	unsigned int start = 0;
	unsigned int end = 0;
	int blk = 0;
	int ret = -1;
	uint8_t *mdata = buf;
	int slen = chip->pagesize > SZ_2K ? 16 : 8;
	uint8_t sdata[slen];
	struct mtd_info *mtd = awnand_chip_to_mtd(chip);
	int pages_per_blk = (1 << chip->pages_per_blk_shift);
	int had_read_len = 0;
	sbrom_toc1_head_info_t *head = buf;
	unsigned int uboot_len = 0;
	int page = 0;
	uint32_t check_sum = 0;
	uint8_t *mbuf = kzalloc(chip->pagesize, GFP_KERNEL);
	if (mbuf == NULL) {
		awrawnand_err("upload uboot: kzalloc fail\n");
		goto out1;
	}
	printf("Enter %s\n", __func__);

	rawnand_uboot_blknum(&start, &end);

	chip->select_chip(mtd, 0);
	for (blk = start; blk < end; blk++) {
		if (chip->block_bad(mtd, blk)) {
			awrawnand_err("block@%d is bad,skip it\n", blk);
			continue;
		}

		if (had_read_len == 0)
			mdata = buf;

		page = blk << chip->pages_per_blk_shift;
		for (; page < ((blk << chip->pages_per_blk_shift) + pages_per_blk);
				page++) {

			ret = chip->read_page(mtd, chip, mdata, chip->pagesize, sdata, slen, page);
			if (ret) {
				awrawnand_err("read page@%d fail\n", page);
				break;
			}

			had_read_len += chip->pagesize;
			mdata += chip->pagesize;

			if (head->magic != TOC_MAIN_INFO_MAGIC) {
				had_read_len = 0;
				break;
			}

			uboot_len = head->valid_len;
			if (len < uboot_len) {
				awrawnand_err("upload uboot buffer size(len)@%d is less the"
						"real ubootsize@%d, pls check\n", len,
						uboot_len);
				goto out;
			}

			if (had_read_len >= uboot_len) {
				memcpy(buf + (had_read_len - chip->pagesize),
						mbuf, uboot_len - (had_read_len - chip->pagesize));
			}

			/*last page data align to pagesize read, prevent illegal access buf*/
			if ((uboot_len - had_read_len) <= chip->pagesize) {
				mdata = mbuf;
			}

			if (had_read_len >= uboot_len) {
				printf("to check sum\n");
				check_sum = sunxi_sprite_generate_checksum(buf,
						uboot_len, head->add_sum);
				had_read_len = 0;
				if (check_sum == head->add_sum) {
					awrawnand_print("upload uboot success\n");
					ret = 0;
					goto out;
				} else
					break;
			}

		} /*for (; ; page++*/
	} /*for (blk = start; blk < end; blk++)*/

	chip->select_chip(mtd, -1);
	printf("Exit %s\n", __func__);


out:
	kfree(mbuf);
	return ret;
out1:
	return -ENOMEM;
}

static int rawslcnand_mtd_download_uboot(struct aw_nand_chip *chip,
		unsigned int len, void *buf)
{
	unsigned int start = 0;
	unsigned int end = 0;
	int blk = 0;
	int ret = 0;
	uint8_t *mdata = buf;
	/*uboot spare require*/
	int slen = chip->pagesize > SZ_2K ? 16 : 8;
	uint8_t sdata[slen];
	struct mtd_info *mtd = awnand_chip_to_mtd(chip);
	int pages_per_blk = (1 << chip->pages_per_blk_shift);
	int writen_len = 0;
	int copy = 0;
	int page = 0;
	int fail_flag = 0;
	uint8_t *mbuf = kzalloc(chip->pagesize, GFP_KERNEL);
	if (mbuf == NULL) {
		awrawnand_err("download uboot: kzalloc fail\n");
		goto out;
	}

	/*uboot oob layout*/
	sdata[0] = 0xff;
	sdata[1] = 0;
	sdata[2] = NAND_VERSION_0;
	sdata[3] = NAND_VERSION_1;
	memset(sdata + 4, 0xff, slen - 4);

	rawnand_uboot_blknum(&start, &end);

	awrawnand_info("download uboot len@0x%x [%d to%d]\n", len, start, end);

	chip->select_chip(mtd, 0);
	for (blk = start; blk < end; blk++) {

		if (chip->block_bad(mtd, blk)) {
			awrawnand_err("block@%d is bad,skip it\n", blk);
			continue;
		}

		ret = chip->erase(mtd, (blk << chip->pages_per_blk_shift));
		if (ret) {
			awrawnand_err("erase block@%d fail, skip it\n", blk);
			continue;
		}

		if (writen_len == 0)
			mdata = buf;

		page = blk << chip->pages_per_blk_shift;
		for (; page < ((blk << chip->pages_per_blk_shift) + pages_per_blk);
				page++) {


			ret = chip->write_page(mtd, chip, mdata, chip->pagesize, sdata, slen, page);
			if (ret) {
				awrawnand_err("write page@%d fail\n", page);
				fail_flag = 1;
			}

			writen_len += chip->pagesize;
			mdata += chip->pagesize;

			/*prevent len is not align to pagesize*/
			if (((len - writen_len) < chip->pagesize) &&
					((len - writen_len) > 0)) {
				memcpy(mbuf, mdata, (len - writen_len));
				mdata = mbuf;
			}

			if (writen_len >= len) {
				rawnand_mtd_download_uboot_fill_remain_pages_in_block(chip,
						(page + 1));
				if (fail_flag == 0)
					awrawnand_print("W uboot cp@%d ok\n", copy++);
				writen_len = 0;
				break;
			}
		} /*for (; ; page++*/
	} /*for (blk = start; blk < end; blk++)*/
	chip->select_chip(mtd, -1);


	/*at least one copy is ok*/
	kfree(mbuf);
	return copy ? 0 : -1;
out:
	return -ENOMEM;
}

/**
 * rawnand_mtd_upload_uboot - upload uboot img interface
 * @len: uboot size
 * @buf: uboot img
 **/
int rawnand_mtd_upload_uboot(unsigned int len, void *buf)
{
	int ret = 0;
	struct aw_nand_chip *chip = get_rawnand();

	if (chip->type == SLC_NAND) {
		ret = rawslcnand_mtd_upload_uboot(chip, len, buf);
	} else {
		awrawnand_err("upload uboot don't support nand type@%s\n",
				chip->type == SLC_NAND ? "slc" : "mlc");
	}

	return ret;
}

/**
 * rawnand_mtd_download_uboot - download uboot img interface
 * @len: uboot size
 * @buf: uboot img
 **/
int rawnand_mtd_download_uboot(unsigned int len, void *buf)
{
	int ret = 0;
	struct aw_nand_chip *chip = get_rawnand();

	if (chip->type == SLC_NAND) {
		ret = rawslcnand_mtd_download_uboot(chip, len, buf);
	} else {
		awrawnand_err("download uboot don't support nand type@%s\n",
				chip->type == SLC_NAND ? "slc" : "mlc");
	}

#ifdef CONFIG_AW_RAWNAND_BURN_CHECK_UBOOT
	uint8_t *uboot = kzalloc(len, GFP_KERNEL);
	if (uboot == NULL) {
		awrawnand_err("kzalloc buffer for read uboot after write fail\n");
		goto out;
	}

	rawnand_mtd_upload_uboot(len, uboot);
	if (!memcmp(buf, uboot, len))
		awrawnand_print("write & read the uboot is same\n");
	else
		awrawnand_print("write & read the uboot is diff\n");
out:
	kfree(uboot);
#endif

	return ret;
}

/**
 * rawnand_mtd_upload_boot0 - read boot0 interface
 * @len: boot0 size
 * @buf: boot0 img
 **/
int rawnand_mtd_upload_boot0(unsigned int len, void *buf)
{

	int ret = 0;
	struct aw_rawnand_boot0 *boot0 = get_rawnand_boot0();

	rawnand_mtd_download_boot0_init();

	if (g_boot0.nand_type == SLC_NAND) {
		ret = rawslcnand_mtd_upload_boot0(boot0, len, buf);
	} else {
		awrawnand_err("don't support %snand to down boot0\n");
		ret = -EINVAL;
	}

	rawnand_mtd_download_boot0_exit();

	return ret;
}
/**
 * rawnand_mtd_download_boot0 - download boot0 interface
 * @len: boot0 size
 * @buf: boot0 img
 * */
int rawnand_mtd_download_boot0(unsigned int len, void *buf)
{

	int ret = 0;
	struct aw_rawnand_boot0 *boot0 = get_rawnand_boot0();

	rawnand_mtd_download_boot0_init();

	if (g_boot0.nand_type == SLC_NAND) {
		ret = rawslcnand_mtd_download_boot0(boot0, len, buf);
	} else {
		awrawnand_err("don't support %snand to down boot0\n");
		ret = -EINVAL;
	}
#ifdef CONFIG_AW_RAWNAND_BURN_CHECK_BOOT0
	uint8_t *rbuf = malloc_align(len, 64);
	if (!rbuf)
		awrawnand_err("malloc read buf fail\n");
	rawnand_mtd_upload_boot0(len, rbuf);

	if (memcmp(rbuf, buf, len))
		awrawnand_err("read boot0 fail\n");
	else
		awrawnand_info("read&write is the same\n");
#endif
	rawnand_mtd_download_boot0_exit();
	return ret;
}
