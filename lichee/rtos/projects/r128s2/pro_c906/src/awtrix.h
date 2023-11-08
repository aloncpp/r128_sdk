#pragma once

#include <stdint.h>
#include <time.h>

#define NUMBER_FONT_START_X (2) // 字体偏移，左上角为原点
#define NUMBER_FONT_START_Y (1)
#define NUMBER_FONT_COL (5) // 数字字体行数
#define NUMBER_FONT_RAW (3) // 数字字体列数
#define AWTRIX_MAX_RAW (8)  // 屏幕像素点行数
#define AWTRIX_MAX_COL (32) // 屏幕像素点列数

#define AWTRIX_WEATHER_MIN_SUNNY 0
#define AWTRIX_WEATHER_MAX_SUNNY 3

#define AWTRIX_WEATHER_MIN_CLOUDY 4
#define AWTRIX_WEATHER_MAX_CLOUDY 9

#define AWTRIX_WEATHER_MIN_RAIN 10
#define AWTRIX_WEATHER_MAX_RAIN 19

#define AWTRIX_WEATHER_MIN_SNOW 20
#define AWTRIX_WEATHER_MAX_SNOW 25

#define AWTRIX_WEATHER_MIN_DUST 26
#define AWTRIX_WEATHER_MAX_DUST 29

#define AWTRIX_WEATHER_MIN_WINDY 30
#define AWTRIX_WEATHER_MAX_WINDY 36

#define AWTRIX_WEATHER_MIN_CLOD 37
#define AWTRIX_WEATHER_MAX_CLOD 37

#define AWTRIX_WEATHER_MIN_HOT 38
#define AWTRIX_WEATHER_MAX_HOT 38

#define AWTRIX_WEATHER_MIN_UNKONW 39
#define AWTRIX_WEATHER_MAX_UNKONW 39

typedef enum weather_type_e
{
    WEATHER_SUN = 0,
    WEATHER_CLOUDY,
    WEATHER_RAIN,
    WEATHER_SNOW,
    WEATHER_DUST,
    WEATHER_WINDY,
    WEATHER_COLD,
    WEATHER_HOT,
    WEATHER_UNKONW,
}weather_type_t;

typedef union pixel_u
{
    struct
    {
        uint8_t b;
        uint8_t g;
        uint8_t r;
    };
    uint32_t rgb;
} pixel_t;

typedef struct
{
    char text[20];   // 天气类型
    int temperature; // 温度
    weather_type_t type;
    int level;
} weather_t;


int awtrix_pixel_set_cursor(int x, int y);

int awtrix_pixel_get_cursor(int *x, int *y);

int awtrix_pixel_add_point(pixel_t *pixel, uint8_t cover, uint8_t red, uint8_t green, uint8_t blue);

int awtrix_pixel_add_char(pixel_t *local_pixel, uint8_t ch, uint8_t cover, uint8_t red, uint8_t green, uint8_t blue);

int awtrix_pixel_add_string(pixel_t *pixel, char *str, uint8_t cover, uint8_t red, uint8_t green, uint8_t blue);

int awtrix_pixel_add_weather(pixel_t *pixel, uint8_t index, uint8_t cover, int brightness);

int awtrix_pixel_add_5x6_icon(pixel_t *pixel, uint8_t index, uint8_t cover);

int awtrix_display_set_clock(pixel_t *pixel, struct tm timeinfo);

int awtrix_display_set_clock_2(pixel_t *pixel, struct tm timeinfo);

int awtrix_display_set_weather(pixel_t *pixel, int brightness);

int awtrix_pixel_send_data(pixel_t *pixel); //任务函数;

int awtrix_pixel_clear(pixel_t *pixel);

int awtrix_set_weather_info(char *msg_buf);

pixel_t *awtrix_get_pixel();

pixel_t *awtrix_init();