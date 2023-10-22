/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */
#ifndef __ARCH_CC_H__
#define __ARCH_CC_H__

#include <stddef.h>
#include <stdint.h>
//#include "sys/defs.h"
#undef LITTLE_ENDIAN
#undef BIG_ENDIAN
/* basic headers */
#define LWIP_NO_STDDEF_H   0
#define LWIP_NO_STDINT_H   0
#define LWIP_NO_INTTYPES_H 1
#define LWIP_NO_LIMITS_H   1

#if 0
/* Types based on stdint.h */
typedef uint8_t         u8_t;
typedef int8_t          s8_t;
typedef uint16_t        u16_t;
typedef int16_t         s16_t;
typedef uint32_t        u32_t;
typedef int32_t         s32_t;
typedef uintptr_t       mem_ptr_t;
#endif

typedef unsigned int sys_prot_t;

/* Define (sn)printf formatters for these lwIP types */
#define X8_F  "02x"
#define U16_F "hu"
#define S16_F "hd"
#define X16_F "hx"
#define U32_F "u"
#define S32_F "d"
#define X32_F "x"
#define SZT_F "u"

#include <errno.h>

/* Use private struct timeval */
#define LWIP_TIMEVAL_PRIVATE    0
#if (!LWIP_TIMEVAL_PRIVATE)
#include <sys/time.h>
#endif

#if (defined(__GNUC__))
    /* GNU Compiler */
    #define PACK_STRUCT_BEGIN
    #define PACK_STRUCT_STRUCT __attribute__((__packed__))
    #define PACK_STRUCT_END
    #define PACK_STRUCT_FIELD(fld) fld
    #define ALIGNED(n) __attribute__((aligned (n)))
#elif defined(__CC_ARM)
    /* ARM Compiler */
    #define PACK_STRUCT_BEGIN __packed
    #define PACK_STRUCT_STRUCT
    #define PACK_STRUCT_END
    #define PACK_STRUCT_FIELD(fld) fld
    #define ALIGNED(n) __align(n)
#else
    #error "Compiler not supported."
#endif

#if defined(__GNUC__) && defined(__thumb2__) && (!LWIP_FREERTOS_ORIG)
    /* Provide Thumb-2 routines for GCC to improve performance */
    #define LWIP_CHKSUM             thumb2_checksum
    /* Set algorithm to 0 so that unused lwip_standard_chksum function
       doesn't generate compiler warning */
    #define LWIP_CHKSUM_ALGORITHM   0

    uint16_t thumb2_checksum(const void* pData, int length);
#else
    /* Used with IP headers only */
    #define LWIP_CHKSUM_ALGORITHM   1
#endif

/* Debug on/off */
#define LWIP_DEBUG 1
#define LWIP_NOASSERT 1

#include <stdio.h>
/* Plaform specific diagnostic output */
#define LWIP_PLATFORM_DIAG(x)   do {printf x;} while(0)
#define LWIP_PLATFORM_ASSERT(x)                                 \
    do {                                                        \
        printf("%s at line %d in %s\n", x, __LINE__, __FILE__); \
    } while(0)

#include<stdlib.h>
#define LWIP_RAND() ((u32_t)rand())

#endif /* __ARCH_CC_H__ */
