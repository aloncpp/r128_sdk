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

	# __memcpy_align_operation

	.macro  __memcpy_align_operation reg1, reg2, reg3, elen, lmul, size
	vsetvli t0, \reg2, \elen, \lmul    # Vectors of 8b
	vle.v v0, (\reg1)             # Load bytes
	vse.v v0, (\reg3)             # Store bytes
	addi \reg1, \reg1, \size      # Bump pointer
	addi \reg2, \reg2, -\size     # Decrement count
	addi \reg3, \reg3, \size       # Bump pointer
	.endm

	.balign 4
	.global vector_memcpy_size128

	# void *vector_memcpy_size128(void *d, const void *s, size_t n);
	# a0=d, a1=s, a2=n
	# n=multiples of 128
	#
vector_memcpy_size128:
	mv a3, a0 # Copy destination
loop_128:
	__memcpy_align_operation a1, a2, a3, e8, m8, 128
	bnez a2, loop_128               # Any more?

	ret                         # Return


	.balign 4
	.global vector_memcpy_size64

	# void *vector_memcpy_size64(void *d, const void *s, size_t n);
	# a0=d, a1=s, a2=n
	# n=multiples of 64
	#
vector_memcpy_size64:
	mv a3, a0 # Copy destination
loop_64:
	__memcpy_align_operation a1, a2, a3, e8, m4, 64
	bnez a2, loop_64               # Any more?

	ret                         # Return


	.balign 4
	.global vector_memcpy_size32

	# void *vector_memcpy_size32(void *d, const void *s, size_t n);
	# a0=d, a1=s, a2=n
	# n=multiples of 32
	#
vector_memcpy_size32:
	mv a3, a0 # Copy destination
loop_32:
	__memcpy_align_operation a1, a2, a3, e8, m2, 32
	bnez a2, loop_32               # Any more?

	ret                         # Return


	.balign 4
	.global vector_memcpy_size16

	# void *vector_memcpy_size16(void *d, const void *s, size_t n);
	# a0=d, a1=s, a2=n
	# n=multiples of 16
	#
vector_memcpy_size16:
	mv a3, a0 # Copy destination
loop_16:
	__memcpy_align_operation a1, a2, a3, e8, m1, 16
	bnez a2, loop_16               # Any more?

	ret                         # Return

	.balign 4
	.global vector_memcpy_size128_tail

	# void *vector_memcpy_size128_tail(void *d, const void *s, size_t n);
	# a0=d, a1=s, a2=n
	# tail function handles unaligned data
	#
vector_memcpy_size128_tail:
	mv a3, a0 # Copy destination
	li a4, 128
	bltu a2, a4, tail_128

tail_loop_128:
	__memcpy_align_operation a1, a2, a3, e8, m8, 128
	bgeu a2, a4, tail_loop_128
	beqz a2, tail_end

tail_128:
	li a4, 64
	bltu a2, a4, tail_64
	__memcpy_align_operation a1, a2, a3, e8, m4, 64
	beqz a2, tail_end

tail_64:
	li a4, 1
	sll a4, a4, a2
	vsetvli t0, x0, e64, m1    # e64, m1
	vmxor.mm v0, v0, v0        # v0=0
	vmv.s.x v0, a4             # v0[0] = a4
	vmsbf.m v0, v0
	vsetvli t0, x0, e8, m4     # e8, m1
	vle.v v8, (a1), v0.t         # Load bytes
	vse.v v8, (a3), v0.t         # Store bytes

tail_end:
	ret                       # Return

/* dump
vector_memcpy_size128:
	mv a3, a0 # Copy destination
loop:
	vsetvli t0, a2, e32, m8    # Vectors of 8b
	vle.v v0, (a1)             # Load bytes
	vse.v v0, (a3)             # Store bytes
	addi a1, a1, 128              # Bump pointer
	addi a2, a2, -128              # Decrement count
	add a3, a3, 128              # Bump pointer
	bnez a2, loop               # Any more?
	ret                         # Return

 */

	.balign 4
	.global memcpy_O3_rvv

	# void *memcpy_O3_rvv(void *d, const void *s, size_t n);
	# a0=d, a1=s, a2=n
	# Assembly of memcpy in newlibc compiled under RVV and O3
	#


memcpy_O3_rvv:
	 addi    sp,sp,-16
	 xor     a5,a1,a0
	 sd      s0,8(sp)
	 andi    a5,a5,7
	 add     a7,a0,a2
	 bnez    a5,memcpy0xd0
	 li      a4,7
	 bgeu    a4,a2,memcpy0xd0
	 andi    a4,a0,7
	 mv      t1,a0
	 bnez    a4,memcpy0x60
memcpy0x1e:
	andi    a4,a7,-8
	addi    a3,a4,-64
	bltu    t1,a3,memcpy0x7e
memcpy0x2a:
	bgeu    t1,a4,memcpy0x56
	addi    a4,a4,-1
	sub     a4,a4,t1
	srli    a4,a4,0x3
	addi    a6,a4,1
	j       memcpy0x3e
memcpy0x3c:
	mv      a5,a2
memcpy0x3e:
	lrd     a3,a1,a5,3
	addi    a2,a5,1
	srd     a3,t1,a5,3
	bne     a5,a4,memcpy0x3c
	slli    a4,a6,0x3
	add     t1,t1,a4
	add     a1,a1,a4
memcpy0x56:
	bltu    t1,a7,memcpy0xd6
memcpy0x5a:
	ld      s0,8(sp)
	addi    sp,sp,16
	ret
memcpy0x60:
	lbuia   a4,(a1),1,0
	sbia    a4,(t1),1,0
	andi    a4,t1,7
	beqz    a4,memcpy0x1e
	lbuia   a4,(a1),1,0
	sbia    a4,(t1),1,0
	andi    a4,t1,7
	bnez    a4,memcpy0x60
	j       memcpy0x1e
memcpy0x7e:
	addi    a1,a1,72
	ld      s0,-72(a1)
	ld      t2,-64(a1)
	ld      t2,-64(a1)
	ld      t0,-56(a1)
	ld      t6,-48(a1)
	ld      t5,-40(a1)
	ld      t4,-32(a1)
	ld      t3,-24(a1)
	ld      a6,-16(a1)
	ld      a2,-8(a1)
	sdia    s0,(t1),9,3
	sd      t2,-64(t1)
	sd      t0,-56(t1)
	sd      t6,-48(t1)
	sd      t5,-40(t1)
	sd      t4,-32(t1)
	sd      t3,-24(t1)
	sd      a6,-16(t1)
	sd      a2,-8(t1)
	bltu    t1,a3,memcpy0x7e
	j       memcpy0x2a
memcpy0xd0:
	mv      t1,a0
	bgeu    a0,a7,memcpy0x5a
memcpy0xd6:
	 addi    a2,a7,-1
	 addi    a3,t1,1
	 sub     a2,a2,t1
	 sltiu   a2,a2,31
	 sltu    a5,a7,a3
	 xori    a2,a2,1
	 xori    a5,a5,1
	 and     a2,a2,a5
	 mv      a4,t1
	 beqz    a2,memcpy0x152
	 or      a5,t1,a1
	 andi    a5,a5,31
	 bnez    a5,memcpy0x152
	 sub     a5,a7,t1
	 sltu    a2,a7,a3
	 li      a6,1
	 mv      a3,a5
	 mvnez   a3,a6,a2
	 andi    a2,a3,-32
	 mv      a5,a1
	 add     a2,a2,a1
	 vsetvli zero,zero,e8,m2,d1
memcpy0x11c:
	vle.v   v8,(a5)
	vse.v   v8,(a4)
	addi    a5,a5,32
	addi    a4,a4,32
	bne     a5,a2,memcpy0x11c
	andi    a4,a3,-32
	add     a5,t1,a4
	add     a1,a1,a4
	beq     a3,a4,memcpy0x5a
memcpy0x13e:
	lbuia   a4,(a1),1,0
	sbia    a4,(a5),1,0
	bltu    a5,a7,memcpy0x13e
	ld      s0,8(sp)
	addi    sp,sp,16
	ret
memcpy0x150:
	addi    a3,a3,1
memcpy0x152:
	lbuia   a5,(a1),1,0
	sb      a5,-1(a3)
	bltu    a3,a7,memcpy0x150
	j       memcpy0x5a

