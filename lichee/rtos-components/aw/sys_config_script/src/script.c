#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "script.h"
#include <stdio.h>
#define SCRIPT_DEBUG 0

#if SCRIPT_DEBUG
#define SCRIPT_DEBUG_PRINTF(fmt, arg...)	printf("\e[35m[script]\e[0m"fmt"\n", ##arg)

static void ndump(uint8_t *buf, int count)
{
	char line[50];
	char *tmp = line;
	char *in  = (char *)buf;
	int i;
	SCRIPT_DEBUG_PRINTF("inbuf:%x", (uint32_t)buf);
	for (i = 0; i < count; i++) {
		snprintf(tmp, 5, "%02x ", *((uint8_t *)in));
		tmp += 3;
		if (((i % 16) == 15)) {
			SCRIPT_DEBUG_PRINTF("%s", line);
			tmp = line;
		}
		in++;
	}
	if (((i % 16) != 0)) {
		SCRIPT_DEBUG_PRINTF("%s", line);
		tmp = line;
	}
}
#define SCRIPT_DEBUG_DUMP(buf, cnt)	ndump(buf, cnt)
#else
#define SCRIPT_DEBUG_PRINTF(fmt, arg...)
#define SCRIPT_DEBUG_DUMP(buf, cnt)
#endif

#define SCRIPT_ERROR_PRINTF(fmt, arg...)	printf("\e[31m[script]\e[0m"fmt"\n", ##arg)
#define SCRIPT_INFO_PRINTF(fmt, arg...)	printf("\e[32m[script]\e[0m"fmt"\n", ##arg)

#define DATA_TYPE(x)	((x->pattern>>16) & 0xffff)
#define DATA_WORD(x)	((x.pattern>> 0) & 0xffff)

#define SCRIPT_GET_MAIN_KEY(x, i) (script_main_key_t *)(x->script_mod_buf + \
					(sizeof(script_head_t)) + i * sizeof(script_main_key_t))

#define SCRIPT_GET_SUB_KEY(x, m, i) (script_sub_key_t *)(x->script_mod_buf + \
					(m->offset<<2) + (i * sizeof(script_sub_key_t)))

#define SCRIPT
static int32_t _test_str_length(const char *str)
{
	int32_t length = 0;

	while (str[length++]) {
		if(length > 32) {
			length = 32;
			break;
		}
	}

	return length;
}

script_parser_t *script_parser_init(void *script_buf)
{
	script_parser_t *parser;
	script_head_t   *script_head;
	unsigned char *sys_config_pos;
	script_main_key_t *main_key;
	script_sub_key_t *sub_key;
	unsigned int sub_key_pattern;
	int main_key_index;
	int sub_key_size, sub_key_index;

	if (script_buf) {
		parser = malloc(sizeof(script_parser_t));
		if(NULL == parser)
			return NULL;

		parser->script_mod_buf = script_buf;

		script_head = (script_head_t *)(parser->script_mod_buf);

		if (script_head->main_key_count <= 0) {
			parser->script_mod_buf = NULL;
			free(parser);
			SCRIPT_ERROR_PRINTF("main_key_count %d error!", script_head->main_key_count);
			return NULL;
		}

		parser->script_main_key_count = script_head->main_key_count;

		sys_config_pos = (unsigned char *)parser->script_mod_buf;
		main_key_index = parser->script_main_key_count - 1;
		main_key = (script_main_key_t *)(sys_config_pos + sizeof(script_head_t));
		sub_key = (script_sub_key_t *)(sys_config_pos +
						(main_key[main_key_index].offset << 2));

		SCRIPT_DEBUG_PRINTF("sys_config_pos:%p", sys_config_pos);

		SCRIPT_DEBUG_PRINTF("main_key_index:%d", main_key_index);

		while (main_key[main_key_index].lenth <= 0) {
			main_key_index--;
			if (main_key_index < 0) {
				parser->script_mod_buf = NULL;
				free(parser);
				SCRIPT_ERROR_PRINTF("main_key error!");
				return NULL;
			}
		}

		sub_key_index = main_key[main_key_index].lenth - 1;
		sub_key_pattern = sub_key[sub_key_index].pattern;
		sub_key_size = (DATA_WORD(sub_key[sub_key_index]) << 2);

		SCRIPT_DEBUG_PRINTF("sub_key_index:%d", sub_key_index);
		SCRIPT_DEBUG_PRINTF("main_key[%d].sub_key[%d].sub_key_pattern:%08x",
						main_key_index, sub_key_index, sub_key_pattern);
		SCRIPT_DEBUG_PRINTF("sub_key_size:%d", sub_key_size);

		parser->script_mod_buf = script_buf;

		return parser;
	}

	return NULL;
}

int script_parser_exit(script_parser_t *parser)
{

	if(NULL == parser)
		return SCRIPT_PARSER_EMPTY_BUFFER;

	parser->script_mod_buf = NULL;
	parser->script_main_key_count = 0;
	parser->script_mod_buf_size = 0;

	free(parser);

	return SCRIPT_PARSER_OK;
}

int script_parser_fetch(script_parser_t *parser, const char *main_name,
				const char *sub_name, int value[], int count)
{
	char   main_bkname[32], sub_bkname[32];
	const char *main_char, *sub_char;
	script_main_key_t  *main_key = NULL;
	script_sub_key_t   *sub_key = NULL;
	int32_t    i, j;
	int32_t    pattern, word_count;

	if (NULL == parser)
		return SCRIPT_PARSER_EMPTY_BUFFER;

	//检查脚本buffer是否存在
	if (!parser->script_mod_buf)
		return SCRIPT_PARSER_EMPTY_BUFFER;
	//检查主键名称和子键名称是否为空
	if ((main_name == NULL) || (sub_name == NULL))
		return SCRIPT_PARSER_KEYNAME_NULL;
	//检查数据buffer是否为空
	if (value == NULL)
		return SCRIPT_PARSER_DATA_VALUE_NULL;
	//保存主键名称和子键名称，如果超过31字节则截取31字节
	main_char = main_name;
	if (_test_str_length(main_name) > 31) {
		memset(main_bkname, 0, 32);
		strncpy(main_bkname, main_name, 31);
		main_char = main_bkname;
	}

	sub_char = sub_name;
	if (_test_str_length(sub_name) > 31) {
		memset(sub_bkname, 0, 32);
		strncpy(sub_bkname, sub_name, 31);
		sub_char = sub_bkname;
	}

	for (i = 0; i < parser->script_main_key_count; i++) {
		main_key = SCRIPT_GET_MAIN_KEY(parser, i);
		if(strcmp(main_key->name, main_char))    //如果主键不匹配，寻找下一个主键
			continue;
		//主键匹配，寻找子键名称匹配
		for(j=0;j<main_key->lenth;j++) {
			sub_key = SCRIPT_GET_SUB_KEY(parser, main_key, j);
			if(strcmp(sub_key->name, sub_char))    //如果主键不匹配，寻找下一个主键
				continue;

			pattern    = (sub_key->pattern>>16) & 0xffff;             //获取数据的类型
			word_count = (sub_key->pattern>> 0) & 0xffff;             //获取所占用的word个数
			//取出数据
			switch (pattern) {
			case DATA_TYPE_SINGLE_WORD:                           //单word数据类型
				value[0] = *(int *)(parser->script_mod_buf + (sub_key->offset<<2));
				break;

			case DATA_TYPE_STRING:     							  //字符串数据类型
				if(count < word_count)
					word_count = count;
				memcpy((char *)value, parser->script_mod_buf + (sub_key->offset<<2), word_count << 2);
				break;

			case DATA_TYPE_MULTI_WORD:
				break;
			case DATA_TYPE_GPIO_WORD:							 //多word数据类型
				{
					user_gpio_set_t  *user_gpio_cfg = (user_gpio_set_t *)value;
					//发现是GPIO类型，检查是否足够存放用户数据
					if(sizeof(user_gpio_set_t) > (count<<2))
						return SCRIPT_PARSER_BUFFER_NOT_ENOUGH;
					strncpy(user_gpio_cfg->name, sub_char, strlen(sub_char) + 1);
					memcpy(&user_gpio_cfg->port, parser->script_mod_buf + (sub_key->offset<<2),
									sizeof(user_gpio_set_t) - 32);
					break;
				}
			}

			return SCRIPT_PARSER_OK;
		}
	}

	return SCRIPT_PARSER_KEY_NOT_FIND;
}

int script_parser_subkey_count(script_parser_t *parser, const char *main_name)
{
	char   main_bkname[32];
	const char *main_char;
	script_main_key_t  *main_key = NULL;
	int32_t    i;

	if (NULL == parser)
		return SCRIPT_PARSER_EMPTY_BUFFER;

	if (!parser->script_mod_buf)
		return SCRIPT_PARSER_EMPTY_BUFFER;

	if (main_name == NULL)
		return SCRIPT_PARSER_KEYNAME_NULL;
	//保存主键名称和子键名称，如果超过31字节则截取31字节
	main_char = main_name;
	if (_test_str_length(main_name) > 31) {
		memset(main_bkname, 0, 32);
		strncpy(main_bkname, main_name, 31);
		main_char = main_bkname;
	}

	for (i = 0; i < parser->script_main_key_count; i++) {
		main_key = SCRIPT_GET_MAIN_KEY(parser, i);

		if (strcmp(main_key->name, main_char))    //如果主键不匹配，寻找下一个主键
			continue;

		return main_key->lenth;    //返回当前主键下的子键个数
	}

	return -1;                     //-1 表示没有对应的主键
}

int script_parser_mainkey_count(script_parser_t *parser)
{

	if (NULL == parser)
		return SCRIPT_PARSER_EMPTY_BUFFER;

	if (!parser->script_mod_buf)
		return SCRIPT_PARSER_EMPTY_BUFFER;

	return 	parser->script_main_key_count;
}

int script_parser_mainkey_get_gpio_count(script_parser_t *parser,
				const char *main_name)
{
	char   main_bkname[32];
	const char *main_char;
	script_main_key_t  *main_key = NULL;
	script_sub_key_t   *sub_key = NULL;
	int32_t    i, j;
	int32_t    pattern, gpio_count = 0;

	if (NULL == parser)
		return SCRIPT_PARSER_EMPTY_BUFFER;

	if (!parser->script_mod_buf)
		return SCRIPT_PARSER_EMPTY_BUFFER;

	if (main_name == NULL)
		return SCRIPT_PARSER_KEYNAME_NULL;
	//保存主键名称和子键名称，如果超过31字节则截取31字节
	main_char = main_name;
	if (_test_str_length(main_name) > 31) {
		memset(main_bkname, 0, 32);
		strncpy(main_bkname, main_name, 31);
		main_char = main_bkname;
	}

	for (i = 0; i < parser->script_main_key_count; i++) {
		main_key = SCRIPT_GET_MAIN_KEY(parser, i);
		if (strcmp(main_key->name, main_char))    //如果主键不匹配，寻找下一个主键
			continue;
		//主键匹配，寻找子键名称匹配
		for (j = 0; j < main_key->lenth; j++) {
			sub_key = SCRIPT_GET_SUB_KEY(parser, main_key, j);

			pattern    = DATA_TYPE(sub_key);             //获取数据的类型
			//取出数据
			if (DATA_TYPE_GPIO_WORD == pattern)
				gpio_count++;
		}
	}

	return gpio_count;
}

int script_parser_mainkey_get_gpio_cfg(script_parser_t *parser, const char *main_name,
				user_gpio_set_t *gpio_cfg, int gpio_count)
{
	char   main_bkname[32];
	const char *main_char;
	script_main_key_t  *main_key = NULL;
	script_sub_key_t   *sub_key = NULL;
	user_gpio_set_t  *user_gpio_cfg = (user_gpio_set_t *)gpio_cfg;
	int32_t    i, j;
	int32_t    pattern, user_index;

	if(NULL == parser)
		return SCRIPT_PARSER_EMPTY_BUFFER;

	if(!parser->script_mod_buf)
		return SCRIPT_PARSER_EMPTY_BUFFER;

	if(main_name == NULL)
		return SCRIPT_PARSER_KEYNAME_NULL;
	//首先清空用户buffer
	memset(user_gpio_cfg, 0, sizeof(user_gpio_set_t) * gpio_count);
	//保存主键名称和子键名称，如果超过31字节则截取31字节
	main_char = main_name;
	if(_test_str_length(main_name) > 31) {
		memset(main_bkname, 0, 32);
		strncpy(main_bkname, main_name, 31);
		main_char = main_bkname;
	}

	for(i = 0; i < parser->script_main_key_count; i++) {
		main_key = SCRIPT_GET_MAIN_KEY(parser, i);
		if (strcmp(main_key->name, main_char))    //如果主键不匹配，寻找下一个主键
			continue;
		//主键匹配，寻找子键名称匹配
		user_index = 0;
		for(j=0;j<main_key->lenth;j++) {
			sub_key = SCRIPT_GET_SUB_KEY(parser, main_key, j);
			pattern    = DATA_TYPE(sub_key);             //获取数据的类型
			//取出数据
			if(DATA_TYPE_GPIO_WORD == pattern) {
				strncpy( user_gpio_cfg[user_index].name, sub_key->name, strlen(sub_key->name) + 1);
				memcpy(&user_gpio_cfg[user_index].port, parser->script_mod_buf +
								(sub_key->offset << 2), sizeof(user_gpio_set_t) - 32);
				user_index++;
				if(user_index >= gpio_count)
					break;
			}
		}
		return SCRIPT_PARSER_OK;
	}

	return SCRIPT_PARSER_KEY_NOT_FIND;
}

void script_show(script_parser_t *parser)
{
	script_main_key_t  *main_key = NULL;
	script_sub_key_t   *sub_key = NULL;
	int value[64];
	int32_t    i, j;
	int32_t    pattern, word_count;

	if(NULL == parser)
		return;

	if(!parser->script_mod_buf)
		return;

	for(i=0;i<parser->script_main_key_count;i++) {

		main_key = SCRIPT_GET_MAIN_KEY(parser, i);
		SCRIPT_INFO_PRINTF("main_key: %s, sub key cnt:%d", main_key->name,
						main_key->lenth);

		for(j = 0; j < main_key->lenth; j++) {
			sub_key = SCRIPT_GET_SUB_KEY(parser, main_key, j);
			pattern    = DATA_TYPE(sub_key);             //获取数据的类型
			word_count = (sub_key->pattern >> 0) & 0xffff;
			switch (pattern) {
			case DATA_TYPE_SINGLE_WORD:                           //单word数据类型
				value[0] = *(int *)(parser->script_mod_buf + (sub_key->offset << 2));
				SCRIPT_INFO_PRINTF("\t%s = 0x%x", sub_key->name, value[0]);
				break;

			case DATA_TYPE_STRING:     							  //字符串数据类型
				memcpy((char *)value, parser->script_mod_buf + (sub_key->offset << 2),
								word_count << 2);
				SCRIPT_INFO_PRINTF("\t%s = %s", sub_key->name, (char *)value);
				break;

			case DATA_TYPE_MULTI_WORD:
				SCRIPT_INFO_PRINTF("\t%s = (not support)", sub_key->name);
				break;
			case DATA_TYPE_GPIO_WORD:							 //多word数据类型
				{
					user_gpio_set_t  *gpio_cfg = (user_gpio_set_t *)value;
					//发现是GPIO类型，检查是否足够存放用户数据
					if(sizeof(user_gpio_set_t) > 64)
						return;
					memcpy(&gpio_cfg->port, parser->script_mod_buf +
								(sub_key->offset << 2), sizeof(user_gpio_set_t) - 32);
					SCRIPT_INFO_PRINTF("\t%s = P%c%d", sub_key->name,
									gpio_cfg->port + 0x40, gpio_cfg->port_num);
					break;
				}
			}
		}
	}

}
