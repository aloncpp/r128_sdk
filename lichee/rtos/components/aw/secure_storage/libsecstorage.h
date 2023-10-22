#ifndef _LIBSECSTORAGE_H_
#define _LIBSECSTORAGE_H_

#include <aw_types.h>

#define CE_ALIGN_SIZE	0x20

/*
 * ss_read - read data from file and decrypt
 *
 * file :  file name
 * dst_buffer: the decrypt data
 */
int ss_read(char *file, u8* dst_buffer);

/*
 * ss_write - encrypt src_buffer with chipid and save the encrypted data to file
 *
 * file :  file name
 * src_buffer: original data which will be encrypted
 * length: original data size (MUST be 16 bytes aligned)
 */
int ss_write(char *file, u8 *src_buffer, int length);

#endif
