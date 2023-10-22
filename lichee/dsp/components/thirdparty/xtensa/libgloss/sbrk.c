/* sbrk system call for the Xtensa semihosting simulator.    */
/* NOTE: This file is also shared by libgloss (../libgloss). */

/*
 * Copyright (c) 2004 by Tensilica Inc.  ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of Tensilica Inc.
 * They may not be modified, copied, reproduced, distributed, or disclosed to
 * third parties in any manner, medium, or form, in whole or in part, without
 * the prior written consent of Tensilica Inc.
 */

#include <errno.h>
#include <sys/reent.h>

#include <xtensa/config/core.h>
#if XCHAL_HAVE_ISL
#include <xtensa/tie/xt_exception_dispatch.h>
#endif

/* Define a weak symbol to indicate the presence of an OS. Normally this
   symbol will not be defined, so the value of "os_flag" will be zero.
   An OS can define this to a nonzero value to indicate its presence.
 */
#pragma weak __os_flag
extern char  __os_flag;

static const void * os_flag = &__os_flag;

/*
 *  Boundaries of available memory ("heap"), as defined by the linker.
 *  NOTE:  the stack may be growing downward into the same area
 *  (depending on the linker script).
 */
extern char _end;               /* start of available memory (end of BSS) */
extern char _heap_sentry;       /* end of available memory */
extern char __stack;            /* top of stack (for XTOS) */

/*
 *  Rather than refer directly to _heap_sentry (a symbol, constant for
 *  a given program), we refer to a variable initialized to its address.
 *  This allows override / adjustment of the heap endpoint at runtime.
 *  For XCLIB this variable needs to be placed in the library's data
 *  section.
 */
#if XSHAL_CLIB == XTHAL_CLIB_XCLIB
char * _heap_sentry_ptr __attribute__ ((section(".clib.data"))) = &_heap_sentry;
#else
char * _heap_sentry_ptr = &_heap_sentry;
#endif


void *
_sbrk_r (struct _reent *reent, int incr)
{
  static char *heap_end;
  void *prev_heap_end;
  char *stack_ptr;
#if XCHAL_HAVE_ISL
  char *isl;
#endif

  if (heap_end == 0)
    heap_end = &_end;
  prev_heap_end = heap_end;

  /* The heap is limited by _heap_sentry_ptr -- we're out of memory if we
     exceed that.  */
  if (heap_end + incr >= _heap_sentry_ptr)
    {
      reent->_errno = ENOMEM;
      return (void *) -1;
    }

  /* We have a heap and stack collision if the stack started above the
     heap and is now below the end of the heap.  */
  /* Do stack checking only if os_flag is zero. Nonzero value indicates
     stack is application-managed (e.g. by an RTOS).  */
  if (os_flag == 0)
    {
      __asm__ ("mov %0, sp" : "=a" (stack_ptr));
      if (&__stack > &_end && heap_end + incr > stack_ptr)
        {
          reent->_errno = ENOMEM;
          return (void *) -1;
        }

#if XCHAL_HAVE_ISL
      /* If the interrupt stack limit is set and the address is within
         the range of the heap (_end <-> _heap_sentry) then move it up
         if required as long as there is room to move it. */
      isl = (char *)(XT_RSR_ISL() & ~0xF);
      if (isl && (isl >= &_end) && (isl <= _heap_sentry_ptr))
        {
          char *new_isl = (char *)(((unsigned int)(heap_end + incr) + 256 + 15) & ~0xF);

          if (new_isl > isl)
            {
              /* Need to move, is there room ? */
              if ((new_isl >= stack_ptr) || (new_isl >= _heap_sentry_ptr))
                {
                  reent->_errno = ENOMEM;
                  return (void *) -1;
                }
              XT_WSR_ISL((unsigned int)new_isl + 1); /* Set wrap detection */
            }
        }
#endif
    }

  heap_end += incr;
  return prev_heap_end;
}

