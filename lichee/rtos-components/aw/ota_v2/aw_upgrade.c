#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <awlog.h>
//#include <errno.h>
#include <console.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <env.h>
#include <aw_upgrade.h>
#include "ota_debug.h"

static int upgrade_slice_sub_action(char *source_file, char *source_buffer, char *dest_file, int offset, int slice_size, int action)
{
	int source = -1, dest = -1;
	char *in = NULL, *verify = NULL;
	int finish = 0, avail_in = 0, total_in = 0, ret = 0;

	struct stat statbuff;

	int upgrade_source = 0;
	if (source_buffer) {
		upgrade_source = UPGRADE_FROM_BUFFER;
		OTA_DBG("upgrade from buffer, action:%d\n", action);
	} else if (source_file) {
		upgrade_source = UPGRADE_FROM_FILE;
		OTA_DBG("upgrade from file, action:%d\n", action);
		if (-1 == stat(source_file, &statbuff)) {
			OTA_ERR("stat %s wrong\n", source_file);
			ret = -1;
			goto upgrade_slice_sub_action_out;
		} else {
			if (statbuff.st_size != slice_size) {
				OTA_ERR("size error, file size is %d, not %d\n", statbuff.st_size, slice_size);
				ret = -1;
				goto upgrade_slice_sub_action_out;
			}
		}
	} else {
		OTA_ERR("error: both source_file and source_buffer are NULL\n");
		ret = -1;
		goto upgrade_slice_sub_action_out;
	}

	in = malloc(CHUNK);
	if (in == NULL) {
		OTA_ERR("malloc %x for in buffer fail\n", CHUNK);
		ret = -1;
		goto upgrade_slice_sub_action_out;
	}

	verify = malloc(CHUNK);
	if (verify == NULL) {
		OTA_ERR("malloc %x for verify buffer fail\n", CHUNK);
		ret = -1;
		goto upgrade_slice_sub_action_out;
	}

	if (action == UPGRADE_SLICE_ACTION_WRITE) {
		if (upgrade_source == UPGRADE_FROM_FILE)
			source = open(source_file, O_RDONLY);
		dest = open(dest_file, O_RDWR);
	} else if (action == UPGRADE_SLICE_ACTION_VERIFY) {
		if (upgrade_source == UPGRADE_FROM_FILE)
			source = open(source_file, O_RDONLY);
		dest = open(dest_file, O_RDONLY);
	} else {
		OTA_ERR("unknow action: %d\n", action);
		ret = -1;
		goto upgrade_slice_sub_action_out;
	}

	if (upgrade_source == UPGRADE_FROM_FILE) {
		if (source < 0) {
			OTA_ERR("open %s fail\n", source_file);
			ret = -1;
			goto upgrade_slice_sub_action_out;
		}
	}

	if (dest < 0) {
		OTA_ERR("open %s fail\n", dest_file);
		ret = -1;
		goto upgrade_slice_sub_action_out;
	}

	lseek(dest, offset, SEEK_SET);

	do {
		memset(in, 0, CHUNK);
		memset(verify, 0, CHUNK);

		if (upgrade_source == UPGRADE_FROM_FILE) {
			avail_in = read(source, in, CHUNK);
			if (avail_in == -1) {
				OTA_ERR("read source fail\n");
				ret = -1;
				break;
			}
			if (avail_in == 0) {
				OTA_ERR("read reach source end\n");
				ret = -1;
				break;
			}
		} else if (upgrade_source == UPGRADE_FROM_BUFFER) {
			if (total_in + CHUNK > slice_size)
				avail_in = slice_size - total_in;
			else
				avail_in = CHUNK;
			memcpy(in, source_buffer + total_in, avail_in);
		}

		/* check slice_size */
		total_in += avail_in;
		if (total_in >= slice_size) {
			avail_in -= (total_in - slice_size);
			finish = 1;
		}

		if (action == UPGRADE_SLICE_ACTION_WRITE) {
			if (write(dest, in, avail_in) != avail_in) {
				OTA_ERR("write dest fail\n");
				ret = -1;
				break;
			}
		} else if (action == UPGRADE_SLICE_ACTION_VERIFY) {
			if (read(dest, verify, avail_in) != avail_in) {
				OTA_ERR("read dest fail\n");
				ret = -1;
				break;
			}

			if (memcmp(in, verify, avail_in) != 0) {
				OTA_ERR("memcmp fail\n");
				ret = -1;
				break;
			}
		}

	} while (finish != 1);

upgrade_slice_sub_action_out:

	if (source)
		close(source);
	if (dest)
		close(dest);

	if (in)
		free(in);
	if (verify)
		free(verify);

	return ret;
}

int upgrade_slice(char *source_file, char *source_buffer, char *dest_file, int offset, int slice_size)
{
	int try_count = UPGRADE_SLICE_RETRY + 1;
	int ret;

	if (source_buffer)
		OTA_DBG("upgrade slice from buffer to %s, offset %d, size %d\n", dest_file, offset, slice_size);
	else
		OTA_DBG("upgrade slice %s to %s, offset %d, size %d\n", source_file, dest_file, offset, slice_size);

	while (try_count--) {
		ret = upgrade_slice_sub_action(source_file, source_buffer, dest_file, offset, slice_size, UPGRADE_SLICE_ACTION_WRITE);
		if (ret) {
			OTA_ERR("upgrade slice, write fail\n");
			continue;
		}
		/* no cache , don't need to sync */
		/* sync(); */
		ret = upgrade_slice_sub_action(source_file, source_buffer, dest_file, offset, slice_size, UPGRADE_SLICE_ACTION_VERIFY);
		if (ret) {
			OTA_ERR("upgrade slice, verify fail\n");
			continue;
		}

		/* verify ok */
		break;
	}

	return ret;
}

/*int get_rtos_to_upgrade(char *target_rtos)
{
	char *now = NULL;
	int ret = 0;

	if (fw_env_open())
		return -1;

	now = fw_getenv("rtosAB_now");
	OTA_DBG("rtosAB_now = %s\n", now);
	if (strncmp(now, "A", 1) == 0)
		strcpy(target_rtos, "/dev/rtosB");
	else if (strncmp(now, "B", 1) == 0)
		strcpy(target_rtos, "/dev/rtosA");
	else
		ret = -1;

	fw_env_close();

	return ret;
}*/

int ota_upgrade_slice(uint8_t* target, uint8_t* buffer, uint32_t offset, uint32_t size, uint32_t flag)
{
	int ret;
	char *target_file;
	char target_rtos[16 + 5 + 1]; /* 5 is "/dev/" */

	OTA_DBG("file_name:%s, offset:%d, slice_size:%d, is_last_slice:%d\n",
		target, offset, size, flag);
	/*if (strcmp(target, "rtos") == 0) {
		get_rtos_to_upgrade(target_rtos);
		target_file = target_rtos;
	} else*/
	if (strncmp(target, "/", 1) == 0) {
		target_file = target;
	} else {
		OTA_ERR("invalid file_name %s\n", target);
		return -1;
	}
	//OTA_DBG("upgrade %s to %s\n", target, target_file);

	ret = upgrade_slice(DEFAULT_SOURCE_FILE, buffer, target_file, offset, size);

	return ret;
}

int aw_upgrade_start(uint32_t flag)
{
	char *now = NULL;
	int ret = 0;

	printf("to do\n");
#if 0
	if (fw_env_open())
		return -1;

	now = fw_getenv("rtosAB_now");
	OTA_DBG("%s, %d, rtosAB_now = %s\n", __func__, __LINE__, now);

	if (strncmp(now, "A", 1) == 0) {
		ret = fw_env_write("rtosAB_next", "A");
	} else if (strncmp(now, "B", 1) == 0) {
		ret = fw_env_write("rtosAB_next", "B");
	} else {
		OTA_ERR("invalid rtosAB_now\n");
		ret = -1;
	}

	if (ret)
		goto aw_upgrade_start_out;

	ret = fw_env_write("bootcount", "0");
	if (ret)
		goto aw_upgrade_start_out;

	ret = fw_env_write("upgrade_available", "0");
	if (ret)
		goto aw_upgrade_start_out;

	/* flush to flash */
	fw_env_flush();

aw_upgrade_start_out:

	fw_env_close();
#endif
	return ret;
}

int aw_upgrade_end(uint32_t flag)
{
	char *now = NULL;
	int ret = 0;

	printf("to do\n");
#if 0
	if (fw_env_open())
		return -1;

	now = fw_getenv("rtosAB_now");
	OTA_DBG("rtosAB_now = %s\n", now);

	if (strncmp(now, "A", 1) == 0) {
		ret = fw_env_write("rtosAB_next", "B");
	} else if (strncmp(now, "B", 1) == 0) {
		ret = fw_env_write("rtosAB_next", "A");
	} else {
		OTA_ERR("invalid rtosAB_now\n");
		ret = -1;
	}

	if (ret)
		goto aw_upgrade_end_out;

	ret = fw_env_write("bootcount", "0");
	if (ret)
		goto aw_upgrade_end_out;

	ret = fw_env_write("upgrade_available", "1");
	if (ret)
		goto aw_upgrade_end_out;

	/* flush to flash */
	fw_env_flush();

aw_upgrade_end_out:

	fw_env_close();
#endif

	return ret;
}

static int aw_upgrade_cmd(int argc, char **argv)
{
	struct tt_struct *tt;
	char *file_name = NULL;
	uint32_t offset;
	uint32_t slice_size;
	uint32_t is_last_slice;
	char *buffer = NULL;
	char *reserve = NULL;
	struct stat statbuff;
	char *source_file = NULL;
	char *source_buffer = NULL;
	uint32_t source_file_size;
	int source = -1;
	int ret;

	if (argc == 1) {
		/*
		 * for test upgrade end
		 * aw_upgrade
		 */
		ret = aw_upgrade_end(0);
	} else if (argc == 5) {
		/*
		 * for test upgrade from file
		 * aw_upgrade /dev/xxx 0 1097728 1
		 */
		file_name = argv[1];
		offset = atoi(argv[2]);
		slice_size = atoi(argv[3]);
		is_last_slice = atoi(argv[4]);
		buffer = NULL;
		ret = ota_upgrade_slice(file_name,
					buffer, offset, slice_size, is_last_slice);
	} else if (argc == 6) {
		/*
		 * for test upgrade from buffer
		 * aw_upgrade /dev/xxx /data/update/update_1.bin       0 512000 0
		 * aw_upgrade /dev/xxx /data/update/update_2.bin  512000 512000 0
		 * aw_upgrade /dev/xxx /data/update/update_3.bin 1240000  73728 1
		 */
		file_name = argv[1];
		source_file = argv[2];
		offset = atoi(argv[3]);
		slice_size = atoi(argv[4]);
		is_last_slice = atoi(argv[5]);
		buffer = NULL;

		source = open(source_file, O_RDONLY);
		if (source < 0) {
			OTA_ERR("open %s fail\n", source_file);
			ret = -1;
			goto aw_upgrade_cmd_out;
		}

		if (-1 == stat(source_file, &statbuff)) {
			OTA_ERR("stat %s wrong\n", source_file);
			ret = -1;
			goto aw_upgrade_cmd_out;
		} else {
			source_file_size = statbuff.st_size;
			OTA_ERR("source file size:%d\n", source_file_size);
			source_buffer = malloc(source_file_size);
			if (source_buffer == NULL) {
				OTA_ERR("malloc %d for source buffer fail\n", source_file_size);
				ret = -1;
				goto aw_upgrade_cmd_out;
			}
			if (source_file_size != read(source, source_buffer, source_file_size)) {
				OTA_ERR("read source file fail\n");
				ret = -1;
				goto aw_upgrade_cmd_out;
			}
			buffer = source_buffer;
		}

		if (slice_size > source_file_size) {
			OTA_ERR("slice_size %d large than source_file_size:%d\n", slice_size, source_file_size);
			ret = -1;
			goto aw_upgrade_cmd_out;
		}

		ret = ota_upgrade_slice(file_name, buffer, offset, slice_size, is_last_slice);
	} else {
		OTA_ERR("not support agrc = %d\n", argc);
		ret = -1;
		goto aw_upgrade_cmd_out;
	}

aw_upgrade_cmd_out:

	if (source >= 0)
		close(source);

	if (source_buffer)
		free(source_buffer);

	return ret;
}
FINSH_FUNCTION_EXPORT_CMD(aw_upgrade_cmd, aw_upgrade, Tina RTOS aw_upgrade);
