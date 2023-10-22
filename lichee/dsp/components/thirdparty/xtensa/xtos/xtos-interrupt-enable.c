
// xtos-interrupt-enable.c -- Enable the specified interrupt number.

// Copyright (c) 2004-2017 Cadence Design Systems, Inc.
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


// Force the XTOS interrupt table to be linked in if this function is called.
__asm__ (".global   xtos_interrupt_table\n");

#if XCHAL_HAVE_XEA2
extern uint32_t _xtos_enabled;
extern uint32_t _xtos_vpri_enabled;
#endif


//-----------------------------------------------------------------------------
//  xtos_interrupt_enable
//
//  Enable the specified interrupt. Can be called from user code or from an
//  interrupt handler running at an interrupt level <= XTOS_LOCKLEVEL.
//
//  XEA2: If INTENABLE is virtualized, does not set INTENABLE directly, but
//  instead computes it as a function of the current virtual priority.
//
//  Parameters:
//
//  intnum                  The interrupt number (0 .. XCHAL_NUM_INTERRUPTS - 1)
//                          to be enabled.
//
//  Returns:                0 on success, -1 on error.
//-----------------------------------------------------------------------------
int32_t
xtos_interrupt_enable(uint32_t intnum)
{
#if XCHAL_HAVE_INTERRUPTS && XCHAL_HAVE_XEA2
    uint32_t save_ps;
    uint32_t mask;
#endif

#if XCHAL_HAVE_INTERRUPTS

    if (intnum >= (uint32_t) XCHAL_NUM_INTERRUPTS) {
        // Invalid interrupt number
        return -1;
    }

#if XCHAL_HAVE_XEA2
    mask    = ((uint32_t)1U << intnum);	// parasoft-suppress MISRA2012-RULE-12_2 "Range checked".
    save_ps = (uint32_t) XT_RSIL(XTOS_LOCKLEVEL);

#if XTOS_VIRTUAL_INTENABLE
    _xtos_enabled |= mask;
    mask = _xtos_enabled & _xtos_vpri_enabled;
#else
    mask = mask | XT_RSR_INTENABLE();
#endif

    XT_WSR_INTENABLE(mask);
    XT_WSR_PS(save_ps);
    XT_RSYNC();
    return 0;
#elif XCHAL_HAVE_XEA3
    xthal_interrupt_enable(intnum);
    return 0;
#else
    return -1;
#endif

#else // No interrupts

    // Unsupported function
    UNUSED(intnum);
    return -1;

#endif // XCHAL_HAVE_INTERRUPTS
}

