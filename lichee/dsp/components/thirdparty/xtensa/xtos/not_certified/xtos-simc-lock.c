
// xtos-simc-lock.c - Implement the shared locks needed to support using
// the C library across multiple cores. Currently works only with XCLIB.

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
#include <sys/reent.h>

#include <xtensa/hal.h>
#include <xtensa/config/core.h>
#include <xtensa/config/system.h>
#include <xtensa/xtruntime.h>


#if XCHAL_HAVE_PRID && XCHAL_HAVE_EXCLUSIVE && (XSHAL_CLIB == XTHAL_CLIB_XCLIB)

// The 8 extra locks are for C++ support. If you are not going
// to use C++ at all, these can be removed to save memory.
#define NUM_LOCKS	(_MAX_LOCK + FOPEN_MAX + 8)

// Override (weak) default and set to 1 to enable locking.
int32_t _xclib_use_mt = 1;

// Typedef the handle required by xclib.
typedef xtos_mutex_p _Rmtx;

// Lock variables. These must be placed in a memory region that supports
// L32EX/S32EX. The memory region must also be coherent cached or uncached.
xtos_mutex xtos_locks[NUM_LOCKS];

// Internal count of locks initialized.
static int32_t lcnt;

// Declarations to avoid compiler warnings.
void _Mtxinit(_Rmtx * mtx);
void _Mtxdst(_Rmtx * mtx);
void _Mtxlock(_Rmtx * mtx);
void _Mtxunlock(_Rmtx * mtx);


//-----------------------------------------------------------------------------
// _Mtxinit
//
// Initialize a mutex that will be used by the Xtensa C Library (XCLIB).
// This function is called by XCLIB during library init. In a multicore system
// this should be called only on core 0. It is called once for each mutex that
// the library needs.
//
// Parameters:
//
// mtx                      Pointer to storage for lock handle.
//
// Returns:                 Nothing.
//-----------------------------------------------------------------------------
void
_Mtxinit(_Rmtx * mtx)
{
    if (lcnt < NUM_LOCKS) {
        xtos_mutex_init(&(xtos_locks[lcnt]));
        *mtx = &(xtos_locks[lcnt]);
        lcnt++;
    }
}

//-----------------------------------------------------------------------------
// _Mtxdst
//
// Destroy a mutex used by the Xtensa C Library (XCLIB). This function is called
// by XCLIB during library shutdown. Not necessary to do anything here, but we
// will clear the locks for completeness.
//
// Parameters:
//
// mtx                      Pointer to storage for lock handle.
//
// Returns:                 Nothing.
//-----------------------------------------------------------------------------
void
_Mtxdst(_Rmtx * mtx)
{
    xtos_mutex_init(*mtx);
}

//-----------------------------------------------------------------------------
// _Mtxlock
//
// Lock the specified mutex. Locking is recursive (i.e. XCLIB may lock the same
// mutex multiple times).
//
// Parameters:
//
// mtx                      Pointer to storage for lock handle.
//
// Returns:                 Nothing.
//-----------------------------------------------------------------------------
void
_Mtxlock(_Rmtx * mtx)
{
    xtos_mutex_lock(*mtx);
}

//-----------------------------------------------------------------------------
// _Mtxunlock
//
// Unlock the specified mutex. Locking is recursive so multiple unlocks may be
// required before the lock is actually released.
//
// Parameters:
//
// mtx                      Pointer to storage for lock handle.
//
// Returns:                 Nothing.
//-----------------------------------------------------------------------------
void
_Mtxunlock(_Rmtx * mtx)
{
    xtos_mutex_unlock(*mtx);
}

#endif	// XCHAL_HAVE_PRID && XCHAL_HAVE_EXCLUSIVE && (XSHAL_CLIB == XTHAL_CLIB_XCLIB)

