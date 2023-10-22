// xea3/double-handler-min.S  --  Minimalist Double Exception Vector for XEA3

// Copyright (c) 2016-2017 Tensilica Inc.
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


//-----------------------------------------------------------------------------
// XEA3 Double Exception handler.
//
// This is a very minimalist implementation of the double exception handler.
// It is only expected to be executed upon fatal errors (exceptions in
// non-interruptible dispatch code).
//
// The double exception vector, in the vector segment, jumps directly here.
//-----------------------------------------------------------------------------

#if XCHAL_HAVE_XEA3

	// The default handler does nothing but halt. User-defined handlers
	// can override this.

	.section	.DispatchHandler.text, "ax"
	.align		4
	.global		_DoubleExceptionHandler
_DoubleExceptionHandler:

	halt

	.size	_DoubleExceptionHandler, . - _DoubleExceptionHandler

#endif	// XCHAL_HAVE_XEA3

