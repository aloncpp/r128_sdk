#ifndef __DECOMPRESS_GIF_H__
#define __DECOMPRESS_GIF_H__

struct gif_image_hdr {
    unsigned int width;
    unsigned int height;
    unsigned int fmt;
    unsigned int delay;
};

#ifndef IMAGE_FORMAT_RGB565
#define IMAGE_FORMAT_RGB565     (2)
#endif

#ifndef IMAGE_FORMAT_RGB888
#define IMAGE_FORMAT_RGB888     (3)
#endif

#ifndef IMAGE_FORMAT_ARGB8888
#define IMAGE_FORMAT_ARGB8888   (4)
#endif

/* input layout: |gif file buffer n byte| */
void *decompress_gif_create(void *src, unsigned int src_size);

/* output layout: |header 16 byte|image buffer (width * height * fmt_byte) byte| */
int decompress_gif_get(void *_dgif, void **pout, unsigned int *pout_size);

void decompress_gif_destroy(void *_dgif);

#endif
