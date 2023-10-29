#pragma once

#include <stdint.h>

#define NUMBER_FONT_START_X (2) // 字体偏移，左上角为原点
#define NUMBER_FONT_START_Y (1)
#define NUMBER_FONT_COL (5) // 数字字体行数
#define NUMBER_FONT_RAW (3) // 数字字体列数
#define AWTRIX_MAX_RAW (8)  // 屏幕像素点行数
#define AWTRIX_MAX_COL (32) // 屏幕像素点列数

typedef union pixel_u
{
    struct
    {
        uint8_t b;
        uint8_t g;
        uint8_t r;
    };
    uint32_t rgb;
} pixel_t ;

