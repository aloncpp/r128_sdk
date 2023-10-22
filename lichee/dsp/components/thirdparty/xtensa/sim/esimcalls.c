/* Miscellaneous system calls for the Xtensa semihosting simulator.  */

/*
 * Copyright (c) 2006 by Tensilica Inc.  ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of Tensilica Inc.
 * They may not be modified, copied, reproduced, distributed, or disclosed to
 * third parties in any manner, medium, or form, in whole or in part, without
 * the prior written consent of Tensilica Inc.
 */

#include <xtensa/simcall.h>

void
xt_iss_event_fire (unsigned event_id)
{
  register int a2 __asm__ ("a2") = SYS_event_fire;
  register unsigned a3 __asm__ ("a3") = event_id;
  register int ret_val __asm__ ("a2");
  register int ret_err __asm__ ("a3");
  __asm__ volatile ("simcall"
		    : "=a" (ret_val), "=a" (ret_err)
		    : "a" (a2), "a" (a3));
}

void
xt_iss_event_wait (unsigned event_id)
{
  register int a2 __asm__ ("a2") = SYS_event_stall;
  register unsigned a3 __asm__ ("a3") = event_id;
  register int ret_val __asm__ ("a2");
  register int ret_err __asm__ ("a3");
  __asm__ volatile ("simcall"
		    : "=a" (ret_val), "=a" (ret_err)
		    : "a" (a2), "a" (a3));
}
