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

#include <string.h>
#include <stdlib.h>
#include "kernel/os/os_time.h"
#include "kernel/os/os_debug.h"
#include "FreeRTOS.h"

extern uint32_t hal_interrupt_get_nest(void);
/* check if in ISR context or not */
int XR_OS_IsISRContext(void)
{
	return hal_interrupt_get_nest();
}

TickType_t XR_OS_CalcWaitTicks(XR_OS_Time_t msec)
{
	TickType_t tick;

	if (msec == XR_OS_WAIT_FOREVER) {
		tick = portMAX_DELAY;
	} else if (msec != 0) {
		tick = XR_OS_MSecsToTicks(msec);
		if (tick == 0)
			tick = 1;
	} else {
		tick = 0;
	}
	return tick;
}

void *XR_OS_Malloc(size_t size)
{
	return malloc(size);
}

void XR_OS_Free(void *ptr)
{
	free(ptr);
}

void *XR_OS_Memcpy(void *dest, const void *src, size_t len)
{
	return memcpy(dest, src, len);
}

void *XR_OS_Memset(void *s, int c, size_t n)
{
	return memset(s, c, n);
}

int XR_OS_Memcmp(const void *mem1, const void *mem2, size_t len)
{
	return memcmp(mem1, mem2, len);
}

void *XR_OS_Memmove(void *dest, const void *src, size_t len)
{
	return memmove(dest, src, len);
}
