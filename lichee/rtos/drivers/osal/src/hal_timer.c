#include <string.h>

#include <hal_osal.h>
#include <FreeRTOS.h>
#include <timers.h>

#include <ktimer.h>
#include <hal/aw_list.h>
#include <hal_atomic.h>

#ifndef __DEQUALIFY
#define __DEQUALIFY(type, var) ((type)(uintptr_t)(const volatile void *)(var))
#endif
#ifndef offsetof
#define offsetof(type, field) \
	((size_t)(uintptr_t)((const volatile void *)&((type *)0)->field))
#endif
#ifndef __containerof
#define __containerof(ptr, type, field) \
	__DEQUALIFY(type *, (const volatile char *)(ptr) - offsetof(type, field))
#endif

static struct list_head os_timer_list = LIST_HEAD_INIT(os_timer_list);
static struct list_head os_timer_restore_list = LIST_HEAD_INIT(os_timer_restore_list);
static HAL_SPIN_LOCK_INIT(os_timer_lock);

typedef struct _os_timer_data_t
{
    timeout_func callback;
    void *arg;

    struct list_head node;
    osal_timer_t xTimer;
    uint8_t inlist;
    uint8_t reseting;
    uint8_t remain;
    TickType_t expiry_delta;
    TickType_t period_rec;
} os_timer_data_t;

static void os_timer_common_callback(TimerHandle_t xTimer)
{
    BaseType_t ret = pdPASS;
    os_timer_data_t *priv;

    priv = pvTimerGetTimerID(xTimer);
    if (priv && priv->callback)
    {
	if (priv->reseting) {
	    ret = osal_timer_control(priv->xTimer, OSAL_TIMER_CTRL_SET_TIME, &priv->period_rec);
	    if (ret != HAL_OK)
		hal_log_err("%s: set period err %ld\n", __func__, ret);
	}

        priv->callback(priv->arg);
    }
    else
    {
        hal_log_warn("Invalid timer callback\n");
    }
}

osal_timer_t osal_timer_create(const char *name,
                               timeout_func timeout,
                               void *parameter,
                               unsigned int tick,
                               unsigned char flag)
{
    unsigned long cpsr;
    osal_timer_t timer;
    os_timer_data_t *priv;
    priv = hal_malloc(sizeof(os_timer_data_t));
    if (priv == NULL)
    {
        return NULL;
    }
    memset(priv, 0, sizeof(os_timer_data_t));
    priv->callback = timeout;
    priv->arg = parameter;
    timer = xTimerCreate(name, tick, flag & OSAL_TIMER_FLAG_PERIODIC ? pdTRUE : pdFALSE, priv, os_timer_common_callback);
    if (timer) {
	cpsr = hal_spin_lock_irqsave(&os_timer_lock);
	priv->xTimer = timer;
	priv->inlist = 1;
	priv->reseting = 0;
	priv->remain = 0;
	priv->expiry_delta = 0;
	priv->period_rec = 0;
	list_add_tail(&priv->node, &os_timer_list);
	hal_spin_unlock_irqrestore(&os_timer_lock, cpsr);
    }

    return timer;
}

hal_status_t osal_timer_delete_timedwait(osal_timer_t timer, int ticks)
{
    unsigned long cpsr;
    BaseType_t ret;
    os_timer_data_t *priv;
    TickType_t xDelay = (TickType_t)ticks;

    priv = pvTimerGetTimerID((TimerHandle_t)timer);

    ret = xTimerDelete(timer, xDelay);
    if (ret != pdPASS)
    {
        hal_log_err("err %ld\n", ret);
        return HAL_ERROR;
    } else {
	cpsr = hal_spin_lock_irqsave(&os_timer_lock);
	priv->xTimer = NULL;
	priv->inlist = 0;
	priv->reseting = 0;
	priv->remain = 0;
        priv->expiry_delta = 0;
        priv->period_rec = 0;
	list_del(&priv->node);
	hal_spin_unlock_irqrestore(&os_timer_lock, cpsr);
    }

    hal_free(priv);
    return HAL_OK;
}

hal_status_t osal_timer_delete(osal_timer_t timer)
{
    TickType_t xDelay = portMAX_DELAY;

    return osal_timer_delete_timedwait(timer, xDelay);
}

hal_status_t osal_timer_start_timedwait(osal_timer_t timer, int ticks)
{
    BaseType_t ret;
    BaseType_t taskWoken;
    TickType_t xDelay = (TickType_t)ticks;

    if (hal_interrupt_get_nest())
    {
        taskWoken = pdFALSE;
        ret = xTimerStartFromISR(timer, &taskWoken);
        if (ret != pdPASS)
        {
            hal_log_err("err %ld\n", ret);
            return HAL_ERROR;
        }
        portEND_SWITCHING_ISR(taskWoken);
    }
    else
    {
        ret = xTimerStart(timer, xDelay);
        if (ret != pdPASS)
        {
            hal_log_err("err %ld\n", ret);
            return HAL_ERROR;
        }
    }

    return HAL_OK;
}

hal_status_t osal_timer_start(osal_timer_t timer)
{
     TickType_t xDelay = 0;

    return osal_timer_start_timedwait(timer, xDelay);
}

hal_status_t osal_timer_stop_timedwait(osal_timer_t timer, int ticks)
{
    BaseType_t ret;
    BaseType_t taskWoken;
    TickType_t xDelay = (TickType_t)ticks;

    if (hal_interrupt_get_nest())
    {
        taskWoken = pdFALSE;
        ret = xTimerStopFromISR(timer, &taskWoken);
        if (ret != pdPASS)
        {
            hal_log_err("err %ld\n", ret);
            return HAL_ERROR;
        }
        portEND_SWITCHING_ISR(taskWoken);
    }
    else
    {
        ret = xTimerStop(timer, xDelay);
        if (ret != pdPASS)
        {
            hal_log_err("err %ld\n", ret);
            return HAL_ERROR;
        }
    }

    return HAL_OK;
}

hal_status_t osal_timer_stop(osal_timer_t timer)
{
    TickType_t xDelay = portMAX_DELAY;

    return osal_timer_stop_timedwait(timer, xDelay);
}

hal_status_t osal_timer_control(osal_timer_t timer, int cmd, void *arg)
{
    unsigned long cpsr;
    BaseType_t ret = pdPASS;
    BaseType_t taskWoken;
    BaseType_t periodTick;
    os_timer_data_t *priv;

    if (hal_interrupt_get_nest())
    {
        taskWoken = pdFALSE;
        switch (cmd)
        {
            case OSAL_TIMER_CTRL_SET_TIME:
                periodTick = *(BaseType_t *)(arg);
                ret = xTimerChangePeriodFromISR(timer, periodTick, &taskWoken);
                break;
            default:
                break;
        }
        if (ret != pdPASS)
        {
            hal_log_err("err %ld\n", ret);
            return HAL_ERROR;
        }
        portEND_SWITCHING_ISR(taskWoken);
    }
    else
    {
        switch (cmd)
        {
            case OSAL_TIMER_CTRL_SET_TIME:
                periodTick = *(BaseType_t *)(arg);
                ret = xTimerChangePeriod(timer, periodTick, 0);
                break;
            default:
                break;
        }
        if (ret != pdPASS)
        {
            hal_log_err("err %ld\n", ret);
            return HAL_ERROR;
        }
    }

    priv = pvTimerGetTimerID((TimerHandle_t)timer);
    if (priv && priv->reseting) {
	cpsr = hal_spin_lock_irqsave(&os_timer_lock);
	priv->reseting = 0;
	hal_spin_unlock_irqrestore(&os_timer_lock, cpsr);
    }

    return HAL_OK;
}

hal_status_t osal_timer_remain(osal_timer_t timer, uint32_t remain)
{
    os_timer_data_t *priv;

    priv = pvTimerGetTimerID((TimerHandle_t)timer);
    if (priv && priv->inlist) {
    	if (remain)
	    priv->remain = 1;
	else
	    priv->remain = 0;
    }

    return HAL_OK;
}


hal_status_t osal_timer_start_all(void)
{
    unsigned long cpsr;
    BaseType_t ret = pdPASS;
    struct list_head *list_node = NULL;
    struct list_head *list_save = NULL;
    struct list_head *list = &os_timer_restore_list;
    os_timer_data_t *priv;

    list_for_each_safe(list_node, list_save, list) {
	priv = __containerof(list_node, os_timer_data_t, node);
	if (priv && priv->xTimer) {
	    if (xTimerIsTimerActive(priv->xTimer) != pdFALSE)
		continue;

	    ret = osal_timer_control(priv->xTimer, OSAL_TIMER_CTRL_SET_TIME, &priv->expiry_delta);
	    if (ret != HAL_OK)
            	hal_log_err("%s: set period err %ld\n", __func__, ret);

	    cpsr = hal_spin_lock_irqsave(&os_timer_lock);
	    if (ret == HAL_OK)
	    	priv->reseting = 1;
	    list_move(&priv->node, &os_timer_list);
	    hal_spin_unlock_irqrestore(&os_timer_lock, cpsr);

	    ret = osal_timer_start(priv->xTimer);
	    if (ret != HAL_OK)
            	hal_log_err("%s: start err %ld\n", __func__, ret);
	}
    }

    return HAL_OK;
}

hal_status_t osal_timer_stop_all(void)
{
    unsigned long cpsr;
    BaseType_t ret = pdPASS;
    struct list_head *list_node = NULL;
    struct list_head *list_save = NULL;
    struct list_head *list = &os_timer_list;
    os_timer_data_t *priv;

    list_for_each_safe(list_node, list_save, list) {
	priv = __containerof(list_node, os_timer_data_t, node);
	if (priv && priv->xTimer && !priv->remain) {
	    if (xTimerIsTimerActive(priv->xTimer) == pdFALSE)
		continue;
	    priv->period_rec = xTimerGetPeriod(priv->xTimer);
    	    if (hal_interrupt_get_nest())
	    	priv->expiry_delta = xTimerGetExpiryTime(priv->xTimer) - xTaskGetTickCountFromISR();
	    else
	    	priv->expiry_delta = xTimerGetExpiryTime(priv->xTimer) - xTaskGetTickCount();

	    ret = osal_timer_stop(priv->xTimer);
	    if (ret != HAL_OK) {
            	hal_log_err("%s: err %ld\n", __func__, ret);
	        priv->reseting = 0;
	        priv->period_rec = 0;
	        priv->expiry_delta = 0;
		osal_timer_start_all();
		return HAL_ERROR;
	    }

	    cpsr = hal_spin_lock_irqsave(&os_timer_lock);
	    list_move(&priv->node, &os_timer_restore_list);
	    hal_spin_unlock_irqrestore(&os_timer_lock, cpsr);
	}
    }

    return HAL_OK;
}
