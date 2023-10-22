/*
 * main.c for standby.bin
 *
 * Copyright (c) 2022 Allwinner.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#include "type.h"
#include "arch.h"
#include "head.h"
#include "enter.h"

u32 backup[2];
int standby_main(void)
{
	/*the function can't use local variable in stack, so static must exist.*/

	/* save stack pointer registger, switch stack to sram */
	backup[0] =  __get_PSP();
	backup[1] =  __get_PSPLIM();
	__set_PSPLIM((u32)head->stack_limit);
	__set_PSP((u32)head->stack_base);
	__ISB();
	__DSB();

	standby_enter();

	/* restore stack pointer register, switch stack back to dram */
	__set_PSP(backup[0]);
	__set_PSPLIM(backup[1]);
	__ISB();
	__DSB();

	return 0;
}


