
// xtos-interrupt-enabled.c -- Return enable state of specified interrupt.

// Copyright (c) 2004-2018 Cadence Design Systems, Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


#include <stdint.h>

#include "xtos-internal.h"

#include <xtensa/tie/xt_core.h>
#if XCHAL_HAVE_INTERRUPTS
#include <xtensa/tie/xt_interrupt.h>
#endif


#if XCHAL_HAVE_XEA2
extern uint32_t _xtos_enabled;
extern uint32_t _xtos_vpri_enabled;
#endif


//-----------------------------------------------------------------------------
//  xtos_interrupt_enabled
//
//  Return the enable state of the specified interrupt. Can be called from
//  user code or from an interrupt handler running at an interrupt level
//  <= XTOS_LOCKLEVEL.
//
//  XEA2: If INTENABLE is virtualized, does not read INTENABLE directly.
//
//  Parameters:
//
//  intnum                  The interrupt number (0 .. XCHAL_NUM_INTERRUPTS - 1)
//                          to be checked.
//
//  Returns:                0 if disabled, 1 if enabled, -1 on error.
//-----------------------------------------------------------------------------
int32_t
xtos_interrupt_enabled(uint32_t intnum)
{
#if XCHAL_HAVE_INTERRUPTS

#if XCHAL_HAVE_XEA2
    uint32_t mask;
    int32_t  ret;
#endif

    if (intnum >= (uint32_t) XCHAL_NUM_INTERRUPTS) {
        // Invalid interrupt number
        return -1;
    }

#if XCHAL_HAVE_XEA2
    mask = ((uint32_t)1U << intnum);	// parasoft-suppress MISRA2012-RULE-12_2 "Range checked".

#if XTOS_VIRTUAL_INTENABLE
    ret = ((_xtos_enabled & mask) != 0U) ? 1 : 0;
#else
    ret = ((XT_RSR_INTENABLE() & mask) != 0U) ? 1 : 0;
#endif

    return ret;
#elif XCHAL_HAVE_XEA3
    return xthal_interrupt_enabled(intnum);
#else
    return -1;
#endif

#else // No interrupts

    // Unsupported function
    UNUSED(intnum);
    return -1;

#endif // XCHAL_HAVE_INTERRUPTS
}

