// exc_table.c -- Exception Handler Table for XEA3

// Copyright (c) 2003-2015 Cadence Design Systems, Inc.
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


#include <xtensa/config/system.h>
#include <xtensa/xtruntime.h>


/* Table of exception handlers, listed by exception cause. Each C handler
   must examine the type/subtype fields of EXCCAUSE to determine the exact
   exception and handle it accordingly.
*/
extern void xtos_default_exc_handler(void * ef);

xtos_handler xtos_exc_handler_table[] = {
    xtos_default_exc_handler,    // No exception
    xtos_default_exc_handler,    // Instruction usage error
    xtos_default_exc_handler,    // Address usage error
    xtos_default_exc_handler,    // External error
    xtos_default_exc_handler,    // Debug error
    xtos_default_exc_handler,    // Syscall exception
    xtos_default_exc_handler,    // Hardware error
    xtos_default_exc_handler,    // Memory mgmt error
    xtos_default_exc_handler,    // TLB miss error
    xtos_default_exc_handler,    // Coprocessor exception
    xtos_default_exc_handler,    // Custom exception
};

