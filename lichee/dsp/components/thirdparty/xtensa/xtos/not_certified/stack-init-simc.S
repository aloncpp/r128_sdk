
// stack-init-simc.S  - Stack setup for single-image multicore program.

// Copyright (c) 1998-2018 Cadence Design Systems, Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


#include <xtensa/config/core.h>
#include <xtensa/coreasm.h>
#include <xtensa/config/system.h>
#include "xtos-internal.h"


#if XCHAL_HAVE_PRID && XCHAL_HAVE_EXCLUSIVE && (XSHAL_CLIB == XTHAL_CLIB_XCLIB)

	.extern	_heap_sentry_ptr	// heap lower bound (sbrk.c)
	.weak	__xtos_simc		// defined for SIMC application
	.weak	__stack_size		// define weak for user override

	.text
	.global	__stack_init
	.type	__stack_init,@function
	.align	4

__stack_init:

	// NOTE: called via call0/callx0, return address is in a0.
	// NOTE: must not rely on valid stack pointer / BSS values.

	movi	a2, __xtos_simc
	beqz	a2, 1f			// nothing to do

#if defined (XCHAL_DATARAM0_VADDR)
	movi	a3, XCHAL_DATARAM0_VADDR
	movi	a4, XCHAL_DATARAM0_SIZE
	sub	a3, a1, a3		// a3 = SP - dataram0_start_addr
	bltu	a3, a4, 1f		// SP in dataram0, no adjustment
#endif
#if defined (XCHAL_DATARAM1_VADDR)
	movi	a3, XCHAL_DATARAM1_VADDR
	movi	a4, XCHAL_DATARAM1_SIZE
	sub	a3, a1, a3		// a3 = SP - dataram1_start_addr
	bltu	a3, a4, 1f		// SP in dataram1, no adjustment
#endif
	movi	a2, __stack_size	// must be multiple of 16 bytes
	rsr.prid  a3
	extui	a3, a3, PRID_ID_SHIFT, PRID_ID_BITS

	mov	a4, a1
2:
	beqz	a3, 3f
	sub	a4, a4, a2
	addi	a3, a3, -1
	j	2b
3:
	mov	a1, a4			// set stack pointer

	// Now we need to adjust the heap end point to make sure that the
	// heap does not run into the stack. This code will be executed
	// by all the cores, so we need to make the lowest value stick.
	// The loop below ensures that the smallest value wins.

	movi	a3, _heap_sentry_ptr
	sub	a4, a4, a2		// a4 = bottom of stack
4:
	l32i	a2, a3, 0		// a2 = heap_sentry value (heap end)
	bltu	a2, a4, 1f		// already below stack bottom, skip
	beq	a2, a4, 1f		// equal to stack bottom, skip
	s32i	a4, a3, 0		// set heap end to smaller value
	j	4b			// repeat and check
1:
	ret

	.size	__stack_init, . - __stack_init

#endif // XCHAL_HAVE_PRID && XCHAL_HAVE_EXCLUSIVE && (XSHAL_CLIB == XTHAL_CLIB_XCLIB)

