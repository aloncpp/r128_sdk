#include <sunxi_hal_ledc.h>
#include <hal_cmd.h>
#include <hal_timer.h>
#include <hal_gpio.h>

#include <awtrix.h>

// 使用RGB 分量合成颜色值
// #define MERAGECOLOR(G, R, B) (((uint32_t)G << 16) | ((uint16_t)R << 8) | B)
// #define PIXEL_NUM 4

pixel_t *pixel = NULL;

// // 生成颜色
// uint32_t WS281x_Wheel(uint8_t wheelPos) {
//   wheelPos = 255 - wheelPos;
//   if (wheelPos < 85) {
//     return MERAGECOLOR(255 - wheelPos * 3, 0, wheelPos * 3);
//   }
//   if (wheelPos < 170) {
//     wheelPos -= 85;
//     return MERAGECOLOR(0, wheelPos * 3, 255 - wheelPos * 3);
//   }
//   wheelPos -= 170;
//   return MERAGECOLOR(wheelPos * 3, 255 - wheelPos * 3, 0);
// }

// // 亮度设置
// uint32_t WS281xLSet(uint32_t rgb, float k) {
//     uint8_t r, g, b;
//     float h, s, v;
//     uint8_t cmax, cmin, cdes;

//     r = (uint8_t) (rgb >> 16);
//     g = (uint8_t) (rgb >> 8);
//     b = (uint8_t) (rgb);

//     cmax = r > g ? r : g;
//     if (b > cmax)
//         cmax = b;
//     cmin = r < g ? r : g;
//     if (b < cmin)
//         cmin = b;
//     cdes = cmax - cmin;

//     v = cmax / 255.0f;
//     s = cmax == 0 ? 0 : cdes / (float) cmax;
//     h = 0;

//     if (cmax == r && g >= b)
//         h = ((g - b) * 60.0f / cdes) + 0;
//     else if (cmax == r && g < b)
//         h = ((g - b) * 60.0f / cdes) + 360;
//     else if (cmax == g)
//         h = ((b - r) * 60.0f / cdes) + 120;
//     else
//         h = ((r - g) * 60.0f / cdes) + 240;

//     v *= k;

//     float f, p, q, t;
//     float rf, gf, bf;
//     int i = ((int) (h / 60) % 6);
//     f = (h / 60) - i;
//     p = v * (1 - s);
//     q = v * (1 - f * s);
//     t = v * (1 - (1 - f) * s);
//     switch (i) {
//     case 0:
//         rf = v;
//         gf = t;
//         bf = p;
//         break;
//     case 1:
//         rf = q;
//         gf = v;
//         bf = p;
//         break;
//     case 2:
//         rf = p;
//         gf = v;
//         bf = t;
//         break;
//     case 3:
//         rf = p;
//         gf = q;
//         bf = v;
//         break;
//     case 4:
//         rf = t;
//         gf = p;
//         bf = v;
//         break;
//     case 5:
//         rf = v;
//         gf = p;
//         bf = q;
//         break;
//     default:
//         break;
//     }

//     r = (uint8_t) (rf * 255.0);
//     g = (uint8_t) (gf * 255.0);
//     b = (uint8_t) (bf * 255.0);

//     return ((uint32_t) r << 16) | ((uint32_t) g << 8) | b;
// }

// // 延时函数
// static inline int msleep(int ms) {
//     vTaskDelay(ms / portTICK_RATE_MS); 
// }

// 测试 LEDC
int ledc_test_loop() {
  int err;
  err = hal_ledc_init();
  if (err) {
    printf("ledc init error\n");
    return -1;
  }

  pixel = awtrix_init();
  if( pixel == NULL )
  {
    printf("pixel init error\n");
    return -1;
  }

  while (1) {
    hal_msleep(33);
    awtrix_pixel_send_data(pixel);
  }

  return 1;
}