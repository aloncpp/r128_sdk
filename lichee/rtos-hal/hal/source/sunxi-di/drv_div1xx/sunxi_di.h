/*
 * Copyright (c) 2020-2031 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef _UAPI_SUNXI_DI_H_
#define _UAPI_SUNXI_DI_H_

#include <typedef.h>
#include <errno.h>
#include "asm-generic/ioctl.h"

#ifndef __KERNEL__
typedef __u8 u8;
typedef __s32 s32;
typedef __u32 u32;
typedef __u64 u64;
#endif /* #ifdef __KERNEL__ */


enum {
	DI_INTP_MODE_INVALID = 0x0,
	DI_INTP_MODE_BOB,
	DI_INTP_MODE_MOTION,
};

enum {
	DI_OUT_0FRAME = 0x0,
	DI_OUT_1FRAME,
	DI_OUT_2FRAME,
};

enum {
	DI_TNR_MODE_INVALID = 0,
	DI_TNR_MODE_ADAPTIVE,
	DI_TNR_MODE_FIX,
};

enum {
	DI_TNR_LEVEL_HIGH = 0,
	DI_TNR_LEVEL_MIDDLE,
	DI_TNR_LEVEL_LOW,
};

enum {
	DI_BOTTOM_FIELD_FIRST = 0,
	DI_TOP_FIELD_FIRST = 1,
};

struct di_version {
	u32 version_major;
	u32 version_minor;
	u32 version_patchlevel;

	u32 ip_version;
};

struct di_timeout_ns {
	unsigned int client_number;
	u64 wait4start;
	u64 wait4finish;
};

/*
 * @mode:
 * @level:
 */
struct di_tnr_mode {
	u32 mode;
	u32 level;
};

struct di_size {
	u32 width;
	u32 height;
};

struct di_rect {
	u32 left;
	u32 top;
	u32 right;
	u32 bottom;
};

struct di_addr {
	u64 y_addr;
	u64 cb_addr;
	u64 cr_addr;
};

struct di_offset {
	u64 y_offset;
	u64 cb_offset;
	u64 cr_offset;
};

/*
 * support dma_buf method or phy_addr_buf method.
 * 1.On dma_buf method: use di_offset
 * 2.On phy_addr_buf method: use di_addr
 */
union di_buf {
	struct di_addr addr;
	struct di_offset offset;
};

/*
 * @format: see DRM_FORMAT_XXX in drm_fourcc.h.
 * @dma_buf_fd: dma buf fd that from userspace.
 *    @dma_buf_fd must be invalid(<0) on phy_addr_buf method.
 * @size.width,@size.height: size of pixel datas of image. unit: pixel.
 */
struct di_fb {
	s32 dma_buf_fd;
	union di_buf buf;
	struct di_size size;
};

struct di_process_fb_arg {
	unsigned int client_number;
	u8 is_interlace;
	u8 field_order; /*1:top field first 0:bottom field first*/
	u32 pixel_format;
	struct di_size size; /*size of source interlace picture*/

	u8 output_mode; /*0: 1-frame 1: 2-frame*/
	u8 di_mode; /*0:motion adaptive mode  1:inter field interpolation mode*/

	struct di_tnr_mode tnr_mode;

	struct di_fb in_fb0;
	struct di_fb in_fb1;
	struct di_fb in_fb2;

	struct di_fb out_fb0;
	struct di_fb out_fb1;
	struct di_fb out_tnr_fb;

};

struct di_mem_arg {
	unsigned int size;
	unsigned int handle;
	u64 phys_addr;
};

#define DI_IOC_MAGIC 'D'
#define DI_IO(nr)          _IO(DI_IOC_MAGIC, nr)
#define DI_IOR(nr, size)   _IOR(DI_IOC_MAGIC, nr, size)
#define DI_IOW(nr, size)   _IOW(DI_IOC_MAGIC, nr, size)
#define DI_IOWR(nr, size)  _IOWR(DI_IOC_MAGIC, nr, size)
#define DI_IOCTL_NR(n)     _IOC_NR(n)

#define DI_IOC_GET_VERSION    DI_IOR(0x0, struct di_version)

/*
#define DI_IOC_RESET          DI_IO(0x1)
#define DI_IOC_CHECK_PARA     DI_IO(0x2)
*/
#define DI_IOC_SET_TIMEOUT    DI_IOW(0x3, struct di_timeout_ns)

#define DI_IOC_SET_VIDEO_SIZE DI_IOW(0x4, struct di_size)
#define DI_IOC_SET_DIT_MODE   DI_IOW(0x5, struct di_dit_mode)
#define DI_IOC_SET_TNR_MODE   DI_IOW(0x6, struct di_tnr_mode)
#define DI_IOC_SET_FMD_ENABLE DI_IOW(0x7, struct di_fmd_enable)

#define DI_IOC_PROCESS_FB     DI_IOW(0x8, struct di_process_fb_arg)
#define DI_IOC_DESTROY     DI_IOW(0x9, int)
//#define DI_IOC_SET_VIDEO_CROP DI_IOW(0x9, struct di_rect)

#define DI_IOC_MEM_REQUEST    DI_IOWR(0x10, struct di_mem_arg)
#define DI_IOC_MEM_RELEASE    DI_IOWR(0x11, struct di_mem_arg)
#define DI_IOC_GET_CLIENT     DI_IOR(0x12, int)

//#define DI_IOC_SET_DEMO_CROP  DI_IOW(0x12, struct di_demo_crop_arg)

extern unsigned int di_device_get_debug_mode(void);
#endif /* #ifndef _UAPI_SUNXI_DI_H_ */
