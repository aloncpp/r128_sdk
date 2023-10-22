/* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
 *
 * Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
 *the the People's Republic of China and other countries.
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

#ifdef CONFIG_COMPONENTS_BT_PM
#include "bt_pm.h"
#include "pm_suspend.h"
#include "pm_wakelock.h"
#include "pm_devops.h"
#include "stdio.h"

#define BT_PM_ERROR_LEVEL   0
#define BT_PM_WARNING_LEVEL 1
#define BT_PM_DEBUG_LEVEL 2
#define BT_PM_INFO_LEVEL 3

#define BT_PM_ERROR(format, ...) do { \
        if (CONFIG_BT_PM_LOG_LEVEL >= BT_PM_ERROR_LEVEL) { \
            printf("[bt_pm_err]" format, ##__VA_ARGS__); \
        }\
    } while(0);

#define BT_PM_WARNING(format, ...) do { \
            if (CONFIG_BT_PM_LOG_LEVEL >= BT_PM_WARNING_LEVEL) { \
                printf("[bt_pm_warn]" format, ##__VA_ARGS__); \
            } \
        } while(0);

#define BT_PM_DEBUG(format, ...) do { \
            if (CONFIG_BT_PM_LOG_LEVEL >= BT_PM_DEBUG_LEVEL) { \
                printf("[bt_pm_debug]" format, ##__VA_ARGS__); \
            }\
        } while(0);

#define BT_PM_INFO(format, ...) do { \
            if (CONFIG_BT_PM_LOG_LEVEL >= BT_PM_INFO_LEVEL) { \
                printf("[bt_pm_info]" format, ##__VA_ARGS__); \
            } \
        } while(0);\

static int bt_pm_suspend(struct pm_device *dev, suspend_mode_t mode)
{
    switch (mode) {
    case PM_MODE_SLEEP:
    case PM_MODE_STANDBY:
    case PM_MODE_HIBERNATION:
        BT_PM_INFO("bt host suspend\n");
        break;
    default:
        break;
    }
    return 0;
}

static int bt_pm_resume(struct pm_device *dev, suspend_mode_t mode)
{
    (void)mode;
     BT_PM_INFO("bt host resume\n");
    return 0;
}

static struct pm_devops bt_host_devops = {
    .suspend = bt_pm_suspend,
    .resume = bt_pm_resume,
};

static struct pm_device bt_host_dev = {
    .name = "bt",
    .ops = &bt_host_devops,
};

static struct wakelock bt_pm_wakelock = {
    .name = "bt_host_wakelock",
    .ref = 0,
};

void bt_pm_lock(void)
{
    pm_wakelocks_acquire(&bt_pm_wakelock, PM_WL_TYPE_WAIT_INC, OS_WAIT_FOREVER);
}

void bt_pm_unlock(void)
{
    pm_wakelocks_release(&bt_pm_wakelock);
}

void bt_pm_init(void)
{
    pm_devops_register(&bt_host_dev);
}

void bt_pm_deinit(void)
{
    pm_devops_unregister(&bt_host_dev);
}

#endif
