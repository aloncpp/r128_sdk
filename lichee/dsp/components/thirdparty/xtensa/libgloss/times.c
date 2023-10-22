/* times.c  -  get process times */

/*
 * Copyright (c) 2006-2011 Tensilica Inc.
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

#include <xtensa/config/core.h>
#include <sys/times.h>
#include <errno.h>
#include "gloss.h"


/*
 *  times
 *
 *  Use the Xtensa processor CCOUNT register if available.
 */

clock_t
_FUNC(times, struct tms *buf)
{
#if XCHAL_HAVE_CCOUNT
  clock_t clk;
  __asm__ ("rsr.ccount %0" : "=a" (clk));
  if (buf) {
    buf->tms_utime  = clk;	/* user time */
    buf->tms_stime  = 0;	/* system time (no separate user vs. system) */
    buf->tms_cutime = 0;	/* user time of children (no children) */
    buf->tms_cstime = 0;	/* system time of children (no children) */
  }
  return clk;
#else
  if (buf) {
    buf->tms_utime  = 0;
    buf->tms_stime  = 0;
    buf->tms_cutime = 0;
    buf->tms_cstime = 0;
  }
  errno = ENOSYS;
  return (clock_t) -1;
#endif
}

