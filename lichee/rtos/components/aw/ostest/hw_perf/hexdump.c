/*
 * lib/hexdump.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation. See README and COPYING for
 * more details.
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>

#include "hexdump.h"

#define min(x, y)   ((x) <= (y) ? (x) : (y))
#define is_power_of_2(x)    ((x) != 0 && (((x) & ((x) - 1)) == 0))
#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))

static const char hex_asc[] = "0123456789abcdef";
static const char hex_asc_upper[] = "0123456789ABCDEF";

#define __force

static inline void bad_unaligned_access_length(void)
{
}

#define __get_unaligned_2_le(__p)                   \
    (__p[0] | __p[1] << 8)

#define __get_unaligned_2_be(__p)                   \
    (__p[0] << 8 | __p[1])

#define __get_unaligned_4_le(__p)                   \
    (__p[0] | __p[1] << 8 | __p[2] << 16 | __p[3] << 24)

#define __get_unaligned_4_be(__p)                   \
    (__p[0] << 24 | __p[1] << 16 | __p[2] << 8 | __p[3])

#define __get_unaligned_8_le(__p)                   \
    ((unsigned long long)__get_unaligned_4_le((__p+4)) << 32 |  \
     __get_unaligned_4_le(__p))

#define __get_unaligned_8_be(__p)                   \
    ((unsigned long long)__get_unaligned_4_be(__p) << 32 |      \
     __get_unaligned_4_be((__p+4)))

#define __get_unaligned_le(ptr) ({          \
        const char *__p = (const char *)ptr;    \
        uint64_t __val;                            \
        switch (sizeof(*ptr)) {                 \
            case 1:                                 \
                __val = *(const uint8_t *)__p;         \
                break;                              \
            case 2:                                 \
                __val = __get_unaligned_2_le(__p);  \
                break;                              \
            case 4:                                 \
                __val = __get_unaligned_4_le(__p);  \
                break;                              \
            case 8:                                 \
                __val = __get_unaligned_8_le(__p);  \
                break;                              \
            default:                                \
                bad_unaligned_access_length();      \
        };                                      \
        (__force __typeof__(*(ptr)))__val;      \
    })

#define get_unaligned   __get_unaligned_le

#define hex_asc_lo(x)   hex_asc[((x) & 0x0f)]
#define hex_asc_hi(x)   hex_asc[((x) & 0xf0) >> 4]

static inline char *hex_byte_pack(char *buf, uint8_t byte)
{
    *buf++ = hex_asc_hi(byte);
	*buf++ = hex_asc_lo(byte);
	return buf;
}
/**
 * hex_to_bin - convert a hex digit to its real value
 * @ch: ascii character represents hex digit
 *
 * hex_to_bin() converts one hex digit to its actual value or -1 in case of bad
 * input.
 */
static int hex_to_bin(char ch)
{
	if ((ch >= '0') && (ch <= '9'))
		return ch - '0';
	ch = tolower(ch);
	if ((ch >= 'a') && (ch <= 'f'))
		return ch - 'a' + 10;
	return -1;
}

/**
 * hex2bin - convert an ascii hexadecimal string to its binary representation
 * @dst: binary result
 * @src: ascii hexadecimal string
 * @count: result length
 *
 * Return 0 on success, -1 in case of bad input.
 */
static int hex2bin(uint8_t *dst, const char *src, size_t count)
{
	while (count--) {
		int hi = hex_to_bin(*src++);
		int lo = hex_to_bin(*src++);

		if ((hi < 0) || (lo < 0))
			return -1;

		*dst++ = (hi << 4) | lo;
	}
	return 0;
}

/**
 * bin2hex - convert binary data to an ascii hexadecimal string
 * @dst: ascii hexadecimal result
 * @src: binary data
 * @count: binary data length
 */
static char *bin2hex(char *dst, const void *src, size_t count)
{
	const unsigned char *_src = src;

	while (count--)
		dst = hex_byte_pack(dst, *_src++);
	return dst;
}

/**
 * hex_dump_to_buffer - convert a blob of data to "hex ASCII" in memory
 * @buf: data blob to dump
 * @len: number of bytes in the @buf
 * @rowsize: number of bytes to print per line; must be 16 or 32
 * @groupsize: number of bytes to print at a time (1, 2, 4, 8; default = 1)
 * @linebuf: where to put the converted data
 * @linebuflen: total size of @linebuf, including space for terminating NUL
 * @ascii: include ASCII after the hex output
 *
 * hex_dump_to_buffer() works on one "line" of output at a time, i.e.,
 * 16 or 32 bytes of input data converted to hex + ASCII output.
 *
 * Given a buffer of uint8_t data, hex_dump_to_buffer() converts the input data
 * to a hex + ASCII dump at the supplied memory location.
 * The converted output is always NUL-terminated.
 *
 * E.g.:
 *   hex_dump_to_buffer(frame->data, frame->len, 16, 1,
 *			linebuf, sizeof(linebuf), true);
 *
 * example output buffer:
 * 40 41 42 43 44 45 46 47 48 49 4a 4b 4c 4d 4e 4f  @ABCDEFGHIJKLMNO
 *
 * Return:
 * The amount of bytes placed in the buffer without terminating NUL. If the
 * output was truncated, then the return value is the number of bytes
 * (excluding the terminating NUL) which would have been written to the final
 * string if enough space had been available.
 */
int hex_dump_to_buffer(const void *buf, size_t len, int rowsize, int groupsize,
		       char *linebuf, size_t linebuflen, bool ascii)
{
	const uint8_t *ptr = buf;
	int ngroups;
	uint8_t ch;
	int j, lx = 0;
	int ascii_column;
	int ret;

	if (rowsize != 16 && rowsize != 32)
		rowsize = 16;

	if (len > rowsize)		/* limit to one line at a time */
		len = rowsize;
	if (!is_power_of_2(groupsize) || groupsize > 8)
		groupsize = 1;
	if ((len % groupsize) != 0)	/* no mixed size output */
		groupsize = 1;

	ngroups = len / groupsize;
	ascii_column = rowsize * 2 + rowsize / groupsize + 1;

	if (!linebuflen)
		goto overflow1;

	if (!len)
		goto nil;

	if (groupsize == 8) {
		const uint64_t *ptr8 = buf;

		for (j = 0; j < ngroups; j++) {
			uint64_t *output = ptr8 + j;
			ret = snprintf(linebuf + lx, linebuflen - lx,
				       "%s%16.16llx", j ? " " : "",
				       get_unaligned(output));
			if (ret >= linebuflen - lx)
				goto overflow1;
			lx += ret;
		}
	} else if (groupsize == 4) {
		const uint32_t *ptr4 = buf;

		for (j = 0; j < ngroups; j++) {
			uint32_t *output = ptr4 + j;
			ret = snprintf(linebuf + lx, linebuflen - lx,
				       "%s%8.8lx", j ? " " : "",
				       get_unaligned(output));
			if (ret >= linebuflen - lx)
				goto overflow1;
			lx += ret;
		}
	} else if (groupsize == 2) {
		const uint16_t *ptr2 = buf;

		for (j = 0; j < ngroups; j++) {
			uint16_t *output = ptr2 + j;
			ret = snprintf(linebuf + lx, linebuflen - lx,
				       "%s%4.4x", j ? " " : "",
				       get_unaligned(output));
			if (ret >= linebuflen - lx)
				goto overflow1;
			lx += ret;
		}
	} else {
		for (j = 0; j < len; j++) {
			if (linebuflen < lx + 2)
				goto overflow2;
			ch = ptr[j];
			linebuf[lx++] = hex_asc_hi(ch);
			if (linebuflen < lx + 2)
				goto overflow2;
			linebuf[lx++] = hex_asc_lo(ch);
			if (linebuflen < lx + 2)
				goto overflow2;
			linebuf[lx++] = ' ';
		}
		if (j)
			lx--;
	}
	if (!ascii)
		goto nil;

	while (lx < ascii_column) {
		if (linebuflen < lx + 2)
			goto overflow2;
		linebuf[lx++] = ' ';
	}
	for (j = 0; j < len; j++) {
		if (linebuflen < lx + 2)
			goto overflow2;
		ch = ptr[j];
		linebuf[lx++] = (isascii(ch) && isprint(ch)) ? ch : '.';
	}
nil:
	linebuf[lx] = '\0';
	return lx;
overflow2:
	linebuf[lx++] = '\0';
overflow1:
	return ascii ? ascii_column + len : (groupsize * 2 + 1) * ngroups - 1;
}

/**
 * print_hex_dump - print a text hex dump to syslog for a binary blob of data
 * @level: kernel log level (e.g. KERN_DEBUG)
 * @prefix_str: string to prefix each line with;
 *  caller supplies trailing spaces for alignment if desired
 * @prefix_type: controls whether prefix of an offset, address, or none
 *  is printed (%DUMP_PREFIX_OFFSET, %DUMP_PREFIX_ADDRESS, %DUMP_PREFIX_NONE)
 * @rowsize: number of bytes to print per line; must be 16 or 32
 * @groupsize: number of bytes to print at a time (1, 2, 4, 8; default = 1)
 * @buf: data blob to dump
 * @len: number of bytes in the @buf
 * @ascii: include ASCII after the hex output
 *
 * Given a buffer of uint8_t data, print_hex_dump() prints a hex + ASCII dump
 * to the kernel log at the specified kernel log level, with an optional
 * leading prefix.
 *
 * print_hex_dump() works on one "line" of output at a time, i.e.,
 * 16 or 32 bytes of input data converted to hex + ASCII output.
 * print_hex_dump() iterates over the entire input @buf, breaking it into
 * "line size" chunks to format and print.
 *
 * E.g.:
 *   print_hex_dump(KERN_DEBUG, "raw data: ", DUMP_PREFIX_ADDRESS,
 *		    16, 1, frame->data, frame->len, true);
 *
 * Example output using %DUMP_PREFIX_OFFSET and 1-byte mode:
 * 0009ab42: 40 41 42 43 44 45 46 47 48 49 4a 4b 4c 4d 4e 4f  @ABCDEFGHIJKLMNO
 * Example output using %DUMP_PREFIX_ADDRESS and 4-byte mode:
 * ffffffff88089af0: 73727170 77767574 7b7a7978 7f7e7d7c  pqrstuvwxyz{|}~.
 */
void print_hex_dump(const char *level, const char *prefix_str, int prefix_type,
		    int rowsize, int groupsize,
		    const void *buf, size_t len, bool ascii)
{
	const uint8_t *ptr = buf;
	int i, linelen, remaining = len;
	unsigned char linebuf[32 * 3 + 2 + 32 + 1];

	if (rowsize != 16 && rowsize != 32)
		rowsize = 16;

	for (i = 0; i < len; i += rowsize) {
		linelen = min(remaining, rowsize);
		remaining -= rowsize;

		hex_dump_to_buffer(ptr + i, linelen, rowsize, groupsize,
				   linebuf, sizeof(linebuf), ascii);

		switch (prefix_type) {
		case DUMP_PREFIX_ADDRESS:
			printf("%s%s%p: %s\n",
			       level, prefix_str, ptr + i, linebuf);
			break;
		case DUMP_PREFIX_OFFSET:
			printf("%s%s%.8x: %s\n", level, prefix_str, i, linebuf);
			break;
		default:
			printf("%s%s%s\n", level, prefix_str, linebuf);
			break;
		}
	}
}

/**
 * print_hex_dump_bytes - shorthand form of print_hex_dump() with default params
 * @prefix_str: string to prefix each line with;
 *  caller supplies trailing spaces for alignment if desired
 * @prefix_type: controls whether prefix of an offset, address, or none
 *  is printed (%DUMP_PREFIX_OFFSET, %DUMP_PREFIX_ADDRESS, %DUMP_PREFIX_NONE)
 * @buf: data blob to dump
 * @len: number of bytes in the @buf
 *
 * Calls print_hex_dump(), with log level of KERN_DEBUG,
 * rowsize of 16, groupsize of 1, and ASCII output included.
 */
void print_hex_dump_bytes(const char *prefix_str, int prefix_type,
			  const void *buf, size_t len)
{
	print_hex_dump(KERN_DEBUG, prefix_str, prefix_type, 16, 1,
		       buf, len, true);
}

void print_hex_dump_words(const char *prefix_str, int prefix_type,
                          const void *addr, unsigned int len)
{
	unsigned int i;
	const unsigned int *add = addr;

	if ((unsigned int)add & 0x03) {
		printf("addr should be align 4B!\n");
		add =  (const void *)(((unsigned int)add) & ~0x03);
	}
	len = DIV_ROUND_UP(len, 4);

	for (i = 0; i < len; i++) {
		if ((i & 0x03) == 0x0)
			printf("\n[%p]: ", add);
		printf("0x%08x ", *add++);
	}
	printf("\n");
}
