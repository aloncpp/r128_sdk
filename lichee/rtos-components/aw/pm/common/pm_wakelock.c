
#include <errno.h>
#include <hal/aw_list.h>
#include <osal/hal_interrupt.h>
#include <interrupt.h>

#include "pm_adapt.h"
#include "pm_debug.h"
#include "pm_wakecnt.h"
#include "pm_wakelock.h"

#include "pm_state.h"

#undef  PM_DEBUG_MODULE
#define PM_DEBUG_MODULE  PM_DEBUG_WAKELOCK
/**
 * A wake lock is a mechanism to indicate that your application needs to have
 * the device stay on. Any application can using a WakeLock.
 * Call pm_wakelocks_acquire to acquire the wake lock and force the device to stay on.
 *
 * Call pm_wakelocks_release when you are done and don't need the lock anymore. It is
 * very important to do this as soon as possible to avoid running down the
 * device's battery excessively.
 */

#define wl_containerof(ptr_module) \
        __containerof(ptr_module, struct wakelock, node)

static LIST_HEAD(wakelocks_list);
static uint32_t          reference;
static TimerHandle_t     wl_timer;
static SemaphoreHandle_t wl_mutex;

static int wl_isvaild(struct wakelock *wl)
{
        if (!wl) {
                return false;
        }

        return true;
}

static int wl_invaild(struct wakelock *wl)
{
        if (wl) {
                wl->expires = OS_WAIT_FOREVER;
                wl->ref     = 0;
                wl->type    = PM_WL_TYPE_UNKOWN;
        }

        return 0;
}

/*
 * if wakelock in wakelocks_list, return itself, or return NULL.
 */
static struct wakelock *wl_check_inlist(struct wakelock *wl)
{
        struct wakelock  *ptr = NULL;
        struct list_head *pos = NULL;
        struct list_head *head = &wakelocks_list;

        /* check exist in list.*/
        list_for_each(pos, head) {
                ptr = wl_containerof(pos);
                if (ptr == wl)
                        goto out;
        }

	ptr = NULL;

out:
        return ptr;
}

/*
 * update expires
 */
static int wl_update_expires(struct wakelock *wl, uint32_t timeout_ms)
{

        int ret = 0;

        /* update wl->expires before insert it.*/
        switch (wl->type) {
        case PM_WL_TYPE_WAIT_TIMEOUT:
                if (!timeout_ms) {
                        pm_err("wakelock %s(%p) error!\n", wl->name, wl);
                        pm_abort(1);
                }

                wl->expires = OS_GetTicks() + OS_MSecsToTicks(timeout_ms);
                /*
                 * make sure that expires of timeout's wakelock cannot be OS_WAIT_FOREVER
                 */
                if (OS_WAIT_FOREVER == wl->expires)
                        wl->expires -= 1;
                break;
        case PM_WL_TYPE_WAIT_INC:
        case PM_WL_TYPE_WAIT_ONCE:
                wl->expires = OS_WAIT_FOREVER;
                break;
        default:
                break;
        }

        return ret;
}


static int wl_list_trydrop(struct wakelock *wl)
{
        int ret = 0;

        -- wl->ref;
        -- reference;

        if (!wl->ref) {
                list_del(&wl->node);
                wl_invaild(wl);
                /* pm_dbg("wakelock %s(%p) drop\n", wl->name, wl); */
        }

        return ret;
}


static int wl_list_sortinsert(struct wakelock *wl)
{
        struct wakelock  *_wl = NULL;
        struct list_head *pos = NULL;
        struct list_head *head = &wakelocks_list;

        /*
         * We insert nodes in order from smallest to largest.
         */
        list_for_each(pos, head) {
                _wl = wl_containerof(pos);

                if (OS_TimeAfterEqual(_wl->expires, wl->expires))
                        break;
        }

        list_add(&wl->node, pos->prev);

        /* pm_dbg("wakelock %s(%p) insert\n", wl->name, wl); */

        return 0;
}

static int wl_timer_update(void)
{
        struct wakelock  *wl = NULL;
        struct list_head *pos = NULL, *n = NULL;
        struct list_head *head = &wakelocks_list;
        uint32_t tick = 0;

        tick = OS_GetTicks();

        xSemaphoreTake(wl_mutex, portMAX_DELAY);
        list_for_each_safe(pos, n, head) {
                wl = wl_containerof(pos);

                if (wl->type == PM_WL_TYPE_WAIT_TIMEOUT || \
                        OS_TimeAfterEqual(tick, wl->expires)) {
                        /* The wakelock timeout */
                        wl_list_trydrop(wl);
                        continue;
                }
        }
        xSemaphoreGive(wl_mutex);

        return 0;
}

static void wl_timer_callback( TimerHandle_t xTimer )
{
	uint32_t a, b;

        wl_timer_update();

	/*check it*/
	a = pm_wakelocks_accumcnt();
	b = pm_wakelocks_refercnt(false);

	if (a != b) {
		pm_err("accumcnt(%d) not equal to totalcnt(%d).\n", a, b);
		pm_wakelocks_showall();
		pm_abort(1);
	}

}

void pm_wakelocks_setname(struct wakelock *wl, const char *name)
{
        if (!wl || !name) {
		pm_invalid();
		return;
        }

        wl->name = name;

	return;
}

int pm_wakelocks_acquire(struct wakelock *wl, enum pm_wakelock_t type, uint32_t timeout_ms)
{
        int ret = 0;
        struct wakelock *_wl = NULL;

        if (!wl_isvaild(wl))
                return -EINVAL;

        if (!wl->name)
                wl->name = "unkown-wakelock";

        /* check exist in list.*/
        xSemaphoreTake(wl_mutex, portMAX_DELAY);
        _wl  = wl_check_inlist(wl);
        if ( _wl == NULL) {
                /* A new wakelock */
                /* pm_dbg("wakelock add %s(%p)\n", wl->name, wl); */

                /* set wakelock type */
                wl->type = type;

                /* update expires */
                if (wl_update_expires(wl, timeout_ms)) {
                        ret = -EINVAL;
			goto out;
                }

                /* update reference */
                wl->ref ++;
                reference ++;

                /* insert to list.*/
                wl_list_sortinsert(wl);
        } else {
                /* A old wakelock */
                /* pm_dbg("wakelock update %s(%p)\n", wl->name, wl); */

                /* update expires */
                if (wl_update_expires(wl, timeout_ms)) {
                        ret = -EINVAL;
                        goto out;
                }

                /*update reference, increase only PM_WL_TYPE_WAIT_INC*/
                switch (type) {
                case PM_WL_TYPE_WAIT_INC:
                        wl->ref ++;
                        reference ++;
                        break;
                default:
                        break;
                }

                /* update list in order to sort it.*/
                list_del(&wl->node);
                wl_list_sortinsert(wl);
        }

out:
        xSemaphoreGive(wl_mutex);
        return ret;
}

int pm_wakelocks_release(struct wakelock *wl)
{
        int ret = 0;
        struct wakelock *_wl = NULL;

        if (!wl_isvaild(wl)) {
                return -EINVAL;
        }

        xSemaphoreTake(wl_mutex, portMAX_DELAY);

        /* check exist in list.*/
        _wl  = wl_check_inlist(wl);
        if ( _wl == NULL ) {
		pm_invalid();
                ret = -EINVAL;
                goto out;
	}

        wl_list_trydrop(wl);

out:
        xSemaphoreGive(wl_mutex);
        return ret;
}

uint32_t pm_wakelocks_accumcnt(void)
{
        struct wakelock  *_wl = NULL;
        struct list_head *pos = NULL;
        struct list_head *head = &wakelocks_list;
        uint32_t cnt = 0;

        xSemaphoreTake(wl_mutex, portMAX_DELAY);
        /* check exist in list.*/
        list_for_each(pos, head) {
                _wl = wl_containerof(pos);
                cnt += _wl->ref;
        }
        xSemaphoreGive(wl_mutex);

        return cnt;
}

uint32_t pm_wakelocks_refercnt(int stash)
{
    uint32_t cnt = 0;
	uint32_t flags;

	/* must disable all wakeupable interrupt */
	flags = hal_interrupt_disable_irqsave();
	cnt = reference;
	if (stash)
		pm_wakecnt_stash();
	hal_interrupt_enable_irqrestore(flags);

	return cnt;
}

void pm_wakelocks_showall(void)
{
        int i = 0;
        struct wakelock *wl;
        struct list_head *list;
        struct list_head *head = &wakelocks_list;

        xSemaphoreTake(wl_mutex, portMAX_DELAY);
        printf("wakelocks:");
        list_for_each(list, head) {
                wl = wl_containerof(list);
                printf("%d: %s ref:%d exp:%u\n", i++, wl->name, wl->ref, wl->expires);
        }
        printf("\n");
        xSemaphoreGive(wl_mutex);
}

void pm_wakelocks_block_hold_mutex(void)
{
		xSemaphoreTake(wl_mutex, portMAX_DELAY);
}

void pm_wakelocks_give_mutex(void)
{
		xSemaphoreGive(wl_mutex);
}

void pm_wakelocks_init(void)
{
	wl_mutex = xSemaphoreCreateMutex();
	if (!wl_mutex) {
		pm_err("wakelock: create semap failed.\n");
		goto err_return;
	}

        wl_timer = xTimerCreate("pm_wakelocks_timer",
                                    OS_MSecsToTicks(50),
                                    pdTRUE,
                                    NULL,
                                    wl_timer_callback);
	if (!wl_timer) {
		pm_err("wakelock: create timer failed.\n");
		goto err_freemutex;
	}

        xTimerStart(wl_timer, 10);
	return;

err_freemutex:
        vSemaphoreDelete(wl_mutex);
err_return:
	return;
}

void pm_wakelocks_deinit(void)
{
        vSemaphoreDelete(wl_mutex);

        xTimerStop(wl_timer, 10);
        xTimerDelete(wl_timer, 10);

	return;
}

