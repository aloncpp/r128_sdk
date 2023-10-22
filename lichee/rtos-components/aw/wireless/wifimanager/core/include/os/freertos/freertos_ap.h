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

#ifndef _FREERTOS_AP_H
#define _FREERTOS_AP_H

#if __cplusplus
extern "C" {
#endif

#include <wmg_ap.h>

#define AP_BIN_FILE    "/usr/sbin/hostapd"
#define AP_CONF_FILE   "/etc/wifi/hostapd.conf"

wmg_ap_inf_object_t* ap_freertos_inf_object_register(void);

#define CMD_LEN        255

#ifndef TEMP_FAILURE_RETRY
#define TEMP_FAILURE_RETRY(expression) \
   (__extension__                                                              \
     ({ long int __result;                                                     \
        do __result = (long int) (expression);                                 \
        while (__result == -1L && errno == EINTR);                             \
        __result; }))
#endif


#if __cplusplus
};  // extern "C"
#endif

#endif  // _FREERTOS_AP_H
