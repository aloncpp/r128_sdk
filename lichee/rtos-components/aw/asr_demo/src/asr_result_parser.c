#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hal_sem.h>
#include <hal_mem.h>
#include <hal_time.h>
#include "asr_result_parser.h"

static inline int asr_result_is_ok(struct asr_result_t *hdl)
{
	cJSON *cjson = hdl->cjson;
	int ok = 0;
	cJSON *cjson_tmp;

	if (!cJSON_HasObjectItem(cjson, "status")) {
		printf("status not found!\n");
		return 0;
	}

	cjson_tmp = cJSON_GetObjectItem(cjson, "status");
	if (cjson_tmp) {
		if(cjson_tmp->type == cJSON_String && !strcmp(cjson_tmp->valuestring, "ok")){
			ok = 1;
		}
		//cJSON_Delete(cjson_tmp);
	}

	return ok;
}

static inline int cjson_get_string(cJSON *root, const char **item, unsigned int depth, char *string, unsigned int size)
{
	cJSON *cjson;
	int ret = -1;

	if (!cJSON_HasObjectItem(root, item[0])) {
		printf("%s not found!\n", item[0]);
		return -1;
	}

	cjson = cJSON_GetObjectItem(root, item[0]);
	if (cjson) {
		if (depth <= 1) {
			if (cjson->type == cJSON_String) {
				unsigned int len = strlen(cjson->valuestring);
				if(len >= size) {
					printf("%s len(%u) >= size(%u)!\n", item[0], len, size);
					ret = -1;
				} else {
					memcpy(string, cjson->valuestring, len + 1);
					ret = len;
				}
			} else {
				printf("%s is not string!\n", item[0]);
				ret = -1;
			}
		} else {
			ret = cjson_get_string(cjson, &item[1], depth - 1, string, size);
		}
		//cJSON_Delete(cjson);
	} else {
		printf("can not get %s!\n", item[0]);
	}

	return ret;
}

static inline int asr_result_get_tts_link(struct asr_result_t *hdl)
{
	char buffer[STRING_BUFFER_SIZE];
	const char *item[] = {"nlpResult", "ttsLink"};
	int len = cjson_get_string(hdl->cjson, item, 2, buffer, STRING_BUFFER_SIZE);

	if (len <= 0) {
		printf("ttsLink string too large!\n");
		goto err;
	}

	len += 1;
	hdl->tts_link = hal_malloc(len);
	if (!hdl->tts_link) {
		printf("no memory! need %ubyte!\n", len);
		goto err;
	}

	memcpy(hdl->tts_link, buffer, len);

	return 1;
err:
	return -1;
}

static inline int asr_result_get_res_list(struct asr_result_t *hdl)
{
	char buffer[STRING_BUFFER_SIZE];
	const char *item[] = {"nlpResult", "resource"};
	int cnt = 0;
	int len = cjson_get_string(hdl->cjson, item, 2, buffer, STRING_BUFFER_SIZE);

	hdl->res_num = 0;
	if (len > 0) {
		len += 1;
		hdl->res_list[cnt] = hal_malloc(len);
		if (!hdl->res_list[cnt]) {
			printf("no memory! need %ubyte!\n", len);
			goto err;
		}

		memcpy(hdl->res_list[cnt], buffer, len);
		hdl->res_num++;
		cnt++;
	}

	return hdl->res_num ? hdl->res_num : -1;
err:
	return -1;
}

static inline int asr_result_get_type(struct asr_result_t *hdl)
{
	char buffer[STRING_BUFFER_SIZE];
	const char *item[] = {"nlpResult", "intent", "name"};

	if (0 > cjson_get_string(hdl->cjson, item, 3, buffer, STRING_BUFFER_SIZE)) {
		printf("%s:%d\n", __func__, __LINE__);
		goto err;
	}

	if (!strcmp(buffer, "music")) {
		hdl->type = ASR_RESULT_MUSIC;
	} else if(!strcmp(buffer, "weather")) {
		hdl->type = ASR_RESULT_WEATHER;
	} else if(!strcmp(buffer, "eappliances")) {
		hdl->type = ASR_RESULT_EAPPLIANCES;
	} else if(!strcmp(buffer, "local")) {
		hdl->type = ASR_RESULT_LOACL;
	} else if(!strcmp(buffer, "unknown")) {
		hdl->type = ASR_RESULT_UNKNOWN;
	} else {
		hdl->type = -1;
		printf("unknown type: %s\n", buffer);
		goto err;
	}

	return 0;
err:
	return -1;
}

void asr_result_destroy(struct asr_result_t *hdl)
{
	printf("%s:%d\n", __func__, __LINE__);

	if (hdl) {
		if (hdl->cjson) {
			cJSON_Delete(hdl->cjson);
			hdl->cjson = NULL;
		}
		if(hdl->tts_link) {
			hal_free(hdl->tts_link);
			hdl->tts_link = NULL;
		}
		for (int i = 0; i < hdl->res_num; i++) {
			hal_free(hdl->res_list[i]);
			hdl->res_list[i] = NULL;
		}
		hal_free(hdl);
	}
}

struct asr_result_t *asr_result_create(const char *value, int id)
{
	printf("%s:%d\n", __func__, __LINE__);
	char buffer[STRING_BUFFER_SIZE];

	struct asr_result_t *hdl = hal_malloc(sizeof(*hdl));
	if (!hdl) {
		goto err;
	}
	memset(hdl, 0, sizeof(*hdl));

	hdl->cjson = cJSON_Parse(value);
	if (!hdl->cjson) {
		printf("%s:%d\n", __func__, __LINE__);
		goto err;
	}

	hdl->status = asr_result_is_ok(hdl);
	if (!hdl->status) {
		printf("%s:%d\n", __func__, __LINE__);
		goto err;
	}

	if (0 > asr_result_get_type(hdl)) {
		printf("get type failed!\n");
	}

	switch(hdl->type){
		case ASR_RESULT_MUSIC:
			if (0 > asr_result_get_res_list(hdl)) {
				printf("get res list failed!\n");
			}
		case ASR_RESULT_UNKNOWN:
		case ASR_RESULT_LOACL:
		case ASR_RESULT_WEATHER:
		case ASR_RESULT_EAPPLIANCES:
			if (0 > asr_result_get_tts_link(hdl)) {
				printf("get tts link failed!\n");
				goto err;
			}
			break;
		default:
			printf("unknown type: %d\n", hdl->type);
			break;
	}

	hdl->id = id;
	printf("%s:%d\n", __func__, __LINE__);
	return hdl;
err:
	asr_result_destroy(hdl);
	return NULL;
}

/*
{
	"asrResult": {
		"_conf": 10000,
		"_decoder": "first-path1",
		"text": "珠海 天气 如何"
	},
	"nlpResult": {
		"entities": [
			{
				"entity": "location",
				"value": "珠海"
			},
			{
				"entity": "weather",
				"value": "天气"
			}
		],
		"intent": {
			"confidence": 0.999481,
			"name": "weather"
		},
		"resText": "珠海今天，12月16日的天气，白天中雨，夜晚阴，最高温度15度，最低温度9度。",
		"ttsLink": "http://your.server.com:8000/xxxxxxx"
	},
	"reason": "",
	"status": "ok"
}
{
	"asrResult": {
		"_conf": 9789,
		"_decoder": "first-path1",
		"text": "我想 听 张 学友 的 吻别"
	},
	"nlpResult": {
		"entities": [
			{
				"entity": "artist",
				"value": "张学友"
			},
			{
				"entity": "song",
				"value": "吻别"
			}
		],
		"intent": {
			"confidence": 1.0,
			"name": "music"
		},
		"resText": "马上播放张学友的吻别",
		"resource": "http://your.server.com:8000/xxxxxxx",
		"ttsLink": "http://your.server.com:8000/xxxxxxx"
	},
	"reason": "",
	"status": "ok"
}
*/
