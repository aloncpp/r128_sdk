#ifndef _AW_OTA_CORE_H_
#define _AW_OTA_CORE_H_

#ifdef __cplusplus
extern "C" {
#endif

#define OTA_FILE_MAX_LEN (128)

/**
 * @brief OTA protocol definition
 */
typedef enum ota_protocol {
    OTA_PROTOCOL_FILE   = 0,
    OTA_PROTOCOL_HTTP   = 1,
    OTA_PROTOCOL_UNKNOWN =2,
} ota_protocol_t;



#ifdef __cplusplus
}
#endif

#endif
