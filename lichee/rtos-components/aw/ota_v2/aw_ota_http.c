#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <console.h>
#include <aw_upgrade.h>
#include "ota_debug.h"
#include "aw_ota_http.h"
#include "httpclient/HTTPCUsr_api.h"

#undef write
#undef read
#undef close

static HTTPParameters *g_http_param;

int ota_update_http_init(void *url)
{
	if (g_http_param == NULL) {
		g_http_param = malloc(sizeof(HTTPParameters));
		if (g_http_param == NULL) {
			OTA_ERR("http param %p\n", g_http_param);
			return -1;
		}
	}
	memset(g_http_param, 0, sizeof(HTTPParameters));
	memcpy(g_http_param->Uri, url, strlen(url));

	OTA_DBG("%s(), success\n", __func__);
	return 0;
}

int ota_update_http_get(uint8_t *buf, uint32_t buf_size, uint32_t *recv_size, uint8_t *eof_flag)
{
	int	ret;

	ret = HTTPC_get(g_http_param, (CHAR *)buf, (INT32)buf_size, (INT32 *)recv_size);
	if (ret == HTTP_CLIENT_SUCCESS) {
		*eof_flag = 0;
		return 0;
	} else if (ret == HTTP_CLIENT_EOS) {
		*eof_flag = 1;
		free(g_http_param);
		g_http_param = NULL;
		return 0;
	} else {
		free(g_http_param);
		g_http_param = NULL;
		OTA_ERR("ret %d\n", ret);
		return -1;
	}
}

static char *base_name(char *path)
{
	char *p = strrchr(path, '/');
	return p ? p + 1 : path;
}

//use this func to download ota-step file
int ota_update_http_download_file(char *url, char *down_dir)
{
	int ret = -1, fd;
	int recvSize = 0;
	char *buf = NULL;
	unsigned int toReadLength = 4096;
	HTTPParameters *clientParams = NULL;
	char default_dir[64] = "/data/", *target_file = NULL;
	char *file_name = base_name(url);

	clientParams = malloc(sizeof(HTTPParameters));
	if (clientParams == NULL) {
		goto exit0;
	}
	memset(clientParams, 0, sizeof(HTTPParameters));

	buf = malloc(toReadLength);
	if (buf == NULL) {
		goto exit0;
	}

	if (down_dir != NULL) {
		OTA_DBG("down_dir:%s, len:%d\n", down_dir, strlen(down_dir));
		memset(default_dir, 0, 64);
		memcpy(default_dir, down_dir, strlen(down_dir));
	}
	strcat(default_dir, file_name);
	target_file = default_dir;
	OTA_DBG("download file to :%s\n", target_file);

	fd = open(target_file, O_CREAT | O_RDWR | O_TRUNC, S_IRWXG | S_IRWXO | S_IRWXU);
	if (fd < 0) {
		printf("Create file failed\n");
		goto exit0;
	}

	strcpy(clientParams->Uri, url);
	do {
		memset(buf, 0, toReadLength);
		ret = HTTPC_get(clientParams, buf, 4096, (INT32 *)&recvSize);
		if ((ret != HTTP_CLIENT_SUCCESS) && (ret != HTTP_CLIENT_EOS)) {
			printf("Transfer err...ret:%d\n", ret);
			ret = -1;
			break;
		}
		if (ret == HTTP_CLIENT_EOS) {
			printf("The end..\n");
			ret = write(fd, buf, recvSize);
			if (ret != recvSize) {
				printf("write error!\n");
				break;
			}
			ret = 0;
			break;
		}

#if 0
		printf("------------------ recv data ------------------\n");
		for (int i = 0; i < recvSize; ++i) {
			printf("0x%02x ", *(buf + i));
			if (((i + 1) % 20) == 0)
				printf("\n");
		}
		printf("--------------------- end ----------------------\n");
#endif
		ret = write(fd, buf, recvSize);
		if (ret != recvSize) {
			printf("write error!\n");
			break;
		}
	} while (1);

	close(fd);
exit0:
	free(buf);
	free(clientParams);
	return ret;
}

//get buf from url, update dev_path
int ota_update_http_update_device(char *url, char *dev_path)
{
	uint32_t recv_size = 0, get_size = 0, buf_offset = 0;
	uint8_t eof_flag = 0;
	int ret = 0;

	char *para = url;
	OTA_DBG("url:%s\n", para);

	char *ota_buf = NULL;
	ota_buf = malloc(4096);
	if (ota_buf == NULL) {
		OTA_ERR("malloc ota_buf failed\n");
		return -1;
	}

	ret = ota_update_http_init(para);
	if (ret != 0) {
		OTA_ERR("ota http init failed\n");
		ret = -1;
		goto http_err;
	}

	while (1) {
		memset(ota_buf, 0, 4096);
		ret = ota_update_http_get(ota_buf, 4096, &recv_size, &eof_flag);
		if (ret != 0) {
			OTA_ERR("http status %d\n", ret);
			goto http_err;
		}

		if (0 == recv_size) {
			OTA_WRN("http recv_size %d\n", recv_size);
			ret = -1;
			goto http_err;
		}

		OTA_DBG("http recv size %d, total size %d\n", recv_size, get_size);
#if 0
		printf("------------------ recv data ------------------\n");
		for (int i = 0; i < recv_size; ++i) {
			printf("0x%02x ", *(ota_buf + i));
			if (((i + 1) % 20) == 0)
				printf("\n");
		}
		printf("--------------------- end ----------------------\n");
#endif
		buf_offset += recv_size;
		//OTA_DBG("buf_offset:%d, eof_flag:%d\n", buf_offset, eof_flag);
		//if ((buf_offset >= 4096) || (eof_flag)) {
		if ((buf_offset > 0) || (eof_flag)) {
			ret = ota_upgrade_slice(dev_path, ota_buf, get_size, recv_size, eof_flag);
			printf("ret = %d\n", ret);
			if (ret)
				goto http_err;
		} else {
			continue;
		}

		get_size += buf_offset;
		buf_offset = 0;
		//otaprogressBar(get_size, HTTPWrapperGetRespContentLength());

		if (eof_flag && (get_size == HTTPWrapperGetRespContentLength())) {
			OTA_DBG("http recv finish, total size %dKB\n", get_size / 1024);
			ret = 0;
			break;
		}
	}

http_err:
	free(ota_buf);
	return ret;
}

static int aw_ota_http_cmd(int argc, char **argv)
{
	uint32_t recv_size = 0, get_size = 0, buf_offset = 0;
	uint8_t ota_buf[4096] = {0};
	uint8_t eof_flag = 0;
	char target_file[64] = "/data/test";
	int ret = 0;

	if (argc < 2 || argc > 3) {
		printf("usage:\n");
		printf("download file: aw_ota_http http://xxx/filename\n");
		printf("               aw_ota_http http://xxx/filename /data/\n");
		printf("update device: aw_ota_http http://xxx/filename /dev/xx\n");
		return -1;
	}

	char *para = argv[1];
	OTA_DBG("url:%s\n", para);

	if (argc == 2) {
		ret = ota_update_http_download_file(para, NULL);
	} else {
		if (strstr(argv[2], "/dev/")) {
			ret = ota_update_http_update_device(para, argv[2]);
		} else {
			ret = ota_update_http_download_file(para, argv[2]);
		}
	}
	return ret;
}
FINSH_FUNCTION_EXPORT_CMD(aw_ota_http_cmd, aw_ota_http, ota_http_test);

#if 0
static int httpc_get(char *url)
{
	int ret = -1;
	int recvSize = 0;
	char *buf = NULL;
	unsigned int toReadLength = 4096;
	HTTPParameters *clientParams = NULL;

	clientParams = malloc(sizeof(HTTPParameters));
	if (clientParams == NULL) {
		goto exit0;
	}
	memset(clientParams, 0, sizeof(HTTPParameters));

	buf = malloc(toReadLength);
	if (buf == NULL) {
		goto exit0;
	}
	memset(buf, 0, toReadLength);

	strcpy(clientParams->Uri, url);
	do {
		ret = HTTPC_get(clientParams, buf, 4096, (INT32 *)&recvSize);
		if ((ret != HTTP_CLIENT_SUCCESS) && (ret != HTTP_CLIENT_EOS)) {
			printf("Transfer err...ret:%d\n", ret);
			ret = -1;
			break;
		}
		if (ret == HTTP_CLIENT_EOS) {
			printf("The end..\n");
			ret = 0;
			break;
		}
	} while (1);

exit0:
	free(buf);
	free(clientParams);
	return ret;
}
#endif
