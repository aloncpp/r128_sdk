#ifndef _MD5_H
#define _MD5_H

typedef unsigned int __u32;

struct MD5Context {
	__u32 buf[4];
	__u32 bits[2];
	union {
		unsigned char in[64];
		__u32 in32[16];
	};
};

/*
 * Calculate and store in 'output' the MD5 digest of 'len' bytes at
 * 'input'. 'output' must have enough space to hold 16 bytes.
 */
void md5 (unsigned char *input, int len, unsigned char output[16]);

#endif /* _MD5_H */
