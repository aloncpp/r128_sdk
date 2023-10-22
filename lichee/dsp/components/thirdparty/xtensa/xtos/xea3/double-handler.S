// xea3/double-handler.S  --  Xtensa Double Exception handler for XEA3

// Copyright (c) 2016-2017 Tensilica Inc.
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
#include <xtensa/corebits.h>
#include <xtensa/xtruntime-frames.h>
#include <xtensa/xtensa-versions.h>


#if XCHAL_HAVE_XEA3


#define SAVE_OFS	60	/* offset from VecBase to SAVE segment */

#define EXCINF_SHIFT	16	/* bit offset of ExcInf field in EXCCAUSE */

/*  Dispatch states:  */
#define DS_NORMAL	0
#define DS_FIRST	1
#define DS_FRNOINT	2
#define DS_FRINTERRUPT	3
#define DS_CHAIN	5
#define DS_CHNOINT	6
#define DS_CHINTERRUPT	7
#define DS_ABANDON	8
#define DS_COMPLETED	9
#define DS_SAVE		10
#define DS_RESTORE	11
#define DS_SPILLW	12
#define DS_SAVEOVER	13
#define DS_OVERFLOW	14
#define DS_UNDERFLOW	15
#define DS_RESTOREU	27
#define DS_OVERZERO	30
#define DS_RESUNDER	31


	.pushsection  .Double.bss, "a"	// .Double.bss must be safe to access during double handling
			.align	4
_DoubleExceptionData:	.space	DoubleExceptionInfoSize	// space to save registers etc during double exception handling
	.popsection

//	The double vector, at VecBase+6, jumps to the handler like this:
//
//DoubleVector:
//	j	_DoubleExceptionHandler


	.section .DispatchHandler.text, "ax"
	.align	4
	.literal_position		// in case someone uses -mtext-section-literals
	.align	4
	.globl	_DoubleExceptionHandlerFull	// this one's to pull in the full handler specifically
	.globl	_DoubleExceptionHandler
_DoubleExceptionHandlerFull:
_DoubleExceptionHandler:
#if XCHAL_NUM_MISC_REGS >= 1
	wsr.misc0  a3
	movi	a3, _DoubleExceptionData
	s32i	a2, a3, DD_a(2)		// save a2
	rsr.misc0  a2
#elif XCHAL_HAVE_ISB && XCHAL_HAVE_VECBASE
	/*  Hackety HACK -- need to know how to reload ISB, yuck!!!  */
	wsr.isb	a3
	slli	a3, a3, 16
	wsr.vecbase a3
	movi	a3, _DoubleExceptionData
	s32i	a2, a3, DD_a(2)		// save a2
	movi	a2, _DoubleExceptionVector - 6	// reload VecBase (FIXME: use a better symbol)
	xsr.vecbase a2
	srli	a2, a2, 16
	movi	a3, XCHAL_ISB_VADDR	// FIXME: this has to be a symbol or variable, from the OS !!!
	xsr.isb  a3
	srli	a3, a3, 16
	slli	a3, a3, 16
	or	a2, a2, a3		// reconstruct original a3 from two 16-bit pieces
	movi	a3, _DoubleExceptionData
#else
# error "No way to handle double exception without corrupting an AR"
	//  Here's what one might do if VECBASE were a full 32 bits:
	wsr.vecbase  a3
	movi	a3, _DoubleExceptionData
	s32i	a2, a3, DD_a(2)		// save a2
	movi	a2, _DoubleExceptionVector - 6	// reload VecBase (FIXME: use a better symbol)
	xsr.vecbase  a2
#endif
	s32i	a2, a3, DD_a(3)		// save a3
	s32i	a0, a3, DD_a(0)		// save a0
	//s32i	a1, a3, DD_a(1)		// save a1

	rsr.exccause	a0
	rsr.excvaddr	a2
	s32i	a0, a3, DD_cause	// might not be correct, in which case is corrected later (if not halting)
	s32i	a2, a3, DD_vaddr	// ditto
//	s32i	a0, a3, DD_ecause	// ditto
//	s32i	a2, a3, DD_evaddr	// ditto

	rsr.epc	a2
	s32i	a2, a3, DD_pc		// might not be correct, in which case is corrected later (if not halting)
	s32i	a2, a3, DD_rpc		// ditto
//	s32i	a2, a3, DD_epc		// ditto

	//  EPC can normally never point inside the double exception handler, which runs with MS.DE=1
	//  (any exceptions there result in HALT).
	//  However, due to having to manually clear MS.DE upon returning from the double handler,
	//  some parts of the handler do run with MS.DE=0.  So it is indeed possible for EPC to
	//  point within the double handler, and those are triple exception cases.  HALT on those,
	//  rather than confusingly attempt to handle the unrecoverable or end up in a loop.
	//
	// if (&_DoubleExceptionHandler <= EPC < .Ldouble_exception_end) {
	movi	a0, _DoubleExceptionHandler
	bltu	a2, a0, 7f
	movi	a0, .Ldouble_exception_end
	bgeu	a2, a0, 7f
	// Restore what we can
	l32i	a0, a3, DD_a(0)
	l32i	a2, a3, DD_a(2)
	l32i	a3, a3, DD_a(3)
	// FIXME: document immediate in XEA3 Debug ARCH spec.
	halt	15	// sw-detected triple exception: exception while exiting double exception with MS.DE=0


	// }
7:	rsr.ms	a0
	extui	a0, a0, 0, 5		// get DispSt field

#ifdef __XTENSA_WINDOWED_ABI__
	//  Windowed ABI only:  SaveOver
	//  NOTE:  AR windows are flipped !! (but that only affects code reading/dumping ARs)
	// if (DispSt == SaveOver) {
	addi	a0, a0, -1		// (put DS_SAVEOVER in immediate range of BNEI)
	beqi	a0, DS_SAVEOVER-1, 2f	// goto Save handling
	addi	a0, a0, 1		//
	// }
#endif /*windowed*/

	// if (DispSt == Save) {
	bnei	a0, DS_SAVE, 7f		// if (DispSt == Save) {
#if XCHAL_HW_MIN_VERSION == XTENSA_HWVERSION_RH_2016_0
	rsr.exccause a0			// old core erratum... exccause overwritten...
	movi	a2, 0
	//... NOTE: might have to clear a11 too if exccause was loaded in it, skipping that for now ...
	j	1f
#endif
2:	//////////////////////////////////////////////////////////////
	//  Exception in Save or SaveOver sequence.

	//   if (EXCCAUSE.ExcInf[4]) {	// cause lost?
	rsr.exccause a0
	movi	a2, 0
	bbci.l	a0, EXCINF_SHIFT+4, 1f
	s32i	a2, a3, DD_cause	// show that double exception cause itself is unknown
	s32i	a2, a3, DD_vaddr
	j	2f
	//   } else {
	//  When we return to dispatch, if EXCCAUSE/EXCVADDR weren't saved in ARs,
	//  ensure no exception is pending so that dispatch just returns and retries
	//  (usually triggers the exception again, unless external and transient).
1:	wsr.exccause a2
	wsr.excvaddr a2
//	s32i	a2, a3, DD_ecause	// show that original was an interrupt
//	s32i	a2, a3, DD_evaddr
2:	//   }

	//  We can compute DEPC.  Do that:
#ifdef __XTENSA_CALL0_ABI__
	extui	a0, a0, EXCINF_SHIFT, 4	// get DEPC offset from ExcInf[3:0]
	addi	a0, a0, (-SAVE_OFS) & 0xF	// subtract SAVE segment offset, modulo 16
	addx2	a0, a0, a0		// multiply by 3 three times (mult by 27) ...
	addx2	a0, a0, a0		// which multiplied by instruction size (3) ...
	addx2	a0, a0, a0		// is 81:  81 mod 16 == 1, so we get back instruction index
	extui	a0, a0, 0, 4		// mod 16
	// a0 is now 0..15 telling us which 24-bit instruction in SAVE segment took the double exception
#else /* windowed ABI */
	extui a0, a0, EXCINF_SHIFT, 3	// get DEPC offset from ExcInf[2:0]
	addi a0, a0, (-SAVE_OFS) & 7 	// subtract SAVE segment offset, modulo 8
	addx2 a0, a0, a0		// multiply by 3
	extui a0, a0, 0, 3		// mod 8
	// a0 is now 0..7 telling us which 24-bit instruction in SAVE segment took the double exception
#endif /* ABI */

	//  Compute DEPC:
	rsr.vecbase  a2			// compute DEPC = VecBase
	addi	a2, a2, SAVE_OFS	//		+ SAVE_OFS
	add	a2, a0, a2		//		...
	addx2	a2, a0, a2		//		+ (instruction index) * 3
	s32i	a2, a3, DD_pc		// update correct DEPC
	s32i	a2, a3, DD_rpc		// that's where we also return to
	//  Alas, in lots of cases (all cases for windowed, some for call0) we can't
	//  actually JX to this return PC, because there is no free AR in the destination
	//  that can contain it; and amazingly, XEA3 has no generic instruction to return
	//  somewhere without clobbering an AR (L32E has too many constraints to be useful
	//  here; in particular it sets MS.DispSt undesirably, and other quirks).
	movi	a2, XT_DOUBLE_IN_SAVE

	bnez	a0, 3f			// special case if DEPC points to start of SAVE

	//  DEPC points to SAVE + 0 (start of SAVE):
	call0	.Lcall_handler		// common code to handle double exception
	l32i	a0, a3, DD_a(0)		// restore a0
	l32i	a3, a3, DD_a(3)		// restore a3
	j	_Save			// jump to SAVE (_Save label should match VecBase + SAVE_OFS)

3:	bnei	a0, 1, 3f

	//  DEPC points to SAVE + 3:
	call0	.Lcall_handler		// common code to handle double exception
	l32i	a0, a3, DD_a(0)		// restore a0
	l32i	a3, a3, DD_a(3)		// restore a3
	j	_Save+3			// jump to SAVE+3 (_Save label should match VecBase + SAVE_OFS)

#ifdef __XTENSA_CALL0_ABI__
3:
	//  DEPC points past SAVE+3:  in Call0 ABI, turns out it's okay to clobber a2:
	call0	.Lcall_handler		// common code to handle double exception
	l32i	a2, a3, DD_rpc		// where to return to
	l32i	a0, a3, DD_a(0)		// restore a0
	l32i	a3, a3, DD_a(3)		// restore a3
	jx	a2

#else /* but for Windowed ABI we have to handle each offset specifically */

3:	bnei	a0, 2, 3f

	//  DEPC points to SAVE + 6:
	call0	.Lcall_handler		// common code to handle double exception
	l32i	a0, a3, DD_a(0)		// restore a0
	l32i	a3, a3, DD_a(3)		// restore a3
	j	_Save+6			// jump to SAVE+6 (_Save label should match VecBase + SAVE_OFS)

3:	bnei	a0, 3, 3f

	//  DEPC points to SAVE + 9:
	call0	.Lcall_handler		// common code to handle double exception
	l32i	a0, a3, DD_a(0)		// restore a0
	l32i	a3, a3, DD_a(3)		// restore a3
	j	_Save+9			// jump to SAVE+9 (_Save label should match VecBase + SAVE_OFS)

3:	bnei	a0, 4, 3f

	//  DEPC points to SAVE + 12:
	call0	.Lcall_handler		// common code to handle double exception
	l32i	a0, a3, DD_a(0)		// restore a0
	l32i	a3, a3, DD_a(3)		// restore a3
	j	_Save+12		// jump to SAVE+12 (_Save label should match VecBase + SAVE_OFS)

3:	bnei	a0, 5, 3f

	//  DEPC points to SAVE + 15:
	call0	.Lcall_handler		// common code to handle double exception
	l32i	a0, a3, DD_a(0)		// restore a0
	l32i	a3, a3, DD_a(3)		// restore a3
	j	_Save+15		// jump to SAVE+15 (_Save label should match VecBase + SAVE_OFS)

3:	bnei	a0, 6, 3f

	//  DEPC points to SAVE + 18:
	call0	.Lcall_handler		// common code to handle double exception
	l32i	a0, a3, DD_a(0)		// restore a0
	l32i	a3, a3, DD_a(3)		// restore a3
	j	_Save+18		// jump to SAVE+18 (_Save label should match VecBase + SAVE_OFS)

3:	//  DEPC points to SAVE + 21:
	call0	.Lcall_handler		// common code to handle double exception
	l32i	a0, a3, DD_a(0)		// restore a0
	l32i	a3, a3, DD_a(3)		// restore a3
	j	_Save+21		// jump to SAVE+21 (_Save label should match VecBase + SAVE_OFS)

#endif

	// FIXME: if JX is to tailchain, MS.DispSt will get set incorrectly; need to special case that


7:	// } else {		// not (Save | SaveOver)

	//   Here, return PC is directly available as EPC (now in a2), got set on double exception.

	//   if (EPC == &_Dispatch_restore_epc_live) {	// can happen for DispSt == Restore (windowed) or ChNoInt (call0)
	movi	a0, _Dispatch_restore_epc_live
	bne	a2, a0, 7f
	//////////////////////////////////////////////////////////////////////////////////////
	//  Exception at end of Restore/exit sequence, where EPC was live and got clobbered.
	addi	a0, a1, -80
	l32i	a11, a0, 0		// restore a11 (previous EPC) from [a1 - 80]
	addi	a15, a2, -3		// point DEPC to .Ldisp_restore_epc
	s32i	a15, a3, DD_rpc		// where to return to
	movi	a2, XT_DOUBLE_IN_RESTORE
.Ldouble_return_via_a15:
	call0	.Lcall_handler		// common code to handle double exception
	l32i	a15, a3, DD_rpc		// where to return to
	l32i	a0, a3, DD_a(0)		// restore a0
	l32i	a3, a3, DD_a(3)		// restore a3
	jx	a15
//or	j	.Ldisp_restore_epc

	//  Has the double exception occurred during dispatch (awesome threesome)?


7:	rsr.ms	a0
	extui	a0, a0, 0, 5		// get DispSt field
	// } elsif (DispSt == First|FrNoInt) {
	beqi	a0, DS_FIRST, 1f
	bnei	a0, DS_FRNOINT, 7f
1:	//////////////////////////////////////////////////////////////
	//  Exception in ENTRY sequence.  EPC was not live.

	//  Cause was overwritten, but already saved in ARs
#ifdef __XTENSA_CALL0_ABI__
	rsr.exccause a11
	rsr.excvaddr a10
#else /* windowed ABI */
	rsr.exccause a13
	rsr.excvaddr a14
#endif /* ABI */

	//  For other cases below...
#ifdef __XTENSA_CALL0_ABI__
//.Ldouble_return_via_a6:
	movi	a2, XT_DOUBLE_IN_ENTRY
	call0	.Lcall_handler		// common code to handle double exception
	l32i	a6, a3, DD_rpc		// where to return to
	l32i	a0, a3, DD_a(0)		// restore a0
	l32i	a3, a3, DD_a(3)		// restore a3
	jx	a6
#else /* windowed ABI */
	movi	a0, _Dispatch_entry
	beq	a2, a0, 1f		// special case for start of _Dispatch_entry, a12 not available
//.Ldouble_return_via_a12:
	movi	a2, XT_DOUBLE_IN_ENTRY
	call0	.Lcall_handler		// common code to handle double exception
	l32i	a12, a3, DD_rpc		// where to return to
	l32i	a0, a3, DD_a(0)		// restore a0
	l32i	a3, a3, DD_a(3)		// restore a3
	jx	a12
1:	//  Return to _Dispatch_entry:
	movi	a2, XT_DOUBLE_IN_ENTRY
	call0	.Lcall_handler		// common code to handle double exception
	l32i	a0, a3, DD_a(0)		// restore a0
	l32i	a3, a3, DD_a(3)		// restore a3
	j	_Dispatch_entry
#endif /* ABI */


7:	// } elsif (DispSt == ChInterrupt) {
	beqi	a0, DS_CHINTERRUPT, 1f
	//  ChInterrupt:  Cause was overwritten, but already saved on stack and can
	//	stay there, no need to restore.
	// } elsif (DispSt == FrInterrupt) {
	bnei	a0, DS_FRINTERRUPT, 7f
	//  FrInterrupt:  Cause was overwritten, but already saved in ARs, need to restore.
#ifdef __XTENSA_CALL0_ABI__
	rsr.exccause a11
	rsr.excvaddr a10
#else /* windowed ABI */
	rsr.exccause a13
	rsr.excvaddr a14
#endif /* ABI */
1:	///////////////////////////////////////////////////////////////////////////////
	//  Exception in awesome threesome sequence, EPC was live (interrupt handler).

	//  EPC was live in dispatch code (contained interrupt handler address), and overwritten by DEPC.
	//  Recover same way S32DIS does on shortcut for higher priority interrupt:  abandon the previous one.
	rsr.ms	a2
	addi	a0, a0, -DS_ABANDON
	sub	a2, a2, a0
	wsr.ms	a2			// set DispSt = DS_ABANDON
	rsync
	//  Note: no register rotation in this case.

	movi	a2, XT_DOUBLE_IN_THREESOME
.Ldouble_return_via_tailchain:
	call0	.Lcall_handler		// common code to handle double exception
	l32i	a0, a3, DD_a(0)		// restore a0
	l32i	a3, a3, DD_a(3)		// restore a3
	j	_Dispatch_tailchain	// FIXME: in XOS this is _xt_dispatch



	// } else /*Completed|Abandon|Chain|ChNoInt|Restore|RestoreU|ResUnder*/ {
7:	///////////////////////////////////////////////////////////////////////////////
	//  All other double exceptions.  EPC was not live.

	//  Cause was overwritten, but already saved on stack and can stay there, no need to restore
	//TODO:  load EXCSAVE/EXCVADDR from stack and store to [a3+DD_ecause/evaddr] ?
#ifdef __XTENSA_CALL0_ABI__
	// nothing?
#else /* windowed ABI */
	// nothing?
#endif /* ABI */

	//  Completed|Abandon|Chain :  always just jump back to Tailchain ?!
	//  ChNoInt|Restore :  use a15 ?!

	movi	a0, _Dispatch_tailchain
	bne	a2, a0, 1f
	movi	a2, XT_DOUBLE_AT_TAILCHAIN
	j	.Ldouble_return_via_tailchain

1:	movi	a2, XT_DOUBLE_IN_EXIT
	j	.Ldouble_return_via_a15

	// }


	//  Common code to handle various cases of double exceptions.
	//  On entry:
	//	MS.DE = 1
	//	a0 = return PC (that correctly returns from the double exception for this case)
	//	a2 = which case of double exception this is (???)
	//	a3 = _DoubleExceptionData
	//	a1, a4..a15 = arbitrary, callee-saved
	//	original a0 in [_DoubleExceptionData+DD_a(0)]
	//	original a2 in [_DoubleExceptionData+DD_a(2)]
	//	original a3 in [_DoubleExceptionData+DD_a(3)]
	//	other info  in [_DoubleExceptionData+DD_***]
	//  On exit:
	//	MS.DE = 1
	//	a0 = (return) PC
	//	a1, a2, a4..a15 restored
	//	a3 = _DoubleExceptionData
	//	original a0 in [_DoubleExceptionData+DD_a(0)]
	//	original a3 in [_DoubleExceptionData+DD_a(3)]
	.align 4
.Lcall_handler:
	s32i	a0, a3, DD_ret
	s32i	a1, a3, DD_a(1)
	s32i	a4, a3, DD_a(4)
	s32i	a5, a3, DD_a(5)
	s32i	a6, a3, DD_a(6)
	s32i	a7, a3, DD_a(7)
	s32i	a8, a3, DD_a(8)
	s32i	a9, a3, DD_a(9)
	s32i	a10, a3, DD_a(10)
	s32i	a11, a3, DD_a(11)
	s32i	a12, a3, DD_a(12)	// a12-a15 are saved for debug only; call0 ABI preserves them
	s32i	a13, a3, DD_a(13)
	s32i	a14, a3, DD_a(14)
	s32i	a15, a3, DD_a(15)
#if XCHAL_HAVE_LOOPS
	rsr.lbeg	a8
	rsr.lend	a9
	rsr.lcount	a10
	s32i	a8,  a3, DD_lbeg
	s32i	a9,  a3, DD_lend
	s32i	a10, a3, DD_lcount
#endif
	rsr.sar	a0
	s32i	a0, a3, DD_sar
	movi	a1, _DoubleExceptionStackBase
	call0	_DoubleExceptionCall0Handler	// user-provided double exception handler (C, call0 ABI)
	movi	a3, _DoubleExceptionData
	l32i	a0, a3, DD_sar
	wsr.sar	a0
#if XCHAL_HAVE_LOOPS
	l32i	a8,  a3, DD_lbeg
	l32i	a9,  a3, DD_lend
	l32i	a10, a3, DD_lcount
	wsr.lbeg	a8
	wsr.lend	a9
	wsr.lcount	a10
#endif
	l32i	a1, a3, DD_a(1)
	l32i	a4, a3, DD_a(4)
	l32i	a5, a3, DD_a(5)
	l32i	a6, a3, DD_a(6)
	l32i	a7, a3, DD_a(7)
	l32i	a8, a3, DD_a(8)
	l32i	a9, a3, DD_a(9)
	l32i	a10, a3, DD_a(10)
	l32i	a11, a3, DD_a(11)
	l32i	a0, a3, DD_ret

	rsr.ms  a2
	addi	a2, a2, -MS_DE		// clear MS.DE
	wsr.ms	a2
	rsync
	//  NOTE:  from here thru return, MS.DE=0, double exceptions are possible
	//  (if any occur, the double handler will detect it via EPC range and HALT early;
	//   EXCEPT in the case of return to Save|SaveOver !?!?)
	l32i	a2, a3, DD_a(2)
	ret

.Ldouble_exception_end:
	.size   _DoubleExceptionHandler, . - _DoubleExceptionHandler


#if 0
	//  Dummy function, which if invoked, pulls in this full implementation
	//  of the double exception handler.
	.text
	.align	4
	.globl	_xtos_double_handling
	.type	_xtos_double_handling,@function
_xtos_double_handling:
	abi_entry
	abi_return
	.size   _xtos_double_handling, . - _xtos_double_handling
#endif


#endif /* XEA3 */

