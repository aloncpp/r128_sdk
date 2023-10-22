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

#ifndef __UNLOCK_BUFFER_H__
#define __UNLOCK_BUFFER_H__

// #define DLOG(fmt, arg...) printf("[%s:%d] " fmt "", __FUNCTION__, __LINE__, ##arg)
// #define MIN(A, B) (A > B ? B : A)
#ifndef MIN
#define MIN(a, b) ((a) > (b) ? (b) : (a))
#endif

typedef struct fifos {
	char *buffer; /* the buffer holding the data */
	int size;     /* the size of the allocated buffer */
	int in;       /* data is added at offset (in % size) */
	int out;      /* data is extracted from off. (out % size) */
} fifo_t;

static inline int minpowtwo(int number)
{
	int result = number;
	result |= (result >> 1);
	result |= (result >> 2);
	result |= (result >> 4);
	result |= (result >> 8);
	result |= (result >> 16);
	return (int)(result + 1) >> 1;
}

static inline int fifo_init(fifo_t *fifo, char *buffer, int size)
{
	if (size <= 0) {
		// printf("[%s]: err size\n", __FUNCTION__);
		return -1;
	}
	if ((size & (size - 1)) != 0) {
		/* size must be a power of 2 */
		// printf("[%s]: fifo size must be a power of 2\n", __FUNCTION__);
		// return -1;
		size = minpowtwo(size);
		if (size <= 0) {
			// printf("[%s]: err size\n", __FUNCTION__);
			return -1;
		}
	}
	fifo->size = size;
	fifo->buffer = buffer;
	fifo->in = 0;
	fifo->out = 0;
	return 0;
}

static inline int fifo_deinit(fifo_t *fifo)
{
	fifo->size = 0;
	fifo->buffer = NULL;
	fifo->in = 0;
	fifo->out = 0;
	return 0;
}

static inline int fifo_push(fifo_t *fifo, const char *buffer, int len)
{
	int l;
	if (fifo == NULL || fifo->buffer == NULL || buffer == NULL) {
		return -1;
	}
	if ((((fifo->in - fifo->out) & (fifo->size - 1)) + len) >= fifo->size) {
		// printf("[%s]: fifo full pkg drop\n", __FUNCTION__);
		// DLOG("fifo数据多\n");
		return -1;
	}
	len = MIN(len, fifo->size - fifo->in + fifo->out);
	/*
	 * Ensure that we sample the fifo->out index -before- we
	 * start putting bytes into the fifo.
	 */
	/* first put the data starting from fifo->in to buffer end */
	l = MIN(len, fifo->size - (fifo->in & (fifo->size - 1)));

	memcpy(fifo->buffer + (fifo->in & (fifo->size - 1)), buffer, l);
	/* then put the rest (if any) at the beginning of the buffer */
	memcpy(fifo->buffer, buffer + l, len - l);
	/*
	 * Ensure that we add the bytes to the fifo -before-
	 * we update the fifo->in index.
	 */
	fifo->in += len;
	return len;
}

static inline int fifo_pop(fifo_t *fifo, char *buffer, int len)
{
	int l;
	if (fifo == NULL || fifo->buffer == NULL || buffer == NULL) {
		return -1;
	}
	if ((int) ((fifo->in - fifo->out) & (fifo->size - 1)) < len) {
		// printf("[%s]: fifo underrun\n", __FUNCTION__);
		// DLOG("fifo数据不足\n");
		return -1;
	}
	len = MIN(len, fifo->in - fifo->out);
	/* first get the data from fifo->out until the end of the buffer */
	l = MIN(len, fifo->size - (fifo->out & (fifo->size - 1)));
	memcpy(buffer, fifo->buffer + (fifo->out & (fifo->size - 1)), l);
	/* then get the rest (if any) from the beginning of the buffer */
	memcpy(buffer + l, fifo->buffer, len - l);
	fifo->out += len;
	return len;
}

static inline int fifo_getlen(fifo_t *fifo)
{
	return (fifo->in - fifo->out) & (fifo->size - 1);
}

static inline int fifo_remain(fifo_t *fifo, int dir, int remain)//0 out pop;1 in push
{
	int len = fifo_getlen(fifo);
	if (len <= remain) {
		return len;
	}
	if (dir) {
		fifo->in -= (len - remain);//for input
	} else {
		fifo->out += (len - remain);
	}
	return fifo_getlen(fifo);
}

static inline int fifo_drain(fifo_t *fifo, int dir)//0 out pop;1 in push
{
	return fifo_remain(fifo, dir, 0);
}

#endif
