/* _exit function for the Xtensa semihosting simulator.  */

/*
 * Copyright (c) 1998-2008 by Tensilica Inc.  ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of Tensilica Inc.
 * They may not be modified, copied, reproduced, distributed, or disclosed to
 * third parties in any manner, medium, or form, in whole or in part, without
 * the prior written consent of Tensilica Inc.
 */

#include <sys/reent.h>
#include <xtensa/simcall.h>
#include <xtensa/config/core-isa.h>
#include <xtensa/config/core.h>


void
_exit (int n)
{
  /* Sync dirty data to memory before terminating.  */
#if XCHAL_DCACHE_IS_COHERENT
  xthal_cache_coherence_optout ();
#elif XCHAL_DCACHE_IS_WRITEBACK
  xthal_dcache_all_writeback ();
#endif

  /*  Issue EXTW instruction (or MEMW if pre-LX) to sync memory and queues:  */
#pragma flush

  {
  register int a2 __asm__ ("a2") = SYS_exit;
  register int a3 __asm__ ("a3") = n;

    __asm__ volatile ("simcall"
		      : : "a" (a2), "a" (a3));
  }
  /*NOTREACHED*/
}

