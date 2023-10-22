// exc_default_handler.S  -  Default handler for exceptions (XEA3)

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
#include <xtensa/xtruntime-frames.h>
#if defined (SIMULATOR)
#include <xtensa/simcall.h>
#endif


#if XCHAL_HAVE_EXCEPTIONS

	// Storage space needed by default exception handler.
	// NOTE: These saved registers will be restored by the ISS.
	// The layout must match that assumed by the ISS. A pointer
	// to this area will be in a8 when the simcall is executed.

	.data
	.align		4
	.local		_xtos_exc_savearea

_xtos_exc_savearea:
	.word		0	// PC
	.word		0	// PS
	.word		0	// a2
	.word		0	// a8

	// XTOS default exception handler. Traps to debugger or
	// simulator (if present). Register a2 needs to hold the syscall
	// cause for the simulator, but it also is the pointer to the
	// exception frame. a2 and a few other registers are saved in
	// memory, and a pointer to the save area is left in a8 for the
	// simulator to read. The simulator will restore these registers.
	// This is not written in C because we don't want an automatic
	// stack access at entry to the handler (which would be bad if
	// we came here because of a stack limit violation).

	.text
	.align		4
	.global		xtos_default_exc_handler

xtos_default_exc_handler:

	l32i	a0, a2, UEXC_exccause	// a0 <- ef->exccause
	wsr.exccause a0			// Restore EXCCAUSE

#if defined (SIMULATOR)

	l32i	a0, a2, UEXC_excvaddr
	wsr.excvaddr a0			// Restore EXCVADDR

	movi	a1, _xtos_exc_savearea	// Save PC/PS/a8 for the simulator
	l32i	a0, a2, UEXC_pc
	s32i	a0, a1, 0
	l32i	a0, a2, UEXC_ps
	s32i	a0, a1, 4
	l32i	a0, a2, UEXC_a8
	s32i	a0, a1, 12

#ifdef __XTENSA_WINDOWED_ABI__
	s32i	a2, a1, 8		// Save a2 before the window rotate
	movi	a0, .Lfakeret		// Fake a return, rotates register
	retw				// window back and restores WB
.Lfakeret:
	movi	a8, _xtos_exc_savearea
	l32i	a8, a8, 8		// a8 = pointer to exception frame
#else
	mov	a8, a2			// a8 = pointer to exception frame
	l32i	a0, a8, UEXC_a0
	l32i	a1, a8, UEXC_a1
	l32i	a2, a8, UEXC_a2
	l32i	a3, a8, UEXC_a3
	l32i	a4, a8, UEXC_a4
	l32i	a5, a8, UEXC_a5
	l32i	a6, a8, UEXC_a6
	l32i	a7, a8, UEXC_a7
#endif
	l32i	 a9, a8, UEXC_a9
	l32i	a10, a8, UEXC_a10
	l32i	a11, a8, UEXC_a11
	l32i	a12, a8, UEXC_a12
	l32i	a13, a8, UEXC_a13
	l32i	a14, a8, UEXC_a14
	l32i	a15, a8, UEXC_a15

	movi	a8, _xtos_exc_savearea	// Leave pointer in a8 for simulator
	s32i	a2, a8, 8		// Save a2
	movi	a2, SYS_gdb_enter_sktloop
	simcall				// Trap to simulator

#else /* SIMULATOR */

	extui	a3, a3, EXCCAUSE_CAUSE_SHIFT, EXCCAUSE_CAUSE_BITS
	beqi	a3, EXCCAUSE_DEBUG, 1f	// Skip break if debug exception
	break	1, 1			// Break to debugger if present
1:
	halt				// Halt here

#endif

	.size	xtos_default_exc_handler, . - xtos_default_exc_handler

#endif /* XCHAL_HAVE_EXCEPTIONS */

