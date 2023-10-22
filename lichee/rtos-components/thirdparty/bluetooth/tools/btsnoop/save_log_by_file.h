/******************************************************************************
 *
 *  Copyright (C) 2014 Google, Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

#ifndef _SAVE_LOG_BY_FILE_H_
#define _SAVE_LOG_BY_FILE_H_

#include "save_log_interface.h"

int file_save_log_start_up(const char *path);
int file_save_log_shut_down(void);
int file_save_log_write(const void *p_data, int len);
void file_save_log_flush(void);
const struct save_log_iface *file_save_log_iface_create(const char *path, uint8_t o_sync);

#endif /* _SAVE_LOG_BY_FILE_H_ */
