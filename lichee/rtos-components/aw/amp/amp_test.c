#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <hal_interrupt.h>
#include "sunxi_amp.h"

static int test_send_to_dev(sunxi_amp_info *amp, sunxi_amp_msg *msg)
{
    xQueueSend(amp->recv_queue, msg, portMAX_DELAY);
    return 0;
}

static int test_send_to_queue(sunxi_amp_info *amp, sunxi_amp_msg *msg)
{
    BaseType_t ret;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (hal_interrupt_get_nest()) {
        ret = xQueueSendFromISR(amp->send_queue, msg, &xHigherPriorityTaskWoken);
        if (ret == pdPASS) {
            (void)xHigherPriorityTaskWoken;
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    }
    else
        ret = xQueueSend(amp->send_queue, msg, portMAX_DELAY);

    if (ret != pdPASS) {
        return -1;
    }
    return 0;
}

static int test_receive_from_dev(sunxi_amp_info *amp, sunxi_amp_msg *msg)
{
    xQueueSend(amp->recv_queue, msg, portMAX_DELAY);
    return 0;
}

sunxi_amp_msg_ops test_ops =
{
    .send_to_queue = test_send_to_queue,
    .send_to_dev = test_send_to_dev,
    .receive_from_dev = test_receive_from_dev,
};

