#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include <semphr.h>
#include "console.h"
#include "tinatest.h"
#include "outlog.h"
#include "../interact/interact-actor.h"


static int serial_before_all(struct list_head *TASK_LIST)
{
    struct tt_struct *tt;
    printf("\n");
    printf("============================");
    printf(" tasks list ");
    printf("============================\n");
    list_for_each_entry(tt, TASK_LIST, node)
        printf("%s\n", tt->name);
    printf("============================");
    printf("    end     ");
    printf("============================\n");

    return 0;
}

static int serial_after_one_end(struct tt_struct *tt)
{
    printf("\n");
    printf("----------------------------");
    printf(" %s ", tt->name);
    printf("----------------------------\n");

    if (tt->result == 0) {
	    printf("----------------------------");
	    printf(" pass ");
	    printf("----------------------------\n");
    }
    else {
	    printf("----------------------------");
	    printf(" fail ");
	    printf("----------------------------\n");
    }

    printf("----------------------------");
    printf(" end ");
    printf("----------------------------\n");
    printf("\n");

    return 0;
}

static int serial_after_all(struct list_head *TASK_LIST)
{
    int result = 0;
    struct tt_struct *tt;

    printf("\n");
    printf("============================");
    printf(" tasks result ");
    printf("============================\n");
    struct tt_struct *task = NULL;
    list_for_each_entry(tt, TASK_LIST, node) {
	    if (tt->result < 0) {
		printf("FAILED (failed testcase: %s  with %d back)\n", tt->name, tt->result);
		result = -1;
	    }

    }
    if (result == 0)
	    printf("PASS\n");
    printf("============================");
    printf("     end      ");
    printf("============================\n");

    return 0;
}

void vPortGetCharFromQueue(char * cRxedChar);
void vPortGetChar(void);
static int serial_interact_common(const char *testcase, const char *ask, char *reply, int len)
{
    int i = 0, ret = 0;

    printf("\n========================");
    printf(" Testcase Information ");
    printf(" ========================\n");
    printf("<%s>:\n", testcase);
    printf("%s\n", ask);

    if (len != 0 && reply != NULL) {
	    printf("uart : not support read string from console.\n");
	    reply[0] = '\0';
    }

    printf("================================");
    printf(" END ");
    printf("==================================\n\n");

    return 0;
}

static int serial_ask(const char *testcase, const char *ask,
        char *reply, int len)
{
    return serial_interact_common(testcase, ask, reply, len);
}

static int serial_tips(const char *testcase, const char *tips)
{
    return serial_interact_common(testcase, tips, NULL, 0);
}

static int serial_istrue(const char *testcase, const char *ask)
{
    int ret = -1;
    int len = strlen(ask);
    char *buf = calloc(1, len + 10);
    if (buf == NULL)
        goto out;

    if (buf[len - 1] == '\n')
        buf[len - 1] = '\0';
    sprintf(buf, "%s [Y|n]: ", ask);

    char resp[10] = {0};
    for (len = 0; len < 20; len++) {
        serial_interact_common(testcase, buf, resp, 10);
        switch (resp[0]) {
        case 'Y':
        case 'y':
        case '\0':
            ret = true;
            goto out;
        case 'N':
        case 'n':
            ret = false;
            goto out;
        default:
            continue;
        }
    }

out:
    free(buf);
    return ret;
}

int serial_module_init(void) {
	int ret = -1;

	outlog_register(
			serial_before_all,
			NULL,
			serial_after_one_end,
			serial_after_all);

	interact_register(
			serial_ask,
			serial_tips,
			serial_istrue);

	return 0;
}
