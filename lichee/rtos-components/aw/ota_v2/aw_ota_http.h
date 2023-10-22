#ifndef _AW_OTA_HTTP_H_
#define _AW_OTA_HTTP_H_

#ifdef __cplusplus
extern "C" {
#endif

int ota_update_http_init(void *url);
int ota_update_http_get(uint8_t *buf, uint32_t buf_size, uint32_t *recv_size, uint8_t *eof_flag);
int ota_update_http_download_file(char *url, char *down_dir);
int ota_update_http_update_device(char *url, char *dev_path);

#ifdef __cplusplus
}
#endif

#endif /* _AW_OTA_HTTP_H_ */
