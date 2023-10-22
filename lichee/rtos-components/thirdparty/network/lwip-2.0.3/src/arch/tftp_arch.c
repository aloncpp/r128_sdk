/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the People's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY'S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS'SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY'S TECHNOLOGY.
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

#include "lwip/apps/tftp_server.h"
#include <fcntl.h>
#include <unistd.h>
#include "console.h"
#include <string.h>
#include <errno.h>

#define TFTP_DBG(fmt,...) \
    printf("aw-tftp[%s:%d] "fmt"\n", __FILE__, __LINE__, ##__VA_ARGS__)

#define TFTP_INFO(fmt,...) \
    printf("aw-tftp[%s:%d] "fmt"\n", __FILE__, __LINE__, ##__VA_ARGS__)

#define TFTP_WARNG(fmt,...) \
    printf("aw-tftp[%s:%d] "fmt"\n", __FILE__, __LINE__, ##__VA_ARGS__)

#define TFTP_ERROR(fmt,...) \
    printf("aw-tftp[%s:%d] "fmt"\n", __FILE__, __LINE__, ##__VA_ARGS__)


typedef struct {
	int8_t fd;
	char *file_name;
}tftp_handler_t;

tftp_handler_t aw_tftp_h = {
	.fd = -1,
	.file_name = NULL,
};

/**
 * Open file for read/write.
 * @param fname Filename
 * @param mode Mode string from TFTP RFC 1350 (netascii, octet, mail)
 * @param write Flag indicating read (0) or write (!= 0) access
 * @returns File handle supplied to other functions
 */

void* tftp_open(const char* fname, const char* mode, u8_t write)
{
	int8_t ret;

	ret = strlen(fname);

	aw_tftp_h.file_name = (char*)malloc(ret);
	if(aw_tftp_h.file_name == NULL) {
		TFTP_ERROR("file name malloc failed: %s",strerror(errno));
		return NULL;
	}
	memset(aw_tftp_h.file_name,0,ret);
	memcpy(aw_tftp_h.file_name,fname,ret-1);
	TFTP_DBG("file name : %s",aw_tftp_h.file_name);
	if(write) {
		ret = open(aw_tftp_h.file_name,O_WRONLY|O_CREAT);
		if(ret < 0) {
			TFTP_ERROR("open failed:%s",strerror(errno));
			goto failed;
		}
	}else{
		ret = open(aw_tftp_h.file_name,O_RDONLY);
		if(ret < 0) {
			TFTP_ERROR("open failed:%s",strerror(errno));
			goto failed;
		}
	}
	aw_tftp_h.fd = ret;
	return &aw_tftp_h;
failed:
	if(aw_tftp_h.file_name) {
		free(aw_tftp_h.file_name);
	}
	return NULL;
}
/**
 * Close file handle
 * @param handle File handle returned by open()
 */
void tftp_close(void* handle)
{
	tftp_handler_t *h=(tftp_handler_t*)handle;
	if(h->fd < 0)
		return ;

	close(h->fd);

	if(aw_tftp_h.file_name) {
		free(aw_tftp_h.file_name);
	}
}
/**
 * Read from file
 * @param handle File handle returned by open()
 * @param buf Target buffer to copy read data to
 * @param bytes Number of bytes to copy to buf
 * @returns &gt;= 0: Success; &lt; 0: Error
 */
int tftp_read(void* handle, void* buf, int bytes)
{
	tftp_handler_t *h=(tftp_handler_t*)handle;
	int ret;
	if(h->fd < 0) {
		TFTP_ERROR("file is not exist.");
		return -1;
	}
	TFTP_DBG("read: %d",bytes);
	ret = read(h->fd,buf,bytes);
	return ret;
}
/**
 * Write to file
 * @param handle File handle returned by open()
 * @param pbuf PBUF adjusted such that payload pointer points
 *             to the beginning of write data. In other words,
 *             TFTP headers are stripped off.
 * @returns &gt;= 0: Success; &lt; 0: Error
 */
int tftp_write(void* handle, struct pbuf* p)
{
	tftp_handler_t *h=(tftp_handler_t*)handle;
	int ret;
	if(h->fd < 0){
		TFTP_ERROR("file is not exist.");
		return -1;
	}
	ret = write(h->fd,p->payload,p->len);
	return ret;
}

struct tftp_context aw_tftp_ct = {
	.open = tftp_open,
	.close = tftp_close,
	.read = tftp_read,
	.write = tftp_write
};
void cmd_tftp_init(void)
{
	if(tftp_init(&aw_tftp_ct) == ERR_OK) {
		printf("tftp init success.\n");
	}
}
FINSH_FUNCTION_EXPORT_CMD(cmd_tftp_init,tftp, Console tftp start Command);
