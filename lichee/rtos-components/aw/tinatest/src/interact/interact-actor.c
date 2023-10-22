#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "tinatest.h"
#include "interact.h"
#include "interact-actor.h"

struct list_head list_actor; // list head for all actors.
int cnt_actor;           // count for all actors.

#define INIT_LNODE() \
    struct actor *act = calloc(sizeof(struct actor), 1); \
    if (NULL == act) { \
        ERROR("malloc failed \n"); \
        return -1; \
    } \
    if (NULL == list_actor.prev) \
        INIT_LIST_HEAD(&list_actor); \
    list_add_tail(&act->lnode, &list_actor); \
    cnt_actor++; \
    DEBUG("register interact: %d\n", cnt_actor);

#define ADD_CMD(CMD) \
    act->func[cmd_ ## CMD] = (cmd_func)CMD;

int interact_register(
        f_ask ask,
        f_tips tips,
        f_istrue istrue)
{
    INIT_LNODE();

    ADD_CMD(ask);
    ADD_CMD(tips);
    ADD_CMD(istrue);

    return 0;
}

/***********************************************************
 * [below] Function to do command.
 **********************************************************/
static int interact_do_ask(struct acting *acting, struct actor *act)
{
    return ((f_ask)(act->func[acting->cmd]))(acting->testcase, acting->text, acting->reply, acting->len);
}

static int interact_do_istrue(struct acting *acting, struct actor *act)
{
    int ret = ((f_istrue)(act->func[acting->cmd]))(acting->testcase, acting->text);
    if (-1 == ret)
        return -1;
    acting->reply = (ret == (int)true ? STR_TRUE : STR_FALSE);
    return 0;
}

static int interact_do_tips(struct acting *acting, struct actor *act)
{
    return ((f_tips)(act->func[acting->cmd]))(acting->testcase, acting->text);
}

/***********************************************************
 * [above] Function to do command.
 **********************************************************/

#define interact_do(CMD, acting, need_resp) \
    if (need_resp != false) { \
        acting->need_respond = true; \
    } \
    if (!act->func[acting->cmd]) \
        return -1; \
    return interact_do_ ## CMD(acting, act);

int interact_actor_do(struct acting *acting, struct actor *act)
{
    switch (acting->cmd) {
    case cmd_ask:
	   interact_do(ask, acting, true);
	   break;
    case cmd_istrue:
	   interact_do(istrue, acting, true);
	   break;
    case cmd_tips:
	   interact_do(tips, acting, false);
	   break;
    default:
        return -1;
    }

    return 0;
}

int interact_actor(struct acting *acting)
{
    struct actor *act = NULL;

    list_for_each_entry(act, &list_actor, lnode) {
	    if (interact_actor_do(acting, act) < 0) {
		    ERROR("do acting->cmd err %d\n", acting->cmd);
		    return -1;
	    }
    }

    return 0;
}

void interact_actor_init(void)
{
	INIT_LIST_HEAD(&list_actor);
	cnt_actor = 0;
}