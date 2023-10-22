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
#include <aw_types.h>
#include "ota_debug.h"
#include "aw_ota_core.h"

#ifdef CONFIG_COMPONENTS_AW_OTA_V2_NETWORK
#include "aw_ota_http.h"
int download_flag;
#endif

extern int cmd_reboot(int argc, char ** argv);

int ota_get_init_env(char *name, char *value)
{
	int ret = -1;
	char *tmp = NULL;

	if (name == NULL) {
		OTA_ERR("please input correct env name.\n");
		return ret;
	}

	if (fw_env_open())
		return ret;

	tmp = fw_getenv(name);
	if (tmp == NULL)
		goto ota_get_init_env_out;
	OTA_DBG("name = %s, val = %s\n", name, tmp);

	/* test
	for (; *tmp != '\0'; tmp++, value++) {
		*value = *tmp;
	}
	*value = '\0';*/

	memcpy(value, tmp, strlen(tmp));
	ret = 0;

ota_get_init_env_out:
	fw_env_close();
	return ret;
}

int ota_set_init_env(char *mode, char *param)
{
	int ret = -1;
	if (mode == NULL || param == NULL) {
		OTA_ERR("input mode or param NULL! please input correct value.\n");
		goto ota_set_init_env_out;
	}

	OTA_DBG("init env, ota_mode:%s, ota_param:%s\n", mode, param);
	if (fw_env_open())
		return -1;

	ret = fw_env_write("ota_mode", mode);
	if (ret)
		goto ota_set_init_env_out;

	ret = fw_env_write("ota_param", param);
	if (ret)
		goto ota_set_init_env_out;

	/* flush to flash */
	fw_env_flush();
	OTA_DBG("init env ok!\n");

ota_set_init_env_out:
	fw_env_close();
	return ret;
}

int ota_check_init_env(void)
{
	char *env_value = NULL;
	int ret = -1;

	if (fw_env_open())
		return ret;

	env_value = fw_getenv("ota_mode");
	if (env_value == NULL)
		goto ota_check_init_env_out;
	printf("ota_mode = %s\n", env_value);


	env_value = fw_getenv("ota_param");
	if (env_value == NULL)
		goto ota_check_init_env_out;
	printf("ota_param = %s\n", env_value);

	ret = 0;
	OTA_DBG("check init env ok!\n");

ota_check_init_env_out:
	fw_env_close();
	return ret;
}

static int dir_name(const char *full_path, char *dir_path)
{
	OTA_DBG("full_path:%s \n", full_path);
	const char *end = strrchr(full_path, '/');

	if(!end)
		return -1;

	unsigned int len = (unsigned int)(end - full_path + 1);
	if(len == 0)
		return -1;

	OTA_DBG("end:%s, len:%u\n", end, len);
	memcpy(dir_path, full_path, len);

	return 0;
}

//get ota_param dir path. eg: if ota_param is /data/xxx/file, get /data/xxx/
static int get_param_dir_path_from_env(char *dir_val)
{
	int ret = -1;
	char tmp_path[OTA_FILE_MAX_LEN] = {0};
	ret = ota_get_init_env("ota_param", tmp_path);

	if (ret) {
		OTA_ERR("get ota_param from env failed!\n");
		return -1;
	}
	OTA_DBG("tmp_path:%s\n", tmp_path);

	ret = dir_name(tmp_path, dir_val);
	if (ret) {
		OTA_ERR("get ota_param dir failed!\n");
		return -1;
	}

	return 0;
}

static int get_image_full_path_from_env(char *path, char *name)
{
	int ret = -1;

	if (get_param_dir_path_from_env(path)) {
		OTA_ERR("get image-file:%s dir failed!\n", name);
		return -1;
	}

	strcat(path, name);
	return 0;
}

static int get_step_file_path_from_env(char *path)
{
	int ret = -1;
	if (get_param_dir_path_from_env(path)) {
		OTA_ERR("get ota-step dir failed!\n");
		return -1;
	}

	strcat(path, "ota-step");
	return 0;
}


#ifdef CONFIG_COMPONENTS_AW_OTA_V2_NETWORK
static int get_step_file_path_from_http(char *path)
{
	int ret = -1;

	//1. get path: http://xxx/ota-step
	if (get_step_file_path_from_env(path)) {
		OTA_ERR("get ota-step url failed!\n");
		return -1;
	}

	//2. download http://xxx/ota-step to /data, maybe /tmp is better
	OTA_DBG("download_flag:%d\n", download_flag);
	if (download_flag == 0) {
		ret = ota_update_http_download_file(path, "/data/");
		if (ret) {
			OTA_ERR("download ota-step from:%s failed!\n", path);
			return -1;
		}
		download_flag = 1;
	}

	//3. path is /data/ota-step
	strcpy(path, "/data/ota-step");
	return 0;
}
#endif

int ota_get_step_file_path(ota_protocol_t type, char *path)
{
	int ret = -1;
	//if want to support other protocol, add more case
	switch(type) {
	case OTA_PROTOCOL_FILE:
		ret = get_step_file_path_from_env(path);
		if (ret) {
			goto ota_get_step_file_path_failed;
		}
		break;

#ifdef CONFIG_COMPONENTS_AW_OTA_V2_NETWORK
	case OTA_PROTOCOL_HTTP:
		ret = get_step_file_path_from_http(path);
		if (ret)  {
			goto ota_get_step_file_path_failed;
		}
		break;
#endif

	default:
		OTA_ERR("not support ota_protocol_t!\n");
		goto ota_get_step_file_path_failed;
	}

	return 0;

ota_get_step_file_path_failed:
	return -1;
}

ota_protocol_t ota_get_protocol(void)
{
	int ret = -1;
	char param[OTA_FILE_MAX_LEN] = {0};

	ret = ota_get_init_env("ota_param", param);
	if (ret) {
		return OTA_PROTOCOL_FILE;
	}
	OTA_DBG("protocol param:%s\n", param);

	//if want to support other protocol, add else if
	if (strncmp(param, "http", 4) == 0) {
		return OTA_PROTOCOL_HTTP;
	} else {
		return OTA_PROTOCOL_FILE;
	}
}

/*
 * input:
 * fd: file descriptor
 * size: limit of dst space
 * output:
 * dst: string of one line
 * return: >=0(success, lenth of one line)/-1(failed)
 */
static int read_one_line(int fd, char *dst, int size){
	int ret = -1;
	int len = 0;
	while(1){
		char ch;
		ret = read(fd, &ch, 1);
		if(ret!=1){
			if(len!=0){
				ret = len;
				goto exit;
			}
			else{
				ret = -1;
				goto exit;
			}
		}
		if(ch==';'){
			while(1){
				ret = read(fd, &ch, 1);
				if(ret!=1 || ch=='\n'){
					ret = 0;
					goto exit;
				}
			}
		}
		if(ch=='\n'){
			dst[len] = 0;
			ret = len;
			goto exit;
		}
		if(ch<0x80 && ch!='\r'){
			dst[len] = ch;
			len++;
		}
	}
exit:
	return ret;
}

static int check_wanted_buf(char *line, char *wanted_buf)
{
	char *p = strstr(line, wanted_buf);
	if (p == NULL)
		return -1;

	return 0;
}

static int parse_one_line_value(char *line, char *val)
{
	int ret = -1;
	char *end = NULL;
	unsigned int len = 0;
	if (!line) {
		return 0;
	}

	//line should be <key="val">, get val
	line = strchr(line, '\"');
	if (!line) {
		goto exit;
	}
	line++;

	end = strchr(line, '\"');
	if (!end) {
		goto exit;
	}

	len = (unsigned int)(end - line);
	if (0 == len) {
		goto exit;
	}

	memcpy(val, line, len);
	val[len] = 0;
	ret = 0;
exit:
	return ret;
}

static int parse_one_line_env(char *line, char *name, char *val)
{
	int ret = -1;
	char *end = NULL, *p = NULL;
	unsigned int len = 0;
	if(!line)
		return 0;
	//line should be <env:name=val>
	p = strstr(line, "env:");
	if (p == NULL) {
		OTA_ERR("not env line:%s\n", line);
		goto exit;
	}

	p = p + strlen("env:");
	OTA_DBG("env name and value-> %s\n", p);
	//get name
	end = strchr(line, '=');
	if (!end) {
		goto exit;
	}

	//len = (unsigned int)(end - line);
	len = (unsigned int)(end - p);
	if (0 == len) {
		goto exit;
	}
	memcpy(name, p, len);
	name[len] = 0;

	//get val
	end++;
	len = strlen(end);
	if (len == 0) {
		return 0;
	}
	memcpy(val, end, len);
	val[len] = 0;
	ret = 0;
exit:
	return ret;
}

int ota_check_step_file(char *path)
{
	int ret = -1, want_flag = 0;
	int len;
	char buf[128] = {0}, wanted_buf[64] = {0};

	if(access(path, F_OK | R_OK )!=0) {
		//path not exist or can not be read
		goto exit;
	}

	int fd = open(path, O_RDONLY);
	if (fd<0) {
		OTA_ERR("open %s failed!\n", path);
		goto exit;
	}

	ret = -1;
	while (1) {
		memset(buf, 0, 128);

		len = read_one_line(fd, buf, 128);
		if (len<0) {
			OTA_DBG("read_one_line EOF!\n");
			ret = 0;
			break;
		} else if (len==0) {
			OTA_DBG("read_one_line empty!\n");
			continue;
		}

		OTA_DBG("line buf:%s\n", buf);
		if (want_flag == 1) {
			if (check_wanted_buf(buf, wanted_buf)) {
				OTA_ERR("next line should have:%s\n", wanted_buf);
				break;
			}
			want_flag = 0;
			continue;
		}

		if (strstr(buf, "[step")) {
			continue;
		} else if (strstr(buf, "image-file")) {
			//image-file next line should be device
			want_flag = 1;
			memset(wanted_buf, 0, 64);
			memcpy(wanted_buf, "device", strlen("device"));
			continue;
		} else if (strstr(buf, "env:")) {
			continue;
		} else {
			OTA_ERR("unknown line:%s\n", buf);
			break;
		}
	}
	close(fd);

exit:
	return ret;
}

int ota_update_from_file(char *from_name, char *dev_path)
{
	int source;
	struct stat statbuff;
	char source_file[OTA_FILE_MAX_LEN] = {0};
	uint8_t* source_buffer;
	int slice_size = 4*1024;
	int source_file_size, update_size, offset = 0, is_last_slice = 0, ret = 0;

	if (get_image_full_path_from_env(source_file, from_name)) {
		ret = -1;
		goto update_from_file_out;
	}
	printf("image file path:%s, device:%s\n", source_file, dev_path);

	if (stat(dev_path, &statbuff) < 0) {
		printf("stat device %s wrong\n", dev_path);
		return -1;
	}

	if (stat(source_file, &statbuff) < 0) {
		printf("stat file %s wrong\n", source_file);
		return -1;
	}

	source = open(source_file, O_RDONLY);
	if (source < 0) {
		printf("open %s fail\n", source_file);
		ret = -1;
		goto update_from_file_out;
	}

	source_buffer = malloc(slice_size);
	if (source_buffer == NULL) {
		printf("malloc %d for source buffer fail\n", slice_size);
		ret = -1;
		goto update_from_file_out;
	}

	source_file_size = statbuff.st_size;
	printf("source file size:%d slice_size:%d\n", source_file_size, slice_size);

	while (offset < source_file_size) {
		memset(source_buffer, 0, slice_size);
		update_size = min(source_file_size - offset, slice_size);
		ret = read(source, source_buffer, update_size);
		if (ret != update_size) {
			printf("read source file fail, now offset:%d ret:%d update_size:%d source_file_size:%d\n",
					offset, ret, update_size, source_file_size);
			ret = -1;
			goto update_from_file_out;
		}

		if (offset + update_size >= source_file_size)
			is_last_slice = 1;
		else
			is_last_slice = 0;
		ret = ota_upgrade_slice(dev_path, source_buffer, offset, update_size, is_last_slice);

		if (ret)
			goto update_from_file_out;
		offset += update_size;
		//otaprogressBar(offset, source_file_size);
	}
	printf("last slice done, source_file_size:%d offset:%d ret:%d\n", source_file_size, offset, ret);

update_from_file_out:
	if (source >= 0) {
		close(source);
	}

	if (source_buffer) {
		free(source_buffer);
	}

	return ret;
}

#ifdef CONFIG_COMPONENTS_AW_OTA_V2_NETWORK
int ota_update_from_http(char *from_name, char *dev_path)
{
	int ret;
	struct stat statbuff;
	char source_file[OTA_FILE_MAX_LEN] = {0};

	if (get_image_full_path_from_env(source_file, from_name)) {
		return -1;
	}
	printf("image file path:%s, device:%s\n", source_file, dev_path);

	if (stat(dev_path, &statbuff) < 0) {
		printf("stat device %s wrong\n", dev_path);
		return -1;
	}

	ret = ota_update_http_update_device(source_file, dev_path);
	if (ret) {
		OTA_ERR("update from http failed!\n");
	}

	return ret;
}
#endif

int ota_update_device(ota_protocol_t type, char *from_name, char *dev_name)
{
	int ret = -1;

	if (from_name == NULL || dev_name == NULL) {
		goto exit;
	}

	//if want to support other protocol, add more case
	switch(type) {
	case OTA_PROTOCOL_FILE:
		ret = ota_update_from_file(from_name, dev_name);
		if (ret) {
			goto exit;
		}
		break;

#ifdef CONFIG_COMPONENTS_AW_OTA_V2_NETWORK
	case OTA_PROTOCOL_HTTP:
		ret = ota_update_from_http(from_name, dev_name);
		if (ret) {
			goto exit;
		}
		break;
#endif
	}

	return 0;

exit:
	return -1;
}

int ota_set_all_env_by_once(int file, char *line)
{
	OTA_DBG("=== %s ===\n", __func__);
	int ret = -1, fd = file, len;
	char buf[128] = {0}, name[32] = {0}, val[96] = {0};
	char *end = NULL;
	memcpy(buf, line, strlen(line));

	if (fw_env_open()) {
		OTA_ERR("open env failed!\n");
		return -1;
	}

	if (parse_one_line_env(buf, name, val)) {
		goto exit;
	}
	if (fw_env_write(name, val)) {
		goto exit;
	}

	while (1) {
		memset(buf, 0, 128);
		memset(name, 0, 32);
		memset(val, 0, 64);

		len = read_one_line(fd, buf, 128);
		if (len<0) {
			OTA_DBG("read_one_line EOF!\n");
			break;
		} else if (len==0) {
			OTA_DBG("read_one_line empty!\n");
			continue;
		}

		OTA_DBG("line buf:%s\n", buf);

		if (strstr(buf, "ota_next=reboot")) {
			/* flush to flash */
			fw_env_flush();
			cmd_reboot(1, NULL);//reboot
		}
		if (parse_one_line_env(buf, name, val)) {
			goto exit;
		}
		printf("name:%s, len:%d; val:%s, len:%d\n", name, strlen(name), val, strlen(val));
		if (fw_env_write(name, val)) {
			goto exit;
		}
	}

	/* flush to flash */
	fw_env_flush();
	ret = 0;
exit:
	fw_env_close();
	return ret;
}

int ota_update(void)
{
	int ret = -1, want_image_file = 0, want_device = 0;
	int len;
	char buf[128] = {0}, wanted_buf[64] = {0};
	char from_name[64] = {0}, dev_name[64] = {0};
	char mode[64] = {0};
	char path[OTA_FILE_MAX_LEN] = {0};

	ota_protocol_t type = ota_get_protocol();
	OTA_DBG("ota_protocol :%d\n", type);

	//get ota_step file path
	ret = ota_get_step_file_path(type, path);
	if (ret) {
		return -1;
	}
	OTA_DBG("ota-step file path:%s\n", path);

	ret = ota_get_init_env("ota_mode", mode);
	if (ret) {
		OTA_ERR("get ota_mode file from env failed!\n");
		goto exit;
	}
	snprintf(wanted_buf, strlen(mode) + 3, "[%s]\n", mode);
	OTA_DBG("wanted ota-mode buf:%s\n", wanted_buf);

	if(access(path, F_OK | R_OK )!=0) {
		//path not exist or can not be read
		goto exit;
	}

	int fd = open(path, O_RDONLY);
	if (fd<0) {
		OTA_ERR("open %s failed!\n", path);
		goto exit;
	}

	ret = -1;
	//get the line which have ota-mode buf
	while (1) {
		memset(buf, 0, 128);

		len = read_one_line(fd, buf, 128);
		if (len<0) {
			OTA_DBG("read_one_line EOF and can't fine ota-mode:%s\n", wanted_buf);
			goto exit;
		} else if (len==0) {
			OTA_DBG("read_one_line empty!\n");
			continue;
		}

		OTA_DBG("line buf:%s\n", buf);
		if (check_wanted_buf(buf, wanted_buf) == 0) {
			//if find ota-mode in this line,break and next line should be image-file
			want_image_file = 1;
			memset(wanted_buf, 0, 64);
			memcpy(wanted_buf, "image-file", strlen("image-file"));
			break;
		}
	}
	while (1) {
		memset(buf, 0, 128);

		len = read_one_line(fd, buf, 128);
		if (len<0) {
			OTA_DBG("read_one_line EOF!\n");
			ret = 0;
			break;
		} else if (len==0) {
			OTA_DBG("read_one_line empty!\n");
			continue;
		}

		OTA_DBG("line buf:%s\n", buf);
		if (want_image_file == 1) { //only first time will go here, [stepx] next line should be image-file
			if (check_wanted_buf(buf, wanted_buf)) {
				OTA_ERR("next line should have:image-file, but buf is:%s\n", buf);
				break;
			}
			want_image_file = 0;
		}

		if (want_device == 1) {
			if (check_wanted_buf(buf, wanted_buf)) {
				OTA_ERR("next line should have:device, but buf is:%s\n", wanted_buf);
				break;
			}
			want_device = 0;

			memset(dev_name, 0, 64);
			if (parse_one_line_value(buf, dev_name)) {
				OTA_ERR("get device value failed.\n");
				break;
			}
			OTA_DBG("device value is:%s\n", dev_name);

			int count = 0; //if ota http update fail, record fail times
			while (count < 3) { //retry 3 times
				if (ota_update_device(type, from_name, dev_name) == 0) // begin to update /dev/xxx
					break;

				OTA_DBG("ota http update fail, retry to update.\n");
				count++;
			}

			if (count >= 3) {
				OTA_ERR("ota update device failed!\n");
				break; //exit ota_update
			}
			continue;
		}

		if (strstr(buf, "[step")) {
			OTA_ERR("already read next step:%s, return!\n", buf);
			break;
		} else if (strstr(buf, "image-file")) {
			//image-file next line should be device
			want_device = 1;
			memset(wanted_buf, 0, 64);
			memcpy(wanted_buf, "device", strlen("device"));

			memset(from_name, 0, 64);
			if (parse_one_line_value(buf, from_name)) {
				OTA_ERR("get image-file value failed.\n");
				break;
			}
			OTA_DBG("image-file vlaue is:%s\n", from_name);
			continue;
		} else if (strstr(buf, "env:")) {
			//set all env by once.env is the end of stepx, so break
			ota_set_all_env_by_once(fd, buf);
			ret = 0;
			break;
		} else {
			OTA_ERR("unknown line:%s\n", buf);
			break;
		}
	}
	close(fd);

exit:
	return ret;
}

int ota_init(void)
{
	char path[OTA_FILE_MAX_LEN] = {0};
	int ret = -1;


	//check_ota_env
	ret = ota_check_init_env();
	if (ret) {
		OTA_DBG("no init env return!\n");
		return -1;
	}

	ota_protocol_t type = ota_get_protocol();
	OTA_DBG("ota_protocol :%d\n", type);

	//get ota_step file path
	ret = ota_get_step_file_path(type, path);
	if (ret) {
		return -1;
	}
	OTA_DBG("ota-step file path:%s\n", path);

	//check ota step file
	ret = ota_check_step_file(path);
	if (ret) {
		OTA_ERR("check ota step failed!\n");
		return -1;
	}
	printf("===ota init succeed!===\n");
	return ret;
}

int ota_end(void)
{
	int ret = 0;

	if (fw_env_open())
		return -1;


	ret = fw_env_write("ota_mode", NULL);
	if (ret)
		goto aw_upgrade_end_out;

	ret = fw_env_write("ota_param", NULL);
	if (ret)
		goto aw_upgrade_end_out;

	/*ret = fw_env_write("bootcount", "0");
	if (ret)
		goto aw_upgrade_end_out;

	ret = fw_env_write("upgrade_available", "1");
	if (ret)
		goto aw_upgrade_end_out;*/

	/* flush to flash */
	fw_env_flush();
	OTA_DBG("set ota env end ok!\n");

aw_upgrade_end_out:

	fw_env_close();

	return ret;
}

static void show_help(void)
{
	printf("Usage: aw_ota_v2 [-i /xxx/file] [-d httpxxxx] [-e stepxxx]\n");
	printf("eg: aw_ota_v2 -i /data/ota-step -e step1\n");
	printf("eg: aw_ota_v2 -d http://192.168.11.183/ota-step -e step1\n");
}

static int aw_ota_v2_cmd(int argc, char **argv)
{
	int opts = 0, len = 0, ret = 0;
	char mode[64] = {0}, param[OTA_FILE_MAX_LEN] = {0};

	if (argc == 1) {
		show_help();
		return -1;
	}

	while ((opts = getopt(argc, argv, "i:d:e:")) != EOF) {
		printf("optopt:%c, optarg:%s\n", optopt, optarg);
		switch (opts) {
		case 'i': memcpy(param, optarg, strlen(optarg));break;
		case 'd': memcpy(param, optarg, strlen(optarg));break;
		case 'e': memcpy(mode, optarg, strlen(optarg));break;
		default:
			show_help();
			return -1;
		}
	}

	ret = ota_set_init_env(mode, param);
	if (ret)
		return -1;

	ret = ota_init();
	if (ret)
		return -1;

	ret = ota_update();
	if (ret) {
		printf("ota_update failed!\n");
		return -1;
	}
	//ret = ota_end();

	return ret;
}
FINSH_FUNCTION_EXPORT_CMD(aw_ota_v2_cmd, aw_ota_v2, Tina RTOS ota ver2);
