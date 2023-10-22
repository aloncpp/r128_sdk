#include <string.h>
#include "ota_opt.h"
#include "ota_debug.h"
#include "ota_http.h"
#include "httpclient/HTTPCUsr_api.h"

#if OTA_OPT_PROTOCOL_HTTP

static HTTPParameters *g_http_param;

ota_status_t ota_update_http_init(void *url)
{
	if (g_http_param == NULL) {
		g_http_param = ota_malloc(sizeof(HTTPParameters));
		if (g_http_param == NULL) {
			OTA_ERR("http param %p\n", g_http_param);
			return OTA_STATUS_ERROR;
		}
	}
	ota_memset(g_http_param, 0, sizeof(HTTPParameters));
	ota_memcpy(g_http_param->Uri, url, strlen(url));

	OTA_DBG("%s(), success\n", __func__);
	return OTA_STATUS_OK;
}

ota_status_t ota_update_http_get(uint8_t *buf, uint32_t buf_size, uint32_t *recv_size, uint8_t *eof_flag)
{
	int	ret;

	ret = HTTPC_get(g_http_param, (CHAR *)buf, (INT32)buf_size, (INT32 *)recv_size);
	if (ret == HTTP_CLIENT_SUCCESS) {
		*eof_flag = 0;
		return OTA_STATUS_OK;
	} else if (ret == HTTP_CLIENT_EOS) {
		*eof_flag = 1;
		ota_free(g_http_param);
		g_http_param = NULL;
		return OTA_STATUS_OK;
	} else {
		ota_free(g_http_param);
		g_http_param = NULL;
		OTA_ERR("ret %d\n", ret);
		return OTA_STATUS_ERROR;
	}
}

#endif /* OTA_OPT_PROTOCOL_HTTP */
