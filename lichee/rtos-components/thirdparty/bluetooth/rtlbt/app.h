/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_H_
#define _APP_H_

#include <stdint.h>

int app_init(void);
int bta_submit_command_wait(uint16_t opcode, int argc, void **argv);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_H_ */

