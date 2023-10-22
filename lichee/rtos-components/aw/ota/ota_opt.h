#ifndef _OTA_OPT_H_
#define _OTA_OPT_H_

#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OTA_OPT_PROTOCOL_FILE		1
#define OTA_OPT_PROTOCOL_HTTP		1

#define OTA_OPT_EXTRA_VERIFY_CRC32	1
#define OTA_OPT_EXTRA_VERIFY_MD5	1
#define OTA_OPT_EXTRA_VERIFY_SHA1	1
#define OTA_OPT_EXTRA_VERIFY_SHA256	1

#define ota_malloc(l)			malloc(l)
#define ota_free(p)			free(p)
#define ota_memcpy(d, s, n)		memcpy(d, s, n)
#define ota_memset(s, c, n) 		memset(s, c, n)
#define ota_memcmp(a, b, l)		memcmp(a, b, l)

#define OTA_BUF_LEN			(1500)
#define OTA_BUF_SIZE			(258 * 1024)
#define OTA_BUF_SLICE_SIZE		(256 * 1024)
#define OTA_FLASH_TIMEOUT		(5000)

/**
 * @brief OTA status definition
 */
typedef enum ota_status {
	OTA_STATUS_OK		= 0,
	OTA_STATUS_ERROR	= -1,
} ota_status_t;

/**
 * @brief OTA protocol definition
 */
typedef enum ota_protocol {
#if OTA_OPT_PROTOCOL_FILE
	OTA_PROTOCOL_FILE	= 0,
#endif
#if OTA_OPT_PROTOCOL_HTTP
	OTA_PROTOCOL_HTTP	= 1,
#endif
	OTA_PROTOCOL_UNKNOWN =2,
} ota_protocol_t;

/**
 * @brief OTA image verification algorithm definition
 */
typedef enum ota_verify {
	OTA_VERIFY_NONE		= 0,
#if OTA_OPT_EXTRA_VERIFY_CRC32
	OTA_VERIFY_CRC32	= 1,
#endif
#if OTA_OPT_EXTRA_VERIFY_MD5
	OTA_VERIFY_MD5		= 2,
#endif
#if OTA_OPT_EXTRA_VERIFY_SHA1
	OTA_VERIFY_SHA1		= 3,
#endif
#if OTA_OPT_EXTRA_VERIFY_SHA256
	OTA_VERIFY_SHA256	= 4,
#endif
} ota_verify_t;

#if 0
/**
 * @brief OTA image verification data structure definition
 */
#define OTA_VERIFY_MAGIC        0x0055AAFF
#define OTA_VERIFY_DATA_SIZE	32
typedef struct ota_verify_data {
	uint32_t ov_magic;             /* OTA Verify Header Magic Number */
	uint16_t ov_length;            /* OTA Verify Data Length              */
	uint16_t ov_version;           /* OTA Verify Version: 0.0              */
	uint16_t ov_type;              /* OTA Verify Type                        */
	uint16_t ov_reserve;
	uint8_t ov_data[OTA_VERIFY_DATA_SIZE];
} ota_verify_data_t;

typedef enum ota_upgrade_status {
	OTA_UPGRADE_STOP,
	OTA_UPGRADE_START,
	OTA_UPGRADE_SUCCESS,
	OTA_UPGRADE_FAIL,
	OTA_UPGRADE_UPDATING
} ota_upgrade_status_t;

typedef void (*ota_callback) (ota_upgrade_status_t status, uint32_t data_size, uint32_t percentage);

ota_status_t ota_init(void);
void ota_deinit(void);
ota_status_t ota_set_skip_size(int32_t skip_size);
ota_status_t ota_set_cb(ota_callback cb);

ota_status_t ota_push_init(void);
ota_status_t ota_push_start(void);
ota_status_t ota_push_data(uint8_t *data, uint32_t size);
ota_status_t ota_push_finish(void);
ota_status_t ota_push_stop(void);

ota_status_t ota_get_image(ota_protocol_t protocol, void *url);
ota_status_t ota_get_verify_data(ota_verify_data_t *data);
ota_status_t ota_verify_image(ota_verify_t verify, uint32_t *value);
void ota_reboot(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* _OTA_OPT_H_ */
