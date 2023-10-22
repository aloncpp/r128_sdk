
// xtos-simc.S  - Define or pull in symbols and objects needed for
// single image multicore support.

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


#include <xtensa/config/core.h>
#include <xtensa/config/system.h>
#include <xtensa/coreasm.h>
#include "xtos-internal.h"


#if XCHAL_HAVE_PRID && XCHAL_HAVE_EXCLUSIVE && (XSHAL_CLIB == XTHAL_CLIB_XCLIB)

	.global	__xtos_simc		// strong version of symbol
	.equ	__xtos_simc, 1		// enable SIMC mode

	.global	__os_flag		// strong version of symbol
	.equ	__os_flag, 1		// override xclib default

	.global	__stack_size
	.weak	__stack_size		// define weak for user override
	.equ	__stack_size, 4096	// default stack size is 4 KB

	// Pull these in by reference

	.global	__stack_init
	.global	__bss_init
	.global	xtos_locks

#endif // XCHAL_HAVE_PRID && XCHAL_HAVE_EXCLUSIVE && (XSHAL_CLIB == XTHAL_CLIB_XCLIB)

