/*
 * (C) Copyright 2007-2013
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Jerry Wang <wangflord@allwinnertech.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef  __private_rtos_h__
#define  __private_rtos_h__

/******************************************************************************/
/*               the control information stored in file head                  */
/******************************************************************************/
struct spare_rtos_ctrl_head
{
	unsigned int  jump_instruction;   /* one intruction jumping to real code */
	unsigned char magic[8];           /* ="rtos" */
	unsigned int  res1;
	unsigned int  res2;
	unsigned int  res3;
	unsigned int  res4;
	unsigned char res5[8];
	unsigned char res6[8];
	int           run_addr;           /* rtos runs the address in memory */
};

typedef struct spare_rtos_head_t
{
	struct spare_rtos_ctrl_head    rtos_head;
	uint8_t rotpk_hash[32];
	unsigned int rtos_dram_size;    /* rtos dram size, passed by boot0*/
	unsigned int reserved[11];	/* padding to 128 Bytes */
					/* reserved[0] rv_entry */
					/* reserved[1] dsp_entry */
}rtos_head_t;

extern struct spare_rtos_head_t  rtos_spare_head;

#endif
