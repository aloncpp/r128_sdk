/* rename system call for the Xtensa semihosting simulator.  */

/*
 * Copyright (c) 2004 by Tensilica Inc.  ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of Tensilica Inc.
 * They may not be modified, copied, reproduced, distributed, or disclosed to
 * third parties in any manner, medium, or form, in whole or in part, without
 * the prior written consent of Tensilica Inc.
 */

#include <sys/reent.h>
#include <xtensa/simcall.h>


/* Win32 systems do not support "link", so we have to provide a separate
   rename function.  */

int
_rename_r (struct _reent *reent, const char *oldpath, const char *newpath)
{
  register int a2 __asm__ ("a2") = SYS_rename;
  register const char *a3 __asm__ ("a3") = oldpath;
  register const char *a4 __asm__ ("a4") = newpath;
  register int ret_val __asm__ ("a2");
  register int ret_err __asm__ ("a3");

  __asm__ ("simcall"
	   : "=a" (ret_val), "=a" (ret_err)
	   : "a" (a2), "a" (a3), "a" (a4));

  if (ret_err)
    reent->_errno = ret_err;

  return ret_val;
}
