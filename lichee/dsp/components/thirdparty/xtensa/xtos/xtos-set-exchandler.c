
/* xtos-set-exchandler.c - register an exception handler in XTOS */

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


#if XCHAL_HAVE_EXCEPTIONS

#if XCHAL_HAVE_XEA2

/*  Assembly-level wrapper for C handlers. */
extern void xtos_c_wrapper_handler(void *arg);
/*  Assembly-level default handler for unhandled exceptions. */
extern void xtos_unhandled_exception(void *arg);
/*  Default/empty handler. */
extern void xtos_p_none(void *arg);

extern xtos_handler xtos_c_handler_table[XCHAL_EXCCAUSE_NUM];
extern xtos_handler xtos_exc_handler_table[XCHAL_EXCCAUSE_NUM];

#endif

#if XCHAL_HAVE_XEA3

extern void xtos_default_exc_handler(void * arg);
extern xtos_handler xtos_exc_handler_table[XCHAL_EXCCAUSE_NUM];

#endif

#endif /* XCHAL_HAVE_EXCEPTIONS */


/*
 *  xtos_set_exception_handler
 *
 *  Parameters:
 *
 *  excnum                      The exception number (0 .. XCHAL_EXCCAUSE_NUM - 1).
 *
 *  handler                     Address of exception handler to be registered.
 *                              Passing NULL will uninstall an existing handler.
 *
 *  pprev                       Pointer to location where address of previous handler
 *                              (if any) will be returned. Can be NULL.
 *
 *  Returns:                    Zero on success, -1 on error.
 */
int32_t
xtos_set_exception_handler(uint32_t        excnum,
                           xtos_handler    handler,
                           xtos_handler *  pprev)
{
#if XCHAL_HAVE_EXCEPTIONS

    xtos_handler ret;
    xtos_handler h = handler;

    if (excnum >= (uint32_t) XCHAL_EXCCAUSE_NUM) {
        return -1;
    }

#if XCHAL_HAVE_XEA2
    if (h == NULL) {
        h = &xtos_p_none;
    }

    ret = xtos_c_handler_table[excnum];
    xtos_exc_handler_table[excnum] = ( (h == &xtos_p_none)
                                       ? &xtos_unhandled_exception
                                       : &xtos_c_wrapper_handler );
    xtos_c_handler_table[excnum] = h;
    if (ret == &xtos_p_none) {
        ret = NULL;
    }
#elif XCHAL_HAVE_XEA3
    if (h == NULL) {
        h = &xtos_default_exc_handler;
    }

    ret = xtos_exc_handler_table[excnum];
    xtos_exc_handler_table[excnum] = h;

    if (ret == &xtos_default_exc_handler) {
        ret = NULL;
    }
#else
    return -1;
#endif

    if (pprev != NULL) {
        *pprev = ret;
    }

    return 0;

#else

    // Unsupported function
    UNUSED(excnum);
    UNUSED(handler);
    UNUSED(pprev);

    return -1;

#endif /* XCHAL_HAVE_EXCEPTIONS */
}

