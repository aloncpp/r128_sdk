// exc-table.S - general exception C handler table

// Copyright (c) 1999-2018 Cadence Design Systems, Inc.
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
#include "xtos-internal.h"


#if XCHAL_HAVE_EXCEPTIONS

	// Table of C exception handlers. When user code registers a handler,
	// the handler address is stored here. The handler may not be written
	// in C but has to follow the C call/return convention.
	// This table is sized to handle all exceptions currently supported.

	.data

	.global	xtos_c_handler_table
	.align	4

xtos_c_handler_table:
	.rept	XEA2_EXCEPTION_COUNT
	.word	xtos_p_none
	.endr

	.size	xtos_c_handler_table, . - xtos_c_handler_table

	// Default 'C' exception handler. This is just a placeholder used to
	// populate the C handler table, and normally never gets executed.
	// NOTE: Control goes first to the debugger if one is present, see
	//     xtos_unhandled_exception in exc-unhandled.S.

	.text

	.align	4
	.global	xtos_p_none
	.type	xtos_p_none,@function

xtos_p_none:
	abi_entry
	//  Do nothing.
	abi_return

	.size	xtos_p_none, . - xtos_p_none

#endif // XCHAL_HAVE_EXCEPTIONS

