
// dispatch-vector-v1.S  --  Xtensa Dispatch Code for XEA3 (Version 1)

// Copyright (c) 1999-2017 Tensilica Inc.
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
#include <xtensa/corebits.h>
#include <xtensa/core-macros.h>


// Uncomment this and rebuild to enable handling exceptions after interrupts
// without full context restore, i.e. without returning to the code that
// caused the exception and having it regenerated.
//#define XTOS_EXC_AFTER_INTR  1


// Enable this and rebuild to disable interrupts during exception handling.
// NOTE: This must be enabled to work around RH.0/1 erratum.

#if XCHAL_RH01_ERRATUM
#define XTOS_DISABLE_INT_ON_EXC  1
#endif


//-----------------------------------------------------------------------------
// The entry point vectors are common for call0 and windowed configurations.
//-----------------------------------------------------------------------------

	.extern	_DoubleExceptionHandler
	.extern	_ResetHandler
	.extern	_xtos_exc_dispatch

	.global	_DispatchVector
	.global	_DoubleExceptionVector
	.weak	_DoubleExceptionVector

	.section .DispatchVector.text, "ax"
#if XCHAL_HAVE_VECBASE
	.align	64
#else
	.align	4
#endif
_DispatchVector:

	.org	0
	j	JumpToResetHandler

	.org	3			// Reserved
	ill

	.org	6
_DoubleExceptionVector:
	j	_DoubleExceptionHandler

	.org	9			// Reserved
	ill

//-----------------------------------------------------------------------------
// Start of dispatch code.
//-----------------------------------------------------------------------------

	.org	12			// Fixed offset for start of dispatch code
	.global	_Dispatch_tailchain
	.global	_Dispatch_entry
	.global	_Dispatch_restore_epc
	.global	_Dispatch_restore_epc_live

_Dispatch_tailchain:

#ifdef __XTENSA_CALL0_ABI__
	//////////////////////////////////////////////////////////////////////
	// NOTE: a15 is expected to be holding the previous stack pointer
	// when we get to the Tailchain segment.

# if XCHAL_RH012_ERRATUM
	ihi	 a1, 0			// Wait for icache miss completion
# endif
# if XCHAL_RH01_ERRATUM
	j	.LfixTC
# else
	l32dis	 a3, a15, 1		// EPC <- handler_table[intnum].addr, a3 <- (intnum << 2)
	l32dis	 a2, a15, 3		// a2  <- handler_table[intnum].arg
# endif
	s32stk	 a4, a15, 2		// Set new stack pointer
	s32dis	 a0, a15, 0		// Jump to handler if interrupt, else fall through
					// Note this also clears local exclusive monitor
	// Fallthrough:  exit dispatch
	//
_Exit:
	addi	 a1, a15, 0		// Restore stack pointer
	j	_Exit_1			// Start exit process

	.org	36			// Fixed offset for Restore/Underflow segment
	.global	_Restore

_Restore:
	// Important: the following restrictions must be observed:
	// 1) The LCOUNT register must be restored after LBEG/LEND.
	// 2) There must be at least 3 instructions between the LCOUNT
	//    restore and the last L32E (the one that branches).

	// NOTE: workaround for LBEG/LEND corruption - changed immediates
	// for a12 and a13 to prevent any writes to LBEG/LEND

	l32e	a12,  a1, -64		// a12 <- [a1-16]
	l32e	a13,  a1, -64		// a13 <- [a1-12]
	l32e	a14,  a1, -36		// a14 <- [a1-08], (LCOUNT <- a14)
	l32e	 a8,  a1, -64		// a8  <- [a1-32]
	l32e	 a9,  a1, -64		// a9  <- [a1-28]
	l32e	a10,  a1, -60		// a10 <- [a1-24], (PS/SAR <- a10)
	l32e	a11,  a1, -48		// a11 <- [a1-20], (EPC    <- a11)
	l32e	a15,  a1, -64		// a15 <- [a1-04], branch to EPC
					// If interrupt, branch to Tailchain, don't update a15

	.org	60			// Fixed offset for Save/Overflow segment
	.global	_Save

_Save:
	s32e	 a0,  a1, -48		// [a1-64] <- a0 ; a0 <- EPC
	s32e	 a6,  a1, -64		// [a1-40] <- a6
	//  From here until end of entry dispatch, double exception handling may clobber a6.
	s32e	 a2,  a1, -64		// [a1-56] <- a2
	s32e	 a3,  a1, -64		// [a1-52] <- a3
	s32e	 a4,  a1, -64		// [a1-48] <- a4
	s32e	 a5,  a1, -64		// [a1-44] <- a5
	s32e	 a7,  a1, -64		// [a1-36] <- a7
	s32e	 a8,  a1, -64		// [a1-32] <- a8
	s32e	 a9,  a1, -28		// [a1-28] <- a9,  (a9  <-   PS/SAR)
	s32e	a10,  a1, -52		// [a1-24] <- a10, (a10 <- ExcVaddr)
	s32e	a11,  a1, -24		// [a1-20] <- a11, (a11 <- ExcCause)
	s32e	a12,  a1, -44		// [a1-16] <- a12, (a12 <-     LBEG)
	s32e	a13,  a1, -40		// [a1-12] <- a13, (a13 <-     LEND)
	s32e	a14,  a1, -36		// [a1-08] <- a14, (a14 <-   LCOUNT)
	s32e	a15,  a1, -32		// [a1-04] <- a15, (a15 <-       a1)

_Dispatch_entry:
# if XCHAL_HAVE_LOOPS
	s32e	a12,  a1, -20		// [a1-84] <- a12 (LBEG)
	s32e	a13,  a1, -24		// [a1-88] <- a13 (LEND)
	s32e	a14,  a1, -28		// [a1-92] <- a14 (LCOUNT)
# endif
# ifdef XTOS_EXC_AFTER_INTR
	s32e	a10,  a1, -4		// [a1-68] <- a10 (ExcVaddr) save across intr
	s32e	a11,  a1, -8		// [a1-72] <- a11 (ExcCause) save across intr
# endif

# ifdef XTOS_DISABLE_INT_ON_EXC
	// Disable interrupts if handling an exception. The handler may
	// enable interrupts. The old interrupt state is restored when
	// PS is restored during return from exception.

	extui	a14, a11, 0, 4		// a14 <- ExcCause.Cause
	_bnez	a14, .Ld1		// branch if we are handling exception
.Ld2:
# endif

# if XCHAL_RH012_ERRATUM
	ihi	 a1, 0			// Wait for icache miss completion
# endif
# if XCHAL_RH01_ERRATUM
	_nop
	_nop
	_nop
	_nop
	l32dis	 a2,  a1, 0		// Load wrapper address from table
# else
	l32dis	 a2,  a1, 1		// Load handler address from table
	l32dis	 a2,  a1, 3		// Load handler argument from table
# endif
	s32stk	 a9,  a1, 6		// [a1-76]     <- a9 (PS/SAR) ; a1 = a1-96
	s32dis	 a0, a15, 4		// [old_a1-80] <- a0 (EPC)
					// If interrupt: a0 <- VecBase+12, branch to handler
					// If exception: a0 <- VecBase+15, fall through

	// Fallthrough:  process exception
	//
	// At this point, we have:
	//   - a11 holds ExcCause
	//   - a15 points to old stack frame, a1 points to new stack frame
	//
	// EXCCAUSE and EXCVADDR may not have been saved into stack frame.
	// Save them if needed. They should be in a10/a11.

# ifndef XTOS_EXC_AFTER_INTR
	s32i	a10,  a1, 28		// [old_a1-68] <- a10 (ExcVaddr)
	s32i	a11,  a1, 24		// [old_a1-72] <- a11 (ExcCause)
# endif

	j	_Exception		// Jump around literals

	.section .DispatchHandler.text, "ax"
	.align	4
	.literal_position		// in case someone uses -mtext-section-literals

# ifdef XTOS_EXC_AFTER_INTR
_Exit_1:
	// Check for pending exception
	l32e	a11,  a1, -8		// a11 <- [a1-72] (ExcCause) (must stay in a11)
	l32e	 a8,  a1, -16		// a8  <- [a1-80] (EPC)
	extui	a10, a11, 0, 4		// a10 <- low 4 bits of ExcCause
	beqz	a10,  _Exit_2
	wsr.exccause  a11
	addi	 a1,  a1, -96		// Adjust for exception frame
	s32dis	 a8, a15, 4		// [old_a1-80] <- a8 (EPC), fall through

_Exception:
	s32i	a15,  a1, 36		// [old_a1-60] <- old SP
	call0	_xtos_exc_dispatch
	movi	 a9, 0
	s32i	 a9,  a1, 24		// [old_a1-72] <- 0 (ExcCause cleared)
	movi	 a9, _Dispatch_tailchain+3
	jx	 a9			// Finish exception processing

_Exit_2:
# else
_Exception:
	s32i	a15,  a1, 36		// [old_a1-60] <- old SP
	j	_xtos_exc_dispatch	// Exc handler returns to a0 (_Dispatch_tailchain+3)

_Exit_1:
# endif

	l32dis	a10,  a1, 13		// a10 <- [a1-76] (PS/SAR)
	l32e	a12,  a1, -20		// a12 <- [a1-84] (LBEG)
	l32e	a13,  a1, -24		// a13 <- [a1-88] (LEND)
	l32e	a14,  a1, -28		// a14 <- [a1-92] (LCOUNT)
	l32dis	a11,  a1, 12		// a11 <- [a1-80] (EPC) If interrupt pending
					// go to _Dispatch_tailchain else fall through
# if XCHAL_RH01_ERRATUM
	wsr.lbeg a12
	wsr.lend a13
# endif
	l32e	 a0,  a1, -64		// a0 <- [a1-64]
	l32e	 a2,  a1, -64		// a2 <- [a1-56] 
	l32e	 a3,  a1, -64		// a3 <- [a1-52] 
	l32e	 a4,  a1, -64		// a4 <- [a1-48]
	l32e	 a5,  a1, -64		// a5 <- [a1-44]
	l32e	 a6,  a1, -64		// a6 <- [a1-40]
	l32e	 a7,  a1, -64		// a7 <- [a1-36]

	// Fall through to _Restore_2 further below...


#else  /*****  Windowed ABI:  *****/

	//////////////////////////////////////////////////////////////////////
	//  Tailchain:

# if XCHAL_RH012_ERRATUM
	ihi	 a1, 0			// Wait for icache miss completion
# endif
# if XCHAL_RH01_ERRATUM
	j	.LfixTC
# else
	l32dis	a11,  a1, 1		// EPC <- handler_table[intnum].addr, a11 <- (intnum << 2)
	l32dis	a10,  a1, 3		// a10 <- handler_table[intnum].arg
# endif
	s32stk	a12,  a1, 2		// Set new stack pointer
	s32dis	 a8,  a1, 0		// Jump to handler if interrupt else fall through
					// Note this also clears local exclusive monitor
	// Fallthrough:  exit dispatch
_Exit:
	j	_Exit_1			// Start exit process

	.org	36			// Fixed offset for Restore/Underflow segment
	.global	_Restore

_Restore:
	// Important: the following restrictions must be observed:
	// 1) The LCOUNT register must be restored after LBEG/LEND.
	// 2) There must be at least 3 instructions between the LCOUNT
	//    restore and the last L32E (the one that branches).

	// NOTE: workaround for LBEG/LEND corruption - changed immediates
	// for a12 and a13 to prevent any writes to LBEG/LEND

	l32e	a12,  a1, -64		// a12 <- [a1-16]
	l32e	a13,  a1, -64		// a13 <- [a1-12]
	l32e	a14,  a1, -36		// a14 <- [a1-08], (LCOUNT <- a14)
	l32e	 a8,  a1, -64		// a8  <- [a1-32]
	l32e	 a9,  a1, -64		// a9  <- [a1-28]
	l32e	a10,  a1, -60		// a10 <- [a1-24], (PS/SAR <- a10)
	l32e	a11,  a1, -48		// a11 <- [a1-20], (EPC    <- a11)
	l32e	a15,  a1, -64		// a15 <- [a1-04], branch to EPC
					// If interrupt, branch to Tailchain, don't update a15

	.org	60			// Fixed offset for Save/Overflow segment
	.global	_Save

_Save:
	s32e	 a8,  a1, -48		// [a1-32] <- a8, (a8 <- EPC)
	s32e	 a9,  a1, -28		// [a1-28] <- a9,  (a9  <-   PS/SAR)
	s32e	a10,  a1, -52		// [a1-24] <- a10, (a10 <- ExcVaddr)
	s32e	a11,  a1, -24		// [a1-20] <- a11, (a11 <- ExcCause)
	s32e	a12,  a1, -44		// [a1-16] <- a12, (a12 <-     LBEG)
	s32e	a13,  a1, -40		// [a1-12] <- a13, (a13 <-     LEND)
	s32e	a14,  a1, -36		// [a1-08] <- a14, (a14 <-   LCOUNT)
	s32e	a15,  a1, -32		// [a1-04] <- a15, (a15 <-       a1)

_Dispatch_entry:
# if XCHAL_HAVE_LOOPS
	s32e	a12,  a1, -20		// [a1-84] <- a12 (LBEG)
	//  From here until out of entry dispatch, double exception handling may clobber a12.
	s32e	a13,  a1, -24		// [a1-88] <- a13 (LEND)
	s32e	a14,  a1, -28		// [a1-92] <- a14 (LCOUNT)
# endif
# ifdef XTOS_EXC_AFTER_INTR
	s32e	a10,  a1, -4		// [a1-68] <- a10 (ExcVaddr) save across intr
	s32e	a11,  a1, -8		// [a1-72] <- a11 (ExcCause) save across intr
# endif

# ifdef XTOS_DISABLE_INT_ON_EXC
	// Disable interrupts if handling an exception. The handler may
	// enable interrupts. The old interrupt state is restored when
	// PS is restored during return from exception.

	extui	a14, a11, 0, 4		// a14 <- ExcCause.Cause
	_bnez	a14, .Ld1		// branch if we are handling exception
.Ld2:
# endif

# if XCHAL_RH012_ERRATUM
	ihi	 a1, 0			// Wait for icache miss completion
# endif
# if XCHAL_RH01_ERRATUM
	_nop
	_nop
	_nop
	_nop
	l32dis	a10,  a1, 0		// Load wrapper address from table
# else
	l32dis	a10,  a1, 1		// Load handler address from table
	l32dis	a10,  a1, 3		// Load handler argument from table
# endif
	s32stk	 a9,  a1, 6		// [a1-76] <- a9 (PS/SAR) ; a9 = a1-96
	s32dis	 a8,  a1, 4		// [a1-80] <- a8 (EPC)
					// If interrupt: a8 <- VecBase+12, branch to handler
					// If exception: a8 <- VecBase+15, fall through

	// Fallthrough:  process exception
	//
	// At this point, we have:
	//   - DispSt = Normal
	//   - a3 holds ExcCause (.Cause != 0)
	//   - window rotated, a1 points to new stack frame
	//
	// EXCCAUSE and EXCVADDR may not have been saved into stack frame.
	// Save them if needed. They should be in a2/a3.

# ifndef XTOS_EXC_AFTER_INTR
	s32i	 a2,  a1, 28		// [old_a1-68] <- a2 (ExcVaddr)
	s32i	 a3,  a1, 24		// [old_a1-72] <- a3 (ExcCause)
# endif

	j	_Exception		// Jump around literals

	.section .DispatchHandler.text, "ax"
	.align	4
	.literal_position

# ifdef XTOS_EXC_AFTER_INTR
_Exit_1:
	// Check for pending exception
	l32e	a11,  a1, -8		// a11 <- [a1-72] (ExcCause) (must stay in a11)
	l32e	 a8,  a1, -16		// a8  <- [a1-80] (EPC)
	extui	a10, a11, 0, 4		// a10 <- low 4 bits of ExcCause
	beqz	a10,  _Exit_2
	wsr.exccause  a11
	addi	 a9,  a1, -96		// Set callee SP, adjust for exc frame
	s32dis	 a8,  a1, 4		// [a1-80] <- a8 (EPC), fall through

_Exception:
	call0	_xtos_exc_dispatch
	movi	 a9, 0
	addi	a10,  a1, -96
	s32i	 a9, a10, 24		// ExcCause cleared
	movi	 a9, _Dispatch_tailchain+3
	jx	 a9			// Finish exception processing

_Exit_2:
# else
_Exception:
	j	_xtos_exc_dispatch	// Exc handler returns to a0 (_Dispatch_tailchain+3)

_Exit_1:
# endif

	l32dis	a10,  a1, 13		// a10 <- [a1-76] (PS/SAR)
	l32e	a12,  a1, -20		// a12 <- [a1-84] (LBEG)
	l32e	a13,  a1, -24		// a13 <- [a1-88] (LEND)
	l32e	a14,  a1, -28		// a14 <- [a1-92] (LCOUNT)
	l32dis	a11,  a1, 12		// a11 <- [a1-80] (EPC) If interrupt pending
					// go to _Dispatch_tailchain else fall through
# if XCHAL_RH01_ERRATUM
	wsr.lbeg a12
	wsr.lend a13
# endif
#endif  /*****  ABI  *****/

	// Fall through...
	//j	_Restore

_Restore_2:
	// Workaround for LBEG/LEND corruption bug.

	l32e	a12,  a1, -44		// a12 <- [a1-16], (LBEG   <- a12)
	l32e	a13,  a1, -40		// a13 <- [a1-12], (LEND   <- a13)
	rsr.lbeg a8
	rsr.lend a9
	wsr.lbeg a8
	wsr.lend a9
	l32e	a14,  a1, -36		// a14 <- [a1-08], (LCOUNT <- a14)
	l32e	 a8,  a1, -64		// a8  <- [a1-32]
	l32e	 a9,  a1, -64		// a9  <- [a1-28]
	l32e	a10,  a1, -60		// a10 <- [a1-24], (PS/SAR <- a10)
_Dispatch_restore_epc:
	l32e	a11,  a1, -48		// a11 <- [a1-20], (EPC    <- a11)
_Dispatch_restore_epc_live:
	l32e	a15,  a1, -64		// a15 <- [a1-04], branch to EPC
					// If interrupt, branch to Tailchain, don't update a15

#ifdef XTOS_DISABLE_INT_ON_EXC
	// Branch here if handling exceptions. We keep these instructions
	// out of the normal dispatch path to avoid increasing interrupt
	// latency when handling interrupts.
.Ld1:
	movi	a14, PS_DI_MASK
	xps	a14, a14		// Set PS.DI - disable interrupts
	j	.Ld2
#endif


#if XCHAL_RH01_ERRATUM
.LfixTC:
	_nop
	_nop
	_nop
	_nop
 #ifdef __XTENSA_CALL0_ABI__
	l32dis	 a2, a15, 0		// EPC <- handler_table[intnum], a2 <- (intnum << 2)
	s32stk	 a4, a15, 2		// Set new stack pointer
	s32dis	 a0, a15, 0		// Jump to handler if interrupt, else fall through
					// Note this also clears local exclusive monitor
	addi	 a1, a15, 0		// Restore stack pointer
 #else
	l32dis	a10,  a1, 0		// EPC <- handler_table[intnum], a10 <- (intnum << 2)
	s32stk	a12,  a1, 2		// Set new stack pointer
	s32dis	 a8,  a1, 0		// Jump to handler if interrupt else fall through
					// Note this also clears local exclusive monitor
 #endif
	j	_Exit_1			// Start exit process
#endif


	.align	4
JumpToResetHandler:
	j.l	_ResetHandler, a0


//-----------------------------------------------------------------------------
// Symbols for the debugger to use in identifying interrupt / exception frames.
//-----------------------------------------------------------------------------

	.global	_Interrupt
	.set	_Interrupt, _Dispatch_tailchain - 1
	.size	_Interrupt, 1

	.global	_GeneralException
	.set	_GeneralException, _Dispatch_tailchain
	.size	_GeneralException, _Exit - _Dispatch_tailchain

	// Size of interrupt frame
	.global	_InterruptFrameSize
	.set	_InterruptFrameSize, 0x60

	// Register save offset in interrupt frame
	.global	_InterruptRegisterSaveOffset 
	.set	_InterruptRegisterSaveOffset, 0x20

	// EPC offset in interrupt frame
	.global	_InterruptEPCSaveOffset
	.set	_InterruptEPCSaveOffset, 0x10

	// Size of exc frame
	.global	_GeneralExceptionFrameSize
	.set	_GeneralExceptionFrameSize, 0x60

	// Register save offset in exc frame
	.global	_GeneralExceptionRegisterSaveOffset
	.set	_GeneralExceptionRegisterSaveOffset, 0x20

	// EPC offset in exc frame
	.global	_GeneralExceptionEPCSaveOffset
	.set	_GeneralExceptionEPCSaveOffset, 0x10

