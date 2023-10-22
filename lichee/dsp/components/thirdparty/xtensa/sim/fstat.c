/* fstat system call for the Xtensa semihosting simulator.  */

/*
 * Copyright (c) 2004 by Tensilica Inc.  ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of Tensilica Inc.
 * They may not be modified, copied, reproduced, distributed, or disclosed to
 * third parties in any manner, medium, or form, in whole or in part, without
 * the prior written consent of Tensilica Inc.
 */

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/reent.h>


int
_fstat_r (struct _reent *reent, int file, struct stat *st)
{
  /* Cause newlib's posix isatty() to return 1 and newlib's makebuf to also
     assume a tty.  */
  st->st_mode = S_IFCHR;	/* always pretend to be a tty */
  st->st_blksize = 0;
  return 0;
#if 0
  reent->_errno = EIO;
  return -1;
#endif
}

