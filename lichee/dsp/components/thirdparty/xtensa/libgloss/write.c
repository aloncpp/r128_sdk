/* write.c  -  write to a file descriptor */

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

#ifdef CONFIG_COMPONENTS_OPENAMP_LOG_TRACE
void log_buff(int ch);
#endif
/*
 *  write
 *
 *  Write to stdout or stderr.
 *  NOTE:
 *	- assumes stdout or stderr (fd == 1 or 2)!!, going to the same port
 *	- converts LF to CR-LF, for terminals (assuming serial port output)
 */

int
_FUNC (write, int fd, const void *buf, int count)
{
	const unsigned char *cbuf = (const unsigned char *)buf;
	int i, c;

	for (i = 0; i < count; i++) {
		c = *cbuf++;
#ifdef CONFIG_COMPONENTS_OPENAMP_LOG_TRACE
		log_buff(c);
#endif

		if (console_uart != UART_UNVALID) {
#if defined(CONFIG_DRIVERS_UART) && !defined(CONFIG_DISABLE_ALL_UART_LOG)
			if (c == '\n')
				hal_uart_put_char(console_uart, '\r');

			hal_uart_put_char(console_uart, c);
#endif
		}
	}

	return count;
}

