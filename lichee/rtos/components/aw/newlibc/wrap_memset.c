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

#include <stdint.h>
#include <stddef.h>

#define MEM_UNALIGNED(addr)	((uint32_t)(unsigned long)(addr) & 0x3)

__attribute__ ((__optimize__ ("-O2")))
void *memset(void *s, int c, size_t n)
{
	uint8_t *ptr = (uint8_t *)s;

	while (MEM_UNALIGNED(ptr)) {
		if (n--) {
			*ptr++ = (uint8_t)c;
		} else {
			return s;
		}
	}

	if (n >= 4) {
		uint32_t *ptr_aligned = (uint32_t *)ptr;
		uint32_t val = c & 0xff;

		val |= (val << 8);
		val |= (val << 16);

		while (n >= 4 * 4) {
			*ptr_aligned++ = val;
			*ptr_aligned++ = val;
			*ptr_aligned++ = val;
			*ptr_aligned++ = val;
			n -= 4 * 4;
		}

		while (n >= 4) {
			*ptr_aligned++ = val;
			n -= 4;
		}

		ptr = (uint8_t *)ptr_aligned;
	}

	while (n--) {
		*ptr++ = (uint8_t)c;
	}

	return s;
}
