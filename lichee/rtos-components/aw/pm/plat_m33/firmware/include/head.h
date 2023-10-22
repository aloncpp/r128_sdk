/*
 * Copyright (c) 2022 Allwinner.
 *
 */
#ifndef _FW_STANDBY_HEAD_
#define _FW_STANDBY_HEAD_

/*
 * 2022/11/04 ver:0.1.2
 * update psram para
 * 2022/11/18 ver:0.1.3
 * add stage_record
 * 2023/03/28 ver:0.1.4
 * add hpsram para
 * 2023/04/28 ver:0.1.5
 * add suspend/resume moment
 */
#define HEAD_MAJOR_VERSION   0U
#define HEAD_MINOR_VERSION   1U
#define HEAD_REVISION_NUMBER 4U

#define HEAD_VERSION (HEAD_MAJOR_VERSION<<16|HEAD_MINOR_VERSION<<8|HEAD_REVISION_NUMBER)

#ifdef EXT_HEAD_COMMIT_ID
#define HEAD_COMMIT_ID EXT_HEAD_COMMIT_ID
#else
#define HEAD_COMMIT_ID "unkown"
#endif

#ifdef  EXT_HEAD_CHANGE_ID
#define HEAD_CHANGE_ID EXT_HEAD_CHANGE_ID
#else
#define HEAD_CHANGE_ID "unkown"
#endif

#ifdef EXT_HEAD_BUILD_DATE
#define HEAD_BUILD_DATE EXT_HEAD_BUILD_DATE
#else
#define HEAD_BUILD_DATE "unkown"
#endif

#ifdef EXT_HEAD_BUILD_AUTHOR
#define HEAD_BUILD_AUTHOR EXT_HEAD_BUILD_AUTHOR
#else
#define HEAD_BUILD_AUTHOR "unkown"
#endif

typedef struct build_info {
	unsigned char commit_id[48];
	unsigned char change_id[48];
	unsigned char build_date[32];
	unsigned char build_author[32];
} build_info_t;


typedef struct crc_info {
	unsigned int crc_start;
	unsigned int crc_len;
	unsigned int crc_enable;
	unsigned int crc_before;
	unsigned int crc_after;
} crc_info_t;

/* lpsram_para */
struct psram_command {
	uint32_t opcode;
	uint32_t addr;
	uint8_t *resp;
	uint32_t rw_cfg;
	uint32_t flags;
};

struct psram_data {
	uint32_t blksz;
	uint32_t blocks;
	uint32_t flags;
	uint32_t rw_cfg;
	uint8_t *buff;
};

struct psram_request {
	struct psram_command 	cmd;
	struct psram_data	data;
};

struct psram_ctrl {
	volatile uint32_t rd_buf_idx;
	volatile uint32_t Psram_WR_FULL;
	volatile uint32_t wait;

	//OS_Semaphore_t lock;
	uint32_t status_int;
	uint32_t inte;
	uint32_t trans_done;
	uint32_t dma_done;

	//OS_Semaphore_t dmaSem;
	//DMA_ChannelInitParam dmaParam;
	//DMA_Channel dma_ch;
	//uint8_t dma_use;
	uint8_t ref;

	uint32_t busconfig;

	uint32_t p_type;        /* psram type */
	uint32_t freq;          /* psram freq */
	uint8_t rdata_w;
	struct psram_request *mrq;
};

typedef struct psram_chip {
	uint8_t			id;
	uint8_t			type;

	uint8_t                 cbus_rcmd;
	uint8_t                 cbus_wcmd;
	uint8_t			mf_id;
	uint8_t                 kgd;

	uint32_t                die;

	uint32_t		buswidth;
	uint32_t                wrap_len;
	uint32_t                capacity;
	uint32_t                freq;
	char                    *name;
	struct psram_ctrl       *ctrl;
	uint32_t                dqs;
} lpsram_para_t;
/* lpsram_para end */

/* hpsram_para */
typedef struct __DRAM_PARA
{
	/* normal configuration */
	unsigned int		dram_clk;
	unsigned int		dram_type;
	unsigned int		dram_zq;
	unsigned int		dram_odt_en;

	/* control configuration */
	unsigned int		dram_para1;
	unsigned int		dram_para2;

	//timing configuration
	unsigned int		dram_mr0;
	unsigned int		dram_mr1;
	unsigned int		dram_mr2;
	unsigned int		dram_mr3;
	unsigned int		dram_tpr0;
	unsigned int		dram_tpr1;
	unsigned int		dram_tpr2;
	unsigned int		dram_tpr3;
	unsigned int		dram_tpr4;
	unsigned int		dram_tpr5;
	unsigned int		dram_tpr6;

	/* reserved */
	unsigned int		dram_tpr7;
	unsigned int		dram_tpr8;
	unsigned int		dram_tpr9;
	unsigned int		dram_tpr10;
	unsigned int		dram_tpr11;
	unsigned int		dram_tpr12;
	unsigned int		dram_tpr13;
}__dram_para_t;
/* hpsram_para end */

typedef struct standby_head {
	/* head version, in order to compatible.*/
	unsigned int  version;
	unsigned int  reserved[3];

	build_info_t build_info;

	void *enter;
	void *paras_start;
	void *paras_end;
	void *code_start;
	void *code_end;
	void *bss_start;
	void *bss_end;
	void *stack_limit;
	void *stack_base;

	uint8_t stage_record;

	uint8_t lpsram_inited;
	uint8_t hpsram_inited;
	lpsram_para_t lpsram_para;
	__dram_para_t hpsram_para;
	crc_info_t lpsram_crc;
	crc_info_t hpsram_crc;

	unsigned int mode;
	unsigned int wakesrc_active;
	unsigned int wakeup_source;
	unsigned int time_to_wakeup_ms; // use SRC_WKTMR

	// need active
	unsigned int pwrcfg;
	unsigned int clkcfg;
	unsigned int anacfg;

	uint64_t suspend_moment;
	uint64_t resume_moment;
} standby_head_t;

extern standby_head_t *head;

#endif

