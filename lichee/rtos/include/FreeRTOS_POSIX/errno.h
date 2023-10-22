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
 * @file errno.h
 * @brief System error numbers.
 *
 * http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/errno.h.html
 *
 * The values defined in this file may not be compatible with the strerror
 * function provided by this system.
 */

#ifndef _FREERTOS_POSIX_ERRNO_H_

#ifdef __cplusplus
extern "C" {
#endif

#define _FREERTOS_POSIX_ERRNO_H_

#include <FreeRTOSConfig.h>
#include_next <errno.h>

/**
 * @brief Define FreeRTOS+POSIX errno, if enabled.
 */
#if ( configUSE_POSIX_ERRNO == 1 )

/**
 * @brief System error variable, errno.
 */
    #undef errno
    extern int FreeRTOS_errno;
    #define errno FreeRTOS_errno
#endif


#ifdef __cplusplus
}
#endif

#endif /* ifndef _FREERTOS_POSIX_ERRNO_H_ */
