#ifndef _PM_WAKELOCK_H_
#define _PM_WAKELOCK_H_

#ifdef CONFIG_KERNEL_FREERTOS
#include <FreeRTOS.h>
#include <semphr.h>
#include <timers.h>
#else
#error "PM do not support the RTOS!!"
#endif

#include <hal/aw_list.h>

#ifdef __cplusplus
extern "C" {
#endif

enum pm_wakelock_t {
        PM_WL_TYPE_UNKOWN = 0,
        PM_WL_TYPE_WAIT_ONCE,
        PM_WL_TYPE_WAIT_INC,
        PM_WL_TYPE_WAIT_TIMEOUT,
};

struct wakelock {
        const char              *name;

        /* pm core use, do not edit */
        struct list_head        node;
        uint32_t                expires;
        uint16_t                ref;
        uint16_t                type;
};

#define PM_WAKELOCK_USE_GLOBE_CNT
#define OS_WAIT_FOREVER		0xffffffffU

void pm_wakelocks_init(void);
void pm_wakelocks_deinit(void);

void pm_wakelocks_setname(struct wakelock *wl, const char *name);
int  pm_wakelocks_acquire(struct wakelock *wl, enum pm_wakelock_t type, uint32_t timeout);
int  pm_wakelocks_release(struct wakelock *wl);

uint32_t pm_wakelocks_accumcnt(void); /*don't use it*/
uint32_t pm_wakelocks_refercnt(int stash);
void     pm_wakelocks_showall(void);

void pm_wakelocks_block_hold_mutex(void);
void pm_wakelocks_give_mutex(void);

#ifdef __cplusplus
}
#endif

#endif /* __XRADIO_PM_H */
