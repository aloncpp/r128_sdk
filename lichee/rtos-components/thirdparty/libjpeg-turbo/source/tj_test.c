#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <turbojpeg.h>
#include "console.h"
typedef unsigned char uchar;

typedef struct tjp_info {
  int outwidth;
  int outheight;
  unsigned long jpg_size;
}tjp_info_t;

// RGB与BGR格式文件转换
void reverseRGB(unsigned char* data, int size)
{
    unsigned char *p = data;
    int i;

    for (i = 0; i < size / 3 ; i++) {
        unsigned char tmp = p[0];
        p[0] = p[2];
        p[2] = tmp;
        p += 3;
    }
}

/*获取当前ms数*/
#if defined(CONFIG_ARCH_DSP)
#include <FreeRTOS.h>
#include <task.h>
static int get_timer_now ()
{
  return (int)xTaskGetTickCount();
}
#else
static int get_timer_now ()
{
    struct timeval now;
    gettimeofday(&now, NULL);
    return(now.tv_sec * 1000 + now.tv_usec / 1000);
}
#endif

/*读取文件到内存*/
uchar *read_file2buffer(char *filepath, tjp_info_t *tinfo)
{
  FILE *fd;
  struct stat fileinfo;
  stat(filepath,&fileinfo);
  tinfo->jpg_size = fileinfo.st_size;

  fd = fopen(filepath,"rb");
  if (NULL == fd) {
    printf("file not open\n");
    return NULL;
  }
  uchar *data = (uchar *)malloc(sizeof(uchar) * fileinfo.st_size);
  fread(data,1,fileinfo.st_size,fd);
  fclose(fd);
  return data;
}

/*写内存到文件*/
void write_buffer2file(char *filename, uchar *buffer, int size)
{
  FILE *fd = fopen(filename,"wb");
  if (NULL == fd) {
    return;
  }
  fwrite(buffer,1,size,fd);
  fclose(fd);
}

/*图片解压缩*/
uchar *tjpeg_decompress(uchar *jpg_buffer, tjp_info_t *tinfo)
{
  tjhandle handle = NULL;
  int img_width,img_height,img_subsamp,img_colorspace;
  int flags = 0, pixelfmt = TJPF_RGB;
  /*创建一个turbojpeg句柄*/
  handle = tjInitDecompress();
  if (NULL == handle)  {
    return NULL;
  }
  /*获取jpg图片相关信息但并不解压缩*/
  int ret = tjDecompressHeader3(handle,jpg_buffer,tinfo->jpg_size,&img_width,&img_height,&img_subsamp,&img_colorspace);
  if (0 != ret) {
    tjDestroy(handle);
    return NULL;
  }
  /*输出图片信息*/
  printf("jpeg width:%d\n",img_width);
  printf("jpeg height:%d\n",img_height);
  printf("jpeg subsamp:%d\n",img_subsamp);
  printf("jpeg colorspace:%d\n",img_colorspace);
  /*计算1/4缩放后的图像大小,若不缩放，那么直接将上面的尺寸赋值给输出尺寸*/
  tjscalingfactor sf;
  sf.num = 1;
  sf.denom = 1;
  tinfo->outwidth = TJSCALED(img_width, sf);
  tinfo->outheight = TJSCALED(img_height, sf);
  printf("w:%d,h:%d\n",tinfo->outwidth,tinfo->outheight);
  /*解压缩时，tjDecompress2（）会自动根据设置的大小进行缩放，但是设置的大小要在它的支持范围，如1/2 1/4等*/
  flags |= 0;
  int size = tinfo->outwidth * tinfo->outheight * 3;
  uchar *rgb_buffer = (uchar *)malloc(sizeof(uchar) * size);
  ret = tjDecompress2(handle, jpg_buffer, tinfo->jpg_size, rgb_buffer, tinfo->outwidth, 0,tinfo->outheight, pixelfmt, flags);
  reverseRGB(rgb_buffer, size);//大端转化为小端
  char *outfile1 = "./data/tjtest.rgb";
  write_buffer2file(outfile1, rgb_buffer, size);
  reverseRGB(rgb_buffer, size);//小端转化回大端进行压缩。
  if (0 != ret) {
    tjDestroy(handle);
    return NULL;
  }
  tjDestroy(handle);
  return rgb_buffer;
}

/*压缩图片*/
int tjpeg_compress(uchar *rgb_buffer, tjp_info_t *tinfo, int quality, uchar **outjpg_buffer, unsigned long *outjpg_size)
{
  tjhandle handle = NULL;
  int flags = 0;
  int subsamp = TJSAMP_422;
  int pixelfmt = TJPF_RGB;
  /*创建一个turbojpeg句柄*/
  handle=tjInitCompress();
  if (NULL == handle) {
    return -1;
  }
  /*将rgb图或灰度图等压缩成jpeg格式图片*/
  int ret = tjCompress2(handle, rgb_buffer,tinfo->outwidth,0,tinfo->outheight,pixelfmt,outjpg_buffer,outjpg_size,subsamp,quality, flags);
  if (0 != ret) {
    tjDestroy(handle);
    return -1;
  }
  tjDestroy(handle);
  return 0;
}

/*测试程序*/
int tj_test()
{
  tjp_info_t tinfo;
    char *filename = "./data/test.jpg";
    int start = get_timer_now();
  /*读图像*/
    uchar *jpeg_buffer = read_file2buffer(filename,&tinfo);
    int rend = get_timer_now();
    printf("loadfile make time:%d\n",rend-start);
    if (NULL == jpeg_buffer) {
        printf("read file failed\n");
        return 1;
    }

    int dstart = get_timer_now();
  /*解压缩*/
    uchar *rgb = tjpeg_decompress(jpeg_buffer,&tinfo);
	if (NULL == rgb) {
        printf("error\n");
    free(jpeg_buffer);
    return -1;
    }
    int dend = get_timer_now();
    printf("decompress make time:%d\n",dend-dstart);
    uchar *outjpeg=NULL;
    unsigned long outjpegsize;
    int cstart = get_timer_now();
  /*压缩*/
    tjpeg_compress(rgb,&tinfo,80,&outjpeg,&outjpegsize);
    printf("out jpeg size = %lu\n",outjpegsize);
    int cend = get_timer_now();
    printf("compress make time:%d\n",cend-cstart);
    char *outfile = "./data/tjtest.jpg";
    int wstart = get_timer_now();
    write_buffer2file(outfile,outjpeg,outjpegsize);
    int wend = get_timer_now();
    printf("write file make time:%d\n",wend-wstart);
    int end = get_timer_now();
    printf("tj all make time:%d\n",end-start);
  free(jpeg_buffer);
  free(rgb);
  return 0;
}

int tj_test_main(int argc, char **argv)
{
    return tj_test();
}

FINSH_FUNCTION_EXPORT_CMD(tj_test_main, tjtest, tjtest);
