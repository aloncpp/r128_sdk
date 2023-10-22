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

#ifndef _SAVE_LOG_INTERFACE_H_
#define _SAVE_LOG_INTERFACE_H_

typedef enum {
    SAVE_ELLISYS = 0, //user can combine Hci Log and Air Log for debugging (by ellisys).
                      //hci log would not save, you can use the uart capture tool (such as ellisys)
                      //to get the hci log directly
    SAVE_BT_LOG = 1,  //user need use Hcidump Tool to save hci log;
    SAVE_WLAN_LOG = 2,//hci log would save in the Wlan Tool
} save_log_type_t;

struct save_log_iface {
    int (*write)(const void *p_data, int len);
    int (*shut_down)(void);
};

#endif /* _SAVE_LOG_INTERFACE_H_ */

