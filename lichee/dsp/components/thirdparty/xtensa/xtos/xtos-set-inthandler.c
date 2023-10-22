
/* xtos-set-inthandler.c - register an interrupt handler in XTOS */

/*
 * Copyright (c) 1999-2018 Cadence Design Systems, Inc.
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


#include <stddef.h>
#include <stdint.h>

#include <xtensa/config/core.h>
#include "xtos-internal.h"


#if XCHAL_HAVE_INTERRUPTS
/*
 *  NOTE: If the NSA/NSAU instructions are configured, then to save a few
 *  cycles in the interrupt dispatcher code, the xtos_interrupt_table[]
 *  array is filled in reverse. Must use the MAPINT() macro defined in
 *  xtos-internal.h to index entries in this array.
 */
extern XtosIntHandlerEntry xtos_interrupt_table[XCHAL_NUM_INTERRUPTS];

/*
 *  Default handler for all unhandled interrupts.
 */
extern void xtos_unhandled_interrupt(void *arg);

#if XCHAL_RH01_ERRATUM
extern XtosIntHandlerEntry _rh0_interrupt_table[XCHAL_NUM_INTERRUPTS];
#endif
#endif


/*
 *  xtos_set_interrupt_handler
 *
 *  Parameters:
 *
 *  intnum                      The interrupt number (0 .. XCHAL_NUM_INTERRUPTS - 1).
 *
 *  handler                     Address of interrupt handler to be registered.
 *                              Passing NULL will uninstall an existing handler.
 *
 *  param                       Parameter to be passed to handler when invoked.
 *
 *  pprev                       Pointer to location where address of previous handler
 *                              (if any) will be returned. Can be NULL.
 *
 *  Returns:                    Zero on success, -1 on error.
 */
int32_t
xtos_set_interrupt_handler(uint32_t        intnum,
                           xtos_handler    handler,
                           void *          param,
                           xtos_handler *  pprev)
{
#if XCHAL_HAVE_INTERRUPTS
    XtosIntHandlerEntry * entry;
    xtos_handler old;

    if (intnum >= (uint32_t) XCHAL_NUM_INTERRUPTS) {
        // Invalid interrupt number.
        return -1;
    }

#if XCHAL_HAVE_XEA2
    if ((int32_t) Xthal_intlevel[intnum] > XTOS_LOCKLEVEL) {
        // Priority level too high to be handled in C.
        return -1;
    }

    entry = &(xtos_interrupt_table[MAPINT(intnum)]); // parasoft-suppress MISRA2012-RULE-10_4_a "Usage is OK"
    old = entry->handler;
#elif XCHAL_HAVE_XEA3
#if XCHAL_RH01_ERRATUM
    entry = &_rh0_interrupt_table[intnum];
#else
    entry = &xtos_interrupt_table[intnum];
#endif
    old = entry->handler;
#else
    return -1;
#endif

    if (handler != NULL) {
        entry->handler = handler;
        entry->u.varg  = param;
    }
    else {
        entry->handler = &xtos_unhandled_interrupt;
        entry->u.narg  = intnum;
    }

    if (pprev != NULL) {
        *pprev = (old == &xtos_unhandled_interrupt) ? NULL : old;
    }

    return 0;
#else
    // Unsupported function.
    UNUSED(intnum);
    UNUSED(handler);
    UNUSED(param);
    UNUSED(pprev);

    return -1;
#endif
}

