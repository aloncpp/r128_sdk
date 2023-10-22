/* gloss.h  -  definitions for Tensilica's implementation of libgloss */

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

#ifdef REENTRANT_SYSCALLS_PROVIDED
# include <sys/reent.h>
# define _FUNC(name,args...)	_ ## name ## _r(struct _reent *reent, args)
# define _FUNCVOID(name)	_ ## name ## _r(struct _reent *reent)
# undef errno
# define errno			reent->_errno
#else
# define _FUNC(name,args...)	name (args)
# define _FUNCVOID(name)	name (void)
#endif

/*  Only one task.  Report same PID (process ID) as for the init task.  */
#define _GLOSS_PID	1


/*  Functions used by libgloss.  */
extern void _exit(int status);
extern char inbyte(void);
extern int  outbyte(char c);

