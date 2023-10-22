/*
 * Amazon FreeRTOS+POSIX V1.0.4
 * Copyright (C) 2018 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://aws.amazon.com/freertos
 * http://www.FreeRTOS.org
 */

/**
 * @file unistd.h
 * @brief Standard symbolic constants and types
 *
 * http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/unistd.h.html
 */

#ifndef _FREERTOS_POSIX_UNISTD_H_
#define _FREERTOS_POSIX_UNISTD_H_

#include "FreeRTOS_POSIX/types.h"

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief Suspend execution for an interval of time.
 *
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/sleep.html
 */
unsigned sleep( unsigned seconds );

/**
 * @brief Suspend execution for microsecond intervals.
 *
 * This is a useful, non-POSIX function.
 * @param[in] usec The number of microseconds to suspend execution.
 *
 * @return 0 always. This function does not specify any failure conditions.
 */
int usleep( useconds_t usec );

extern char *optarg;			/* getopt(3) external variables */
extern int optind, opterr, optopt;
int	 getopt(int, char * const [], const char *);
extern int optreset;			/* getopt(3) external variable */

int close(int);
off_t lseek(int, off_t, int);
int fsync(int);
ssize_t read(int, void *, size_t);
ssize_t write(int, const void *, size_t);
int link(const char *, const char *);
int unlink(const char *);
pid_t   getpid (void);
int rmdir(const char *name);
unsigned int sleep (unsigned int seconds);

/* Values for the second argument to access.
   These may be OR'd together.  */
#define	R_OK	4		/* Test for read permission.  */
#define	W_OK	2		/* Test for write permission.  */
#define	X_OK	1		/* Test for execute permission.  */
#define	F_OK	0		/* Test for existence.  */

/* Test for access to NAME using the real UID and real GID.  */
extern int access (const char *__name, int __type) __THROW __nonnull ((1));

#ifdef __cplusplus
}
#endif

#endif /* ifndef _FREERTOS_POSIX_UNISTD_H_ */
