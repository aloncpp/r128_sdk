#include <awtrix.h>

#define AWTRIX_DISPLAY_CLOCK_HOURS_12 1 // 12小时制

static int awtrix_clock_num = 0;
static uint8_t awtrix_clock_bar_effect = 0x01;

int awtrix_display_set_clock(pixel_t *pixel, struct tm timeinfo)
{
    uint8_t hours = timeinfo.tm_hour;
    uint8_t minutes = timeinfo.tm_min;
    uint8_t seconds = timeinfo.tm_sec;
    
    int hours_high = 0;
    int hours_low = 0;
    int minutes_high = 0;
    int minutes_low = 0;
    int seconds_high = 0;
    int seconds_low = 0;

    awtrix_pixel_clear(pixel);

    if (AWTRIX_DISPLAY_CLOCK_HOURS_12 == 1)
        awtrix_clock_num++;
    else
        awtrix_clock_num = 0;

    if (awtrix_clock_num >= 35)
    {
        awtrix_clock_num = 0;
    }

    for (int i = 0; i < 7; i++)
    {
        if ((awtrix_clock_bar_effect >> i) & 0x01)
        {
            awtrix_pixel_set_cursor((2 + (i * 4)), 3);
            awtrix_pixel_add_char(pixel, '_', 1, 0x10, 0x00, 0x00);
        }
        else
        {
            awtrix_pixel_set_cursor((2 + (i * 4)), 3);
            awtrix_pixel_add_char(pixel, '_', 1, 0, 0x00, 0x00);
        }
    }

    awtrix_clock_bar_effect = awtrix_clock_bar_effect << 1;

    if (awtrix_clock_bar_effect & 0x80)
    {
        awtrix_clock_bar_effect = 0x01;
    }

    if (awtrix_clock_num < 30)
    {
        hours_high = hours / 10;
        hours_low = hours % 10;
        minutes_high = minutes / 10;
        minutes_low = minutes % 10;
        seconds_high = seconds / 10;
        seconds_low = seconds % 10;

        awtrix_pixel_set_cursor(2, 1);
        awtrix_pixel_add_char(pixel, (hours_high + '0'), 1, 0x10, 0x00, 0x00);
        awtrix_pixel_set_cursor(6, 1);
        awtrix_pixel_add_char(pixel, (hours_low + '0'), 1, 0x10, 0x00, 0x00);
        awtrix_pixel_set_cursor(9, 1);
        awtrix_pixel_add_char(pixel, ':', 0, 0x10, 0x00, 0x00);
        awtrix_pixel_set_cursor(12, 1);
        awtrix_pixel_add_char(pixel, (minutes_high + '0'), 1, 0x10, 0x00, 0x00);
        awtrix_pixel_set_cursor(16, 1);
        awtrix_pixel_add_char(pixel, (minutes_low + '0'), 1, 0x10, 0x00, 0x00);
        awtrix_pixel_set_cursor(19, 1);
        awtrix_pixel_add_char(pixel, ':', 0, 0x10, 0x00, 0x00);
        awtrix_pixel_set_cursor(22, 1);
        awtrix_pixel_add_char(pixel, (seconds_high + '0'), 1, 0x10, 0x00, 0x00);
        awtrix_pixel_set_cursor(26, 1);
        awtrix_pixel_add_char(pixel, (seconds_low + '0'), 1, 0x10, 0x00, 0x00);
    }
    else
    {

        hours_high = ((hours >= 12) ? ((hours - 12) / 10) : (hours / 10));
        hours_low = ((hours >= 12) ? ((hours - 12) % 10) : (hours % 10));
        minutes_high = minutes / 10;
        minutes_low = minutes % 10;
        seconds_high = seconds / 10;
        seconds_low = seconds % 10;

        awtrix_pixel_set_cursor(2, 1);
        awtrix_pixel_add_char(pixel, ((hours < 12) ? 'A' : 'P'), 1, 0x10, 0x00, 0x00);
        awtrix_pixel_set_cursor(6, 1);
        awtrix_pixel_add_char(pixel, 'M', 1, 0x10, 0x00, 0x00);
        awtrix_pixel_set_cursor(10, 1);
        awtrix_pixel_add_char(pixel, ' ', 1, 0x10, 0x00, 0x00);
        awtrix_pixel_set_cursor(12, 1);
        awtrix_pixel_add_char(pixel, (hours_high + '0'), 1, 0x10, 0x00, 0x00);
        awtrix_pixel_set_cursor(16, 1);
        awtrix_pixel_add_char(pixel, (hours_low + '0'), 1, 0x10, 0x00, 0x00);
        awtrix_pixel_set_cursor(19, 1);
        awtrix_pixel_add_char(pixel, ':', 0, 0x10, 0x00, 0x00);
        awtrix_pixel_set_cursor(22, 1);
        awtrix_pixel_add_char(pixel, (minutes_high + '0'), 1, 0x10, 0x00, 0x00);
        awtrix_pixel_set_cursor(26, 1);
        awtrix_pixel_add_char(pixel, (minutes_low + '0'), 1, 0x10, 0x00, 0x00);
    }

    return 0;
}


int awtrix_display_set_clock_2(pixel_t *pixel, struct tm timeinfo)
{
    uint8_t hours = timeinfo.tm_hour;
    uint8_t minutes = timeinfo.tm_min;
    uint8_t seconds = timeinfo.tm_sec;
    uint8_t mday = timeinfo.tm_mday;
    uint8_t mon = timeinfo.tm_mon;
    uint8_t wday = timeinfo.tm_wday;

    int hours_high = hours / 10;
    int hours_low = hours % 10;
    int minutes_high = minutes / 10;
    int minutes_low = minutes % 10;
    int seconds_high = seconds / 10;
    int seconds_low = seconds % 10;

    int mday_high = mday / 10;
    int mday_low = mday % 10;

    awtrix_pixel_clear(pixel);


    awtrix_pixel_set_cursor(0, 1);
    awtrix_pixel_add_char(pixel, (hours_high + '0'), 1, 0x10, 0x10, 0x00);
    awtrix_pixel_set_cursor(4, 1);
    awtrix_pixel_add_char(pixel, (hours_low + '0'), 1, 0x10, 0x10, 0x00);

    if ((seconds_low % 2) == 0)
    {
        awtrix_pixel_set_cursor(7, 1);
        awtrix_pixel_add_char(pixel, ':', 1, 0x00, 0x00, 0x10);
    }
    else
    {
        awtrix_pixel_set_cursor(7, 1);
        awtrix_pixel_add_char(pixel, ' ', 1, 0x00, 0x00, 0x10);
    }

    awtrix_pixel_set_cursor(10, 1);
    awtrix_pixel_add_char(pixel, (minutes_high + '0'), 1, 0x10, 0x10, 0x00);
    awtrix_pixel_set_cursor(14, 1);
    awtrix_pixel_add_char(pixel, (minutes_low + '0'), 1, 0x10, 0x10, 0x00);

    awtrix_pixel_set_cursor(19, 0);
    awtrix_pixel_add_5x6_icon(pixel, 0, 1);

    awtrix_pixel_set_cursor(25, 1);
    awtrix_pixel_add_char(pixel, (mday_high + '0'), 1, 0x10, 0x10, 0x00);
    awtrix_pixel_set_cursor(29, 1);
    awtrix_pixel_add_char(pixel, (mday_low + '0'), 1, 0x10, 0x10, 0x00);

    uint8_t sec_bar_x = (seconds / 4);
    for (int i = 0; i < 15; i++)
    {
        awtrix_pixel_set_cursor(i + 1, 7);
        if (i <= sec_bar_x)
            awtrix_pixel_add_point(pixel, 1, 0x23, 0x00, 0x30);
        else
            awtrix_pixel_add_point(pixel, 1, 0x0d, 0x30, 0x00);
    }

    for (int i = 0; i < 12; i++)
    {
        awtrix_pixel_set_cursor((i) + 19, 7);
        if (i < mon)
            awtrix_pixel_add_point(pixel, 1, 0x23, 0x00, 0x30);
        else
            awtrix_pixel_add_point(pixel, 1, 0x0d, 0x30, 0x00);

    }

    return 0;
}