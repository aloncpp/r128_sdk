#ifndef PING_H
#define PING_H

#include "lwip/inet.h"
#include "lwip/ip_addr.h"

#ifdef __cplusplus
extern "C" {//}
#endif

#define PING_TO		5000    /* timeout to wait every reponse(ms) */
#define PING_ID		0xABCD
#define PING_DATA_SIZE	100     /* size of send frame buff, not include ICMP frma head */
#define PING_IP_HDR_SIZE	40
#define GET_TICKS	xTaskGetTickCount

#ifndef OS_MSEC_PER_SEC
#define OS_MSEC_PER_SEC 1000U
#endif

#ifndef OS_TICK
#define OS_TICK configTICK_RATE_HZ
#endif

#ifndef OS_TicksToMSecs
#define OS_TicksToMSecs(t) ((uint32_t)(t) / (OS_MSEC_PER_SEC / OS_TICK))
#endif

struct ping_data {
   ip_addr_t sin_addr;
   u32_t count;                /* number of ping */
   u32_t data_long;          /* the ping packet data long */
};

s32_t ping(struct ping_data *data);

#ifdef __cplusplus
}
#endif

#endif
