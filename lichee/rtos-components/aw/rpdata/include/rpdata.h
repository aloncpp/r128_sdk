/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the people's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.
*
*
* THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
* PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
* WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
* THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
* OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
* IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
* NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
* OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef __RPDATA_H
#define __RPDATA_H

#define RPDATA_VERSION	"V0.5.6"

typedef struct _rpdata rpdata_t;

typedef int (*recv_cb_t)(rpdata_t *rpd, void *data, uint32_t data_len);

struct rpdata_cbs {
	recv_cb_t recv_cb;
};

#define RPDATA_DIR_CM33		(1)
#define RPDATA_DIR_RV		(2)
#define RPDATA_DIR_DSP		(3)
#define RPDATA_DIR_MAX		RPDATA_DIR_DSP

#define RPDATA_DIR_CM33_NAME	"CM33"
#define RPDATA_DIR_RV_NAME	"RV"
#define RPDATA_DIR_DSP_NAME	"DSP"

#if defined(CONFIG_ARCH_ARM_CORTEX_M33)
#define RPDATA_DIR_SELF		RPDATA_DIR_CM33
#elif defined(CONFIG_ARCH_RISCV)
#define RPDATA_DIR_SELF		RPDATA_DIR_RV
#elif defined(CONFIG_ARCH_DSP)
#define RPDATA_DIR_SELF		RPDATA_DIR_DSP
#else
#error "unknown ARCH"
#endif

rpdata_t *rpdata_create(int dir, const char *type, const char *name, size_t buf_len);
rpdata_t *rpdata_connect_with_cb(int dir, const char *type, const char *name,
				struct rpdata_cbs *cbs);
#define rpdata_connect(dir, type, name) 	rpdata_connect_with_cb(dir, type, name, NULL)
rpdata_t *rpdata_tryconnect(int dir, const char *type, const char *name);
void *rpdata_buffer_addr(rpdata_t *rpd);
int rpdata_buffer_len(rpdata_t *rpd);
int rpdata_is_connect(rpdata_t *rpd);
int rpdata_wait_connect(rpdata_t *rpd);
int rpdata_process(rpdata_t *rpd, unsigned int offset, unsigned int data_len);
int rpdata_send(rpdata_t *rpd, unsigned int offset, unsigned int data_len);
int rpdata_recv(rpdata_t *rpd, void *buf, int len, int timeout_ms);
int rpdata_set_recv_cb(rpdata_t *rpd, struct rpdata_cbs *cbs);
int rpdata_set_recv_ringbuffer(rpdata_t *rpd, int size);
int rpdata_clear_recv_ringbuffer(rpdata_t *rpd, int size);
int rpdata_set_private_data(rpdata_t *rpd, void *data);
void *rpdata_get_private_data(rpdata_t *rpd);
int rpdata_trydestroy(rpdata_t *rpd);
int rpdata_destroy(rpdata_t *rpd);

#endif /* __RPDATA_H */
