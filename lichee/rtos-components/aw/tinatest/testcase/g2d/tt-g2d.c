#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tinatest.h>
#include <hal_mem.h>
#include <hal_cache.h>
#include <hal_time.h>
#include "g2d_driver.h"

#define IMG_SIZE1 800*480
#define X1 800
#define Y1 480
#define IMG_SIZE2 480*320
#define X2 480
#define Y2 320
#define IMG_SIZE3 320*240
#define X3 320
#define Y3 240

#define G2D_FILE_PATH "/data/tt_g2d/"

extern int sunxi_g2d_control(int cmd, void *arg);
extern int sunxi_g2d_close(void);
extern int sunxi_g2d_open(void);

static int tt_g2d_fill_test(void)
{
	int ret;
	void *buf1 = NULL;
	char *temp;

	g2d_fillrect_h info;
	unsigned long i;

	printf("hello g2d fill test\n");
	ret = sunxi_g2d_open();
	if (ret) {
		printf("g2d open fail\n");
		return -1;
	}
	buf1 = hal_malloc_coherent(IMG_SIZE1 * 4);
	memset(&info, 0, sizeof(g2d_fillrect_h));
	memset((void *)buf1, 0, IMG_SIZE1 * 4);

	info.dst_image_h.format = G2D_FORMAT_ARGB8888;
	info.dst_image_h.width = X1;
	info.dst_image_h.height = Y1;
	info.dst_image_h.clip_rect.x = 0;
	info.dst_image_h.clip_rect.y = 0;
	info.dst_image_h.clip_rect.w = X1;
	info.dst_image_h.clip_rect.h = Y1;
	info.dst_image_h.color = 0xff0000ff;
	info.dst_image_h.mode = 1;
	info.dst_image_h.alpha = 255;
	info.dst_image_h.laddr[0] = __va_to_pa((uint32_t)buf1);//phy_addr;
	info.dst_image_h.use_phy_addr = 1;

	printf("dst: addr=0x%lx, 0x%lx, 0x%lx, format=0x%x, img w=%ld, h=%ld, rect x=%ld, y=%ld, w=%ld, h=%ld, align=%ld\n\n",
			info.dst_image_h.laddr[0], info.dst_image_h.laddr[1], info.dst_image_h.laddr[2],
			info.dst_image_h.format, info.dst_image_h.width, info.dst_image_h.height,
			info.dst_image_h.clip_rect.x, info.dst_image_h.clip_rect.y,
			info.dst_image_h.clip_rect.w, info.dst_image_h.clip_rect.h,
			info.dst_image_h.align[0]);

	hal_dcache_clean((unsigned long)buf1, IMG_SIZE1 * 4);
	printf("start control\n");

	ret = sunxi_g2d_control(G2D_CMD_FILLRECT_H, &info);
	if (ret){
		printf("g2d G2D_CMD_FILLRECT_H fail\n");
		sunxi_g2d_close();
		hal_free_coherent(buf1);
		return -1;
		}
	else {
		printf("G2D_CMD_FILLRECT_H ok\n");
	}

	temp = (char *)buf1 + IMG_SIZE1 * 4 - 4;

	printf("------dump G2D_CMD_FILLRECT_H output --------\n");
	i = 1280;//only dump the last 1280 byte

	while(i--)
	{
		printf("%x ", *temp);
		temp -=1;
		if(i%32==0)
			printf("\n");
	}

	printf("\ng2d fill test finished\n\n");

out:
	hal_free_coherent(buf1);
	sunxi_g2d_close();
	return ret;

}

static int tt_g2d_rotate_test(void)
{
	int ret;
	void *buf1 = NULL, *buf2 = NULL;
	FILE* file1;
	g2d_blt_h blit_para;
	char name[128];

	printf("hello g2d rotate test\n");
	ret = sunxi_g2d_open();
	if (ret) {
		printf("g2d open fail\n");
		return -1;
	}
	buf1 = hal_malloc_coherent(IMG_SIZE2 * 4);
	buf2 = hal_malloc_coherent(IMG_SIZE2 * 4);

	memset(&blit_para, 0, sizeof(blit_para));
	memset((void *)buf1, 0, IMG_SIZE2 * 4);
	memset((void *)buf2, 0, IMG_SIZE2 * 4);
	blit_para.src_image_h.laddr[0] = __va_to_pa((uint32_t)buf1);//phy_addr1;
	blit_para.dst_image_h.laddr[0] = __va_to_pa((uint32_t)buf2);//phy_addr2;


	blit_para.src_image_h.laddr[0] = (int)(blit_para.src_image_h.laddr[0] );
	blit_para.src_image_h.use_phy_addr = 1;

	blit_para.dst_image_h.laddr[0] = (int)(blit_para.dst_image_h.laddr[0] );
	blit_para.dst_image_h.use_phy_addr = 1;


	blit_para.src_image_h.clip_rect.x = 0;
	blit_para.src_image_h.clip_rect.y = 0;
	blit_para.src_image_h.clip_rect.w = X2;
	blit_para.src_image_h.clip_rect.h = Y2;

	blit_para.dst_image_h.clip_rect.x = 0;
	blit_para.dst_image_h.clip_rect.y = 0;
	blit_para.dst_image_h.clip_rect.w = Y2;
	blit_para.dst_image_h.clip_rect.h = X2;


	blit_para.src_image_h.format = G2D_FORMAT_BGRA8888;
	blit_para.src_image_h.width = X2;
	blit_para.src_image_h.height = Y2;

	blit_para.dst_image_h.format = G2D_FORMAT_BGRA8888;
	blit_para.dst_image_h.width = Y2;
	blit_para.dst_image_h.height = X2;

	blit_para.flag_h = G2D_ROT_90;

	printf("start open file\n");
	sprintf(name, "/%s/bike_480x320_150.bin", G2D_FILE_PATH);
	file1 = fopen(name, "rb+");
	if (file1 == NULL) {
		printf("err in fopen\n");
		ret = -1;
		goto out;
	}
	ret = fread((void*)buf1, IMG_SIZE2 * 4, 1, file1);
	printf("fread,ret=%d\n", ret);
	fclose(file1);

	hal_dcache_clean((unsigned long)buf1, IMG_SIZE2 * 4);
	hal_dcache_clean((unsigned long)buf2, IMG_SIZE2 * 4);

	printf("start control\n");

	ret = sunxi_g2d_control(G2D_CMD_BITBLT_H, &blit_para);
	if (ret) {
		printf("g2d G2D_CMD_BITBLT_H fail\n");
		ret = -1;
		goto out;
	}
	else {
		printf("G2D_CMD_BITBLT_H ok\n");
	}

	hal_dcache_invalidate((unsigned long)buf2, IMG_SIZE2 * 4);

	sprintf(name, "/%s/g2d_rotate_out.bin", G2D_FILE_PATH);
	file1 = fopen(name, "wb+");
	if (file1 == NULL)
		printf("err in fopen");

	ret = fwrite((void *)buf2, IMG_SIZE2 * 4, 1, file1);
	printf("fwrite ,ret=%d\n", ret);
	printf("g2d_rotate_out.bin : 320x480, BGRA\n");
	fclose(file1);

	printf("\ng2d rotate test finished\n\n");
	ret = 0;

out:
	hal_free_coherent(buf1);
	hal_free_coherent(buf2);
	sunxi_g2d_close();
	return ret;

}


static int tt_g2d_blend_test(void)
{
	int ret, i;
	void *src = NULL, *src2 = NULL, *dst = NULL;
	FILE* file[3];
	g2d_bld info;
	unsigned int src_w, src_h, src2_w, src2_h, dst_w, dst_h;
	char name[128];

	printf("hello g2d blending test\n");

	sunxi_g2d_open();
	//note:the tests follow need three  input image file
	src = hal_malloc_coherent(IMG_SIZE2 * 4);
	src2 = hal_malloc_coherent(IMG_SIZE1 * 4);
	dst = hal_malloc_coherent(IMG_SIZE1 * 4);

	if(src == NULL || src2 == NULL || dst == NULL) {
		printf("fatal error, buf is null.\n");
		ret = -1;
		sunxi_g2d_close();
		return ret;
	}

	memset(src, 0, IMG_SIZE2 * 4);
	memset(src2, 0, IMG_SIZE1 * 4);
	memset(dst, 0, IMG_SIZE1 * 4);
	memset(&info, 0, sizeof(g2d_bld));

	// src < src2 = dst. src RGB dst RGB, coor not zero
	src_w = X2;
	src_h = Y2;
	src2_w = X1;
	src2_h = Y1;
	dst_w = X1;
	dst_h = Y1;
	info.src_image[0].laddr[0] = __va_to_pa((uint32_t)src);
	info.src_image[1].laddr[0] = __va_to_pa((uint32_t)src2);
	info.dst_image.laddr[0] = __va_to_pa((uint32_t)dst);

	info.src_image[0].laddr[0] = (unsigned int)(info.src_image[0].laddr[0]);
	info.src_image[0].use_phy_addr = 1;

	info.src_image[1].laddr[0] = (unsigned int)(info.src_image[1].laddr[0]);
	info.src_image[1].use_phy_addr = 1;

	info.dst_image.laddr[0] = (unsigned int)(info.dst_image.laddr[0]);
	info.dst_image.laddr[1] = 0;
	info.dst_image.laddr[2] = 0;
	info.dst_image.use_phy_addr = 1;

	info.bld_cmd = G2D_BLD_SRCOVER;

	info.src_image[0].format = G2D_FORMAT_BGRA8888;
	info.src_image[0].mode = G2D_GLOBAL_ALPHA;
	info.src_image[0].width = src_w;
	info.src_image[0].height = src_h;
	info.src_image[0].clip_rect.x = 0;
	info.src_image[0].clip_rect.y = 0;
	info.src_image[0].clip_rect.w = src_w;
	info.src_image[0].clip_rect.h = src_h;
	info.src_image[0].coor.x = 100;
	info.src_image[0].coor.y = 100;
	info.src_image[0].alpha = 0xd0;

	info.src_image[1].alpha = 0x50;
	info.src_image[1].format = G2D_FORMAT_ARGB8888;
	info.src_image[1].mode = G2D_GLOBAL_ALPHA;
	info.src_image[1].width = src2_w;
	info.src_image[1].height = src2_h;
	info.src_image[1].clip_rect.x = 0;
	info.src_image[1].clip_rect.y = 0;
	info.src_image[1].clip_rect.w = src2_w;
	info.src_image[1].clip_rect.h = src2_h;
	info.src_image[1].coor.x = 0;
	info.src_image[1].coor.y = 0;

	info.dst_image.format = G2D_FORMAT_ARGB8888;
	info.dst_image.mode = G2D_GLOBAL_ALPHA;
	info.dst_image.alpha = 0xff;
	info.dst_image.width = dst_w;
	info.dst_image.height = dst_h;
	info.dst_image.clip_rect.x = 0;
	info.dst_image.clip_rect.y = 0;
	info.dst_image.clip_rect.w = dst_w;
	info.dst_image.clip_rect.h = dst_h;


	printf("start open file\n");
	sprintf(name, "/%s/bike_480x320_150.bin", G2D_FILE_PATH);
	file[0] = fopen(name, "rb+");
	if (file[0] == NULL) {
		printf("err in fopen src file");
		ret = -1;
		goto out;
	}

	ret = fread((void*)src, IMG_SIZE2 * 4, 1, file[0]);

	printf("fread,ret=%d\n", ret);

	fclose(file[0]);

	sprintf(name, "/%s/src_800x480_rgb.bin", G2D_FILE_PATH);
	file[1] = fopen(name, "rb+");
	if (file[1] == NULL) {
		printf("err in fopen src2 file");
		ret = -1;
		goto out;
	}

	ret = fread((void*)src2, IMG_SIZE1 * 4, 1, file[1]);

	printf("fread,ret=%d\n", ret);

	fclose(file[1]);

	sprintf(name, "/%s/g2d_blend_out.bin", G2D_FILE_PATH);
	file[2] = fopen(name, "wb+");
	if (file[2] == NULL)
		printf("err in fopen dst file\n");

	printf("start control G2D_CMD_BLD_H \n");

	//we use hal_malloc_coherent for iamge input,so we need to make sure caches flushed
	hal_dcache_clean((unsigned long)src, IMG_SIZE2 * 4);
	hal_dcache_clean((unsigned long)src2, IMG_SIZE1 * 4);
	hal_dcache_clean((unsigned long)dst, IMG_SIZE1 * 4);

	printf("start control\n");

	ret = sunxi_g2d_control(G2D_CMD_BLD_H, &info);
	if (ret) {
		printf("g2d G2D_CMD_BLD_H fail\n");
		ret = -1;
		goto out;
	} else {
		printf("G2D_CMD_BLD_H ok\n");
	}

	printf("output image to file, may use little time\n");

	ret = fwrite(dst, IMG_SIZE1 * 4, 1, file[2]);

	printf("fwrite ,ret=%d\n",ret);
	printf("g2d_blend_out.bin : 800x480, ARGB\n");

	fclose(file[2]);

	printf("\ng2d blend test finished!\n\n");
	ret = 0;

out:
	hal_free_coherent(src);
	hal_free_coherent(src2);
	hal_free_coherent(dst);
	sunxi_g2d_close();
	return ret;

}

static int tt_g2d_mixer_test(void)
{
	int ret;
	unsigned long arg[6] = {0};
	void *buf1 = NULL,*buf2 = NULL,*buf3 = NULL,*buf4 = NULL;
	char *temp;
	FILE* file1;
	int fb_width, fb_height;
	struct mixer_para info2[2];
	unsigned long i;
	char name[128];

	printf("hello g2d mixer_and_scale test\n");

	ret = sunxi_g2d_open();
	if (ret){
		printf("g2d open fail\n");
		return -1;
	}

	memset(&info2, 0, 2*sizeof(struct mixer_para));

	//note:the tests follow need three  input image file
	buf1 = hal_malloc_coherent(IMG_SIZE3*3/2);
	buf2 = hal_malloc_coherent(IMG_SIZE3*3/2*4);
	buf3 = hal_malloc_coherent(IMG_SIZE2*3/2);
	buf4 = hal_malloc_coherent(IMG_SIZE2*3/2/4);


	if(buf1 == NULL || buf2 == NULL || buf3 == NULL || buf4 == NULL)
	{
		printf("fatal error, buf is null.\n");
		ret = -1;
		sunxi_g2d_close();
		return ret;
	}

	memset(buf1, 0, IMG_SIZE3*3/2);
	memset(buf2, 0, IMG_SIZE3*3/2*4);
	memset(buf3, 0, IMG_SIZE2*3/2);
	memset(buf4, 0, IMG_SIZE2*3/2/4);

	// use G2D_CMD_MIXER_TASK,task 1:scale G2D_FORMAT_YUV420UVC_U1V1U0V0 from 480*320 to (480*2)*(320*2)
	info2[0].src_image_h.laddr[0] = __va_to_pa((uint32_t)buf1);
	info2[0].dst_image_h.laddr[0] = __va_to_pa((uint32_t)buf2);

	info2[0].src_image_h.laddr[0] = (int)(info2[0].src_image_h.laddr[0] );
	info2[0].src_image_h.laddr[1] = (int)(info2[0].src_image_h.laddr[0] + IMG_SIZE3);
	info2[0].src_image_h.laddr[2] = (int)(info2[0].src_image_h.laddr[0] + IMG_SIZE3*5/4);
	info2[0].src_image_h.use_phy_addr = 1;

	info2[0].dst_image_h.laddr[0] = (int)(info2[0].dst_image_h.laddr[0] );
	info2[0].dst_image_h.laddr[1] = (int)(info2[0].dst_image_h.laddr[0] + IMG_SIZE3*4);
	info2[0].dst_image_h.laddr[2] = (int)(info2[0].dst_image_h.laddr[0] + IMG_SIZE3*4*5/4);
	info2[0].dst_image_h.use_phy_addr = 1;

	info2[0].flag_h = G2D_BLT_NONE_H;
	info2[0].op_flag = OP_BITBLT;
	info2[0].src_image_h.format = G2D_FORMAT_YUV420_PLANAR;
	info2[0].src_image_h.width = X3;
	info2[0].src_image_h.height = Y3;
	info2[0].src_image_h.clip_rect.x = 0;
	info2[0].src_image_h.clip_rect.y = 0;
	info2[0].src_image_h.clip_rect.w = X3;
	info2[0].src_image_h.clip_rect.h = Y3;

	info2[0].dst_image_h.format = G2D_FORMAT_YUV420_PLANAR;
	info2[0].dst_image_h.width = X3*2;
	info2[0].dst_image_h.height = Y3*2;
	info2[0].dst_image_h.clip_rect.x = 0;
	info2[0].dst_image_h.clip_rect.y = 0;
	info2[0].dst_image_h.clip_rect.w = X3*2;
	info2[0].dst_image_h.clip_rect.h = Y3*2;

	// use G2D_CMD_MIXER_TASK,task 2:scale G2D_FORMAT_YUV420_PLANAR from 320*240 to (320/2)*(240/2)
	info2[1].src_image_h.laddr[0] = __va_to_pa((uint32_t)buf3);
	info2[1].dst_image_h.laddr[0] = __va_to_pa((uint32_t)buf4);

	info2[1].src_image_h.laddr[0] = (int)(info2[1].src_image_h.laddr[0] );
	info2[1].src_image_h.laddr[1] = (int)(info2[1].src_image_h.laddr[0] + IMG_SIZE2);
	info2[1].src_image_h.laddr[2] = (int)(info2[1].src_image_h.laddr[0] + IMG_SIZE2*5/4);
	info2[1].src_image_h.use_phy_addr = 1;

	info2[1].dst_image_h.laddr[0] = (int)(info2[1].dst_image_h.laddr[0] );
	info2[1].dst_image_h.laddr[1] = (int)(info2[1].dst_image_h.laddr[0] + IMG_SIZE2/4);
	info2[1].dst_image_h.laddr[2] = (int)(info2[1].dst_image_h.laddr[0] + IMG_SIZE2/4*5/4);
	info2[1].dst_image_h.use_phy_addr = 1;

	info2[1].flag_h = G2D_BLT_NONE_H;
	info2[1].op_flag = OP_BITBLT;
	info2[1].src_image_h.format = G2D_FORMAT_YUV420UVC_U1V1U0V0;
	info2[1].src_image_h.width = X2;
	info2[1].src_image_h.height = Y2;

	info2[1].src_image_h.clip_rect.w = X2;
	info2[1].src_image_h.clip_rect.h = Y2;

	info2[1].dst_image_h.format = G2D_FORMAT_YUV420UVC_U1V1U0V0;
	info2[1].dst_image_h.width = X2/2;
	info2[1].dst_image_h.height = Y2/2;

	info2[1].dst_image_h.clip_rect.w = X2/2;
	info2[1].dst_image_h.clip_rect.h = Y2/2;

	printf("start open file\n");
	sprintf(name, "/%s/bike_320x240_020.bin", G2D_FILE_PATH);
	file1 = fopen(name, "rb+");
	if (file1 == NULL)
		printf("err in fopen");
	ret = fread((void*)buf1, IMG_SIZE3*3/2, 1, file1);
	printf("fread,ret=%d\n", ret);
	fclose(file1);

	sprintf(name, "/%s/bike_480x320_220.bin", G2D_FILE_PATH);
	file1 = fopen(name, "rb+");
	if (file1 == NULL)
		printf("err in fopen");
	ret = fread((void*)buf3, IMG_SIZE2*3/2, 1, file1);
	printf("fread,ret=%d\n", ret);
	fclose(file1);

	printf("start control G2D_CMD_MIXER_TASK \n");

	//we use hal_malloc_coherent for iamge input,so we need to make sure caches flushed
	hal_dcache_clean((unsigned long)buf1, IMG_SIZE3*3/2);
	hal_dcache_clean((unsigned long)buf3, IMG_SIZE2*3/2);

	//keep clean to avoid eviction.
	hal_dcache_clean((unsigned long)buf2, IMG_SIZE3*4*3/2);
	hal_dcache_clean((unsigned long)buf4, IMG_SIZE2/4*3/2);

	printf("start control\n");
	arg[0] = (unsigned long)(info2);
	arg[1] = 2;
	ret = sunxi_g2d_control(G2D_CMD_MIXER_TASK, arg);
	if (ret)
	{
		printf("g2d G2D_CMD_MIXER_TASK fail\n");
		ret = -1;
		goto out;
	}
	else 
	{
		printf("G2D_CMD_MIXER_TASK ok\n");
	}

	//after the process of g2d hardware,now we invalidate the caches and get the output image
	hal_dcache_invalidate((unsigned long)buf2, IMG_SIZE3*4*3/2);
	hal_dcache_invalidate((unsigned long)buf4, IMG_SIZE2/4*3/2);

	printf("------write output image to file, may take a little time--------\n");
	sprintf(name, "/%s/g2d_mixer_out1.bin", G2D_FILE_PATH);
	file1 = fopen(name, "wb+");
	if (file1 == NULL)
		printf("err in fopen");

	ret = fwrite(buf2,IMG_SIZE3*4*3/2, 1, file1);
	printf("fwrite ,ret=%d\n",ret);
	printf("g2d_mixer_out1.bin : 640x480, YUV420\n");
	fclose(file1);

	sprintf(name, "/%s/g2d_mixer_out2.bin", G2D_FILE_PATH);
	file1 = fopen(name, "wb+");
	if (file1 == NULL)
		printf("err in fopen");

	ret = fwrite(buf4,IMG_SIZE2/4*3/2, 1, file1);
	printf("fwrite ,ret=%d\n",ret);
	printf("g2d_mixer_out2.bin : 240x460, NV12\n");
	fclose(file1);

	printf("\ng2d mixer_and_scale test finished\n\n");
	ret = 0;

out:
	hal_free_coherent(buf1);
	hal_free_coherent(buf2);
	hal_free_coherent(buf3);
	hal_free_coherent(buf4);
	sunxi_g2d_close();
	return ret;

}

int tt_g2d(int argc, char **argv)
{
	int ret = 0;

	ret = tt_g2d_fill_test();
	if (ret)
	{
		printf("g2d fill test failed!");
		return -1;
	}
	hal_msleep(1000);

	ret = tt_g2d_rotate_test();
	if (ret)
	{
		printf("g2d rotate test failed!");
		return -1;
	}
	hal_msleep(1000);

	ret = tt_g2d_blend_test();
	if (ret)
	{
		printf("g2d blend test failed!");
		return -1;
	}
	hal_msleep(1000);

	ret = tt_g2d_mixer_test();
	if (ret)
	{
		printf("g2d mixer_and_scale test failed!");
		return -1;
	}

	return 0;
}
testcase_init(tt_g2d, g2d, g2d for tinatest);
