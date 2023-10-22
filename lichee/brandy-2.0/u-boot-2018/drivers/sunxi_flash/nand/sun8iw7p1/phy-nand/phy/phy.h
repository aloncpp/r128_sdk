//SPDX-License-Identifier:	GPL-2.0+
#ifndef _PHY_H
#define _PHY_H

#include "mbr.h"
#include "../physic_v2/nand_type.h"
#include "../physic_v2/uboot_head.h"

#define PHY_RESERVED_BLOCK_RATIO 256
#define MAX_PHY_RESERVED_BLOCK 64
#define MIN_PHY_RESERVED_BLOCK 8
#define MIN_PHY_RESERVED_BLOCK_V2 0

#define FACTORY_BAD_BLOCK_SIZE 2048
#define PHY_PARTITION_BAD_BLOCK_SIZE 4096
#define PARTITION_BAD_BLOCK_SIZE 4096

#define MBR_DATA_SIZE
#define BYTES_OF_USER_PER_PAGE 16
#define FACTORY_BAD_BLOCK_ERROR 159
#define BYTES_PER_SECTOR 512
#define SHIFT_PER_SECTOR 9
#define MIN_BYTES_OF_USER_PER_PAGE 16

#define ZONE_RESERVED_BLOCK_RATIO 16
#define SYS_ZONE_RESERVED_BLOCK_RATIO 4
#define MIN_FREE_BLOCK_NUM 20
#define MIN_FREE_BLOCK 2

#define SYS_RESERVED_BLOCK_RATIO 5
#define NORM_RESERVED_BLOCK_RATIO 10
#define MIN_FREE_BLOCK_NUM_RUNNING 15
#define MIN_FREE_BLOCK_NUM_V2 25
#define MAX_FREE_BLOCK_NUM 120
#define MIN_FREE_BLOCK_REMAIN 4
#define ENABLE_CRC_MAGIC 0x63726365 //crce
#define DISABLE_CRC // use CRC should undef it

//==========================================================

struct _nand_lib_cfg {
	unsigned int phy_interface_cfg;

	unsigned int phy_support_two_plane;
	unsigned int phy_nand_support_vertical_interleave;
	unsigned int phy_support_dual_channel;

	unsigned int phy_wait_rb_before;
	unsigned int phy_wait_rb_mode;
	unsigned int phy_wait_dma_mode;
};

struct _nand_phy_partition;

struct _nand_super_block {
	unsigned short Block_NO;
	unsigned short Chip_NO;
};

struct _nand_disk {
	unsigned int size;
	//unsigned int offset;
	unsigned int type;
	unsigned char name[PARTITION_NAME_SIZE];
};

struct _partition {
	struct _nand_disk nand_disk[MAX_PART_COUNT_PER_FTL];
	unsigned int size;
	unsigned int cross_talk;
	unsigned int attribute;
	struct _nand_super_block start;
	struct _nand_super_block end;
	//unsigned int offset;
};

struct _bad_block_list {
	struct _nand_super_block super_block;
	struct _nand_super_block *next;
};

typedef union {
	unsigned char ndata[4096];
	PARTITION_MBR data;
} _MBR;

typedef union {
	unsigned char ndata[2048 + 512];
	struct _partition data[MAX_PARTITION];
} _PARTITION;

typedef union {
	unsigned char ndata[512];
	struct _nand_super_storage_info data;
} _NAND_STORAGE_INFO;

typedef union {
	unsigned char ndata[2048];
	struct _nand_super_block data[512];
} _FACTORY_BLOCK;

typedef union {
	unsigned char ndata[256];
	struct _uboot_info data;
} _UBOOT_INFO;

typedef union {
	unsigned char ndata[1024];
	unsigned char data[1024];
} _NAND_SPECIAL_INFO;
struct _boot_info {
	unsigned int magic;
	unsigned int len;
	unsigned int sum;

	unsigned int no_use_block;
	unsigned int uboot_start_block;
	unsigned int uboot_next_block;
	unsigned int logic_start_block;
	unsigned int nand_specialinfo_page;
	unsigned int nand_specialinfo_offset;
	unsigned int physic_block_reserved;
	unsigned int nand_ddrtype;
	unsigned int ddr_timing_cfg;
	unsigned int enable_crc; // ENABLE_CRC_MAGIC
	unsigned int nouse[128 - 13];

	_MBR mbr; //1k
	_PARTITION partition; //0.5k
	_NAND_STORAGE_INFO storage_info; //0.5k
	_FACTORY_BLOCK factory_block; //1k
	//_UBOOT_INFO uboot_info;             //0.25K
	_NAND_SPECIAL_INFO nand_special_info; //1k               offset 9.5k
};

//ȫ��flash����
struct _nand_info {
	unsigned short type;
	unsigned short SectorNumsPerPage;
	unsigned short BytesUserData;
	unsigned short PageNumsPerBlk;
	unsigned short BlkPerChip;
	unsigned short ChipNum;
	unsigned short FirstBuild;
	unsigned short new_bad_page_addr;
	unsigned long long FullBitmap;
	struct _nand_super_block mbr_block_addr;
	struct _nand_super_block bad_block_addr;
	struct _nand_super_block new_bad_block_addr;
	struct _nand_super_block no_used_block_addr;
	//struct _bad_block_list*         new_bad_block_addr;
	struct _nand_super_block *factory_bad_block;
	struct _nand_super_block *new_bad_block;
	unsigned char *temp_page_buf;
	unsigned char *mbr_data;
	struct _nand_phy_partition *phy_partition_head;
	struct _partition partition[MAX_PARTITION];
	struct _nand_lib_cfg nand_lib_cfg;
	unsigned short partition_nums;
	unsigned short cache_level;
	unsigned short capacity_level;

	unsigned short mini_free_block_first_reserved;
	unsigned short mini_free_block_reserved;

	unsigned int MaxBlkEraseTimes;
	unsigned int EnableReadReclaim;

	unsigned int read_claim_interval;

	struct _boot_info *boot;
};

//==========================================================
//nand phy partition ���ʽӿ�

struct _nand_phy_partition {
	unsigned short PartitionNO;
	unsigned short CrossTalk;
	unsigned short SectorNumsPerPage;
	unsigned short BytesUserData;
	unsigned short PageNumsPerBlk;
	unsigned short TotalBlkNum; //include bad block
	unsigned short GoodBlockNum;
	unsigned short FullBitmapPerPage;
	unsigned short FreeBlock;
	unsigned int Attribute;
	unsigned int TotalSectors;
	struct _nand_super_block StartBlock;
	struct _nand_super_block EndBlock;
	struct _nand_info *nand_info;
	struct _nand_super_block *factory_bad_block;
	struct _nand_super_block *new_bad_block;
	struct _nand_phy_partition *next_phy_partition;
	struct _nand_disk *disk;

	int (*page_read)(unsigned short nDieNum, unsigned short nBlkNum,
			unsigned short nPage, unsigned short SectBitmap,
			void *pBuf, void *pSpare);
	int (*page_write)(unsigned short nDieNum, unsigned short nBlkNum,
			unsigned short nPage, unsigned short SectBitmap,
			void *pBuf, void *pSpare);
	int (*block_erase)(unsigned short nDieNum, unsigned short nBlkNum);
};

//==========================================================
//nand partition ���ʽӿ�

struct _nand_partition_page {
	unsigned short Page_NO;
	unsigned short Block_NO;
};

struct _physic_par {
	struct _nand_partition_page phy_page;
	unsigned short page_bitmap;
	unsigned char *main_data_addr;
	unsigned char *spare_data_addr;
};

//nand partition
struct _nand_partition {
	char name[32];
	unsigned short sectors_per_page;
	unsigned short spare_bytes;
	unsigned short pages_per_block;
	unsigned short total_blocks;
	unsigned short bytes_per_page;
	unsigned int bytes_per_block;
	unsigned short full_bitmap;
	unsigned long long cap_by_sectors;
	unsigned long long cap_by_bytes;
	unsigned long long total_by_bytes;
	struct _nand_phy_partition *phy_partition;

	int (*nand_erase_superblk)(struct _nand_partition *nand,
			struct _physic_par *p);
	int (*nand_read_page)(struct _nand_partition *nand,
			struct _physic_par *p);
	int (*nand_write_page)(struct _nand_partition *nand,
			struct _physic_par *p);
	int (*nand_is_blk_good)(struct _nand_partition *nand,
			struct _physic_par *p);
	int (*nand_mark_bad_blk)(struct _nand_partition *nand,
			struct _physic_par *p);
};

typedef enum {
	NFTL_SUCCESS	   = 0,
	NFTL_FAILURE	   = 1,
	NFTL_INVALID_PARTITION = 2,
	NFTL_INVALID_ADDRESS   = 3,
	NFTL_FLUSH_ERROR       = 4,
	NFTL_PAGENOTFOUND      = 5,
	NFTL_NO_FREE_BLOCKS    = 6,
	NFTL_NO_INVALID_BLOCKS = 7,
	NFTL_COMPLETE	  = 8,
	NFTL_YES	       = 1,
	NFTL_NO		       = 0,
} _nftl_error;
#endif /*_PHY_H*/
