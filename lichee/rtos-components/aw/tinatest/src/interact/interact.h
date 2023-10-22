/*
 * It's external *.h, including apis for testcase/tinatest/outlog
 */
#ifndef __INTERACT_H
#define __INTERACT_H

#define MAX_PATH 256
#define MAX_TEXT 1024
#define MAX_FPATH 512
#define MAX_TEXT_DFPATH (MAX_TEXT - MAX_FPATH)
#define STR_TRUE "true"
#define STR_FALSE "false"

enum cmd {
	cmd_tips = 0,
	cmd_ask,
	cmd_istrue,
	cmd_cnt,
	cmd_err,
};
struct acting {
	enum cmd cmd;
	const char *testcase;
	char text[MAX_TEXT];
	int need_respond;
	char *reply;
	int len;
};
/*
 * api for TinaTest
 */
struct interact_data_t *interact_init(void);
int interact_exit(struct interact_data_t *data);

/*
 * api for outlog(actor)
 */
typedef int (*f_ask) (const char *testcase, const char *ask, char *reply, int len);
typedef int (*f_tips) (const char *testcase, const char *tips);
typedef int (*f_istrue) (const char *testcase, const char *tips);
int interact_register(
        f_ask ask,
        f_tips tips,
        f_istrue istrue);

#endif
