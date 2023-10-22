/* memerror-vector.S  --  Memory Error Exception Vector and Handler */

/*
 * Copyright (c) 2006-2018 Cadence Design Systems, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/*
 * This handler supports multiple user hooks to handle various cases.
 * This is the list of available hooks:
 *
 * _xtos_merr_hook_fatal_dme     -- fatal error, double memory exception
 * _xtos_merr_hook_uncorrectable_local -- fatal error, uncorrectable error in IRAM/DRAM
 * _xtos_merr_hook_uncor_dtag    -- fatal error, uncorrectable error in dcache tag
 * _xtos_merr_hook_uncor_dirty   -- fatal error, uncorrectable error in dirty dcache data
 * _xtos_merr_hook_icache_relock -- non-fatal, hook to relock icache
 * _xtos_merr_hook_dcache_relock -- non-fatal, hook to relock dcache
 * _xtos_merr_hook_nonfatal      -- non-fatal, correctable error
 *
 * They must be written in assembler and most of them only have a0 available
 * initially.  See the code below to see exactly how each is invoked.
 */


#include <xtensa/coreasm.h>
#include <xtensa/corebits.h>

#define ICACHE_WAYWIDTH	(XCHAL_ICACHE_SETWIDTH + XCHAL_ICACHE_LINEWIDTH)	/* LICT's "iis" */
#define DCACHE_WAYWIDTH	(XCHAL_DCACHE_SETWIDTH + XCHAL_DCACHE_LINEWIDTH)	/* LDCT's "dis" */
/*  NOTE:  Memory ECC/parity is not supported on XLMI or on local ROMs:  */
#define HAVE_LOCAL_RAM	(XCHAL_NUM_DATARAM || XCHAL_NUM_INSTRAM)


#if XCHAL_HAVE_MEM_ECC_PARITY
# if defined(__SPLIT__vector)

	//  Place this code in the memory error exception vector:
	.begin	literal_prefix	.MemoryExceptionVector
	.section		.MemoryExceptionVector.text, "ax"

	.global _MemErrorVector
	.align 4

_MemErrorVector:
	hw_erratum_487_fix

	//  Space for literals is not guaranteed for the memory error vector
	//  so we have to make room for them here.
	//  This file is built with -mtext-section-literals .

	j	1f

	.align	4
	.literal_position
1:

	//  I-cache errors are handled in the vector itself, which must be
	//  located in uncached memory if I-cache is configured.  This avoids
	//  having I-cache tag errors cause infinite exception loops.
	//  (Hooks however, are invoked in the handler to properly handle the
	//  multicore case, except for DME.)
	//
	//  Double memory errors (DME bit set) are also handled in the vector
	//  itself, for similar reasons.

	wsr.mesave	a0
	rsr.mesr	a0
	bbsi.l	a0, MESR_DME_SHIFT, .L_fatal_dme
# if XCHAL_ICACHE_SIZE > 0 && XCHAL_HAVE_ICACHE_TEST
	bbci.l	a0, MESR_MEMTYPE_SHIFT+1, .L_not_icache	// branch if not error on a cache
#  if XCHAL_DCACHE_SIZE > 0
	bbci.l	a0, MESR_MEMTYPE_SHIFT, .L_icache	// branch if instruction cache error
#  else
	j	.L_icache
#  endif
.L_not_icache:
	//  Error in a local memory or data cache.
# endif


	//  Go to the memory error handler.
	//  It can run in regular i-cached memory where there is more code space.

.L_i_done:
# if XCHAL_HAVE_VECBASE
	//  Handle the case of multiple cores of the same configuration sharing
	//  the same memory error vector (by necessity because the memory error
	//  vector is non-relocatable so as to allow it to be uncached by being
	//  away from relocatable vectors which are normally cached).
	//
	//  Pickup the address of the handler from kernel vector + 4 (vecbase
	//  relative) and jump there.  That way, symbols will be the correct
	//  ones in the handler for that particular core's program image.
	//
	rsr.vecbase	a0
#  if XCHAL_KERNEL_VECOFS+4 <= 1020
	l32i	a0, a0, XCHAL_KERNEL_VECOFS + 4		// see kernel-vector.S
#  else
	addi	a0, a0, XCHAL_KERNEL_VECOFS + 4		// see kernel-vector.S
	l32i	a0, a0, 0
#  endif
# else
	//  This version works with a single program image.
	//  (Note that properly handling multiple cores of the same config
	//  with independent program images normally requires relocatable
	//  vectors aka VECBASE.)

	movi	a0, _MemErrorHandler
# endif
	jx	a0		// jump to _MemErrorHandler


	//
	//  Fatal (unrecoverable) error, double memory exception
	//
	//  Invoke hook or loop forever.
	//
	//  LIMITATION:  in the case of multiple cores sharing the same memory
	//  error vector, each with an independent program image, there is only
	//  one version of the DME hook rather than one per program image.
	//

	//  Weak reference:  if unresolved, links okay but with zero value:
	.weak	_xtos_merr_hook_fatal_dme
.L_fatal_dme:
	movi	a0, _xtos_merr_hook_fatal_dme
1:
	beqz	a0, 1b		// fatal double memory error, no hook, so infinite loop
	jx	a0		// jump to user hook, if present



	//
	//  Error in the instruction cache.
	//

# if XCHAL_ICACHE_SIZE > 0 && XCHAL_HAVE_ICACHE_TEST
.L_icache:
	bbsi.l	a0, MESR_ERRTYPE_SHIFT, .L_icache_noncorr	// branch if uncorrectable
	//  Correctable error in the instruction cache.
	xsr.mevaddr	a2
	//  TODO FIXME: remove these 5 lines if waynum is in MEVADDR!? by using III if tag and IHI otherwise!?!?!?:
#  if XCHAL_ICACHE_WAYS > 1
	extui	a0, a0, MESR_WAYNUM_SHIFT, 2
	slli	a0, a0, ICACHE_WAYWIDTH
	slli	a2, a2, 32 - ICACHE_WAYWIDTH
	srli	a2, a2, 32 - ICACHE_WAYWIDTH
	or	a2, a2, a0
#  endif
#  if XCHAL_ICACHE_LINE_LOCKABLE
	//  Preserve the lock bit.  So check the tag...
	lict	a0, a2			// load i-cache tag
	bbci.l	a0, XCHAL_ICACHE_TAG_L_SHIFT, .L_icache_corr_unlocked	// branch if unlocked
	//  Correctable error in a locked instruction cache line.
	//  Fix both tag and one word, quicker than figuring out whether error is in tag or data:
	sict	a0, a2			// fix tag
	//  TODO FIXME: on fetch wider than 32-bits, SICW might replicate its 32 bits to the
	//  whole fetch width rather than just write 32-bits, depending on how customer wires up
	//  I-cache RAMs.  With ECC option and I-cache locking, they need 32-bit word write enables.
	licw	a0, a2
	sicw	a0, a2			// fix data word
	j	.L_icache_done
.L_icache_corr_unlocked:
	//  We have to write the whole tag to avoid hitting an error here (if tag error).
	//  So use IIU (which also invalidates) not III (which reads L bit so can hit error).
	iiu	a2, 0		// unlock line ==> also invalidates! (I-side only)
#  else
	iii	a2, 0			// invalidate line (whole set!)
#  endif
.L_icache_done:
	xsr.mevaddr	a2
	j	.L_i_done

.L_icache_noncorr:
	//  Non-correctable error in the instruction cache.
	bbsi.l	a0, MESR_MEMTYPE_SHIFT+2, .L_icache_tag_noncorr	// branch if tag error
	//  Non-correctable error in the instruction cache data.
	//  Just invalidate the line if we can.
#  if XCHAL_ICACHE_LINE_LOCKABLE
	//  If locked, need a different fix sequence.
	xsr.mevaddr	a2

#   if XCHAL_ICACHE_WAYS > 1
	//  This sequence is shorter, but does not retain original MEVADDR so
	//  prevents subsequent use of instructions requiring a virtual address
	//  (such as LICW, IPFL, etc):
//	extui	a0, a0, MESR_WAYNUM_SHIFT, 2
//	slli	a0, a0, ICACHE_WAYWIDTH
//	slli	a2, a2, 32 - ICACHE_WAYWIDTH
//	srli	a2, a2, 32 - ICACHE_WAYWIDTH
//	or	a2, a2, a0

	extui	a0, a0, MESR_WAYNUM_SHIFT, 2	// id of way with mem error
	slli	a0, a0, ICACHE_WAYWIDTH
	xor	a0, a2, a0			// xor corresponding bits of addr
	extui	a0, a0, ICACHE_WAYWIDTH, 2	// take 2 xor'ed way bits
	or	a2, a2, a0			// save them at bottom of addr
	slli	a0, a0, ICACHE_WAYWIDTH
	xor	a2, a2, a0			// and change 2 way bits of addr
#   endif
	lict	a0, a2
	bbsi.l	a0, XCHAL_ICACHE_TAG_L_SHIFT, .L_icache_locked_uncor	// branch if locked
	//  Cache line is not locked, just invalidate:
#   if XCHAL_ICACHE_WAYS > 1
	iii	a2, 0
#   else
	ihi	a2, 0
#   endif
	j	.L_icache_done

	//  NOTE:  we don't use the LICW/SICW sequence below unless the line is locked,
	//	otherwise the i-cache line might get replaced between LICW and SICW
	//	(if we're not extremely careful), which would be disastrous.
	//	Also, for locked lines, LICW/SICW is much safer than IHU/IHI/IPFL
	//	because it doesn't leave a window where the line is unlocked;
	//	however, if the error is non-correctable, we have no choice.

.L_icache_locked_uncor:
	//  If locked and uncorrectable however, the only recourse is relocking.
	//  So we need to recover the virtual address so we can do IPFL.
	//  Note:  can't use MEPC instead of MEVADDR, because (a) it might not
	//  point to the correct cache line, and (b) it might be completely wrong
	//  in the case where the mem error happened e.g. during an LICW or IPFL.
#   if XCHAL_ICACHE_WAYS > 1
	//  Recover virtual address in a2:
	extui	a0, a2, 0, 2			// get saved xor'ed bits at bottom
	slli	a0, a0, ICACHE_WAYWIDTH		// line them up
	xor	a2, a2, a0			// restore original MEVADDR
#   endif
	ihu	a2, 0		// unlock line
	ihi	a2, 0		// invalidate line
	ipfl	a2, 0		// refetch-and-lock the line
	j	.L_icache_done
#  else /* LOCKABLE */
	rsr.mevaddr	a0
	ihi	a0, 0		// invalidate that cache line
	j	.L_i_done
#  endif /* LOCKABLE */

.L_icache_tag_noncorr:
	//  Non-correctable error in the instruction cache tag.
	//  Just invalidate the tag or the entire set.
#  if XCHAL_ICACHE_LINE_LOCKABLE
	//  Note:
	//  With i-cache locking, IIU writes the entire tag without mem-error check,
	//  and III writes part of it (leaves lock bit alone) so can hit errors.
	//  Without i-cache locking, III writes the entire tag without mem-error check.
	//  (Original assumption was that SICT is needed.)
#   if XCHAL_ICACHE_WAYS > 1
	//  NOTE possible optimization: avoid this 8-line alternative if waynum is in MEVADDR!?:
	xsr.mevaddr	a2
	extui	a0, a0, MESR_WAYNUM_SHIFT, 2
	slli	a0, a0, ICACHE_WAYWIDTH
	slli	a2, a2, 32 - ICACHE_WAYWIDTH
	srli	a2, a2, 32 - ICACHE_WAYWIDTH
	or	a2, a2, a0
	iiu	a2, 0		// unlock line ==> also invalidates! (I-side only)
	xsr.mevaddr	a2
#   else
	rsr.mevaddr	a0
	iiu	a0, 0		// unlock line ==> also invalidates! (I-side only)
#   endif
	// If line was locked, can't recover lock state, need external info to recover.
	// User can provide an assembler hook routine _xtos_merr_hook_icache_relock,
	// this is invoked in the memory error handler.
#  else
	rsr.mevaddr	a0
	iii	a0, 0		// invalidate entire set
#  endif
	j	.L_i_done
# endif /* have ICACHE */

	.size	_MemErrorVector, . - _MemErrorVector
	.text
	.end	literal_prefix


# elif defined(__SPLIT__handler)

/*
 *  Some rules and assumptions:
 *
 *	Anything that can interrupt this handler (e.g. NMI):
 *		- must not lock or unlock cache lines
 */


#if XCHAL_DCACHE_LINE_LOCKABLE && (XCHAL_DCACHE_WAYS > 1) && XCHAL_HAVE_PREFETCH
	.comm	_MemErrorSave, 12, 4
#else
	.comm	_MemErrorSave, 8, 4
#endif

	.text
	.align	4
	.literal_position

	.align	4
	.global	_MemErrorHandler

_MemErrorHandler:
	hw_erratum_487_fix
	rsr.mesr	a0
# if XCHAL_ICACHE_SIZE > 0 || XCHAL_DCACHE_SIZE > 0
	bbsi.l	a0, MESR_MEMTYPE_SHIFT+1, .L_cache	// branch if error on a cache
# endif
	//  Error in a local memory.
# if HAVE_LOCAL_RAM
	bbsi.l	a0, MESR_ERRTYPE_SHIFT, .L_uncorrectable_local
	//  Correctable error in a local memory (IRAM or DRAM).
	//  (MEVADDR has all 32 bits, so XSR preserves a register:)
	xsr.mevaddr	a2

	//  NOTE: MEVADDR is always 4-byte aligned, so we could just do L32I/S32I
	//  to correct the error. However, that sequence is not atomic, and an NMI
	//  handler could store to the same location in between. That's usually a
	//  problem for D rather than I. Avoid the issue if we have hw support
	//  (else NMI handler should not write dataram?).

#  if XCHAL_NUM_DATARAM
	bbci.l  a0, MESR_MEMTYPE_SHIFT, .L_instram	// branch if error on InstRAM
#   if (XCHAL_HAVE_S32C1I && !XCHAL_HAVE_EXCLUSIVE)
	//  Unfortunately we need 3 registers to do S32C1I (data,addr,SCOMPARE1) so
	//  we need to save to _MemErrorSave:
	movi	a0, _MemErrorSave
	s32i	a4, a0, 0	// save a4
	l32i	a4, a2, 0	// load data (re-correct)
	rsr.scompare1	a0	// save SCOMPARE1
	wsr.scompare1	a4
	s32c1i	a4, a2, 0	// store if still contains same value (else other store corrected error)
	movi	a4, _MemErrorSave
	wsr.scompare1	a0	// restore SCOMPARE1
	l32i	a4, a4, 0	// restore a4
	j	2f
#   endif /* XCHAL_HAVE_S32C1I */

#   if XCHAL_HAVE_EXCLUSIVE
	// a2 has address, a0 is available.
	l32ex	a0, a2		// load data (re-correct)
	s32ex	a0, a2		// store if not modified
	j	2f
#   endif /* XCHAL_HAVE_EXCLUSIVE */
#  endif /* XCHAL_NUM_DATARAM */

.L_instram:
	l32i	a0, a2, 0	// load data (re-correct)
	s32i	a0, a2, 0	// store data to correct ECC bits
2:
	xsr.mevaddr	a2
# endif /* HAVE_LOCAL_RAM */


	//  Weak reference:  if unresolved, links okay but with zero value:
	//
	//  This hook must return using a RET instruction. It will receive
	//  the return address in a0, and the MESR register value in a2.
	//  The hook may use and trash a2 and a3, but all other registers
	//  must be preserved.

	.weak	_xtos_merr_hook_nonfatal

.L_done:
	//  Finished, now see if there is a user hook to call before returning
	movi	a0, _xtos_merr_hook_nonfatal	// hook address
	beqz	a0, 1f				// no hook, return
	movi	a0, _MemErrorSave		// save area address
	s32i	a2, a0, 0			// save a2
	s32i	a3, a0, 4			// save a3
	movi	a0, _xtos_merr_hook_nonfatal	// re-load hook address
	rsr.mesr	a2			// pass MESR value as arg
	callx0	a0				// call user hook
	movi	a3, _MemErrorSave
	l32i	a2, a3, 0			// restore a2
	l32i	a3, a3, 4			// restore a3
1:
	rsr.mesave	a0			// No need to store handler address
	rfme


# if HAVE_LOCAL_RAM
	//  Weak reference:  if unresolved, links okay but with zero value:
	.weak	_xtos_merr_hook_uncorrectable_local
.L_uncorrectable_local:
	//  Fatal (unrecoverable) error in IRAM or DRAM:  parity or uncorrectable ECC error
	movi	a0, _xtos_merr_hook_uncorrectable_local
1:
	beqz	a0, 1b		// fatal memory error, no hook provided, so infinite loop
	jx	a0		// jump to user hook, if present
# endif


# if XCHAL_ICACHE_SIZE > 0 || XCHAL_DCACHE_SIZE > 0
.L_cache:
	//  Error in one of the caches.
# endif

# if XCHAL_ICACHE_SIZE > 0 && XCHAL_HAVE_ICACHE_TEST
#  if XCHAL_DCACHE_SIZE > 0 && XCHAL_HAVE_DCACHE_TEST
	bbsi.l	a0, MESR_MEMTYPE_SHIFT, .L_dcache	// branch if data cache error
#  endif
	//  Error in the instruction cache.
	//  All such errors were already handled in the vector.
	//  If the vector jumped to here, it's to check whether we need to call a hook
	//  (to handle the multiple-core with shared memory error vector and independent
	//  program images case, the hooks need to be called in the handler, not the vector).
	//
#  if XCHAL_ICACHE_LINE_LOCKABLE
	bbci.l	a0, MESR_ERRTYPE_SHIFT, .L_done		// branch if correctable
	//  Non-correctable error in the instruction cache.
	bbci.l	a0, MESR_MEMTYPE_SHIFT+2, .L_done	// branch if data error
	//  Non-correctable error in the instruction cache tag.

	// If line was locked, can't recover lock state, need external info to recover.
	// User can provide an assembler hook routine _xtos_merr_hook_icache_relock
	// to relock the icache at the index in a2:
	//	- any number of lines might still be locked at that index,
	//	  including all of them
	//	- no stack is provided, a0 must be used as starting point to
	//	  load a save area and saved registers as necessary
	//	- unless routine just does ret (i.e. does not modify any
	//	  register, only possible if it does nothing), it needs to
	//	  return by restoring all registers it modified, ending with:
	//		rsr.mesave	a0
	//		rfme
	//  CAVEAT EMPTOR:  this hook mechanism is subject to change.
	//
	//  Invoke hook or loop forever.
	//
	.weak	_xtos_merr_hook_icache_relock	// if unresolved, links with zero value
	movi	a0, _xtos_merr_hook_icache_relock
1:
	beqz	a0, 1b		// if no hook to recover lock state on icache tag mem error, loop forever
	callx0	a0		// invoke user hook to relock i-cache (index in MEVADDR)
#  endif
	j	.L_done
# endif /* have ICACHE */


# if XCHAL_DCACHE_SIZE > 0 && XCHAL_HAVE_DCACHE_TEST
#  if XCHAL_ICACHE_SIZE > 0 && XCHAL_HAVE_ICACHE_TEST
.L_dcache:
#  endif
	//  Error in the data cache.
#  if XCHAL_DCACHE_IS_WRITEBACK || XCHAL_DCACHE_LINE_LOCKABLE
	bbsi.l	a0, MESR_ERRTYPE_SHIFT, .L_dcache_noncorr	// branch if uncorrectable
	//  Uncorrectable error on a writeback dcache might be unrecoverable:
#  endif
	bbsi.l	a0, MESR_MEMTYPE_SHIFT+2, .L_dcache_tag		// branch if tag error
	//  Error in the data cache data (correctable, or non-correctable in writethru+unlockable cache).
	//  MEVADDR always a real vaddr here; might point to cache-isolate mode area though.
#  if XCHAL_DCACHE_LINE_LOCKABLE
	//  Correctable error on lockable dcache data.
	//  If locked, need to refetch the line (or load/store its contents, which is less safe):
	xsr.mevaddr	a2
#   if XCHAL_DCACHE_WAYS > 1
	//  Need some extra computation to get the correct dcache way's tag:
	movi	a0, _MemErrorSave
	s32i	a4, a0, 0	// save a4
	s32i	a5, a0, 4	// save a5
#    if XCHAL_HAVE_PREFETCH
	s32i	a6, a0, 8	// save a6
	movi	a6, 0
	xsr.prefctl a6		// disable prefetch during LDCT (issue TENX-24760)
#    endif
	rsr.mesr	a4
	extui	a4, a4, MESR_WAYNUM_SHIFT, 2
	slli	a4, a4, DCACHE_WAYWIDTH
	slli	a5, a2, 32 - DCACHE_WAYWIDTH
	srli	a5, a5, 32 - DCACHE_WAYWIDTH
	add	a4, a4, a5
	mov	a5, a0
	ldct	a0, a4
	l32i	a4, a5, 0	// restore a4
#    if XCHAL_HAVE_PREFETCH
	wsr.prefctl a6		// restore prefetch
	l32i	a6, a5, 8	// restore a6
#    endif
	l32i	a5, a5, 4	// restore a5
#   else
	//  Direct-mapped cache (XCHAL_DCACHE_WAYS == 1):
#    if XCHAL_HAVE_PREFETCH
	movi	a0, _MemErrorSave
	s32i	a4, a0, 0	// save a4
	movi	a4, 0
	xsr.prefctl a4		// disable prefetch during LDCT (issue TENX-24760)
#    endif
	ldct	a0, a2		// invalidate and unlock that cache tag
#    if XCHAL_HAVE_PREFETCH
	wsr.prefctl a4		// restore prefetch
	movi	a4, _MemErrorSave
	l32i	a4, a4, 0	// restore a4
#    endif
#   endif /* XCHAL_DCACHE_WAYS */
	//  FIXME:  if castout, a2 is a cache index (see PR 24103), from which
	//	we can construct a physical address! need that paddr reconstruction,
	//	and doesn't work with any address translation.
#   if 0 /* possible future support of translation */
	movi	a4, _xtos_vmap_vaddr	// TBD: do we need two variables for full MMU?
1:
	beqz	a4, 1b		// if no vaddr to use, loop forever (TBD: caxlt: could assume V==P)
	rdtlb1	a5, a4		// save current contents
	... clear lower bits of a4 ...
	xx = some function of a2
	wdtlb	xx, a4
	a2 = virtual address, i.e. some function of a2 and a4 ...
	... do the sequence below ...
	...
	wdtlb	a5, a4		// restore TLB entry
#   endif
	//  NOTE:  the following sequence leaves the line temporarily unlocked, if locked.
	//  We assume NMI handlers don't lock lines or rely on their being locked.
	//  We could have used "l32i a0,a2,0; s32i a0,a2,0" but that's not atomic on the data.
	dhu	a2, 0		// unlock the cache line, if locked
	dhwbi	a2, 0		// writeback and invalidate cache line
	bbci.l	a0, XCHAL_DCACHE_TAG_L_SHIFT, 1f
	dpfl	a2, 0		// re-prefetch-and-lock the cache line
1:
	xsr.mevaddr	a2
#  else /* LOCKABLE */
	//  Error in unlockable data cache data (correctable, or non-correctable in writethru cache).
	rsr.mevaddr	a0
	//  USELESS NOTE:  if writethru dcache and NMI handlers don't store to this, we could use DHI instead:
	//  FIXME:  if castout, a0 is a physical address! doesn't work with any address translation.
	dhwbi	a0, 0		// writeback (if correctable) and invalidate that cache line
#  endif /* LOCKABLE */
	j	.L_done

.L_dcache_tag:
	//  Error in data cache tag (correctable, or non-correctable in writethru+unlockable cache).
	//  MEVADDR only contains cache index here (not waynum), don't expect a vaddr (the ISA
	//  says upper bits are undefined; actual hw does put a vaddr, but in future might not).
	//  Whether or not correctable, just invalidate the particular way's line:
	xsr.mevaddr	a2
	//  NOTE: could remove these 5 lines if hw were designed with waynum in MEVADDR (but is not):
#  if XCHAL_DCACHE_WAYS > 1
	extui	a0, a0, MESR_WAYNUM_SHIFT, 2
	slli	a0, a0, DCACHE_WAYWIDTH
	slli	a2, a2, 32 - DCACHE_WAYWIDTH
	srli	a2, a2, 32 - DCACHE_WAYWIDTH
	or	a2, a2, a0
#  endif
#  if XCHAL_DCACHE_LINE_LOCKABLE
#   if XCHAL_HAVE_PREFETCH
	movi	a0, _MemErrorSave
	s32i	a4, a0, 0	// save a4
	movi	a4, 0
	xsr.prefctl a4		// disable prefetch during LDCT (issue TENX-24760)
#   endif
	ldct	a0, a2		// invalidate and unlock that cache tag
#   if XCHAL_HAVE_PREFETCH
	wsr.prefctl a4		// restore prefetch
	movi	a4, _MemErrorSave
	l32i	a4, a4, 0	// restore a4
#   endif
	bbci.l	a0, XCHAL_DCACHE_TAG_L_SHIFT, 1f	// branch if not locked
	sdct	a0, a2		// if locked, this safely writes whole tag
#  endif
1:
	diwbi	a2, 0		// writeback (if correctable) and invalidate the line
	xsr.mevaddr	a2
	j	.L_done



#  if XCHAL_DCACHE_IS_WRITEBACK || XCHAL_DCACHE_LINE_LOCKABLE
.L_dcache_noncorr:
	//  Uncorrectable error on a (writeback and/or lockable) data cache.
#   if XCHAL_DCACHE_IS_WRITEBACK
	//  On tag errors we don't know whether the line is dirty, so this is unrecoverable:
	bbsi.l	a0, MESR_MEMTYPE_SHIFT+2, .L_uncorrectable_dtag	// branch if tag error
	//  Castouts are by definition dirty, uncorrectable errors on these are unrecoverable:
	bbsi.l	a0, MESR_ACCTYPE_SHIFT, .L_uncorrectable_dirty	// branch if castout
	//  Note: could still be an error on dirty dcache data, also unrecoverable.
#   else
	bbsi.l	a0, MESR_MEMTYPE_SHIFT+2, .L_dcache_tag_noncorr	// branch if tag error
#   endif
	//  Uncorrectable error in dcache data.
	//  May be dirty or locked, so get tag to find out.
	xsr.mevaddr	a2
#   if XCHAL_DCACHE_WAYS > 1
	extui	a0, a0, MESR_WAYNUM_SHIFT, 2	// id of way with mem error
	slli	a0, a0, DCACHE_WAYWIDTH
	xor	a0, a2, a0			// xor corresponding bits of addr
	extui	a0, a0, DCACHE_WAYWIDTH, 2	// take 2 xor'ed way bits
	or	a2, a2, a0			// save them at bottom of addr
	slli	a0, a0, DCACHE_WAYWIDTH
	xor	a2, a2, a0			// and change 2 way bits of addr
#   endif
#   if XCHAL_HAVE_PREFETCH
	movi	a0, _MemErrorSave
	s32i	a4, a0, 0		// save a4
	movi	a4, 0
	xsr.prefctl a4			// disable prefetch during LDCT (issue TENX-24760)
#   endif
	ldct	a0, a2			// get dcache tag
#   if XCHAL_HAVE_PREFETCH
	wsr.prefctl a4			// restore prefetch
	movi	a4, _MemErrorSave
	l32i	a4, a4, 0		// restore a4
#   endif
#   if XCHAL_DCACHE_IS_WRITEBACK
	bbsi.l	a0, XCHAL_DCACHE_TAG_D_SHIFT, .L_uncorrectable_dirty_2	// branch if dirty
#   endif
	//  Data cache line is clean.
#   if XCHAL_DCACHE_LINE_LOCKABLE
	bbsi.l	a0, XCHAL_DCACHE_TAG_L_SHIFT, .L_dcache_nc_locked
#   endif
	//  Data cache line is clean and unlocked.  Just invalidate it.
	//  FIXME:  any stores to this line by an NMI handler will be lost.
	//	On the other hand, if we use DHWBI, any stores by an NMI handler
	//	that don't happen to fix the error result in an unrecoverable castout.
	//	
#   if XCHAL_DCACHE_WAYS > 1
	//  Recover virtual address in a2:
	extui	a0, a2, 0, 2			// get saved xor'ed bits at bottom
	slli	a0, a0, DCACHE_WAYWIDTH		// line them up
	xor	a2, a2, a0			// restore original MEVADDR
#   endif
	dhi	a2, 0		// invalidate that data cache line
	xsr.mevaddr	a2
	j	.L_done

#   if XCHAL_DCACHE_LINE_LOCKABLE
.L_dcache_nc_locked:
#    if XCHAL_DCACHE_WAYS > 1
	//  Recover virtual address in a2:
	extui	a0, a2, 0, 2			// get saved xor'ed bits at bottom
	slli	a0, a0, DCACHE_WAYWIDTH		// line them up
	xor	a2, a2, a0			// restore original MEVADDR
#    endif
	//  Unlock, invalidate, and relock it:
	dhu	a2, 0		// unlock that data cache line
	dhi	a2, 0		// invalidate that data cache line
	dpfl	a2, 0		// prefetch-and-lock the line again
	xsr.mevaddr	a2
	j	.L_done
#   endif

#   if XCHAL_DCACHE_IS_WRITEBACK
	//  Weak reference:  if unresolved, links okay but with zero value:
	.weak	_xtos_merr_hook_uncor_dtag
.L_uncorrectable_dtag:
	//  Fatal (unrecoverable) error in dcache tag (maybe dirty):  parity or uncorrectable ECC error
	movi	a0, _xtos_merr_hook_uncor_dtag
1:
	beqz	a0, 1b		// fatal non-corr dcache tag, no hook, so infinite loop
	jx	a0		// jump to user hook, if present

	//  Weak reference:  if unresolved, links okay but with zero value:
	.weak	_xtos_merr_hook_uncor_dirty
.L_uncorrectable_dirty_2:
	xsr.mevaddr	a2
.L_uncorrectable_dirty:
	//  Fatal (unrecoverable) error, parity or non-correctable ECC error on dirty cache data
	movi	a0, _xtos_merr_hook_uncor_dirty
1:
	beqz	a0, 1b		// fatal non-corr dirty cache line, no hook, so infinite loop
	jx	a0		// jump to user hook, if present
#   else
.L_dcache_tag_noncorr:
	//  Uncorrectable error on a lockable writethru data cache tag.
	//  We have to invalidate the line, but that way we lose the lock bit.
	//  Provide a hook to relock if necessary (using knowledge outside this module
	//  about what needs to be locked).  See _xtos_merr_hook_icache_relock for details.
	//  CAVEAT EMPTOR:  this hook mechanism is subject to change.
	.weak	_xtos_merr_hook_dcache_relock	// if unresolved, links with zero value
	movi	a0, _xtos_merr_hook_dcache_relock
1:
	beqz	a0, 1b		// if no hook to recover lock state on dcache tag mem error, loop forever
	callx0	a0		// invoke user hook to relock d-cache (index in MEVADDR)
	j	.L_done
#   endif

#  endif /* DCACHE IS WRITEBACK || LINE_LOCKABLE */

# endif /* have DCACHE */

	.size	_MemErrorHandler, . - _MemErrorHandler



# endif /* splitting */
#endif /* XCHAL_HAVE_MEM_ECC_PARITY */

