/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the People's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.
*
*
* THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
* PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
* WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
* THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
* OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
* IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
* NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
* OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <xtensa/coreasm.h>

#include "xtensa_timer.h"

#define GPRCM_DSP_BOOT_FLAG_REG	(0x40050000 + 0x01cc)
#define GPRCM_DSP_BOOT_ADDR_REG	(0x40050000 + 0x01d0)
#define GPRCM_DSP_BOOT_ARG_REG	(0x40050000 + 0x01d4)
#define XTENSA_INFO_REG_ADDR	(0x40050210)

#define BOOT_FLAG_DEEP_SLEEP (0x429b0001)
#define BOOT_FLAG_COLD_RESET (0x429b0000)

.section .bss
	/* for xtensa architecture, the first argument of .align directive is the alignment request in bytes.
	 * This behavior is different with ARM architecture, so we use the .balign directive.
	 * The first argument of .balign directive is the alignment request in bytes.
	 */
	.balign 4
	.global g_xtensa_sleep_flag
g_xtensa_sleep_flag:
.int 0

	.balign 4
	.global intlevel_store
intlevel_store:
.int 0

/* all general regs(32bit)*/
	.balign 4
	.global g_xtensa_general_regs
g_xtensa_general_regs:
	.space 16 * 4

/* all special regs(32bit) */
	.balign 4
	.global g_xtensa_special_regs
g_xtensa_special_regs:
	.space 256 * 4

#if XCHAL_HAVE_MPU
/* all MPU foreground map(8byte per entry) */
	.balign 4
	.global g_xtensa_mpu_fg_map
g_xtensa_mpu_fg_map:
	.space XCHAL_MPU_ENTRIES * 8
#endif

#if XCHAL_HAVE_CP
/* all register for coprocessor1(HiFi5 AudioEngineLX) */
	.balign XCHAL_CP1_SA_ALIGN
	.global g_xtensa_cp1_regs
g_xtensa_cp1_regs:
	.space XCHAL_CP1_SA_SIZE
#endif

/* save the value of special reg to memeory address(a2 + index * 4)
 * use a2 and a3.
 * a3 is temp reg.
 */
.macro save_special_reg name index
	rsr	a3, \name
	s32i	a3, a2, \index * 4
.endm

/* restore the value of special reg from memeory address(a2 + index * 4).
 * use a2 and a3.
 * a3 is temp reg.
 */
.macro restore_special_reg name index
	l32i	a3, a2, \index * 4
	wsr	a3, \name
.endm

/* load_or_store_general_reg - execute the option instruction for all general registers(except a2 reg).
 * this macro doesn't load or store a2 reg. Because this macro use a2 reg to save base address.
 */
.macro load_or_store_general_reg option
    \option    a0,  a2, 0*4
	\option    a1,  a2, 1*4
	/*\option    a2,  a2, 2*4*/
	\option    a3,  a2, 3*4
	\option    a4,  a2, 4*4
	\option    a5,  a2, 5*4
	\option    a6,  a2, 6*4
	\option    a7,  a2, 7*4
	\option    a8,  a2, 8*4
	\option    a9,  a2, 9*4
	\option    a10,  a2, 10*4
	\option    a11,  a2, 11*4
	\option    a12,  a2, 12*4
	\option    a13,  a2, 13*4
	\option    a14,  a2, 14*4
	\option    a15,  a2, 15*4
.endm


.text

/*******************************************************************************

suspend_xtensa_processor

Save Xtensa processor state and other resume information. This function can be 
called for c/c++ code.

*******************************************************************************/

	.globl suspend_xtensa_processor
	.type  suspend_xtensa_processor, %function
	.balign 4
suspend_xtensa_processor:
	entry	a1, 32 /* The length is 16 byte which is used to save local variables */

	/* use a2 to save base address after save a2 to stack */
	s32i	a2, a1, 0

	/* save general registers except a2 */
	movi	a2, g_xtensa_general_regs
	load_or_store_general_reg s32i

	/* save a2 after other general registers have been saved */
	l32i	a3, a1, 0 /* load orignal content of a2 register form stack */
	s32i	a3, a2, 2*4

	movi	a2, XTENSA_INFO_REG_ADDR
	movi	a3, 0x1
	s32i	a3, a2, 0

	/* save some special registers */
	movi	a2, g_xtensa_special_regs
	save_special_reg LBEG 0
	save_special_reg LEND 1
	save_special_reg LCOUNT 2
	save_special_reg SAR 3
	save_special_reg BR 4
	save_special_reg WindowBase 72
	save_special_reg WindowStart 73

	movi	a2, XTENSA_INFO_REG_ADDR
	movi	a3, 0x2
	s32i	a3, a2, 0

	/* save other special registers */
	movi	a2, g_xtensa_special_regs
	save_special_reg PREFCTL 40
	save_special_reg MEMCTL 97
	save_special_reg INTERRUPT 226
	save_special_reg INTENABLE 228
	save_special_reg PS 230
	save_special_reg VECBASE 231

#ifdef XTENSA_SUSPEND_DEBUG
	movi	a2, XTENSA_INFO_REG_ADDR
	movi	a3, 0x3
	s32i	a3, a2, 0
#endif

#if XCHAL_HAVE_MPU

#if XCHAL_MPU_LOCK
#define WPTLB_INS_AS_REG_BIT_MASK 0xFFFFFFE3
#else
#define WPTLB_INS_AS_REG_BIT_MASK 0xFFFFFFE1
#endif
#define WPTLB_INS_AT_REG_BIT_MASK 0x001FFF1F

#define RPTLB1_INS_RESUTL_BIT_MASK 0x001FFF00

	/* save special registers which is related to MPU */
	movi	a2, g_xtensa_special_regs
	save_special_reg ERACCESS 95
	save_special_reg CACHEADRDIS 98

	/* save MPU foreground map */
	movi	a2, g_xtensa_mpu_fg_map
	movi	a3, XCHAL_MPU_ENTRIES
	addx8	a2, a3, a2
read_mpu_entry_loop:
	addi	a3, a3, -1 /* a3 is the entry id(segment num) */
	addi	a2, a2, -8

	rptlb0	a4, a3
	movi	a5, WPTLB_INS_AS_REG_BIT_MASK
	and a4, a4, a5 /* clear some reserved bit */
	s32i.n	a4, a2, 0

	rptlb1	a4, a3 /* the result of rptlb1 instruction has no segment num field */
	movi	a5, RPTLB1_INS_RESUTL_BIT_MASK
	and a4, a4, a5 /* clear some reserved bit */
	add	a4, a4, a3 /* save segment num */
	movi	a5, WPTLB_INS_AT_REG_BIT_MASK
	and a4, a4, a5 /* clear some reserved bit */
	s32i.n	a4, a2, 4

	bnez	a3, read_mpu_entry_loop
#endif

	movi	a2, XTENSA_INFO_REG_ADDR
	movi	a3, 0x4
	s32i	a3, a2, 0

#if XCHAL_HAVE_CP
	/* save special registers which is related to coprocessor */
	movi	a2, g_xtensa_special_regs
	save_special_reg CPENABLE 224

	/* set CPENABLE[1] to access coprocessor1 */
	movi	a2, 0x2
	wsr.cpenable	a2
	rsync

	/* save coprocessor1 registers */
	movi	a2, g_xtensa_cp1_regs
	movi	a3, 1
	call0 xthal_save_cpregs_nw

	/* recover CPENABLE special reg */
	movi	a2, g_xtensa_special_regs
	restore_special_reg CPENABLE 224
	rsync
#endif

	movi	a2, XTENSA_INFO_REG_ADDR
	movi	a3, 0x5
	s32i	a3, a2, 0

	/* save invisible physical ar registers(used by caller) to stack */
	/* when call xthal_window_spill_nw, LCOUNT should be 0 */
	movi	a3, 0
	wsr.lcount a3

	/* when call xthal_window_spill_nw, PS.WOE shoud be 0 and PS.INTLEVEL >= XCHAL_EXCM_LEVEL */
	movi	a3, ~(PS_WOE_MASK | PS_INTLEVEL_MASK) //0xfffbfff0
	rsr.ps	a2
	s32i	a2, a1, 4
	and	a2, a2, a3
	addi	a2, a2, XCHAL_EXCM_LEVEL
	wsr.ps	a2
	rsync

	call0 xthal_window_spill_nw

	l32i	a2, a1, 4
	wsr.ps	a2
	rsync

	movi	a2, XTENSA_INFO_REG_ADDR
	movi	a3, 0x6
	s32i	a3, a2, 0

	/* recover a0 register which has beed modified by call0 instruction */
	movi	a2, g_xtensa_general_regs
	l32i	a0, a2, 0*4

	/* recover LCOUNT special reg */
	movi	a2, g_xtensa_special_regs
	restore_special_reg LCOUNT 2

	movi	a2, XTENSA_INFO_REG_ADDR
	movi	a3, 0x7
	s32i	a3, a2, 0

	/* set boot flag: from deep sleep */
	movi	a2, GPRCM_DSP_BOOT_FLAG_REG
	movi	a3, BOOT_FLAG_DEEP_SLEEP
	s32i	a3, a2, 0

	/* set boot addr */
	movi	a2, GPRCM_DSP_BOOT_ADDR_REG
	movi	a3, resume_xtensa_processor_nw
	s32i	a3, a2, 0

	/* set boot arg */
	movi	a2, GPRCM_DSP_BOOT_ARG_REG
	movi	a3, g_xtensa_general_regs
	s32i	a3, a2, 0

	/* update the sleep flag */
	movi	a2, g_xtensa_sleep_flag
	movi	a3, 1
	s32i	a3, a2, 0

	movi	a2, XTENSA_INFO_REG_ADDR
	movi	a3, 0xa
	s32i	a3, a2, 0

	/* no need to recover general registers, because the windowed register mechanism */
	retw


/*******************************************************************************

resume_xtensa_processor_nw

    !! MUST BE CALLED ONLY BY 'CALL0/CALLX0' INSTRUCTION !!

Restore Xtensa processor state and clear g_xtensa_sleep_flag. This function should
be excuted after processor reset.

*******************************************************************************/

	.globl resume_xtensa_processor_nw
	.type  resume_xtensa_processor_nw, %function
	.balign 4
resume_xtensa_processor_nw:

	/* restore VECBASE special reg */
	movi	a2, g_xtensa_special_regs
	restore_special_reg VECBASE 231

	/* update the resume information */
	movi	a2, XTENSA_INFO_REG_ADDR
	movi	a3, 0x11
	s32i	a3, a2, 0

	/* set INTENABLE special reg */
	movi	a2, 0
	wsr.intenable	a2
	rsync

	/* set PS special reg(PS.WOE = 0, RING = 0, INTLEVE = XCHAL_EXCM_LEVEL) */
	movi	a3, ~(PS_WOE_MASK | PS_RING_MASK | PS_INTLEVEL_MASK)
	rsr.ps	a2
	and	a2, a2, a3
	addi	a2, a2, XCHAL_EXCM_LEVEL
	wsr.ps	a2
	isync

	movi	a2, XTENSA_INFO_REG_ADDR
	movi	a3, 0x12
	s32i	a3, a2, 0

	movi	a2, g_xtensa_special_regs
	restore_special_reg MEMCTL 97

	movi	a2, XTENSA_INFO_REG_ADDR
	movi	a3, 0x13
	s32i	a3, a2, 0

	movi	a2, g_xtensa_special_regs
	restore_special_reg PREFCTL 40

	movi	a2, XTENSA_INFO_REG_ADDR
	movi	a3, 0x14
	s32i	a3, a2, 0

#if XCHAL_HAVE_MPU
	/* restore MPU foreground map */
	movi	a2, g_xtensa_mpu_fg_map
	movi	a3, XCHAL_MPU_ENTRIES
	addx8	a2, a3, a2
write_mpu_entry_loop:
	addi	a3, a3, -1
	addi	a2, a2, -8

	l32i	a4, a2, 0
	l32i	a5, a2, 4
	wptlb	a5, a4 /* WPTLB at, as */

	bnez	a3, write_mpu_entry_loop

	/* restore special registers which is related to MPU */
	movi	a2, g_xtensa_special_regs
	restore_special_reg ERACCESS 95
	restore_special_reg CACHEADRDIS 98
#endif

	movi	a2, XTENSA_INFO_REG_ADDR
	movi	a3, 0x15
	s32i	a3, a2, 0

	/* restore special registers */
	movi	a2, g_xtensa_special_regs
	restore_special_reg LBEG 0
	restore_special_reg LEND 1
	restore_special_reg LCOUNT 2
	restore_special_reg SAR 3
	restore_special_reg BR 4

	movi	a2, XTENSA_INFO_REG_ADDR
	movi	a3, 0x16
	s32i	a3, a2, 0

#if XCHAL_HAVE_CP
	/* set CPENABLE[1] to access coprocessor1 */
	movi	a2, 0x2
	wsr.cpenable	a2
	rsync

	/* restore coprocessor1 registers */
	movi	a2, g_xtensa_cp1_regs
	movi	a3, 1
	call0 xthal_restore_cpregs_nw

	/* restore special registers which is related to coprocessor */
	movi	a2, g_xtensa_special_regs
	restore_special_reg CPENABLE 224
	rsync
#endif

	/* no need to recover WindowBase and WindowStart special reg */
	//movi	a2, g_xtensa_special_regs
	//restore_special_reg WindowBase 72
	//restore_special_reg WindowStart 73

	/* init WindowBase and WindowStart special reg */
	movi a1, 1
	movi a0, 0
	wsr.windowstart a1
	wsr.windowbase a0
	rsync

	movi	a2, XTENSA_INFO_REG_ADDR
	movi	a3, 0x17
	s32i	a3, a2, 0

	/* set boot flag: from cold reset */
	movi	a2, GPRCM_DSP_BOOT_FLAG_REG
	movi	a3, BOOT_FLAG_COLD_RESET
	s32i	a3, a2, 0

	/* update the sleep flag */
	movi	a2, g_xtensa_sleep_flag
	movi	a3, 0
	s32i	a3, a2, 0

	/* restore general registers except a2 */
	movi	a2, g_xtensa_general_regs
	load_or_store_general_reg l32i

	/* save a2 and a3 to stack */
	s32i	a3, a1, 0
	movi	a3, g_xtensa_general_regs
	l32i	a2, a3, 2*4
	s32i	a2, a1, 4

	movi	a2, XTENSA_INFO_REG_ADDR
	movi	a3, 0x18
	s32i	a3, a2, 0

	/* Update the timer comparator for the next tick. */
#ifdef XT_CLOCK_FREQ
	movi	a2, XT_TICK_DIVISOR
#else
	movi	a3, _xt_tick_divisor
	l32i	a2, a3, 0
#endif
	rsr	a3, XT_CCOMPARE
	add	a4, a3, a2
	wsr	a4, XT_CCOMPARE
	esync

	movi	a2, g_xtensa_special_regs
	restore_special_reg INTSET 226
	rsync
	restore_special_reg INTENABLE 228
	rsync
	restore_special_reg PS 230
	isync

	movi	a2, XTENSA_INFO_REG_ADDR
	movi	a3, 0x1a
	s32i	a3, a2, 0

	/* restore a2 and a3 */
	l32i	a2, a1, 4
	l32i	a3, a1, 0

	retw



	.globl _suspend_save_intlevel
	.type  _suspend_save_intlevel, %function
	.balign 4
_suspend_save_intlevel:
	entry	a1, 32

	movi	a3, PS_INTLEVEL_MASK
	rsr.ps	a2
	and	a2, a2, a3
	movi	a4, intlevel_store
	s32i	a2, a4, 0

	retw



	.globl _resume_restore_intlevel
	.type  _resume_restore_intlevel, %function
	.balign 4
_resume_restore_intlevel:
	entry	a1, 32

	movi	a3, ~(PS_INTLEVEL_MASK)
	rsr.ps	a2
	and	a2, a2, a3
	movi	a4, intlevel_store
	l32i	a3, a4, 0
	or	a2, a2, a3
	wsr.ps	a2
	isync

	retw
