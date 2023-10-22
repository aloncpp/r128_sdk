/*
 * (C) Copyright 2007-2022
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
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

#include <private_rtos.h>

#define SUNXI_TFM_HEAD_SIZE     ((sizeof(rtos_spare_head)) & 0x00FFFFF)

#define JUMP_OFFSET             ((SUNXI_TFM_HEAD_SIZE - 4) & 0x007fffff)
#define LOW_JUMP                (0xB800 | (JUMP_OFFSET & 0x00000fff) >> 1)
#define HIGH_JUMP               (0xF000 | JUMP_OFFSET >> 12)
#define JUMP_INSTRUCTION        ((HIGH_JUMP) | (LOW_JUMP << 16))

extern const unsigned char __VECTOR_BASE[];

#pragma pack(1)
struct spare_rtos_head_t rtos_spare_head __attribute__((section(".head"))) =
{
	{
		/* jump_instruction */
		JUMP_INSTRUCTION,
		"rtos",
		0,
		0,
		0,
		0,
		"0.0.0",
		"0.0.0",
		(int)__VECTOR_BASE,
	},
};
#pragma pack()

/*******************************************************************************
*
*                  关于Boot_file_head中的jump_instruction字段
*
*  jump_instruction字段存放的是一条跳转指令：( B  BACK_OF_Boot_file_head )，此跳
*转指令被执行后，程序将跳转到Boot_file_head后面第一条指令。
*
*  ARM指令中的B指令编码如下：
*          +--------+---------+------------------------------+
*          | 31--28 | 27--24  |            23--0             |
*          +--------+---------+------------------------------+
*          |  cond  | 1 0 1 0 |        signed_immed_24       |
*          +--------+---------+------------------------------+
*  《ARM Architecture Reference Manual》对于此指令有如下解释：
*  Syntax :
*  B{<cond>}  <target_address>
*    <cond>    Is the condition under which the instruction is executed. If the
*              <cond> is ommitted, the AL(always,its code is 0b1110 )is used.
*    <target_address>
*              Specified the address to branch to. The branch target address is
*              calculated by:
*              1.  Sign-extending the 24-bit signed(wro's complement)immediate
*                  to 32 bits.
*              2.  Shifting the result left two bits.
*              3.  Adding to the contents of the PC, which contains the address
*                  of the branch instruction plus 8.
*
*  由此可知，此指令编码的最高8位为：0b11101010，低24位根据Boot_file_head对齐到字
*的大小动态生成，,另外还需考虑到vector所在段的对齐大小，所以指令的组装过程如下：
*  ALIGN_UP(sizeof(boot_file_head_t), 64)      求出文件头到vector的偏移
*  / sizeof( int )                             占用的“字”的个数
*  - 2                                         减去PC预取的指令条数
*  & 0x00FFFFFF                                求出signed-immed-24
*  | 0xEA000000                                组装成B指令
*
*******************************************************************************/
