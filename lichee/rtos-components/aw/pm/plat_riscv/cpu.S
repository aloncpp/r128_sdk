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

#define GPRCM_RV_BOOT_FLAG_REG	(0x40050000 + 0x01d8)
#define GPRCM_RV_BOOT_ADDR_REG	(0x40050000 + 0x01dc)
#define GPRCM_RV_BOOT_ARG_REG	(0x40050000 + 0x01e0)
#define RV_RECREG		(0x4005020c)

.section .bss
	.align 2
	.globl rv_regs
	.globl rv_sleep_flag

/* all regs 229 * 8*/
rv_regs:
	.space 1832

rv_sleep_flag:
.long 0

/* t0 is used to transfer value */
/* save the ctrol status reg value to index * 8(t0) */
.macro save_csr name index
	csrr	t1, \name
	sd	t1, \index * 8(t0)
.endm

/* restore the csr value to index */
.macro restore_csr name index
	ld	t1, \index * 8(t0)
	csrw	\name, t1
.endm

/* general_reg - execute the option instruction for all general registers(32 * 64bit).
 * Load base to t0, and then excute the option. 
 * Move base to (base + regs_num * byte at last.
 */
.macro general_reg option base
	la		t0, \base
	\option		x0,  0*8(t0)
	\option		ra,  1*8(t0)
	\option		sp,  2*8(t0)
	\option		gp,  3*8(t0)
	\option		tp,  4*8(t0)

	/* \option		t0,  5*8(t0) #t0 in use, save in the final */
	\option		t1,  6*8(t0)
	\option		t2,  7*8(t0)

	\option		s0,  8*8(t0)
	\option		s1,  9*8(t0)

	\option		a0,  10*8(t0)
	\option		a1,  11*8(t0)
	\option		a2,  12*8(t0)
	\option		a3,  13*8(t0)
	\option		a4,  14*8(t0)
	\option		a5,  15*8(t0)
	\option		a6,  16*8(t0)
	\option		a7,  17*8(t0)

	\option		s2,  18*8(t0)
	\option		s3,  19*8(t0)
	\option		s4,  20*8(t0)
	\option		s5,  21*8(t0)
	\option		s6,  22*8(t0)
	\option		s7,  23*8(t0)
	\option		s8,  24*8(t0)
	\option		s9,  25*8(t0)
	\option		s10, 26*8(t0)
	\option		s11, 27*8(t0)

	\option		t3,  28*8(t0)
	\option		t4,  29*8(t0)
	\option		t5,  30*8(t0)
	\option		t6,  31*8(t0)
	addi	t0, t0, 32*8
.endm

/* float_reg - execute the option instruction for all float registers(32 * 64bit).
 * Move base to (base + reg_nums * byte) at last.
 */
.macro float_reg option base
	\option		ft0,  0*8(\base)
	\option		ft1,  1*8(\base)
	\option		ft2,  2*8(\base)
	\option		ft3,  3*8(\base)
	\option		ft4,  4*8(\base)
	\option		ft5,  5*8(\base)
	\option		ft6,  6*8(\base)
	\option		ft7,  7*8(\base)

	\option		fs0,  8*8(\base)
	\option		fs1,  9*8(\base)

	\option		fa0,  10*8(\base)
	\option		fa1,  11*8(\base)
	\option		fa2,  12*8(\base)
	\option		fa3,  13*8(\base)
	\option		fa4,  14*8(\base)
	\option		fa5,  15*8(\base)
	\option		fa6,  16*8(\base)
	\option		fa7,  17*8(\base)

	\option		fs2,  18*8(\base)
	\option		fs3,  19*8(\base)
	\option		fs4,  20*8(\base)
	\option		fs5,  21*8(\base)
	\option		fs6,  22*8(\base)
	\option		fs7,  23*8(\base)
	\option		fs8,  24*8(\base)
	\option		fs9,  25*8(\base)
	\option		fs10, 26*8(\base)
	\option		fs11, 27*8(\base)

	\option		ft8,  28*8(\base)
	\option		ft9,  29*8(\base)
	\option		ft10, 30*8(\base)
	\option		ft11, 31*8(\base)
	addi	\base, \base, 32* 8
.endm

.text
	.align 2
	.globl _rv_cpu_save_boot_flag
	.type  _rv_cpu_save_boot_flag, %function
_rv_cpu_save_boot_flag:
	sd	t0, -8(sp)
	sd	t1, -16(sp)

	/* set boot flag: from deep sleep */
	li	t0, GPRCM_RV_BOOT_FLAG_REG
	li	t1, 0x429b0001
	sw	t1, 0(t0)
	/* set boot addr */
	li	t0, GPRCM_RV_BOOT_ADDR_REG
	la	t1, _rv_cpu_resume
	sw	t1, 0(t0)
	/* set boot arg */
	li	t0, GPRCM_RV_BOOT_ARG_REG
	la	t1, rv_regs
	sw	t1, 0(t0)

	ld	t0, -8(sp)
	ld	t1, -16(sp)

	ret

.text
	.align 2
	.globl _rv_cpu_suspend
	.type  _rv_cpu_suspend, %function
_rv_cpu_suspend:
	/* save t0 and t1 value to stack, they will be used next */
	sd	t0, -8(sp)
	sd	t1, -16(sp)

	/* get t0 value from stack and save it to reserve addr in general_reg */
	la	t0, rv_regs
	ld	t1, -8(sp)
	sd	t1, 5*8(t0)
	ld	t1, -16(sp)
	/* save general regs to the boot arg addr except t0 */
	general_reg sd rv_regs

	/* save float regs */
	float_reg fsd t0

	/* save float status regs */

	/* === Standard M-mode(Machine Mode) Control Registers === */
	/* save M-mode Exception Setting Registers */
	save_csr mstatus	0
	save_csr misa		1
	save_csr medeleg	2
	save_csr mideleg	3
	save_csr mie		4
	save_csr mtvec		5
	save_csr mcounteren	6
	addi t0, t0, 7*8
	/* save M-mode Exception Handling registers */
	save_csr mscratch	0
	save_csr mepc		1
	save_csr mcause		2
	save_csr mtval		3
	save_csr mip		4
	addi t0, t0, 5*8
	/* save M-mode Memory Protect Registers */
	save_csr pmpcfg0	0
	save_csr pmpcfg2	1
	save_csr pmpaddr0	2
	save_csr pmpaddr1	3
	save_csr pmpaddr2	4
	save_csr pmpaddr3	5
	save_csr pmpaddr4	6
	save_csr pmpaddr5	7
	save_csr pmpaddr6	8
	save_csr pmpaddr7	9
	save_csr pmpaddr8	10
	save_csr pmpaddr9	11
	save_csr pmpaddr10	12
	save_csr pmpaddr11	13
	save_csr pmpaddr12	14
	save_csr pmpaddr13	15
	save_csr pmpaddr14	16
	save_csr pmpaddr15	17
	addi t0, t0, 18*8
	/* save M-mode Counter Registers */
	save_csr mcycle		0
	save_csr minstret	1
	save_csr mhpmcounter3	2
	save_csr mhpmcounter4	3
	save_csr mhpmcounter5	4
	save_csr mhpmcounter6	5
	save_csr mhpmcounter7	6
	save_csr mhpmcounter8	7
	save_csr mhpmcounter9	8
	save_csr mhpmcounter10	9
	save_csr mhpmcounter11	10
	save_csr mhpmcounter12	11
	save_csr mhpmcounter13	12
	save_csr mhpmcounter14	13
	save_csr mhpmcounter15	14
	save_csr mhpmcounter16	15
	save_csr mhpmcounter17	16
	save_csr mhpmcounter18	17
	save_csr mhpmcounter19	18
	save_csr mhpmcounter20	19
	save_csr mhpmcounter21	20
	save_csr mhpmcounter22	21
	save_csr mhpmcounter23	22
	save_csr mhpmcounter24	23
	save_csr mhpmcounter25	24
	save_csr mhpmcounter26	25
	save_csr mhpmcounter27	26
	save_csr mhpmcounter28	27
	save_csr mhpmcounter29	28
	save_csr mhpmcounter30	29
	save_csr mhpmcounter31	30
	addi t0, t0, 31*8
	/* save M-mode Counter Config Registers */
	save_csr mhpmevent3	0
	save_csr mhpmevent4	1
	save_csr mhpmevent5	2
	save_csr mhpmevent6	3
	save_csr mhpmevent7	4
	save_csr mhpmevent8	5
	save_csr mhpmevent9	6
	save_csr mhpmevent10	7
	save_csr mhpmevent11	8
	save_csr mhpmevent12	9
	save_csr mhpmevent13	10
	save_csr mhpmevent14	11
	save_csr mhpmevent15	12
	save_csr mhpmevent16	13
	save_csr mhpmevent17	14
	save_csr mhpmevent18	15
	save_csr mhpmevent19	16
	save_csr mhpmevent20	17
	save_csr mhpmevent21	18
	save_csr mhpmevent22	19
	save_csr mhpmevent23	20
	save_csr mhpmevent24	21
	save_csr mhpmevent25	22
	save_csr mhpmevent26	23
	save_csr mhpmevent27	24
	save_csr mhpmevent28	25
	save_csr mhpmevent29	26
	save_csr mhpmevent30	27
	save_csr mhpmevent31	28
	addi t0, t0, 29*8

	/* === Extend M-mode Control Registers === */
	/* M-mode Processor Control and Status Extend Registers */
	save_csr mcountinhibit	0
	save_csr mxstatus	1
	save_csr mhcr		2
	save_csr mcor		3
	save_csr mhint		4
	save_csr mrvbr		5
	save_csr mcer		6
	save_csr mcounterwen	7
	save_csr mcounterinten	8
	save_csr mcounterof	9
	save_csr mhpmcr		10
	//save_csr mhpmsp		11
	//save_csr mhpmep		12
	addi t0, t0, 13*8
	/* M-mode Cache Access Extend Registers */
	save_csr mcins		0
	save_csr mcindex	1
	save_csr mcdata0	2
	save_csr mcdata1	3
	addi t0, t0, 4*8

	/* === Standard S-mode(Super User Mode) Control Registers === */
	/* save S-mode Exception Config Registers */
	save_csr sstatus	0
	save_csr sie		1
	save_csr stvec		2
	save_csr scounteren	3
	addi t0, t0, 4*8
	/* save S-mode exception handling Registers */
	save_csr sscratch	0
	save_csr sepc		1
	save_csr scause		2
	save_csr stval		3
	save_csr sip		4
	addi t0, t0, 5*8
	/* save s-mode Address Transfer Register */
	save_csr satp		0
	addi t0, t0, 1*8

	/* === Extend S-mode Control Registers === */
	/* save S-mode Process Control and Status Extend Registers */
	save_csr sxstatus	0
	save_csr shcr		1
	save_csr scounterinten	2
	save_csr scounterof	3
	//save_csr scountinhibit	4
	save_csr shpmcr		5
	//save_csr shpmsp		6
	//save_csr shpmep		7
	save_csr scycle		8
	//save_csr sinstret	9
	save_csr shpmcounter3	10
	save_csr shpmcounter4	11
	save_csr shpmcounter5	12
	save_csr shpmcounter6	13
	save_csr shpmcounter7	14
	save_csr shpmcounter8	15
	save_csr shpmcounter9	16
	save_csr shpmcounter10	17
	save_csr shpmcounter11	18
	save_csr shpmcounter12	19
	save_csr shpmcounter13	20
	save_csr shpmcounter14	21
	save_csr shpmcounter15	22
	save_csr shpmcounter16	23
	save_csr shpmcounter17	24
	save_csr shpmcounter18	25
	save_csr shpmcounter19	26
	save_csr shpmcounter20	27
	save_csr shpmcounter21	28
	save_csr shpmcounter22	29
	save_csr shpmcounter23	30
	save_csr shpmcounter24	31
	save_csr shpmcounter25	32
	save_csr shpmcounter26	33
	save_csr shpmcounter27	34
	save_csr shpmcounter28	35
	save_csr shpmcounter29	36
	save_csr shpmcounter30	37
	save_csr shpmcounter31	38
	addi t0, t0, 39*8

	/* === Standard U-mode(User mode) === */
	/* save U-mode float Control Registers(with extend control regitster fxcr) */
	save_csr fflags		0
	save_csr frm		1
	save_csr fcsr		2
	save_csr fxcr		3
	addi t0, t0, 4*8

	/* save U-mode counter regitsters(do not need to save, read only) */

	/* Vector Extend Registers */
	//save_csr vstart		0
	save_csr vxsat		1
	save_csr vxrm		2
	addi t0, t0, 6*8

	/* update the sleep flag */
	la	t0, rv_sleep_flag
	li	t1, 1
	sw	t1, 0(t0)

	/* restore the scene */
	general_reg ld rv_regs
	ld 	t0, -8(sp)

	ret

	.align 2
	.globl _rv_cpu_resume
	.type  _rv_cpu_resume, %function
_rv_cpu_resume:
	li	t0, RV_RECREG
	li	t1, 0xaabb
	sw	t1, 0(t0)

	/* remove bootflag */
	li	t0, GPRCM_RV_BOOT_FLAG_REG
	li	t1, 0x429b0000
	sw	t1, 0(t0)

	/* load regs arg addr to t0. skip general/float regs, which will be restored later. usr t0 and t1 for other regs restore */
	la	t0, rv_regs
	addi	t0, t0, 64*8

	/* === Standard M-mode(Machine Mode) Control Registers === */
	/* restore M-mode Exception Setting Registers */
	restore_csr mstatus		0
	restore_csr misa		1
	restore_csr medeleg		2
	restore_csr mideleg		3
	restore_csr mie			4
	restore_csr mtvec		5
	restore_csr mcounteren		6
	addi t0, t0, 7*8
	/* restore M-mode Exception Handling registers */
	restore_csr mscratch		0
	restore_csr mepc		1
	restore_csr mcause		2
	restore_csr mtval		3
	restore_csr mip			4
	addi t0, t0, 5*8
	/* restore M-mode Memory Protect Registers */
	restore_csr pmpcfg0		0
	restore_csr pmpcfg2		1
	restore_csr pmpaddr0		2
	restore_csr pmpaddr1		3
	restore_csr pmpaddr2		4
	restore_csr pmpaddr3		5
	restore_csr pmpaddr4		6
	restore_csr pmpaddr5		7
	restore_csr pmpaddr6		8
	restore_csr pmpaddr7		9
	restore_csr pmpaddr8		10
	restore_csr pmpaddr9		11
	restore_csr pmpaddr10		12
	restore_csr pmpaddr11		13
	restore_csr pmpaddr12		14
	restore_csr pmpaddr13		15
	restore_csr pmpaddr14		16
	restore_csr pmpaddr15		17
	addi t0, t0, 18*8
	/* restore M-mode Counter Registers */
	restore_csr mcycle		0
	restore_csr minstret		1
	restore_csr mhpmcounter3	2
	restore_csr mhpmcounter4	3
	restore_csr mhpmcounter5	4
	restore_csr mhpmcounter6	5
	restore_csr mhpmcounter7	6
	restore_csr mhpmcounter8	7
	restore_csr mhpmcounter9	8
	restore_csr mhpmcounter10	9
	restore_csr mhpmcounter11	10
	restore_csr mhpmcounter12	11
	restore_csr mhpmcounter13	12
	restore_csr mhpmcounter14	13
	restore_csr mhpmcounter15	14
	restore_csr mhpmcounter16	15
	restore_csr mhpmcounter17	16
	restore_csr mhpmcounter18	17
	restore_csr mhpmcounter19	18
	restore_csr mhpmcounter20	19
	restore_csr mhpmcounter21	20
	restore_csr mhpmcounter22	21
	restore_csr mhpmcounter23	22
	restore_csr mhpmcounter24	23
	restore_csr mhpmcounter25	24
	restore_csr mhpmcounter26	25
	restore_csr mhpmcounter27	26
	restore_csr mhpmcounter28	27
	restore_csr mhpmcounter29	28
	restore_csr mhpmcounter30	29
	restore_csr mhpmcounter31	30
	addi t0, t0, 31*8
	/* restore M-mode Counter Config Registers */
	restore_csr mhpmevent3		0
	restore_csr mhpmevent4		1
	restore_csr mhpmevent5		2
	restore_csr mhpmevent6		3
	restore_csr mhpmevent7		4
	restore_csr mhpmevent8		5
	restore_csr mhpmevent9		6
	restore_csr mhpmevent10		7
	restore_csr mhpmevent11		8
	restore_csr mhpmevent12		9
	restore_csr mhpmevent13		10
	restore_csr mhpmevent14		11
	restore_csr mhpmevent15		12
	restore_csr mhpmevent16		13
	restore_csr mhpmevent17		14
	restore_csr mhpmevent18		15
	restore_csr mhpmevent19		16
	restore_csr mhpmevent20		17
	restore_csr mhpmevent21		18
	restore_csr mhpmevent22		19
	restore_csr mhpmevent23		20
	restore_csr mhpmevent24		21
	restore_csr mhpmevent25		22
	restore_csr mhpmevent26		23
	restore_csr mhpmevent27		24
	restore_csr mhpmevent28		25
	restore_csr mhpmevent29		26
	restore_csr mhpmevent30		27
	restore_csr mhpmevent31		28
	addi t0, t0, 29*8

	li	t2, RV_RECREG
	li	t1, 0x1
	sw	t1, 0(t2)

	/* === Extend M-mode Control Registers === */
	/* M-mode Processor Control and Status Extend Registers */
	restore_csr mcountinhibit	0
	restore_csr mxstatus		1
	restore_csr mhcr		2
	restore_csr mcor		3
	restore_csr mhint		4
	restore_csr mrvbr		5
	restore_csr mcer		6
	restore_csr mcounterwen		7
	restore_csr mcounterinten	8
	restore_csr mcounterof		9
	restore_csr mhpmcr		10
	//restore_csr mhpmsp		11
	//restore_csr mhpmep		12
	addi t0, t0, 13*8
	/* M-mode Cache Access Extend Registers */
	restore_csr mcins		0
	restore_csr mcindex		1
	restore_csr mcdata0		2
	restore_csr mcdata1		3
	addi t0, t0, 4*8

	li	t2, RV_RECREG
	li	t1, 0x2
	sw	t1, 0(t2)

	/* === Standard S-mode(Super User Mode) Control Registers === */
	/* restore S-mode Exception Config Registers */
	restore_csr sstatus		0
	restore_csr sie			1
	restore_csr stvec		2
	restore_csr scounteren		3
	addi t0, t0, 4*8
	/* restore S-mode exception handling Registers */
	restore_csr sscratch		0
	restore_csr sepc		1
	restore_csr scause		2
	restore_csr stval		3
	restore_csr sip			4
	addi t0, t0, 5*8
	/* restore s-mode Address Transfer Register */
	restore_csr satp		0
	addi t0, t0, 1*8

	li	t2, RV_RECREG
	li	t1, 0x3
	sw	t1, 0(t2)

	/* === Extend S-mode Control Registers === */
	/* restore S-mode Process Control and Status Extend Registers */
	restore_csr sxstatus		0
	restore_csr shcr		1
	restore_csr scounterinten	2
	restore_csr scounterof		3
	//restore_csr scountinhibit	4
	restore_csr shpmcr		5
	//restore_csr shpmsp		6
	//restore_csr shpmep		7
	restore_csr scycle		8
	//restore_csr sinstret		9
	restore_csr shpmcounter3	10
	restore_csr shpmcounter4	11
	restore_csr shpmcounter5	12
	restore_csr shpmcounter6	13
	restore_csr shpmcounter7	14
	restore_csr shpmcounter8	15
	restore_csr shpmcounter9	16
	restore_csr shpmcounter10	17
	restore_csr shpmcounter11	18
	restore_csr shpmcounter12	19
	restore_csr shpmcounter13	20
	restore_csr shpmcounter14	21
	restore_csr shpmcounter15	22
	restore_csr shpmcounter16	23
	restore_csr shpmcounter17	24
	restore_csr shpmcounter18	25
	restore_csr shpmcounter19	26
	restore_csr shpmcounter20	27
	restore_csr shpmcounter21	28
	restore_csr shpmcounter22	29
	restore_csr shpmcounter23	30
	restore_csr shpmcounter24	31
	restore_csr shpmcounter25	32
	restore_csr shpmcounter26	33
	restore_csr shpmcounter27	34
	restore_csr shpmcounter28	35
	restore_csr shpmcounter29	36
	restore_csr shpmcounter30	37
	restore_csr shpmcounter31	38
	addi t0, t0, 39*8

	li	t2, RV_RECREG
	li	t1, 0x4
	sw	t1, 0(t2)

	/* === Standard U-mode(User mode) === */
	/* restore U-mode float Control Registers(with extend control regitster fxcr) */
	restore_csr fflags		0
	restore_csr frm			1
	restore_csr fcsr		2
	restore_csr fxcr		3
	addi t0, t0, 4*8
	/* restore U-mode counter regitsters(do not need t0 restore, read only) */
	/* Vector Extend Registers */
	//restore_csr vstart		0
	restore_csr vxsat		1
	restore_csr vxrm		2
	addi t0, t0, 6*8

	li	t2, RV_RECREG
	li	t1, 0x5
	sw	t1, 0(t2)

	/* restore float_regs */
	la	t0, rv_regs
	addi	t0, t0, 32*8
	float_reg fld t0

	li	t2, RV_RECREG
	li	t1, 0x6
	sw	t1, 0(t2)

	/* update the sleep flag */
	la	t0, rv_sleep_flag
	sd	x0, 0(t0)

	/* restore general regs */
	general_reg ld rv_regs
	/* restore t0 register */
	sd	t1, -8(sp)
	la	t1, rv_regs
	ld	t0, 5*8(t1)
	ld	t1, -8(sp)

	ret

