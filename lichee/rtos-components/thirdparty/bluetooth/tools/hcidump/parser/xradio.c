/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2004-2011  Marcel Holtmann <marcel@holtmann.org>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <errno.h>

#include "parser.h"

void xradio_dump(int level, struct frame *frm)
{
    printf("[xrdump]fdi:xradio_dump null\n");
}

#if 0
void vendor_dump(int level, struct frame *frm, uint8_t event_len)
{
    uint8_t param0    = get_u8(frm);
    unsigned char data_buffer[event_len];
    memset(data_buffer, 0 ,event_len);
    if (param0 != 0x07) {
        printf("xrdump[fdi]:vendor_dump unknow param0!\n");
        return;
    }
    memcpy(data_buffer, frm->ptr, (event_len - 1));
    printf("%s", data_buffer);
}
#endif
