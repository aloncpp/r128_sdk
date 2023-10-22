/* stat system call for the Xtensa semihosting simulator.  */

/*
 * Copyright (c) 2004 by Tensilica Inc.  ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of Tensilica Inc.
 * They may not be modified, copied, reproduced, distributed, or disclosed to
 * third parties in any manner, medium, or form, in whole or in part, without
 * the prior written consent of Tensilica Inc.
 */

#include <errno.h>
#include <sys/stat.h>
#include <sys/reent.h>


int
_stat_r (struct _reent *reent, const char *path, struct stat *st)
{
  reent->_errno = EIO;
  return -1;
}
