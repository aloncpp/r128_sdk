/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _FREERTOS_MONITOR_H
#define _FREERTOS_MONITOR_H

#if __cplusplus
extern "C" {
#endif

#include "nl80211.h"
#include <wmg_common.h>

//这个结构体是内核定义的，不过头文件没有，暂时性自己定义
struct ucred {
	__u32 pid;
	__u32 uid;
	__u32 gid;
};

#define MON_CONF_PATH   "/etc/wifi/wifi_monitor.conf"

wmg_monitor_inf_object_t* monitor_freertos_inf_object_register(void);

#if __cplusplus
};  // extern "C"
#endif

#endif  // _FREERTOS_MONITOR_H
