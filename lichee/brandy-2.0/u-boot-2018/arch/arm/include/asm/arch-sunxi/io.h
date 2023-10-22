/*
 * Allwinnertech rtos io operation header file.
 * Copyright (C) 2019  Allwinnertech Co., Ltd. or its affiliates.  All Rights Reserved.
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
 */

#ifndef _RTOS_IO_H_
#define _RTOS_IO_H_

#include "aw_types.h"

#define get_bvalue(addr)	(*((volatile unsigned char  *)(addr)))
#define put_bvalue(addr, v)	(*((volatile unsigned char  *)(addr)) = (unsigned char)(v))
#define get_hvalue(addr)	(*((volatile unsigned short *)(addr)))
#define put_hvalue(addr, v)	(*((volatile unsigned short *)(addr)) = (unsigned short)(v))
#define get_wvalue(addr)	(*((volatile unsigned int   *)(addr)))
#define put_wvalue(addr, v)	(*((volatile unsigned int   *)(addr)) = (unsigned int)(v))

#define set_bit(addr, v)    (*((volatile unsigned char  *)(addr)) |=  (unsigned char)(v))
#define clr_bit(addr, v)    (*((volatile unsigned char  *)(addr)) &= ~(unsigned char)(v))
#define set_bbit(addr, v)   (*((volatile unsigned char  *)(addr)) |=  (unsigned char)(v))
#define clr_bbit(addr, v)   (*((volatile unsigned char  *)(addr)) &= ~(unsigned char)(v))
#define set_hbit(addr, v)   (*((volatile unsigned short *)(addr)) |=  (unsigned short)(v))
#define clr_hbit(addr, v)   (*((volatile unsigned short *)(addr)) &= ~(unsigned short)(v))
#define set_wbit(addr, v)   (*((volatile unsigned int   *)(addr)) |=  (unsigned int)(v))
#define clr_wbit(addr, v)   (*((volatile unsigned int   *)(addr)) &= ~(unsigned int)(v))

#define readb(addr)	    (*((volatile unsigned char  *)(addr)))
#define readw(addr)	    (*((volatile unsigned short *)(addr)))
#define readl(addr)	    (*((volatile unsigned int  *)(addr)))
#define writeb(v, addr)	(*((volatile unsigned char  *)(addr)) = (unsigned char)(v))
#define writew(v, addr)	(*((volatile unsigned short *)(addr)) = (unsigned short)(v))
#define writel(v, addr)	(*((volatile unsigned int   *)(addr)) = (unsigned int)(v))

#define cmp_wvalue(addr, v) (v == (*((volatile unsigned int *) (addr))))

/**
 * sr32 - clear & set a value in a bit range for a 32 bit address
 */
static __inline void sr32(u32 addr, u32 start_bit, u32 num_bits, u32 value)
{
    u32 tmp, msk = (1 << num_bits) - 1;
    tmp = readl(addr) & ~(msk << start_bit);
    tmp |= value << start_bit;
    writel(tmp, addr);
}

#endif /* _RTOS_IO_H_ */
