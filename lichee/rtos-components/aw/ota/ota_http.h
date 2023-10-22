#ifndef _OTA_HTTP_H_
#define _OTA_HTTP_H_

#include "ota_opt.h"

#ifdef __cplusplus
extern "C" {
#endif

#if OTA_OPT_PROTOCOL_HTTP
ota_status_t ota_update_http_init(void *url);
ota_status_t ota_update_http_get(uint8_t *buf, uint32_t buf_size, uint32_t *recv_size, uint8_t *eof_flag);
#endif

#ifdef __cplusplus
}
#endif

#endif /* _OTA_HTTP_H_ */
