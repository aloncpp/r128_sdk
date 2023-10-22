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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "sunxi_amp.h"
#include <hal_cache.h>

#include "include/rpdata_amp.h"

__attribute__((weak)) int amp_service_rpdata_ioctl(int cmd, void *data, uint32_t len)
{
	return 0;
}

static int rpdata_magic_check(int cmd)
{
	if (RPDATA_AMP_MAGIC_MASK(cmd) != RPDATA_AMP_MAGIC) {
		printf("rpdata amp magic check failed:0x%x\n", cmd);
		while (1);
		return -1;
	}
	return 0;
}

static int _rpdata_test1(void)
{
	printf("[%s] line:%d \n", __func__, __LINE__);
	return 0;
}

static int _rpdata_ioctl(int cmd, void *data, uint32_t len)
{
	int ret;

	/*printf("[%s] line:%d data:%p, len=%u\n", __func__, __LINE__, data, len);*/
	/*printf("cmd:0x%08x\n", cmd);*/
	if (rpdata_magic_check(cmd) < 0) {
		printf("cmd check failed\n");
		return -1;
	}
	if (data != NULL)
		hal_dcache_invalidate((unsigned long)data, (unsigned long)len);
	ret = amp_service_rpdata_ioctl(cmd, data, len);
	if (cmd & RPDATA_AMP_OUT)
		hal_dcache_clean((unsigned long)data, (unsigned long)len);
	/*printf("[%s] line:%d \n", __func__, __LINE__);*/

	return ret;
}

sunxi_amp_func_table rpdata_table[] = {
	{.func = (void *)&_rpdata_test1,			.args_num = 0, .return_type = RET_POINTER},
	{.func = (void *)&_rpdata_ioctl,			.args_num = 3, .return_type = RET_POINTER},
};
