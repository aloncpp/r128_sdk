// exc_dispatch.S  -  Exception dispatch for XEA3

// Copyright (c) 2003-2017 Cadence Design Systems, Inc.
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
#include <xtensa/config/system.h>


#if XCHAL_HAVE_EXCEPTIONS

	.section	.DispatchHandler.text, "ax"
	.align		4
	.global		_xtos_exc_dispatch
	.weakref	_exc_table, xtos_exc_handler_table
	.extern		xtos_default_exc_handler
	.extern		_Dispatch_tailchain
	.begin		no-schedule

	// Control comes here from the exception vector.
	// At this point, we have:
	//   - All register state saved on the stack
	//   - a0 holds the return PC
	// Call0:
	//   - a11 holds ExcCause, also saved on stack
	//   - a15 points to old stack frame, a1 points to new stack frame
	// Windowed:
	//   - registers already rotated (so a2 will hold arg)
	//   - a3 holds ExcCause, also saved on stack

_xtos_exc_dispatch:
	movi	a4, _exc_table					// Address of exc handler table
	movi	a2, xtos_default_exc_handler			// Address of default handler
	beqz	a4, 1f						// No handler table ?
#ifdef __XTENSA_CALL0_ABI__
	extui	a3, a11, EXCCAUSE_CAUSE_SHIFT, EXCCAUSE_CAUSE_BITS	// Extract exc cause
#else
	extui	a3, a3, EXCCAUSE_CAUSE_SHIFT, EXCCAUSE_CAUSE_BITS	// Extract exc cause
#endif
	addx4	a4, a3, a4
	l32i	a4, a4, 0					// Load handler address
1:
	moveqz	a4, a2, a4					// If no exception table or
								// no handler then use default
	mov	a2, a1						// Set handler parameter

#if XCHAL_RH01_ERRATUM
	callx0	a4						// Call handler
	movi	a8, _Dispatch_tailchain + 3			// NOTE registers rotated back
	movi	a9, 0
#ifdef  __XTENSA_CALL0_ABI__
	s32i	a9,  a1, 24					// [old_a1-72] <- 0 (ExcCause cleared)
#else
	addi	a10,  a1, -96
	s32i	a9, a10, 24					// ExcCause cleared
#endif
	jx	a8
	_nop							// Extra instructions inserted...
	_nop
	_nop
	_nop
#else
	jx	a4						// Return directly from handler
#endif

	.end	no-schedule
	.size	_xtos_exc_dispatch, . - _xtos_exc_dispatch

#endif /* XCHAL_HAVE_EXCEPTIONS */

