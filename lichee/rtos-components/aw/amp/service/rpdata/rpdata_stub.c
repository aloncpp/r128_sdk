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
#include <reent.h>

#include "sunxi_amp.h"
#include <hal_cache.h>
#include <hal_mutex.h>
#include <console.h>

#include "include/rpdata_amp.h"

MAYBE_STATIC int rpdata_test1(void)
{
	int dir;

	dir = RPC_MSG_DIR_RV;
	printf("[%s] line:%d dir=%d\n", __func__, __LINE__, dir);
	return func_stub(RPCCALL_RPDATA(rpdata_test1, dir), 1, 0, NULL);
}

int rpdata_ioctl(int dir, int cmd, void *data, uint32_t len)
{
	int ret;
	void *args[3] = {0};
	args[0] = (void *)(uintptr_t)cmd;
	args[1] = (void *)data;
	args[2] = (void *)(uintptr_t)len;

	/*printf("[%s] line:%d data:%p, len:%u\n", __func__, __LINE__, data, len);*/
	if (cmd & RPDATA_AMP_IN) {
		hal_dcache_clean((unsigned long)data, (unsigned long)len);
	}
	if (cmd & RPDATA_AMP_OUT) {
		hal_dcache_invalidate((unsigned long)data, (unsigned long)len);
	}

	ret = func_stub(RPCCALL_RPDATA(rpdata_ioctl, dir), 1, ARRAY_SIZE(args), args);

	if (cmd & RPDATA_AMP_OUT) {
		hal_dcache_invalidate((unsigned long)data, (unsigned long)len);
	}
	return 0;
}
