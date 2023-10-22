#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <hal_cmd.h>
#include <hal_log.h>
#include <hal_thread.h>
#include "sunxi_hal_twi.h"
#include "spi_camera/spi_camera.h"
#include "spi_camera/spi_sensor/spi_sensor.h"
#include "test_camera.h"

#define SPI_SENSOR_WIDTH 240
#define SPI_SENSOR_HEIGHT 320
#define SPI_SENSOR_TIMEOUT_MS 1000
#define TEST_COUNT 3
uint8_t rgb_24[SPI_SENSOR_WIDTH * SPI_SENSOR_HEIGHT * 3];

struct rt_thread *thread_camera;

int GREYToRGB24(void *RGB24, void *GREY, int width, int height)
{
    unsigned char *src_y = NULL;
    unsigned char *dst_RGB = NULL;
    int temp[3];

    if (!RGB24 || !GREY || width <= 0 || height <= 0) {
        printf(" GREYToRGB24 incorrect input parameter!\n");
        return -1;
    }

    src_y = (unsigned char *)GREY;
    dst_RGB = (unsigned char *)RGB24;

    for (int y = 0; y < height; y ++) {
        for (int x = 0; x < width; x ++) {
            int Y = y * width + x;

            temp[0] = src_y[Y] + ((7289 * 128) >> 12) - 228; //b
            temp[1] = src_y[Y] - ((1415 * 128) >> 12) - ((2936 * 128) >> 12) + 136; //g
            temp[2] = src_y[Y] + ((5765 * 128) >> 12) - 180; //r

            dst_RGB[3 * Y] = (temp[0]<0 ? 0 : temp[0]>255 ? 255 : temp[0]);
            dst_RGB[3 * Y + 1] = (temp[1]<0 ? 0 : temp[1]>255 ? 255 : temp[1]);
            dst_RGB[3 * Y + 2] = (temp[2]<0 ? 0 : temp[2]>255 ? 255 : temp[2]);
        }
    }

    return 0;
}

int YUV422PToRGB24(void *RGB24, void *YUV422P, int width, int height)
{
    unsigned char *src_y = NULL;
    unsigned char *src_u = NULL;
    unsigned char *src_v = NULL;
    unsigned char *dst_RGB = NULL;
    int temp[3];

    if (!RGB24 || !YUV422P || width <= 0 || height <= 0) {
        printf(" YUV422PToRGB24 incorrect input parameter!\n");
        return -1;
    }

    src_y = (unsigned char *)YUV422P;
    src_u = (unsigned char *)YUV422P + width * height;
    src_v = (unsigned char *)YUV422P + width * height * 3 / 2;
    dst_RGB = (unsigned char *)RGB24;

    for (int y = 0; y < height; y ++) {
        for (int x = 0; x < width; x ++) {
            int Y = y * width + x;
            int U = Y >> 1;
            int V = U;

            temp[0] = src_y[Y] + ((7289 * src_u[U]) >> 12) - 228; //b
            temp[1] = src_y[Y] - ((1415 * src_u[U]) >> 12) - ((2936 * src_v[V]) >> 12) + 136; //g
            temp[2] = src_y[Y] + ((5765 * src_v[V]) >> 12) - 180; //r

            dst_RGB[3 * Y] = (temp[0] < 0 ? 0 : temp[0] > 255 ? 255 : temp[0]);
            dst_RGB[3 * Y + 1] = (temp[1] < 0 ? 0 : temp[1] > 255 ? 255 : temp[1]);
            dst_RGB[3 * Y + 2] = (temp[2] < 0 ? 0 : temp[2] > 255 ? 255 : temp[2]);
        }
    }

    return 0;
}

int YUVToBMP(const char *bmp_path, char *yuv_data, ConverFunc func, int width, int height)
{
    //unsigned char *rgb_24 = NULL;
    FILE *fp = NULL;
    BITMAPFILEHEADER BmpFileHeader;
    BITMAPINFOHEADER BmpInfoHeader;

    if (!bmp_path || !yuv_data || !func || width <= 0 || height <= 0) {
        printf(" YUVToBMP incorrect input parameter!\n");
        return -1;
    }

    /* Fill header information */
    memset(&BmpFileHeader, 0, sizeof(BmpFileHeader));
    BmpFileHeader.bfType = 0x4d42;
    BmpFileHeader.bfSize = width * height * 3 + sizeof(BmpFileHeader) + sizeof(BmpInfoHeader);
    BmpFileHeader.bfReserved1 = 0;
    BmpFileHeader.bfReserved2 = 0;
    BmpFileHeader.bfOffBits = sizeof(BmpFileHeader) + sizeof(BmpInfoHeader);

    memset(&BmpInfoHeader, 0, sizeof(BmpInfoHeader));
    BmpInfoHeader.biSize = sizeof(BmpInfoHeader);
    BmpInfoHeader.biWidth = width;
    BmpInfoHeader.biHeight = -height;
    BmpInfoHeader.biPlanes = 0x01;
    BmpInfoHeader.biBitCount = 24;
    BmpInfoHeader.biCompression = 0;
    BmpInfoHeader.biSizeImage = 0;
    //BmpInfoHeader.biXPelsPerMeter = 0;
    //BmpInfoHeader.biYPelsPerMeter = 0;
    BmpInfoHeader.biClrUsed = 0;
    BmpInfoHeader.biClrImportant = 0;

    GREYToRGB24(rgb_24, yuv_data, width, height);

    /* Create bmp file */
    fp = fopen(bmp_path, "wb+");
    if (!fp) {
        printf(" Create bmp file:%s faled!\n", bmp_path);
        return -1;
    }

    fwrite(&BmpFileHeader, sizeof(BmpFileHeader), 1, fp);
    fwrite(&BmpInfoHeader, sizeof(BmpInfoHeader), 1, fp);
    fwrite(rgb_24, width * height * 3, 1, fp);

    fclose(fp);

    return 0;
}

static int read_frame(struct spi_ipeg_mem *spi_mem, int i, int fmt_type)
{
    FILE* fd;
    long long res;
    char bmp_data_path[128];
    char yuv_data_path[128];

    disable_irq();
    hal_log_info("%s line: %d addr = 0x%08x size = %d",
            __func__, __LINE__, spi_mem->buf.addr, spi_mem->buf.size);

    if (fmt_type) {
        sprintf(yuv_data_path, "/data/GREY_%d.bin", i);

        fd = fopen(yuv_data_path, "wb+");
        if (fd < 0) {
            hal_log_info("open /data/nv12.bin error %d", fd);
            return -1;
        }
        res = fwrite(spi_mem->buf.addr, spi_mem->buf.size, 1, fd);
        if (res < 0) {
            hal_log_info("write fail(%d), line%d..", res, __LINE__);
            fclose(fd);
            return -1;
        }
        fclose(fd);
    } else {
        sprintf(bmp_data_path, "/data/GREY_%d.bmp", i);
        if (YUVToBMP(bmp_data_path, spi_mem->buf.addr, GREYToRGB24, SPI_SENSOR_WIDTH, SPI_SENSOR_HEIGHT)) {
            printf("YUVToBMP error\n");
            return -1;
        }
        hal_log_info("write YUV image ok");
    }
    enable_irq();

    return 0;
}

static void main_test(int fmt_type)
{
    struct spi_fmt fmt;
    struct spi_ipeg_mem *spi_mem;

    fmt.width = SPI_SENSOR_WIDTH;
    fmt.height = SPI_SENSOR_HEIGHT;

    hal_spi_set_fmt(&fmt);

    if (hal_spi_sensor_reqbuf(TEST_COUNT) != 0) {
        return ;
    }

    hal_log_info("spi stream on!");
    hal_spi_sensor_s_stream(1);
    for (uint32_t i = 0; i < TEST_COUNT; i++) {
        hal_log_info("test count = %d", i);
        spi_mem = hal_spi_dqbuf(spi_mem, SPI_SENSOR_TIMEOUT_MS);
        if (NULL != spi_mem) {
            read_frame(spi_mem, i, fmt_type);
            hal_spi_qbuf();
        }
        hal_msleep(10);
    }

    hal_spi_sensor_s_stream(0);
    hal_spi_sensor_freebuf();
    hal_log_info("spi stream off!!");
}

static void spi_camera_thread(void *argc)
{
    int fmt_type;
    fmt_type = (int)( *((int *)argc));
    hal_log_info("fmt_type=%s", fmt_type ? "bmp" : "yuv");

    hal_spi_sensor_probe();

    main_test(fmt_type);

    hal_spi_sensor_remove();
    hal_thread_stop(thread_camera);
}

static int cmd_test_spi_camera(int argc, char **argv)
{
    int fmt;

    fmt = strtol(argv[1], NULL, 0);

    if (fmt == 0 || fmt == 1) {
        thread_camera = hal_thread_create(
                            (void *)spi_camera_thread,
                            (void*)&fmt,
                            "spi_camera_thread",
                            409600,
                            HAL_THREAD_PRIORITY_SYS);

        hal_thread_start(thread_camera);
    } else
        printf("please input fmt(0 or 1) 0: bmp 1: yuv\n");
    return 0;

}

FINSH_FUNCTION_EXPORT_CMD(cmd_test_spi_camera, camera_test, spi hal spi camera tests)