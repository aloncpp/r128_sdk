#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tinatest.h"
#include "aw_types.h"
#include "interact-actor.h"

/********************************************************
 * [above]: library for c api
 * [below]: c/c++ api for testcase
 *******************************************************/

int ttips(const char *tips){
	int len;
	int ret;
	char name[TT_NAME_LEN_MAX+1];
	struct acting *acting = malloc(sizeof(*acting));
	if(!acting)
		return -1;

	len = min(strlen(tips), MAX_TEXT);
	acting->cmd = cmd_tips;
	tname(name);
	acting->testcase = name;//current_tt->name;
	strncpy(acting->text, tips, len);
	acting->text[len] = '\0';

	ret = interact_actor(acting);
exit:
	free(acting);
	return ret;
}

int task(const char *ask, char *reply, int length){
	int len;
	int ret;
	char name[TT_NAME_LEN_MAX+1];
	struct acting *acting = malloc(sizeof(*acting));
	if(!acting)
		return -1;

	len = min(strlen(ask), MAX_TEXT);
	acting->cmd = cmd_ask;
	tname(name);
	acting->testcase = name;//current_tt->name;
	acting->reply = reply;
	acting->len = length;
	strncpy(acting->text, ask, len);
	acting->text[len] = '\0';

	ret = interact_actor(acting);
exit:
	free(acting);
	return ret;
};

int ttrue(const char *tips){
	int len;
	int ret;
	char name[TT_NAME_LEN_MAX+1];
	struct acting *acting = malloc(sizeof(*acting));
	if(!acting)
		return -1;

	len = min(strlen(tips), MAX_TEXT);
	acting->cmd = cmd_istrue;
	tname(name);
	acting->testcase = name;//current_tt->name;
	strncpy(acting->text, tips, len);
	acting->text[len] = '\0';

	if (interact_actor(acting) < 0) {
		ERROR("interact_actor err\n");
		ret = -1;
		goto exit;
	}

	if (!strncmp(acting->reply, STR_TRUE, sizeof(STR_TRUE)))
		ret = 0;
	else
		ret = -1;
exit:
	free(acting);
	return ret;
};

struct interact_data_t{
	void *Reserved;
};

struct interact_data_t *interact_init(void){
	interact_actor_init();
	return (struct interact_data_t *)0x10;
}

int interact_exit(struct interact_data_t *data){
	
	return 0;
}

