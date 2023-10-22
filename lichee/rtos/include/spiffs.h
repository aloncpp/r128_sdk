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

#ifndef __SPIFFS_H_
#define __SPIFFS_H_

#include <stdbool.h>
#include <sys/types.h>
#include <stdint.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Register and mount SPIFFS to VFS with given path prefix.
 *
 * @param source: The partition path, tha must has prefix '/dev/'.
 *        target: The target path to mount.
 *        format: Whether format when mount failed.
 *
 * @return
 *          - OK                  if success
 *          - ERR_NO_MEM          if objects could not be allocated
 *          - ERR_INVALID_STATE   if already mounted or partition is encrypted
 *          - ERR_NOT_FOUND       if partition for SPIFFS was not found
 *          - FAIL                if mount or format fails
 */
int spiffs_mount(const char *source, const char *target, bool format);

/**
 * Unregister and unmount SPIFFS from VFS
 *
 * @param target: The target path had mounted.
 *
 * @return
 *          - OK if successful
 *          - ERR_INVALID_STATE already unregistered
 */
int spiffs_umount(const char *target);

/**
 * Check if SPIFFS is mounted
 *
 * @param blkpart  Optional, label of the partition to check.
 *                         If not specified, first partition with subtype=spiffs is used.
 *
 * @return  
 *          - true    if mounted
 *          - false   if not mounted
 */
bool spiffs_mounted(const char *blkpart);

/**
 * Format the SPIFFS partition
 *
 * @param blkpart  Optional, label of the partition to format.
 *                         If not specified, first partition with subtype=spiffs is used.
 * @return  
 *          - OK      if successful
 *          - FAIL    on error
 */
int spiffs_format(const char *blkpart);

/**
 * Get information for SPIFFS
 *
 * @param blkpart           Optional, label of the partition to get info for.
 *                                  If not specified, first partition with subtype=spiffs is used.
 * @param[out] total_bytes          Size of the file system
 * @param[out] used_bytes           Current used bytes in the file system
 *
 * @return  
 *          - OK                  if success
 *          - ERR_INVALID_STATE   if not mounted
 */
int spiffs_info(const char *blkpart, size_t *total_bytes, size_t *used_bytes);

#ifdef __cplusplus
}
#endif

#endif /* __SPIFFS_H_ */
