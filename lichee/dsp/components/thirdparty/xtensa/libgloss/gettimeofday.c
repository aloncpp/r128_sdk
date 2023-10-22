/* gettimeofday.c  -  stub for current time of day */

/*
 * Copyright (c) 2004-2013 Tensilica Inc.
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

#include <sys/time.h>
#include <errno.h>
#include "gloss.h"

#include <xtensa/config/core.h>
#include <xtensa/core-macros.h>


/*
 *  gettimeofday
 *
 *  Just a stub, to allow applications to build that call this function.
 *
 *  Use the Xtensa processor CCOUNT register if available, to report
 *  time elapsed as if since 1970-01-01 00:00, assuming a core clock
 *  frequency of 2^24 Hz (about 16.78 MHz).  Keeps track of CCOUNT
 *  register wraparounds as long as this function is called at least
 *  once every 2^32 - 1 clock cycles, otherwise reported time misses
 *  some integral multiple of 2^32 elapsed clock cycles.
 *  NOTE:  accurate time elapsed also depends on actual core clock
 *  frequency, which this function does not track.
 *
 *  (Any mechanism to reliably count CCOUNT register overflows
 *   generally involves interrupts and/or special hardware, which
 *   introduces an OS/RTOS or board/system dependency.  Correct
 *   processor clock frequency is also board/system dependent.
 *   Maintaining correct date/time across power cycling etc
 *   also involves board/system dependencies.
 *   As a stub that avoids such dependencies, these features
 *   are not available.)
 *
 *  Note:  we avoid using division or modulo, which can be much more code.
 *
 *  Note:  should be re-entrant, except when invoked in an NMI handler.
 */

int
_FUNC (gettimeofday, struct timeval *tv, struct timezone *tz)
{
#if XCHAL_HAVE_CCOUNT
# define CLOCK_RATE_LOG2	24	/* assumed clock frequency, log2 */
  static struct {
    unsigned last_ccount;		/* to detect CCOUNT wrap-arounds */
    unsigned monotonic;			/* enforce monotonically increasing tv_sec */
  } g = {0,0};

  if (tv) {
    /* Not protected against interrupts any more. Even if this function gets
       called from an interrupt handler, assuming that the handler run time
       is much smaller than the CCOUNT rollover time, g.monotonic should get
       updated correctly.
     */
    unsigned ccount = XTHAL_GET_CCOUNT();

    if (ccount < g.last_ccount) {
        g.monotonic += (1 << (32 - CLOCK_RATE_LOG2));
    }
    g.last_ccount = ccount;

    tv->tv_sec = g.monotonic + (ccount >> CLOCK_RATE_LOG2);	/* "seconds" portion */
    ccount &= ((1 << CLOCK_RATE_LOG2) - 1);	/* fraction of a "second" portion */
# if CLOCK_RATE_LOG2 > 20
    ccount >>= (CLOCK_RATE_LOG2 - 20);
# elif CLOCK_RATE_LOG2 < 20
    ccount <<= (20 - CLOCK_RATE_LOG2);
# endif
    ccount -= (ccount >> 5);		/* adjustments to make ccount < 1000000 */
    ccount -= (ccount >> 6);		/* ... */
    tv->tv_usec = ccount;		/* microseconds portion (0 .. 999936) */
  }

  return 0;
#else
  if (tv) {
    tv->tv_sec = 0;
    tv->tv_usec = 0;
  }
  errno = ENOSYS;
  return -1;
#endif
}

