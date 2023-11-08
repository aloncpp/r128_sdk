#ifndef __FONTS_H
#define __FONTS_H

#include <stdio.h>
#include <stdlib.h>

#define FONTS_ASCII_5_3_NUMBER  65

typedef struct
{
    uint8_t *font;
}ascii_5_3_font_t;


uint8_t fonts_ascii_5_3_init(ascii_5_3_font_t *font);


#endif