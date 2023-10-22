
// bss-init-simc.S  - BSS initialization for single-image multicore program.

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

	.text
	.global	__bss_init
	.type	__bss_init,@function
	.align	4

__bss_init:
	// void __bss_init(unsigned int *table_start, unsigned int *table_end)
	//
	// table_start -- points to first entry in BSS table (2 words)
	// table_end   -- points to end of BSS table
	//
	// This function clears BSS sections in dataram for all cores.
	// BSS sections not in dataram are cleared only by core 0.

	abi_entry
	rsr.prid  a4
	extui	a4, a4, PRID_ID_SHIFT, PRID_ID_BITS

.L1:
	bgeu	a2, a3, .Lret		// at table end, finish
	l32i	a5, a2, 0		// entry start address
	l32i	a6, a2, 4		// entry end address
	addi	a2, a2, 8		// advance to next entry
	beq	a5, a6, .L1		// empty section, skip

#if defined (XCHAL_DATARAM0_VADDR)
	movi	a7, XCHAL_DATARAM0_VADDR
	movi	a8, XCHAL_DATARAM0_SIZE
	sub	a7, a5, a7		// a7 = start_addr - dataram0_start_addr
	bgeu	a7, a8, .L2		// not within dataram0 range
	j	.Lclear
#endif
.L2:
#if defined (XCHAL_DATARAM1_VADDR)
	movi	a7, XCHAL_DATARAM1_VADDR
	movi	a8, XCHAL_DATARAM1_SIZE
	sub	a7, a5, a7		// a7 = start_addr - dataram1_start_addr
	bgeu	a7, a8, .L3		// not within dataram1 range
	j	.Lclear
#endif
.L3:
	bnez	a4, .L1			// not core 0, skip
.Lclear:
	movi	a7, 0			// value to store
	sub	a10, a6, a5		// a10 = length, assumed a multiple of 4
	bbci.l	a10, 2, .L1z
	s32i	a7, a5, 0		// clear 4 bytes to make length multiple of 8
	addi	a5, a5, 4
.L1z:
	bbci.l	a10, 3, .L2z
	s32i	a7, a5, 0		// clear 8 bytes to make length multiple of 16
	s32i	a7, a5, 4
	addi	a5, a5, 8
.L2z: 
	srli	a10, a10, 4		// length is now multiple of 16, divide by 16
	floopnez  a10, clearzte
	s32i	a7, a5,  0		// clear 16 bytes at a time...
	s32i	a7, a5,  4
	s32i	a7, a5,  8
	s32i	a7, a5, 12
	addi	a5, a5, 16
	floopend  a10, clearzte
	bltu	a2, a3, .L1
.Lret:
	abi_return

	.size	__bss_init, . - __bss_init

#endif	// XCHAL_HAVE_PRID && XCHAL_HAVE_EXCLUSIVE && (XSHAL_CLIB == XTHAL_CLIB_XCLIB)

