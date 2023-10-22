/*
 * Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of XRADIO TECHNOLOGY CO., LTD. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SDD_H_
#define _SDD_H_

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef CONFIG_DRIVER_XR829
#define SYS_SDD_FILE     "/reserve/sdd_xr829.bin"
#elif defined(CONFIG_DRIVER_R128)
#define SYS_SDD_FILE     "/data/sys_sdd_40M.bin"
#else
#define SYS_SDD_FILE     "/data/sys_sdd_24M.bin"
#endif

//#include "sys/endian.h"

#define SDD_MAX_SIZE                        (1024)

/* Special Section */

//Element ID
#define SDD_VERSION_ELT_ID                  0xC0
#define SDD_SECTION_HEADER_ELT_ID           0xC1
#define SDD_END_OF_CONTENT_ELT_ID           0xFE
#define SDD_LAST_SECT_ID                    0xFF

/* WLAN BLE Static Section */

//Section ID
#define SDD_WLAN_STATIC_SECT_ID             0x00

//Element ID
#define SDD_REFERENCE_FREQUENCY_ELT_ID      0xC5
#define SDD_COUNTRY_INFO_ELT_ID             0xDF
#define SDD_WAKEUP_MARGIN_ELT_ID            0xE9
#define SDD_PTA_CFG_ELT_ID                  0xEB


/* WLAN Dynamic Section */

//Section ID
#define SDD_WLAN_DYNAMIC_SECT_ID            0x01

//Element ID
#define SDD_XTAL_TRIM_ELT_ID                0xC9
#define SDD_REF_CLK_OFFSET_ELT_ID           0xCA


/* WLAN PHY Section */

//Section ID
#define SDD_WLAN_PHY_SECT_ID                0x03

//Element ID
#define SDD_USER_POWER_2G4_ELT_ID           0xB0
#define SDD_USER_POWER_5G_ELT_ID            0xB1
#define SDD_STD_RSSI_COMP_ELT_ID            0xB2


/* BT BLE PHY Section */

//Section ID
#define SDD_BT_BLE_PHY_SECT_ID              0x13

//Element ID
#define SDD_POWER_BO_VOLT_2G4_ELT_ID        0x20
#define SDD_POWER_BO_TEMP_2G4_ELT_ID        0x22

/* WLAN BT BLE ANT Section */

//Section ID
#define SDD_WLAN_BT_BLE_ANT_SECT_ID         0x06

//Element ID
#define SDD_ANT_MOD_ELT_ID                  0x11

struct sdd_ie {
	uint8_t id;
	uint8_t length;
	uint8_t data[];
};

struct sdd_sec_header {
	uint8_t type;
	uint8_t ie_length;
	uint8_t id;
	uint8_t major_ver;
	uint8_t minor_ver;
	uint8_t reserve;
	uint16_t sec_length;
};

struct sdd {
	uint32_t size;
	uint32_t *data;
};

struct sdd_ant_mod_elt_value {
	uint8_t IsAntDynamicSw;
	uint8_t Wlan_ant_cfg;
	uint8_t Bt_ant_cfg;
	uint8_t Ble_ant_cfg;
	uint8_t TxPwrDiversityInd_cfg;
	uint8_t reserved;
};

uint32_t sdd_request(struct sdd *sdd);
void sdd_release(struct sdd *sdd);
int sdd_save(struct sdd *sdd);
struct sdd_ie *sdd_find_ie(struct sdd *sdd, int isec, int ies);
int sdd_set_ie(struct sdd *sdd, int isec, int ies, uint8_t *data);
int sdd_get_file(int print_type);
int sdd_dump_file(int print_type);

#endif /* _SDD_H_ */
