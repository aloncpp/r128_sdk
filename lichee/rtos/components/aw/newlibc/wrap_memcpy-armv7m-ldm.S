/*
 * Copyright (c) 2013 ARM Ltd
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the company may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ARM LTD ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ARM LTD BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* This memcpy routine is optimised for Cortex-M3/M4 cores with/without
   unaligned access.

   If compiled with GCC, this file should be enclosed within following
   pre-processing check:
   if defined (__ARM_ARCH_7M__) || defined (__ARM_ARCH_7EM__)

   Prototype: void *memcpy (void *dst, const void *src, size_t count);

   The job will be done in 5 steps.
   Step 1: Align src/dest pointers, copy mis-aligned if fail to align both
   Step 2: Repeatedly copy big block size of __OPT_BIG_BLOCK_SIZE
   Step 3: Repeatedly copy big block size of __OPT_MID_BLOCK_SIZE
   Step 4: Copy word by word
   Step 5: Copy byte-to-byte

   Tunable options:
     __OPT_BIG_BLOCK_SIZE: Size of big block in words.  Default to 64.
     __OPT_MID_BLOCK_SIZE: Size of big block in words.  Default to 16.
 */
	.syntax unified
	.text
	.align	2
	.global	memcpy
	.thumb
	.thumb_func
	.type	memcpy, %function
memcpy:
	@ r0: dst
	@ r1: src
	@ r2: len

	push	{r0, r4, lr}
	orr	r3, r1, r0
	ands	r3, r3, #3
	bne	.Lmisaligned_copy

	/* both src and dst are aligned to word */
.Lbig_block_both_aligned:
	subs	r2, #64
	blo	.Lmid_block_both_aligned

	/* Kernel loop for big block copy */
	.align 2
.Lbig_block_both_aligned_loop:
	ldmia	r1!, {r3, r4, r12, lr}
	stmia	r0!, {r3, r4, r12, lr}
	ldmia	r1!, {r3, r4, r12, lr}
	stmia	r0!, {r3, r4, r12, lr}
	ldmia	r1!, {r3, r4, r12, lr}
	stmia	r0!, {r3, r4, r12, lr}
	ldmia	r1!, {r3, r4, r12, lr}
	stmia	r0!, {r3, r4, r12, lr}
	subs	r2, #64
	bhs .Lbig_block_both_aligned_loop

.Lmid_block_both_aligned:
	adds	r2, #48
	blo	.Lcopy_word_by_word

	/* Kernel loop for mid-block copy */
	.align 2
.Lmid_block_both_aligned_loop:
	ldmia	r1!, {r3, r4, r12, lr}
	stmia	r0!, {r3, r4, r12, lr}
	subs	r2, #16
	bhs	.Lmid_block_both_aligned_loop
	b	.Lcopy_word_by_word

	/* only dst is aligned to word */
.Lbig_block_dst_aligned:
	orr	r3, r1, r0
	ands	r3, r3, #3
	beq	.Lbig_block_both_aligned

	subs	r2, #64
	blo	.Lmid_block_dst_aligned

	/* Kernel loop for big block copy */
	.align 2
.Lbig_block_dst_aligned_loop:
	ldr	r3,  [r1], #4
	ldr	r4,  [r1], #4
	ldr	r12, [r1], #4
	ldr	lr,  [r1], #4
	stmia	r0!, {r3, r4, r12, lr}
	ldr	r3,  [r1], #4
	ldr	r4,  [r1], #4
	ldr	r12, [r1], #4
	ldr	lr,  [r1], #4
	stmia	r0!, {r3, r4, r12, lr}
	ldr	r3,  [r1], #4
	ldr	r4,  [r1], #4
	ldr	r12, [r1], #4
	ldr	lr,  [r1], #4
	stmia	r0!, {r3, r4, r12, lr}
	ldr	r3,  [r1], #4
	ldr	r4,  [r1], #4
	ldr	r12, [r1], #4
	ldr	lr,  [r1], #4
	stmia	r0!, {r3, r4, r12, lr}
	subs	r2, #64
	bhs .Lbig_block_dst_aligned_loop

.Lmid_block_dst_aligned:
	adds	r2, #48
	blo	.Lcopy_word_by_word

	/* Kernel loop for mid-block copy */
	.align 2
.Lmid_block_dst_aligned_loop:
	ldr	r3,  [r1], #4
	ldr	r4,  [r1], #4
	ldr	r12, [r1], #4
	ldr	lr,  [r1], #4
	stmia	r0!, {r3, r4, r12, lr}
	subs	r2, #16
	bhs	.Lmid_block_dst_aligned_loop

.Lcopy_word_by_word:
	adds	r2, #12
	blo	.Lcopy_less_than_4

	/* Kernel loop for small block copy */
	.align 2
.Lcopy_word_by_word_loop:
	ldr	r3, [r1], #4
	str	r3, [r0], #4
	subs	r2, #4
	bhs	.Lcopy_word_by_word_loop

.Lcopy_less_than_4:
	adds	r2, #4
	beq	.Ldone

	lsls	r2, r2, #31
	itt ne
	ldrbne  r3, [r1], #1
	strbne  r3, [r0], #1

	bcc	.Ldone
	ldrh	r3, [r1]
	strh	r3, [r0]

.Ldone:
	pop {r0, r4, pc}

	.align 2
.Lmisaligned_copy:
	/* Define label DST_ALIGNED to BIG_BLOCK.  It will go to aligned copy
	   once destination is adjusted to aligned.  */
#define Ldst_aligned Lbig_block_dst_aligned

	/* Copy word by word using LDR when alignment can be done in hardware,
	i.e., SCTLR.A is set, supporting unaligned access in LDR and STR.  */

	cmp	r2, #8
	blo	.Lbyte_copy

	/* Align dst only, not trying to align src.  That is the because
	handling of aligned src and misaligned dst need more overhead than
	otherwise.  By doing this the worst case is when initial src is aligned,
	additional up to 4 byte additional copy will executed, which is
	acceptable.  */

	ands	r3, r0, #3
	beq	.Ldst_aligned

	rsb	r3, #4
	subs	r2, r3

	lsls    r3, r3, #31
	itt ne
	ldrbne  r3, [r1], #1
	strbne  r3, [r0], #1

	bcc .Ldst_aligned

	ldrh    r3, [r1], #2
	strh    r3, [r0], #2
	b	.Ldst_aligned

.Lbyte_copy:
	subs	r2, #4
	blo	.Lcopy_less_than_4

.Lbyte_copy_loop:
	subs    r2, #1
	ldrb    r3, [r1], #1
	strb    r3, [r0], #1
	bhs	.Lbyte_copy_loop

	ldrb	r3, [r1]
	strb	r3, [r0]
	ldrb	r3, [r1, #1]
	strb	r3, [r0, #1]
	ldrb	r3, [r1, #2]
	strb	r3, [r0, #2]

	pop {r0, r4, pc}

	.size	memcpy, .-memcpy
