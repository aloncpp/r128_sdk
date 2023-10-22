#ifndef CAMERA_PREVIEW_H
#define CAMERA_PREVIEW_H

#include <hal_cache.h>

#define CAMERA_PREVIEW_DBG_EN   0
#if (CAMERA_PREVIEW_DBG_EN == 1)
#define cp_dbg(x, arg...) printf("[camera_preview_debug]"x, ##arg)
#else
#define cp_dbg(x, arg...)
#endif

typedef enum {
	CP_FMT_JPEG,
	CP_FMT_RGB565,
	CP_FMT_RGB888,
	CP_FMT_ARGB8888,
	CP_FMT_NV12,
	CP_FMT_YUYV,
} camera_preview_fmt;

struct camera_preview_buf {
	void *addr;
	unsigned long size;
	unsigned int width;
	unsigned int height;
	camera_preview_fmt fmt;
};

#endif  /*CAMERA_PREVIEW_H*/
