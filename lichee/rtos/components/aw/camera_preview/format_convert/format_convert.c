#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <hal_mem.h>
#include "format_convert.h"
#include "turbojpeg.h"
#include "jpeglib.h"

#ifdef CONFIG_DRIVERS_G2D
#include "g2d_driver.h"
extern int sunxi_g2d_control(int cmd, void *arg);
extern int sunxi_g2d_close(void);
extern int sunxi_g2d_open(void);

static int g2d_yuv_format_convert(struct camera_preview_buf src_buf, struct camera_preview_buf dst_buf, int fbindex)
{
	g2d_blt_h blit_para;

	if (sunxi_g2d_open()) {
		printf("[%s,%d]: g2d open fail\n", __func__, __LINE__);
		return -1;
	}

	memset(&blit_para, 0, sizeof(g2d_blt_h));
	blit_para.src_image_h.laddr[0] = __va_to_pa((uint32_t)src_buf.addr);//phy_addr1;
	blit_para.dst_image_h.laddr[0] = __va_to_pa((uint32_t)(dst_buf.addr + dst_buf.size * fbindex));//phy_addr2;

	blit_para.src_image_h.laddr[0] = (int)(blit_para.src_image_h.laddr[0] );
	blit_para.src_image_h.laddr[1] = (int)(blit_para.src_image_h.laddr[0] + src_buf.width * src_buf.height);
	if (src_buf.fmt == CP_FMT_YUYV)
		blit_para.src_image_h.laddr[2] = (int)(blit_para.src_image_h.laddr[0] +
				src_buf.width * src_buf.height * 3 / 2);
	else
		blit_para.src_image_h.laddr[2] = (int)(blit_para.src_image_h.laddr[0] +
				src_buf.width * src_buf.height * 5 / 4);
	blit_para.src_image_h.use_phy_addr = 1;


	blit_para.dst_image_h.laddr[0] = (int)(blit_para.dst_image_h.laddr[0] );
	blit_para.dst_image_h.use_phy_addr = 1;

	cp_dbg("%s: src_addr=0x%lx, dst_addr=0x%lx\n", __func__, blit_para.src_image_h.laddr[0],
			blit_para.dst_image_h.laddr[0]);

	blit_para.src_image_h.clip_rect.x = 0;
	blit_para.src_image_h.clip_rect.y = 0;
	blit_para.src_image_h.clip_rect.w = src_buf.width;
	blit_para.src_image_h.clip_rect.h = src_buf.height;

	blit_para.dst_image_h.clip_rect.x = 0;
	blit_para.dst_image_h.clip_rect.y = 0;
	blit_para.dst_image_h.clip_rect.w = dst_buf.width;
	blit_para.dst_image_h.clip_rect.h = dst_buf.height;

	if (src_buf.fmt == CP_FMT_YUYV)
		blit_para.src_image_h.format = G2D_FORMAT_IYUV422_V0Y1U0Y0;
	else
		blit_para.src_image_h.format = G2D_FORMAT_YUV420UVC_V1U1V0U0;
	blit_para.src_image_h.width = src_buf.width;
	blit_para.src_image_h.height = src_buf.height;

	if (dst_buf.fmt == CP_FMT_RGB565)
		blit_para.dst_image_h.format = G2D_FORMAT_RGB565;
	else if (dst_buf.fmt == CP_FMT_RGB888)
		blit_para.dst_image_h.format = G2D_FORMAT_RGB888;
	else
		blit_para.dst_image_h.format = G2D_FORMAT_ARGB8888;
	blit_para.dst_image_h.width = dst_buf.width;
	blit_para.dst_image_h.height = dst_buf.height;

	blit_para.flag_h = G2D_BLT_NONE_H;

	blit_para.dst_image_h.color = 0xee8899;
	blit_para.dst_image_h.mode = G2D_PIXEL_ALPHA;
	blit_para.dst_image_h.alpha = 255;
	blit_para.src_image_h.color = 0xee8899;
	blit_para.src_image_h.mode = G2D_PIXEL_ALPHA;
	blit_para.src_image_h.alpha = 255;

//	hal_dcache_clean((unsigned long)src_buf.addr, src_buf.size);
	hal_dcache_clean((unsigned long)dst_buf.addr, dst_buf.size);

	if (sunxi_g2d_control(G2D_CMD_BITBLT_H, &blit_para) < 0) {
		printf("[%s,%d]: g2d G2D_CMD_BITBLT_H fail\n", __func__, __LINE__);
		sunxi_g2d_close();
		return -1;
	}
	else {
		cp_dbg("G2D_CMD_BITBLT_H ok\n");
	}

	hal_dcache_invalidate((unsigned long)dst_buf.addr, dst_buf.size);
	sunxi_g2d_close();
	return 0;
}
#endif

static int sw_yuv_format_convert(struct camera_preview_buf src_buf, struct camera_preview_buf dst_buf, int fbindex)
{
	printf("Software YUV format convert RGB not support yet! Please enable CONFIG_DRIVERS_G2D config!\n");
	return 0;
}

static int yuv_format_convert(struct camera_preview_buf src_buf, struct camera_preview_buf dst_buf, int fbindex)
{
	int ret = 0;
#ifdef CONFIG_DRIVERS_G2D
	ret = g2d_yuv_format_convert(src_buf, dst_buf, fbindex);
#else
	ret = sw_yuv_format_convert(src_buf, dst_buf, fbindex);
#endif
	return ret;
}

static int jpeg_format_convert(struct camera_preview_buf src_buf, struct camera_preview_buf dst_buf, int fbindex)
{
#ifdef CONFIG_COMPONENTS_LIBJPEG_TURBO
	int ret = 0;
	if (dst_buf.fmt == CP_FMT_RGB565) {
		struct jpeg_decompress_struct cinfo;
		struct jpeg_error_mgr jerr;

		/*绑定错误处理结构对象*/
		cinfo.err = jpeg_std_error(&jerr);
		/*初始化*/
		jpeg_create_decompress(&cinfo);
		/*指定输入內存*/
		jpeg_mem_src(&cinfo, src_buf.addr, src_buf.size);
		/*读取文件信息*/
		jpeg_read_header(&cinfo, TRUE);

		/*设置缩放比例*/
		cinfo.scale_num  = 1;
		cinfo.scale_denom = 1;

		/*设置输出像素格式*/
		cinfo.out_color_space = JCS_RGB565;
		/*解压*/
		jpeg_start_decompress(&cinfo);
		/*存储解压数据*/
		unsigned char *data = (unsigned char *)(dst_buf.addr + dst_buf.size * fbindex);
		/*逐行扫描获取解压信息*/
		JSAMPROW row_pointer[1];
		while (cinfo.output_scanline < cinfo.output_height) {
			row_pointer[0] = &data[(cinfo.output_height - cinfo.output_scanline - 1)
				* cinfo.output_width * 2];
			jpeg_read_scanlines(&cinfo, row_pointer, 1);
		}
		/*完成解压*/
		jpeg_finish_decompress(&cinfo);
		/*销毁解压cinfo信息*/
		jpeg_destroy_decompress(&cinfo);

	} else {
		tjhandle handle = NULL;
		int pixelfmt = 0;
		if (dst_buf.fmt == CP_FMT_RGB888)
			pixelfmt = TJPF_BGR;
		else
			pixelfmt = TJPF_BGRA;

		/*创建一个turbojpeg句柄*/
		handle = tjInitDecompress();
		if (handle == NULL)  {
			return -1;
		}
		/*解压缩时，tjDecompress2()会自动根据设置的大小进行缩放，但是设置的大小要在它的支持范围，如1/2 1/4等*/
		ret = tjDecompress2(handle, src_buf.addr, src_buf.size, dst_buf.addr + dst_buf.size * fbindex,
				dst_buf.width, 0, dst_buf.height, pixelfmt, 0);
		if (ret != 0) {
			tjDestroy(handle);
			return -1;
		}
		tjDestroy(handle);
	}
#else
	printf("Please enable CONFIG_COMPONENTS_LIBJPEG_TURBO config!\n");
#endif
	return 0;
}

int format_convert(struct camera_preview_buf src_buf, struct camera_preview_buf dst_buf, int fbindex)
{
	int ret = 0;
	if (src_buf.fmt == CP_FMT_JPEG)
		ret = jpeg_format_convert(src_buf, dst_buf, fbindex);
	else
		ret = yuv_format_convert(src_buf, dst_buf, fbindex);
	return ret;
}
