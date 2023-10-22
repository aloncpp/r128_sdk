/*
 *  Copyright (C) 2019 Realtek Semiconductor Corporation.
 *
 */

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#define DEBUG
#include "app_common.h"

#include "trace_app.h"
#include "blue_cmd.h"
#include "os_task.h"

struct list_struct
{
    struct list_struct *next;
    struct list_struct *prev;
};

#define cmd_entry(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - ((size_t)&((type *)0)->member) );})

#define cmd_for_each_safe(pos, n, head) \
    for (pos = (head)->next, n = pos->next; pos != (head); \
         pos = n, n = pos->next)

#define BTA_WAKE_CODE    0x78

#define REQ_PENDING    0
#define REQ_DONE    1
struct bta_command
{
    struct list_struct list;
    void *task_handle;
    unsigned int id;
    uint16_t opcode;
    uint16_t req_status;
    int req_result;
    int argc;
    void **argv;
    uint32_t rparams[2];
};

struct bta_struct
{
    struct list_struct cmd_head;
    void *queue_lock;
    void *main_task;
    void *event_handle;
    bcmd_handler_t command_handler;
    int initialized;
    unsigned int ref;
};

static void *bta_struct_task_handle_get()
{
    void *task_handle = NULL;

    if (os_task_handle_get(&task_handle) == true)
    {
        return task_handle;
    }
    else
    {
        return NULL;
    }
}


static inline void cmd_add_tail(struct list_struct *_new, struct list_struct *head)
{
    struct list_struct *prev, *next;

    prev = head->prev;
    next = head;

    next->prev = _new;
    _new->next = next;
    _new->prev = prev;
    prev->next = _new;
}

static inline void cmd_del(struct list_struct *entry)
{
    struct list_struct *prev, *next;

    prev = entry->prev;
    next = entry->next;

    next->prev = prev;
    prev->next = next;

    entry->next = 0;
    entry->prev = 0;
}

static void cmd_del_by_cmd(struct list_struct *head, struct bta_command *c)
{
    struct list_struct *n, *pos;
    struct bta_command *cmd;

    cmd_for_each_safe(pos, n, head)
    {
        cmd = cmd_entry(pos, struct bta_command, list);
        if (cmd == c)
        {
            cmd_del(pos);
            break;
        }
    }
}

struct bta_struct *bta_struct_ref(struct bta_struct *bta)
{
    if (!bta)
    {
        return NULL;
    }

    bta->ref++;

    return bta;
}

struct bta_struct *bta_struct_init(void *main_task, bcmd_handler_t handler,
                                   void *event_handle)
{
    struct bta_struct *bta = NULL;
    struct list_struct *head = NULL;

    bta = malloc(sizeof(*bta));
    if (!bta)
    {
        pr_err("Couldn't alloc bta structure");
        return NULL;
    }
    memset(bta, 0, sizeof(*bta));

    bta->main_task = main_task;
    bta->event_handle = event_handle;
    pr_err("main process %p", main_task);

    head = &bta->cmd_head;
    head->next = head;
    head->prev = head;

    bta->command_handler = handler;

    /* Initialize lock */
    if (!osif_mutex_create(&bta->queue_lock))
    {
        bta->queue_lock = NULL;
        free(bta);
        return NULL;
    }

    bta->ref = 1;
    bta->initialized = 1;

    return bta;
}

/* NOTE: make sure the process thread exit before destroying bta */
void bta_struct_destroy(struct bta_struct *bta)
{
    struct list_struct *head;
    struct list_struct *n, *pos;
    struct bta_command *cmd;

    if (!bta)
    {
        return;
    }

    pr_info("bta->ref %u", bta->ref);

    bta->initialized = 0;
    bta->main_task = NULL;
    bta->event_handle = NULL;

    osif_mutex_take(bta->queue_lock, 0xFFFFFFFFUL);
    head = &bta->cmd_head;
    cmd_for_each_safe(pos, n, head)
    {
        cmd = cmd_entry(pos, struct bta_command, list);
        cmd_del(pos);
        cmd->req_status = REQ_DONE;
        cmd->req_result = -1;
        if (!osif_task_signal_send(cmd->task_handle, 0))
        {
            pr_err("Can't send signal to calling task");
        }
        /* The calling task will free the cmd */
        /* free(cmd); */
    }
    osif_mutex_give(bta->queue_lock);

    osif_mutex_delete(bta->queue_lock);

    free(bta);
}

void bta_struct_unref(struct bta_struct *bta)
{
    if (!bta)
    {
        return;
    }

    bta->ref--;

    if (bta->ref > 0)
    {
        return;
    }

    bta_struct_destroy(bta);
}

int __bta_submit_command_wait(struct bta_struct *bta, uint16_t opcode,
                              int argc, void **argv)
{
    struct bta_command *cmd;
    static uint32_t cmd_id = 1;
    int result = -1;
    uint8_t event = EVENT_CUSTOM_OP;

    if (!bta || !bta->initialized)
    {
        return -1;
    }

    if (!bta->main_task)
    {
        pr_err("Please set main task");
        return -1;
    }

    /* Running in the bta mainloop */
    if (bta->main_task == bta_struct_task_handle_get())
    {
        pr_info("Process command directly");
        if (bta->command_handler)
        {
            result = bta->command_handler(opcode, argc, argv);
        }
        else
        {
            result = -1;
        }
        return result;
    }

    cmd = malloc(sizeof(*cmd));
    if (!cmd)
    {
        return -1;
    }

    cmd->opcode = opcode;
    cmd->argc = argc;
    cmd->argv = argv;
    cmd->req_status = REQ_PENDING;
    cmd->req_result = 0;
    cmd->task_handle = bta_struct_task_handle_get();
    memset(cmd->rparams, 0, sizeof(cmd->rparams));
    if (cmd_id < 1)
    {
        cmd_id = 1;
    }
    cmd->id = cmd_id++;
    (cmd->list).prev = &cmd->list;
    (cmd->list).next = &cmd->list;

    osif_mutex_take(bta->queue_lock, 0xFFFFFFFFUL);
    result = 0;

    /* Avoid bta to be released */
    bta_struct_ref(bta);

    cmd_add_tail(&cmd->list, &bta->cmd_head);
    /* Wake up the main process */
    if (bta->event_handle)
    {
        if (!osif_msg_send(bta->event_handle, &event, 0))
        {
            pr_err("Couldn't send event to main process");
            result = -1;
        }
    }
    else
    {
        pr_err("Event handle is nil");
        result = -1;
    }

    if (result < 0)
    {
        cmd_del_by_cmd(&bta->cmd_head, cmd);
        osif_mutex_give(bta->queue_lock);
        goto cmd_free;
    }

    result = -1;
    osif_mutex_give(bta->queue_lock);

    while (1)
    {
        if (!osif_task_signal_recv(NULL, 2000))
        {
            pr_err("receive task signal error");
            osif_mutex_take(bta->queue_lock, 0xFFFFFFFFUL);
            cmd_del_by_cmd(&bta->cmd_head, cmd);
            osif_mutex_give(bta->queue_lock);
            result = -1;
            break;
        }

        osif_mutex_take(bta->queue_lock, 0xFFFFFFFFUL);
        if (cmd->req_status == REQ_DONE)
        {
            result = cmd->req_result;
            osif_mutex_give(bta->queue_lock);
            break;
        }
        osif_mutex_give(bta->queue_lock);
    }

cmd_free:
    free(cmd);

    bta_struct_unref(bta);

    return result;
}

/* This function runs in main process context */
void bta_run_command(struct bta_struct *bta)
{
    struct bta_command *cmd = NULL;
    int result;

    pr_debug();

    if (bta->main_task != bta_struct_task_handle_get())
    {
        APP_PRINT_INFO0("Run in the wrong context: %p");
        return;
    }

    osif_mutex_take(bta->queue_lock, 0xFFFFFFFFUL);

    if ((bta->cmd_head).next != &bta->cmd_head)
    {
        struct list_struct *head = &bta->cmd_head;

        cmd = cmd_entry((head->next), struct bta_command, list);
        cmd_del(&cmd->list);
        /* Should never block long time in command handler */
        if (bta->command_handler)
            result = bta->command_handler(cmd->opcode, cmd->argc,
                                          cmd->argv);
        else
        {
            result = -1;
        }
        cmd->req_status = REQ_DONE;
        cmd->req_result = result;

        /* if task sends signal failed, the calling task will be
         * timeout
         */
        if (!osif_task_signal_send(cmd->task_handle, 0))
        {
            APP_PRINT_INFO0("Can't send signal to calling task");
        }
    }

    osif_mutex_give(bta->queue_lock);

    /* No longer access cmd because the cmd will be freed by the user
     * task
     */
}
