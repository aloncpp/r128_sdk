/**
 * Copyright (c) 2017, Realsil Semiconductor Corporation. All rights reserved.
 *
 */

#include <stdio.h>
#include <string.h>
#include "os_sched.h"
#include "os_pool.h"
#include "os_sync.h"
#include "os_mem.h"
#include "bt_board.h"
#include "hci_board.h"
#include "hci_process.h"

#if 0 
#include "hci_tp.h"
#include "hci_uart.h"
#include "bt_types.h"
#include "trace_app.h"



#include "build_info.h"
#include "bt_intf.h"
#endif


const BAUDRATE_MAP baudrates2[] =
{
    {0x0000701d, 115200},
    {0x0252C00A, 230400},
    {0x03F75004, 921600},
    {0x05F75004, 921600},
    {0x00005004, 1000000},
    {0x04928002, 1500000},
    {0x00005002, 2000000},
    {0x0000B001, 2500000},
    {0x04928001, 3000000},
    {0x052A6001, 3500000},
    {0x00005001, 4000000},
};
unsigned int baudrates_length = sizeof(baudrates2) / sizeof(BAUDRATE_MAP);

static uint32_t hci_tp_baudrate;

void bt_power_on(void)
{
	   
}

void bt_power_off(void)
{
	  
}


void bt_reset(void)
{
}

bool hci_board_init()
{
    return true;
}

void hci_normal_start(void)
{

}

