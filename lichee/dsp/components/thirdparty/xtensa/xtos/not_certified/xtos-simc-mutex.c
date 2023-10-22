
// xtos-simc-mutex.c - XTOS functions for mutexes.

// Copyright (c) 1998-2018 Cadence Design Systems, Inc.
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

#include <xtensa/hal.h>
#include <xtensa/config/core.h>
#include <xtensa/config/system.h>
#include <xtensa/xtruntime.h>
#include <xtensa/tie/xt_core.h>


#if XCHAL_HAVE_PRID && XCHAL_HAVE_EXCLUSIVE && (XSHAL_CLIB == XTHAL_CLIB_XCLIB)

void
xtos_mutex_init(xtos_mutex_p pmtx)
{
    if (pmtx != NULL) {
        pmtx->owner = 0U;
        pmtx->count = 0U;
    }
}


int32_t
xtos_mutex_lock(xtos_mutex_p pmtx)
{
    uint32_t id = XT_RSR_PRID() + 1U;

    if (pmtx != NULL) {
        if (pmtx->owner == id) {
            pmtx->count++;
        }
        else {
            int32_t ret;

            do {
                ret = xthal_compare_and_set((int32_t *) &(pmtx->owner), 0, id);
            } while (ret != 0);
            pmtx->count = 1U;
        }
        return 0;
    }

    return -1;
}


int32_t
xtos_mutex_unlock(xtos_mutex_p pmtx)
{   
    uint32_t id = XT_RSR_PRID() + 1U;

    if ((pmtx != NULL) && (pmtx->owner == id)) {
        pmtx->count--;
        if (pmtx->count == 0U) {
            pmtx->owner = 0U;
        }
        return 0;
    }

    return -1;
}

#endif // XCHAL_HAVE_PRID && XCHAL_HAVE_EXCLUSIVE && (XSHAL_CLIB == XTHAL_CLIB_XCLIB)

