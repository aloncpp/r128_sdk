#ifndef __SHAPE_H
#define __SHAPE_H

#include <stdio.h>
#include <stdlib.h>

#define SHAPE_MAX_RAW   5
#define SHAPE_MAX_COL   5
#define SHAPE_8x8_RAW   8
#define SHAPE_8x8_COL   8
#define SHAPE_5x6_RAW   6
#define SHAPE_5x6_COL   5

#define SHAPE_8x8_COLOR_1   0x001000
#define SHAPE_8x8_COLOR_2   0x100000
#define SHAPE_8x8_COLOR_3   0x000010

#define SHAPE_5x6_COLOR_1   0x100000
#define SHAPE_5x6_COLOR_2   0x001000
#define SHAPE_5x6_COLOR_3   0x000010

#define SHAPE_COLOR_NULL    0x000000
#define SHAPE_COLOR_WHIITE  0xFFFFFF
#define SHAPE_COLOR_RED     0xFF0000
#define SHAPE_COLOR_YELLOW  0xECD452
#define SHAPE_COLOR_ORANGE  0xF78818
#define SHAPE_COLOR_BLUE_L  0x49BDFE
#define SHAPE_COLOR_BLUE_D  0x4237FE
#define SHAPE_COLOR_GREP    0xC7C2C2
    // {                 0,                 0,                 0,                 0,                 0,                 0,                 0,                 0},

typedef struct 
{
    uint32_t *shape; 
}icon_shape_t;

typedef struct 
{
    uint32_t *shape; 
}weather_shape_t;

int weather_shape_init(weather_shape_t *shape);

int shape_8x8_init(icon_shape_t *shape);
int shape_5x6_init(icon_shape_t *shape);

#endif