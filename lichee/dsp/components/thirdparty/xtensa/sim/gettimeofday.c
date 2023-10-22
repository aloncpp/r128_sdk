/* gettimeofday system call for the Xtensa semihosting simulator.  */

/*
 * Copyright (c) 2004 by Tensilica Inc.  ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of Tensilica Inc.
 * They may not be modified, copied, reproduced, distributed, or disclosed to
 * third parties in any manner, medium, or form, in whole or in part, without
 * the prior written consent of Tensilica Inc.
 */

#include <sys/time.h>
#include <sys/reent.h>
#include <xtensa/simcall.h>


int
_gettimeofday_r (struct _reent *reent, struct timeval *tv, struct timezone *tz)
{
  register int a2 __asm__ ("a2") = SYS_gettimeofday;
  register struct timeval *a3 __asm__ ("a3") = tv;
  register struct timezone *a4 __asm__ ("a4") = tz;
  register int ret_val __asm__ ("a2");
  register int ret_err __asm__ ("a3");

  __asm__ ("simcall"
	   : "=a" (ret_val), "=a" (ret_err)
	   : "a" (a2), "a" (a3), "a" (a4));

  if (ret_err)
    reent->_errno = ret_err;

  return ret_val;
}

