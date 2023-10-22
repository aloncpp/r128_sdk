#include <stdio.h>
#include <stdlib.h>
#include <hal_time.h>

#include "xr_bt_defs.h"
#include "btmg_common.h"
#include "btmg_log.h"
#include "btmg_a2dp_sink.h"
#include "btmg_a2dp_source.h"
#include "btmg_spp_client.h"
#include "btmg_spp_server.h"
#include "btmg_hfp_hf.h"
#include "btmg_hfp_ag.h"

#define INTERVAL_TIME_NUM 5

struct interval_time {
    uint64_t start;
    void *tag;
    bool enable;
};

static struct interval_time diff_time[INTERVAL_TIME_NUM];

uint64_t btmg_interval_time(void *tag, uint64_t expect_time_ms)
{
    static bool init = false;

    int i;
    if (init == false) {
        memset(diff_time, 0, sizeof(diff_time));
        for (i = 0; i < INTERVAL_TIME_NUM; i++) {
            diff_time[i].enable = false;
            diff_time[i].tag = NULL;
        }
        init = true;
    }

    for (i = 0; i < INTERVAL_TIME_NUM; i++) {
        if (diff_time[i].enable == true && diff_time[i].tag == tag)
            break;
    }

    if (i >= INTERVAL_TIME_NUM) {
        for (i = 0; i < INTERVAL_TIME_NUM; i++) {
            if (diff_time[i].enable == false && diff_time[i].tag == NULL) {
                diff_time[i].enable = true;
                diff_time[i].tag = tag;
                //diff_time[i].start = XR_OS_GetTicks();
                diff_time[i].start = hal_gettime_ns();
                return 0;
            }
        }
    }

    for (i = 0; i < INTERVAL_TIME_NUM; i++) {
        if (diff_time[i].enable == true && diff_time[i].tag == tag) {
            uint64_t cur_time;
            uint64_t t = 0;
            //cur_time = XR_OS_GetTicks();
            cur_time = hal_gettime_ns();
            t = (cur_time - diff_time[i].start + t) / 1000000 ;

            if (t >= expect_time_ms) {
                //diff_time[i].start = XR_OS_GetTicks();
                diff_time[i].start = hal_gettime_ns();
                return t;
            }
        }
    }
    return 0;
}


int str2bda(const char *strmac, xr_bd_addr_t bda)
{
    uint8_t i;
    uint8_t *p;
    char *str, *next;

    if (strmac == NULL)
        return -1;

    p = (uint8_t *)bda;
    str = (char *)strmac;

    for (i = 0; i < 6; ++i) {
        p[i] = str ? strtoul(str, &next, 16) : 0;
        if (str)
            str = (*next) ? (next + 1) : next;
    }

    return 0;
}

void bda2str(xr_bd_addr_t bda, const char *bda_str)
{
    if (bda == NULL || bda_str == NULL) {
        return;
    }

    uint8_t *p = bda;
    sprintf((uint8_t *)bda_str, "%02x:%02x:%02x:%02x:%02x:%02x", p[0], p[1], p[2], p[3], p[4],
            p[5]);
}

bool btmg_disconnect_dev_list(dev_list_t *dev_list)
{
    dev_node_t *dev_node = NULL;
    int status_err = 0;

    if (!dev_list) {
        BTMG_ERROR("dev_list is null");
        return false;
    }

    if (dev_list->list_cleared) {
        BTMG_ERROR("dev_list is cleared, nothing could be done");
        return false;
    }

    dev_node = dev_list->head;

    while (dev_node != NULL) {
        dev_list->sem_flag = 1;
        if (dev_node->profile & A2DP_SRC_DEV) {
            status_err = bt_a2dp_source_disconnect(dev_node->dev_addr);
            if (!status_err) {
                XR_OS_SemaphoreWait(&dev_list->sem, XR_OS_SEMAPHORE_MAX_COUNT);
            }
            BTMG_DEBUG("A2DP_SRC_DEV SemaphoreWait OK!");
        }
        if (dev_node->profile & A2DP_SNK_DEV) {
            status_err = bt_a2dp_sink_disconnect(dev_node->dev_addr);
            if (!status_err) {
                XR_OS_SemaphoreWait(&dev_list->sem, XR_OS_SEMAPHORE_MAX_COUNT);
            }
            BTMG_DEBUG("A2DP_SNK_DEV SemaphoreWait OK!");
        }
        if (dev_node->profile & SPP_CLIENT_DEV) {
            status_err = bt_sppc_disconnect(dev_node->dev_addr);
            if (!status_err) {
                XR_OS_SemaphoreWait(&dev_list->sem, XR_OS_SEMAPHORE_MAX_COUNT);
            }
            BTMG_DEBUG("SPP_CLIENT_DEV SemaphoreWait OK!");
        }
        if (dev_node->profile & SPP_SERVER_DEV) {
            status_err = bt_spps_disconnect(dev_node->dev_addr);
            if (!status_err) {
                XR_OS_SemaphoreWait(&dev_list->sem, XR_OS_SEMAPHORE_MAX_COUNT);
            }
            BTMG_DEBUG("SPP_SERVER_DEV SemaphoreWait OK!");
        }
        if (dev_node->profile & HFP_HF_DEV) {
            status_err = bt_hfp_hf_disconnect_audio(dev_node->dev_addr);
            if (!status_err) {
                XR_OS_SemaphoreWait(&dev_list->sem, XR_OS_SEMAPHORE_MAX_COUNT);
            }
            BTMG_DEBUG("HFP_HF_DEV audio SemaphoreWait OK!");
            status_err = bt_hfp_hf_disconnect(dev_node->dev_addr);
            if (!status_err) {
                XR_OS_SemaphoreWait(&dev_list->sem, XR_OS_SEMAPHORE_MAX_COUNT);
            }
            BTMG_DEBUG("HFP_HF_DEV SemaphoreWait OK!");
        }
        if (dev_node->profile & HFP_AG_DEV) {
            status_err = bt_hfp_ag_disconnect_audio(dev_node->dev_addr);
            if (!status_err) {
                XR_OS_SemaphoreWait(&dev_list->sem, XR_OS_SEMAPHORE_MAX_COUNT);
            }
            BTMG_DEBUG("HFP_AG_DEV audio SemaphoreWait OK!");
            status_err = bt_hfp_ag_disconnect(dev_node->dev_addr);
            if (!status_err) {
                XR_OS_SemaphoreWait(&dev_list->sem, XR_OS_SEMAPHORE_MAX_COUNT);
            }
            BTMG_DEBUG("HFP_AG_DEV SemaphoreWait OK!");
        }

        dev_node = dev_node->next;
    }
    dev_list->sem_flag = 0;
    BTMG_DEBUG("btmg_dev_list_disconnect_devices FINISH!");
    return true;
}
