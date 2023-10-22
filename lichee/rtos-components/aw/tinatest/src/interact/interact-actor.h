#ifndef __INTERACT_ACTOR_H
#define __INTERACT_ACTOR_H

#include "aw_list.h"
#include "aw_types.h"
#include "interact.h"


/*
 * struction for a actor.
 *
 * @func: all cmds function pointer.
 * @reply: the buf for reply from outlog to testcase.
 * @pth: the pthread key for actor thread.
 * @lnode: list node.
 */
typedef void (*cmd_func)(void *);   // a general pointer for function.
struct actor {
    cmd_func func[cmd_cnt];
    struct list_head lnode;
};

/*
 * struction for interaction between core and outlog thread,
 * which stores the infomation of doing-interaction.
 *
 * @id: id for acting
 * @end_cnt: count for finishing actor.
 * @need_respond: which, set by outlog, means core should respond to testcase.
 * @cmd: id for cmd
 * @path: the testcase name which couse this acting.
 * @text: the text word from testcase to core/outlog.
 * @reply: the buf for relpy from outlog to testcase.
 * @rfd: a set of fd work for select.
 * @timeout: select timeout.
 * @pfd: fd between core and outlog.
 * @mutex: mutex lock while accessing this struction: acting.
 */
extern void interact_actor_init(void);
extern int interact_actor(struct acting *act);
extern struct list_head list_actor;
extern int cnt_actor;

#endif
