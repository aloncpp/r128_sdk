#include <stdio.h>
#include <gif_lib.h>
#include <getarg.h>
#include <hal_mem.h>
#include <hal_thread.h>
#include <hal_queue.h>
#include <hal_time.h>
#include <string.h>
#include <limits.h>
#include <console.h>
#include <FreeRTOS.h>
#include <task.h>
#include <decompress_gif.h>

/* 打印出每一帧的解码转码时间 */
//#define DECOMPRESS_GIF_PERFORMANCE_DEBUG

/*
    所有组件共用一个线程，可以减小rtos频繁创建、销毁线程的一些开销，但每张gif的解码将会顺序执行；
    如果只有一个解码器运行，则没有影响；
    TODO: 如果有并行解码的需求，可以改进为交替解码；
*/
#define DECOMPRESS_GIF_SHARED_THREAD

/* 打印出每一帧的解码细节 */
//#define DECOMPRESS_GIF_DEBUG_PRINT

/* 每一帧的解码结果都会单独保存，好处是可以向前输出图像、循环输出图像，对内存开销较大，没有采用这种方式，也没有完成该功能 */
//#define DRAW_TO_SAVED_IMAGE

/* 出于重复利用文件buffer的目的，解码模块不释放获取的文件buffer，buffer还是由调用者来管理 */
#define DECOMPRESS_GIF_KEEP_INPUT_BUFFER

/* 节约内存开销，由连续解码将数据放入队列，改为使用请求一次，获取一次的方式 */
/* 代价是发起请求后才会进行格式转换，做不到立即返回 */
/* 需要零拷贝或最多只能申请到1个buffer时启用比较合适，限制申请的buffer可以通过修改queue的深度来解决，buffer数量 = 深度 + 1 */
#define DECOMPRESS_GIF_REQ_QUEUE

#define IMAGE_OUTPUT_FORMAT (IMAGE_FORMAT_ARGB8888)

#if defined(DECOMPRESS_GIF_DEBUG_PRINT)
#define DGIF_DEBUG_PRINT(fmt, arg...)       do{ printf(fmt, ##arg); }while(0)
#define DGIF_DEBUG(fmt, arg...)             do{ printf("\e[35m[DGIF-DEBUG]<%s:%4d>\e[0m" fmt, __func__, __LINE__, ##arg); }while(0)
#define DGIF_INFO(fmt, arg...)              do{ printf("\e[34m[DGIF-INFO] <%s:%4d>\e[0m" fmt, __func__, __LINE__, ##arg); }while(0)
#define DGIF_WARN(fmt, arg...)              do{ printf("\e[33m[DGIF-WARN] <%s:%4d>\e[0m" fmt, __func__, __LINE__, ##arg); }while(0)
#define DGIF_ERROR(fmt, arg...)             do{ printf("\e[31m[DGIF-ERROR]<%s:%4d>\e[0m" fmt, __func__, __LINE__, ##arg); }while(0)
#define DGIF_API_ENTRY_DEBUG(fmt, arg...)   do{ printf("\e[35m[DGIF-ENTRY]<%s:%4d>\e[0m" fmt, __func__, __LINE__, ##arg); }while(0)
#define DGIF_API_EXIT_DEBUG(fmt, arg...)    do{ printf("\e[35m[DGIF-EXIT] <%s:%4d>\e[0m" fmt, __func__, __LINE__, ##arg); }while(0)
#else
#define DGIF_DEBUG_PRINT(fmt, arg...)       do{  }while(0)
#define DGIF_DEBUG(fmt, arg...)             do{  }while(0)
#define DGIF_INFO(fmt, arg...)              do{ printf("\e[34m[DGIF-INFO] \e[0m " fmt, ##arg); }while(0)
#define DGIF_WARN(fmt, arg...)              do{ printf("\e[33m[DGIF-WARN] \e[0m " fmt, ##arg); }while(0)
#define DGIF_ERROR(fmt, arg...)             do{ printf("\e[31m[DGIF-ERROR]\e[0m " fmt, ##arg); }while(0)
#define DGIF_API_ENTRY_DEBUG(fmt, arg...)   do{  }while(0)
#define DGIF_API_EXIT_DEBUG(fmt, arg...)    do{  }while(0)
#endif

#if defined(DECOMPRESS_GIF_SHARED_THREAD)
struct dgif_shared_thread_t {
    hal_queue_t queue;
    unsigned int run;
    void *thread;
};

struct dgif_thread_handler_t {
    void (*fun)(void *arg);
    void *arg;
};

static struct dgif_shared_thread_t g_shared_thread;
static void dgif_shared_thread(void *arg)
{
    struct dgif_shared_thread_t *thread = (struct dgif_shared_thread_t *)arg;

    while (thread->run) {
        struct dgif_thread_handler_t handler;
        while (0 > hal_queue_recv(thread->queue, (void *)&handler, 1000 * 10)) {
            DGIF_DEBUG("%s:%u error!\n", __func__, __LINE__);
        }
        handler.fun(handler.arg);
    }

    thread->thread = NULL;
    hal_thread_stop(NULL);
}

static int run_thread(void (*fun)(void *arg), void *arg)
{
    if (g_shared_thread.queue == NULL) {
        g_shared_thread.queue = hal_queue_create("shared_thread_queue", sizeof(struct dgif_thread_handler_t), 4);
        g_shared_thread.run = 1;
        g_shared_thread.thread = hal_thread_create(dgif_shared_thread, &g_shared_thread, "dgif_shread", 4096, 1);
        if (!g_shared_thread.thread) {
            hal_queue_t queue = g_shared_thread.queue;
            g_shared_thread.queue = NULL;
            hal_queue_delete(queue);
        }
    }

    if (!g_shared_thread.queue) {
        return -1;
    }

    struct dgif_thread_handler_t handler = {
        .fun = fun,
        .arg = arg
    };

    while (0 > hal_queue_send_wait(g_shared_thread.queue, (void *)&handler, 1000 * 10)) {
        DGIF_DEBUG("%s:%u error!\n", __func__, __LINE__);
    }

    return 0;
}
#endif

struct dgif_t {
    // giflib
    GifFileType *GifFile;
    // stream
    unsigned char *buf;
    unsigned int size;
    unsigned int pos;
    // output
    unsigned int finish;
#if defined(DECOMPRESS_GIF_REQ_QUEUE)
    hal_queue_t req_queue;
#endif
    hal_queue_t queue;
    //private
    unsigned int run;
#if defined(DECOMPRESS_GIF_SHARED_THREAD)
    int thread_alive;
#else
    void *thread;
#endif
};

struct dgif_out_t {
    void *data;
    unsigned int size;
};

static int decompress_gif_input_fun(GifFileType *gif, GifByteType *buf, int size)
{
    struct dgif_t *dgif = (struct dgif_t *)gif->UserData;

    if ((dgif->size - dgif->pos) < size) {
        DGIF_INFO("read end!\n");
        size = dgif->size - dgif->pos;
    }

    memcpy(buf, &dgif->buf[dgif->pos], size);
    dgif->pos += size;

    return size;
}

static inline void GifDestroyScreenBuffer(GifRowType *ScreenBuffer, GifFileType *GifFile)
{
    if (!ScreenBuffer)
        return;

    int i;
    for (i = 0; i < GifFile->SHeight; i++) {
        if (ScreenBuffer[i])
            hal_free(ScreenBuffer[i]);
    }
    hal_free(ScreenBuffer);
}

static inline GifRowType *GifCreateScreenBufferWithBackGroundColor(GifFileType *GifFile)
{
    int size, i;
    GifRowType *ScreenBuffer = NULL;
    
    size = GifFile->SHeight * sizeof(*ScreenBuffer);
    ScreenBuffer = hal_malloc(size);
    if (!ScreenBuffer) {
        DGIF_ERROR("%s:%u no memory!\n", __func__, __LINE__);
        goto err;
    }
    memset(ScreenBuffer, 0, size);

    size = GifFile->SWidth * sizeof(GifPixelType);
    ScreenBuffer[0] = hal_malloc(size);
    if (!ScreenBuffer[0]) {
        DGIF_ERROR("%s:%u no memory!\n", __func__, __LINE__);
        goto err;
    }
    for (i = 0; i < GifFile->SWidth; i++)
        ScreenBuffer[0][i] = GifFile->SBackGroundColor;

    for (i = 1; i < GifFile->SHeight; i++) {
        ScreenBuffer[i] = hal_malloc(size);
        if (!ScreenBuffer[i]) {
            DGIF_ERROR("%s:%u no memory!\n", __func__, __LINE__);
            goto err;
        }
        memcpy(ScreenBuffer[i], ScreenBuffer[0], size);
    }

    return ScreenBuffer;
err:
    GifDestroyScreenBuffer(ScreenBuffer, GifFile);
    return NULL;
}
#if defined(DECOMPRESS_GIF_REQ_QUEUE)
static inline void ScreenBuffer2ARGB8888(void *image, GifRowType *ScreenBuffer, unsigned int width, unsigned int height, ColorMapObject *ColorMap, int TransparentColor)
{
    unsigned int *img_data = (unsigned int *)image;
    unsigned int w, h;

    if (TransparentColor == -1) {
        for (h = 0; h < height; h++) {
            GifRowType GifRow = ScreenBuffer[h];
            for (w = 0; w < width; w++) {
                GifColorType *ColorMapEntry = &ColorMap->Colors[GifRow[w]];
                *img_data++ = 0xff000000 | (ColorMapEntry->Red << 16) | (ColorMapEntry->Green << 8) | ColorMapEntry->Blue;
            }
        }
    } else {
        for (h = 0; h < height; h++) {
            GifRowType GifRow = ScreenBuffer[h];
            for (w = 0; w < width; w++) {
                if (TransparentColor == GifRow[w]) {
                    img_data++;
                    continue;
                }
                GifColorType *ColorMapEntry = &ColorMap->Colors[GifRow[w]];
                *img_data++ = 0xff000000 | (ColorMapEntry->Red << 16) | (ColorMapEntry->Green << 8) | ColorMapEntry->Blue;
            }
        }
    }
}

static inline void ScreenBuffer2RGB888(void *image, GifRowType *ScreenBuffer, unsigned int width, unsigned int height, ColorMapObject *ColorMap, int TransparentColor)
{
    unsigned char *img_data = (unsigned char *)image;
    unsigned int w, h;

    if (TransparentColor == -1) {
        for (h = 0; h < height; h++) {
            GifRowType GifRow = ScreenBuffer[h];
            for (w = 0; w < width; w++) {
                GifColorType *ColorMapEntry = &ColorMap->Colors[GifRow[w]];
                *img_data++ = ColorMapEntry->Blue;
                *img_data++ = ColorMapEntry->Green;
                *img_data++ = ColorMapEntry->Red;
            }
        }
    } else {
        for (h = 0; h < height; h++) {
            GifRowType GifRow = ScreenBuffer[h];
            for (w = 0; w < width; w++) {
                if (TransparentColor == GifRow[w]) {
                    img_data+=3;
                    continue;
                }
                GifColorType *ColorMapEntry = &ColorMap->Colors[GifRow[w]];
                *img_data++ = ColorMapEntry->Blue;
                *img_data++ = ColorMapEntry->Green;
                *img_data++ = ColorMapEntry->Red;
            }
        }
    }
}

static inline void ScreenBuffer2RGB565(void *image, GifRowType *ScreenBuffer, unsigned int width, unsigned int height, ColorMapObject *ColorMap, int TransparentColor)
{
    unsigned short *img_data = (unsigned short *)image;
    unsigned int w, h;

    if (TransparentColor == -1) {
        for (h = 0; h < height; h++) {
            GifRowType GifRow = ScreenBuffer[h];
            for (w = 0; w < width; w++) {
                GifColorType *ColorMapEntry = &ColorMap->Colors[GifRow[w]];
                *img_data++ = ((ColorMapEntry->Red & 0xf8) << 8) | ((ColorMapEntry->Green & 0xfc) << 3) | (ColorMapEntry->Blue >> 3);
            }
        }
    } else {
        for (h = 0; h < height; h++) {
            GifRowType GifRow = ScreenBuffer[h];
            for (w = 0; w < width; w++) {
                if (TransparentColor == GifRow[w]) {
                    img_data++;
                    continue;
                }
                GifColorType *ColorMapEntry = &ColorMap->Colors[GifRow[w]];
                *img_data++ = ((ColorMapEntry->Red & 0xf8) << 8) | ((ColorMapEntry->Green & 0xfc) << 3) | (ColorMapEntry->Blue >> 3);
            }
        }
    }
}

static inline int ScreenBuffer2DgifOutput(struct dgif_out_t *out, GifRowType *ScreenBuffer, unsigned int width, unsigned int height, ColorMapObject *ColorMap, int TransparentColor, int delay)
{
    struct gif_image_hdr *hdr;
    unsigned char *data = NULL;
    unsigned char *img_data = NULL;
    if (!out || !ScreenBuffer || !ColorMap || !width || !height) {
        DGIF_ERROR("%s:%u para error!\n", __func__, __LINE__);
        goto err;
    }

    unsigned int size;
    if (out->data) {
        hdr = (struct gif_image_hdr *)out->data;
        img_data = sizeof(*hdr) + (unsigned char *)out->data;
        if (hdr->fmt == 0) {
            hdr->fmt = IMAGE_FORMAT_ARGB8888;
        } else if (hdr->fmt != IMAGE_FORMAT_ARGB8888 && hdr->fmt != IMAGE_FORMAT_RGB888 && hdr->fmt != IMAGE_FORMAT_RGB565) {
            DGIF_ERROR("%s format error! %u\n", __func__, hdr->fmt);
            hdr->fmt = IMAGE_FORMAT_ARGB8888;
        }
        size = sizeof(unsigned int) * 4 + width * height * hdr->fmt;
        if (size > out->size) {
            DGIF_WARN("%s:%u buffer too small!\n", __func__, __LINE__);
            data = hal_malloc(size);
            if (!data) {
                DGIF_ERROR("%s:%u no memory!\n", __func__, __LINE__);
                goto err;
            }
            memcpy(data, hdr, sizeof(*hdr));
            hdr = (struct gif_image_hdr *)data;
            img_data = sizeof(*hdr) + (unsigned char *)data;
        }
        hdr->width = width;
        hdr->height = height;
        hdr->delay = delay;
    } else {
        size = sizeof(unsigned int) * 4 + width * height * 4;
        data = hal_malloc(size);
        if (!data) {
            DGIF_ERROR("%s:%u no memory!\n", __func__, __LINE__);
            goto err;
        }
        hdr = (struct gif_image_hdr *)data;
        img_data = sizeof(*hdr) + (unsigned char *)data;
        hdr->width = width;
        hdr->height = height;
        hdr->fmt = IMAGE_FORMAT_ARGB8888;
        hdr->delay = delay;
    }

    if (hdr->fmt == IMAGE_FORMAT_ARGB8888)
        ScreenBuffer2ARGB8888(img_data, ScreenBuffer, width, height, ColorMap, TransparentColor);
    else if (hdr->fmt == IMAGE_FORMAT_RGB888)
        ScreenBuffer2RGB888(img_data, ScreenBuffer, width, height, ColorMap, TransparentColor);
    else if (hdr->fmt == IMAGE_FORMAT_RGB565)
        ScreenBuffer2RGB565(img_data, ScreenBuffer, width, height, ColorMap, TransparentColor);
    else {
        DGIF_ERROR("%s format error 2! %u\n", __func__, hdr->fmt);
        goto err;
    }

    if (data) {
        out->data = data;
        out->size = size;
    }

    return 0;
err:
    if (data) {
        hal_free(data);
    }
    return -1;
}
#elif (IMAGE_OUTPUT_FORMAT == IMAGE_FORMAT_ARGB8888)
static inline int ScreenBuffer2DgifOutput(struct dgif_out_t *out, GifRowType *ScreenBuffer, unsigned int width, unsigned int height, ColorMapObject *ColorMap, int TransparentColor, int delay)
{
    unsigned char *data = NULL;
    if (!out || !ScreenBuffer || !ColorMap || !width || !height) {
        DGIF_ERROR("%s:%u para error!\n", __func__, __LINE__);
        goto err;
    }

    unsigned int size = sizeof(unsigned int) * 4 + width * height * 4;
    data = hal_malloc(size);
    if (!data) {
        DGIF_ERROR("%s:%u no memory!\n", __func__, __LINE__);
        goto err;
    }

    unsigned int *pwidth = (unsigned int *)(data + sizeof(int) * 0);
    unsigned int *pheight = (unsigned int *)(data + sizeof(int) * 1);
    unsigned int *pfmt = (unsigned int *)(data + sizeof(int) * 2);
    unsigned int *pdelay = (unsigned int *)(data + sizeof(int) * 3);
    unsigned int *img_data = (unsigned int *)(data + sizeof(int) * 4);

    *pwidth = width;
    *pheight = height;
    *pfmt = IMAGE_FORMAT_ARGB8888;
    *pdelay = delay;

    unsigned int w, h;

    if (TransparentColor == -1) {
        for (h = 0; h < height; h++) {
            GifRowType GifRow = ScreenBuffer[h];
            for (w = 0; w < width; w++) {
                GifColorType *ColorMapEntry = &ColorMap->Colors[GifRow[w]];
                *img_data++ = 0xff000000 | (ColorMapEntry->Red << 16) | (ColorMapEntry->Green << 8) | ColorMapEntry->Blue;
            }
        }
    } else {
        for (h = 0; h < height; h++) {
            GifRowType GifRow = ScreenBuffer[h];
            for (w = 0; w < width; w++) {
                if (TransparentColor == GifRow[w]) {
                    img_data++;
                    continue;
                }
                GifColorType *ColorMapEntry = &ColorMap->Colors[GifRow[w]];
                *img_data++ = 0xff000000 | (ColorMapEntry->Red << 16) | (ColorMapEntry->Green << 8) | ColorMapEntry->Blue;
            }
        }
    }

    out->data = data;
    out->size = size;
    return 0;
err:
    if (data) {
        hal_free(data);
    }
    return -1;
}
#elif (IMAGE_OUTPUT_FORMAT == IMAGE_FORMAT_RGB888)
static inline int ScreenBuffer2DgifOutput(struct dgif_out_t *out, GifRowType *ScreenBuffer, unsigned int width, unsigned int height, ColorMapObject *ColorMap, int TransparentColor, int delay)
{
    unsigned char *data = NULL;
    if (!out || !ScreenBuffer || !ColorMap || !width || !height) {
        DGIF_ERROR("%s:%u para error!\n", __func__, __LINE__);
        goto err;
    }

    unsigned int size = sizeof(unsigned int) * 4 + width * height * 3;
    data = hal_malloc(size);
    if (!data) {
        DGIF_ERROR("%s:%u no memory!\n", __func__, __LINE__);
        goto err;
    }

    unsigned int *pwidth = (unsigned int *)(data + sizeof(int) * 0);
    unsigned int *pheight = (unsigned int *)(data + sizeof(int) * 1);
    unsigned int *pfmt = (unsigned int *)(data + sizeof(int) * 2);
    unsigned int *pdelay = (unsigned int *)(data + sizeof(int) * 3);
    unsigned char *img_data = data + sizeof(int) * 4;

    *pwidth = width;
    *pheight = height;
    *pfmt = IMAGE_FORMAT_RGB888;
    *pdelay = delay;

    unsigned int w, h;

    if (TransparentColor == -1) {
        for (h = 0; h < height; h++) {
            GifRowType GifRow = ScreenBuffer[h];
            for (w = 0; w < width; w++) {
                GifColorType *ColorMapEntry = &ColorMap->Colors[GifRow[w]];
                *img_data++ = ColorMapEntry->Blue;
                *img_data++ = ColorMapEntry->Green;
                *img_data++ = ColorMapEntry->Red;
            }
        }
    } else {
        for (h = 0; h < height; h++) {
            GifRowType GifRow = ScreenBuffer[h];
            for (w = 0; w < width; w++) {
                if (TransparentColor == GifRow[w]) {
                    img_data += 3;
                    continue;
                }
                GifColorType *ColorMapEntry = &ColorMap->Colors[GifRow[w]];
                *img_data++ = ColorMapEntry->Blue;
                *img_data++ = ColorMapEntry->Green;
                *img_data++ = ColorMapEntry->Red;
            }
        }
    }

    out->data = data;
    out->size = size;
    return 0;
err:
    if (data) {
        hal_free(data);
    }
    return -1;
}
#elif (IMAGE_OUTPUT_FORMAT == IMAGE_FORMAT_RGB565)
static inline int ScreenBuffer2DgifOutput(struct dgif_out_t *out, GifRowType *ScreenBuffer, unsigned int width, unsigned int height, ColorMapObject *ColorMap, int TransparentColor, int delay)
{
    unsigned char *data = NULL;
    if (!out || !ScreenBuffer || !ColorMap || !width || !height) {
        DGIF_ERROR("%s:%u para error!\n", __func__, __LINE__);
        goto err;
    }

    unsigned int size = sizeof(unsigned int) * 4 + width * height * 2;
    data = hal_malloc(size);
    if (!data) {
        DGIF_ERROR("%s:%u no memory!\n", __func__, __LINE__);
        goto err;
    }

    unsigned int *pwidth = (unsigned int *)(data + sizeof(int) * 0);
    unsigned int *pheight = (unsigned int *)(data + sizeof(int) * 1);
    unsigned int *pfmt = (unsigned int *)(data + sizeof(int) * 2);
    unsigned int *pdelay = (unsigned int *)(data + sizeof(int) * 3);
    unsigned short *img_data = (unsigned short *)(data + sizeof(int) * 4);

    *pwidth = width;
    *pheight = height;
    *pfmt = IMAGE_FORMAT_RGB565;
    *pdelay = delay;

    unsigned int w, h;

    if (TransparentColor == -1) {
        for (h = 0; h < height; h++) {
            GifRowType GifRow = ScreenBuffer[h];
            for (w = 0; w < width; w++) {
                GifColorType *ColorMapEntry = &ColorMap->Colors[GifRow[w]];
                *img_data++ = ((ColorMapEntry->Red & 0xf8) << 8) | ((ColorMapEntry->Green & 0xfc) << 3) | (ColorMapEntry->Blue >> 3);
            }
        }
    } else {
        for (h = 0; h < height; h++) {
            GifRowType GifRow = ScreenBuffer[h];
            for (w = 0; w < width; w++) {
                if (TransparentColor == GifRow[w]) {
                    img_data++;
                    continue;
                }
                GifColorType *ColorMapEntry = &ColorMap->Colors[GifRow[w]];
                *img_data++ = ((ColorMapEntry->Red & 0xf8) << 8) | ((ColorMapEntry->Green & 0xfc) << 3) | (ColorMapEntry->Blue >> 3);
            }
        }
    }

    out->data = data;
    out->size = size;
    return 0;
err:
    if (data) {
        hal_free(data);
    }
    return -1;
}
#endif

#if defined(DECOMPRESS_GIF_SHARED_THREAD)
static void decompress_gif_handler(void *arg)
#else
static void decompress_gif_thread(void *arg)
#endif
{
    struct dgif_t *dgif = (struct dgif_t *)arg;
    GifFileType *GifFile = dgif->GifFile;
    GifRowType *ScreenBuffer = NULL;
#ifdef DECOMPRESS_GIF_PERFORMANCE_DEBUG
    unsigned long long start, end;
    start = end = xTaskGetTickCount();
#endif

    ScreenBuffer = GifCreateScreenBufferWithBackGroundColor(GifFile);
    if (!ScreenBuffer) {
        DGIF_ERROR("%s:%u no memory!\n", __func__, __LINE__);
        goto exit_loop;
    }

#ifdef DECOMPRESS_GIF_PERFORMANCE_DEBUG
    start = end;
    end = xTaskGetTickCount();
    DGIF_INFO("Create ScreenBuffer cost %fms\n", (double)(end - start) * portTICK_PERIOD_MS);
#endif

    while(dgif->run) {
        GifRecordType RecordType = UNDEFINED_RECORD_TYPE;
        if (DGifGetRecordType(GifFile, &RecordType) == GIF_ERROR) {
            DGIF_ERROR("DGifGetRecordType error!\n");
            break;
        }
        switch(RecordType) {
        case IMAGE_DESC_RECORD_TYPE: {
            SavedImage *sp = NULL;
#if 0 // DRAW_TO_SAVED_IMAGE
            size_t ImageSize;
#endif
            int delay = 0;
            int TransparentColor = -1;
            DGIF_DEBUG("IMAGE_DESC_RECORD_TYPE\n");
            if (DGifGetImageDesc(GifFile) == GIF_ERROR) {
                DGIF_ERROR("DGifGetImageDesc error!\n");
                goto desc_err;
            }
            sp = &GifFile->SavedImages[GifFile->ImageCount - 1];
            if (sp->ImageDesc.Width <= 0) {
                DGIF_ERROR("%s:%u error!\n", __func__, __LINE__);
                goto desc_err;
            }
            if (sp->ImageDesc.Height <= 0) {
                DGIF_ERROR("%s:%u error!\n", __func__, __LINE__);
                goto desc_err;
            }
            if (sp->ImageDesc.Width > (INT_MAX / sp->ImageDesc.Height)) {
                DGIF_ERROR("%s:%u error!\n", __func__, __LINE__);
                goto desc_err;
            }
#if 0 // DRAW_TO_SAVED_IMAGE
            ImageSize = sp->ImageDesc.Width * sp->ImageDesc.Height;
            if (ImageSize > (SIZE_MAX / sizeof(GifPixelType))) {
                DGIF_ERROR("%s:%u error!\n", __func__, __LINE__);
                goto desc_err;
            }
            sp->RasterBits = (unsigned char *)hal_malloc(ImageSize * sizeof(GifPixelType));
            if (!sp->RasterBits) {
                DGIF_ERROR("%s:%u error!\n", __func__, __LINE__);
                goto desc_err;
            }
#else
            // TODO check
            int Row = sp->ImageDesc.Top; /* Image Position relative to Screen. */
            int Col = sp->ImageDesc.Left;
            int Width = sp->ImageDesc.Width;
            int Height = sp->ImageDesc.Height;
#endif
            if (sp->ImageDesc.Interlace) {
                int i, j;
                static const int InterlacedOffset[] = { 0, 4, 2, 1 };
                static const int InterlacedJumps[] = { 8, 8, 4, 2 };
                for (i = 0; i < 4; i++) {
#if 0 // DRAW_TO_SAVED_IMAGE
                    for (j = InterlacedOffset[i]; j < sp->ImageDesc.Height; j += InterlacedJumps[i]) {
                        if (DGifGetLine(GifFile, sp->RasterBits + j * sp->ImageDesc.Width, sp->ImageDesc.Width) == GIF_ERROR) {
                            DGIF_ERROR("%s:%u error!\n", __func__, __LINE__);
                            goto desc_err;
                        }
                    }
#else
                    for (j = Row + InterlacedOffset[i]; j < (Row + Height); j += InterlacedJumps[i]) {
                        if (DGifGetLine(GifFile, &ScreenBuffer[j][Col], Width) == GIF_ERROR) {
                            DGIF_ERROR("%s:%u error!\n", __func__, __LINE__);
                            goto desc_err;
                        }
                    }
#endif

                }
            } else {
#if 0 // DRAW_TO_SAVED_IMAGE
                if (DGifGetLine(GifFile, sp->RasterBits, ImageSize) == GIF_ERROR) {
                    DGIF_ERROR("%s:%u error!\n", __func__, __LINE__);
                    goto desc_err;
                }
#else
                int j;
                for (j = Row; j < (Row + Height); j++) {
                    if (DGifGetLine(GifFile, &ScreenBuffer[j][Col], Width) == GIF_ERROR) {
                        DGIF_ERROR("%s:%u error!\n", __func__, __LINE__);
                        goto desc_err;
                    }
                }
#endif
                if (GifFile->ExtensionBlocks) {
                    sp->ExtensionBlocks = GifFile->ExtensionBlocks;
                    sp->ExtensionBlockCount = GifFile->ExtensionBlockCount;

                    GifFile->ExtensionBlocks = NULL;
                    GifFile->ExtensionBlockCount = 0;
                }
            }
            ColorMapObject *ColorMap = GifFile->Image.ColorMap ? GifFile->Image.ColorMap : GifFile->SColorMap;
            if (!ColorMap) {
                DGIF_ERROR("%s:%u error!\n", __func__, __LINE__);
                goto desc_err;
            }
#ifdef DECOMPRESS_GIF_PERFORMANCE_DEBUG
            start = end;
            end = xTaskGetTickCount();
#endif
            DGIF_DEBUG("[1/4] %u size [%d, %d] at (%d, %d)\n", GifFile->ImageCount, Width, Height, Col, Row);
#if 0
            DGIF_DEBUG("[2/4] %u ExtensionBlockCount: %u\n", GifFile->ImageCount, sp->ExtensionBlockCount);
            int i;
            for (i = 0; i < sp->ExtensionBlockCount; i++) {
                ExtensionBlock *ExtensionBlocks = &sp->ExtensionBlocks[i];
                switch(ExtensionBlocks->Function) {
                case CONTINUE_EXT_FUNC_CODE:    DGIF_DEBUG("\tCONTINUE_EXT_FUNC_CODE\n"); break;
                case COMMENT_EXT_FUNC_CODE:     DGIF_DEBUG("\tCOMMENT_EXT_FUNC_CODE\n"); break;
                case GRAPHICS_EXT_FUNC_CODE:    DGIF_DEBUG("\tGRAPHICS_EXT_FUNC_CODE\n"); break;
                case PLAINTEXT_EXT_FUNC_CODE:   DGIF_DEBUG("\tPLAINTEXT_EXT_FUNC_CODE\n"); break;
                case APPLICATION_EXT_FUNC_CODE: DGIF_DEBUG("\tAPPLICATION_EXT_FUNC_CODE\n"); break;
                default: break;
                }
            }
#else
            int i;
            DGIF_DEBUG("[2/4] %u", GifFile->ImageCount);
            for (i = 0; i < sp->ExtensionBlockCount; i++) {
                ExtensionBlock *ExtensionBlocks = &sp->ExtensionBlocks[i];
                if (ExtensionBlocks->Function == GRAPHICS_EXT_FUNC_CODE) {
                    GraphicsControlBlock gcb;
                    if (DGifExtensionToGCB(ExtensionBlocks->ByteCount, ExtensionBlocks->Bytes, &gcb) == GIF_ERROR) {
                        DGIF_ERROR("%s:%u error!\n", __func__, __LINE__);
                        goto desc_err;
                    }
                    DGIF_DEBUG_PRINT(", disposal mode %d", gcb.DisposalMode);
                    DGIF_DEBUG_PRINT(", user input flag %s", gcb.UserInputFlag ? "on" : "off");
                    DGIF_DEBUG_PRINT(", delay %d", gcb.DelayTime);
                    delay = gcb.DelayTime;
                    TransparentColor = gcb.TransparentColor;
                    DGIF_DEBUG_PRINT(", transparent index %d", gcb.TransparentColor);
                }
            }
            DGIF_DEBUG_PRINT("\n");
#endif
#ifdef DECOMPRESS_GIF_PERFORMANCE_DEBUG
            DGIF_INFO("[3/4] %u cost %fms\n", GifFile->ImageCount, (double)(end - start) * portTICK_PERIOD_MS);
#endif
#if 0 // DRAW_TO_SAVED_IMAGE
            // TODO SavedImage to ScreenBuffer
#endif
            struct dgif_out_t out = {NULL, 0};
#if defined(DECOMPRESS_GIF_REQ_QUEUE)
            while (0 > hal_queue_recv(dgif->req_queue, (void *)&out, 1000 * 10)) {
                DGIF_DEBUG("%s:%u error!\n", __func__, __LINE__);
            }
            if (!out.data && out.size == 0xffffffff) {
                DGIF_DEBUG("%s:%u user stop!\n", __func__, __LINE__);
                goto user_exit;
            }
#endif
#ifdef DECOMPRESS_GIF_PERFORMANCE_DEBUG
            start = end;
            end = xTaskGetTickCount();
#endif
            if (ScreenBuffer2DgifOutput(&out, ScreenBuffer, GifFile->SWidth, GifFile->SHeight, ColorMap, TransparentColor, delay)) {
                DGIF_ERROR("%s:%u error!\n", __func__, __LINE__);
                goto desc_err;
            }
#ifdef DECOMPRESS_GIF_PERFORMANCE_DEBUG
            start = end;
            end = xTaskGetTickCount();
            DGIF_INFO("[4/4] %u cost %fms\n", GifFile->ImageCount, (double)(end - start) * portTICK_PERIOD_MS);
#endif
            while (0 > hal_queue_send_wait(dgif->queue, (void *)&out, 1000 * 10)) {
                DGIF_DEBUG("%s:%u error!\n", __func__, __LINE__);
            }
#ifdef DECOMPRESS_GIF_PERFORMANCE_DEBUG
            start = end;
            end = xTaskGetTickCount();
#endif
desc_err:
#if 0 // DRAW_TO_SAVED_IMAGE
            //本来可以缓存的，但是内存实在有限，只能用完就释放了
            if (sp->RasterBits) {
                hal_free(sp->RasterBits);
                sp->RasterBits = NULL;
            }
#endif
            break;
        }
        case EXTENSION_RECORD_TYPE: {
            DGIF_DEBUG("EXTENSION_RECORD_TYPE\n");
            int ExtFunction;
            GifByteType *ExtData;
            if (DGifGetExtension(GifFile, &ExtFunction, &ExtData) == GIF_ERROR) {
                DGIF_ERROR("%s:%u error!\n", __func__, __LINE__);
                goto ext_err;
            }
            if (ExtData != NULL) {
                if (GifAddExtensionBlock(&GifFile->ExtensionBlockCount,
                    &GifFile->ExtensionBlocks,
                    ExtFunction, ExtData[0], &ExtData[1]) == GIF_ERROR) {
                    DGIF_ERROR("%s:%u error!\n", __func__, __LINE__);
                    goto ext_err;
                }
            }
            for (;;) {
                if (DGifGetExtensionNext(GifFile, &ExtData) == GIF_ERROR) {
                    DGIF_ERROR("%s:%u error!\n", __func__, __LINE__);
                    goto ext_err;
                }
                if (ExtData == NULL)
                    break;
                if (GifAddExtensionBlock(&GifFile->ExtensionBlockCount,
                    &GifFile->ExtensionBlocks,
                    CONTINUE_EXT_FUNC_CODE, 
                    ExtData[0], &ExtData[1]) == GIF_ERROR) {
                    DGIF_ERROR("%s:%u error!\n", __func__, __LINE__);
                    goto ext_err;
                }
            }
ext_err:
            break;
        }
        case TERMINATE_RECORD_TYPE: {
            DGIF_DEBUG("TERMINATE_RECORD_TYPE\n");
            goto exit_loop;
        }
        default:
            DGIF_ERROR("unknown RecordType! %u\n", RecordType);
            break;
        }
    }
exit_loop:
    {
        struct dgif_out_t out = {.data = NULL, .size = 0};
        while (0 > hal_queue_send_wait(dgif->queue, (void *)&out, 1000 * 10)) {
            DGIF_ERROR("%s:%u error!\n", __func__, __LINE__);
        }
#if defined(DECOMPRESS_GIF_REQ_QUEUE)
        do {
            while (0 > hal_queue_recv(dgif->req_queue, (void *)&out, 1000 * 10)) {
                DGIF_DEBUG("%s:%u error!\n", __func__, __LINE__);
            }
            // 至少接收一次，用于消耗掉返回空结果的请求队列，后续接收用于等待destroy的返回命令
        } while (dgif->run);
#endif
    }
    // 用户触发的退出，无需等待destroy的返回命令
user_exit:
    if (ScreenBuffer)
        GifDestroyScreenBuffer(ScreenBuffer, GifFile);

#if defined(DECOMPRESS_GIF_SHARED_THREAD)
    dgif->thread_alive = 0;
#else
    dgif->thread = NULL;
    hal_thread_stop(NULL);
#endif
}

void decompress_gif_destroy(void *_dgif)
{
    struct dgif_t *dgif = (struct dgif_t *)_dgif;
    if (!dgif)
        return;

    dgif->run = 0;
#if defined(DECOMPRESS_GIF_REQ_QUEUE)
    // 如果解码线程阻塞在等待请求的位置，需要发送一个结束的请求来恢复线程，并命令其返回
    struct dgif_out_t out = {NULL, 0xffffffff};
    while (0 > hal_queue_send_wait(dgif->req_queue, (void *)&out, 1000 * 10)) {
        DGIF_DEBUG("%s:%u error!\n", __func__, __LINE__);
    }
#endif

#if defined(DECOMPRESS_GIF_SHARED_THREAD)
    while (dgif->thread_alive) {
        hal_msleep(10);
    }
#else
    while (dgif->thread) {
        hal_msleep(10);
    }
#endif

#if defined(DECOMPRESS_GIF_REQ_QUEUE)
    if (dgif->req_queue) {
        if (!hal_is_queue_empty(dgif->req_queue)) {
            DGIF_WARN("%s:%u req_queue warning!\n", __func__, __LINE__);
            while (0 > hal_queue_recv(dgif->req_queue, (void *)&out, 1000 * 10)) {
                DGIF_DEBUG("%s:%u error!\n", __func__, __LINE__);
            }
        }
        hal_queue_delete(dgif->req_queue);
        dgif->req_queue = NULL;
    }
    if (dgif->queue) {
        if (!hal_is_queue_empty(dgif->queue)) {
            DGIF_WARN("%s:%u queue warning!\n", __func__, __LINE__);
        }
        hal_queue_delete(dgif->queue);
        dgif->queue = NULL;
    }
#else
    if (dgif->queue) {
        while(!hal_is_queue_empty(dgif->queue)) {
            void *data = NULL;
            decompress_gif_get(dgif, &data, NULL);
            if (data)
                hal_free(data);
        }
        hal_queue_delete(dgif->queue);
        dgif->queue = NULL;
    }
#endif

    if (dgif->GifFile) {
        int Error;
        GifFileType *GifFile = dgif->GifFile;
        dgif->GifFile = NULL;

        if (GifFile->ImageCount == 0) {
            GifFile->Error = D_GIF_ERR_NO_IMAG_DSCR;
            DGIF_ERROR("%s:%u error!\n", __func__, __LINE__);
        }

        if (DGifCloseFile(GifFile, &Error) == GIF_ERROR) {
            DGIF_ERROR("DGifCloseFile failed!\n");
            PrintGifError(Error);
        }
    }

#if defined(DECOMPRESS_GIF_KEEP_INPUT_BUFFER)
    dgif->buf = NULL;
#else
    if (dgif->buf) {
        hal_free(dgif->buf);
        dgif->buf = NULL;
    }
#endif

    hal_free(dgif);
}

void *decompress_gif_create(void *src, unsigned int src_size)
{
    int Error;
    if (!src || !src_size)
        goto err;

    struct dgif_t *dgif = hal_malloc(sizeof(*dgif));
    if (!dgif)
        goto err;

    dgif->finish = 0;
    dgif->queue = hal_queue_create("dgif_queue", sizeof(struct dgif_out_t), 1);
    if (!dgif->queue)
        goto err;

#if defined(DECOMPRESS_GIF_REQ_QUEUE)
    dgif->req_queue = hal_queue_create("dgif_req_queue", sizeof(struct dgif_out_t), 1);
    if (!dgif->req_queue)
        goto err;
#endif

    dgif->buf = src;
    dgif->size = src_size;
    dgif->pos = 0;
    dgif->GifFile = DGifOpen(dgif, decompress_gif_input_fun, &Error);
    if (!dgif->GifFile)
        goto err;

    dgif->run = 1;
#if defined(DECOMPRESS_GIF_SHARED_THREAD)
    if (run_thread(decompress_gif_handler, dgif))
        goto err;
    dgif->thread_alive = 1;
#else
    dgif->thread = hal_thread_create(decompress_gif_thread, dgif, "dgif_thread", 4096, 1);
    if (!dgif->thread)
        goto err;
#endif

    return dgif;
err:
    if (dgif)
        dgif->buf = NULL;
    decompress_gif_destroy(dgif);
    return NULL;
}

int decompress_gif_get(void *_dgif, void **pout, unsigned int *pout_size)
{
    struct dgif_t *dgif = (struct dgif_t *)_dgif;
    struct dgif_out_t out = {.data = NULL, .size = 0,};

#if defined(DECOMPRESS_GIF_REQ_QUEUE)
    if (pout)
        out.data = *pout;
    if (pout_size)
        out.size = *pout_size;

    while (1) {
        // run = 1 说明已经进入destroy，不应该再发起请求
        if(!dgif->run) {
            DGIF_ERROR("decompress thread will exit!\n");
            return -1;
        }
        if(0 > hal_queue_send_wait(dgif->req_queue, (void *)&out, 1000 * 10))
            DGIF_DEBUG("%s:%u error!\n", __func__, __LINE__);
        else
            break;
    }
#endif

    // finish = 1 说明已经解码线程已经退出，不会再接收到回应
    while (!dgif->finish) {
        if(0 > hal_queue_recv(dgif->queue, (void *)&out, 1000 * 10))
            DGIF_DEBUG("%s:%u error!\n", __func__, __LINE__);
        else
            break;
    }

    if (!out.data || !out.size) {
        DGIF_DEBUG("%s:%u empty!\n", __func__, __LINE__);
        dgif->finish = 1;
        return -1;
    }

    if (pout)
        *pout = out.data;

    if (pout_size)
        *pout_size = out.size;

    return 0;
}

static inline int get_file_size(const char *path)
{
#if !defined(CONFIG_ARCH_DSP)
    struct stat fileinfo;
    if (!stat(path, &fileinfo)) {
        return fileinfo.st_size;
    }
#endif
    return -1;
}

static inline int read_file_to_buffer(const char *path, void **pfile_out, unsigned int *pfile_size)
{
    int ret = -1;
    FILE *fd = NULL;
    int file_size;
    unsigned char *data = NULL;

    file_size = get_file_size(path);
    if (file_size < 0) {
        DGIF_ERROR("get_file_size %s failed!\n", path);
        goto exit;
    }

    data = hal_malloc(file_size);
    if (!data) {
        DGIF_ERROR("%s:%u no memory!\n", __func__, __LINE__);
        goto exit;
    }

    fd = fopen(path, "rb");
    if (NULL == fd) {
        DGIF_ERROR("open %s failed!\n", path);
        goto exit;
    }

    ret = fread(data, 1, file_size, fd);
    if (ret != file_size) {
        DGIF_ERROR("read %s failed!\n", path);
        goto exit;
    }

    *pfile_size = file_size;
    *pfile_out = data;
    data = NULL;
    ret = 0;
exit:
    if (fd) {
        fclose(fd);
    }
    if (data)
        hal_free(data);
    return ret;
}

static int run = 0;
int cmd_dgif_test(int argc, char **argv)
{
    int cnt = 0;
    int ret = -1;
    void *dgif = NULL;
    const char *path = "/data/test.gif";
    void* file_out = NULL;
    unsigned int file_size;

    if (argc >= 2) {
        path = argv[1];
    }

    if (read_file_to_buffer(path, &file_out, &file_size)) {
        DGIF_ERROR("read_file_to_buffer failed!\n");
        goto exit;
    }

    dgif = decompress_gif_create(file_out, file_size);
    if (!dgif) {
        goto exit;
    }
#if defined(DECOMPRESS_GIF_KEEP_INPUT_BUFFER)
#else
    file_out = NULL;
#endif

    run = 1;
    unsigned long long start, end;
    start = xTaskGetTickCount();
    while(run) {
        void *img_out = NULL;
        unsigned int img_size = 0;
        if (decompress_gif_get(dgif, &img_out, &img_size)) {
            break;
        }
        if (img_out) {
#if 0
            struct gif_image_hdr *hdr = img_out;
            unsigned long long image = (unsigned long long)(sizeof(struct gif_image_hdr) + (unsigned char *)img_out);
            int show_image_argb8888(unsigned long long addr, unsigned int width, unsigned int height);
            int show_image_rgb888(unsigned long long addr, unsigned int width, unsigned int height);
            int show_image_rgb565(unsigned long long addr, unsigned int width, unsigned int height);
            if (hdr->fmt == IMAGE_FORMAT_RGB565)
                show_image_rgb565(image, hdr->width, hdr->height);
            else if (hdr->fmt == IMAGE_FORMAT_RGB888)
                show_image_rgb888(image, hdr->width, hdr->height);
            else if (hdr->fmt == IMAGE_FORMAT_ARGB8888)
                show_image_argb8888(image, hdr->width, hdr->height);
            else
                DGIF_ERR("format error!\n");
#endif
            cnt++;
            DGIF_INFO("%s %d recv %p %u\n", __func__, cnt, img_out, img_size);
            hal_free(img_out);
            img_out = NULL;
            img_size = 0;
        }
    }
    end = xTaskGetTickCount();
    DGIF_INFO("%u images cost %fms\n", cnt, (double)(end - start) * portTICK_PERIOD_MS);
    if (cnt)
        DGIF_INFO("cost %fms per image\n", (double)(end - start) * portTICK_PERIOD_MS / cnt);


    ret = 0;
exit:
    hal_msleep(1);
    decompress_gif_destroy(dgif);
    if (file_out) {
        hal_free(file_out);
    }
    DGIF_INFO("%s exit\n", __func__);
    return ret;
}

FINSH_FUNCTION_EXPORT_CMD(cmd_dgif_test, dgif_test, dgif_test);

int cmd_dgif_test_stop(int argc, char **argv)
{
    run = 0;
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_dgif_test_stop, dgif_test_stop, dgif_test_stop);

int dgif_from_buffer(void *buffer, unsigned int size)
{
    int cnt = 0;
    int ret = -1;
    void *dgif = NULL;

    dgif = decompress_gif_create(buffer, size);
    if (!dgif) {
        goto exit;
    }
    unsigned long long start, end;
    start = xTaskGetTickCount();
    while(1) {
        void *img_out = NULL;
        unsigned int img_size = 0;;
        if (decompress_gif_get(dgif, &img_out, &img_size)) {
            break;
        }
        if (img_out) {
            cnt++;
            DGIF_INFO("%s %d recv %p %u\n", __func__, cnt, img_out, img_size);
            hal_free(img_out);
            img_out = NULL;
            img_size = 0;
        }
    }
    end = xTaskGetTickCount();
    DGIF_INFO("%u images cost %fms\n", cnt, (double)(end - start) * portTICK_PERIOD_MS);
    if (cnt)
        DGIF_INFO("cost %fms per image\n", (double)(end - start) * portTICK_PERIOD_MS / cnt);

    ret = 0;
exit:
    hal_msleep(1);
    decompress_gif_destroy(dgif);
    DGIF_INFO("%s exit\n", __func__);
    return ret;
}
