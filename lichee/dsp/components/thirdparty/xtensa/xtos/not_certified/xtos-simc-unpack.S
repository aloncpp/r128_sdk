
// xtos-simc-unpack.S  - ROM unpack for single-image multicore program.

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
#include <xtensa/config/system.h>
#include <xtensa/coreasm.h>
#include "xtos-internal.h"


#if XCHAL_HAVE_PRID && XCHAL_HAVE_EXCLUSIVE && (XSHAL_CLIB == XTHAL_CLIB_XCLIB)

	// This code must live in a section that does not need unpacking.
	// It is called from the reset handler so convenient to put it in
	// the same section. NOTE if the reset handler is moved to another
	// section then this code (and literals) should move with it.

	.section	.ResetHandler.text, "ax"
	.global		__reset_user_init
	.type		__reset_user_init,@function
	.align		4

	//---------------------------------------------------------------------
	// This function unpacks program sections from the ROM image.
	// Core 0 unpacks all common sections that go into shared memory.
	// Each core unpacks sections that go into local inst/data memory.
	// At entry:
	//   a0 - return address
	//   a1 - NOT a valid stack pointer
	//---------------------------------------------------------------------

__reset_user_init:

	// Set up base/bounds for all local memories present.

#if defined (XCHAL_DATARAM0_VADDR)
	movi	a8, XCHAL_DATARAM0_VADDR
	movi	a9, XCHAL_DATARAM0_SIZE
#endif
#if defined (XCHAL_DATARAM1_VADDR)
	movi	a10, XCHAL_DATARAM1_VADDR
	movi	a11, XCHAL_DATARAM1_SIZE
#endif
#if defined (XCHAL_INSTRAM0_VADDR)
	movi	a12, XCHAL_INSTRAM0_VADDR
	movi	a13, XCHAL_INSTRAM0_SIZE
#endif
#if defined (XCHAL_INSTRAM1_VADDR)
	movi	a14, XCHAL_INSTRAM1_VADDR
	movi	a15, XCHAL_INSTRAM1_SIZE
#endif

	// There is no stack, so we can use a1 for other purposes.

	movi	a2, _rom_store_table	// ROM table address
	rsr.prid  a1
	extui	a1, a1, PRID_ID_SHIFT, PRID_ID_BITS
	beqz	a2, .Lunpackdone	// Nothing to unpack

.Lunpack_one:
	l32i	a3, a2, 0		// Destination start address
	l32i	a4, a2, 4		// Destination end address
	l32i	a5, a2, 8		// Source start address
	addi	a2, a2, 12
	bgeu	a3, a4, .Lupnext	// Skip if start >= end

	// Check the destination start address against local memories.

#if defined (XCHAL_DATARAM0_VADDR)
	sub	a7, a3, a8		// a7 = dest_start_addr - dataram0_start_addr
	bltu	a7, a9, .Luploop	// Dest in this memory, unpack
#endif
#if defined (XCHAL_DATARAM1_VADDR)
	sub	a7, a3, a10
	bltu	a7, a11, .Luploop
#endif
#if defined (XCHAL_INSTRAM0_VADDR)
	sub	a7, a3, a12
	bltu	a7, a13, .Luploop
#endif
#if defined (XCHAL_INSTRAM1_VADDR)
	sub	a7, a3, a14
	bltu	a7, a15, .Luploop
#endif

	// If we came here, this unpack section is not in any local memory.
	// Only core0 should unpack this section.

	bnez	a1, .Lunpack_one

	// Unpacking loop. Can't assume section size is anything other than
	// a multiple of 4.

.Luploop:
	l32i	a6, a5, 0
	addi	a5, a5, 4
	s32i	a6, a3, 0
	addi	a3, a3, 4
	bltu	a3, a4, .Luploop
	j	.Lunpack_one

.Lupnext:
	bnez	a3, .Lunpack_one	// If both zero then end of table
	bnez	a5, .Lunpack_one
.Lunpackdone:
	movi	a2, 1			// Indicate unpack done
	ret

	.size	__reset_user_init, . - __reset_user_init

#endif	// XCHAL_HAVE_PRID && XCHAL_HAVE_EXCLUSIVE && (XSHAL_CLIB == XTHAL_CLIB_XCLIB)

