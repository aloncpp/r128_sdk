/* read.c  -  read from a file descriptor */

/*
 * Copyright (c) 2011 Tensilica Inc.
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

#include "gloss.h"

#include <hal_uart.h>
#include <console.h>

/*
 *  read
 *
 *  Read count bytes from file descriptor fd, into buf.
 *
 *  NOTE:
 *	- assume stdin (fd==0)!
 *	- stops at end of line (CR or LF), sort-of as for cooked character input
 *	  (but no line editing!)
 */

int
_FUNC (read, int fd, void *buf, int count)
{
	unsigned char c, *cbuf = (unsigned char*)buf;

	if (console_uart == UART_UNVALID) {
		return -1;
	}

	for (; count > 0; count--) {
		*cbuf++ = c = hal_uart_get_char(console_uart);
		if (c == '\r' || c == '\n')
			break;
	}
	return cbuf - (unsigned char*)buf;
}

