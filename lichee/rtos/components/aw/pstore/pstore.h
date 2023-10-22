/*
 * =====================================================================================
 *
 *       Filename:  pstore.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  02/02/2023 02:31:59 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef PSTORE_H
#define PSTORE_H

#ifndef _PSTORE_H_
#define _PSTORE_H_

struct pstore_header
{
    int valid;
    int payload_len;
    uint64_t index;
    uint64_t panic_time;
    struct timeval real_time;
    uint32_t part_size;
    uint32_t crc;
};

int pstore_init(uint32_t addr, uint32_t size);
int pstore_printf(const char *fmt, ...);
int pstore_flush(void);
int pstore_read_header(uint8_t *buffer, int size);
int pstore_read_payload(uint8_t *buffer, int size);
int pstore_erase_all(void);

#endif

#endif  /*PSTORE_H*/
