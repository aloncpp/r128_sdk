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

#include <common.h>
#include <private_uboot.h>

/*when nvic register 0xe000ed08 is align to 128, we only can use 16 interrupts.
* but, we use 128 interrupts. so, align to 1024 is better.
* BROM_FILE_HEAD_SIZE is 1600, so add 448 here
*/
#define BROM_FILE_HEAD_SIZE		((sizeof(struct spare_boot_head_t) + 448) & 0x00FFFFF)

#define JUMP_OFFSET		((BROM_FILE_HEAD_SIZE - 4) & 0x007fffff)
#define LOW_JUMP		(0xF800 | (JUMP_OFFSET & 0x00000fff) >> 1)
#define HIGH_JUMP		(0xF000 | JUMP_OFFSET >> 12)
#define JUMP_INSTRUCTION	((HIGH_JUMP) | (LOW_JUMP << 16))

#pragma pack(1)
struct spare_boot_head_t  uboot_spare_head =
{
    {
        /* jump_instruction */
	JUMP_INSTRUCTION,
        UBOOT_MAGIC,
        STAMP_VALUE,
        ALIGN_SIZE,
        0,
        0,
        UBOOT_VERSION,
        UBOOT_PLATFORM,
        {CONFIG_SYS_TEXT_BASE}
    },
    {
        { 0 },		//dram para
        1008,			//run core clock
        1200,			//run core vol
        0,			//uart port
        {             //uart gpio
            {0}, {0}
        },
        0,			//twi port
        {             //twi gpio
		{0}, {0}
        },
        0,			//work mode
        0,			//storage mode
        { {0} },		//nand gpio
	{ 0 },		//nand info
        { {0} },		//sdcard gpio
	{ 0 }, 		//sdcard info
        0,                          //secure os
        0,                          //monitor
	0,                        /* see enum UBOOT_FUNC_MASK_EN */
	{0},						//reserved data
        UBOOT_START_SECTOR_IN_SDMMC, //OTA flag
        0,                           //dtb offset
        0,                           //boot_package_size
		0,							//dram_scan_size
	{0}			//reserved data
    },

};

#pragma pack()
