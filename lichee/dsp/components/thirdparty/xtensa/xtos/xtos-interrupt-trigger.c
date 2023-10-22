
// xtos-interrupt-trigger.c -- Trigger the specified interrupt number.

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

#include <xtensa/core-macros.h>
#include "xtos-internal.h"


// Force the XTOS interrupt table to be linked in if this function is called.
__asm__ (".global   xtos_interrupt_table\n");


//-----------------------------------------------------------------------------
//  xtos_interrupt_trigger
//
//  Trigger the specified interrupt.
//
//  Parameters:
//
//  intnum                  The interrupt number (0 .. XCHAL_NUM_INTERRUPTS - 1)
//                          to be triggered.
//
//  Returns:                0 on success, -1 on error.
//-----------------------------------------------------------------------------
int32_t
xtos_interrupt_trigger(uint32_t intnum)
{
#if XCHAL_HAVE_INTERRUPTS

    if (intnum >= (uint32_t) XCHAL_NUM_INTERRUPTS) {
        // Invalid interrupt number
        return -1;
    }

    xthal_interrupt_trigger(intnum);
    return 0;

#else

    // Unsupported function
    UNUSED(intnum);
    return -1;

#endif // XCHAL_HAVE_INTERRUPTS
}

