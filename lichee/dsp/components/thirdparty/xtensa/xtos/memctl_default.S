// memctl_default.S  - Default startup value for MEMCTL register.

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


#include <xtensa/coreasm.h>
#include <xtensa/config/system.h>
#include <xtensa/core-macros.h>


// These weak symbols are used by the reset handler code to initialize
// the MEMCTL register early in the reset sequence.
// They are here rather than in reset-vector.S because the symbols
// need to be external, so resolution is delayed until link time.
//
// Normally, __memctl_default inits the caches, and __memctl_default_post
// sets up snoop, loop buffer and branch prediction behavior.
//
// To define your own value to override these defaults, redefine the
// symbol __memctl_default to the desired value, e.g. -
//
//    xt-xcc test.c -g -o test -Wl,--defsym=__memctl_default=0x08080808
//
// or, redefine __memctl_default_post, e.g. -
//
//    xt-xcc test.c -g -o test -Wl,--defsym=__memctl_default_post=0x00000001
//

	.global	__memctl_default
	.weak	__memctl_default
	.equ	__memctl_default, XCHAL_MEMCTL_DEFAULT

	.global	__memctl_default_post
	.weak	__memctl_default_post
	.equ	__memctl_default_post, XCHAL_MEMCTL_DEFAULT_POST


// This weak symbol is used by the reset handler code to initialize
// the PREFCTL register early in the reset sequence.
// It is defined here to prevent it from getting resolved during
// assembly. It needs to be unresolved until link time for overrides
// from application code to work.
//
// To define your own value to override these defaults, redefine the
// symbol __prefctl_default to the desired value, e.g. -
//
//    xt-xcc test.c -g -o test -Wl,--defsym=__prefctl_default=<value>

	.global	__prefctl_default
	.weak	__prefctl_default
	.equ	__prefctl_default, XCHAL_CACHE_PREFCTL_DEFAULT


// This weak symbol is used by the reset handler code.
// It is defined here to prevent it from getting resolved during
// assembly. It needs to be unresolved until link time for overrides
// from application code to work.
//
// To define your own value to override this default, use the macro
// XTHAL_L2_SETUP().
//

#if XCHAL_HAVE_L2
	.global	_Xthal_L2_ctrl_init
	.weak	_Xthal_L2_ctrl_init
	.equ	_Xthal_L2_ctrl_init, XTHAL_L2_DEFAULT_CTRL_INIT
#endif

