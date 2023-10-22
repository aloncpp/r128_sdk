/*
 *  drivers/standby/head.c
 *
 * Copyright (c) 2018 Allwinner.
 * 2018-09-14 Written by fanqinghua (fanqinghua@allwinnertech.com).
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY, without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "type.h"
#include "head.h"
#include "link.h"

extern int standby_main(void);

char lpsram_name[] = "NONE";
uint8_t lpsram_resp;
uint8_t lpsram_buff;
struct psram_request lpsram_mrq = {
	.cmd.resp = &lpsram_resp,
	.data.buff = &lpsram_buff,
};
struct psram_ctrl ctrl = {
	.mrq = &lpsram_mrq,
};

__attribute__((section(".standby_head"))) standby_head_t  g_standby_head =
{
	.version      = HEAD_VERSION,

	.build_info   = {
		.commit_id    = HEAD_COMMIT_ID,
		.change_id    = HEAD_CHANGE_ID,
		.build_date   = HEAD_BUILD_DATE,
		.build_author = HEAD_BUILD_AUTHOR,
	},

	.enter = standby_main,
	.paras_start =  &_standby_paras_start,
	.paras_end   =  &_standby_paras_end,
	.code_start  =  &_standby_code_start,
	.code_end    =  &_standby_code_end,
	.bss_start   =  &_standby_bss_start,
	.bss_end     =  &_standby_bss_end,
	.stack_limit =  &_standby_stack_limit,
	.stack_base  =  &_standby_stack_base,

	.stage_record = 1,

	.hpsram_inited = 1,
	.lpsram_inited = 1,

	.lpsram_para.name = lpsram_name,
	.lpsram_para.ctrl = &ctrl,

	.lpsram_crc = {
		.crc_start  = 0x08000000,
		.crc_len    = 0x00800000,
		.crc_enable = 1,
		.crc_before = 0,
		.crc_after  = 0,
	},
	.hpsram_crc = {
		.crc_start  = 0x0c000000,
		.crc_len    = 0x02000000,
		.crc_enable = 0,
		.crc_before = 0,
		.crc_after  = 0,
	},

	.mode = 0,
	.wakesrc_active = 0,
	.wakeup_source = 0,
	.time_to_wakeup_ms = 0,

	.pwrcfg = 0,
	.clkcfg = 0,
	.anacfg = 0,

	.suspend_moment = 0,
	.resume_moment = 0,
};

standby_head_t *head = (standby_head_t *)&g_standby_head;

