#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <sunxi_hal_ledc.h>
#include <hal_cmd.h>
#include <hal_timer.h>
#include <hal_gpio.h>

#include "awtrix_api.h"
#include "fonts.h"

#define MERAGECOLOR(R, G, B) (((uint32_t)G << 16) | ((uint16_t)R << 8) | B)

static int awtrix_pixel_cursor_x = 0;
static int awtrix_pixel_cursor_y = 0;
ascii_5_3_font_t ascii_font[FONTS_ASCII_5_3_NUMBER];
pixel_t *awtrix_pixel;

int awtrix_pixel_add_char(pixel_t *local_pixel, uint8_t ch, uint8_t cover, uint8_t red, uint8_t green, uint8_t blue)
{

    pixel_t *pixel[AWTRIX_MAX_RAW];

    for (int i = 0; i < AWTRIX_MAX_RAW; i++)
        pixel[i] = &local_pixel[i * AWTRIX_MAX_COL];

    if ((ch < ' ') || (ch > '`'))
        return -1;

    for (int i = 0; i < 5; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            if (ascii_font[ch - ' '].font[i * 3 + j] == 1)
            {
                pixel[i + awtrix_pixel_cursor_y][j + awtrix_pixel_cursor_x].r = red;
                pixel[i + awtrix_pixel_cursor_y][j + awtrix_pixel_cursor_x].g = green;
                pixel[i + awtrix_pixel_cursor_y][j + awtrix_pixel_cursor_x].b = blue;
            }
            else
            {
                if (cover == 1)
                {
                    pixel[i + awtrix_pixel_cursor_y][j + awtrix_pixel_cursor_x].rgb = 0;
                }
            }
        }
    }
    awtrix_pixel_cursor_x += 4;
    return 0;
}


int awtrix_pixel_send_data(pixel_t *pixel) //任务函数
{

    if (pixel == NULL)
        return false;

    uint8_t flag = 0;
    int num = 0;

    for (int i = 0; i < AWTRIX_MAX_COL; i++)
    {
        for (int j = 0; j < AWTRIX_MAX_RAW; j++)
        {
            sunxi_set_led_brightness(num+1, MERAGECOLOR(0,0,0x10));
            if (!flag)
            {
                if (pixel[(j * AWTRIX_MAX_COL) + i].rgb > 0)
                {
                    sunxi_set_led_brightness(num+1, MERAGECOLOR(pixel[(j * AWTRIX_MAX_COL) + i].r, pixel[(j * AWTRIX_MAX_COL) + i].g, pixel[(j * AWTRIX_MAX_COL) + i].b));
                }
                else
                {
                    sunxi_set_led_brightness(num+1, MERAGECOLOR(0,0,0));
                }
            }
            else
            {
                if (pixel[(AWTRIX_MAX_RAW - j - 1) * AWTRIX_MAX_COL + i].rgb > 0)
                {
                    sunxi_set_led_brightness(num+1, MERAGECOLOR(pixel[(AWTRIX_MAX_RAW - j - 1) * AWTRIX_MAX_COL + i].r, pixel[(AWTRIX_MAX_RAW - j - 1) * AWTRIX_MAX_COL + i].g, pixel[(AWTRIX_MAX_RAW - j - 1) * AWTRIX_MAX_COL + i].b));
                }
                else
                {
                    sunxi_set_led_brightness(num+1, MERAGECOLOR(0,0,0));
                }
            }
            num++;
        }
        flag = ~flag;
    }

    return true;
}

int awtrix_init()
{
    awtrix_pixel = malloc(sizeof(pixel_t)*(AWTRIX_MAX_RAW*AWTRIX_MAX_COL));
    if( awtrix_pixel == NULL )
    {
        return false;
    }

    fonts_ascii_5_3_init(ascii_font);

    return true;
}

