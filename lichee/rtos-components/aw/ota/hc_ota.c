#include <console.h>
#include <pthread.h>
#include <aw_upgrade.h>
#include "ota_opt.h"
#include "ota_debug.h"
#include "ota_http.h"
#include "aw_ota.h"
#include "HTTPClientWrapper.h"

#define MAX_HC_OTA_PARA_SIZE	(1024)
#define HC_OTA_PRIORITY		(4)
#define HC_OTA_STACKSIZE	(10 * 1024)

struct progress
{
        int socket;
        int start;
        int end;
        void (*FUNC)(int);
};

static struct progress pgress;

static void ota_help(void)
{
	OTA_DBG("hcota [url]\n\n"
			"example:\n"
			"\thcota http://192.168.1.100/rtos_ota_app.fex\n\n");
}

int hc_download_rtos(void *para)
{
	int ret = OTA_STATUS_ERROR;
	uint32_t recv_size = 0, get_size = 0;
	uint8_t *ota_buf = NULL;
	uint8_t eof_flag = 0;
	char *target_file = NULL;
	char target_rtos[16 + 5 + 1]; /* 5 is "/dev/" */
	char *url = (char *)para;
	uint32_t buf_offset = 0;

	ota_buf = (uint8_t *)ota_malloc(OTA_BUF_SIZE);
        if (NULL == ota_buf) {
                OTA_ERR("alloc ota_buf fail\n");
		goto ota_err;
        }

	aw_upgrade_start(0); /* reset OTA env info before restart a new OTA */

	if (get_rtos_to_upgrade(target_rtos) != 0) {
		goto ota_err;
	} else {
		target_file = target_rtos;
	}

#if 0
	OTA_DBG("start to erase flash...\n");
	int part_erase(const char *partiton);
	if (part_erase(target_file) != 0) {
		OTA_ERR("part_erase flash failed\n");
		goto ota_err;
        }
        OTA_DBG("erase %s success\n", target_file);

	int part_config(const char *partition, uint32_t erase_before_write);
	if (part_config(target_file, 1) != 0) {
		OTA_ERR("part_config flash failed\n");
		goto ota_err;
	}
#endif
        OTA_DBG("url:%s\n", para);

	ret = ota_update_http_init(para);
	if (ret != OTA_STATUS_OK) {
		OTA_ERR("ota http init failed\n");
		goto ota_err;
	}

	while (1) {
		ret = ota_update_http_get(ota_buf + buf_offset, OTA_BUF_LEN, &recv_size, &eof_flag);
		if (ret != OTA_STATUS_OK) {
			OTA_ERR("http status %d\n", ret);
			goto ota_err;
		}

		if (0 == recv_size) {
			OTA_WRN("http recv_size %d\n", recv_size);
			ret = OTA_STATUS_ERROR;
			goto ota_err;
		} else {
			//OTA_DBG("http recv size %d, total size %d\n", recv_size, get_size);
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
			if ((buf_offset >= OTA_BUF_SLICE_SIZE) || (eof_flag)) {
				//ret = upgrade_slice(DEFAULT_SOURCE_FILE, ota_buf, target_file, get_size, recv_size);
				ret = upgrade_slice(DEFAULT_SOURCE_FILE, ota_buf, target_file, get_size, buf_offset);
				if (ret)
					goto ota_err;
			} else {
				continue;
			}

			get_size += buf_offset;
			buf_offset = 0;
			//get_size += recv_size;
			otaprogressBar(get_size, HTTPWrapperGetRespContentLength());
		}

		if (eof_flag && (get_size == HTTPWrapperGetRespContentLength())) {
			OTA_DBG("http recv finish, total size %dKB\n", get_size / 1024);
			aw_upgrade_end(0);
			ret = OTA_STATUS_OK;
			break;
		}
	}

ota_err:
        if (ota_buf)
                free(ota_buf);

        return ret;
}

int update_from_hc_network(char* url)
{
	OTA_DBG("%s,Start to download...\n", __func__);
#if 0
	pthread_attr_t attr;
	struct sched_param sched;
	sched.sched_priority = HC_OTA_PRIORITY;

	pthread_attr_init(&attr);
	pthread_attr_setschedparam(&attr, &sched);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	pthread_attr_setstacksize(&attr, HC_OTA_STACKSIZE);
	//pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

        pthread_t hc_download_thread;
        pthread_create(&hc_download_thread, &attr, hc_download_rtos, (void *)url);
	//pthread_setname_np(hc_download_thread, "hcotaThread");
        pthread_join(hc_download_thread, NULL);
#else
	return hc_download_rtos(url);
#endif
}

#ifdef CONFIG_COMPONENTS_AW_UPGRADE_MD5SUM
int verify_md5_for_hc_network(unsigned char md5[16])
{
        int ret = 0;
        int source_file_size = HTTPWrapperGetRespContentLength();
        char target_rtos[16 + 5 + 1]; /* 5 is "/dev/" */
        unsigned char target_md5[16] = {0};

        get_rtos_to_upgrade(target_rtos);
        OTA_DBG("target:%s\n", target_rtos);
        OTA_DBG("source_file_size:%d\n", source_file_size);

        int get_md5sum(char *path, unsigned int file_size, unsigned char decrypt[16]);
        ret = get_md5sum(target_rtos, source_file_size, target_md5);
        if (ret)
        {
                return -1;
        }

        if (memcmp(md5, target_md5, 16) != 0)
        {
                OTA_DBG("verify md5 value failed !\n");
                return -1;
        }
        printf("verify md5 value success !\n");

        return 0;
}
#endif

int hc_ota_upgrade(int from, char *path, int start, int end, void (*FUNC)(int))
{
        int ret = 0;
        pgress.start = start;
        pgress.end = end;
        pgress.FUNC = FUNC;
        //OTA_DBG("FUNC:%s----------\n", FUNC);

        if (from == 1)
        {
                if (strncmp(path, "http", 4) != 0)
                {
                        ret = -1;
                        OTA_ERR("path not start with http : %s\n", path);
                        goto ota_upgrade_out;
                }
                ret = update_from_hc_network(path);
        }
        else if (from == 0)
        {
                if (strncmp(path, "/data", 5) != 0)
                {
                        ret = -1;
                        OTA_ERR("path not start with /data : %s\n", path);
                        goto ota_upgrade_out;
                }
                ret = update_from_flash(path);
        }

ota_upgrade_out:
        return ret;
}

int hc_ota_task(char *para)
{
	ota_status_t ret = OTA_STATUS_ERROR;
	char *para_malloc = NULL;

	if (para == NULL) {
		clear_para_after_end();

		para_malloc = malloc(MAX_HC_OTA_PARA_SIZE);
                if (para_malloc == NULL) {
                        OTA_ERR("ota task malloc fail\n");
                        ret = -1;
                        goto ota_err;
                }

                ret = load_para(para_malloc);
                if (ret)
                        goto ota_err;
                OTA_DBG("load para success:%s\n", para);
                para = para_malloc;
	} else {
		save_para(para);
	}

	if (strncmp(para, "http", 4) == 0) {
		ret = update_from_hc_network(para);
	} else if (strncmp(para, "/data", 5) == 0) {
		ret = update_from_flash(para);
#ifdef CONFIG_COMPONENTS_AW_UPGRADE_MD5SUM
		int verify_md5_for_flash(char *path);
                if (ret == 0)
                {
                        ret = verify_md5_for_flash(para);
                }
#endif
	} else {
		OTA_ERR("para not start with http or /data: %s\n", para);
		goto ota_err;
	}

ota_err:
        if (para_malloc)
                free(para_malloc);

	return ret;
}

static int cmd_hcota(int argc, char **argv)
{
	int ret = 0;

	if (1 == argc) {
		OTA_ERR("missing valid url or path\n");
		ota_help();
		ret = -1;
	} else {
		ret = hc_ota_task(argv[1]);
	}

	return ret;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_hcota, hcota, Tina RTOS aw hc-ota demo);

