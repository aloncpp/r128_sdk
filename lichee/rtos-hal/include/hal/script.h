#ifndef _SCRIPT_H_
#define _SCRIPT_H_

#define   DATA_TYPE_SINGLE_WORD  (1)
#define   DATA_TYPE_STRING       (2)
#define   DATA_TYPE_MULTI_WORD   (3)
#define   DATA_TYPE_GPIO_WORD    (4)

#define   SCRIPT_PARSER_OK       (0)
#define   SCRIPT_PARSER_EMPTY_BUFFER    (-1)
#define   SCRIPT_PARSER_KEYNAME_NULL    (-2)
#define   SCRIPT_PARSER_DATA_VALUE_NULL (-3)
#define   SCRIPT_PARSER_KEY_NOT_FIND    (-4)
#define   SCRIPT_PARSER_BUFFER_NOT_ENOUGH (-5)

typedef struct
{
	int main_key_count;
	int version[3];
} script_head_t;

typedef struct
{
	char name[32];
	int  lenth;
	int  offset;
} script_main_key_t;

typedef struct
{
	char name[32];
	int  offset;
	int  pattern;
} script_sub_key_t;

typedef struct
{
	char name[32];
	int  port;
	int  port_num;
	int  mul_sel;
	int  pull;
	int  drv_level;
	int  data;
} user_gpio_set_t;

typedef struct
{
	char  *script_mod_buf;
	int    script_mod_buf_size;
	int    script_main_key_count;
} script_parser_t;

script_parser_t *script_parser_init(void *script_buf);
int script_parser_exit(script_parser_t *parser);
int script_parser_fetch(script_parser_t *parser, const char *main_name,
				const char *sub_name, int value[], int count);
int script_parser_subkey_count(script_parser_t *parser, const char *main_name);
int script_parser_mainkey_count(script_parser_t *parser);
int script_parser_mainkey_get_gpio_count(script_parser_t *parser,
				const char *main_name);
int script_parser_mainkey_get_gpio_cfg(script_parser_t *parser, const char *main_name,
				user_gpio_set_t *gpio_cfg, int gpio_count);
void script_show(script_parser_t *parser);

#endif
