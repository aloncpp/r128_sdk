// Copyright 2015-2017 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// start from esp: https://github.com/espressif/esp-idf

#define pr_fmt(fmt) "SPIFFS: " fmt

#include <FreeRTOS.h>
#include <blkpart.h>
#include <spiffs.h>
#include <vfs.h>
#include <semphr.h>
#include <awlog.h>

#include "spiffs_api.h"

void spiffs_api_lock(spiffs *fs)
{
    xSemaphoreTake(((spiffs_t *)(fs->user_data))->lock, portMAX_DELAY);
}

void spiffs_api_unlock(spiffs *fs)
{
    xSemaphoreGive(((spiffs_t *)(fs->user_data))->lock);
}

s32_t spiffs_api_read(spiffs *fs, uint32_t addr, uint32_t size, uint8_t *dst)
{
    spiffs_t *efs = fs->user_data;
    struct part *part = (struct part *)efs->part;
    int ret;

    ret = blkpart_read(part, addr, size, dst);
    if (ret != size) {
        pr_err("failed to read addr %08x, size %08x, ret %d\n", addr, size, ret);
        return -1;
    }
    return 0;
}

s32_t spiffs_api_write(spiffs *fs, uint32_t addr, uint32_t size, uint8_t *src)
{
    spiffs_t *efs = fs->user_data;
    struct part *part = (struct part *)efs->part;
    int ret;

    ret = blkpart_write(part, addr, size, src);
    if (ret != size) {
        pr_err("failed to write addr %08x, size %08x, ret %d\n", addr, size, ret);
        return -1;
    }
    return 0;
}

s32_t spiffs_api_erase(spiffs *fs, uint32_t addr, uint32_t size)
{
    spiffs_t *efs = fs->user_data;
    struct part *part = (struct part *)efs->part;
    int ret;

    ret = blkpart_erase(part, addr, size);
    if (ret) {
        pr_err("failed to erase addr %08x, size %08x, ret %d\n", addr, size, ret);
        return -1;
    }
    return 0;
}

void spiffs_api_check(spiffs *fs, spiffs_check_type type, 
                            spiffs_check_report report, uint32_t arg1, uint32_t arg2)
{
    static const char * spiffs_check_type_str[3] = {
        "LOOKUP",
        "INDEX",
        "PAGE"
    };

    static const char * spiffs_check_report_str[7] = {
        "PROGRESS",
        "ERROR",
        "FIX INDEX",
        "FIX LOOKUP",
        "DELETE ORPHANED INDEX",
        "DELETE PAGE",
        "DELETE BAD FILE"
    };

    if (report != SPIFFS_CHECK_PROGRESS) {
        pr_err("CHECK: type:%s, report:%s, %x:%x\n",
                spiffs_check_type_str[type],
                spiffs_check_report_str[report], arg1, arg2);
    } else {
        pr_debug("CHECK PROGRESS: report:%s, %x:%x\n",
                spiffs_check_report_str[report], arg1, arg2);
    }
}
