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
#include <hal_clk.h>
#include <sunxi_hal_htimer.h>
#include "sunxi_htimer.h"

void hal_htimer_init(void)
{
    int ret;

    ret = sunxi_htimer_clk_init();
    if (ret < 0)
        return;

    sunxi_htimer_init();
}

void hal_htimer_stop(hal_htimer_id timer)
{
    sunxi_htimer_stop(timer);
}

void hal_htimer_start(hal_htimer_id timer, bool periodic)
{
    sunxi_htimer_start(timer, periodic);
}

hal_htimer_status_t hal_htimer_set_oneshot(hal_htimer_id timer, uint32_t delay_us, timer_callback callback, void *callback_param)
{
    int ret = 0;

    ret = sunxi_htimer_set_oneshot(delay_us, timer, callback, callback_param);

    if (ret < 0)
    {
        return HAL_TIMER_STATUS_ERROR;
    }

    return HAL_TIMER_STATUS_OK;
}

hal_htimer_status_t hal_htimer_set_periodic(hal_htimer_id timer, uint32_t delay_us, timer_callback callback, void *callback_param)
{
    int ret = 0;

    ret = sunxi_htimer_set_periodic(delay_us, timer, callback, callback_param);

    if (ret < 0)
    {
        return HAL_TIMER_STATUS_ERROR;
    }

    return HAL_TIMER_STATUS_OK;
}
