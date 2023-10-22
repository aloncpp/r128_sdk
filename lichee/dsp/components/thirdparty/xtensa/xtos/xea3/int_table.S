// int_table.S - Interrupt Handler table and default handler for XEA3.

// Copyright (c) 2003-2015 Cadence Design Systems, Inc.
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


#include <xtensa/coreasm.h>
#include <xtensa/config/specreg.h>
#include "../xtos-internal.h"


#if XCHAL_HAVE_INTERRUPTS

	// Interrupt Handler Table

#if XCHAL_HAVE_ISB
	.data
#else
	.section	.intr.data, "aw"
#endif
	// This name is used by the reset handler to set up the ISB/ITB.
	// Non-XTOS applications must use the same name.
	.global	_xt_interrupt_table
	.weak	_xt_interrupt_table

	// This name is used by XTOS internally.
	.global	xtos_interrupt_table
	.align	16

_xt_interrupt_table:
xtos_interrupt_table:

#if XCHAL_RH01_ERRATUM
	// Redirection table, handler addresses only no args.
	.set	i, 0
	.rept	XCHAL_NUM_INTERRUPTS
		.word	_xtos_interrupt_wrapper		// wrapper code
		.set	i, i+1
	.endr

	.align	4
	.global	_rh0_interrupt_table

_rh0_interrupt_table:
#endif

	.set	i, 0
	.rept	XCHAL_NUM_INTERRUPTS
		.word	xtos_unhandled_interrupt	// default handler
	 	.word	i				// default arg - intr number
		.set	i, i+1
	.endr

	// Default Interrupt Handler

	.text
	.align	4
	.global	xtos_unhandled_interrupt
	.type	xtos_unhandled_interrupt,@function

xtos_unhandled_interrupt:
	abi_entry
#if XCHAL_HAVE_DEBUG
	break	1, 15		// break into debugger if present
#else
1:
	j	1b		// loop forever
# endif
	abi_return

	.size	xtos_unhandled_interrupt, . - xtos_unhandled_interrupt

#if XCHAL_RH01_ERRATUM
	.align	4
	.global	_xtos_interrupt_wrapper
	.extern	_Dispatch_tailchain
	.begin	no-schedule

_xtos_interrupt_wrapper:
	abi_entry
	movi	a3, _rh0_interrupt_table
	addx2	a2, a2, a3	// a2 already has intnum << 2
	l32i	a3, a2, 0	// a3 <- handler address
#ifdef __XTENSA_CALL0_ABI__
	l32i	a2, a2, 4	// a2 <- handler argument
	callx0	a3
	movi	a0, _Dispatch_tailchain
#else
	l32i	a10, a2, 4	// a10 <- handler argument
	callx8	a3
#endif
	abi_return
	_nop
	_nop
	_nop
	_nop

	.end	no-schedule
#endif

#endif /* XCHAL_HAVE_INTERRUPTS */

