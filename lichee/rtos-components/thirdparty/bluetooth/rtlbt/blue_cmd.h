/*
 *  Copyright (C) 2019 Realtek Semiconductor Corporation.
 *
 */
#ifndef __BLUE_CMD_H
#define __BLUE_CMD_H
#include <stdio.h>
#include <stdint.h>

#if __cplusplus
extern "C" {
#endif

#define EVENT_CUSTOM_OP 0x7F

    struct bta_struct;

    typedef int (*bcmd_handler_t)(uint16_t opcode, int argc, void **argv);

    struct bta_struct *bta_struct_init(void *main_task, bcmd_handler_t handler,
                                       void *event_handle);
    int __bta_submit_command_wait(struct bta_struct *bta, uint16_t opcode,
                                  int argc, void **argv);
    void bta_run_command(struct bta_struct *bta);
    void bta_struct_unref(struct bta_struct *bta);


#if __cplusplus
};
#endif

#endif
