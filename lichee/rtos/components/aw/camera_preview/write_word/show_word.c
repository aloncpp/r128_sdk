/*
 * Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *	1. Redistributions of source code must retain the above copyright
 *	   notice, this list of conditions and the following disclaimer.
 *	2. Redistributions in binary form must reproduce the above copyright
 *	   notice, this list of conditions and the following disclaimer in the
 *	   documentation and/or other materials provided with the
 *	   distribution.
 *	3. Neither the name of XRADIO TECHNOLOGY CO., LTD. nor the names of
 *	   its contributors may be used to endorse or promote products derived
 *	   from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "show_word.h"

/*color : 0x00RRGGBB*/
static void lcd_put_pixel(struct ft *f, int x, int y, unsigned int color)
{
    unsigned char *pen_8 = f->fbmem + y * f->line_width + x * f->pixel_width;
    unsigned short *pen_16;
    unsigned int *pen_32;
    unsigned int red, green, blue;

    pen_16 = (unsigned short *)pen_8;
    pen_32 = (unsigned int *)pen_8;

    switch (f->bits_per_pixel)
    {
        case 8:
        {
            *pen_8 = color;
            break;
        }
        case 16:
        {
            /* 565 */
            red   = (color >> 16) & 0xff;
            green = (color >> 8) & 0xff;
            blue  = (color >> 0) & 0xff;
            color = ((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3);
            *pen_16 = color;
            break;
        }
        case 32:
        {
            *pen_32 = color;
            break;
        }
        default:
        {
            printf("can't surport %dbpp\n", f->bits_per_pixel);
            break;
        }
    }
}


static void lcd_put_ascii(struct ft *f, int x, int y, unsigned char c)
{
    unsigned char *dots = (unsigned char *)&fontdata_8x16[c * 16];
    int i, b;
    unsigned char byte;

    /*8 * 16, display 16 line*/
    for (i = 0; i < 16; i++)
    {
        byte = dots[i];
        for (b = 7; b >= 0; b--)
        {
            if (byte & (1 << b))
            {
                /* show */
                lcd_put_pixel(f, x + (7 - b) * 2, y + i * 2, 0xffffffff);
                lcd_put_pixel(f, x + (7 - b) * 2 + 1, y + i * 2, 0xffffffff);
            }
            else
            {
                /* hide */
                lcd_put_pixel(f, x + (7 - b) * 2 + 1, y + i * 2, 0);
            }
        }
    }
}

void print_ascii(struct ft *f, unsigned char *ascii_str, int x, int y)
{
    int i;
    int distance = 18;  /*相邻字符的间距*/
    int num_per_line = 18;  /*每一行的字符数量*/
    int size = strlen((const char *)ascii_str);
    for (i = 0; i < size; i++)
    {
        lcd_put_ascii(f, x + i % num_per_line * distance,
                      y + i / num_per_line * distance, ascii_str[i]);
    }
}
