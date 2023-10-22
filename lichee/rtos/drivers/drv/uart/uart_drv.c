#include <sunxi_drv_uart.h>
#include <uart_drv.h>
#include <aw_common.h>
#include <hal_uart.h>
#ifdef CONFIG_COMPONENT_IO_MULTIPLEX
#include <poll.h>
#include <waitqueue.h>
#endif

#define CHECK_POLL_OPS_FUNC_IS_VALAID(drv, func) \
	drv && drv->poll_ops && drv->poll_ops->func


int uart_drv_open(struct devfs_node *node)
{
	return 1;
}

int uart_drv_close(struct devfs_node *node)
{
	return 0;
}

ssize_t uart_drv_read(struct devfs_node *node, uint32_t addr, uint32_t size, void *data)
{
    size_t ret = 0;
    sunxi_driver_uart_t *pusr, *pvfy;
    sunxi_hal_driver_usart_t *hal_drv = NULL;

    if (node == NULL)
    {
        while (1);
    }

    pvfy = container_of(node, sunxi_driver_uart_t, base);
    pusr = (sunxi_driver_uart_t *)node->private;

    hal_assert(pvfy == pusr);

    if (pusr)
    {
        hal_drv = (sunxi_hal_driver_usart_t *)pusr->hal_drv;
    }

    if (hal_drv && hal_drv->receive)
    {
        ret = hal_drv->receive(pusr->dev_id, data, size);
    }

    return ret;
}

ssize_t uart_drv_write(struct devfs_node *node, uint32_t addr, uint32_t size, const void *data)
{
    size_t ret = 0;
    sunxi_driver_uart_t *pusr, *pvfy;
    sunxi_hal_driver_usart_t *hal_drv = NULL;

    if (node == NULL)
    {
        while (1);
    }

    pvfy = container_of(node, sunxi_driver_uart_t, base);
    pusr = (sunxi_driver_uart_t *)node->private;

    hal_assert(pvfy == pusr);

    if (pusr)
    {
        hal_drv = (sunxi_hal_driver_usart_t *)pusr->hal_drv;
    }

    if (hal_drv && hal_drv->send)
    {
        ret = hal_drv->send(pusr->dev_id, data, size);
    }

    return ret;
}

#ifdef CONFIG_COMPONENT_IO_MULTIPLEX
extern uint32_t uGetInterruptNest(void);
static void uart_wqueue_wakeup(rt_wqueue_t *queue, void *key)
{
    rt_base_t level;
    register int need_schedule = 0;

    rt_list_t *queue_list;
    struct rt_list_node *node;
    struct rt_wqueue_node *entry;
    long data = 0;
    struct rt_poll_node *poll_node;
    BaseType_t pxHigherPriorityTaskWoken = pdFALSE;

    queue_list = &(queue->waiting_list);

    level = vPortEnterCritical();
    /* set wakeup flag in the queue */
    queue->flag = RT_WQ_FLAG_WAKEUP;

    if (!(rt_list_isempty(queue_list)))
    {
        for (node = queue_list->next; node != queue_list; node = node->next)
        {
            entry = list_entry(node, struct rt_wqueue_node, list);
            poll_node = (struct rt_poll_node *)(entry);
            if (entry->wakeup(entry, key) == 0)
            {
                vPortExitCritical(level);
                if (uGetInterruptNest()) {
                    ret = xQueueSendFromISR(poll_node->pt->xQueue, &data, &pxHigherPriorityTaskWoken);
                    if (ret == pdPASS) {
                        portYIELD_FROM_ISR(pxHigherPriorityTaskWoken);
                    }
                } else {
                    xQueueSend(poll_node->pt->xQueue, &data, portMAX_DELAY);
                }
                level = vPortEnterCritical();
                need_schedule = 1;
                rt_wqueue_remove(entry);
                break;
            }
        }
    }
    vPortExitCritical(level);

    if (need_schedule)
    {
	void vTaskScheduerEnable(void);
        vTaskScheduerEnable();
    }
}

static int32_t uart_wakeup_poll_waitqueue(int32_t dev_id, short key)
{
    struct devfs_node *node;
    char devname [16];
    int32_t ret = -1;

    memset(&devname, 0, sizeof(devname));
    snprintf(devname, sizeof(devname), "uart%d", dev_id);
    node = devfs_get_node(devname);
    if (node) {
	uart_wqueue_wakeup(&node->wait_queue, (void *)(unsigned int)key);
	ret = 0;
    }
    return ret;
}

extern struct devfs_file *devfs_fd_to_file(int fd);
static int uart_fops_poll(int fd, struct rt_pollreq *req)
{
    struct devfs_node *dev;

    struct devfs_file *filp;
    filp = devfs_fd_to_file(fd);
    int mask = 0;
    short key;
    int ret = -1;
    sunxi_driver_uart_t *pusr, *pvfy;
    sunxi_hal_driver_usart_t *hal_drv = NULL;

    dev = (struct devfs_node *)filp->node;
    if (!dev)
    {
        return mask;
    }

    pvfy = container_of(dev, sunxi_driver_uart_t, base);
    pusr = (sunxi_driver_uart_t *)dev->private;

    hal_assert(pvfy == pusr);

    if (!pusr)
    {
        return mask;
    }

    hal_drv = (sunxi_hal_driver_usart_t *)pusr->hal_drv;
    key = req->_key;

    if (CHECK_POLL_OPS_FUNC_IS_VALAID(hal_drv, check_poll_state))
    {
        mask = hal_drv->poll_ops->check_poll_state(pusr->dev_id, key);
    }

    if (!mask && req->_proc != NULL)
    {
        if (CHECK_POLL_OPS_FUNC_IS_VALAID(hal_drv, register_poll_wakeup))
        {
            hal_drv->poll_ops->register_poll_wakeup(uart_wakeup_poll_waitqueue);
        }
        rt_poll_add(&dev->wait_queue, req);
    }
	/*
	 * TODO:
	 * Maybe it need to unregister poll_wakeup function, but need to take care of
	 * select the same fd as the same time
	 * */
    return mask;
}


static int uart_fops_open(const char * path, int flags, int mode)
{
    return 0;
}

static int uart_fops_close(int fd)
{
    return 0;
}

static int uart_fops_ioctl(int fd, int cmd, void *args)
{
    return 0;
}

static int uart_fops_read(int fd, void * dst, size_t size)
{
    struct devfs_node *dev;
    struct devfs_file *filp;
    filp = devfs_fd_to_file(fd);
    dev = (struct devfs_node *)filp->node;
    return uart_drv_read(dev, 0, size, dst);
}

static int uart_fops_write(int fd, const void * data, size_t size)
{
    struct devfs_node *dev;
    struct devfs_file *filp;
    filp = devfs_fd_to_file(fd);
    dev = (struct devfs_node *)filp->node;
    return uart_drv_write(dev , 0, size, (const void *)data);
}

static int uart_fops_lseek(int fd, off_t size, int mode)
{
    return 0;
}

static vfs_t uart_device_fops =
{
    .open       = uart_fops_open,
    .close      = uart_fops_close,
    .ioctl      = uart_fops_ioctl,
    .lseek      = uart_fops_lseek,
    .read       = uart_fops_read,
    .write      = uart_fops_write,
    .poll       = uart_fops_poll,
};
#endif

static sunxi_driver_uart_t uart0, uart1, uart2, uart3;
#ifdef CONFIG_COMPONENTS_AW_DEVFS
int register_uart_device(struct devfs_node *dev, void *usr_data, char *dev_name)
{
    dev->name = dev_name;
    dev->size = UINT32_MAX;
    dev->alias = "";
    dev->open = uart_drv_open;
    dev->close = uart_drv_close;
    dev->read = uart_drv_read;
    dev->write = uart_drv_write;
    dev->private = usr_data;

#ifdef CONFIG_COMPONENT_IO_MULTIPLEX
    dev->fops = &uart_device_fops;
    rt_wqueue_init(&(dev->wait_queue));
#endif
    return devfs_add_node(dev);
}

static sunxi_hal_poll_ops uart_poll_ops =
{
    .check_poll_state = hal_uart_check_poll_state,
    .hal_poll_wakeup = hal_uart_poll_wakeup,
    .register_poll_wakeup = hal_uart_register_poll_wakeup,
};

const sunxi_hal_driver_usart_t sunxi_hal_usart_driver =
{
    .get_version  = hal_uart_get_version,
    .get_capabilities = hal_uart_get_capabilities,
    .initialize = hal_uart_init,
    .uninitialize = hal_uart_deinit,
    .power_control = hal_uart_power_control,
    .send = hal_uart_send,
    .receive = (void *)hal_uart_receive,
    .get_tx_count = hal_uart_get_tx_count,
    .get_rx_count = hal_uart_get_rx_count,
    .get_status = hal_uart_get_status,
    .set_modem_control = hal_uart_set_modem_control,
    .get_modem_status = hal_uart_get_modem_status,
    .receive_polling = hal_uart_receive_polling,
#ifdef CONFIG_SUNXI_UART_SUPPORT_POLL
    .poll_ops = &uart_poll_ops,
#endif
};

void sunxi_driver_uart_init(void)
{
    struct devfs_node *node0, *node1, *node2, *node3;

#ifdef CONFIG_SUNXI_UART_REGISTER_UART0
    node0 = &uart0.base;
    uart0.dev_id = 0;
    uart0.hal_drv = &sunxi_hal_usart_driver;
    register_uart_device(node0, &uart0, "uart0");
#endif

#ifdef CONFIG_SUNXI_UART_REGISTER_UART1
    node1 = &uart1.base;
    uart1.dev_id = 1;
    uart1.hal_drv = &sunxi_hal_usart_driver;
    register_uart_device(node1, &uart1, "uart1");
#endif

#ifdef CONFIG_SUNXI_UART_REGISTER_UART2
    node2 = &uart2.base;
    uart2.dev_id = 2;
    uart2.hal_drv = &sunxi_hal_usart_driver;
    register_uart_device(node2, &uart2, "uart2");
#endif

#ifdef CONFIG_SUNXI_UART_REGISTER_UART3
    node3 = &uart3.base;
    uart3.dev_id = 3;
    uart3.hal_drv = &sunxi_hal_usart_driver;
    register_uart_device(node3, &uart3, "uart3");
#endif

}
#endif
