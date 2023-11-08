#include <awtrix.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <cJSON.h>

weather_t local_weather;

int awtrix_set_weather_info(char *msg_buf)
{
    cJSON *root = cJSON_Parse(msg_buf); //读取心知天气回调包
    if (root == NULL)
        return -1;
    cJSON *array = cJSON_GetObjectItem(root, "results"); //读取关键字
    if (array == NULL)
        return -1;
    cJSON *results0 = cJSON_GetArrayItem(array, 0); //判断位置
    if (results0 == NULL)
        return -1;
    cJSON *location = cJSON_GetObjectItem(results0, "location"); //读取关键字
    if (location == NULL)
        return -1;
    cJSON *now = cJSON_GetObjectItem(results0, "now"); //读取关键字
    if (now == NULL)
        return -1;
    cJSON *now_code = cJSON_GetObjectItem(now, "code");
    if (now_code == NULL)
        return -1;
    cJSON *now_text = cJSON_GetObjectItem(now, "text");
    if (now_text == NULL)
        return -1;
    cJSON *now_temperature = cJSON_GetObjectItem(now, "temperature");
    if (now_temperature == NULL)
        return -1;

    int code = atoi(now_code->valuestring);

    if ((code >= AWTRIX_WEATHER_MIN_SUNNY) && (code <= AWTRIX_WEATHER_MAX_SUNNY))
    {
        local_weather.type = WEATHER_SUN;
        local_weather.level = code - AWTRIX_WEATHER_MIN_SUNNY;
    }
    else if ((code >= AWTRIX_WEATHER_MIN_CLOUDY) && (code <= AWTRIX_WEATHER_MAX_CLOUDY))
    {
        local_weather.type = WEATHER_CLOUDY;
        local_weather.level = code - AWTRIX_WEATHER_MIN_CLOUDY;
    }
    else if ((code >= AWTRIX_WEATHER_MIN_RAIN) && (code <= AWTRIX_WEATHER_MAX_RAIN))
    {
        local_weather.type = WEATHER_RAIN;
        local_weather.level = code - AWTRIX_WEATHER_MIN_RAIN;
    }
    else if ((code >= AWTRIX_WEATHER_MIN_SNOW) && (code <= AWTRIX_WEATHER_MAX_SNOW))
    {
        local_weather.type = WEATHER_SNOW;
        local_weather.level = code - AWTRIX_WEATHER_MIN_SNOW;
    }
    else if ((code >= AWTRIX_WEATHER_MIN_DUST) && (code <= AWTRIX_WEATHER_MAX_DUST))
    {
        local_weather.type = WEATHER_DUST;
        local_weather.level = code - AWTRIX_WEATHER_MIN_DUST;
    }
    else if ((code >= AWTRIX_WEATHER_MIN_WINDY) && (code <= AWTRIX_WEATHER_MAX_WINDY))
    {
        local_weather.type = WEATHER_WINDY;
        local_weather.level = code - AWTRIX_WEATHER_MIN_WINDY;
    }
    else if ((code >= AWTRIX_WEATHER_MIN_CLOD) && (code <= AWTRIX_WEATHER_MAX_CLOD))
    {
        local_weather.type = WEATHER_COLD;
        local_weather.level = code - AWTRIX_WEATHER_MIN_CLOD;
    }
    else if ((code >= AWTRIX_WEATHER_MIN_HOT) && (code <= AWTRIX_WEATHER_MAX_HOT))
    {
        local_weather.type = WEATHER_HOT;
        local_weather.level = code - AWTRIX_WEATHER_MIN_HOT;
    }
    else if ((code >= AWTRIX_WEATHER_MIN_UNKONW) && (code <= AWTRIX_WEATHER_MAX_UNKONW))
    {
        local_weather.type = WEATHER_UNKONW;
        local_weather.level = code - AWTRIX_WEATHER_MIN_UNKONW;
    }
    else
    {
        local_weather.type = -1;
        local_weather.level = -1;
    }

    memset(local_weather.text, '\0', sizeof(local_weather.text));
    memcpy(local_weather.text, now_text->valuestring, strlen(now_text->valuestring));
    local_weather.temperature = atoi(now_temperature->valuestring);

    printf("local_weather.text: %s\n", local_weather.text);
    printf("local_weather.temperature: %d\n", local_weather.temperature);
    printf("local_weather.type: %d\n", local_weather.type);
    printf("local_weather.level: %d\n", local_weather.level);

    cJSON_Delete(root);

    // free(weather_post_buff);
    // weather_post_buff = NULL;
    
    return 0;
}

int awtrix_display_set_weather(pixel_t *pixel, int brightness)
{
    int temp_high = local_weather.temperature / 10;
    int temp_low = local_weather.temperature % 10;
    
    awtrix_pixel_clear(pixel);

    if( local_weather.temperature < 0 )
    {
        awtrix_pixel_set_cursor(2, 1);
        awtrix_pixel_add_char(pixel, '-', 1, 0x10, 0x00, 0x00);
    }
    awtrix_pixel_set_cursor(4, 1);
    awtrix_pixel_add_char(pixel, (temp_high + '0'), 1, 0x10, 0x00, 0x00);
    awtrix_pixel_set_cursor(8, 1);
    awtrix_pixel_add_char(pixel, (temp_low + '0'), 1, 0x10, 0x00, 0x00);
    awtrix_pixel_set_cursor(12, 1);
    awtrix_pixel_add_point(pixel, 1, 0x10, 0x00, 0x00);
    awtrix_pixel_set_cursor(12, 2);
    awtrix_pixel_add_point(pixel, 1, 0x10, 0x00, 0x00);
    awtrix_pixel_set_cursor(13, 1);
    awtrix_pixel_add_point(pixel, 1, 0x10, 0x00, 0x00);
    awtrix_pixel_set_cursor(13, 2);
    awtrix_pixel_add_point(pixel, 1, 0x10, 0x00, 0x00);
    awtrix_pixel_set_cursor(15, 0);
    awtrix_pixel_add_weather(pixel, local_weather.type, 1, brightness);
}
