// user-vector.S  -  User Vector for General Exceptions

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

#include <xtensa/coreasm.h>
#include <xtensa/config/system.h>
#include "xtos-internal.h"

#if XCHAL_HAVE_EXCEPTIONS

	// Vector code
	.section		.UserExceptionVector.text, "ax"
	.align 4
	.global _UserExceptionVector
_UserExceptionVector:
# if (((XSHAL_USER_VECTOR_SIZE >= 28) && XCHAL_HAVE_ADDX && XCHAL_HAVE_DENSITY && XCHAL_HAVE_L32R) || (XSHAL_USER_VECTOR_SIZE >= 36)) && !defined (XSHAL_ERRATUM_487_FIX)
	//  There is space to dispatch right at the vector:

	addi	a1, a1, -ESF_TOTALSIZE	// allocate exception stack frame, etc.
	s32i	a2, a1, UEXC_a2
	s32i	a3, a1, UEXC_a3
	movi	a3, xtos_exc_handler_table
	rsr.exccause	a2		// get exception cause
	//interlock
	addx4	a3, a2, a3
	l32i	a3, a3, 0
	s32i	a4, a1, UEXC_a4
	jx	a3			// jump to cause-specific handler

	.size	_UserExceptionVector, . - _UserExceptionVector

# else
	//  The vector may be as small as 12 bytes:

	addi	a1, a1, -ESF_TOTALSIZE		// allocate exception stack frame, etc.
	s32i	a2, a1, UEXC_a2
	movi	a2, _UserExceptionFromVector	// load user exception handler address
	//interlock
	jx	a2				// jump to handler

	.size	_UserExceptionVector, . - _UserExceptionVector

	//  Dispatch outside vector:
	.text
	//.subsection 2
	.align	4
	.global	_UserExceptionFromVector
_UserExceptionFromVector:
	hw_erratum_487_fix
	s32i	a3, a1, UEXC_a3
	movi	a3, xtos_exc_handler_table
	rsr.exccause	a2		// get exception cause
	s32i	a4, a1, UEXC_a4
	addx4	a3, a2, a3
	l32i	a3, a3, 0
	jx	a3			// jump to cause-specific handler

	.size	_UserExceptionFromVector, . - _UserExceptionFromVector

# endif


	.weak	xtos_cause3_handler

	/*
	 *  Table of assembly-level general-exception handlers
	 *  (quickly entered) for user vectored exceptions.
	 *  Provides entries for all possible 64 exception causes
	 *  currently allowed for in the EXCCAUSE register.
	 *
	 *  NOTE:  entries that have a corresponding C handler
	 *  (registered at run-time) point to xtos_c_wrapper_handler;
	 *  entries that have no handler point to xtos_unhandled_exception.
	 */

	.data

	.global	xtos_exc_handler_table
	.align	4

xtos_exc_handler_table:
	.word	xtos_unhandled_exception	// 0 IllegalInstruction
	.word	_xtos_syscall_handler		// 1 Syscall
	.word	xtos_unhandled_exception	// 2 InstructionFetchError
	.word	xtos_unhandled_exception	// 3 LoadStoreError
# if XCHAL_HAVE_INTERRUPTS
	.word	_xtos_l1int_handler		// 4 Level1Interrupt
# else
	.word	xtos_unhandled_exception	// 4 Level1Interrupt (not configured)
# endif
# if XCHAL_HAVE_WINDOWED && !defined(__XTENSA_CALL0_ABI__)
	.word	_xtos_alloca_handler		// 5 Alloca (MOVSP)
# else
	.word	xtos_unhandled_exception	// 5 Alloca (MOVSP) (not configured)
# endif
	.word	xtos_unhandled_exception	// 6 IntegerDivideByZero
	.word	xtos_unhandled_exception	// 7 Speculation
	.word	xtos_unhandled_exception	// 8 Privileged
	.word	xtos_unhandled_exception	// 9 Unaligned
	.word	xtos_unhandled_exception	//10 (reserved for Tensilica)
	.word	xtos_unhandled_exception	//11 (reserved for Tensilica)
	.word	xtos_cause3_handler		//12 PIF data error on fetch
	.word	xtos_cause3_handler		//13 PIF data error on ld/st
	.word	xtos_cause3_handler		//14 PIF address error on fetch
	.word	xtos_cause3_handler		//15 PIF address error on ld/st
	.word	xtos_unhandled_exception	//16 InstTLBMiss
	.word	xtos_unhandled_exception	//17 InstTLBMultiHit
	.word	xtos_unhandled_exception	//18 InstFetchPrivilege
	.word	xtos_unhandled_exception	//19 (reserved for Tensilica)
	.word	xtos_unhandled_exception	//20 InstFetchProhibited
	.word	xtos_unhandled_exception	//21 (reserved for Tensilica)
	.word	xtos_unhandled_exception	//22 (reserved for Tensilica)
	.word	xtos_unhandled_exception	//23 (reserved for Tensilica)
	.word	xtos_unhandled_exception	//24 LoadStoreTLBMiss
	.word	xtos_unhandled_exception	//25 LoadStoreTLBMultiHit
	.word	xtos_unhandled_exception	//26 LoadStorePrivilege
	.word	xtos_unhandled_exception	//27 (reserved for Tensilica)
	.word	xtos_unhandled_exception	//28 LoadProhibited
	.word	xtos_unhandled_exception	//29 StoreProhibited
	.word	xtos_unhandled_exception	//30 (reserved for Tensilica)
	.word	xtos_unhandled_exception	//31 (reserved for Tensilica)
	.rept	8
	.word	xtos_unhandled_exception	//32-39 Coprocessor<n>Disabled (n = 0..7)
	.endr

	.size	xtos_exc_handler_table, . - xtos_exc_handler_table

	.text

	//  NOTES:
	//
	//  Here are alternative vectors.  They will NOT work with
	//  the handlers currently provided with XTOS.  However they
	//  might be useful to someone writing their own handlers
	//  from scratch.  Note that XSR is only available on T1040
	//  and later hardware.
	//
//***  The typical tiny 9-byte vector:  ***
//	wsr.excsave1	a3			// save user a3
//	movi	a3, _UserExceptionFromVector	// load user exception handler address
//	jx	a3
//
//***  Minimizing EXCCAUSE-dispatch delay, not assuming valid SP:  ***
//	wsr.depc	a0	// save a0 (double exceptions fatal here, so not expected)
//	rsr.exccause	a0
//	xsr.excsave1	a1	// EXCSAVE1 always contains &exception_handlers[0]
//	//interlock
//	addx4	a0, a0, a1
//	l32i	a0, a0, TABLE_OFS + EXC_CODE_KERNEL*4
//	xsr.excsave1	a1	// restore a1 (DEPC contains original a0)
//	jx	a0		// jump to cause-specific handler
//
//***  Doing EXCCAUSE-dispatch with table in EXCSAVE1:  ***
//	addi	a1, a1, -ESF_TOTALSIZE	// allocate exception stack frame, etc.
//	s32i	a2, a1, UEXC_a2
//	rsr.exccause	a2
//	xsr.excsave1	a4	// EXCSAVE1 always contains &exception_handlers[0]
//	s32i	a3, a1, UEXC_a3
//	addx4	a2, a2, a4
//	l32i	a2, a2, TABLE_OFS + EXC_CODE_KERNEL*4
//	xsr.excsave1	a4	// restore a1 (DEPC contains original a0)
//	jx	a2		// jump to cause-specific handler

#endif /* XCHAL_HAVE_EXCEPTIONS */

