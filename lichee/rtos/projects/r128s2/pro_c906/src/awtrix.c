#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <sunxi_hal_ledc.h>
#include <hal_cmd.h>
#include <hal_timer.h>
#include <hal_gpio.h>

#include <awtrix.h>
#include "fonts.h"
#include "shape.h"
#include <time.h>

#define MERAGECOLOR(R, G, B) (((uint32_t)R << 16) | ((uint16_t)G << 8) | B)

static int awtrix_pixel_cursor_x = 0;
static int awtrix_pixel_cursor_y = 0;

ascii_5_3_font_t ascii_font[FONTS_ASCII_5_3_NUMBER];
weather_shape_t weather_shape[8];
icon_shape_t icon_8x8_shape[8];
icon_shape_t icon_5x6_shape[1];

pixel_t *local_pixel;

unsigned int awtrix_buf[AWTRIX_MAX_RAW*AWTRIX_MAX_COL];

SemaphoreHandle_t xMutexHandle;

int awtrix_pixel_lock()
{
    BaseType_t xReturn = pdPASS;
    xReturn = xSemaphoreTake(xMutexHandle, portMAX_DELAY);
    if( pdPASS != xReturn )
    {
        return FAIL;
    }
    return OK;
}

int awtrix_pixel_unlock()
{
    BaseType_t xReturn = pdPASS;
    xReturn = xSemaphoreGive(xMutexHandle);
    if( pdPASS != xReturn )
    {
        return FAIL;
    }
    return OK;
}

// 左上角(0,0)
int awtrix_pixel_set_cursor(int x, int y)
{
    if ((x < 0) || (x >= AWTRIX_MAX_COL) || (y < 0) || (y >= AWTRIX_MAX_RAW))
        return -1;
    awtrix_pixel_cursor_x = x;
    awtrix_pixel_cursor_y = y;

    return 0;
}
int awtrix_pixel_get_cursor(int *x, int *y)
{
    if ((x == NULL) || (y == NULL))
        return -1;
    *x = awtrix_pixel_cursor_x;
    *y = awtrix_pixel_cursor_y;

    return 0;
}


int awtrix_pixel_add_char(pixel_t *pixel, uint8_t ch, uint8_t cover, uint8_t red, uint8_t green, uint8_t blue)
{

    pixel_t *p[AWTRIX_MAX_RAW];

    awtrix_pixel_lock();

    for (int i = 0; i < AWTRIX_MAX_RAW; i++)
        p[i] = &pixel[i * AWTRIX_MAX_COL];

    if ((ch < ' ') || (ch > '`'))
        return -1;

    for (int i = 0; i < 5; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            if (ascii_font[ch - ' '].font[i * 3 + j] == 1)
            {
                p[i + awtrix_pixel_cursor_y][j + awtrix_pixel_cursor_x].r = red;
                p[i + awtrix_pixel_cursor_y][j + awtrix_pixel_cursor_x].g = green;
                p[i + awtrix_pixel_cursor_y][j + awtrix_pixel_cursor_x].b = blue;
            }
            else
            {
                if (cover == 1)
                {
                    p[i + awtrix_pixel_cursor_y][j + awtrix_pixel_cursor_x].rgb = 0;
                }
            }
        }
    }
    awtrix_pixel_cursor_x += 4;

    awtrix_pixel_unlock();

    return 0;
}

int awtrix_pixel_add_point(pixel_t *pixel, uint8_t cover, uint8_t red, uint8_t green, uint8_t blue)
{
    pixel_t *p[AWTRIX_MAX_RAW];

    awtrix_pixel_lock();

    for (int i = 0; i < AWTRIX_MAX_RAW; i++)
        p[i] = &pixel[i * AWTRIX_MAX_COL];

    p[awtrix_pixel_cursor_y][awtrix_pixel_cursor_x].r = red;
    p[awtrix_pixel_cursor_y][awtrix_pixel_cursor_x].g = green;
    p[awtrix_pixel_cursor_y][awtrix_pixel_cursor_x].b = blue;

    awtrix_pixel_unlock();

    return 0;
}

int awtrix_pixel_clear(pixel_t *pixel)
{
    awtrix_pixel_lock();

    for (int i = 0; i < AWTRIX_MAX_RAW; i++)
    {
        for (int j = 0; j < AWTRIX_MAX_COL; j++)
        {
            pixel[i * AWTRIX_MAX_COL + j].rgb = 0;
        }
    }

    awtrix_pixel_unlock();

    return 0;
}


int awtrix_pixel_add_5x6_icon(pixel_t *pixel, uint8_t index, uint8_t cover)
{

    pixel_t *p[AWTRIX_MAX_RAW];

    awtrix_pixel_lock();

    for (int i = 0; i < AWTRIX_MAX_RAW; i++)
        p[i] = &pixel[i * AWTRIX_MAX_COL];

    for (int i = 0; i < SHAPE_5x6_RAW; i++)
    {
        for (int j = 0; j < SHAPE_5x6_COL; j++)
        {
            if (icon_5x6_shape[index].shape[i * SHAPE_5x6_COL + j] == 1)
            {
                p[i + awtrix_pixel_cursor_y][j + awtrix_pixel_cursor_x].rgb = SHAPE_5x6_COLOR_1;
            }
            else if (icon_5x6_shape[index].shape[i * SHAPE_5x6_COL + j] == 2)
            {
                p[i + awtrix_pixel_cursor_y][j + awtrix_pixel_cursor_x].rgb = SHAPE_5x6_COLOR_2;
            }
            else if (icon_5x6_shape[index].shape[i * SHAPE_5x6_COL + j] == 3)
            {
                p[i + awtrix_pixel_cursor_y][j + awtrix_pixel_cursor_x].rgb = SHAPE_5x6_COLOR_3;
            }
            else
            {
                if (cover == 1)
                {
                    p[i + awtrix_pixel_cursor_y][j + awtrix_pixel_cursor_x].rgb = 0;
                }
            }
        }
    }
    awtrix_pixel_cursor_x += 4;

    awtrix_pixel_unlock();

    return 0;
}

int awtrix_pixel_add_string(pixel_t *pixel, char *str, uint8_t cover, uint8_t red, uint8_t green, uint8_t blue)
{
    int ret = -1;
    for (int i = 0; i < strlen(str); i++)
    {
        ret = awtrix_pixel_add_char(pixel, str[i], cover, red, green, blue);
        if (ret != 0)
            return ret;
    }
    return 0;
}


int awtrix_pixel_add_weather(pixel_t *pixel, uint8_t index, uint8_t cover, int brightness)
{
    pixel_t *p[AWTRIX_MAX_RAW];

    for (int i = 0; i < AWTRIX_MAX_RAW; i++)
        p[i] = &pixel[i * AWTRIX_MAX_COL];

    for (int i = 0; i < SHAPE_8x8_RAW; i++)
    {
        for (int j = 0; j < SHAPE_8x8_COL; j++)
        {
            if (weather_shape[index].shape[i * SHAPE_8x8_COL + j])
            {
                pixel_t color = (pixel_t)weather_shape[index].shape[i * SHAPE_8x8_COL + j];
                p[i + awtrix_pixel_cursor_y][j + awtrix_pixel_cursor_x].r = (uint8_t)(color.r * brightness / 100);
                p[i + awtrix_pixel_cursor_y][j + awtrix_pixel_cursor_x].g = (uint8_t)(color.g * brightness / 100);
                p[i + awtrix_pixel_cursor_y][j + awtrix_pixel_cursor_x].b = (uint8_t)(color.b * brightness / 100);

            }
            else
            {
                if (cover == 1)
                {
                    p[i + awtrix_pixel_cursor_y][j + awtrix_pixel_cursor_x].rgb = 0;
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

    awtrix_pixel_lock();

    for (int i = 0; i < AWTRIX_MAX_COL; i++)
    {
        for (int j = 0; j < AWTRIX_MAX_RAW; j++)
        {
            if (!flag)
            {
                if (pixel[(j * AWTRIX_MAX_COL) + i].rgb > 0)
                {
                    awtrix_buf[num] = MERAGECOLOR(pixel[(j * AWTRIX_MAX_COL) + i].r, pixel[(j * AWTRIX_MAX_COL) + i].g, pixel[(j * AWTRIX_MAX_COL) + i].b);
                    
                }
                else
                {
                    awtrix_buf[num] = 0;
                }
            }
            else
            {
                if (pixel[(AWTRIX_MAX_RAW - j - 1) * AWTRIX_MAX_COL + i].rgb > 0)
                {
                    awtrix_buf[num] =  MERAGECOLOR(pixel[(AWTRIX_MAX_RAW - j - 1) * AWTRIX_MAX_COL + i].r, pixel[(AWTRIX_MAX_RAW - j - 1) * AWTRIX_MAX_COL + i].g, pixel[(AWTRIX_MAX_RAW - j - 1) * AWTRIX_MAX_COL + i].b);
                }
                else
                {
                    awtrix_buf[num] = 0;
                }
            }
            num++;
        }
        flag = ~flag;
    }

    sunxi_set_led_awtrix(AWTRIX_MAX_RAW*AWTRIX_MAX_COL, awtrix_buf);

    awtrix_pixel_unlock();

    return true;
}

pixel_t *awtrix_get_pixel()
{
    return local_pixel;
}

pixel_t *awtrix_init()
{
    local_pixel = malloc(sizeof(pixel_t)*(AWTRIX_MAX_RAW*AWTRIX_MAX_COL));
    if( local_pixel == NULL )
    {
        return NULL;
    }
    xMutexHandle = xSemaphoreCreateMutex();
    if(xMutexHandle == NULL)
    {
        return NULL;
    }

    memset(local_pixel, 0x00, sizeof(pixel_t)*(AWTRIX_MAX_RAW*AWTRIX_MAX_COL));

    fonts_ascii_5_3_init(ascii_font);
    weather_shape_init(weather_shape);
    shape_8x8_init(icon_8x8_shape);
    shape_5x6_init(icon_5x6_shape);

    return local_pixel;
}
